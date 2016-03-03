/* =============================================================================
	File: data_mgr.cpp

 =============================================================================== */

#include "data_mgr.h"
#include "utility.h"
//#include "netcdf.h"

/* =============================================================================
    Data Handling Class
 =============================================================================== */
CData::CData()
{
	m_NCFileID = -1;
	m_UseCount = 0;
	m_CoordVars = vector<int> (4, -1);
	m_CoordNoValue = Vec3d(NODATA, NODATA, NODATA);
	m_LoadAsTerrain = false;
	m_TimeFormat = TIME_FMT_MDY;
	m_RegularGrid = false;
	m_ScaleAtt = "scale_factor";
	m_OffsetAtt = "add_offset";
	m_Dirty = false;
	m_LastError = new char[MAX_PATH];
}

/* =============================================================================
 =============================================================================== */
CData::~CData()
{
	clean();
	delete [] m_LastError;
	assert(m_UseCount == 0);
}

/* =============================================================================
 =============================================================================== */
void CData::clean()
{
	m_Dim.clear();
	std::vector<CDimension> ().swap(m_Dim);
	m_Att.clear();
	std::vector<CAttribute> ().swap(m_Att);
	m_Var.clear();
	std::vector<CVariable> ().swap(m_Var);

	m_BSPTree.deleteNodes();
	m_BSPCellNdx.clear();
	std::vector<int> ().swap(m_BSPCellNdx);
	m_BSPGrid.clear();
	std::vector<Vec3d> ().swap(m_BSPGrid);
	m_BSPDims.clear();
	std::vector<int> ().swap(m_BSPDims);

	//if (m_NCFileID != -1)
	//	ncclose(m_NCFileID);
	m_NCFileID = -1;
}

/* =============================================================================
 =============================================================================== */
void CVariable::updateMinMax()
{
	if (getTrnDataLoaded())
		return;
	m_Max = -1e30;
	m_Min = 1e30;

	bool bDData = getDDataSize() > 0;
	int iSize = getSize();
	float fNoVal = (float) m_NoValue;
	for (int i = 0; i < iSize; i++)
	{
		float  *fp = bDData ? NULL : &m_FData[i];
		double *dp = bDData ? &m_DData[i] : NULL;
		double val = fp ? *fp : *dp;
		if (!finite(val) || isnan(val))
		{
			if (fp)
				*fp = fNoVal;
			else
				*dp = m_NoValue;
			val = m_NoValue;
		}
		else if (val != m_NoValue && (float) val != fNoVal)
		{
			if (val < m_Min) m_Min = val;
			if (val > m_Max) m_Max = val;
		}
	}

	//  if no values then set max and min to 0
	if (m_Min == 1e30) m_Max = m_Min = 0;
}

/* =============================================================================
 =============================================================================== */
void CVariable::updateTrnMinMax(int iRow)
{
	int iRowSize = getTrnDataRowLen();
	float fNoVal = (float) m_NoValue;
	for (int i = 0; i < iRowSize; i++)
	{
		float	*fp = &m_TrnData[iRow][i];
		if (!finite(*fp) || isnan(*fp))
			*fp = fNoVal;
		else if (*fp != fNoVal)
		{
			if (*fp < m_Min) m_Min = *fp;
			if (*fp > m_Max) m_Max = *fp;
		}
	}
}

/* =============================================================================
 =============================================================================== */
void CData::getTimeSlice(int iVar, int iSlice, vector<float> &pVal)
{
	if (getVarIsDData(iVar))
		return;

	int iStart = 0, iSize = 0;
	if (iSlice < 0)
	{
		iSize = getVarSize(iVar);
		iStart = 0;
	}
	else
	{
		iSize = getTimeSliceSize(iVar);
		iStart = iSlice * iSize;
	}

	float	*pVar = &(getVarFData(iVar)[0]);
	assert(pVar != NULL);
	if (pVar == NULL)
		return;
	for (int i = 0; i < iSize; i++) 
		pVal[i] = pVar[i + iStart];
}

/* =============================================================================
 =============================================================================== */
int CData::getDimCnt() const
{
	return (int) m_Dim.size();
}

/* =============================================================================
 =============================================================================== */
bool CData::getDimValid(int iDim) const
{
	return iDim >= 0 && iDim < getDimCnt();
}

/* =============================================================================
 =============================================================================== */
