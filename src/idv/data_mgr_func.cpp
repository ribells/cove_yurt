/* =============================================================================
	File: data_util.cpp

 =============================================================================== */

#include "netcdf.h"
#include <string.h>
#include "data_mgr.h"
#include "utility.h"
#include "timer.h"

void	download_progress_dlg(string caption, string text);
void	download_progress_update(string pText, double val);
void	download_progress_end();

/* =============================================================================
    DEPTH FUNCTIONS
 =============================================================================== */
float convertPressureToDepth(float P, float lat)
{
	// Saunders, 1981, "Practical conversion of pressure to depth"
	// Journal of Physical Oceanography, April 1981, V11, p573-574.
	float	x = sin(lat * vl_pi / 180);
	float	c1 = (5.92 + 5.25 * x * x) * 1.e-3;
	float	c2 = 2.21E-6;
	float	DEPTHM = (1 - c1) * P - c2 * P * P; //  meters
	return DEPTHM;
}

/* =============================================================================
 =============================================================================== */
float convertPressureToAlt(float P)
{
	float	H = -7.2 * log(P / 1012.5);
	return H;
}

/* =============================================================================
 =============================================================================== */
bool CData::setDepthFromPressure(int iPressureVar, int iDepthVar)
{
	setCoordVars(iPressureVar);
	if (getLatVar() == -1)
		return setLastError("Unable to find latitude variable to set depth from pressure");

	if (getVarSize(iPressureVar) != getVarSize(iDepthVar))
		return setLastError("Pressure variable and depth variable are different sizes");

	double	*pLat = &(getVarDData(getLatVar())[0]);
	float	*pPressure = &(getVarFData(iPressureVar)[0]);
	float	*pDepth = &(getVarFData(iDepthVar)[0]);
	for (int i = 0; i < getVarSize(iDepthVar); i++) 
		pDepth[i] = convertPressureToDepth(pPressure[i], pLat[i]);

	return true;
}

/* =============================================================================
 =============================================================================== */
bool CData::setDepthFromSigma(int iSigmaVar, int iBottomVar, int iDepthVar)
{
	g_Timers.startPerfTimer(CPU_PERF_TIMER);

	if (!getVarValid(iSigmaVar) || !getVarValid(iBottomVar) || !getVarValid(iDepthVar))
		return setLastError("Invalid variable passed to setDepthFromSigma function");

	if (!loadVar(iSigmaVar) || !loadVar(iBottomVar))
		return setLastError("Unable to load variable " + getVarName(iSigmaVar) + " or " + getVarName(iBottomVar));

	vector<float>	&pSigma = getVarFData(iSigmaVar);
	int				iSigma = getVarSize(iSigmaVar);

	vector<float>	&pBottom = getVarFData(iBottomVar);
	int				iBottom = getVarSize(iBottomVar);

	int				iFinalSize = iSigma * iBottom;

	if (iFinalSize == 0)
		return setLastError("Final size is 0");
	if (!initVarFData(iDepthVar))
		return setLastError("Memory allocation failure");

	float	fNoVal = getVarNoValue(iBottomVar);
	setVarNoValue(iDepthVar, fNoVal);

	vector<int> vDims = getVarDims(iSigmaVar);
	vector<int> vDims2 = getVarDims(iBottomVar);
	for (int i = 0; i < vDims2.size(); i++) 
		vDims.push_back(vDims2[i]);
	setVarDims(iDepthVar, vDims);

	vector<float>	&pDepth = getVarFData(iDepthVar);
	if (pDepth.size() == 0)
		return setLastError("Memory allocation failure");

	int iCnt = 0;
	for (int i = 0; i < iSigma; i++)
		for (int j = 0; j < iBottom; j++) 
			pDepth[iCnt++] = pBottom[j] == fNoVal ? fNoVal : pSigma[i] * pBottom[j];

	g_Timers.stopPerfTimer(CPU_PERF_TIMER);

	return true;
}

/* =============================================================================
    UTM & LL FUNCTIONS
 =============================================================================== */
vector<double> getUTMfromLL(double lat, double lon)
{
	vector<double>	output;
	double			zx, zy;
	double			E, N;
	if (!::getUTMfromLL(lat, lon, zx, zy, E, N))
		return output;
	output.push_back(zx);
	output.push_back(zy);
	output.push_back(E);
	output.push_back(N);
	return output;
}

/* =============================================================================
 =============================================================================== */
vector<double> getLLfromUTM(double fz_num, double fz_letter, double E, double N)
{
	vector<double>	output;
	double			lat, lon;
	if (!::getLLfromUTM(fz_num, fz_letter, E, N, lat, lon))
		return output;
	output.push_back(lat);
	output.push_back(lon);
	return output;
}

/* =============================================================================
 =============================================================================== */
bool CData::UTMtoLL(double fZoneNum, double fZoneLetter, int iVarEast, int iVarNorth, int iVarLat, int iVarLon)
{
	if (!getVarValid(iVarEast) || !getVarValid(iVarNorth) || !getVarValid(iVarLat) || !getVarValid(iVarLon))
		return setLastError("Invalid variable passed to UTMtoLL function");

	if (!loadVar(iVarEast) || !loadVar(iVarNorth))
		return setLastError("Unable to load variable " + getVarName(iVarEast) + " or " + getVarName(iVarNorth));

	vector<float>	&pFVarEast = getVarFData(iVarEast);
	vector<double>	&pDVarEast = getVarDData(iVarEast);
	bool			bFVarEast = pFVarEast.size() > 0;
	vector<float>	&pFVarNorth = getVarFData(iVarNorth);
	vector<double>	&pDVarNorth = getVarDData(iVarNorth);
	bool			bFVarNorth = pFVarNorth.size() > 0;

	//  make sure we have memory set
	int				iSize = getVarSize(iVarEast);
	if (iSize == 0)
		return setLastError("Size of east var is 0");
	if (!initVarDData(iVarLat) || !initVarDData(iVarLon)) 
		return setLastError("Memory allocation failure");

	setVarDims(iVarLon, getVarDims(iVarEast));
	setVarType(iVarLon, DATA_LON);
	initVarDData(iVarLon);
	setVarDims(iVarLat, getVarDims(iVarNorth));
	setVarType(iVarLat, DATA_LAT);
	initVarDData(iVarLat);

	vector<double>	&pVarLat = getVarDData(iVarLat);
	vector<double>	&pVarLon = getVarDData(iVarLon);
	for (int i = 0; i < iSize; i++)
	{
		::getLLfromUTM
		(
			fZoneNum,
			fZoneLetter,
			bFVarEast ? pFVarEast[i] : pDVarEast[i],
			bFVarNorth ? pFVarNorth[i] : pDVarNorth[i],
			pVarLat[i],
			pVarLon[i]
		);
	}

	return true;
}

/* =============================================================================
    CONVERT ROMS
 =============================================================================== */
bool CData::convertROMSCoords(int xi_rho, int eta_rho, int s_rho, int eta_u, int eta_v, int eta_psi, int s_w)
{
	g_Timers.startPerfTimer(CPU_PERF_TIMER);

	int iVarCnt = getVarCnt();
	for (int iVar = 0; iVar < iVarCnt; iVar++)
	{
		int iType = getVarType(iVar);
		if (!(iType == DATA_NONE || iType == DATA_TIME))
			if (findAtt(iVar, "_FillValue") == -1)
				setVarNoValue(iVar, -99999);
		if (!(iType == DATA_SCALAR || iType == DATA_ENUM)) 
			continue;

		int iDimCnt = (int) getVarDims(iVar).size();
		for (int d = 0; d < iDimCnt; d++)
		{
			int iDim = getVarDims(iVar)[d];
			if (!(iDim == eta_u || iDim == eta_v || iDim == eta_psi || iDim == s_w) || iDimCnt < 2)
				continue;
			if (g_Env.m_Verbose) 
				cout << (" converting variable " + getVarName(iVar));
			g_Timers.stopPerfTimer(CPU_PERF_TIMER);
			g_Timers.startPerfTimer(IO_PERF_TIMER);
			if (!loadVar(iVar)) 
				return setLastError("Unable to load variable " + getVarName(iVar));
			g_Timers.stopPerfTimer(IO_PERF_TIMER);
			g_Timers.startPerfTimer(CPU_PERF_TIMER);

			int iDepthMax = 1;
			for (int ndx = 0; ndx < iDimCnt - 2; ndx++) 
				iDepthMax *= getDimSize(getVarDims(iVar)[ndx]);

			int				iYMax = getDimSize(getVarDims(iVar)[iDimCnt - 2]);
			int				iXMax = getDimSize(getVarDims(iVar)[iDimCnt - 1]);
			float			fNoVal = getVarNoValue(iVar);
			vector<float>	&pSrc = getVarFData(iVar);
			vector<float>	pData;

			int				iNewSize = 0;
			vector<int>		vDims = getVarDims(iVar);
			if (iDim == eta_u || iDim == eta_psi)
			{
				iNewSize = iDepthMax * iYMax * (iXMax + 1);
				if (iNewSize == 0)
					break;
				try
				{
					pData.resize(iNewSize);
				}
				catch(std::bad_alloc const &)
				{
					return setLastError("Memory allocation failure");
				}

				int iSrc = 0;
				int iDst = 0;
				for (int i = 0; i < iDepthMax; i++)
				{
					for (int y = 0; y < iYMax; y++, iSrc += iXMax)
					{
						pData[iDst++] = fNoVal;
						for (int x = 0; x < iXMax - 1; x++)
						{
							if (pSrc[iSrc + x] == fNoVal || pSrc[iSrc + x + 1] == fNoVal)
								pData[iDst++] = fNoVal;
							else
								pData[iDst++] = (pSrc[iSrc + x] + pSrc[iSrc + x + 1]) / 2.0f;
						}

						pData[iDst++] = fNoVal;
					}
				}

				vDims[d] = eta_rho;
				vDims[d + 1] = xi_rho;
				setVarDims(iVar, vDims);
			}

			if (iDim == eta_psi)
			{
				pSrc.swap(pData);
				pSrc = getVarFData(iVar);
				pData.resize(0);
				iXMax++;
			}

			if (iDim == eta_v || iDim == eta_psi)
			{
				iNewSize = iDepthMax * (iYMax + 1) * (iXMax);
				if (iNewSize == 0)
					break;
				try
				{
					pData.resize(iNewSize);
				}
				catch(std::bad_alloc const &)
				{
					return setLastError("Memory allocation failure");
				}

				int iSrc = 0;
				int iDst = 0;
				for (int i = 0; i < iDepthMax; i++)
				{
					for (int x = 0; x < iXMax; x++) 
						pData[iDst++] = fNoVal;
					for (int y = 0; y < iYMax - 1; y++, iSrc += iXMax)
					{
						for (int x = 0; x < iXMax; x++)
						{
							if (pSrc[iSrc + x] == fNoVal || pSrc[iSrc + x + iXMax] == fNoVal)
								pData[iDst++] = fNoVal;
							else
								pData[iDst++] = (pSrc[iSrc + x] + pSrc[iSrc + x + iXMax]) / 2.0f;
						}
					}

					for (int x = 0; x < iXMax; x++) 
						pData[iDst++] = fNoVal;
					iSrc += iXMax;
				}

				vDims[d] = eta_rho;
				vDims[d + 1] = xi_rho;
				setVarDims(iVar, vDims);
			}

			if (iDim == s_w)
			{
				iNewSize = (iDepthMax - 1) * iYMax * iXMax;
				if (iNewSize == 0)
					break;
				try
				{
					pData.resize(iNewSize);
				}
				catch(std::bad_alloc const &)
				{
					return setLastError("Memory allocation failure");
				}

				int iSrc = 0;
				int iDst = 0;
				int iLevelMax = iYMax * iXMax;
				for (int i = 0; i < iDepthMax - 1; i++, iSrc += iLevelMax)
					for (int x = 0; x < iLevelMax; x++)
						pData[iDst++] = (pSrc[iSrc + x] + pSrc[iSrc + x + iLevelMax]) / 2.0f;
				vDims[d] = s_rho;
				setVarDims(iVar, vDims);
			}

			//  reset variable
			if (pData.size())
				pSrc.swap(pData);

			break;
		}

		resetVarSize(iVar);
	}

	g_Timers.stopPerfTimer(CPU_PERF_TIMER);

	return true;
}

/* =============================================================================
 =============================================================================== */
