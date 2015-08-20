#pragma once

#include <vector>
#include <string>
using namespace std;

#include "vl/VLf.h"
#include "vl/VLd.h"
#include <sstream>
#include "assert.h"

#include "utility.h"

void setRecacheWebData(bool bReload);
void resetPerfValues();
double getPerfValue(int i);
void setPerfValue(int i, double d);
void setCOVEPaths(string strServerPath, string strLocalPath);
string getCOVELocalPath(string strPath);
string cacheCOVEFile(string strPath);
bool writePerfFile(string strPath);
vector<double> getUTMfromLL(double lat, double lon);
vector<double> getLLfromUTM(double dz_num, double dz_letter, double E, double N);

//////////////////////////  BSP TREE   ///////////////////////////////////

//simple BSP node class to create axis aligned tree over first two dimensions
class BSPNode
{
public:
	Vec3d	m_max, m_min;
	int		m_axis;
	BSPNode * m_child[2];
	vector<int> m_cell;
	vector<Vec3d> m_cellmax;
	vector<Vec3d> m_cellmin;

	BSPNode() { for (int i = 0; i < 2; i++) m_child[i] = NULL; m_axis = 1; }
	~BSPNode(){ deleteNodes(); }

	void deleteNodes();

	void setMin(Vec3d m) { m_min = m; }
	void setMax(Vec3d m) { m_max = m; }

	void appendCell(int j) { m_cell.push_back(j); }
	void appendCellmax(Vec3d m) { m_cellmax.push_back(m); }
	void appendCellmin(Vec3d m) { m_cellmin.push_back(m); }

	void create_tree(int iLeafCntMax);
	void create_nodes(int iDepth, vector<Vec3d> & p_cellmax, vector<Vec3d> & p_cellmin);
	int get_max_leaf_cnt(int iMax) const;
	bool contains(Vec3d pnt) const;
	void get_cell_list(Vec3d pnt, vector<int> &clist) const;
	void fixupChildNodes();
};

typedef BSPNode * BSPNodePtr;

//==========[ class CData ]==================================================

class CDimension {
private:
	string	m_Name;
	int		m_Size;
	bool	m_Unlimited;
	vector<float> m_CurSampling; //used to track resampling

public:
	CDimension() : m_Size(0), m_Unlimited(false) {}
	~CDimension() {	m_CurSampling.clear(); }

	string getName() const { return m_Name; }
	void setName(string str) { m_Name = str; }
	int getSize() const { return m_Size; }
	void setSize(int i) { m_Size = i; }
	bool getUnlimited() const { return m_Unlimited; }
	void setUnlimited(bool b) { m_Unlimited = b; }

	vector<float> getCurSampling() const { return m_CurSampling; }
	void updateCurSampling(vector<float> vNewDim);
};

class CAttribute {
private:
	string	m_Name;
	string	m_Text;
	vector<double> m_Data;

public:
	CAttribute() {}
	~CAttribute() {}

	void Copy(const CAttribute &a) { *this = a; }
	CAttribute(const CAttribute &a)	{ Copy(a); }

	string getName() const { return m_Name; }
	void setName(string str) { m_Name = str; }
	string getText() const { return m_Text; }
	void setText(string str) { m_Text = str; }

	double getValue(int i) const { return m_Data[i]; }
	int getDataSize() const { return m_Data.size(); }
	const vector<double> & getData() const { return m_Data; }
	bool setDataSize(int iSize) { 
		if (iSize == getDataSize())
			return true;
		try {
			m_Data.clear();	std::vector<double>().swap(m_Data);
			m_Data.resize(iSize);
		} catch (std::bad_alloc const&) {
			return false;
		}
		return true;
	}
	bool setData(vector<double> &fVec) { m_Data = fVec; return true; }
	bool setData(double fval, int iSize)
	{ 
		if (!setDataSize(iSize))
			return false;

		for (int i = 0; i < iSize; i++) 
			m_Data[i] = fval;
		return true;
	}

	string getString() const
	{
		ostringstream s;
		if (m_Text.size())
			s << m_Name << " = " << m_Text;
		else if (m_Data.size())
		{
			s << m_Name << " = " << m_Data[0];
			for (int i = 1; i < m_Data.size(); i++)
				s << ", " << m_Data[i];
		}
		return s.str();
	}
};