int CData::findDim(string strName) const
{
	strName = lwrstr(strName);
	for (int i = 0; i < m_Dim.size(); i++)
		if (strName == lwrstr(m_Dim[i].getName()))
			return i;
	return -1;
}

/* =============================================================================
 =============================================================================== */
int CData::newDim(string strName, int iSize)
{
	if (findDim(strName) != -1)
		return -1;

	CDimension dim;
	dim.setName(strName);
	dim.setSize(iSize);
	m_Dim.push_back(dim);
	return m_Dim.size() - 1;
}

/* =============================================================================
 =============================================================================== */
bool CData::delDim(int iDim)
{
	if (!getDimValid(iDim))
		return setLastError("Invalid dimension index");
	m_Dim.erase(m_Dim.begin() + iDim);
	return true;
}

/* =============================================================================
 =============================================================================== */
string CData::getDimName(int iDim) const
{
	if (!getDimValid(iDim))
	{
		setLastError("Invalid dimension index");
		return "";
	}

	return m_Dim[iDim].getName();
}

/* =============================================================================
 =============================================================================== */
bool CData::setDimName(int iDim, string strName)
{
	if (!getDimValid(iDim))
		return setLastError("Invalid dimension index");
	m_Dim[iDim].setName(strName);
	return true;
}

/* =============================================================================
 =============================================================================== */
int CData::getDimSize(int iDim) const
{
	if (!getDimValid(iDim))
	{
		setLastError("Invalid dimension index");
		return -1;
	}

	return m_Dim[iDim].getSize();
}

/* =============================================================================
 =============================================================================== */
bool CData::setDimSize(int iDim, int iSize)
{
	if (!getDimValid(iDim))
		return setLastError("Invalid dimension index");
	m_Dim[iDim].setSize(iSize);
	for (int i = 0; i < getVarCnt(); i++) 
		resetVarSize(i);
	return true;
}

/* =============================================================================
 =============================================================================== */
bool CData::getDimUnlimited(int iDim) const
{
	if (!getDimValid(iDim))
		return setLastError("Invalid dimension index");
	return m_Dim[iDim].getUnlimited();
}

/* =============================================================================
 =============================================================================== */
bool CData::setDimUnlimited(int iDim, bool b)
{
	if (!getDimValid(iDim))
		return setLastError("Invalid dimension index");
	m_Dim[iDim].setUnlimited(b);
	return true;
}

/* =============================================================================
 =============================================================================== */
bool CData::getAttVarValid(int iVar) const
{
	return iVar >= -1 && iVar < getVarCnt();
}

/* =============================================================================
 =============================================================================== */
bool CData::getAttValid(int iVar, int iAtt) const
{
	return iAtt >= 0 && iAtt < getAttCnt(iVar);
}

/* =============================================================================
 =============================================================================== */
int CData::getAttCnt(int iVar) const
{
	if (!getAttVarValid(iVar))
	{
		setLastError("Invalid variable index");
		return -1;
	}

	if (iVar > -1)
		return (int) m_Var[iVar].getAttCnt();
	return m_Att.size();
}

/* =============================================================================
 =============================================================================== */
int CData::findAtt(int iVar, string strName) const
{
	if (!getAttVarValid(iVar))
	{
		setLastError("Invalid variable index");
		return -1;
	}

	if (iVar > -1)
		return m_Var[iVar].findAtt(strName);
	strName = lwrstr(strName);
	for (int i = 0; i < m_Att.size(); i++)
		if (strName == lwrstr(m_Att[i].getName()))
			return i;
	return -1;
}

/* =============================================================================
 =============================================================================== */
int CData::newAtt(int iVar, string strName)
{
	if (!getAttVarValid(iVar))
	{
		setLastError("Invalid variable index");
		return -1;
	}

	if (findAtt(iVar, strName) != -1)
		return -1;

	CAttribute att;
	att.setName(strName);
	if (iVar < 0)
	{
		m_Att.push_back(att);
		return m_Att.size() - 1;
	}

	m_Var[iVar].addAtt(att);
	return m_Var[iVar].getAttCnt() - 1;
}

/* =============================================================================
 =============================================================================== */
bool CData::delAtt(int iVar, int iAtt)
{
	if (!getAttVarValid(iVar) || !getAttValid(iVar, iAtt)) 
		return setLastError("Invalid variable or attribute index");
	if (iVar > -1)
		m_Var[iVar].delAtt(iAtt);
	else
		m_Att.erase(m_Att.begin() + iAtt);
	return true;
}

