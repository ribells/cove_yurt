/* =============================================================================
	File: data.cpp

 =============================================================================== */

#include <string.h>
#include "xmlParser.h"
//#include "netcdf.h"
#include "data_mgr.h"
#include "utility.h"
#include "timer.h"
#include "web/web_file.h"

#include <algorithm>

/* =============================================================================
 =============================================================================== */

void setRecacheWebData(bool bReload)
{
	m_RecacheWebData = bReload;
}

/* =============================================================================
 =============================================================================== */
void resetPerfValues()
{
	g_Timers.resetPerfData();
}

/* =============================================================================
 =============================================================================== */
double getPerfValue(int i)
{
	return g_Timers.getPerfTimer((TimerType) i);
}

/* =============================================================================
 =============================================================================== */
void setPerfValue(int i, double d)
{
	g_Timers.setPerfTimer((TimerType) i, d);
}

/* =============================================================================
 =============================================================================== */
void setCOVEPaths(string strServerPath, string strLocalPath)
{
	g_Env.m_COVEServerPath = strServerPath;
	g_Env.m_LocalCachePath = strLocalPath;
}

/* =============================================================================
 =============================================================================== */
string getCOVELocalPath(string strPath)
{
	return getLocalPath(strPath);
}

/* =============================================================================
 =============================================================================== */
void cleanWebFile(string strPath)
{
	if (!m_RecacheWebData) 
		return;

	string	strLocal = getLocalPath(strPath);
	if (strLocal != strPath) 
		delfile(strLocal);
}

/* =============================================================================
 =============================================================================== */
string cacheCOVEFile(string strPath)
{
	cleanWebFile(strPath);
	return cacheWebFile(strPath);
}

/* =============================================================================
 =============================================================================== */
bool writePerfFile(string strPath)
{
	string	strLocalPath = getLocalPath(strPath);

	//  build XML tree
	XMLNode xBase, xn0, xn1, xn2;
	char	buf[256];

	xBase = XMLNode::createXMLTopNode("");
	xn0 = xBase.addChild("xml", TRUE);
	xn0.addAttribute("version", "1.0");
	xn0.addAttribute("encoding", "UTF-8");
	xn0 = xBase.addChild("cove_system");
	xn1 = xn0.addChild("performance");

	string	strTags[NUM_PERF] = { "gpu", "cpu", "io", "net", "wf_io", "wf_net", "vis_net" };
	for (int i = 0; i < NUM_PERF; i++)
	{
		xn2 = xn1.addChild(strTags[i].c_str());

		double	time = getPerfValue(i);
		sprintf(buf, "%.2lf", time);
		xn2.addText(buf);
	}

	//  write the structure to a file
	int		nCnt;
	char	*pch = xBase.createXMLString(true, &nCnt);
	writefile(strLocalPath, pch, nCnt);
	free(pch);
	return true;
}

/* =============================================================================
 =============================================================================== */

bool CData::setLastError(string strError) const
{
	g_Timers.stopPerfTimer(CPU_PERF_TIMER);
	g_Timers.stopPerfTimer(IO_PERF_TIMER);
	g_Timers.stopPerfTimer(NET_PERF_TIMER);
	int ilen = min((int)strError.size(), MAX_PATH-1);
	memcpy(m_LastError, strError.c_str(), ilen);
	m_LastError[ilen] = '\0';
	cout << ("CData Error: " + strError);
	return false;
}

/* =============================================================================
 =============================================================================== */
string CData::getLastError() const
{
	string	strRet = m_LastError;
	*m_LastError = '\0';
	return strRet;
}

/* =============================================================================
 =============================================================================== */
int CData::getMemSize(int iType) const
{
	int iMemSize = 0;
	for (int i = 0; i < getVarCnt(); i++)
	{
		if (iType == -1 || iType == getVarType(i))
			iMemSize += getVarSize(i) * (getVarIsDData(i) ? sizeof(double) : sizeof(float));
	}

	return iMemSize;
}

/* =============================================================================
 =============================================================================== */
string CData::getHeader() const
{
	ostringstream	s;
	s << endl << "TOTAL SIZE: " << davesCommas(getMemSize() / 1000) << " kb" << endl;
	for (int i = 0; i < m_Att.size(); i++) 
		s << "Attribute " << i << ": " << m_Att[i].getString() << endl;
	s << endl;
	for (int i = 0; i < m_Dim.size(); i++)
		s << " Dim " << i << " " << m_Dim[i].getName() << "(" << m_Dim[i].getSize() << ")" << endl;
	s << endl;
	for (int i = 0; i < m_Var.size(); i++)
	{
		if (m_Var[i].getDimCnt() == 0) 
			continue;
		s << " Var " << i << ": " << m_Var[i].getName() << "(" << m_Var[i].getType() << ")";
		s << " dims[";
		for (int j = 0; j < m_Var[i].getDimCnt(); j++)
		{
			if (j) s << ",";
			s << m_Var[i].getDim(j);
		}

		s << "] ";
		s << getVarSize(i) << " pnts:(" << m_Var[i].getMin() << "," << m_Var[i].getMax() << ")" << endl;
		for (int j = 0; j < m_Var[i].getAttCnt(); j++)
			s << "   Attribute " << j << ": " << m_Var[i].getAtt(j).getString() << endl;
	}

	return s.str();
}

/* =============================================================================
 =============================================================================== */
void CData::setExtents()
{
	for (int i = 0; i < m_Var.size(); i++)
		if (m_Var[i].isLoaded()) 
			m_Var[i].updateMinMax();
}

/* =============================================================================
 =============================================================================== */
string CData::getGridExtents() const
{
	vector<double>	time = getTimeMinMax();
	vector<double>	pos = getPosMinMax();

	ostringstream	s;
	s << endl << "  Grid Variables: " << (getTimeVar() >= 0 ? getVar(getTimeVar()).getName() : "NO TIME") << ", " <<
		(getLatVar() >= 0 ? getVar(getLatVar()).getName() : "NO LAT") << ", " <<
			(getLonVar() >= 0 ? getVar(getLonVar()).getName() : "NO LONG") << ", " <<
				(getDepthVar() >= 0 ? getVar(getDepthVar()).getName() : "NO DEPTH") << endl;
	s << "  min pos: " << pos[0] << ", " << pos[1] << ", " << pos[2] << endl;
	s << "  max pos: " << pos[3] << ", " << pos[4] << ", " << pos[5] << endl;

	string	strStart = getNiceDateTimeString(time[0]);
	string	strFinish = getNiceDateTimeString(time[1]);
	s << "  time:  " << strStart << " to " << strFinish << endl;

	return s.str();
}

/* =============================================================================
 =============================================================================== */
vector<double> CData::getTimeMinMax() const
{
	vector<double>	time;
	for (int i = 0; i < 2; i++) 
		time.push_back(0);

	bool	bFirst = true;
	for (int i = 0; i < m_Var.size(); i++)
	{
		if (m_Var[i].getType() != DATA_TIME)
			continue;
		if (bFirst || time[0] > m_Var[i].getMin()) 
			time[0] = m_Var[i].getMin();
		if (bFirst || time[1] < m_Var[i].getMax()) 
			time[1] = m_Var[i].getMax();
		bFirst = false;
	}

	return time;
}

/* =============================================================================
 =============================================================================== */
vector<double> CData::getPosMinMax() const
{
	vector<double>	pos;
	for (int i = 0; i < 6; i++) 
		pos.push_back(0);

	bool	bFirst[3] = { true, true, true };
	int		type[3] = { DATA_LAT, DATA_LON, DATA_DEPTH };
	for (int i = 0; i < m_Var.size(); i++)
	{
		for (int j = 0; j < 3; j++)
		{
			if (m_Var[i].getType() != type[j])
				continue;
			if (bFirst[j] || pos[j] > m_Var[i].getMin())
				pos[j] = m_Var[i].getMin();
			if (bFirst[j] || pos[j + 3] < m_Var[i].getMax())
				pos[j + 3] = m_Var[i].getMax();
			bFirst[j] = false;
		}
	}

	return pos;
}