enum { DATA_NONE = -1, DATA_TIME, DATA_LAT, DATA_LON, DATA_DEPTH, DATA_SCALAR, DATA_ENUM, DATA_TERRAIN };

class CVariable {
private:
	string	m_Name;
	vector<int> m_DimNdx;
	vector<CAttribute> m_Att;

	vector<float>	m_FData;
	vector<double> m_DData;
	vector< vector< float > > m_TrnData;
	vector<string> m_EnumString;

	int		m_Size;
	int		m_Type;
	int		m_ID;
	bool	m_Reloadable;

	double	m_NoValue;
	double	m_Min, m_Max;

	vector<int> m_CoordVars;
	Vec3d m_CoordNoValue;

public:
	CVariable() : m_NoValue(NODATA), m_Type(DATA_NONE), m_Size(0), m_Min(0), m_Max(0), m_Reloadable(true)
	{
		m_CoordVars = vector<int>(4,-1);
		m_CoordNoValue = Vec3d(NODATA, NODATA, NODATA);
	}
	~CVariable()
	{
		clearFData();
		clearDData();
		clearTrnData();
	}

	void Copy(const CVariable &a) { *this = a; }
	CVariable(const CVariable &a)	{ Copy(a); }

	vector<int> getCoordVars() const { return m_CoordVars; }
	void setCoordVars(vector<int> vVars) { m_CoordVars = vVars; }
	Vec3d getCoordNoValue() const { return m_CoordNoValue; }
	void setCoordNoValue(Vec3d pos) { m_CoordNoValue = pos; }

	string getName() const { return m_Name; }
	void setName(string str) { m_Name = str; }

	bool isLoaded() const { return (getFDataLoaded()|| getDDataLoaded()|| getTrnDataLoaded()); }
	bool getReloadable() const { return m_Reloadable; }
	void setReloadable(bool b) { m_Reloadable = b; }

	int getFDataSize() const { return m_FData.size(); }
	bool getFDataLoaded() const { return getFDataSize() != 0; }
	const vector<float> & getFData() const { return const_cast<vector<float> &>(m_FData); }
	vector<float> & editFData() const { return const_cast<vector<float> &>(m_FData); }
	bool initFDataMem() 
	{
		if (m_Size == getFDataSize())
			return true;
		try {
			m_FData.clear();	std::vector<float>().swap(m_FData);
			m_FData.resize(m_Size);
		} catch (std::bad_alloc const&) {
			return false;
		}
		return true;
	}
	bool clearFData() { m_FData.clear(); std::vector<float>().swap(m_FData); return true; }
	bool setFDataValue(int i, float val) { m_FData[i] = val; return true; }
	float getFDataValue(int i) const { return m_FData[i]; }
	bool setFData(const vector<float> &fVec)
	{ 
		if (fVec.size() != m_Size)
			return false;
		m_FData = fVec;
		return true;
	}
	bool setFData(float fval)
	{ 
		if (!initFDataMem())
			return false;
		for (int i = 0; i < m_Size; i++)
			m_FData[i] = fval;
		return true;
	}

	int getDDataSize() const { return m_DData.size(); }
	bool getDDataLoaded() const { return getDDataSize() != 0; }
	const vector<double> & getDData() const { return const_cast<vector<double> &>(m_DData); }
	vector<double> & editDData() const { return const_cast<vector<double> &>(m_DData); }
	bool initDDataMem()
	{
		if (m_Size == getDDataSize())
			return true;
		try {
			m_DData.clear();	std::vector<double>().swap(m_DData);
			m_DData.resize(m_Size);
		} catch (std::bad_alloc const&) {
			return false;
		}
		return true;
	}
	bool clearDData() { m_DData.clear(); std::vector<double>().swap(m_DData); return true; }
	bool setDDataValue(int i, double val) { m_DData[i] = val; return true;}
	double getDDataValue(int i) const { return m_DData[i]; }
	bool setDData(const vector<double> &dVec)
	{
		if (dVec.size() != m_Size)
			return false;
		m_DData = dVec;
		return true;
	}
	bool setDData(double dval)
	{ 
		if (!initDDataMem())
			return false;
		for (int i = 0; i < m_Size; i++)
			m_DData[i] = dval;
		return true;
	}