/* =============================================================================
 =============================================================================== */
string CData::getAttName(int iVar, int iAtt) const
{
	if (!getAttVarValid(iVar) || !getAttValid(iVar, iAtt))
	{
		setLastError("Invalid variable or attribute index");
		return "";
	}

	if (iVar > -1)
		return m_Var[iVar].getAttName(iAtt);
	return m_Att[iAtt].getName();
}

/* =============================================================================
 =============================================================================== */
bool CData::setAttName(int iVar, int iAtt, string strName)
{
	if (!getAttVarValid(iVar) || !getAttValid(iVar, iAtt))
		return setLastError("Invalid variable or attribute index");
	if (iVar > -1)
		m_Var[iVar].setAttName(iAtt, strName);
	else
		m_Att[iAtt].setName(strName);
	return true;
}

/* =============================================================================
 =============================================================================== */
string CData::getAttText(int iVar, int iAtt) const
{
	if (!getAttVarValid(iVar) || !getAttValid(iVar, iAtt))
	{
		setLastError("Invalid variable or attribute index");
		return "";
	}

	if (iVar > -1)
		return m_Var[iVar].getAttText(iAtt);
	return m_Att[iAtt].getText();
}

/* =============================================================================
 =============================================================================== */
bool CData::setAttText(int iVar, int iAtt, string strText)
{
	if (!getAttVarValid(iVar) || !getAttValid(iVar, iAtt)) 
		return setLastError("Invalid variable or attribute index");
	if (iVar > -1)
		m_Var[iVar].setAttText(iAtt, strText);
	else
		m_Att[iAtt].setText(strText);
	return true;
}

/* =============================================================================
 =============================================================================== */
vector<double> CData::getAttData(int iVar, int iAtt) const
{
	if (!getAttVarValid(iVar) || !getAttValid(iVar, iAtt))
	{
		setLastError("Invalid variable or attribute index");

		static vector<double> tmp;
		return tmp;
	}

	if (iVar > -1)
		return m_Var[iVar].getAttData(iAtt);
	return m_Att[iAtt].getData();
}

/* =============================================================================
 =============================================================================== */
bool CData::setAttData(int iVar, int iAtt, vector<double> vData)
{
	if (!getAttVarValid(iVar) || !getAttValid(iVar, iAtt)) 
		return setLastError("Invalid variable or attribute index");

	bool bRet = false;
	if (iVar > -1)
		bRet = m_Var[iVar].setAttData(iAtt, vData);
	else
		bRet = m_Att[iAtt].setData(vData);
	if (!bRet)
		return setLastError("Memory allocation failure");
	return true;
}

/* =============================================================================
 =============================================================================== */
int CData::getVarCnt() const
{
	return (int) m_Var.size();
}

/* =============================================================================
 =============================================================================== */
int CData::getVarTypeCnt(int iType) const
{
	int iCnt = 0;
	for (int i = 0; i < getVarCnt(); i++)
		if (getVarType(i) == iType)
			iCnt++;
	return iCnt;
}

/* =============================================================================
 =============================================================================== */
bool CData::getVarValid(int iVar) const
{
	return iVar >= 0 && iVar < getVarCnt();
}

/* =============================================================================
 =============================================================================== */
bool CData::getVarDimValid(int iVar, int iDim) const
{
	return getVarValid(iVar) && iDim >= 0 && iDim < getVarDims(iVar).size();
}

/* =============================================================================
 =============================================================================== */
bool CData::getVarIsDData(int i) const
{
	return getVarValid(i) ? m_Var[i].getIsDData() : false;
}

/* =============================================================================
 =============================================================================== */
int CData::findVar(string strName) const
{
	strName = lwrstr(strName);
	for (int i = 0; i < m_Var.size(); i++)
	{
		if (strName == lwrstr(m_Var[i].getName()))
			return i;
	}

	return -1;
}

/* =============================================================================
 =============================================================================== */
int CData::newVar(string strName, int iType, vector<int> vDims)
{
	CVariable var;
	if (findVar(strName) != -1)
		return -1;
	var.setName(strName);
	var.setType(iType);
	var.setID(-1);
	var.setDims(vDims);
	m_Var.push_back(var);

	int iVar = m_Var.size() - 1;
	resetVarSize(iVar);
	if (iType >= DATA_TIME && iType <= DATA_DEPTH)
		resetCoordVars();
	else
		resetCoordVars(iVar);
	return iVar;
}