/* =============================================================================
 =============================================================================== */
bool CData::createRegularGrid(vector<double> time, vector<double> lat, vector<double> lon, vector<float> depth)
{
	if (time.size() == 0)
		time.push_back(getCurrentTime());
	if (time.size() == 0 || lat.size() == 0 || lon.size() == 0 || depth.size() == 0)
		return setLastError("Size of at least 1 parameter is 0");

	//  only time dimension
	string	dimlist[] = { "time", "level", "y_coord", "x_coord", "" };
	int		dimsize[4] = { time.size(), depth.size(), lat.size(), lon.size() };
	for (int i = 0; dimlist[i].size(); i++)
	{
		int iDim = newDim(dimlist[i], dimsize[i]);
		if (i == 0) 
			setDimUnlimited(iDim, true);
	}

	//  create variables
	string	varlist[] = { "time", "depth", "latitude", "longitude", "" };
	int		iType[] = { DATA_TIME, DATA_DEPTH, DATA_LAT, DATA_LON };
	int		iDims[4][4] = { { 0, -1 }, { 1, 2, 3, -1 }, { 2, 3, -1 }, { 2, 3, -1 } };
	for (int i = 0; varlist[i].size(); i++)
	{
		vector<int> vDims;
		for (int j = 0; iDims[i][j] != -1; j++) 
			vDims.push_back(iDims[i][j]);

		int iVar = newVar(varlist[i], iType[i], vDims);
		if (!loadVar(iVar)) 
			return false;
	}

	//  time
	setVarDData(0, time);

	//  depth
	int		iCnt = 0;
	float	*pDepth = &(getVarFData(1)[0]);
	for (int i = 0; i < depth.size(); i++)
		for (int j = 0; j < lat.size() * lon.size(); j++, iCnt++) 
			pDepth[iCnt] = depth[i];

	//  lat and lon
	double	*pLat = &(getVarDData(2)[0]);
	double	*pLon = &(getVarDData(3)[0]);
	iCnt = 0;
	for (int i = 0; i < lat.size(); i++)
		for (int j = 0; j < lon.size(); j++, iCnt++)
		{
			pLat[iCnt] = lat[i];
			pLon[iCnt] = lon[j];
		}

	setExtents();
	return true;
}

/* =============================================================================
 =============================================================================== */
bool CData::createModelGrid
(
	vector<double>	time,
	int				iYCoord,
	int				iXCoord,
	int				iLevels,
	vector<double>	lat,
	vector<double>	lon,
	vector<float>	depth
)
{
	if (time.size() == 0) 
		time.push_back(getCurrentTime());
	if (time.size() == 0 || lat.size() == 0 || lon.size() == 0 || depth.size() == 0)
		return setLastError("Size of at least 1 parameter is 0");

	string	dimlist[] = { "time", "level", "y_coord", "x_coord", "" };
	int		dimsize[4] = { (int) time.size(), iLevels, iYCoord, iXCoord };
	for (int i = 0; dimlist[i].size(); i++)
	{
		int iDim = newDim(dimlist[i], dimsize[i]);
		if (i == 0) 
			setDimUnlimited(iDim, true);
	}

	//  create variables
	string	varlist[] = { "time", "depth", "latitude", "longitude", "" };
	int		iType[] = { DATA_TIME, DATA_DEPTH, DATA_LAT, DATA_LON };
	int		iDims[4][4] = { { 0, -1 }, { 1, 2, 3, -1 }, { 2, 3, -1 }, { 2, 3, -1 } };
	for (int i = 0; varlist[i].size(); i++)
	{
		vector<int> vDims;
		for (int j = 0; iDims[i][j] != -1; j++) 
			vDims.push_back(iDims[i][j]);

		int iVar = newVar(varlist[i], iType[i], vDims);
		if (!loadVar(iVar))
			return false;
	}

	setVarDData(0, time);
	setVarFData(1, depth);
	setVarDData(2, lat);
	setVarDData(3, lon);

	setExtents();
	return true;
}

/* =============================================================================
 =============================================================================== */
bool CData::createPathData
(
	const vector<double>	&time,
	const vector<double>	&lat,
	const vector<double>	&lon,
	const vector<float>	&depth,
	int				depthSteps
)
{
	if (time.size() == 0 || lat.size() == 0 || lon.size() == 0 || depth.size() == 0 || depthSteps == 0)
		return setLastError("Size of at least 1 parameter is 0");

	if (lat.size() != lon.size() || lat.size() != depth.size())
		return setLastError("Lat, lon, and depth vectors must all be of same size");

	//  only time dimension
	string	dimlist[] = { "time", "levels", "" };
	if (depthSteps == 1)
		dimlist[1] = "";

	int dimsize[3] = { time.size(), depthSteps };
	for (int i = 0; dimlist[i].size(); i++)
	{
		int iDim = newDim(dimlist[i], dimsize[i]);
		if (i == 0) 
			setDimUnlimited(iDim, true);
	}

	//  create variables
	string	varlist[] = { "time", "depth", "latitude", "longitude", "" };
	int		iType[] = { DATA_TIME, DATA_DEPTH, DATA_LAT, DATA_LON };
	int		iDims[4][4] = { { 0, -1 }, { 0, 1, -1 }, { 0, -1 }, { 0, -1 } };
	for (int i = 0; varlist[i].size(); i++)
	{
		vector<int> vDims;
		if (depthSteps == 1)
			vDims.push_back(0);
		else
			for (int j = 0; iDims[i][j] != -1; j++) 
				vDims.push_back(iDims[i][j]);

		int iVar = newVar(varlist[i], iType[i], vDims);
		if (!loadVar(iVar)) 
			return false;
	}

	//  time
	setVarDData(0, time);

	//  lat and lon
	setVarDData(2, lat);
	setVarDData(3, lon);

	//  depth
	int		iCnt = 0;
	float	*pDepth = &(getVarFData(1)[0]);
	for (int i = 0; i < lat.size(); i++)
	{
		float	fInc = 0;
		float	fDepth = depth[i];
		if (depthSteps > 1)
		{
			fInc = depth[i] / (depthSteps - 1);
			fDepth = 0;
		}

		for (int j = 0; j < depthSteps; j++, iCnt++, fDepth += fInc) 
			pDepth[iCnt] = fDepth;
	}

	setExtents();
	return true;
}

struct t_rec
{
	double	time;
	int		idx;
};

bool operator < (const struct t_rec &a, const struct t_rec &b)
{
	return a.time < b.time;
}

/* =============================================================================
 =============================================================================== */
bool CData::sortTime()
{
	int iTimeDim = -1;
	for (int i = 0; i < getDimCnt() && iTimeDim == -1; i++)
		if (getDimUnlimited(i)) 
			iTimeDim = i;
	if (iTimeDim == -1)
		return true;

	int iTimeSteps = getDimSize(iTimeDim);

	int iTimeVar = -1;
	for (int i = 0; i < getVarCnt() && iTimeVar == -1; i++)
		if (getVarType(i) == DATA_TIME && getVarSize(i) == iTimeSteps) 
			iTimeVar = i;
	if (iTimeVar == -1)
		return true;

	bool			bOrdered = true;
	double			*pTime = &(getVarDData(iTimeVar)[0]);
	vector<t_rec>	recs;
	recs.resize(iTimeSteps);
	for (int i = 0; i < iTimeSteps; i++)
	{
		t_rec	r;
		r.idx = i;
		r.time = pTime[i];
		recs[i] = r;
		if (i + 1 < iTimeSteps && pTime[i] > pTime[i + 1]) 
			bOrdered = false;
	}

	if (bOrdered)
		return true;

	cout << (" - sorting records by time");
	for (int i = 0; i < getVarCnt(); i++) 
		loadVar(i);
	sort(recs.begin(), recs.end());

	vector<float>	vDims;
	for (int i = 0; i < iTimeSteps; i++) 
		vDims.push_back(recs[i].idx);

	bool	bRet = resetDim(iTimeDim, vDims);
	return bRet;
}