	bool getTrnDataLoaded() const { return getTrnDataRowCnt() != 0; }
	vector< vector< float > > & getTrnData() { return m_TrnData; }
	int getTrnDataSize() const { return getTrnDataRowCnt() * getTrnDataRowLen(); }
	bool setTrnData(vector< vector< float > > &fVec) { m_TrnData = fVec; return true;}
	bool initTrnDataMem(int yDim, int xDim)
	{ 
		m_Size = yDim * xDim;
		if (m_Size == getTrnDataSize())
			return true;
		try {
			m_TrnData.clear();	std::vector< vector <float> >().swap(m_TrnData);
			m_TrnData.resize(yDim);
			for (int i = 0; i < yDim; i++) 
				m_TrnData[i].resize(xDim);
		} catch (std::bad_alloc const&) {
			return false;
		}
		return true;
	}
	bool clearTrnData() { m_TrnData.clear(); std::vector< vector <float> >().swap(m_TrnData); return true; }

	int getTrnDataRowCnt() const { return m_TrnData.size(); }
	int getTrnDataRowLen() const { return m_TrnData.size() ? (int)m_TrnData[0].size() : 0; }
	float getTrnDataValue( int yOff, int xOff ) const { return (m_TrnData[yOff])[xOff]; }
	const vector<float> & getTrnDataRow(int i) const { return const_cast<vector<float> &>(m_TrnData[i]); }
	vector<float> & editTrnDataRow(int i) const { return const_cast<vector<float> &>(m_TrnData[i]); }

	int getSize() const { return m_Size; }
	void setSize(int size) { m_Size = size; }

	bool getHasNoValue() const { return (findAtt("_FillValue") != -1); }
	double getNoValue() const { return m_NoValue; }
	void setNoValue(double val)
	{ 
		m_NoValue = val;
		//create attribute for no value
		int iAtt = findAtt("_FillValue");
		if (iAtt == -1)
		{
			CAttribute NewAtt;
			NewAtt.setName("_FillValue");
			addAtt(NewAtt);
			iAtt = getAttCnt()-1;
		}
		editAtt(iAtt).setData(val, 1);
	}

	int getID() const { return m_ID; }
	void setID(int val) { m_ID = val; }

	void setEnumValue(int i, string str)
	{
		//check for NO_VALUE for enum type
		if (str == "-1" || str == "")
		{ 
			m_FData[i] = -1; 
			return;
		}
		int ndx;
		for (ndx = 0; ndx < m_EnumString.size(); ndx++)
			if (str == m_EnumString[ndx])
				break;
		if (ndx == m_EnumString.size()) 
			m_EnumString.push_back(str);
		m_FData[i] = ndx;
	}
	int getEnumValue(int i) const { return (int)m_FData[i]; }
	string getEnumString(int i) const { return (i >= 0 && i < getEnumCnt()) ? m_EnumString[i] : ""; }
	int getEnumCnt() const { return m_EnumString.size(); }

	int getDimCnt() const { return m_DimNdx.size(); }
	int getDim (int iNdx) const
	{ 
		if (iNdx < 0 || iNdx >= getDimCnt())
			return NULL;
		return m_DimNdx[iNdx];
	}
	const vector<int> & getDims() const { return m_DimNdx; }
	vector<int> & editDims() const { return const_cast<vector<int> &>(m_DimNdx); }
	bool setDims(vector<int> &vDims) 
	{ 
		//verify thats dims are unique
		if (vDims.size() > 1)
		{
			for (int i = 0; i < vDims.size()-1; i++)
				for (int j = i+1; j < vDims.size(); j++)
					if (vDims[i] == vDims[j])
						return false;
		}
		m_DimNdx = vDims; return true;
	}

	int getAttCnt() const { return m_Att.size(); }
	bool getAttValid(int i) const { return i >= 0 && i < getAttCnt(); }
	const CAttribute & getAtt(int iAtt) const { assert(getAttValid(iAtt)); return m_Att[iAtt];	}
	CAttribute & editAtt(int iAtt) const { assert(getAttValid(iAtt)); return const_cast<CAttribute &>(m_Att[iAtt]);	}
	void addAtt(CAttribute & Att) { m_Att.push_back(Att); }
	void delAtt(int iAtt) { if (getAttValid(iAtt)) m_Att.erase(m_Att.begin()+iAtt); }