bool CData::convertROMS(bool bTime, bool bVars)
{
	cout << ("Converting ROMS file to COVE format");

	//  change time to correct format
	if (bTime)
	{
		int iTimeVar = findVar("ocean_time");
		if (iTimeVar == -1)
			return true;
		int iAtt = findAtt(iTimeVar, "units");
		if (iAtt != -1)
		{
			string	strDate = getAttText(iTimeVar, iAtt);
			while(strDate.size() && !isdigit(strDate[0]))	//  remove text before date
				strDate = strDate.substr(1);
			if (strDate.size())
			{
				dtime	dStart = scanDateTime(strDate, m_TimeFormat);
				if (!scaleVar(iTimeVar, 1.0, dStart)) 
					return false;
			}
		}
	}

	if (!bVars)
		return true;

	vector<float>	vLevels;

	if (findDim("xi_rho") == -1)
		return true;

	//download_progress_dlg("Loading ROMS File", "Converting ROMS to COVE Format");
	//download_progress_update("Updating Coordinates", 0.1);

	//  change vars to same coordinate system
	int xi_rho = findDim("xi_rho");
	int eta_rho = findDim("eta_rho");
	int s_rho = findDim("s_rho");
	int eta_u = findDim("eta_u");
	int eta_v = findDim("eta_v");
	int eta_psi = findDim("eta_psi");
	int s_w = findDim("s_w");
	if (!convertROMSCoords(xi_rho, eta_rho, s_rho, eta_u, eta_v, eta_psi, s_w))
	{
		//download_progress_end();
		return false;
	}

	//  set depth from sigma
	//download_progress_update("Setting Depth from Sigma Values", 0.4);

	int iSigmaVar = findVar("Cs_r");
	scale_var(iSigmaVar, -1.0, 0.0);

	int iBottomVar = findVar("h");
	setVarNoValue(iBottomVar, -99999.0);

	int iDepthVar = newVar("depth", DATA_DEPTH, getVarDims(iBottomVar));
	setVarNoValue(iDepthVar, -99999.0);
	bool bSuccess = false;
	if (setDepthFromSigma(iSigmaVar, iBottomVar, iDepthVar))
	{
		//download_progress_update("Adding Sealevel and Bottom Layer", 0.6);

		//  add extra layer at sea level and sea floor
		int s_rho = findDim("s_rho");
		int iLevels = getDimSize(s_rho);
		vLevels.push_back(0);
		for (int i = 0; i < iLevels; i++) 
			vLevels.push_back(i);
		vLevels.push_back(iLevels - 1);
		if (resetDim(s_rho, vLevels))
		{
			vector<float>	&fp = getVarFData(iDepthVar);
			vector<float>	&hp = getVarFData(iBottomVar);
			if (fp.size() != 0 || hp.size() != 0)
			{
				int iLevelSize = getVarSize(iBottomVar);
				int iTopRow = (iLevels + 1) * iLevelSize;
				for (int i = 0; i < iLevelSize; i++)
				{
					fp[iTopRow + i] = 0;
					fp[i] = hp[i];
				}
				bSuccess = true;
			}
		}
	}

	if (!bSuccess)
	{
		//download_progress_end();
		return false;
	}

	//  mask out invalid values
	//download_progress_update("Masking Variables", 0.8);

	int iMaskVar = findVar("mask_rho");
	setVarNoValue(iMaskVar, 0.0);
	for (int i = 0; i < getVarCnt(); i++)
		if (getVarType(i) == DATA_SCALAR && getVarSize(i) >= getVarSize(iMaskVar))
			maskVar(i, iMaskVar);
	
	//download_progress_update("Removing Unused Dimensions", 0.9);

	int iDimDel[] = { 12, 11, 10, 8, 7, 6, 5, 3, 2, 1, -1 };
	for (int j = 0; iDimDel[j] != -1; j++)
	{
		for (int i = getVarCnt() - 1; i >= 0; i--)
		{
			vector<int> &vDims = getVar(i).editDims();
			for (int k = 0; k < vDims.size(); k++)
			{
				if (vDims[k] == iDimDel[j])
				{
					delVar(i);
					break;
				}
				else if (vDims[k] > iDimDel[j])
					vDims[k]--;
			}
		}

		delDim(iDimDel[j]);
	}

	setDimName(0, "lon");
	setDimName(1, "lat");
	setDimName(2, "level");
	setDimName(3, "time");
	resetCoordVars();

	//download_progress_end();
	return true;
}

/* =============================================================================
    COVNERT REGULAR GRID
 =============================================================================== */
bool CData::convertFERRET()
{
	int iLatVar = getLatVar();
	int iLonVar = getLonVar();
	if (getVarDims(iLatVar).size() * getVarDims(iLonVar).size() != 1)
		return false;

	cout << ("Converting ferret grid to COVE format");

	//  get the current data
	vector<double>	lat = getVarDData(iLatVar);
	vector<double>	lon = getVarDData(iLonVar);

	//  reset var dims
	int				iLatSize = 0;
	for (int i = 1; i < lat.size(); i++, iLatSize++)
		if (lat[i] != lat[i - 1])
			break;

	int			iLonSize = lon.size() / iLatSize;
	int			iLatDim = newDim("lat", iLatSize);
	int			iLonDim = newDim("lon", iLonSize);
	vector<int> vDims;
	vDims.push_back(iLatDim);
	vDims.push_back(iLonDim);
	for (int i = 0; i < getVarCnt(); i++) 
		setVarDims(i, vDims);

	return true;
}

/* =============================================================================
 =============================================================================== */
bool CData::convertRegularGrid()
{
	int iLatVar = getLatVar();
	int iLonVar = getLonVar();
	int iDepthVar = getDepthVar();
	if (iDepthVar == -1 && iLatVar != -1 && iLonVar != -1)
	{
		int idim = newDim("depth", 1);
		vector<int> vDims(1, idim);
		iDepthVar = newVar("depth", DATA_DEPTH, vDims);
		setVarFData(iDepthVar, 0);
	}

	int varList[] = { iDepthVar, iLatVar, iLonVar };
	if (getVarDims(iDepthVar).size() * getVarDims(iLatVar).size() * getVarDims(iLonVar).size() != 1) 
		return false;

	cout << ("Converting regular grid to COVE format");

	//  get the current data
	vector<float>	depth = getVarFData(iDepthVar);
	vector<double>	lat = getVarDData(iLatVar);
	vector<double>	lon = getVarDData(iLonVar);

	//  reset var dims
	int				iDepthDim = getVarDims(iDepthVar)[0];
	int				iLatDim = getVarDims(iLatVar)[0];
	int				iLonDim = getVarDims(iLonVar)[0];
	int				dimList[] = { iDepthDim, iLatDim, iLonDim };
	int				iDims[3][4] = { { 0, 1, 2, -1 }, { 1, 2, -1 }, { 1, 2, -1 } };
	for (int i = 0; i < 3; i++)
	{
		vector<int> vDims;
		for (int j = 0; iDims[i][j] != -1; j++) 
			vDims.push_back(dimList[iDims[i][j]]);
		setVarDims(varList[i], vDims);
		if (i == 0)
			initVarFData(varList[i]);
		else
			initVarDData(varList[i]);
	}

	//  reset depth
	int		iCnt = 0;
	float	*pDepth = &(getVarFData(iDepthVar)[0]);
	for (int i = 0; i < depth.size(); i++)
		for (int j = 0; j < lat.size() * lon.size(); j++, iCnt++) 
			pDepth[iCnt] = depth[i];

	//  lat and lon
	double	*pLat = &(getVarDData(iLatVar)[0]);
	double	*pLon = &(getVarDData(iLonVar)[0]);
	iCnt = 0;
	for (int i = 0; i < lat.size(); i++)
		for (int j = 0; j < lon.size(); j++, iCnt++)
		{
			pLat[iCnt] = lat[i];
			pLon[iCnt] = lon[j];
		}

	return true;
}

/* =============================================================================
 =============================================================================== */
bool CData::checkRegularGrid()
{
	if (!setCoordVars(getLargestOfType(DATA_SCALAR)))
		return false;

	int iLatVar = getLatVar();
	int iLonVar = getLonVar();
	if (iLatVar == -1 || iLatVar == -1)
		return false;

	int			iFormat = getDataFormat();
	vector<int> vDims = getVarDims(iLonVar);
	if ((iFormat != DFORMAT_3DMODEL && iFormat != DFORMAT_2DSURFACE) || vDims.size() < 2) 
		return false;

	vector<double>	lat = getVarDData(iLatVar);
	vector<double>	lon = getVarDData(iLonVar);
	int				iLatCnt = getLatCnt();
	int				iLonCnt = getLonCnt();
	for (int i = 0; i < iLatCnt; i++)
	{
		for (int j = 0; j < iLonCnt; j++)
		{
			if (i > 0 && lon[i * iLonCnt + j] != lon[(i - 1) * iLonCnt + j])
				return false;
			if (j > 0 && lat[i * iLonCnt + j] != lat[i * iLonCnt + j - 1])
				return false;
		}
	}

	return true;
}

/* =============================================================================
    VERTICAL SECTION
 =============================================================================== */
bool CData::createVertSectData(const CData &srcData, int iVar)
{

	//  figure out number of depth levels to create
	int iTimeVar = srcData.getTimeVar();
	int iDepthVar = srcData.getDepthVar();
	int iLatVar = srcData.getLatVar();
	int iLonVar = srcData.getLonVar();

	if (srcData.getDataFormat() != DFORMAT_PATH
		||	iVar < 0
		||	iTimeVar == -1
		||	iDepthVar == -1
		||	iLatVar == -1
		||	iLonVar == -1)
		return setLastError("Invalid Parameters");

	float	mindepth = max((float) floor(srcData.getVarMin(iDepthVar)), 1.0f);
	float	maxdepth = ceil(srcData.getVarMax(iDepthVar));
	if (srcData.getVarMin(iDepthVar) == srcData.getVarMax(iDepthVar) || mindepth >= maxdepth)
		return setLastError("Unable to create vertical section");

	int				iLevels = min(50, (int) (maxdepth - mindepth));
	int				iDepthSize = srcData.getVarSize(iDepthVar);
	const vector<float>	&varDepth = srcData.getVar(iDepthVar).getFData();

	const vector<float>	&varData = srcData.getVar(iVar).getFData();
	float			fNoVal = srcData.getVarNoValue(iVar);
	int				size = srcData.getVarSize(iVar);
	int				newsize = size * iLevels;
	if (newsize == 0)
		return 0;

	const vector<double>	&lat = srcData.getVar(iLatVar).getDData();
	const vector<double>	&lon = srcData.getVar(iLonVar).getDData();
	const vector<double>	&time = srcData.getVar(iTimeVar).getDData();
	const vector<float> depth(iDepthSize, maxdepth);

	if (!createPathData(time, lat, lon, depth, iLevels))
		return setLastError("Unable to create Vertical Section data set");

	vector<int> vDims = getVarDims(1);
	int			iNewVar = newVar(srcData.getVarName(iVar), DATA_SCALAR, vDims);
	if (!editVar(iNewVar).initFDataMem())
		return setLastError("Error allocating memory");

	vector<float>	&vData = getVar(iNewVar).editFData();
	vector<float>	&vDepth = getVar(1).editFData();

	//  create positions and put current data in grid to create base points
	Vec3d			prevpos = vl_0;
	int				iLastNdx = -1;
	float			fLastVal = fNoVal;
	float			fLastPntDist = 0;
	for (int i = 0; i < size; i++)
	{
		int		ndx = i * iLevels;
		Vec3d	pos = getDataPos(iNewVar, ndx);
		bool	bSet = false;
		for (int j = 0; j < iLevels; j++, ndx++)
		{
			float	depth = vDepth[ndx];
			vData[ndx] = fNoVal;

			//  fill in the data values
			if (!bSet && depth > varDepth[i])
			{
				float	fPntDist = abs(depth - varDepth[i]);
				float	fNewVal = varData[i];
				if (iLastNdx == j && fPntDist > fLastPntDist)
					vData[ndx] = fLastVal;
				else
				{
					vData[ndx] = fNewVal;
					fLastPntDist = fPntDist;
				}

				bSet = true;

				//  interpolate across vertical gaps
				if (iLastNdx != -1 && abs(iLastNdx - j) > 1)
				{
					int		iStep = iLastNdx < j ? 1 : -1;
					float	fStep = (float) iStep * (fLastVal - fNewVal) / (float) (iLastNdx - j);
					float	fVal = fLastVal + fStep;
					for (int k = iLastNdx + iStep; k != j; k += iStep, fVal += fStep) 
						vData[ndx - iLevels + k] = fVal;
				}

				iLastNdx = j;
				fLastVal = fNewVal;
			}
		}
	}

	//  now fill in horizontal gaps
	for (int j = 0; j < iLevels; j++)
	{
		for (int i = 0; i < size; i++)
		{
			if (vData[i * iLevels + j] == fNoVal)
				continue;

			int iStart = i;
			int iEnd = i + 1;
			for (; iEnd < size; iEnd++)
				if (vData[iEnd * iLevels + j] != fNoVal)
					break;
			if (iEnd == size)		//  no match in rest of data at this depth
				break;
			if (iEnd == iStart + 1)	//  no points to interpolate
				continue;
			if (iEnd < iStart + 500) //  limit matching to 500 data point span
			{

				// interpolate between two collected data points at same depth
				float	fStartVal = vData[iStart * iLevels + j];
				float	fEndVal = vData[iEnd * iLevels + j];
				float	fInc = (fEndVal - fStartVal) / (float) (iEnd - iStart);
				for (int k = iStart + 1, iCur = 1; k < iEnd; k++, iCur++)
					vData[k * iLevels + j] = fStartVal + (float) iCur * fInc;
			}

			i = iEnd - 1;
		}
	}

	return true;
}

/* =============================================================================
    check if the point is in the triangle using Barycentric Technique
 =============================================================================== */
static bool PointInTriangle(Vec3d P, Vec3d A, Vec3d B, Vec3d C, double &u, double &v)
{
	// P = A + u * (C - A) + v * (B - A)
	// Compute vector
	Vec3d	v0 = (B - A) * Vec3d(1, 1, 0);
	Vec3d	v1 = (C - A) * Vec3d(1, 1, 0);
	Vec3d	v2 = (P - A) * Vec3d(1, 1, 0);

	//  Compute dot products
	double	dot00 = dot(v0, v0);
	double	dot01 = dot(v0, v1);
	double	dot02 = dot(v0, v2);
	double	dot11 = dot(v1, v1);
	double	dot12 = dot(v1, v2);

	//  Compute barycentric coordinates
	double	invDenom = 1 / (dot00 * dot11 - dot01 * dot01);
	u = (dot11 * dot02 - dot01 * dot12) * invDenom;
	v = (dot00 * dot12 - dot01 * dot02) * invDenom;

	//  Check if point is in triangle
	return((u >= 0) && (v >= 0) && (u + v <= 1));
}

/* =============================================================================
    check if the point is in the cell by looking at top and bottom planes
 =============================================================================== */
bool PointInCell(Vec3d pos, Vec3d *pCrnr)
{

	// assume model cell lat/lon are equivilent on top/bottom plane - depths vary at all point
	double		u, v;
	const int	tri[4][3] = { { 0, 1, 3 }, { 0, 3, 2 }, { 4, 5, 7 }, { 4, 7, 6 } };

	for (int i = 0; i < 2; i++)
	{
		if (!PointInTriangle(pos, pCrnr[tri[i][0]], pCrnr[tri[i][1]], pCrnr[tri[i][2]], u, v))
			continue;

		Vec3d	pnt_0 = pCrnr[tri[i][0]] + u * (pCrnr[tri[i][2]] - pCrnr[tri[i][0]]) + v *
			(pCrnr[tri[i][1]] - pCrnr[tri[i][0]]);
		Vec3d	pnt_1 = pCrnr[tri[i + 2][0]] + u * (pCrnr[tri[i + 2][2]] - pCrnr[tri[i + 2][0]]) + v *
			(pCrnr[tri[i + 2][1]] - pCrnr[tri[i + 2][0]]);

		//  check if depth is above the lower plane and below the top plane
		return(pos[2] >= min(pnt_0[2], pnt_1[2]) && pos[2] <= max(pnt_0[2], pnt_1[2]));
	}

	return false;
}