/* =============================================================================
 =============================================================================== */
bool CData::readCSV(string fileName)
{
	ostringstream	s;

	g_Timers.startPerfTimer(NET_PERF_TIMER);
	cleanWebFile(fileName);
	fileName = cacheWebFile(fileName);
	g_Timers.stopPerfTimer(NET_PERF_TIMER);

	g_Timers.startPerfTimer(IO_PERF_TIMER);

	//
	// note: can read line at a time to minimize space requirement like we do with ascii terrain
	// but need to build up fdata/ddata locally and then set them along with final time dimension
	// currently unnecessary since csv size is usually tiny compared to other datasets
	// load data into buffer
	//
	char	*filebuf;
	int		len;
	if (!readfile(fileName, filebuf, len))
		return setLastError("Unable to read CSV file");

	//  parse out csv tokens
	vector<vector<string> > csvTokens;
	bool					bRet = getCSVTokens(filebuf, csvTokens);
	delete[] filebuf;
	if (!bRet || csvTokens.size() == 0)
		return setLastError("Unable to parse CSV file");

	int iRecCnt = csvTokens.size() - 1;
	if (iRecCnt == 0)
		return setLastError("No records found in CSV file");

	//  only time dimension
	int iDim = newDim("time", iRecCnt);
	setDimUnlimited(iDim, true);

	bool	bSetDepth = true;
	int		iCurVar = -1;
	double	fNoVal = NODATA;
	vector<int> vDims(1, 0);

	int iVarCnt = csvTokens[0].size();

	//  create variables from the header
	for (int i = 0; i < iVarCnt; i++)
	{
		string	token = csvTokens[0][i];
		int		iVar = newVar(token, DATA_SCALAR, vDims);
		token = lwrstr(token);

		if (token.find("time") != string::npos || token.find("date") != string::npos)
		{
			setVarType(iVar, DATA_TIME);
			newAtt(iVar, "long_name");
			setAttText(iVar, 0, "time");
		}
		else if (token.find("latitude") != string::npos || token.find("lat") != string::npos)
		{
			setVarType(iVar, DATA_LAT);
			newAtt(iVar, "long_name");
			setAttText(iVar, 0, "latitude");
		}
		else if (token.find("longitude") != string::npos || token.find("lon") != string::npos)
		{
			setVarType(iVar, DATA_LON);
			newAtt(iVar, "long_name");
			setAttText(iVar, 0, "longitude");
		}
		else if (token.find("depth") != string::npos)
		{
			setVarType(iVar, DATA_DEPTH);
			bSetDepth = false;
			newAtt(iVar, "long_name");
			setAttText(iVar, 0, "depth");
		}
		else if (token.find("name") != string::npos
				||	token.find("image") != string::npos
				||	token.find("video") != string::npos
				||	token.find("comment") != string::npos)
		{
			setVarType(iVar, DATA_ENUM);
			setVarNoValue(iVar, -1);
		}
		else
		{
			setVarType(iVar, DATA_SCALAR);
			setVarNoValue(iVar, fNoVal);
		}

		if ((getVarType(iVar) == DATA_SCALAR || getVarType(iVar) == DATA_ENUM) && (iCurVar == -1)) 
			iCurVar = i;
		if (!loadVar(iVar)) 
			return setLastError("Unable to load variable");
	}

	//  if no depth data is included then add field
	int iDepthVar = -1;
	if (bSetDepth)
	{
		iDepthVar = newVar("depth", DATA_DEPTH, vDims);
		setVarFData(iDepthVar, 0.0);

		int iAtt = newAtt(iDepthVar, "standard_name");
		setAttText(iDepthVar, iAtt, "depth");
		setVarNoValue(iDepthVar, fNoVal);
	}

	//  load in the data
	for (int i = 0; i < iVarCnt; i++)
	{
		float	*pFData = getVarIsDData(i) ? NULL : &(getVarFData(i)[0]);
		double	*pDData = getVarIsDData(i) ? &(getVarDData(i)[0]) : NULL;

		for (int j = 0; j < iRecCnt; j++)
		{
			string	token = (i < csvTokens[j + 1].size()) ? csvTokens[j + 1][i] : "";
			double	val;
			if (getVarType(i) == DATA_TIME)
			{
				if (i < iVarCnt - 1 && getVarType(i + 1) == DATA_TIME)
				{
					token += " " + csvTokens[j + 1][i + 1];
					csvTokens[j + 1][i + 1] = token;
				}

				val = scanDateTime(token, m_TimeFormat);

				if (val < 0)
					val = fNoVal;
				else
					pDData[j] = val;
			}
			else if (getVarType(i) == DATA_ENUM)
			{
				m_Var[i].setEnumValue(j, token);
			}
			else if (getVarIsDData(i))
			{
				val = token.size() ? atof(token.c_str()) : fNoVal;
				pDData[j] = val;
			}
			else
			{
				val = token.size() ? atof(token.c_str()) : fNoVal;
				pFData[j] = val;
			}
		}
	}

	//  set up the depth now that we have lat and lon for points
	if (bSetDepth)
	{
		int iPressureVar = findVar("pressure");
		if (iPressureVar == -1)
			iPressureVar = findVar("PrDM");
		if (iPressureVar != -1)
			if (!setDepthFromPressure(iPressureVar, iDepthVar)) 
				return setLastError("Memory allocation failure");
	}

	setExtents();
	setDirty(false);

	g_Timers.stopPerfTimer(IO_PERF_TIMER);

	return true;
}

/* =============================================================================
 =============================================================================== */
bool CData::loadVar(int iVar)
{
	if (!getVarValid(iVar))
		return setLastError("Invalid variable index");

	int iErr = 0;
	if (!m_Var[iVar].isLoaded())
	{
		if (getNCFileID() == -1 || m_Var[iVar].getID() == -1)
		{
			if (getVarIsDData(iVar))
				m_Var[iVar].initDDataMem();
			else
				m_Var[iVar].initFDataMem();
		}
		else
		{
			if (g_Env.m_Verbose) 
				cout << (" - loading variable data: " + getVarName(iVar));

			int size = getVarSize(iVar);
			if (size == 0)
				return setLastError("Size of variable is 0");

			/*
			if (getVarIsDData(iVar))
			{
				if (!m_Var[iVar].initDDataMem()) 
					return setLastError("Memory allocation failure");

				double	*pData = &(getVarDData(iVar)[0]);
				iErr = nc_get_var_double(getNCFileID(), m_Var[iVar].getID(), pData);
				if (iErr == NC_ECHAR)	//  if fail, then try to load as char
				{
					char	*pch = new char[size];
					iErr = nc_get_var_text(getNCFileID(), m_Var[iVar].getID(), pch);
					if (iErr == 0)
						for (int i = 0; i < size; i++) 
							pData[i] = pch[i];
					delete pch;
				}

				if (iErr != 0) 
					m_Var[iVar].clearDData();
			}
			else
			{
				if (!m_Var[iVar].initFDataMem()) 
					return setLastError("Memory allocation failure");

				float	*pData = &(getVarFData(iVar)[0]);
				iErr = nc_get_var_float(getNCFileID(), m_Var[iVar].getID(), pData);
				if (iErr == NC_ECHAR)	//  if fail, then try to load as char
				{
					char	*pch = new char[size];
					iErr = nc_get_var_text(getNCFileID(), m_Var[iVar].getID(), pch);
					if (iErr == 0)
						for (int i = 0; i < size; i++) 
							pData[i] = pch[i];
					delete pch;
				}

				if (iErr != 0)
					m_Var[iVar].clearFData();
				else
					scale_var(iVar);
			}
			*/

			m_Var[iVar].updateMinMax();
		}

		if (getVarType(iVar) == DATA_SCALAR) 
			resetCoordVars(iVar);
	}

	if (iErr != 0)
		return setLastError("Error trying to load variable");
	if (getVarType(iVar) == DATA_SCALAR) 
		setCoordVars(iVar);

	return true;
}