	string getAttName(int iAtt) const { return getAttValid(iAtt) ? m_Att[iAtt].getName() : ""; }
	void setAttName(int iAtt, string strName) { if (getAttValid(iAtt)) m_Att[iAtt].setName(strName); }

	string getAttText(int iAtt) const { return getAttValid(iAtt) ? m_Att[iAtt].getText() : ""; }
	void setAttText(int iAtt, string strText) { if (getAttValid(iAtt)) m_Att[iAtt].setText(strText); }

	const vector<double> & getAttData(int iAtt) const { return const_cast<vector<double> &>(m_Att[iAtt].getData()); }
	bool setAttData(int iAtt, vector<double> & vp)
	{
		if (!getAttValid(iAtt))
			return false;
		return m_Att[iAtt].setData(vp);
	}

	int findAtt(string strName) const
	{
		strName = lwrstr(strName);
		for (int i = 0; i < getAttCnt(); i++)
			if (strName == lwrstr(m_Att[i].getName()))
				return i;
		return -1;
	}
	string getAllAttText() const
	{
		string strOut;
		for (int i = 0; i < getAttCnt(); i++)
			strOut += m_Att[i].getString();
		return strOut;
	}

	int getType() const { return m_Type; }
	void setType(int i) { m_Type = i; }
	int getIsDData() const { return m_Type==DATA_TIME||m_Type==DATA_LAT||m_Type==DATA_LON; }
	int getTypeSize() const { return getIsDData() ? sizeof(double) : sizeof(float); }

	void updateMinMax();
	void initTrnMinMax() { m_Max = -1e30; m_Min = 1e30; }
	void endTrnMinMax()	{ if (m_Max == -1e30) m_Max = m_Min = 0; }
	void updateTrnMinMax(int iRow);
	double getMin() const { return m_Min; }
	double getMax() const { return m_Max; }
	void setMin(double d) { m_Min = d; }
	void setMax(double d) { m_Max = d; }

};

enum { DFORMAT_POINTS, DFORMAT_PATH, DFORMAT_STREAMNETWORK, DFORMAT_2DSURFACE, DFORMAT_3DMODEL, DFORMAT_SHAPES, DFORMAT_MAX};

enum {PART_SPEED, PART_VEL_U, PART_VEL_V, PART_VEL_W, PART_START, PART_CURTIME, PART_INDEX, PART_MAX};

class CData
{
	friend class CDataLayer;
	friend class CTerrainLayer;
private:
	vector<CDimension> m_Dim;
	vector<CAttribute> m_Att;
	vector<CVariable>  m_Var;

	int		m_NCFileID;

	vector<int> m_CoordVars;
	Vec3d	m_CoordNoValue;

	string	m_ScaleAtt;
	string	m_OffsetAtt;

	bool	m_LoadAsTerrain;
	bool	m_Dirty;
	bool	m_RegularGrid;

	int		m_TimeFormat;

	int		m_UseCount;

	BSPNode m_BSPTree;
	vector<Vec3d> m_BSPGrid;
	vector<int>	m_BSPCellNdx;
	vector<int> m_BSPDims;

	char	*m_LastError;	//stored in a pointer to allow const error setting functions

public:
	CData();
	~CData();

	void clean();

	int isCOVEFormat() const;
	int getMemSize(int iType = -1) const;
	string getHeader() const;
	int getDataFormat() const;
	string getLastError() const;

	void setBSPTree(CData* hOldData);

	bool readNetCDF(string fileName);
	bool readNetCDF_Raw(string fileName);
	bool convertNetCDF(bool bTime, bool bVars);
	bool writeNetCDF(string fileName);
	bool readTerrain(string fileName) ;
	bool writeTerrain(string fileName) { return writeNetCDF(fileName); }
	bool readCSV(string fileName);
	bool writeCSV(string fileName);

	void setDirty(bool b) { m_Dirty = b; }
	bool getDirty() const { return m_Dirty; }

	vector<double> getTimeMinMax() const;
	vector<double> getPosMinMax() const;