/* =============================================================================
    Interpolate a new value given a cell and var in the data source and an
    input point
 =============================================================================== */
float PointInterp(Vec3d pos, Vec3d *pCrnr, float *pValues)
{

	// get the corners, determine barycentric coordinates to point and interpolate value between triangles
	double		u, v;
	const int	tri[4][3] = { { 0, 1, 3 }, { 0, 3, 2 }, { 4, 5, 7 }, { 4, 7, 6 } };
	for (int i = 0; i < 2; i++)
	{
		if (!PointInTriangle(pos, pCrnr[tri[i][0]], pCrnr[tri[i][1]], pCrnr[tri[i][2]], u, v))
			continue;

		Vec3d	pnt_top = pCrnr[tri[i][0]] + u * (pCrnr[tri[i][2]] - pCrnr[tri[i][0]]) + v *
			(pCrnr[tri[i][1]] - pCrnr[tri[i][0]]);
		Vec3d	pnt_bot = pCrnr[tri[i + 2][0]] + u * (pCrnr[tri[i + 2][2]] - pCrnr[tri[i + 2][0]]) + v *
			(pCrnr[tri[i + 2][1]] - pCrnr[tri[i + 2][0]]);
		float	val_top = pValues[tri[i][0]] + u * (pValues[tri[i][2]] - pValues[tri[i][0]]) + v *
			(pValues[tri[i][1]] - pValues[tri[i][0]]);
		float	val_bot = pValues[tri[i + 2][0]] + u * (pValues[tri[i + 2][2]] - pValues[tri[i + 2][0]]) + v *
			(pValues[tri[i + 2][1]] - pValues[tri[i + 2][0]]);
		if (val_top == val_bot)
			return val_top;

		float	offset = (pnt_top[2] - pos[2]) / (pnt_top[2] - pnt_bot[2]) * (val_top - val_bot);
		return val_top - offset;
	}

	return 0;
}

/* =============================================================================
    Figure out min and max of a point list
 =============================================================================== */
void PointMinMax(vector<Vec3d> &pCrnr, int iCnt, Vec3d &tmin, Vec3d &tmax)
{
	tmin = tmax = pCrnr[0];
	for (int i = 1; i < iCnt; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			if (pCrnr[i][j] < tmin[j]) tmin[j] = pCrnr[i][j];
			if (pCrnr[i][j] > tmax[j]) tmax[j] = pCrnr[i][j];
		}
	}
}

/* =============================================================================
    Figure out min and max of a point list
 =============================================================================== */
void PointMinMax(vector<int> &pcell, vector<Vec3d> &pmin, vector<Vec3d> &pmax, Vec3d &tmin, Vec3d &tmax)
{
	tmin = pmin[0];
	tmax = pmax[0];
	for (int i = 1; i < pmin.size(); i++)
	{
		if (pcell[i] == -1)
			continue;
		for (int j = 0; j < 3; j++)
		{
			if (pmin[i][j] < tmin[j]) tmin[j] = pmin[i][j];
			if (pmax[i][j] > tmax[j]) tmax[j] = pmax[i][j];
		}
	}
}

/* =============================================================================
    BSP TREE
 =============================================================================== */
void BSPNode::create_tree(int iLeafCntMax)
{
	PointMinMax(m_cell, m_cellmin, m_cellmax, m_min, m_max);

	int iLeafCnt = m_cell.size();
	int iDepthMax = 15;
	int iDepth;
	for (iDepth = 5; (iLeafCnt > iLeafCntMax) && (iDepth < iDepthMax); iDepth++)
	{
		create_nodes(iDepth, m_cellmax, m_cellmin);
		iLeafCnt = get_max_leaf_cnt(0);
	}

	if (g_Env.m_Verbose)
	{
		ostringstream	s;
		s <<
			"BSP tree created from " <<
			m_cell.size() <<
			" cells.  Max cells in a leaf is " <<
			iLeafCnt <<
			" at level " <<
			iDepth <<
			endl;
		cout << (s.str());
	}
}

/* =============================================================================
 =============================================================================== */
void BSPNode::create_nodes(int iDepth, vector<Vec3d> &p_cellmax, vector<Vec3d> &p_cellmin)
{

	//  check termination conditions
	if (iDepth == 0)
		return;
	if (m_cell.size() <= 1)
		return;

	//  if returning use current tree
	if (m_child[0])
	{
		m_child[0]->create_nodes(iDepth - 1, p_cellmax, p_cellmin);
		m_child[1]->create_nodes(iDepth - 1, p_cellmax, p_cellmin);
		return;
	}

	//  simple check for degenerate split axis
	for (int i = 0; i < 2; i++)
	{
		if (m_max[m_axis] != m_min[m_axis])
			break;
		m_axis = (m_axis + 1) % 2;
	}

	if (m_max[m_axis] == m_min[m_axis])
		return;

	//  split at midpoint of cells along split axis
	double	splitval = (m_min[m_axis] + m_max[m_axis]) / 2 - m_min[m_axis];
	Vec3d	v_split = vl_0;
	v_split[m_axis] = splitval;

	int child_axis = (m_axis + 1) % 2;

	//  create the children and recurse
	m_child[0] = new BSPNode();
	m_child[0]->m_min = m_min;
	m_child[0]->m_max = m_max - v_split;
	m_child[1] = new BSPNode();
	m_child[1]->m_min = m_min + v_split;
	m_child[1]->m_max = m_max;
	for (int i = 0; i < 2; i++)
	{
		BSPNode *pChild = m_child[i];
		pChild->m_axis = child_axis;
		for (int c = 0; c < m_cell.size(); c++)
		{
			if (m_cell[c] == -1)
				continue;
			if (p_cellmin[m_cell[c]][m_axis] > pChild->m_max[m_axis]
				||	p_cellmax[m_cell[c]][m_axis] < pChild->m_min[m_axis])
					continue;
			pChild->m_cell.push_back(m_cell[c]);
		}

		pChild->create_nodes(iDepth - 1, p_cellmax, p_cellmin);
	}
}

/* =============================================================================
 =============================================================================== */
void BSPNode::deleteNodes()
{
	for (int i = 0; i < 2; i++)
		if (m_child[i])
		{
			m_child[i]->deleteNodes();
			delete m_child[i];
			m_child[i] = NULL;
		}
}

/* =============================================================================
 =============================================================================== */
int BSPNode::get_max_leaf_cnt(int iMax) const
{
	if (m_child[0] == NULL)
	{
		if (m_cell.size() > iMax) 
			iMax = m_cell.size();
	}
	else
	{
		iMax = m_child[0]->get_max_leaf_cnt(iMax);
		iMax = m_child[1]->get_max_leaf_cnt(iMax);
	}

	return iMax;
}

/* =============================================================================
 =============================================================================== */
bool BSPNode::contains(Vec3d pnt) const
{
	for (int i = 0; i < 3; i++)
		if (m_max[i] < pnt[i] || m_min[i] > pnt[i])
		return false;
	return true;
}

/* =============================================================================
 =============================================================================== */
void BSPNode::get_cell_list(Vec3d pnt, vector<int> &clist) const
{
	if (m_child[0] == NULL)
		clist = m_cell;
	else if (m_child[0]->m_max[m_axis] > pnt[m_axis])
		m_child[0]->get_cell_list(pnt, clist);
	else
		m_child[1]->get_cell_list(pnt, clist);
}

/* =============================================================================
 =============================================================================== */
void BSPNode::fixupChildNodes()
{
	for (int i = 0; i < 2; i++)
	{
		if (m_child[i])
		{
			BSPNode *pOldChild = m_child[i];
			m_child[i] = new BSPNode();
			*m_child[i] = *pOldChild;
			m_child[i]->fixupChildNodes();
		}
	}
}

/* =============================================================================
 =============================================================================== */
void CData::setBSPTree(CDataPtr hOldData)
{
	m_BSPTree = hOldData->m_BSPTree;
	m_BSPTree.fixupChildNodes();
	m_BSPGrid = hOldData->m_BSPGrid;
	m_BSPCellNdx = hOldData->m_BSPCellNdx;
	m_BSPDims = hOldData->m_BSPDims;
}

/* =============================================================================
    DATA REGRIDDING ROUTINES Create a BSP tree for the model grid
 =============================================================================== */
bool CData::createModelBSPTree(int iSrcVar)
{
	if (getRegularGrid())
		return true;
	if (matchBSPDims(getVarDims(iSrcVar)))
		return true;

	cout << ("Creating BSP Tree");

	//  get all the data positions
	int size = getTimeSliceSize(iSrcVar);
	try
	{
		m_BSPGrid.resize(size);
	}
	catch(std::bad_alloc const &)
	{
		return setLastError("BSP memory allocation failure");
	}

	vector<int> vDims(getVarDims(iSrcVar).size(), -1);
	vDims[0] = 0;
	getVarPosSlice(iSrcVar, m_BSPGrid, vDims);

	//  generate corner indices to each of the cells
	int iLatCnt = getLatCnt();
	int iLonCnt = getLonCnt();
	int iDepthCnt = getDepthCnt();
	int iNextRow = iLonCnt;
	int iNextLevel = iDepthCnt == 1 ? 0 : iLonCnt * iLatCnt;
	int iCellCnt = (iLatCnt - 1) * (iLonCnt - 1) * max(1, (iDepthCnt - 1));
	try
	{
		m_BSPCellNdx.resize(iCellCnt * 8);
	}
	catch(std::bad_alloc const &)
	{
		return setLastError("BSP memory allocation failure");
	}

	int iNdx = 0, iCellNdx = 0;
	for (int i = 0; i < max(1, (iDepthCnt - 1)); i++)
	{
		for (int j = 0; j < iLatCnt - 1; j++)
		{
			for (int k = 0; k < iLonCnt - 1; k++, iCellNdx++)
			{
				m_BSPCellNdx[iNdx++] = iCellNdx;
				m_BSPCellNdx[iNdx++] = iCellNdx + 1;
				m_BSPCellNdx[iNdx++] = iCellNdx + iNextRow;
				m_BSPCellNdx[iNdx++] = iCellNdx + iNextRow + 1;
				m_BSPCellNdx[iNdx++] = iCellNdx + iNextLevel;
				m_BSPCellNdx[iNdx++] = iCellNdx + iNextLevel + 1;
				m_BSPCellNdx[iNdx++] = iCellNdx + iNextRow + iNextLevel;
				m_BSPCellNdx[iNdx++] = iCellNdx + iNextRow + iNextLevel + 1;
			}

			iCellNdx++;
		}

		iCellNdx += iLonCnt;
	}

	//  create a BSP tree based on these points
	m_BSPTree.deleteNodes();
	vector<Vec3d> pCrnr(8, vl_0);
	for (int j = 0; j < iCellCnt; j++)
	{
		int		ndx[8];
		Vec3d	tmin, tmax;
		bool	bValid = true;
		for (int n = 0; n < 8; n++)
		{
			ndx[n] = m_BSPCellNdx[j * 8 + n];
			pCrnr[n] = m_BSPGrid[ndx[n]];
			if (!isDataPosValid(iSrcVar, pCrnr[n]))
			{
				bValid = false;
				break;
			}
		}

		m_BSPTree.appendCell(bValid ? j : -1);
		if (bValid)
			PointMinMax(pCrnr, 8, tmin, tmax);
		else
			tmin = tmax = vl_0;
		m_BSPTree.appendCellmin(tmin);
		m_BSPTree.appendCellmax(tmax);
	}

	if (m_BSPTree.m_cell.size() == 0) 
		return setLastError("Unable to create BSP tree for this data set.");
	m_BSPTree.create_tree(iCellCnt / 1000);
	setBSPDims(getVarDims(iSrcVar));
	return true;
}

/* =============================================================================
 =============================================================================== */
template<class T>
int binSearch(double val, int iStart, int iEnd, const vector<T> &fdata, int iOffset, int iStride, bool bDataInc)
{
	if (iEnd < iStart)
		return -1;

	int iMid = (iEnd + iStart) / 2;
	int ndx = iMid * iStride + iOffset;
	if (bDataInc)
	{
		if ((val >= fdata[ndx] && val < fdata[ndx + iStride]))
			return iMid;
		if (val < fdata[ndx])
			iEnd = iMid - 1;
		else
			iStart = iMid + 1;
	}
	else
	{
		if ((val <= fdata[ndx] && val > fdata[ndx + iStride]))
			return iMid;
		if (val > fdata[ndx])
			iEnd = iMid - 1;
		else
			iStart = iMid + 1;
	}

	return binSearch(val, iStart, iEnd, fdata, iOffset, iStride, bDataInc);
}

/* =============================================================================
    See if point is in a grid cell using bsp tree or regular grid
 =============================================================================== */
int CData::getGridCell(Vec3d pos) const
{
	if (getRegularGrid())
	{
		int iLat = getLatVar();
		int iLon = getLonVar();
		int iDepth = getDepthVar();
		if (iDepth <= 1)
			return -1;

		vector<int> vDims = getVarDims(iDepth);
		int			xSize = getLonCnt();
		int			ySize = getLatCnt();
		int			zSize = getDepthCnt();
		int			iLevSize = xSize * ySize;
		int			xGrid = -1, yGrid = -1, zGrid = -1;
		if (getVar(iLon).getDData()[xSize - 1] == pos[1])	//  check if end val
			xGrid = xSize - 2;
		else
			xGrid = binSearch(pos[1], 0, xSize - 2, getVar(iLon).getDData(), 0, 1, true);
		if (getVar(iLon).getDData()[iLevSize - 1] == pos[0]) //  check if end val
			yGrid = ySize - 2;
		else
			yGrid = binSearch(pos[0], 0, ySize - 2, getVar(iLat).getDData(), 0, xSize, true);
		if (xGrid == -1 || yGrid == -1)
			return -1;

		int iOffset = yGrid * xSize + xGrid;
		if (getVar(iDepth).getFData()[iLevSize * (zSize - 1) + iOffset] == -pos[2])	//  check if end val
			zGrid = max(0, zSize - 2);
		else
			zGrid = binSearch(-pos[2], 0, zSize - 2, getVar(iDepth).getFData(), iOffset, iLevSize, false);
		if (zGrid == -1) 
			return -1;
		return zGrid * iLevSize + iOffset;
	}
	else
	{
		if (!m_BSPTree.contains(pos))
			return -1;

		vector<int> clist;
		m_BSPTree.get_cell_list(pos, clist);

		int		ndx[8];
		Vec3d	pCrnr[8];
		for (int j = 0; j < clist.size(); j++)
		{
			for (int n = 0; n < 8; n++)
			{
				ndx[n] = m_BSPCellNdx[clist[j] * 8 + n];
				pCrnr[n] = m_BSPGrid[ndx[n]];
			}

			if (PointInCell(pos, pCrnr)) 
				return clist[j];
		}
	}

	return -1;
}