/* =============================================================================
 =============================================================================== */
bool CData::delVar(int iVar)
{
	if (!getVarValid(iVar))
		return setLastError("Invalid variable index");
	m_Var.erase(m_Var.begin() + iVar);
	resetCoordVars();
	return true;
}

/* =============================================================================
 =============================================================================== */
string CData::getVarName(int iVar) const
{
	if (!getVarValid(iVar))
	{
		setLastError("Invalid variable index");
		return "";
	}

	return m_Var[iVar].getName();
}

/* =============================================================================
 =============================================================================== */
bool CData::setVarName(int iVar, string strName)
{
	if (!getVarValid(iVar))
		return setLastError("Invalid variable index");
	m_Var[iVar].setName(strName);
	return true;
}

/* =============================================================================
 =============================================================================== */
int CData::getVarType(int iVar) const
{
	if (!getVarValid(iVar))
	{
		setLastError("Invalid variable index");
		return -1;
	}

	return m_Var[iVar].getType();
}

/* =============================================================================
 =============================================================================== */
bool CData::setVarType(int iVar, int iType)
{
	if (!getVarValid(iVar))
		return setLastError("Invalid variable index");

	bool bPosType =
		(
			(getVarType(iVar) >= DATA_TIME && getVarType(iVar) <= DATA_DEPTH)
		||	(iType >= DATA_TIME && iType <= DATA_DEPTH)
		);
	m_Var[iVar].setType(iType);
	if (bPosType) resetCoordVars();
	return true;
}

/* =============================================================================
 =============================================================================== */
bool CData::resetVarSize(int iVar)
{
	if (!getVarValid(iVar))
	{
		setLastError("Invalid variable index");
		return false;
	}

	int iSize = 1;
	for (int i = 0; i < m_Var[iVar].getDimCnt(); i++) 
		iSize *= m_Dim[m_Var[iVar].getDim(i)].getSize();
	m_Var[iVar].setSize(iSize);
	return true;
}

/* =============================================================================
 =============================================================================== */
int CData::getVarSize(int iVar) const
{
	if (!getVarValid(iVar))
	{
		setLastError("Invalid variable index");
		return -1;
	}

	return m_Var[iVar].getSize();
}

/* =============================================================================
 =============================================================================== */
bool CData::updateVarMinMax(int iVar)
{
	if (!getVarValid(iVar))
		return setLastError("Invalid variable index");;
	m_Var[iVar].updateMinMax();
	return true;
}

/* =============================================================================
 =============================================================================== */
double CData::getVarMin(int iVar) const
{
	if (!getVarValid(iVar))
	{
		setLastError("Invalid variable index");
		return getdNAN();
	}

	return m_Var[iVar].getMin();
}

/* =============================================================================
 =============================================================================== */
double CData::getVarMax(int iVar) const
{
	if (!getVarValid(iVar))
	{
		setLastError("Invalid variable index");
		return getdNAN();
	}

	return m_Var[iVar].getMax();
}

/* =============================================================================
 =============================================================================== */
double CData::getVarNoValue(int iVar) const
{
	if (!getVarValid(iVar))
	{
		setLastError("Invalid variable index");
		return getfNAN();
	}

	return m_Var[iVar].getNoValue();
}

/* =============================================================================
 =============================================================================== */
bool CData::setVarNoValue(int iVar, double f)
{
	if (!getVarValid(iVar))
		return setLastError("Invalid variable index");
	m_Var[iVar].setNoValue(f);
	return true;
}

/* =============================================================================
 =============================================================================== */
const vector<int> CData::getVarDims(int iVar) const
{
	static vector<int> vDims;
	if (!getVarValid(iVar))
	{
		setLastError("Invalid variable index");
		return vDims;
	}

	return m_Var[iVar].getDims();
}

/* =============================================================================
 =============================================================================== */
bool CData::setVarDims(int iVar, vector<int> vDims)
{
	if (!getVarValid(iVar))
		return setLastError("Invalid variable index");

	int iOldSize = m_Var[iVar].getSize();
	m_Var[iVar].setDims(vDims);
	resetVarSize(iVar);
	if (iOldSize != m_Var[iVar].getSize())
	{
		clearVarFData(iVar);
		clearVarDData(iVar);
	}

	resetCoordVars(iVar);
	return true;
}