	bool createRegularGrid(vector<double> time, vector<double> lat, vector<double> lon, vector<float> depth);
	bool createModelGrid(vector<double> time, int iYCoord, int iXCoord, int iLevels, vector<double> lat, vector<double> lon, vector<float> depth);
	bool createPathData(const vector<double> &time, const vector<double> &lat, const vector<double> &lon, const vector<float> &depth, int depthSteps);

	bool cropDim(int iDim, int iStart, int iEnd, bool bInclude);
	bool resampleDim(int iDim, double dFactor);
	bool resetDim(int iDim, vector<float> vNewDim);

	bool filterVarDim(int iVar, vector<int>vDims, vector<float> vFilter);
	float interpVarPoint(int iVar, vector<float>vDims, int iType);

	bool scaleVar(int iVar, double dScale, double dOffset);
	bool compareVar(CData & Data, int iDataVar, int iVar);
	bool maskVar(int iVar, int iMaskVar);
	bool cropVar(int iVar, float fMin, float fMax, bool bInclude);
	int  copyVar(CData & Data, int iVar, string strNewName);

	bool combine(CData & NewData);

	bool createVertSectData(const CData & srcData, int iVar);
	bool sampleModel(CData & Model, int iSrcVar, int iDestVar);
	bool setParticleDataValues(int iValue, int iParticleCntOrig);
	bool createParticleData(CData & Model, vector<int> iFlowVars, vector<double> ppos, vector<double> time, double cycle, double step, int iValue, bool bForward, bool bTimeRelease, bool bMultiFile);

	bool PCA(int iVar);
	vector<int> PCAGetVars(int iVar);
	bool PCAProject(int iVar, vector<int> iEVars);
	bool PCAUnProject(int iVar, vector<int> iEVars);

	bool UTMtoLL(double iZoneNum, double iZoneLetter, int iVarEast, int iVarNorth, int iVarLat, int iVarLon);
	bool setDepthFromSigma(int iSigmaVar, int iBottomVar, int iDepthVar);
	bool setDepthFromPressure(int iPressureVar, int iDepthVar);

	int  getDimCnt() const;
	bool getDimValid(int iDim) const;
	int  findDim(string strName) const;
	int  newDim(string strName, int iSize);
	bool delDim(int iDim);

	string getDimName(int iDim) const;
	bool setDimName(int iDim, string strName);
	int  getDimSize(int iDim) const;
	bool setDimSize(int iDim, int iSize);
	bool getDimUnlimited(int iDim) const;
	bool setDimUnlimited(int iDim, bool b);

	int  getAttCnt(int iVar) const;
	bool getAttVarValid(int iVar) const;
	bool getAttValid(int iVar, int iAtt) const;
	int  findAtt(int iVar, string strName) const;
	int  newAtt(int iVar, string strName);
	bool delAtt(int iVar, int iAtt);

	string getAttName(int iVar, int iAtt) const;
	bool setAttName(int iVar, int iAtt, string strName);
	string getAttText(int iVar, int iAtt) const;
	bool setAttText(int iVar, int iAtt, string strText);
	vector<double> getAttData(int iVar, int iAtt) const;
	bool setAttData(int iVar, int iAtt, vector<double> vData);

	void setScaleAtt(string str) { m_ScaleAtt = str; }
	void setOffsetAtt(string str) { m_OffsetAtt = str; }

	int  getVarCnt() const;
	int	 getVarTypeCnt(int iType) const;
	bool getVarValid(int iVar) const;
	bool getVarIsDData(int i) const;
	int  findVar(string strName) const;
	int  newVar(string strName, int iType, vector<int> vDims);
	bool delVar(int iVar);

	string getVarName(int iVar) const;
	bool setVarName(int iVar, string strName);
	int  getVarType(int iVar) const;
	bool setVarType(int iVar, int ival);
	int  getVarSize(int iVar) const;
	bool updateVarMinMax(int iVar);
	double getVarMin(int iVar) const;
	double getVarMax(int iVar) const;
	double getVarNoValue(int iVar) const;
	bool setVarNoValue(int iVar, double f);

	vector<int> getCoordVars(int iVar);
	vector<double> getVarCoordNoValue(int iVar);
	bool isVarPosValid(int iVar, vector<double> pos);