/* =============================================================================
 =============================================================================== */
int CData::guessVarType(int iVar)
{
	if (getVarDims(iVar).size() == 0)
		return DATA_NONE;
	if (lwrstr(getVarName(iVar)) == "connect_point_mask")
		return DATA_NONE;

	string	type_string[4] = { "latitude", "longitude", "depth", "time" };
	string	type_string2[4] = { "lat", "long", "altitude", "time" };
	int		type_val[4] = { DATA_LAT, DATA_LON, DATA_DEPTH, DATA_TIME };

	int		iType = DATA_NONE;
	bool	bHasUnlimitedDim = false;
	for (int i = 0; i < getDimCnt(); i++)
		if (getDimUnlimited(i)) 
			bHasUnlimitedDim = true;

	bool	bHasUnlimitedVarDim = false;
	if (bHasUnlimitedDim)
	{
		vector<int> vDims = getVarDims(iVar);
		for (int i = 0; i < vDims.size(); i++)
			if (getDimUnlimited(vDims[i])) bHasUnlimitedVarDim = true;
	}
	else
		bHasUnlimitedVarDim = true;

	string	var_name = lwrstr(getVarName(iVar));
	for (int i = 0; i < 4 && iType == DATA_NONE; i++)
		if (var_name == type_string[i] || var_name == type_string2[i]) 
			iType = type_val[i];

	//  if standard name then just read it
	int iAtt = findAtt(iVar, "standard_name");
	if (iAtt != -1)
	{
		string	var_att_name = lwrstr(getAttText(iVar, iAtt));
		for (int i = 0; i < 4 && iType == DATA_NONE; i++)
			if (var_att_name.find(type_string[i]) != string::npos) 
				iType = type_val[i];
	}

	//  if no type yet then look at name
	if (iType == DATA_NONE)
	{

		//  if cove format then check if name or long_name is a match
		iAtt = findAtt(-1, "Conventions");
		if (iAtt != -1 && getAttText(-1, iAtt) == "COVE")
		{
			string	var_name = lwrstr(getVarName(iVar));
			iAtt = findAtt(iVar, "long_name");

			string	var_long_name = iAtt == -1 ? "" : lwrstr(getAttText(iVar, iAtt));
			for (int i = 0; i < 4 && iType == DATA_NONE; i++)
			{
				if (var_name == type_string[i] || var_long_name.substr(0, type_string[i].length()) == type_string[i])
					iType = type_val[i];
			}
		}
		else	//  else start guessing based on other attributes and var name
		{
			iAtt = findAtt(iVar, "long_name");
			if (iAtt == -1) 
				iAtt = findAtt(iVar, "title");

			string	var_att_name = iAtt == -1 ? "" : lwrstr(getAttText(iVar, iAtt));
			string	var_name = lwrstr(getVarName(iVar));
			for (int i = 0; i < 4 && iType == DATA_NONE; i++)
			{
				if (var_name.find(type_string[i]) != string::npos || var_att_name.find(type_string[i]) != string::npos)
					iType = type_val[i];
				if (iType == DATA_TIME && (getVarDims(iVar).size() > 1 || !bHasUnlimitedVarDim)) 
					iType = DATA_NONE;

				//  mbari auv hack
				if (iType == DATA_TIME && var_att_name.find("profile") != string::npos) 
					iType = DATA_NONE;
			}
		}
	}

	if (iType == DATA_NONE && bHasUnlimitedVarDim) 
		iType = DATA_SCALAR;
	return iType;
}

/* =============================================================================
 =============================================================================== */
int CData::getDataFormat() const
{
	if (findVar("connect_point_mask") != -1)
		return DFORMAT_STREAMNETWORK;
	if (findDim("shapes") != -1)
		return DFORMAT_SHAPES;
	if (getDimCnt() <= 2 && getLatCnt() == getLonCnt())
		return DFORMAT_PATH;
	if (getDimCnt() >= 2 && getLatCnt() > 1 && getLonCnt() > 1)
		return getDepthCnt() == 1 ? DFORMAT_2DSURFACE : DFORMAT_3DMODEL;
	return DFORMAT_POINTS;
}

/* =============================================================================
 =============================================================================== */
bool CData::resetCoordVars(int iVar)
{
	if (!getVarValid(iVar))
		return false;

	int type_val[4] = { DATA_TIME, DATA_LAT, DATA_LON, DATA_DEPTH };
	vector<int> vCoordVars(4, -1);

	//  find a variable of each type with matching dimension
	vector<int> vDims = getVarDims(iVar);

	for (int t = 0; t < 4; t++)
	{
		for (int v = 0; v < getVarCnt(); v++)
		{
			if (getVarType(v) != type_val[t])
				continue;
			for (int d1 = 0; d1 < vDims.size(); d1++)
			{
				if (getVarDims(v).size() > vDims.size() - d1)
					break;
				if (vDims[d1] != getVarDims(v)[0])
					continue;

				int d2 = 0;
				for (; d2 < getVarDims(v).size(); d2++)
					if (vDims[d1 + d2] != getVarDims(v)[d2])
						break;
				if (d2 == getVarDims(v).size()) vCoordVars[t] = v;
			}

			if (vCoordVars[t] != -1)
				break;
		}
	}

	//  update CoordNoValue based on CoordVars
	Vec3d	CoordNoValue = Vec3d(NODATA, NODATA, NODATA);
	for (int i = 0; i < 3; i++)
	{
		if (vCoordVars[i + 1] >= 0 && getVar(vCoordVars[i + 1]).findAtt("_FillValue") >= 0)
			CoordNoValue[i] = getVar(vCoordVars[i + 1]).getNoValue();
	}

	editVar(iVar).setCoordVars(vCoordVars);
	editVar(iVar).setCoordNoValue(CoordNoValue);
	setCoordVars(iVar);
	return true;
}

/* =============================================================================
 =============================================================================== */