/* =============================================================================
 =============================================================================== */
float CData::getVarFDataValue(int iVar, int ival)
{
	if (!getVarValid(iVar))
	{
		setLastError("Invalid variable index");
		return getfNAN();
	}

	if (!loadVar(iVar))
	{
		setLastError("Unable to load variable " + getVarName(iVar));
		return getfNAN();
	}

	return m_Var[iVar].getFDataValue(ival);
}

/* =============================================================================
 =============================================================================== */
bool CData::setVarFDataValue(int iVar, int ival, float f)
{
	if (!getVarValid(iVar))
		return setLastError("Invalid variable index");
	if (!loadVar(iVar))
		return setLastError("Unable to load variable " + getVarName(iVar));
	return m_Var[iVar].setFDataValue(ival, f);
}

/* =============================================================================
 =============================================================================== */
vector<float> &CData::getVarFData(int iVar)
{
	static vector<float> tmp;
	if (!getVarValid(iVar))
	{
		setLastError("Invalid variable index");
		return tmp;
	}

	if (!loadVar(iVar))
	{
		cout << ("Unable to load variable " + getVarName(iVar));
		return tmp;
	}

	return m_Var[iVar].editFData();
}

/* =============================================================================
 =============================================================================== */
bool CData::setVarFData(int iVar, float f)
{
	if (!getVarValid(iVar))
		return setLastError("Invalid variable index");
	if (!m_Var[iVar].setFData(f))
		return setLastError("Memory allocation failure");
	return true;
}

/* =============================================================================
 =============================================================================== */
bool CData::setVarFData(int iVar, const vector<float> &vData)
{
	if (!getVarValid(iVar))
		return setLastError("Invalid variable index");
	if (!m_Var[iVar].setFData(vData))
		return setLastError("Memory allocation failure");
	return true;
}

/* =============================================================================
 =============================================================================== */
bool CData::clearVarFData(int iVar)
{
	if (!getVarValid(iVar))
		return setLastError("Invalid variable index");
	return m_Var[iVar].clearFData();
}

/* =============================================================================
 =============================================================================== */
bool CData::initVarFData(int iVar)
{
	if (!getVarValid(iVar))
		return setLastError("Invalid variable index");
	return m_Var[iVar].initFDataMem();
}

/* =============================================================================
 =============================================================================== */
double CData::getVarDDataValue(int iVar, int ival)
{
	if (!getVarValid(iVar))
	{
		setLastError("Invalid variable index");
		return getdNAN();
	}

	if (!loadVar(iVar))
	{
		setLastError("Unable to load variable " + getVarName(iVar));
		return getdNAN();
	}

	return m_Var[iVar].getDDataValue(ival);
}

/* =============================================================================
 =============================================================================== */
bool CData::setVarDDataValue(int iVar, int ival, double d)
{
	if (!getVarValid(iVar))
		return setLastError("Invalid variable index");
	if (!loadVar(iVar))
		return setLastError("Unable to load variable " + getVarName(iVar));
	return m_Var[iVar].setDDataValue(ival, d);
}

/* =============================================================================
 =============================================================================== */
vector<double> &CData::getVarDData(int iVar)
{
	static vector<double> tmp;
	if (!getVarValid(iVar))
	{
		setLastError("Invalid variable index");
		return tmp;
	}

	if (!loadVar(iVar))
	{
		cout << ("Unable to load variable " + getVarName(iVar));
		return tmp;
	}

	return m_Var[iVar].editDData();
}

/* =============================================================================
 =============================================================================== */
bool CData::setVarDData(int iVar, double d)
{
	if (!getVarValid(iVar))
		return setLastError("Invalid variable index");
	if (!m_Var[iVar].setDData(d))
		return setLastError("Memory allocation failure");
	return true;
}

/* =============================================================================
 =============================================================================== */
bool CData::setVarDData(int iVar, const vector<double> &vData)
{
	if (!getVarValid(iVar))
		return setLastError("Invalid variable index");
	if (!m_Var[iVar].setDData(vData))
		return setLastError("Memory allocation failure");
	return true;
}

/* =============================================================================
 =============================================================================== */
bool CData::clearVarDData(int iVar)
{
	if (!getVarValid(iVar))
		return setLastError("Invalid variable index");
	return m_Var[iVar].clearDData();
}