	bool getVarDimValid(int iVar, int iDim) const;
	const vector<int> getVarDims(int iVar) const;
	bool setVarDims(int iVar, const vector<int> vDims);

	float getVarFDataValue(int iVar, int ival);
	bool setVarFDataValue(int iVar, int ival, float f);
	vector<float> & getVarFData(int iVar);
	bool setVarFData(int iVar, const vector<float> & vData);
	bool setVarFData(int iVar, float f);
	bool clearVarFData(int iVar);
	bool initVarFData(int iVar);

	double getVarDDataValue(int iVar, int ival);
	bool setVarDDataValue(int iVar, int ival, double d);
	vector<double> & getVarDData(int iVar);
	bool setVarDData(int iVar, const vector<double> & vData);
	bool setVarDData(int iVar, double d);
	bool clearVarDData(int iVar);
	bool initVarDData(int iVar);

	bool getVarSlice(int iVar, vector<float> &vDest, vector<int> vDims, bool bDataIndex);
	bool putVarSlice(int iVar, vector<float> &vSrc, vector<int> vDims);

	bool addRecord(vector<double> vData);

private:

	int getUseCount() const { return m_UseCount; }
	void setUseCount(int i) { m_UseCount = i; }

	bool isSameFormat(CData *pData) const;

	bool setLastError(string strError)const;

	bool getVarSlice_R(const float *pSrc, int idxSrc, float * &pDest, const vector<int> &vDims, const vector<int> &szDims, int idxDim, bool bIndex);
	bool putVarSlice_R(float * &pSrc, float *pDest, const vector<int> &vDims, const vector<int> &szDims, int idxDim);
	bool filterVarDim_R(int iVar, const vector<float> &vFilter, vector<int> &vDims, const vector<int> &szDims, int idxDim);
	float interpVar_R(const float *pSrc, const vector<float> &vDims, const vector<int> &szDims, int idxDim, int iType);

	bool sortTime();
	bool resetVarSize(int iVar);

	string getGridExtents() const;

	const CVariable & getVar(int i) const { return const_cast<CVariable &>(m_Var[i]); }
	CVariable & editVar(int i) const { return const_cast<CVariable &>(m_Var[i]); }
	const CAttribute & getAtt(int i) const { return const_cast<CAttribute &>(m_Att[i]); }
	CAttribute & editAtt(int i) const { return const_cast<CAttribute &>(m_Att[i]); }
	const CDimension & getDim(int i) const { return const_cast<CDimension &>(m_Dim[i]); }
	CDimension & editDim(int i) const { return const_cast<CDimension &>(m_Dim[i]); }

	int isNCFile() const { return m_NCFileID != -1; }
	void setNCFileID(int i) { m_NCFileID = i; }
	int getNCFileID() const { return m_NCFileID; }

	bool convertROMSCoords(int xi_rho, int eta_rho, int s_rho, int eta_u, int eta_v, int eta_psi, int s_w);
	bool convertROMSTime();
	bool convertROMS(bool bTime, bool bVars);
	bool convertRegularGrid();
	bool convertFERRET();

	bool resetCoordVars(int iVar);
	void resetCoordVars()
	{
		for (int i = 0; i < getVarCnt(); i++)
			resetCoordVars(i);
	}
	bool setCoordVars(int iVar)
	{ 
		if (!getVarValid(iVar))
			return false;
		m_CoordVars = getVar(iVar).getCoordVars();
		m_CoordNoValue = getVar(iVar).getCoordNoValue();
		return true;
	}
	int getLatVar() const { return m_CoordVars[DATA_LAT]; }
	int getLonVar() const { return m_CoordVars[DATA_LON]; }
	int getDepthVar() const { return m_CoordVars[DATA_DEPTH]; }
	int getTimeVar() const { return m_CoordVars[DATA_TIME]; }
	Vec3d getCoordNoValue() const { return m_CoordNoValue; }