bool CData::readNetCDF_Raw(string fileName)
{
	/*
	int				ncdfid;					//  read netCDF file handle
	int				dim_array[MAX_NC_DIMS]; //  size of selected variable dimensions
	int				ndims, nvars, natts;	// # of selected variable dimensions
	nc_type			data_type;				//  netCDF data type
	char			dim_name[NC_MAX_NAME];
	char			var_name[NC_MAX_NAME];
	char			att_name[NC_MAX_NAME];
	char			att_text[2048];
	size_t			att_len;
	long			dimsize;

	ostringstream	s;

	g_Timers.startPerfTimer(NET_PERF_TIMER);
	cleanWebFile(fileName);
	fileName = cacheWebFile(fileName);
	g_Timers.stopPerfTimer(NET_PERF_TIMER);

	g_Timers.startPerfTimer(IO_PERF_TIMER);

	//  Open the netcdf file for reading.
	if (!fileexists(fileName)) 
		return setLastError("Cannot find Netcdf file");
	if ((ncdfid = ncopen(fileName.c_str(), NC_NOWRITE)) == -1) 
		return setLastError("Cannot load Netcdf file");

	setNCFileID(ncdfid);

	if (g_Env.m_Verbose) 
		cout << (" - loading metadata");

	//  Inquire about the dimensions.
	nc_inq_ndims(ncdfid, &ndims);

	int unlimdim = -1;
	nc_inq_unlimdim(ncdfid, &unlimdim);
	for (int i = 0; i < ndims; i++)
	{
		ncdiminq(ncdfid, i, dim_name, &dimsize);

		int iDim = newDim(dim_name, dimsize);
		if (i == unlimdim) setDimUnlimited(iDim, true);
	}

	//  Inquire about the attributes.
	nc_inq_natts(ncdfid, &natts);
	for (int att_id = 0; att_id < natts; att_id++)
	{

		//  Retrieve the info about the named variable.
		nc_inq_attname(ncdfid, NC_GLOBAL, att_id, att_name);

		int iAtt = newAtt(-1, att_name);
		nc_inq_atttype(ncdfid, NC_GLOBAL, att_name, &data_type);
		nc_inq_attlen(ncdfid, NC_GLOBAL, att_name, &att_len);
		if (data_type == NC_CHAR)
		{
			nc_get_att_text(ncdfid, NC_GLOBAL, att_name, att_text);
			att_text[att_len] = '\0';
			setAttText(-1, iAtt, att_text);
		}
		else if (att_len)
		{
			vector<double> vp(att_len, 0.0);
			nc_get_att_double(ncdfid, NC_GLOBAL, att_name, &vp[0]);
			setAttData(-1, iAtt, vp);
		}
	}

	//  Inquire about the variables.
	nc_inq_nvars(ncdfid, &nvars);
	for (int var_id = 0; var_id < nvars; var_id++)
	{

		//  Retrieve the info about the named variable.
		nc_inq_var(ncdfid, var_id, var_name, &data_type, &ndims, dim_array, &natts);

		vector<int> vDims;
		for (int i = 0; i < ndims; i++) 
			vDims.push_back(dim_array[i]);

		int iVar = newVar(var_name, DATA_NONE, vDims);
		m_Var[iVar].setID(var_id);

		//  get the variable attributes.
		for (int att_id = 0; att_id < natts; att_id++)
		{
			nc_type attdata_type;	//  netCDF data type

			//  Retrieve the info about the named variable.
			nc_inq_attname(ncdfid, var_id, att_id, att_name);

			int iAtt = newAtt(iVar, att_name);
			nc_inq_atttype(ncdfid, var_id, att_name, &attdata_type);
			nc_inq_attlen(ncdfid, var_id, att_name, &att_len);
			if (attdata_type == NC_CHAR)
			{
				nc_get_att_text(ncdfid, var_id, att_name, att_text);
				att_text[att_len] = '\0';
				setAttText(iVar, iAtt, att_text);
			}
			else if (att_len)
			{
				vector<double> vp(att_len, 0);
				nc_get_att_double(ncdfid, var_id, att_name, &vp[0]);
				setAttData(iVar, iAtt, vp);
			}
		}

		double	val = 0.0;
		if (nc_get_att_double(ncdfid, var_id, "_FillValue", &val) == 0)
			setVarNoValue(iVar, val);
		else if (nc_get_att_double(ncdfid, var_id, "missing_value", &val) == 0)
			setVarNoValue(iVar, val);

		//  set the type and load if necessary
		int iType = guessVarType(iVar);
		setVarType(iVar, iType);

		if (iType >= DATA_TIME && iType <= DATA_DEPTH)
		{
			if (!loadVar(iVar)) 
				return setLastError("Unable to load variable " + getVarName(iVar));
			if (iType == DATA_LON)
			{

				//  normalize lon
				vector<double>	&pd = getVarDData(iVar);
				for (int i = 0; i < getVarSize(iVar); i++)
					if (pd[i] > 180) 
						pd[i] -= 360;
			}
		}
	}

	g_Timers.stopPerfTimer(IO_PERF_TIMER);
	return true;
	*/
	return false;
}

/* =============================================================================
 =============================================================================== */
bool CData::convertNetCDF(bool bTime, bool bVars)
{
	if (bTime && !bVars)
		return true;
	if (!setCoordVars(getLargestOfType(DATA_SCALAR)))
		return false;

	//  convert if this is not already in COVE format.
	if (!isCOVEFormat())
	{
		int iAtt;
		if ((iAtt = findAtt(-1, "type")) != -1 && getAttText(-1, iAtt).find("ROMS") != -1)
		{
			if (!convertROMS(bTime, bVars))
				return false;
		}
		else if ((iAtt = findAtt(-1, "history")) != -1 && getAttText(-1, iAtt).find("FERRET") != -1)
		{
			if (!convertFERRET())
				return false;
		}
		else if (getDataFormat() == DFORMAT_3DMODEL || getDataFormat() == DFORMAT_2DSURFACE)
		{
			if (convertRegularGrid()) m_RegularGrid = true;
		}
	}

	if (!m_RegularGrid) 
		m_RegularGrid = checkRegularGrid();

	setExtents();
	setDirty(false);
	return true;
}

/* =============================================================================
 =============================================================================== */
bool CData::readNetCDF(string fileName)
{
	if (readNetCDF_Raw(fileName)) 
		return convertNetCDF(true, true);
	return false;
}

/* =============================================================================
 =============================================================================== */