/* =============================================================================
    Get the grid cell value by interpolating over space and time
 =============================================================================== */
double CData::getGridValue(int iSrcVar, Vec3d pos, double time, int iCell, int ndx_time)
{
	if (iSrcVar == -1)
		return 0;

	float	fNoVal = getVarNoValue(iSrcVar);
	if (iCell == -1 || ndx_time == -1)
		return fNoVal;

	int		iSrcTimeSteps = getTimeSteps();
	int		iSrcPntCnt = getVarSize(iSrcVar) / iSrcTimeSteps;

	//  set corners of grid cell and indexes into data
	Vec3d	pCrnr[8];
	int		ndx[8];
	if (getRegularGrid())
	{
		int			iLat = getLatVar();
		int			iLon = getLonVar();
		int			iDepth = getDepthVar();
		vector<int> vDims = getVarDims(iDepth);
		int			xSize = getLonCnt();
		int			ySize = getLatCnt();
		int			zSize = getDepthCnt();
		int			iLevSize = xSize * ySize;
		int			iLevOffset = zSize == 1 ? 0 : iLevSize;
		pos[2] *= -1;

		int		iOffset = iCell;

		// same order as minmaxpos return list - min lat/lon/dep max lat/lon/de
		double	mpos[6] =
		{
			getVar(iLat).getDData()[iOffset % iLevSize],
			getVar(iLon).getDData()[iOffset % iLevSize],
			getVar(iDepth).getFData()[iOffset],
			getVar(iLat).getDData()[iOffset % iLevSize + xSize],
			getVar(iLon).getDData()[iOffset % iLevSize + 1],
			getVar(iDepth).getFData()[iOffset + iLevOffset]
		};
		int		order[8][3] =
		{
			{ 0, 1, 5 },
			{ 0, 4, 5 },
			{ 3, 1, 5 },
			{ 3, 4, 5 },
			{ 0, 1, 2 },
			{ 0, 4, 2 },
			{ 3, 1, 2 },
			{ 3, 4, 2 }
		};
		int		offsets[8] =
		{
			iOffset + iLevOffset,
			iOffset + 1 + iLevOffset,
			iOffset + xSize + iLevOffset,
			iOffset + xSize + 1 + iLevOffset,
			iOffset,
			iOffset + 1,
			iOffset + xSize,
			iOffset + xSize + 1
		};
		for (int i = 0; i < 8; i++)
		{
			ndx[i] = offsets[i];
			pCrnr[i] = Vec3d(mpos[order[i][0]], mpos[order[i][1]], mpos[order[i][2]]);
		}
	}
	else
	{
		for (int i = 0; i < 8; i++)
		{
			ndx[i] = m_BSPCellNdx[iCell * 8 + i];
			pCrnr[i] = m_BSPGrid[ndx[i]];
		}
	}

	//  get the data at the corners of the grid
	float	value1[8];
	float	value2[8];
	float	*pData = &(getVarFData(iSrcVar)[0]);
	for (int i = 0; i < 8; i++)
	{
		value1[i] = pData[ndx_time * iSrcPntCnt + ndx[i]];
		if (value1[i] == fNoVal)
		{
			return fNoVal;
		}

		if (iSrcTimeSteps > 1)
		{
			value2[i] = pData[(ndx_time + 1) * iSrcPntCnt + ndx[i]];
			if (value2[i] == fNoVal)
			{
				return fNoVal;
			}
		}
	}

	//  interpolate data
	float	val1 = PointInterp(pos, pCrnr, value1);
	if (iSrcTimeSteps == 1 || getDataTime(ndx_time) == time)
		return val1;

	float	val2 = PointInterp(pos, pCrnr, value2);
	double	cell_time1 = getDataTime(ndx_time);
	double	cell_time2 = getDataTime(ndx_time + 1);
	return val1 - (cell_time1 - time) / (cell_time1 - cell_time2) * (val1 - val2);
}

/* =============================================================================
    RESET VARIABLE DATA FROM A MODEL Given a source data set (such as a model)
    and variable, ;
    determine new values for a variable in the current data grid
 =============================================================================== */
bool CData::sampleModel(CData &Model, int iSrcVar, int iDestVar)
{
	ostringstream	s;

	g_Timers.startPerfTimer(CPU_PERF_TIMER);

	if (!loadVar(iDestVar) || !Model.loadVar(iSrcVar))
		return setLastError("Unable to load variable " + getVarName(iDestVar));

	if (Model.getVarDims(iSrcVar).size() < 3) 
		return setLastError("Data source does not appear to be a model.");

	Model.createModelBSPTree(iSrcVar);

	//  find which cell each grid point is in
	int				iDimCnt = getVarDims(iDestVar).size();
	bool			bTimeSlices = (iDimCnt > 2);

	int				iPntCnt = bTimeSlices ? getTimeSliceSize(iDestVar) : getVarSize(iDestVar);
	vector<Vec3d>	DestGrid;
	vector<int>		CellHit;
	try
	{
		DestGrid.resize(iPntCnt);
		CellHit.resize(iPntCnt);
	}
	catch(std::bad_alloc const &)
	{
		return setLastError("Memory allocation failure");
	}

	vector<int> vDims(getVarDims(iDestVar).size(), -1);
	vDims[0] = bTimeSlices ? 0 : -1;
	getVarPosSlice(iDestVar, DestGrid, vDims);

	int iTot = 0;
	for (int i = 0; i < iPntCnt; i++)
	{
		CellHit[i] = Model.getGridCell(DestGrid[i]);
		if (CellHit[i] != -1)
			iTot++;
	}

	if (g_Env.m_Verbose)
	{
		s << "Total data cells found: " << iTot << " out of " << iPntCnt;
		cout << (s.str());
	}

	//  interpolate data value in SourceData cell and store
	float	fSrcNoVal = Model.getVarNoValue(iSrcVar);
	float	fDestNoVal = getVarNoValue(iDestVar);

	int		iTimeSteps = getTimeSteps();
	iPntCnt = getVarSize(iDestVar) / iTimeSteps;

	double	dest_time = 0;

	int		iSrcTimeSteps = Model.getTimeSteps();
	double	src_time = Model.getDataTime(0);
	int		ndx_src_time = 0;
	iTot = 0;

	float	*pData = &(getVarFData(iDestVar)[0]);
	for (int t = 0; t < iTimeSteps; t++)
	{
		dest_time = getDataTime(t);
		if (iSrcTimeSteps > 1)	//  assume time in both is monotonically increasing
		{
			if (dest_time < src_time)
				continue;
			for (; ndx_src_time < iSrcTimeSteps - 1; ndx_src_time++)
				if (Model.getDataTime(ndx_src_time + 1) >= dest_time) 
					break;
			if (ndx_src_time == iSrcTimeSteps - 1)
				continue;
			src_time = Model.getDataTime(ndx_src_time);
		}

		//  fill in the data for the time slice
		for (int i = 0; i < iPntCnt; i++)
		{
			int		ndxDestPnt = bTimeSlices ? i : t * iPntCnt + i;
			Vec3d	pos = DestGrid[ndxDestPnt];
			int		iCurCell = CellHit[ndxDestPnt];

			double	val = Model.getGridValue(iSrcVar, pos, dest_time, iCurCell, ndx_src_time);
			if (val == fSrcNoVal)
				val = fDestNoVal;
			else
				iTot++;

			//  store data value in grid
			pData[t * iPntCnt + i] = val;
		}
	}

	//  output some data about new points
	updateVarMinMax(iDestVar);

	if (g_Env.m_Verbose)
	{
		s.str("");
		s << "Total data valid points interpolated: " << iTot << " out of " << iPntCnt * iTimeSteps << endl;
		s << "  min = " << m_Var[iDestVar].getMin() << "   max = " << m_Var[iDestVar].getMax() << endl;
		cout << (s.str());
	}

	//  cleanup
	g_Timers.stopPerfTimer(CPU_PERF_TIMER);

	return true;
}

/* =============================================================================
    ADVECT PARTICLES BASED ON MODEL
 =============================================================================== */
bool CData::setParticleDataValues(int iValue, int iParticleCntOrig)
{
	if (getDimCnt() < 1 || getDimSize(0) < 2 || getVarCnt() != 5)
		return false;

	int				iTimeSteps = getDimSize(0);
	int				iParticleCnt = getDimCnt() == 2 ? getDimSize(1) : 1;
	double			dStep = getVarDDataValue(0, 1) - getVarDDataValue(0, 0);

	vector<double>	&lat = getVarDData(1);
	vector<double>	&lon = getVarDData(2);
	vector<float>	&depth = getVarFData(3);
	float			fNoValue = getVarNoValue(4);

	for (int i = 0, iDst = 0, iSrc = 0; i < iTimeSteps; i++)
	{
		for (int j = 0; j < iParticleCnt; j++, iDst++, iSrc++)
		{
			Vec3d	pos = Vec3d(lat[iSrc], lon[iSrc], depth[iSrc]);
			if (pos[0] == NODATA)
				setVarFDataValue(4, iDst, fNoValue);
			else
			{
				double	val = NODATA;
				if (iValue == PART_INDEX)
					val = j % iParticleCntOrig;
				else if (iValue == PART_START)
					val = dStep * (j / iParticleCntOrig);
				else if (iValue == PART_CURTIME)
					val = dStep * i;
				else if (iValue <= PART_VEL_W)
				{

					//  determine speed in m/s
					int		ndx0 = (i < iTimeSteps - 1) ? iSrc : iSrc - iParticleCnt;
					int		ndx1 = ndx0 + iParticleCnt;
					Vec3d	pos0 = Vec3d(lat[ndx0], lon[ndx0], depth[ndx0]);
					Vec3d	pos1 = Vec3d(lat[ndx1], lon[ndx1], depth[ndx1]);
					if (!ISNODATA(pos0[0]) && !ISNODATA(pos1[0]))
					{
						Vec3d	vel = (Vec3d(pos1[0], pos1[1], 0.0) - Vec3d(pos0[0], pos0[1], 0.0)) * MetersPerDegree;
						vel[2] = pos1[2] - pos0[2];
						switch(iValue)
						{
						case PART_SPEED:	val = len(vel) / dStep; break;
						case PART_VEL_U:	val = vel[0] / dStep; break;
						case PART_VEL_V:	val = vel[1] / dStep; break;
						case PART_VEL_W:	val = vel[2] / dStep; break;
						}
					}
				}

				setVarFDataValue(4, iDst, val);
			}
		}
	}

	m_Var[4].updateMinMax();
	return true;
}

/* =============================================================================
 =============================================================================== */