	int getDepthCnt() const
	{  
		if ( getDepthVar() < 0)
			return 1;
		if (getLatCnt() * getLonCnt() == getVarSize(getDepthVar()))
			return 1;
		return getDim(getVar(getDepthVar()).getDim(0)).getSize();
	}
	int getLatCnt() const
	{ 
		if ( getLatVar() < 0)
			return 1;
		const CVariable & var = getVar(getLatVar());
		if (var.getDimCnt() <= 2 && getTimeSteps() == getDim(var.getDim(0)).getSize())
			return var.getSize();
		int ndx = max(0,var.getDimCnt()-2);
		return getDim(var.getDim(ndx)).getSize();
	}
	int getLonCnt() const
	{
		if ( getLonVar() < 0)
			return 1;
		const CVariable & var = getVar(getLonVar());
		if (var.getDimCnt() <= 2 && getTimeSteps() == getDim(var.getDim(0)).getSize())
			return var.getSize();
		int ndx = max(0,var.getDimCnt()-1);
		return getDim(var.getDim(ndx)).getSize();
	}
	int getTimeSteps() const
	{
		if (getTimeVar() < 0)
			return 1;
		return getVarSize(getTimeVar());
	}
	double getDataTime(int i) const
	{ 
		if (getTimeVar() < 0)
			return 0;
		return getVar(getTimeVar()).getDDataValue(i); 
	}
	int getTimeSliceSize(int iVar) const { return getVarSize(iVar) / getTimeSteps(); }
	void getTimeSlice(int iVar, int iSlice, vector<float> & pVal);

	int getFirstOfType(int iType)
	{
		for (int i = 0; i < getVarCnt(); i++)
			if (m_Var[i].getType() == iType)
			return i;
		return -1;
	}
	int getLargestOfType(int iType) const
	{
		int iSize = 0;
		int iVar = -1;
		for (int i = 0; i < getVarCnt(); i++)
			if (m_Var[i].getType() == iType && m_Var[i].getSize() > iSize)
			{
				iSize = m_Var[i].getSize();
				iVar = i;
			}
			return iVar;
	}

	bool loadVar(int iVar);
	bool scale_var(int iVar);
	bool scale_var(int iVar, double dScale, double dOffset);
	int guessVarType(int iVar);
	void setExtents();

	void setRegularGrid(bool b) { m_RegularGrid = b; }
	bool getRegularGrid() const { return m_RegularGrid; }
	bool checkRegularGrid();

	void setTimeFormat(int i) { m_TimeFormat = i; }
	bool getTimeFormat() { return m_TimeFormat; }

	bool initTerrain(int xDim, int yDim, double lonMin, double latMax, float xSpacing, float ySpacing, float fNoData, bool bCenter);
	void setLoadAsTerrain(bool b) { m_LoadAsTerrain = b; }
	bool getLoadAsTerrain() { return m_LoadAsTerrain; }
	bool loadTerrainBIL(string fileName);
	bool loadTerrainASC(string fileName);
	bool convertTerrainToDataset();
	bool loadTerrainVar(CData &Source, int iVar, int yDim, int xDim, bool bFlip);

	bool createModelBSPTree(int iSrcVar);
	int getGridCell(Vec3d pos) const;
	double getGridValue(int iSrcVar, Vec3d pos, double time, int iCell, int ndx_time);
	void setBSPDims(vector<int> dimlist)
	{
		m_BSPDims.clear();
		for (int i = 0; i < dimlist.size(); i++)
			m_BSPDims.push_back(dimlist[i]);
	}
	bool matchBSPDims(vector<int> dimlist)
	{
		if (m_BSPDims.size() != dimlist.size())
			return false;
		for (int i = 0; i < dimlist.size(); i++)
			if (m_BSPDims[i] != dimlist[i])
				return false;
		return true;
	}

	Vec3d getDataPos(int iVar, int i);

	Vec3d getDataCoordNoValue(int iVar)
	{
		setCoordVars(iVar);
		return m_CoordNoValue;
	}
	bool isDataPosValid(int iVar, Vec3d pos)
	{
		setCoordVars(iVar);
		for (int i = 0; i < 3; i++)
			if (m_CoordNoValue[i]!=0 && pos[i] == m_CoordNoValue[i])
				return false;
		return true;
	}
	bool getVarPosSlice(int iVar, vector<Vec3d> &pDest, const vector<int> &dims, int iStart=0, int iSize=-1);
	vector<double> advectParticles(vector<int> iFlowVars, vector<double> ppos, vector<double> time, double cycle, double step, bool bForward, bool bTimeRelease);
};

typedef CData * CDataPtr;