bool CData::writeNetCDF(string fileName)
{
	/*
	// When we create netCDF variables and dimensions, we get back an ID for each one.
	int ncid;
	int dimids[NC_MAX_VAR_DIMS];
	int varids[NC_MAX_VARS];
	int dim_array[MAX_NC_DIMS]; //  size of selected variable dimensions

	//  error handling.
	int retval = 0;

	fileName = getLocalPath(fileName);

	// Create the file. The NC_CLOBBER parameter tells netCDF to overwrite
	// this file, if it already exists.
	if ((retval = nc_create(fileName.c_str(), NC_CLOBBER, &ncid)))
		return setLastError("ncError: " + (string) nc_strerror(retval));

	//  Define the dimensions. NetCDF will hand back an ID for each.
	for (int i = 0; i < getDimCnt(); i++)
	{
		if ((retval = nc_def_dim	(ncid,	getDimName(i).c_str(),
			getDimUnlimited(i) ? NC_UNLIMITED : getDimSize(i),&(dimids[i]))))
			return setLastError("ncError: " + (string) nc_strerror(retval));
	}

	//  mark COVE files
	int iAtt = findAtt(-1, "Conventions");
	if (iAtt == -1) iAtt = newAtt(-1, "Conventions");
	setAttText(-1, iAtt, "COVE");

	//  Define the global attributes.
	for (int i = 0; i < getAttCnt(-1); i++)
	{
		if (getAttText(-1, i).size())
		{
			retval = nc_put_att_text
				(
					ncid,
					NC_GLOBAL,
					getAttName(-1, i).c_str(),
					getAttText(-1, i).size(),
					getAttText(-1, i).c_str()
				);
		}
		else if (getAtt(i).getData().size())
		{
			retval = nc_put_att_double
				(
					ncid,
					NC_GLOBAL,
					getAttName(-1, i).c_str(),
					NC_DOUBLE,
					getAttData(-1, i).size(),
					&(getAtt(i).getData()[0])
				);
		}

		if (retval) 
			return setLastError("ncError: " + (string) nc_strerror(retval));
	}

	// Define the variable. The type of the variable in this case is NC_INT (4-byte integer).
	for (int i = 0; i < getVarCnt(); i++)
	{
		for (int d = 0; d < getVarDims(i).size(); d++) 
			dim_array[d] = getVarDims(i)[d];
		dim_array[getVarDims(i).size()] = -1;
		if ( (retval = nc_def_var(ncid,	getVarName(i).c_str(),
			getVarIsDData(i) ? NC_DOUBLE : NC_FLOAT,(int) getVarDims(i).size(),	dim_array,	&(varids[i]))))
			return setLastError("ncError: " + (string) nc_strerror(retval));

		//  add standard_name if this is a position or time type
		string	type_string[4] = { "latitude", "longitude", "depth", "time" };
		int		type_val[4] = { DATA_LAT, DATA_LON, DATA_DEPTH, DATA_TIME };
		for (int t = 0; t < 4; t++)
		{
			if (getVar(i).getType() == type_val[t])
			{
				int iAtt = findAtt(i, "standard_name");
				if (iAtt == -1) iAtt = newAtt(i, "standard_name");
				setAttText(i, iAtt, type_string[t]);
				break;
			}
		}

		for (int j = 0; j < getAttCnt(i); j++)
		{
			if (getAttText(i, j).size())
			{
				retval = nc_put_att_text
					(
						ncid,
						varids[i],
						getAttName(i, j).c_str(),
						getAttText(i, j).size(),
						getAttText(i, j).c_str()
					);
			}
			else if (getVar(i).getAttData(j).size())
			{
				if (getVarIsDData(i) || getAttName(i, j) != "_FillValue")
				{
					retval = nc_put_att_double
						(
							ncid,
							varids[i],
							getAttName(i, j).c_str(),
							NC_DOUBLE,
							getAttData(i, j).size(),
							&(getVar(i).getAttData(j)[0])
						);
				}
				else
				{
					float	fNoVal = getVar(i).getAttData(j)[0];
					retval = nc_put_att_float
						(
							ncid,
							varids[i],
							getAttName(i, j).c_str(),
							NC_FLOAT,
							getAttData(i, j).size(),
							&fNoVal
						);
				}
			}

			if (retval) 
				return setLastError("ncError: " + (string) nc_strerror(retval));
		}
	}

	if (g_Env.m_Verbose) 
		cout << ("    writing file definition");

	//  End define mode. This tells netCDF we are done defining metadata.
	if ((retval = nc_enddef(ncid))) 
		return setLastError("ncError: " + (string) nc_strerror(retval));

	//  Write the data to the file.
	for (int i = 0; i < getVarCnt(); i++)
	{
		if (g_Env.m_Verbose) 
			cout << ("    writing  " + m_Var[i].getName());

		bool	bTempLoad = false;
		if (!m_Var[i].isLoaded())
		{
			if (!loadVar(i))
			{
				nc_close(ncid);
				delfile(fileName);
				return setLastError("Unable to load variable " + getVarName(i));
			}

			bTempLoad = true;
		}

		size_t	iStart[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
		size_t	iCount[] = { 1, 1, 1, 1, 1, 1, 1, 1 };
		if (m_Var[i].getTrnDataLoaded())
		{
			for (int i = 0; i < m_Var[i].getTrnDataRowCnt(); i)
			{
				iStart[0] = i;
				iCount[1] = m_Var[i].getTrnDataRowLen();
				retval = nc_put_vara_float(ncid, varids[i], iStart, iCount, &(m_Var[i].getTrnDataRow(i)[0]));
			}
		}
		else
		{
			for (int j = 0; j < getVarDims(i).size(); j++) 
				iCount[j] = getDimSize(getVarDims(i)[j]);
			if (getVar(i).getDData().size())
			{
				if (getVar(i).getDData().size() == getVar(i).getDDataSize())
					retval = nc_put_vara_double(ncid, varids[i], iStart, iCount, &(getVarDData(i)[0]));
			}
			else
			{
				if (getVarSize(i) == getVar(i).getFDataSize())
					retval = nc_put_vara_float(ncid, varids[i], iStart, iCount, &(getVarFData(i)[0]));
			}
		}

		if (bTempLoad) 
			clearVarFData(i);
		if (retval) 
			return setLastError("ncError: " + (string) nc_strerror(retval));
	}

	// Close the file. This frees up any internal netCDF resources associated
	// with the file, and flushes any buffers.
	if ((retval = nc_close(ncid))) 
		return setLastError("mcError: " + (string) nc_strerror(retval));

	setDirty(false);
	if (g_Env.m_Verbose) 
		cout << ("Netcdf file written: " + fileName);

	return true;
	*/
	return false;
}

/* =============================================================================
 =============================================================================== */
bool CData::writeCSV(string fileName)
{
	string			strCSV;
	vector<double>	vNoVal;

	fileName = getLocalPath(fileName);

	if (getDimCnt() > 1)
		return false;

	bool	bComma = false;
	for (int i = 0; i < getVarCnt(); i++)
	{
		if (getVarDims(i).size() == 0)
			continue;
		if (bComma) strCSV += ",";
		strCSV += getVarName(i);
		vNoVal.push_back(getVarNoValue(i));
		bComma = true;
	}

	if (strCSV == "")
		return false;
	strCSV += '\n';

	char	buf[256];
	for (int d = 0; d < getDimSize(0); d++)
	{
		bComma = false;
		for (int i = 0; i < getVarCnt(); i++)
		{
			if (getVarDims(i).size() == 0)
				continue;
			if (bComma) strCSV += ',';
			switch(getVarType(i))
			{
			case DATA_TIME:
				strCSV += getGMTDateTimeString(getVarDData(i)[d]);
				bComma = true;
				break;

			case DATA_LAT:
			case DATA_LON:
				if (getVarDData(i)[d] != vNoVal[i])
				{
					sprintf(buf, "%lf", getVarDData(i)[d]);
					strCSV += buf;
				}

				bComma = true;
				break;

			case DATA_DEPTH:
			case DATA_SCALAR:
				if (getVarFData(i)[d] != vNoVal[i])
				{
					sprintf(buf, "%f", getVarFData(i)[d]);
					strCSV += buf;
				}

				bComma = true;
				break;

			case DATA_ENUM:
				if (getVarFData(i)[d] != vNoVal[i])
				{
					int ndx = getVarFData(i)[d];
					strCSV += getVar(i).getEnumString(ndx);
				}
				else
				{
					strCSV += "\"\"";
				}

				bComma = true;
				break;
			}
		}

		strCSV += '\n';
	}

	if (!writefile(fileName, strCSV.c_str(), strCSV.size())) 
		return setLastError("Error saving CSV file.");

	setDirty(false);
	return true;
}

/* =============================================================================
    LoadTerrain ;
    * create standard netcdf format ;
    dimensions: x, y ;
    vars: x(x): long_name="Longitude", actual_range=min,max ;
    vars: y(y): long_name="Latitude", actual_range=min,max ;
    vars: z(y,x): long_name="Topography (m)", _FillValue = -1#QNAN0f,
    actual_range=min,max ;
 =============================================================================== */