bool CData::createParticleData
(
	CData			&Model,
	vector<int>		iFlowVars,
	vector<double>	ppos,
	vector<double>	time,
	double			cycle,
	double			step,
	int				iValue,
	bool			bForward,
	bool			bTimeRelease,
	bool			bMultiFile
)
{
	g_Timers.startPerfTimer(CPU_PERF_TIMER);

	//  verify parameters
	int iTimeSteps = step < 1.0 ? 0 : max(2, (int) ((time[1] - time[0]) / step) + 1);
	if (iTimeSteps < 1 || iFlowVars.size() == 0 || iFlowVars.size() > 3 || ppos.size() == 0 || time.size() == 0)
		return setLastError("At least one of the parameters is of size 0");

	int	iFlowVarCnt = 0;
	int		iSize = 0;
	for (int i = 0; i < iFlowVars.size(); i++)
	{
		if (iFlowVars[i] < 0)
			continue;
		if (iFlowVars[i] >= Model.getVarCnt() || Model.getVarDims(iFlowVars[i]).size() < 3)
			return setLastError("Error with passed advection force variables");
		if (iSize == 0)
			iSize = Model.getVarSize(iFlowVars[i]);
		else if (Model.getVarSize(iFlowVars[i]) != iSize)
			return setLastError("Invalid flow variable size " + getVarName(iFlowVars[i]));
		if (!Model.loadVar(iFlowVars[i])) 
			return setLastError("Unable to load variable " + getVarName(iFlowVars[i]));
		iFlowVarCnt++;
	}

	if (iFlowVarCnt == 0 || iSize == 0) 
		return setLastError("No valid momentum variables entered");

	int iFlowVar0 = -1;
	for (int i = 0; i < iFlowVars.size() && iFlowVar0 == -1; i++)
		if (iFlowVars[i] != -1) 
			iFlowVar0 = iFlowVars[i];

	float			fSrcNoVal = Model.getVarNoValue(iFlowVar0);
	Vec3d			posNoVal = Vec3d(NODATA, NODATA, NODATA);

	int				iParticleCntOrig = ppos.size() / 3;
	int				iParticleStep = bTimeRelease ? iTimeSteps - 1 : 1;
	int				iParticleCnt = iParticleCntOrig * iParticleStep;

	vector<Vec3d>	posList;
	try
	{
		posList.resize(iParticleCnt * iTimeSteps, posNoVal);
	}
	catch(std::bad_alloc const &)
	{
		return setLastError("Memory allocation failure");
	}

	if (!Model.createModelBSPTree(iFlowVar0))
		return false;

	vector<Vec3d>	posStart = vectord2vec(ppos);

	if (bMultiFile) bForward = true; //  only can move forward in time for now
	if (bMultiFile && getDimCnt() != 0)
	{

		//  copy current poslist in variable to poslist
		vector<double>	&lat = getVarDData(1);
		vector<double>	&lon = getVarDData(2);
		vector<float>	&depth = getVarFData(3);
		for (int i = 0; i < posList.size(); i++) 
			posList[i] = Vec3d(lat[i], lon[i], -depth[i]);
	}
	else
	{
		for (int i = 0; i < iParticleCnt; i++)
		{
			if (posStart[i % iParticleCntOrig] == posNoVal)	//  skip if turned off
				continue;

			int iOffset = bTimeRelease ? (i / iParticleCntOrig) * iParticleCnt : 0;
			posList[i + iOffset] = posStart[i % iParticleCntOrig];
		}
	}

	if (posStart.size() == 0 || posList.size() == 0)
		return setLastError("Bad position data.");

	bool	bFlowLines = (time[0] == time[1]);
	double	time_inc = bFlowLines ? 0 : step;
	double	time_step = step;
	if (!bForward)
	{
		time_inc *= -1;
		time_step *= -1;
	}

	int				iDataTimeSteps = Model.getTimeSteps();
	vector<double>	data_time = Model.getTimeMinMax();
	double			time_cur = bForward ? time[0] : time[1];

	cout << ("Advecting particles");

	int iTot = 0;
	int iCur = iParticleCnt;
	for (int t = 0; t < iTimeSteps - 1; t++, time_cur += time_inc)
	{
		double	advect_tm = time_cur;
		if (cycle != NO_TIME)
		{
			double	offset = fmod(advect_tm - data_time[0], cycle);
			advect_tm = offset >= 0 ? data_time[0] + offset : data_time[0] + cycle + offset;
			advect_tm = min(time[1], max(time[0], advect_tm));
		}

		//  figure out time slice in model
		int ndx_data_time = -1;
		if (advect_tm >= data_time[0] && advect_tm <= data_time[1])
		{
			for (ndx_data_time = 0; ndx_data_time < iDataTimeSteps - 1; ndx_data_time++)
				if (Model.getDataTime(ndx_data_time + 1) >= advect_tm)
					break;
		}

		if (ndx_data_time == -1 && bMultiFile)	//  if multifile then don't kill already created data
		{
			iCur += iParticleCnt;
			continue;
		}

		for (int i = 0; i < iParticleCnt; i++, iCur++)
		{
			if (posStart[i % iParticleCntOrig] == posNoVal)	//  skip if turned off
				continue;
			if (bTimeRelease && i / iParticleCntOrig > t)
				continue;

			//  set initial position
			int		ndxCur = t * iParticleCnt + i;
			int		ndxNew = ndxCur + iParticleCnt;
			Vec3d	pos = posList[ndxCur];
			posList[iCur] = pos;	//  default position at ndxNew

			//  find the cell that the position is in from the bsp tree
			int iCurCell;
			if (pos[0] != NODATA && (iCurCell = Model.getGridCell(pos)) != -1)
			{

				//  find [u,v,w] for position in the cell at the current time
				Vec3d	val = vl_0;
				for (int v = 0; v < iFlowVars.size(); v++)
					val[v] = Model.getGridValue(iFlowVars[v], pos, advect_tm, iCurCell, ndx_data_time);

				//  have u and v - can figure out new pos with newton's method
				if (val[0] == fSrcNoVal || val[1] == fSrcNoVal || val[2] == fSrcNoVal)
					posList[ndxNew] = posNoVal;
				else
				{
					posList[ndxNew] = posList[ndxCur] + val * time_step / MetersPerDegree;	//  assume m/s
					iTot++;
				}
			}
		}
	}

	//  output some data about new points
	if (g_Env.m_Verbose)
	{
		ostringstream	s;
		s << "Total data valid points interpolated: " << iTot << " out of " << posList.size() - iParticleCnt << endl;
		cout << (s.str());
	}

	if (!bForward)	//  turn the list around
	{
		vector<Vec3d>	posList2;
		try
		{
			posList2.resize(posList.size());
		}
		catch(std::bad_alloc const &)
		{
			return setLastError("Memory allocation failure");
		}

		int iCur = 0;
		for (int t = 0; t < iTimeSteps; t++)
		{
			for (int i = 0, iSrc = iParticleCnt * (iTimeSteps - t - 1); i < iParticleCnt; i++, iCur++, iSrc++)
			{
				if (posStart[i % iParticleCntOrig] == posNoVal)	//  skip if turned off
					continue;
				posList2[iCur] = posList[iSrc];
			}
		}

		posList = posList2;
	}

	double	dStart = time[0];

	//  make new data set dims and variables
	if (getDimCnt() == 0)
	{

		//  time dimension
		int iDim = newDim("time", iTimeSteps);
		setDimUnlimited(iDim, true);

		//  particle dimension
		if (iParticleCnt > 1) 
			newDim("particle", iParticleCnt);

		//  create variables
		char	*varlist[] = { "time", "latitude", "longitude", "depth", "particle", NULL };
		int		iType[] = { DATA_TIME, DATA_LAT, DATA_LON, DATA_DEPTH, DATA_SCALAR };
		for (int i = 0; varlist[i]; i++)
		{
			vector<int> vDims(1, 0);

			int iVar = newVar(varlist[i], iType[i], vDims);
			if (iType[i] == DATA_TIME)
				initVarDData(iVar);
			else
			{
				if (iParticleCnt > 1)
				{
					vDims.push_back(1);
					setVarDims(iVar, vDims);
				}

				if (getVarIsDData(iVar))
				{
					if (!initVarDData(iVar))
						return setLastError("Memory allocation failure");
				}
				else
				{
					if (!initVarFData(iVar))
						return setLastError("Memory allocation failure");
				}
			}
		}
	}

	//  load data from the particle list
	for (int i = 0, iDst = 0, iSrc = 0; i < iTimeSteps; i++, dStart += step)
	{
		setVarDDataValue(0, i, dStart);
		for (int j = 0; j < iParticleCnt; j++, iDst++, iSrc++)
		{
			if (posStart[j % iParticleCntOrig] == posNoVal)		//  skip if turned off
				continue;
			setVarDDataValue(1, iDst, posList[iSrc][0]);
			setVarDDataValue(2, iDst, posList[iSrc][1]);
			setVarFDataValue(3, iDst, -posList[iSrc][2]);
		}
	}

	setParticleDataValues(iValue, iParticleCntOrig);

	setExtents();

	g_Timers.stopPerfTimer(CPU_PERF_TIMER);

	return true;
}

/* =============================================================================
    COMBINE FILES combine two files of the same format and different time
    periods
 =============================================================================== */
bool CData::combine(CData &NewData)
{
	g_Timers.startPerfTimer(CPU_PERF_TIMER);

	//  check that dimensions and variables match up
	int		iTimeVar = getFirstOfType(DATA_TIME);
	int		iTimeDim = iTimeVar == -1 ? -1 : getVarDims(iTimeVar)[0];
	bool	bDimMatch = true;
	if (getDimCnt() != NewData.getDimCnt())
		bDimMatch = false;
	else
	{
		for (int i = 0; bDimMatch && i < getDimCnt(); i++)
			if (getDimName(i) != NewData.getDimName(i)) 
				bDimMatch = false;
	}

	if (!bDimMatch)
		return setLastError("Dimensions do no match.  Unable to combine files");

	//  resample dims on incoming file if necessary
	for (int i = 0; i < NewData.getDimCnt(); i++)
	{
		vector<float>	vDims = getDim(i).getCurSampling();
		vector<float>	vDimsNew = NewData.getDim(i).getCurSampling();
		if (vDims.size() > 0 && vDims != vDimsNew)
			NewData.resetDim(i, vDims);
	}

	//  if no time dimension just copy in or append new variables
	int		iNewTimeVar = NewData.getFirstOfType(DATA_TIME);
	bool	bTimeMatch = iTimeVar != -1 && compareVar(NewData, iNewTimeVar, iTimeVar);

	if (iTimeDim == -1 || bTimeMatch)
	{
		bool	bVarMatch = (NewData.getVarCnt() == getVarCnt());

		for (int i = 0; i < getVarCnt() && bVarMatch; i++)
		{
			int iNewVar = NewData.findVar(getVarName(i));
			if (iNewVar != i)
				bVarMatch = false;
		}

		if (bVarMatch && !bTimeMatch)	//  append - no time var
		{
			int iDim = -1;
			for (int i = 0; i < getVarCnt(); i++)
			{
				if (!loadVar(i) || !NewData.loadVar(i))
					return setLastError("Unable to load variable " + getVarName(i));

				if (getVarDims(i).size() > 0 && iDim == -1) 
					iDim = getVarDims(i)[0];
				if (getVarIsDData(i))
				{
					vector<double>	&vData1 = getVarDData(i);
					vector<double>	&vData2 = NewData.getVarDData(i);
					vData1.insert(vData1.end(), vData2.begin(), vData2.end());
					NewData.clearVarDData(i);
				}
				else
				{
					vector<float>	&vData1 = getVarFData(i);
					vector<float>	&vData2 = NewData.getVarFData(i);
					vData1.insert(vData1.end(), vData2.begin(), vData2.end());
					NewData.clearVarFData(i);
				}

				editVar(i).setReloadable(false);
			}

			//  now reset dims for new vars
			if (iDim != -1)
			{
				int iDimSize = getDimSize(iDim) + NewData.getDimSize(iDim);
				setDimSize(iDim, iDimSize);
			}
		}
		else	//  copy over new variable
		{
			for (int i = 0; i < NewData.getVarCnt(); i++)
			{
				int iVar = findVar(NewData.getVarName(i));
				if (iVar == -1) 
					copyVar(NewData, i, NewData.getVarName(i));
			}
		}

		setExtents();
		g_Timers.stopPerfTimer(CPU_PERF_TIMER);
		return true;
	}

	vector<vector<double> > pTimeData;

	//  for each var in new data with time slices
	for (int i = 0; i < getVarCnt(); i++)
	{

		//  if no time dimension, skip it
		if (getVarDims(i).size() == 0 || getVarDims(i)[0] != iTimeDim)
			continue;

		//  if can't find variable in incoming data, skip it
		int iNewVar = NewData.findVar(getVarName(i));
		if (iNewVar == -1)
			continue;

		//  make sure it's in memory
		g_Timers.startPerfTimer(IO_PERF_TIMER);
		if (!loadVar(i) || !NewData.loadVar(iNewVar))
			return setLastError("Unable to load variable " + getVarName(i));
		g_Timers.stopPerfTimer(IO_PERF_TIMER);
		g_Timers.startPerfTimer(CPU_PERF_TIMER);

		//  figure out what the data looks like
		bool	bTimeVar = getVarType(i) == DATA_TIME;
		bool	bDDataVar = getVarIsDData(i);
		int		iNewSize = getVarSize(i) + NewData.getVarSize(iNewVar);
		if (iNewSize == 0)
			continue;

		vector<float>	pfVals;
		vector<double>	pdVals;
		try
		{
			bDDataVar ? pdVals.resize(iNewSize) : pfVals.resize(iNewSize);
		}
		catch(std::bad_alloc const &)
		{
			return setLastError("Memory allocation failure");
		}

		int		iSliceSize = getTimeSliceSize(i);

		float	*pfData0;
		const float *pfData1, *pfSlice;
		double	*pdData0;
		const double *pdData1, *pdSlice;
		if (bDDataVar)
		{
			pdData0 = &(getVar(i).editDData()[0]);
			pdData1 = &(NewData.getVar(iNewVar).getDData()[0]);
		}
		else
		{
			pfData0 = &(getVar(i).editFData()[0]);
			pfData1 = &(NewData.getVar(iNewVar).getFData()[0]);
		}

		int ndx0 = 0, ndx1 = 0;
		int iWriteCnt = 0;
		int iTime = getTimeVar();
		if (iTime == -1) iTime = iTimeVar;

		int iNewTime = NewData.getTimeVar();
		if (iNewTime == -1) iNewTime = iNewTimeVar;
		while(ndx0 < getVarSize(iTime) || ndx1 < NewData.getVarSize(iNewTime))
		{

			//  get pointer to timeslice for next time step
			double	dTime0 = ndx0 < getVarSize(iTime) ? getVarDDataValue(iTime, ndx0) : MAX_DOUBLE;
			double	dTime1 = ndx1 < NewData.getVarSize(iNewTime) ? NewData.getVarDDataValue
				(
					iNewTime,
					ndx1
				) : MAX_DOUBLE;
			if (dTime0 <= dTime1)
			{
				if (bDDataVar)
				{
					pdSlice = pdData0 + iSliceSize * ndx0;
				}
				else
				{
					pfSlice = pfData0 + iSliceSize * ndx0;
				}

				ndx0++;
				if (dTime0 == dTime1) ndx1++;
			}
			else
			{
				if (bDDataVar)
				{
					pdSlice = pdData1 + iSliceSize * ndx1;
				}
				else
				{
					pfSlice = pfData1 + iSliceSize * ndx1;
				}

				ndx1++;
			}

			//  insert timeslice
			if (bDDataVar)
				memcpy(&(pdVals[iSliceSize * iWriteCnt]), pdSlice, iSliceSize * sizeof(double));
			else
				memcpy(&(pfVals[iSliceSize * iWriteCnt]), pfSlice, iSliceSize * sizeof(float));
			iWriteCnt++;
		}

		//  finally if a time var then hold onto it, otherwise update
		if (bTimeVar)
		{
			pdVals.resize(iSliceSize * iWriteCnt);
			pTimeData.push_back(pdVals);	//  save time data
		}
		else if (bDDataVar)
		{
			pdVals.resize(iSliceSize * iWriteCnt);
			getVarDData(i).swap(pdVals);
			if (NewData.getVar(iNewVar).getReloadable()) NewData.clearVarDData(iNewVar); //  clear out unneeded memory
		}
		else
		{
			pfVals.resize(iSliceSize * iWriteCnt);
			getVarFData(i).swap(pfVals);
			if (NewData.getVar(iNewVar).getReloadable()) NewData.clearVarFData(iNewVar); //  clear out unneeded memory
		}

		editVar(i).setReloadable(false);
	}

	//  reset time dim to size of time variable.
	setDimSize(iTimeDim, (int) pTimeData[0].size());

	//  and now that it's safe update the time variables
	int ndx = 0;
	for (int i = 0; i < getVarCnt(); i++)
	{
		if (getVarType(i) != DATA_TIME || getVarDims(i).size() == 0 || getVarDims(i)[0] != iTimeDim) 
			continue;
		getVarDData(i).swap(pTimeData[ndx]);
		ndx++;
	}

	setExtents();
	g_Timers.stopPerfTimer(CPU_PERF_TIMER);

	return true;
}