/* =============================================================================
 =============================================================================== */
bool CData::initVarDData(int iVar)
{
	if (!getVarValid(iVar))
		return setLastError("Invalid variable index");
	return m_Var[iVar].initDDataMem();
}

/* =============================================================================
 =============================================================================== */
int CData::isCOVEFormat() const
{
	int iAtt = findAtt(-1, "Conventions");
	if (iAtt == -1)
		return 0;
	if (getAttText(-1, iAtt) == "COVE" || getAttText(-1, iAtt) == "CF-1.4")
		return 1;
	return 0;
}

vector<int> CData::getCoordVars (int iVar)
{
	if (!setCoordVars(iVar))
	{
		setLastError("Cannot set position variables");
		return vector<int> (4, -1);
	}
	return m_CoordVars;
}

/* =============================================================================
 =============================================================================== */
vector<double> CData::getVarCoordNoValue(int iVar)
{
	vector<double> v;
	if (!getVarValid(iVar))
	{
		setLastError("Invalid variable index");
		return v;
	}

	return vec2vectord(getDataCoordNoValue(iVar));
}

/* =============================================================================
 =============================================================================== */
bool CData::isVarPosValid(int iVar, vector<double> pos)
{
	if (!getVarValid(iVar))
		return setLastError("Invalid variable index");
	return isDataPosValid(iVar, Vec3d(pos[0], pos[1], pos[2]));
}

/* =============================================================================
 =============================================================================== */
bool CData::isSameFormat(CData *pData) const
{
	if (getDimCnt() != pData->getDimCnt() || getVarCnt() != pData->getVarCnt() || getAttCnt(-1) != pData->getAttCnt(-1))
		return false;
	for (int i = 0; i < getDimCnt(); i++)
	{
		if (getDim(i).getName() != pData->getDim(i).getName() || getDim(i).getSize() != pData->getDim(i).getSize())
			return false;
	}

	for (int i = 0; i < getVarCnt(); i++)
	{
		if (getVar(i).getName() != pData->getVar(i).getName() || getVar(i).getSize() != pData->getVar(i).getSize())
			return false;
	}

	return true;
}

/* =============================================================================
 =============================================================================== */
bool CData::addRecord(vector<double> vData)
{
	if (getDimCnt() != 1)
		return false;
	if (vData.size() != getVarCnt())
		return false;

	int iNewSize = getDimSize(0) + 1;
	editDim(0).setSize(iNewSize);
	for (int i = 0; i < getVarCnt(); i++)
	{
		if (getVarIsDData(i))
			getVarDData(i).push_back(vData[i]);
		else
			getVarFData(i).push_back((float) vData[i]);
		editVar(i).setSize(iNewSize);
	}

	return true;
}

/* =============================================================================
 =============================================================================== */
Vec3d CData::getDataPos(int iVar, int i)
{
	setCoordVars(iVar);

	int iLat = getLatVar(), iLon = getLonVar(), iDepth = getDepthVar();
	int iLatSize = iLat == -1 ? 0 : getVarSize(iLat);
	int iLonSize = iLon == -1 ? 0 : getVarSize(iLon);
	int iDepthSize = iDepth == -1 ? 0 : getVarSize(iDepth);
	Vec3d pos;
	if (getDimCnt() <= 2 && getTimeSteps() > 1 && iDepthSize > iLatSize)
	{
		int iStep = iDepthSize / iLatSize;
		pos = Vec3d
			(
				iLatSize ? getVar(iLat).getDData()[i / iStep] : 0,
				iLonSize ? getVar(iLon).getDData()[i / iStep] : 0,
				iDepthSize ? getVar(iDepth).getFData()[i] : 0
			);
	}
	else if (iLatSize != iLonSize)
	{
		pos = Vec3d
			(
				iLatSize ? getVar(iLat).getDData()[i / iLonSize] : 0,
				iLonSize ? getVar(iLon).getDData()[i % iLonSize] : 0,
				iDepthSize ? getVar(iDepth).getFData()[i % iDepthSize] : 0
			);
	}
	else
	{
		pos = Vec3d
			(
				iLatSize ? getVar(iLat).getDData()[i % iLatSize] : 0,
				iLonSize ? getVar(iLon).getDData()[i % iLonSize] : 0,
				iDepthSize ? getVar(iDepth).getFData()[i % iDepthSize] : 0
			);
	}

	return pos;
}