bool CData::initTerrain
(
	int		xDim,
	int		yDim,
	double	lonMin,
	double	latMax,
	float	xSpacing,
	float	ySpacing,
	float	fNoData,
	bool	bCenter
)
{

	//  generate new dimensions
	newDim("x", xDim);
	newDim("y", yDim);

	//  generate new variables
	int iVar, iAtt;

	if (xDim == 0 || yDim == 0) {
		return setLastError("An extents of the terrain is 0");
	}
	vector<int> vDims(1, 0);
	iVar = newVar("x", DATA_LON, vDims);
	iAtt = newAtt(iVar, "long_name");
	setAttText(iVar, iAtt, "Longitude");
	if (!initVarDData(iVar)) {
		return setLastError("Memory allocation failure");
	}
	vector<double>	&pLon = getVarDData(iVar);
	if (bCenter) 
		lonMin += xSpacing / 2; //  offset to move data to left of cell
	for (int i = 0; i < xDim; i++) {
		pLon[i] = lonMin + i * xSpacing;
	}
	vDims[0] = 1;
	iVar = newVar("y", DATA_LAT, vDims);
	iAtt = newAtt(iVar, "long_name");
	setAttText(iVar, iAtt, "Latitude");
	if (!initVarDData(iVar))
		return setLastError("Memory allocation failure");

	vector<double>	&pLat = getVarDData(iVar);
	if (bCenter)
		latMax -= ySpacing / 2; //  offset to move data to bottom of cell
	for (int i = 0; i < yDim; i++) 
		pLat[i] = latMax - i * ySpacing;

	vDims.push_back(0);
	iVar = newVar("z", DATA_TERRAIN, vDims);
	setVarNoValue(iVar, fNoData);
	iAtt = newAtt(iVar, "long_name");
	setAttText(iVar, iAtt, "Topography (m)");
	if (!m_Var[iVar].initTrnDataMem(yDim, xDim)) {
		return setLastError("Memory allocation failure");
	}
	return true;
}

/* =============================================================================
 =============================================================================== */
bool CData::loadTerrainVar(CData &Source, int iVar, int yDim, int xDim, bool bFlip)
{
	if (Source.getNCFileID() == -1 || !Source.getVarValid(iVar) || yDim * xDim == 0)
		return setLastError("Unable to load terrain variable");

	if (getVarCnt() != 3)
		return setLastError("Unexpected bad terrain format");
	if (g_Env.m_Verbose) 
		cout << "\n - loading terrain data: " + Source.getVarName(iVar);

	if (m_Var[2].getTrnDataRowCnt() != yDim)
		if (!m_Var[2].initTrnDataMem(yDim, xDim)) 
			return setLastError("Memory allocation failure");

	bool	b1DVar = Source.getVarDims(iVar).size() == 1;

	//  load the terrain data a line at a time
	size_t	vStart[2] = { 0, 0 };
	size_t	vCount[2] = { 1, 1 };
	if (!finite(m_Var[2].getNoValue()) || isnan(m_Var[2].getNoValue()))
		m_Var[2].setNoValue(NODATA);
	m_Var[2].initTrnMinMax();
	for (int i = 0; i < yDim; i++)
	{
		int ndx = bFlip ? yDim - i - 1 : i;
		if (b1DVar)
			vStart[0] = i * xDim;
		else
			vStart[0] = i;
		if (b1DVar)
			vCount[0] = xDim;
		else
			vCount[1] = xDim;
		//nc_get_vara_float(Source.getNCFileID(), iVar, vStart, vCount, &(m_Var[2].editTrnDataRow(ndx)[0]));
		m_Var[2].updateTrnMinMax(ndx);
	}

	m_Var[2].endTrnMinMax();

	if (bFlip)
	{
		if (!loadVar(1)) 
			return setLastError("Unable to load variable " + getVarName(1));

		vector<double>	&pLat = getVarDData(1);
		for (int i = 0; i < yDim / 2; i++)
		{
			float	tmp = pLat[i];
			pLat[i] = pLat[yDim - i - 1];
			pLat[yDim - i - 1] = tmp;
		}
	}

	return true;
}

/* =============================================================================
 =============================================================================== */
bool CData::convertTerrainToDataset()
{

	//  change terrain to a depth variable
	if (getVarCnt() < 2 || getVarType(2) != DATA_TERRAIN)
		return setLastError("Unexpected terrain format");
	if (!initVarFData(2)) 
		return setLastError("Memory allocation failure");

	float	*pData = &(getVar(2).editFData()[0]);
	int		iRows = getVar(2).getTrnDataRowCnt();
	int		iCols = getVar(2).getTrnDataRowLen();
	for (int i = 0; i < iRows; i++, pData += iCols)
		memcpy(pData, &(getVar(2).getTrnDataRow(i)[0]), iCols * sizeof(float));

	editVar(2).clearTrnData();
	setVarType(2, DATA_SCALAR);

	setExtents();
	return true;
}

/* =============================================================================
 =============================================================================== */
bool CData::readTerrain(string fileName)
{
	g_Timers.startPerfTimer(NET_PERF_TIMER);
	cleanWebFile(fileName);
	fileName = cacheWebFile(fileName);
	g_Timers.stopPerfTimer(NET_PERF_TIMER);

	g_Timers.startPerfTimer(IO_PERF_TIMER);

	if (getExt(fileName) == ".bil" || getExt(fileName) == ".bin")
	{
		bool	bRet = loadTerrainBIL(fileName);
		if (bRet && !getLoadAsTerrain())
			bRet = convertTerrainToDataset();
		g_Timers.stopPerfTimer(IO_PERF_TIMER);
		return bRet;
	}
	else if (getExt(fileName) == ".asc" || getExt(fileName) == ".txt")
	{
		bool	bRet = loadTerrainASC(fileName);
		if (bRet && !getLoadAsTerrain()) 
			bRet = convertTerrainToDataset();
		g_Timers.stopPerfTimer(IO_PERF_TIMER);
		return bRet;
	}

	g_Timers.stopPerfTimer(IO_PERF_TIMER);

	if (!readNetCDF_Raw(fileName))
		return setLastError("Cannot load terrain file");

	g_Timers.startPerfTimer(IO_PERF_TIMER);

	if (-1 == findDim("x"))	//  data starts at upper left hand corner
	{
		clean();

		CData	NewData;
		NewData.readNetCDF_Raw(fileName);

		//  new global dimensions for data
		if (!NewData.loadVar(0))
			return setLastError("Unable to load terrain variable");

		float	lonMin = NewData.getVarFDataValue(0, 0);

		//float lonMax = NewData.getVarFDataValue(0, 1);
		if (!NewData.loadVar(1))
			return setLastError("Unable to load terrain variable");

		//float latMin = NewData.getVarFDataValue(1, 0);
		float	latMax = NewData.getVarFDataValue(1, 1);

		if (!NewData.loadVar(2))
			return setLastError("Unable to load terrain variable");

		float	depthMin = NewData.getVarFDataValue(2, 0);
		float	depthMax = NewData.getVarFDataValue(2, 1);

		if (!NewData.loadVar(3))
			return setLastError("Unable to load terrain variable");

		float	xSpacing = NewData.getVarFDataValue(3, 0);
		float	ySpacing = NewData.getVarFDataValue(3, 1);

		if (!NewData.loadVar(4))
			return setLastError("Unable to load terrain variable");

		float	xDim = NewData.getVarFDataValue(4, 0);
		float	yDim = NewData.getVarFDataValue(4, 1);

		//  save scaling and offset attributes
		int		id = NewData.findAtt(5, "scale_factor");
		float	scale = id == -1 ? 1.0 : NewData.getAttData(5, id)[0];
		id = NewData.findAtt(5, "add_offset");

		float	offset = id == -1 ? 1.0 : NewData.getAttData(5, id)[0];
		id = NewData.findAtt(5, "_FillValue");

		float	fNoData = id == -1 ? NODATA : NewData.getAttData(5, id)[0];

		//  create terrain data in standard format
		if (!initTerrain(xDim, yDim, lonMin, latMax, xSpacing, ySpacing, fNoData, false))
			return false;

		editVar(2).setMin(depthMin);
		editVar(2).setMax(depthMax);

		if (!loadTerrainVar(NewData, 5, yDim, xDim, false))
			return false;

		//  restore scale and offset
		id = newAtt(2, "scale_factor");
		vector<double> ddata(1, scale);
		setAttData(2, id, ddata);
		id = newAtt(2, "add_offset");
		ddata[0] = offset;
		setAttData(2, id, ddata);
	}
	else	//  swap if data starts lower left hand corner
	{
		int xd = findDim("x");
		int xDim = getDimSize(xd);
		int yd = findDim("y");
		int yDim = getDimSize(yd);

		setVarType(0, DATA_LON);
		setVarType(1, DATA_LAT);
		if (!loadVar(0))
			return setLastError("Unable to load terrain variable");
		if (!loadVar(1))
			return setLastError("Unable to load terrain variable");

		double	latMax = getVarDDataValue(1, 0);
		double	latMin = getVarDDataValue(1, yDim - 1);

		//  don't need to do init terrain since it's in default format
		bool	bFlip = latMax < latMin;
		loadTerrainVar(*this, 2, yDim, xDim, bFlip);
	}

	if (!getLoadAsTerrain())
		return convertTerrainToDataset();

	g_Timers.stopPerfTimer(IO_PERF_TIMER);

	return true;
}