/* =============================================================================
    MODIFYING DIMS
 =============================================================================== */
void CDimension::updateCurSampling(vector<float> vNewDim)
{
	vector<float>	vOldDim = m_CurSampling;
	if (vOldDim.size() == 0)
		for (int i = 0; i < m_Size; i++) 
			vOldDim.push_back(i);
	m_CurSampling.clear();
	for (int i = 0; i < vNewDim.size(); i++)
	{
		float	fNdx = 0.0;
		if (vNewDim[i] >= vOldDim.size() - 1)
			fNdx = vOldDim.size() - 1;
		else
		{
			int ndx = (int) vNewDim[i];
			fNdx = vOldDim[ndx] + (vOldDim[ndx + 1] - vOldDim[ndx]) * (vNewDim[i] - ndx);
		}

		m_CurSampling.push_back(fNdx);
	}
}

/* =============================================================================
 =============================================================================== */
bool CData::cropDim(int iDim, int iStart, int iEnd, bool bInclude)
{
	int iCnt = getDimSize(iDim);
	if (iCnt == 0 || iStart < 0 || iEnd >= iCnt || iStart > iEnd)
		return setLastError("Unable to crop dimension as requested.");

	vector<float>	vNewDim;
	if (bInclude)
	{
		for (int i = iStart; i <= iEnd; i++) 
			vNewDim.push_back(i);
	}
	else
	{
		for (int i = 0; i < iCnt; i++)
			if (i < iStart || i > iEnd) vNewDim.push_back(i);
	}

	return resetDim(iDim, vNewDim);
}

/* =============================================================================
 =============================================================================== */
bool CData::resampleDim(int iDim, double dFactor)
{
	int iCnt = getDimSize(iDim);
	if (dFactor <= 0.0)
		return setLastError("Unable to resample selected dimension.");

	vector<float>	vNewDim;
	vNewDim.push_back(0);
	for (float d = 1.0 / dFactor; d <= iCnt - 1; d += 1.0 / dFactor) 
		vNewDim.push_back(d);
	return resetDim(iDim, vNewDim);
}

/* =============================================================================
 =============================================================================== */
bool CData::resetDim(int iDim, vector<float> vNewDim)
{
	g_Timers.startPerfTimer(CPU_PERF_TIMER);

	int iDimSize = getDimSize(iDim);
	int iNewDimSize = (int) vNewDim.size();
	if (iDimSize == 0 || iNewDimSize == 0)
		return setLastError("Unable to reset dimension as requested.");

	// save resampling if this is not a time variable - used by combine if available
	int iTimeVar = getFirstOfType(DATA_TIME);
	int iTimeDim = iTimeVar == -1 ? -1 : getVarDims(iTimeVar)[0];
	if (iDim != iTimeDim) editDim(iDim).updateCurSampling(vNewDim);

	for (int iVar = 0; iVar < getVarCnt(); iVar++)
	{

		//  check that variable is using the dimension
		bool	bMatched = false;
		int		iNewDataSize = 1;			//  new size of final data
		int		iSliceCnt = 1;				//  number of slices
		int		iSliceStep = 1;				//  stride in each slice
		int		iSliceSize = 1;				//  size of each old slice
		int		iNewSliceSize = 1;			//  size of each new slice
		for (int j = 0; j < getVarDims(iVar).size(); j++)
		{
			int d = getVarDims(iVar)[j];
			int iSize = getDimSize(d);
			if (bMatched)
			{
				iSliceStep *= iSize;		//  once matched grow step and slice size
				iSliceSize *= iSize;
				iNewSliceSize *= iSize;
			}

			if (d == iDim)
			{
				bMatched = true;
				iSliceCnt = iNewDataSize;	// lock in the slice cnt
				iSliceSize = iSize;
				iNewSliceSize = iNewDimSize;
				iSize = iNewDimSize;
			}

			iNewDataSize *= iSize;
		}

		if (!bMatched)
			continue;

		assert(iSliceSize * iSliceCnt == getVarSize(iVar));

		if (iNewDataSize == 0)
			continue;

		//  create new var mem
		bool	bDDataVar = getVarIsDData(iVar);
		if (iNewDataSize == 0)
			continue;

		vector<float>	pfDest;
		vector<double>	pdDest;
		try
		{
			bDDataVar ? pdDest.resize(iNewDataSize) : pfDest.resize(iNewDataSize);
		}
		catch(std::bad_alloc const &)
		{
			return setLastError("Memory allocation failure");
		}

		//  make sure it's in memory
		g_Timers.stopPerfTimer(CPU_PERF_TIMER);
		g_Timers.startPerfTimer(IO_PERF_TIMER);
		if (!loadVar(iVar)) 
			return setLastError("Unable to load variable " + getVarName(iVar));
		g_Timers.stopPerfTimer(IO_PERF_TIMER);
		g_Timers.startPerfTimer(CPU_PERF_TIMER);

		float			fNoValue = getVarNoValue(iVar);
		vector<float>	&pfSrc = getVarFData(iVar);
		vector<double>	&pdSrc = getVarDData(iVar);

		if (g_Env.m_Verbose)
		{
			ostringstream	s;
			s << getVarName(iVar) << ": old size : " << getVarSize(iVar) << "  new size : " << iNewDataSize;
			cout << (s.str());
		}

		//  iterate to copy over only requested slices
		for (int iSlice = 0; iSlice < iSliceCnt; iSlice++)
		{
			for (int i = 0; i < iNewDimSize; i++)
			{
				int		ndx = (int) vNewDim[i];
				float	factor2 = vNewDim[i] - ndx;
				float	factor1 = 1.0 - factor2;
				int		iDest = iSliceStep * i + iSlice * iNewSliceSize;
				int		iSrc1 = iSliceStep * ndx + iSlice * iSliceSize;

				//  no interpolation, just copy over slice
				if (factor2 == 0.0)
				{
					if (bDDataVar)
						memcpy(&pdDest[iDest], &pdSrc[iSrc1], iSliceStep * sizeof(double));
					else
						memcpy(&pfDest[iDest], &pfSrc[iSrc1], iSliceStep * sizeof(float));
					continue;
				}

				//  otherwise figure out new data point
				int iSrc0 = (ndx == 0) ? iSrc1 : iSliceStep * (ndx - 1) + iSlice * iSliceSize;
				int iSrc2 = iSliceStep * (ndx + 1) + iSlice * iSliceSize;
				int iSrc3 = (ndx == iDimSize - 2) ? iSrc2 : iSliceStep * (ndx + 2) + iSlice * iSliceSize;

				for (int v = 0; v < iSliceStep; v++)
				{
					if (bDDataVar)
					{
						pdDest[iDest + v] = pdSrc[iSrc1 + v] * factor1 + pdSrc[iSrc2 + v] * factor2;
					}
					else
					{
						float	val[4] = { pfSrc[iSrc0 + v], pfSrc[iSrc1 + v], pfSrc[iSrc2 + v], pfSrc[iSrc3 + v] };
						if (val[1] == fNoValue || val[2] == fNoValue)
							pfDest[iDest + v] = fNoValue;
						else
						{
							if (val[0] == fNoValue) val[0] = val[1];
							if (val[3] == fNoValue) val[3] = val[2];

							// assumes reasonably even spacing of data for splines
							CatmullRomValue(pfDest[iDest + v], factor2, val);

							// linear interpolation
							//  pfDest[iDst+v] = val[1] * factor1 + val[2] * factor2;
						}
					}
				}
			}
		}

		//  change data pointer
		if (bDDataVar)
			getVarDData(iVar).swap(pdDest);
		else
			getVarFData(iVar).swap(pfDest);

		editVar(iVar).setReloadable(false);
		setDirty(true);
	}

	//  update dimension
	setDimSize(iDim, iNewDimSize);
	setExtents();

	g_Timers.stopPerfTimer(CPU_PERF_TIMER);

	return true;
}

/* =============================================================================
    MODIFYING VARS
 =============================================================================== */
bool CData::scale_var(int iVar, double dScale, double dOffset)
{
	if (!getVarValid(iVar))
		return setLastError("Invalid variable index");

	float	fNoVal = getVarNoValue(iVar);
	if (getVarIsDData(iVar))
	{
		vector<double>	&pd = getVarDData(iVar);
		for (int i = 0; i < pd.size(); i++)
		{
			double	d = pd[i];
			if (!finite(d) || isnan(d))
				pd[i] = fNoVal;
			else if (d != fNoVal)
				pd[i] = (d * dScale) + dOffset;
		}
	}
	else if (m_Var[iVar].getTrnDataLoaded())
	{
		for (int r = 0; r < m_Var[iVar].getTrnDataRowCnt(); r++)
		{
			vector<float>	&pf = m_Var[iVar].editTrnDataRow(r);
			for (int i = 0; i < pf.size(); i++)
			{
				float	f = pf[i];
				if (!finite(f) || isnan(f))
					pf[i] = fNoVal;
				else if (f != fNoVal)
					pf[i] = (f * dScale) + dOffset;
			}
		}
	}
	else
	{
		if (!loadVar(iVar))
			return setLastError("Unable to load variable " + getVarName(iVar));

		vector<float>	&pf = getVarFData(iVar);
		for (int i = 0; i < pf.size(); i++)
		{
			float	f = pf[i];
			if (!finite(f) || isnan(f))
				pf[i] = fNoVal;
			else if (f != fNoVal)
				pf[i] = (f * dScale) + dOffset;
		}
	}

	editVar(iVar).setReloadable(false);
	return true;
}

/* =============================================================================
 =============================================================================== */
bool CData::scale_var(int iVar)
{
	int		iAttScale = -1, iAttOffset = -1;
	double	dScale = 1.0, dOffset = 0.0;
	if (m_ScaleAtt.size())
	{
		iAttScale = findAtt(iVar, m_ScaleAtt);
		if (iAttScale != -1)
		{
			dScale = getAttData(iVar, iAttScale)[0];
			getAttData(iVar, iAttScale)[0] = 1.0;
		}
	}

	if (m_OffsetAtt.size())
	{
		iAttOffset = findAtt(iVar, m_OffsetAtt);
		if (iAttOffset != -1)
		{
			dOffset = getAttData(iVar, iAttOffset)[0];
			getAttData(iVar, iAttOffset)[0] = 0;
		}
	}

	if (iAttScale == -1 && iAttOffset == -1)
		return true;
	scale_var(iVar, dScale, dOffset);
	return true;
}

/* =============================================================================
 =============================================================================== */
bool CData::scaleVar(int iVar, double dScale, double dOffset)
{
	g_Timers.startPerfTimer(CPU_PERF_TIMER);

	bool	bRet = scale_var(iVar, dScale, dOffset);
	g_Timers.stopPerfTimer(CPU_PERF_TIMER);
	return bRet;
}

/* =============================================================================
 =============================================================================== */
bool CData::compareVar(CData &Data, int iDataVar, int iVar)
{
	if (getVarType(iVar) != Data.getVarType(iDataVar) || getVarSize(iVar) != Data.getVarSize(iDataVar))
		return false;

	g_Timers.stopPerfTimer(CPU_PERF_TIMER);
	g_Timers.startPerfTimer(IO_PERF_TIMER);
	if (!loadVar(iVar) || !Data.loadVar(iDataVar))
		return setLastError("Unable to load variable " + getVarName(iVar) + " or " + Data.getVarName(iDataVar));
	g_Timers.stopPerfTimer(IO_PERF_TIMER);
	g_Timers.startPerfTimer(CPU_PERF_TIMER);

	int iSize = getVarSize(iVar);
	if (getVarIsDData(iVar))
	{
		const double *pd0 = &(getVar(iVar).getDData()[0]);
		const double *pd1 = &(Data.getVar(iDataVar).getDData()[0]);
		if (memcmp(pd0, pd1, iSize * sizeof(double)) != 0)
			return false;
	}
	else
	{
		const float	*pd0 = &(getVar(iVar).getFData()[0]);
		const float	*pd1 = &(Data.getVar(iDataVar).getFData()[0]);
		if (memcmp(pd0, pd1, iSize * sizeof(float)) != 0)
			return false;
	}

	editVar(iVar).setReloadable(false);
	g_Timers.stopPerfTimer(CPU_PERF_TIMER);

	return true;
}

/* =============================================================================
 =============================================================================== */
bool CData::cropVar(int iVar, float fMin, float fMax, bool bInclude)
{
	g_Timers.startPerfTimer(CPU_PERF_TIMER);

	if (!getVarValid(iVar) || getVarIsDData(iVar))
		return setLastError("Invalid variable passed to function");
	if (!loadVar(iVar)) 
		return setLastError("Unable to load variable " + getVarName(iVar));

	vector<float>	&pData = getVarFData(iVar);
	int				iVarSize = getVarSize(iVar);
	float			fNoVal = getVarNoValue(iVar);
	for (int i = 0; i < iVarSize; i++)
	{
		if (pData[i] == fNoVal) 
			continue;
		if (bInclude)
		{
			if ((fMin != fNoVal && pData[i] < fMin) || (fMin != fNoVal && pData[i] > fMax))
				pData[i] = fNoVal;
		}
		else
		{
			if ((fMin != fNoVal && pData[i] > fMin) && (fMin != fNoVal && pData[i] < fMax))
				pData[i] = fNoVal;
		}
	}

	editVar(iVar).setReloadable(false);
	setExtents();
	g_Timers.stopPerfTimer(CPU_PERF_TIMER);

	return true;
}

/* =============================================================================
 =============================================================================== */
bool CData::maskVar(int iVar, int iMaskVar)
{
	if ( !getVarValid(iVar)
		||	!getVarValid(iMaskVar)
		||	getVarSize(iMaskVar) == 0
		||	(getVarSize(iMaskVar) > getVarSize(iVar)))
		return setLastError("Invalid variable passed to function");

	int iVarDim = (int) getVarDims(iVar).size() - 1;
	for (int i = (int) getVarDims(iMaskVar).size() - 1; i >= 0; i--, iVarDim--)
	{
		if (getVarDims(iVar)[iVarDim] != getVarDims(iMaskVar)[i])
			return setLastError("Dimensions do not align for masking");
	}

	// for each plane, set to noval where maskvar == 0
	g_Timers.stopPerfTimer(CPU_PERF_TIMER);
	g_Timers.startPerfTimer(IO_PERF_TIMER);
	if (!loadVar(iVar) || !loadVar(iMaskVar))
		return setLastError("Unable to load variable " + getVarName(iVar) + " or " + getVarName(iMaskVar));
	g_Timers.stopPerfTimer(IO_PERF_TIMER);
	g_Timers.startPerfTimer(CPU_PERF_TIMER);

	int				iMaskSize = getVarSize(iMaskVar);
	int				iPlanes = getVarSize(iVar) / iMaskSize;

	vector<float>	&pMask = getVarFData(iMaskVar);
	float			fMaskNoVal = getVar(iMaskVar).getHasNoValue() ? getVarNoValue(iMaskVar) : 0.0f;
	vector<float>	&pData = getVarFData(iVar);
	float			fNoVal = getVarNoValue(iVar);
	int				iCnt = 0;
	for (int i = 0; i < iPlanes; i++)
		for (int j = 0; j < iMaskSize; j++, iCnt++)
			if (pMask[j] == fMaskNoVal) 
				pData[iCnt] = fNoVal;

	g_Timers.stopPerfTimer(CPU_PERF_TIMER);

	editVar(iVar).setReloadable(false);
	return true;
}

/* =============================================================================
 =============================================================================== */
int CData::copyVar(CData &Data, int iVar, string strNewName)
{
	g_Timers.startPerfTimer(CPU_PERF_TIMER);

	if (!Data.getVarValid(iVar))
		return -1;
	if (!Data.loadVar(iVar)) 
		return setLastError("Unable to load variable " + getVarName(iVar));

	m_Var.push_back(Data.m_Var[iVar]);

	int iNdx = getVarCnt() - 1;
	editVar(iNdx).setName(strNewName);
	editVar(iNdx).setID(-1);
	editVar(iNdx).setReloadable(false);

	g_Timers.stopPerfTimer(CPU_PERF_TIMER);

	return iNdx;
}

/* =============================================================================
    SLICING/DICING/FILTERING
 =============================================================================== */
bool CData::getVarSlice_R
(
	const float		*pSrc,
	int			idxSrc,
	float		* &pDest,
	const vector<int> &vDims,
	const vector<int> &szDims,
	int			idxDim,
	bool		bDataIndex
)
{
	int idx = vDims[idxDim];
	int size = szDims[idxDim];
	int strideSrc = 1;
	for (int i = idxDim + 1; i < vDims.size(); i++) 
		strideSrc *= szDims[i];
	idxSrc += (idx == -1 ? 0 : strideSrc * idx);

	int end = idx == -1 ? size : 1;
	for (int i = 0; i < end; i++, idxSrc += strideSrc)
	{
		if (strideSrc == 1)
			*pDest++ = (bDataIndex) ? idxSrc : pSrc[idxSrc];
		else
			getVarSlice_R(pSrc, idxSrc, pDest, vDims, szDims, idxDim + 1, bDataIndex);
	}

	return true;
}

/* =============================================================================
    pass vDims with -1 set for dimensions to get slice of and a value for
    others
 =============================================================================== */
bool CData::getVarSlice(int iVar, vector<float> &vDest, const vector<int> vDims, bool bDataIndex)
{
	if (!getVarValid(iVar) || getVarIsDData(iVar))
		return setLastError("Invalid variable passed to function");
	if (getVarDims(iVar).size() != vDims.size()) 
		return setLastError("Dimension list size doesn't match variable");

	g_Timers.startPerfTimer(IO_PERF_TIMER);
	if (!bDataIndex && !loadVar(iVar)) 
		return setLastError("Unable to load variable " + getVarName(iVar));
	g_Timers.stopPerfTimer(IO_PERF_TIMER);
	g_Timers.startPerfTimer(CPU_PERF_TIMER);

	vector<int> szDims;
	int			size = 1;
	for (int i = 0; i < getVarDims(iVar).size(); i++)
	{
		szDims.push_back(getDimSize(getVarDims(iVar)[i]));
		if (vDims[i] == -1) 
			size *= getDimSize(getVarDims(iVar)[i]);
	}

	if (size > vDest.size())
	{
		try
		{
			vDest.resize(size);
		}
		catch(std::bad_alloc const &)
		{
			return setLastError("Memory allocation failure");
		}
	}

	float	*pSrc = &(getVarFData(iVar)[0]);
	float	*pDest = &(vDest[0]);
	getVarSlice_R(pSrc, 0, pDest, vDims, szDims, 0, bDataIndex);
	return true;
}

/* =============================================================================
 =============================================================================== */
bool CData::putVarSlice_R(float * &pSrc, float *pDest, const vector<int> &vDims, const vector<int> &szDims, int idxDim)
{
	int idx = vDims[idxDim];
	int size = szDims[idxDim];
	int strideDest = 1;
	for (int i = idxDim + 1; i < vDims.size(); i++)
		strideDest *= szDims[i];
	pDest += (idx == -1 ? 0 : strideDest * idx);

	int end = idx == -1 ? size : 1;
	for (int i = 0; i < end; i++, pDest += strideDest)
	{
		if (strideDest == 1)
			*pDest = *pSrc++;
		else
			putVarSlice_R(pSrc, pDest, vDims, szDims, idxDim + 1);
	}

	return true;
}

/* =============================================================================
 =============================================================================== */
bool CData::putVarSlice(int iVar, vector<float> &vSrc, vector<int> vDims)
{
	if (!getVarValid(iVar) || getVarIsDData(iVar)) 
		return setLastError("Invalid variable passed to function");
	if (getVarDims(iVar).size() != vDims.size()) 
		return setLastError("Dimension list size doesn't match variable");

	g_Timers.startPerfTimer(IO_PERF_TIMER);
	if (!loadVar(iVar)) 
		return setLastError("Unable to load variable " + getVarName(iVar));
	g_Timers.stopPerfTimer(IO_PERF_TIMER);
	g_Timers.startPerfTimer(CPU_PERF_TIMER);

	vector<int> szDims;
	int			size = 1;
	for (int i = 0; i < getVarDims(iVar).size(); i++)
	{
		szDims.push_back(getDimSize(getVarDims(iVar)[i]));
		if (vDims[i] == -1)
			size *= getDimSize(getVarDims(iVar)[i]);
	}

	if (size != vSrc.size())
		return setLastError("Data source is wrong size");

	float	*pSrc = &(vSrc[0]);
	float	*pDest = &(getVarFData(iVar)[0]);
	putVarSlice_R(pSrc, pDest, vDims, szDims, 0);
	editVar(iVar).setReloadable(false);
	return true;
}

/* =============================================================================
    pass vDims with -1 set for dimensions to get slice of and a value for
    others
 =============================================================================== */
bool CData::getVarPosSlice(int iVar, vector<Vec3d> &pDest, const vector<int> &vDims, int iStart, int iSize)
{
	if (!getVarValid(iVar) || getVarIsDData(iVar)) 
		return setLastError("Invalid variable passed to function");
	if (getVarDims(iVar).size() != vDims.size()) 
		return setLastError("Dimension list size doesn't match variable");

	setCoordVars(iVar);

	vector<int> szDims;
	int			size = 1;
	for (int i = 0; i < getVarDims(iVar).size(); i++)
	{
		szDims.push_back(getDimSize(getVarDims(iVar)[i]));
		if (vDims[i] == -1) 
			size *= getDimSize(getVarDims(iVar)[i]);
	}

	if (iSize == -1) iSize = size;
	iSize = min(iSize, size - iStart);
	if (iSize > pDest.size())
	{
		try
		{
			pDest.resize(iSize);
		}
		catch(std::bad_alloc const &)
		{
			return setLastError("Memory allocation failure");
		}
	}

	vector<float>	idxPos;
	try
	{
		idxPos.resize(size);
	}
	catch(std::bad_alloc const &)
	{
		return setLastError("Memory allocation failure");
	}

	//  get indices
	float	*pSrc = NULL;
	float	*p_idxPos = &(idxPos[0]);
	getVarSlice_R(pSrc, 0, p_idxPos, vDims, szDims, 0, true);

	double	*pLat = NULL, *pLon = NULL;
	float	*pDepth = NULL;
	int		szTime = 1, szDepth = 1, szLat = 1, szLon = 1;
	if (getTimeVar() != -1)
	{
		szTime = getVarSize(getTimeVar());
	}

	if (getLatVar() != -1)
	{
		pLat = &(getVarDData(getLatVar())[0]);
		szLat = getVarSize(getLatVar());
	}

	if (getLonVar() != -1)
	{
		pLon = &(getVarDData(getLonVar())[0]);
		szLon = getVarSize(getLonVar());
	}

	float	fNoValue = NODATA;
	if (getDepthVar() != -1)
	{
		pDepth = &(getVarFData(getDepthVar())[0]);
		szDepth = getVarSize(getDepthVar());
		fNoValue = m_Var[getDepthVar()].getNoValue();
	}

	int iLatStride = 1, iLonStride = 1, iDepthStride = 1;
	if (szLon * szLat * szDepth == iSize)
	{
		iLatStride = szLon;
		iDepthStride = szLon * szLat;
	}

	int ndx = iStart;
	for (int i = 0; i < iSize; i++, ndx++)
	{
		double	lat = pLat ? iLatStride > 1 ? pLat[ndx / iLatStride] : pLat[ndx % szLat] : 0;
		double	lon = pLon ? iLonStride > 1 ? pLon[ndx / iLonStride] : pLon[ndx % szLon] : 0;
		float	depth = pDepth ? iDepthStride > 1 ? pDepth[ndx / iDepthStride] : pDepth[ndx % szDepth] : 0;
		if (depth != fNoValue)
			depth = -depth;
		pDest[i] = Vec3d(lat, lon, depth);
	}

	return true;
}

/* =============================================================================
 =============================================================================== */
float CData::interpVar_R(const float *pSrc, const vector<float> &vDims, const vector<int> &szDims, int idxDim, int iType)
{
	float	fpos = vDims[idxDim];
	int		idx = floor(fpos);
	int		size = szDims[idxDim];

	int		stride = 1;
	for (int i = idxDim + 1; i < vDims.size(); i++)
		stride *= szDims[i];
	pSrc += stride * idx;

	float	val = 0;
	float	fval[4];
	if (stride == 1)
	{
		fval[1] = pSrc[idx];
		fval[2] = pSrc[idx + 1];
		if (iType == 2)
		{
			fval[0] = pSrc[max(0, idx - 1)];
			fval[3] = pSrc[min(size - 1, idx + 2)];
		}
	}
	else
	{
		fval[1] = interpVar_R(pSrc, vDims, szDims, idxDim + 1, iType);
		fval[2] = interpVar_R(pSrc + stride, vDims, szDims, idxDim + 1, iType);
		if (iType == 2)
		{
			fval[0] = (idx == 0) ? fval[1] : interpVar_R(pSrc - stride, vDims, szDims, idxDim + 1, iType);
			fval[3] = (idx == size - 1) ? fval[2] : interpVar_R(pSrc + 2 * stride, vDims, szDims, idxDim + 1, iType);
		}
	}

	switch(iType)
	{
	case 0: //  nearest neighborval = float(fpos - 0.5f) != floor(fpos) ? fval[1] : fval[2]; break;
	case 1: //  linearval = (fval[1] + fval[2]) / 2.0f; break;
	case 2: CatmullRomValue(val, fpos, fval); break;
	}

	return val;
}

/* =============================================================================
    Pass vDims with location to get interpolated value for the variable iVar as
    return value ;
    iType sets method to use - NN, Linear, Spline
 =============================================================================== */
float CData::interpVarPoint(int iVar, vector<float> vDims, int iType)
{
	if (!getVarValid(iVar) || getVarIsDData(iVar) || vDims.size() != getVarDims(iVar).size())
		return setLastError("Invalid variable passed to function");

	vector<int> szDims;
	for (int i = 0; i < getVarDims(iVar).size(); i++)
	{
		szDims.push_back(getDimSize(getVarDims(iVar)[i]));
		if (vDims[i] < 0 || ceil(vDims[i]) >= (float) getDimSize(getVarDims(iVar)[i]))
			return setLastError("Invalid dimension position passed to function");
	}

	//  load the data
	g_Timers.stopPerfTimer(CPU_PERF_TIMER);
	g_Timers.startPerfTimer(IO_PERF_TIMER);
	if (!loadVar(iVar)) 
		return setLastError("Unable to load variable " + getVarName(iVar));
	g_Timers.stopPerfTimer(IO_PERF_TIMER);
	g_Timers.startPerfTimer(CPU_PERF_TIMER);

	float	*pSrc = &(getVarFData(iVar)[0]);
	bool	bRet = interpVar_R(pSrc, vDims, szDims, 0, iType);

	g_Timers.stopPerfTimer(CPU_PERF_TIMER);

	return bRet;
}

/* =============================================================================
 =============================================================================== */
bool FilterData(vector<float> &vData, const vector<float> &vFilter, double fNoVal)
{
	int size = vData.size();
	int fsize = vFilter.size();

	if (size == 0 || fsize == 0 || fsize % 2 != 1)
		return false;
	if (size == 1)
		return true;

	vector<float>	vOrig;
	try
	{
		vOrig = vData;
	}
	catch(std::bad_alloc const &)
	{
		cout << "Memory allocation failure!" << endl;
		return false;
	}

	//  this allows filter weights other than 1.0
	float	fTotWt = 0;
	for (int i = 0; i < fsize; i++) 
		fTotWt += vFilter[i];

	int fOffset = fsize / 2;

	for (int i = 0; i < size; i++)
	{
		if (vOrig[i] == fNoVal)
			continue;

		float	totval = 0.0;
		float	totwt = 0.0;
		for (int j = 0, k = i - fOffset; j < fsize; j++, k++)
		{
			float	val = k < 0 || k >= size ? fNoVal : vOrig[k];
			if (val == fNoVal)
				continue;
			totval += val * vFilter[j];
			totwt += vFilter[j];
		}

		vData[i] = totval * (fTotWt / totwt);
	}

	return true;
}