/* =============================================================================
 =============================================================================== */
bool CData::loadTerrainBIL(string fileName)
{

	//  see if we can read the file
	FILE	*fp;
	if ((fp = fopen(fileName.c_str(), "rb")) == NULL) 
		return setLastError("Unable to load terrain file");

	//  read in the .hdr file to get layout data
	setExt(fileName, ".hdr");

	char	*filebuf;
	int		len;
	if (!readfile(fileName, filebuf, len))
	{
		string	strDefault = getFilePath(fileName) + "header.hdr";
		if (!readfile(strDefault, filebuf, len)) 
			return setLastError("Unable to load terrain header file");
	}

	//  variables from the header file
	bool	bIntelData = true;
	float	fNoData = NODATA;
	int		xDim = 0, yDim = 0;
	float	xSpacing = 1, ySpacing = 1;
	double	lonMin = 0, latMax = 0;
	bool	bFloat = false;
	bool	b32 = false;

	//  parse out data from the header
	char	seps[] = " ,\t\n\r";
	char	*token;
	token = strtok(filebuf, seps);
	while(token != NULL)
	{

		//  Convert the token to lower case.
		string	strToken = lwrstr(token);

		if (strToken == "byteorder")
		{
			token = strtok(NULL, seps);
			if (tolower(token[0]) == 'm')
				bIntelData = false;
		}
		else if (strToken == "nrows")
		{
			token = strtok(NULL, seps);
			yDim = atoi(token);
		}
		else if (strToken == "ncols")
		{
			token = strtok(NULL, seps);
			xDim = atoi(token);
		}
		else if (strToken == "ulxmap")
		{
			token = strtok(NULL, seps);
			lonMin = atof(token);
		}
		else if (strToken == "ulymap")
		{
			token = strtok(NULL, seps);
			latMax = atof(token);
		}
		else if (strToken == "xdim")
		{
			token = strtok(NULL, seps);
			xSpacing = atof(token);
		}
		else if (strToken == "ydim")
		{
			token = strtok(NULL, seps);
			ySpacing = atof(token);
		}
		else if (strToken == "nodata")
		{
			token = strtok(NULL, seps);
			fNoData = atof(token);
		}
		else if (strToken == "pixeltype")
		{
			token = strtok(NULL, seps);
			bFloat = lwrstr(token) == "float";
		}
		else if (strToken == "nbits")
		{
			token = strtok(NULL, seps);
			b32 = lwrstr(token) == "32";
		}

		token = strtok(NULL, seps);
	}

	delete[] filebuf;

	int iTypeSize = bFloat || b32 ? sizeof(float) : sizeof(short);
	int iRowSize = xDim * iTypeSize;

	fseek(fp, 0, SEEK_END);

	int iFileLen = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	//  set up NetCDF like file for data
	if (iFileLen != yDim * xDim * iTypeSize
		||	!initTerrain(xDim, yDim, lonMin, latMax, xSpacing, ySpacing, fNoData, true))
	{
		fclose(fp);
		return false;
	}

	//  read the date from the file and turn into float
	m_Var[2].initTrnMinMax();
	for (int i = 0; i < yDim; i++)
	{
		float	*pData = &(m_Var[2].editTrnDataRow(i))[0];
		fread(pData, iRowSize, 1, fp);
		swapEndian(bIntelData, pData, iTypeSize, iRowSize);
		if (!bFloat)
		{
			unsigned char	*ps = (unsigned char *) pData + (xDim - 1) * iTypeSize;
			float			*pf = (pData) + xDim - 1;
			for (int j = 0; j < xDim; j++, ps -= iTypeSize, pf--) 
				*pf = b32 ? * (int *) (ps) : * (short *) (ps);
		}

		m_Var[2].updateTrnMinMax(i);
	}

	m_Var[2].endTrnMinMax();
	fclose(fp);

	return true;
}

/* =============================================================================
 =============================================================================== */
bool CData::loadTerrainASC(string fileName)
{

	//  read in the file
	char	*filebuf;
	int		len = 1024;
	bool	bRet = false;

	FILE	*fp = NULL;
	if ((fp = fopen(fileName.c_str(), "rb")) == NULL)
		return setLastError("Unable to load file");

	//  variables from the header file
	float	fNoData = NODATA;
	int		xDim = 0, yDim = 0;
	float	xSpacing = 1, ySpacing = 1;
	double	lonMin = 0, latMax = 0, latMin = 0;

	//  parse out data from the header
	char	seps[] = " ,\t\n\r";
	char	*token;

	//  figure out max line length xDim * 64
	if (!mem_alloc(filebuf, len + 1))
	{
		fclose(fp);
		return false;
	}

	while(fgets(filebuf, len, fp))
	{
		if (!isalpha(filebuf[0]))
			break;
		token = strtok(filebuf, seps);

		//  Convert the token to lower case.
		string	strToken = lwrstr(token);

		if (strToken == "nrows")
		{
			token = strtok(NULL, seps);
			yDim = atoi(token);
		}
		else if (strToken == "ncols")
		{
			token = strtok(NULL, seps);
			xDim = atoi(token);

			//  reallocate read in buffer
			len = xDim * 32;
			delete filebuf;
			if (!mem_alloc(filebuf, len + 1))
			{
				fclose(fp);
				return false;
			}
		}
		else if (strToken == "xllcorner")
		{
			token = strtok(NULL, seps);
			lonMin = atof(token);
		}
		else if (strToken == "yllcorner")
		{
			token = strtok(NULL, seps);
			latMin = atof(token);
		}
		else if (strToken == "cellsize")
		{
			token = strtok(NULL, seps);
			xSpacing = atof(token);
			ySpacing = xSpacing;
		}
		else if (strToken == "nodata_value")
		{
			token = strtok(NULL, seps);
			fNoData = atof(token);
		}
	}

	latMax = latMin + (yDim - 1) * ySpacing;

	//  set up NetCDF like file for data
	if (initTerrain(xDim, yDim, lonMin, latMax, xSpacing, ySpacing, fNoData, false))
	{
		//  read the date from the file and turn into float
		m_Var[2].initTrnMinMax();
		for (int i = 0; i < yDim; i++)
		{
			token = strtok(filebuf, seps);

			float	*pf = &(m_Var[2].editTrnDataRow(i))[0];
			for (int j = 0; j < xDim; j++, pf++)
			{
				if (!token || 1 != sscanf(token, "%g", pf)) *pf = fNoData;
				token = strtok(NULL, seps);
			}

			m_Var[2].updateTrnMinMax(i);
			if (!fgets(filebuf, len, fp))
				break;
		}

		m_Var[2].endTrnMinMax();
		bRet = true;
	}

	delete[] filebuf;
	fclose(fp);
	return bRet;
}