/* =============================================================================
 =============================================================================== */
bool CData::filterVarDim_R(int iVar, const vector<float> &vFilter, vector<int> &vDims, const vector<int> &szDims, int idxDim)
{
	int idx = vDims[idxDim];
	int size = szDims[idxDim];
	int end = idx == -1 ? 1 : size;
	for (int i = 0; i < end; i++)
	{
		if (idxDim == vDims.size() - 1)
		{
			int cnt = -1;
			for (int j = 0; j < vDims.size() && cnt == -1; j++)
			{
				if (vDims[j] != -1)
					continue;
				cnt = szDims[j];
			}

			if (cnt == -1)
				return false;
			vector<float> vData(cnt);

			float	*pTmp = &(vData[0]);
			float	*pSrc = &(getVarFData(iVar)[0]);
			float	fNoVal = getVar(iVar).getNoValue();
			getVarSlice_R(pSrc, 0, pTmp, vDims, szDims, 0, false);

			bool	bRet = false;
			bRet = FilterData(vData, vFilter, fNoVal);
			if (!bRet)
				return setLastError("Filter failed.");
			pTmp = &(vData[0]);
			putVarSlice_R(pTmp, pSrc, vDims, szDims, 0);
		}
		else
		{
			if (idx != -1)
				vDims[idxDim] = i;
			if (!filterVarDim_R(iVar, vFilter, vDims, szDims, idxDim + 1))
				return false;
		}
	}

	return true;
}

/* =============================================================================
    pass vDims with 1 set for dimensions to filter and 0 for others
 =============================================================================== */
bool CData::filterVarDim(int iVar, vector<int> vDims, vector<float> vFilter)
{
	if (!getVarValid(iVar) || getVarIsDData(iVar) || vDims.size() != getVarDims(iVar).size())
		return setLastError("Invalid variable passed to function");

	//  load the data
	g_Timers.stopPerfTimer(CPU_PERF_TIMER);
	g_Timers.startPerfTimer(CPU_PERF_TIMER);
	if (!loadVar(iVar)) 
		return setLastError("Unable to load variable " + getVarName(iVar));
	g_Timers.stopPerfTimer(IO_PERF_TIMER);
	g_Timers.startPerfTimer(CPU_PERF_TIMER);

	//  filter back through dimensions one by one
	bool	bRet = true;
	for (int d = vDims.size() - 1; d >= 0 && bRet; d--)
	{
		if (vDims[d] == 0)
			continue; //  not filtering this dimension

		//  fill in vectors to recursively go through dimension slices
		vector<int> szDims;
		vector<int> vVarDims = getVarDims(iVar);
		for (int i = 0; i < vVarDims.size(); i++)
		{
			szDims.push_back(getDimSize(vVarDims[i]));
			vVarDims[i] = (i == d) ? -1 : 0;
		}

		bRet = filterVarDim_R(iVar, vFilter, vVarDims, szDims, 0);
	}

	g_Timers.stopPerfTimer(CPU_PERF_TIMER);

	editVar(iVar).setReloadable(false);
	setExtents();
	setDirty(true);

	return bRet;
}

/* =============================================================================
    PCA ROUTINES
 =============================================================================== */
bool CData::PCA(int iVar)
{
	g_Timers.startPerfTimer(CPU_PERF_TIMER);

	if (!getVarValid(iVar) || getVarIsDData(iVar)) 
		return setLastError("Invalid variable passed to function");

	if (getVarDims(iVar).size() < 2 || getDimSize(getVarDims(iVar)[0]) < 2)
		return setLastError("Dimensions are problematic for PCA");

	if (!loadVar(iVar)) 
		return setLastError("Unable to load variable " + getVarName(iVar));

	int				iSize = getVarSize(iVar);
	vector<float>	&pData = getVarFData(iVar);
	float			fNoVal = getVarNoValue(iVar);
	int				iSteps = getDimSize(getVarDims(iVar)[0]);
	int				iRowSize = iSize / iSteps;

	//  first find the mean
	vector<float> pca_mean(iRowSize, 0);
	vector<double> vTot(iRowSize, 0.0);
	vector<int> vCnt(iRowSize, 0);
	for (int i = 0; i < iSteps; i++)
		for (int j = 0; j < iRowSize; j++)
			if (pData[i * iRowSize + j] != fNoVal)
			{
				vTot[j] += pData[i * iRowSize + j];
				vCnt[j]++;
			}

	for (int j = 0; j < iRowSize; j++) 
		pca_mean[j] = vTot[j] / (double) vCnt[j];

	// set up matrix ;
	// due to limitation in SVD solver, A must have at least as many rows as columns
	int		iRows = max(iRowSize, iSteps);
	Matd	A(iRows, iRowSize, vl_0);
	for (int i = 0; i < iSteps; i++)
		for (int j = 0; j < iRowSize; j++)
			if (pData[i * iRowSize + j] != fNoVal) 
				A[i][j] = pData[i * iRowSize + j] - pca_mean[j];

	//  solve SVD
	Vecd	diagonal;
	Matd	U, V;
	SVDFactorization(A, U, V, diagonal);

	//  store principal components in dataset
	int iEigenSize = diagonal.Elts();
	vector<float> eig_vals(iEigenSize, 0);
	vector<float> eig_vecs(iEigenSize * iEigenSize, 0);
	for (int i = 0; i < iEigenSize; i++)
	{
		eig_vals[i] = (diagonal[i]);
		for (int j = 0; j < iEigenSize; j++) 
			eig_vecs[i * iEigenSize + j] = V[j][i];
	}

	string	strDim = "PCA_DIM";
	int		iDim = findDim(strDim);
	if (iDim != -1)
		for (int i = 1; findDim(strDim = getNewName(strDim, i)) != -1; i++);
	iDim = newDim(strDim, iEigenSize);

	string	strDim2 = "PCA_DIM_H";
	int		iDim2 = findDim(strDim);
	if (iDim2 != -1)
		for (int i = 1; findDim(strDim2 = getNewName(strDim2, i)) != -1; i++);
	iDim2 = newDim(strDim2, iEigenSize);

	//  new vars to hold vectors, values, and mean
	string	strEVecs = getVarName(iVar) + "_eigenvectors";
	int		iEVecs = findVar(strEVecs);
	if (iEVecs != -1) delVar(iEVecs);
	vector<int> vDims(2, iDim);
	vDims[1] = iDim2;
	iEVecs = newVar(strEVecs, DATA_SCALAR, vDims);
	setVarFData(iEVecs, eig_vecs);

	string	strEVals = getVarName(iVar) + "_eigenvalues";
	int		iEVals = findVar(strEVals);
	if (iEVals != -1) delVar(iEVals);
	vDims.pop_back();
	iEVals = newVar(strEVals, DATA_SCALAR, vDims);
	setVarFData(iEVals, eig_vals);

	string	strEMean = getVarName(iVar) + "_pcamean";
	int		iEMean = findVar(strEMean);
	if (iEMean != -1) delVar(iEMean);
	iEMean = newVar(strEMean, DATA_SCALAR, vDims);
	setVarFData(iEMean, pca_mean);

	g_Timers.stopPerfTimer(CPU_PERF_TIMER);

	return true;
}

vector<int> CData::PCAGetVars (int iVar)
{
	vector<int> vVars(3, -1);

	if (!getVarValid(iVar) || getVarIsDData(iVar))
	{
		setLastError("Invalid variable passed to function");
		return vVars;
	}
	string	strEVecs =
	getVarName(iVar)
	+ "_eigenvectors";
	vVars[0] = findVar(strEVecs);
	string	strEVals =
	getVarName(iVar)
	+ "_eigenvalues";
	vVars[1] = findVar(strEVals);
	string	strEMean =
	getVarName(iVar)
	+ "_pcamean";
	vVars[2] = findVar(strEMean);
	return vVars;
}

/* =============================================================================
 =============================================================================== */
bool CData::PCAProject(int iVar, vector<int> iEVars)
{
	g_Timers.startPerfTimer(CPU_PERF_TIMER);

	if (iEVars.size() != 3)
		return setLastError("Invalid eigen vector variable set");

	int iEVecs = iEVars[0];
	int iEMean = iEVars[2];
	if (!getVarValid(iVar) || !getVarValid(iEVecs) || !getVarValid(iEMean))
		return setLastError("Invalid variable passed to function");

	if (!loadVar(iVar)) 
		return setLastError("Unable to load variable " + getVarName(iVar));

	int				iSize = getVarSize(iVar);
	vector<float>	&pData = getVarFData(iVar);
	float			fNoVal = getVarNoValue(iVar);
	int				iSteps = getDimSize(getVarDims(iVar)[0]);
	int				iRowSize = iSize / iSteps;

	if (iEVecs == -1 || iEMean == -1 || getVarSize(iEVecs) != iRowSize * iRowSize || getVarSize(iEMean) != iRowSize)
		return setLastError("PCA not available or incorrect dimensions for this dataset");

	vector<float>	&eig_vecs = getVarFData(iEVecs);
	vector<float>	&pca_mean = getVarFData(iEMean);

	//  project into pca space
	Matd			VT(iRowSize, iRowSize);
	for (int i = 0; i < iRowSize; i++)
		for (int j = 0; j < iRowSize; j++) 
			VT[i][j] = eig_vecs[i * iRowSize + j];

	for (int i = 0; i < iSteps; i++)
	{
		Matd	x(iRowSize, 1, vl_0);
		for (int j = 0; j < iRowSize; j++)
			if (pData[i * iRowSize + j] != fNoVal) 
				x[j][0] = pData[i * iRowSize + j] - pca_mean[j];

		Matd	pca_vec = VT * x;
		for (int j = 0; j < iRowSize; j++)
			if (pData[i * iRowSize + j] != fNoVal) 
				pData[i * iRowSize + j] = pca_vec[j][0];
	}

	g_Timers.stopPerfTimer(CPU_PERF_TIMER);

	return true;
}

/* =============================================================================
 =============================================================================== */
bool CData::PCAUnProject(int iVar, vector<int> iEVars)
{
	g_Timers.startPerfTimer(CPU_PERF_TIMER);

	if (iEVars.size() != 3) 
		return setLastError("Invalid eigen vector variable set");

	int iEVecs = iEVars[0];
	int iEMean = iEVars[2];
	if (!getVarValid(iVar) || !getVarValid(iEVecs) || !getVarValid(iEMean))
		return setLastError("Invalid variable passed to function");

	if (!loadVar(iVar)) 
		return setLastError("Unable to load variable " + getVarName(iVar));

	int				iSize = getVarSize(iVar);
	vector<float>	&pData = getVarFData(iVar);
	float			fNoVal = getVarNoValue(iVar);
	int				iSteps = getDimSize(getVarDims(iVar)[0]);
	int				iRowSize = iSize / iSteps;

	if (iEVecs == -1 || iEMean == -1 || getVarSize(iEVecs) != iRowSize * iRowSize || getVarSize(iEMean) != iRowSize)
		return setLastError("PCA not available or incorrect dimensions for this dataset");

	vector<float>	&eig_vecs = getVarFData(iEVecs);
	vector<float>	&pca_mean = getVarFData(iEMean);

	//  project into pca space
	Matd			V(iRowSize, iRowSize);
	for (int i = 0; i < iRowSize; i++)
		for (int j = 0; j < iRowSize; j++) 
			V[j][i] = eig_vecs[i * iRowSize + j];

	for (int i = 0; i < iSteps; i++)
	{
		Matd	pca_vec(iRowSize, 1, vl_0);
		for (int j = 0; j < iRowSize; j++)
			if (pData[i * iRowSize + j] != fNoVal) 
				pca_vec[j][0] = pData[i * iRowSize + j];

		Matd	x = V * pca_vec;
		for (int j = 0; j < iRowSize; j++)
			if (pData[i * iRowSize + j] != fNoVal) 
				pData[i * iRowSize + j] = x[j][0] + pca_mean[j];
	}

	g_Timers.stopPerfTimer(CPU_PERF_TIMER);

	return true;
}

/* =============================================================================
 =============================================================================== */
void pca_test()
{
	CData			data;
	vector<double>	time;
	time.resize(32);

	double	tm = getCurrentTime();
	for (int i = 0; i < 32; i++, tm += DUR_HOUR) 
		time[i] = tm;
	vector<double> lat(4, 0.0f);
	vector<double> lon(4, 0.0f);
	vector<float> dep(1, 0.0f);

	data.createRegularGrid(time, lat, lon, dep);

	vector<int> vDims;
	for (int i = 0; i < 4; i++) 
		vDims.push_back(i);

	int iVar = data.newVar("test", DATA_SCALAR, vDims);
	data.setVarFData(iVar, 15.0f);

	vector<float>	&fdata = data.getVarFData(iVar);
	for (int i = 0; i < 80; i++) 
		fdata[i] += (float) rand() / (float) RAND_MAX;

	data.PCA(iVar);

	vector<int> iEVars = data.PCAGetVars(iVar);
	int			iPCA = data.copyVar(data, iVar, "test_pca");
	data.PCAProject(iPCA, iEVars);

	int				iUNPCA = data.copyVar(data, iPCA, "test_unpca");

	//  clear out all but first component
	vector<float>	&edata = data.getVarFData(iUNPCA);
	int				row_size = data.getVarSize(iEVars[1]);
	int				row_cnt = data.getDimSize(0);
	for (int i = 0; i < row_cnt; i++)
		for (int j = 3; j < row_size; j++) 
			edata[i * row_size + j] = 0;

	data.PCAUnProject(iUNPCA, iEVars);

	data.writeNetCDF("c:/dev/cove/data/output/test.nc");
}
