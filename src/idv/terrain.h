#pragma once

#include "scene/scene_mgr.h"
#include "layer.h"
#include "data_mgr.h"
#include "const.h"
#include <string>

//==========[ class CTerrain ]==================================================

const int TREE_COLS_MAX=10;
const int TREE_ROWS_MAX=5;

const int DOWNLOAD_TRY_MAX=3;

Vec3f getScenePntFromGPS(Vec3d pos, t_scale scale);
Vec3d getGPSFromScenePnt(Vec3f pos, t_scale scale);
double getSceneDistScale(Vec3f pos1, Vec3f pos2);
Vec3d getSceneRotFromGPS(Vec3d pos);
Vec3d getSceneRot(Vec3f pos_in);
double getObjectRot();
Vec3d recenterGPS(Vec3d pos);

string getNewName(string strName, int i);

bool writeBilFile(string fileName, float** pData, int iRows, int iCols, bool bFlip);
bool writeBilFileHeader(string fileName, int iRows, int iCols, short iNoData, float ulxmap, float ulymap, float fCellSizeX, float  fCellSizeY);

// The data structure that actually holds the bathymetric data and it's metadata
class CTerrainLayer : public CLayer
{
private:
	CDataPtr m_DataPtr;

	float ** m_RowData; //cache of values in m_Data for speed
	float	m_NoData;
	int		m_Cols;
	int		m_Rows;
	float	m_CellSizeX;
	float	m_CellSizeY;

	float	m_Scale;
	float	m_Offset;
	bool	m_Wrap;

public:
	CTerrainLayer() : m_DataPtr(NULL), m_RowData(NULL), m_Scale(1.0f), m_Offset(0.0f), 
		m_NoData(NODATA), m_Wrap(false) {}
	~CTerrainLayer() { clean(); }
	void clean()
	{
		if (m_DataPtr) delete m_DataPtr; m_DataPtr = NULL;
		if (m_RowData) delete m_RowData; m_RowData = NULL;
	}
	CTerrainLayer(CTerrainLayer const &t)
	{
		*this = t;
		clearDataPtrs();
		setActive(false);
	}
	CTerrainLayer & operator = (const CTerrainLayer & t)
	{
		CLayer::operator =(t);
		setScale(t.getScale());
		setOffset(t.getOffset());
		return *this;
	}

	void clearDataPtrs()
	{
		m_DataPtr = NULL;
		m_RowData = NULL;
	}

	CData & getData() const { return *m_DataPtr; }
	float** getRowData() const { return m_RowData; }
	void setRowData(float** ppf) { if (m_RowData) delete m_RowData; m_RowData = ppf; }
	float getVal( int yOff, int xOff ) const { return m_RowData[yOff][xOff]; }

	int		getCols() const { return m_Cols; }
	int		getRows() const { return m_Rows; }
	float	getCellSizeX() const { return m_CellSizeX; }
	float	getCellSizeY() const { return m_CellSizeY; }

	bool setActive(bool b);

	bool getWrap() const { return m_Wrap; }
	void setWrap(bool b) { m_Wrap = b; }

	float getScale() const { return m_Scale; }
	void setScale(float f) { m_Scale = f; }
	float getOffset() const { return m_Offset; }
	void setOffset(float f) { m_Offset = f; }

	bool update(string name, string link, string id, bool bActive);
	bool writeTerrain(string file) { return writeBILFile(file); }
	bool completeLoad();

	bool getPosHeight( double dLat, double dLon, float &fDepth );

private:
	bool readTerrainFile(string fileName);
	bool writeBILFile(string fileName);
};

typedef CTerrainLayer * CTerrainLayerPtr;


class CTerrainSet
{
private:
	vector<CTerrainLayerPtr> m_TerrainLayerList;
	vector<CTerrainLayerPtr> m_ActiveLayerList;

	Vec3d	m_MinPos;
	Vec3d	m_MaxPos;

	Vec3d	m_MeshCenterGPS;
	double	m_TileScale;
	double	m_TileFactor;
	double	m_ShowSkirts;

public:
	CTerrainSet()
	{
		m_MinPos = vl_0; m_MaxPos = vl_0;
		m_MeshCenterGPS = vl_0;
		m_TileFactor = 1.0;
		m_TileScale = 1.0;
		m_ShowSkirts = true;
	}
	~CTerrainSet()
	{
		while (getTerrainLayerCnt())
			delTerrainLayer(0);
	}

	Vec3d getMeshCenterGPS() const;
	bool setMeshCenterGPS(Vec3d vPos);
	double getTileFactor() const { return m_TileFactor; }
	void setTileFactor(double d) { m_TileFactor = d; }
	double getTileScale() const { return m_TileScale; }
	void setTileScale(double d) { m_TileScale = d; }
	bool getShowSkirts() const { return m_ShowSkirts; }
	void setShowSkirts(bool b) { m_ShowSkirts = b; }

	Vec3d getMinPos() const {return m_MinPos;}
	Vec3d getMaxPos() const {return m_MaxPos;}
	bool getMinCellSize(double m_LonMin, double m_LatMin, double m_LonMax, double m_LatMax, double &fMinCell, int *pTable = NULL);
	int  getMaxLevel(double m_LonMin, double m_LatMin, double m_LonMax, double m_LatMax);

	bool getPosHeight( double dLat, double dLon, float &fDepth);
	bool getGridPosHeight( double dLat, double dLon, float &fDepth );
	bool getGrid(double dLonMin, double dLonMax, double dLatMin, double dLatMax, float *pTerrain, Vec3f *vBmpNormals, int iCols, int iRows);
	bool createGridNormals(float * pHeights, int iRows, int iCols, float fCellSize, Vec3f *pNormals);

	void updateActiveLayerList();
	CTerrainLayer & getActiveLayer(int iNdx) { return *(m_ActiveLayerList[iNdx]); }
	int getActiveLayerCnt() const { return m_ActiveLayerList.size(); }

	bool setActive(int i, bool b)
	{
		bool bRet = getTerrainLayer(i).setActive(b);
		updateActiveLayerList();
		return bRet;
	}

	int getTerrainLayerCnt() const { return m_TerrainLayerList.size(); }
	bool getTerrainLayerValid(int i) const { return i >= 0 && i < getTerrainLayerCnt(); }

	CTerrainLayer & getTerrainLayer(int iNdx) const
	{ 
		assert(getTerrainLayerValid(iNdx));
		return *m_TerrainLayerList[iNdx];
	}
	int getTerrainLayerIndex(string name) const
	{ 
		for (int i = 0; i < getTerrainLayerCnt(); i++)
			if (name == getTerrainLayer(i).getName())
				return i;
		return -1;
	}
	int getTerrainLayerIndex(const CTerrainLayerPtr hTerrainLayer) const
	{ 
		for (int i = 0; i < getTerrainLayerCnt(); i++)
			if (hTerrainLayer == getTerrainLayerPtr(i))
				return i;
		return -1;
	}

	const CTerrainLayerPtr getTerrainLayerPtr(int iNdx) const
	{ 
		if (!getTerrainLayerValid(iNdx))
			return NULL;
		return m_TerrainLayerList[iNdx];
	}

	int getActiveCnt() const
	{
		int iActive = 0;
		for (int i = 0; i < getTerrainLayerCnt(); i++)
			if (getTerrainLayer(i).getActive())
				iActive++;
		return iActive;
	}

	bool addTerrainLayer(CTerrainLayer & TerrainLayer, int iNdx = -1)
	{
		if (!getTerrainLayerValid(iNdx))
		{
			m_TerrainLayerList.push_back(new CTerrainLayer());
			iNdx = getTerrainLayerCnt() - 1;
		}
		else
		{
			vector<CTerrainLayerPtr> :: iterator iter = m_TerrainLayerList.begin();
			for (int i = 0; iter != m_TerrainLayerList.end() && i < iNdx; iter++, i++);
			m_TerrainLayerList.insert(iter, new CTerrainLayer());
		}
		return editTerrainLayer(TerrainLayer, iNdx);
	}

	bool editTerrainLayer(CTerrainLayer & TerrainLayer, int iNdx)
	{
		if (!getTerrainLayerValid(iNdx))
			return false;
		getTerrainLayer(iNdx) = TerrainLayer;
		return true;
	}

	bool delTerrainLayer(int iNdx)
	{ 
		if (!getTerrainLayerValid(iNdx))
			return false;
		delete (m_TerrainLayerList[iNdx]);
		m_TerrainLayerList.erase(m_TerrainLayerList.begin() + iNdx);
		updateActiveLayerList();
		return true;
	}

	bool moveTerrainLayerUp(int iNdx)
	{ 
		if (!getTerrainLayerValid(iNdx))
			return false;
		string strGroup = getTerrainLayer(iNdx).getGroup();
		int iSwap = -1;
		for (int i = 0; i < getTerrainLayerCnt(); i++)
		{
			if (i == iNdx)	break;
			if (strGroup != getTerrainLayer(i).getGroup())	continue;
			iSwap = i;
		}
		if (iSwap == -1)
			return false;
		CTerrainLayerPtr tmp = m_TerrainLayerList[iNdx];
		m_TerrainLayerList[iNdx] = m_TerrainLayerList[iSwap];
		m_TerrainLayerList[iSwap] = tmp;
		return true;
	}

};


//  Class to hold all the information for making terrain gradients

class CGradient
{
private:
	vector<string> m_File;
	vector<double> m_Min;
	vector<double> m_Max;

	unsigned char * m_Map;
	vector<double> m_Depth;
	vector<double> m_TxVal;
	int		m_MapWidth;
	int		m_MapHeight;

	int		m_TexID;
	unsigned char *  m_Bmp;
	int		m_BmpWidth;
	int		m_BmpHeight;

	double	m_Contour;
	bool	m_GradHiRes;
	bool	m_GradBins;

public:
	CGradient()
	{
		m_TexID = -1;
		m_Map = NULL;
		m_Bmp = NULL;
		m_MapWidth = 0;
		m_MapHeight = 0;
		m_BmpWidth = 0;
		m_BmpHeight = 0;
		m_Contour = 0.0;
		m_GradHiRes = false;
		m_GradBins = false;
	}

	~CGradient() { clean(); }

	void cleanFiles()
	{
		m_File.clear(); 
		m_Min.clear();
		m_Max.clear();
	}
	void cleanData()
	{
		if (m_Map) delete [] m_Map; m_Map = NULL;
		m_MapWidth = m_MapHeight = 0;
		if (m_Bmp) delete [] m_Bmp; m_Bmp = NULL;
		m_BmpWidth = m_BmpHeight = 0;
		deleteTexture(m_TexID); m_TexID = -1;
	}
	void clean()
	{
		cleanData();
		cleanFiles();
	}

	CGradient(CGradient const &t)
	{
		*this = t;
		clearDataPtrs();
	}

	bool isSame(const CGradient & New) const
	{
		return (m_File == New.getFileList() && m_Min == New.getMinList() && m_Max == New.getMaxList()
			&& m_Contour == New.getContour() && m_GradHiRes == New.getGradHiRes() && m_GradBins == New.getGradBins());
	}
	void clearDataPtrs()
	{
		m_TexID = -1;
		m_Map = NULL;
		m_Bmp = NULL;
	}

	bool addFile(string fName, double fMin, double fMax);
	bool updateMap();
	unsigned char * createGradBitmap(float *pGradVals, int iCnt, bool bLine, bool bBins, bool bFlip, bool bLog);

	int getCnt() const { return m_File.size(); }
	double getMin(int i) const { return m_Min[i]; }
	double getMax(int i) const { return m_Max[i]; }
	string getFile(int i) const { return m_File[i]; }

	vector<string> getFileList() const { return m_File; }
	vector<double> getMinList() const { return m_Min; }
	vector<double> getMaxList() const { return m_Max; }

	int getTexID() const { return m_TexID; }
	void setTexID(int id)
	{ 
		deleteTexture(m_TexID);
		m_TexID = id;
	}

	double getMapDepth(int i) const { return m_Depth[i]; }
	double getMapVal(int i) const { return m_TxVal[i]; }
	int	getMapCnt() const { return m_Depth.size(); }
	int getMapWidth() const { return m_MapWidth; }

	int getBmpHeight() const { return m_BmpHeight; }
	int getBmpWidth() const { return m_BmpWidth; }
	unsigned char * getBmp() const { return m_Bmp; }

	unsigned char * getGradMap() const { return m_Map; }

	color getColor(double dVal) const;

	bool getGradHiRes() const { return m_GradHiRes; }
	void setGradHiRes(bool b) { m_GradHiRes = b; }
	bool getGradBins() const { return m_GradBins; }
	void setGradBins(bool b) { m_GradBins = b; }
	double getContour() const { return m_Contour; }
	void setContour(double f) { m_Contour = f; }
};

typedef CGradient * CGradientPtr;

enum { LAYER_NORMAL, LAYER_SEALEVEL, LAYER_POSITIVE, LAYER_NEGATIVE};
enum { SCREEN_PIXELS, SCREEN_PERCENT};

class CTextureLayer : public CLayer
{
private:
	CGradient	m_Gradient;
	C3DModel	m_Model;

	string	m_ID;
	string	m_ImageType;
	int		m_MinLevel;
	int		m_MaxLevel;

	double  m_Transparency;
	bool	m_ForceTransparency;
	bool	m_Vertical;
	double	m_VertScale;

	int		m_LayerType;

	int		m_ScreenType;
	int		m_ScreenTex;
	int		m_ScreenW, m_ScreenH;

	//CMovie *m_MoviePtr;
	double	m_Start;
	double	m_Finish;
	bool	m_Loop;

public:
	CTextureLayer() : m_MaxLevel(3), m_MinLevel(0), m_Transparency(1.0), 
		m_ForceTransparency(false), m_Vertical(false), m_VertScale(1.0),
		m_LayerType(LAYER_NORMAL), m_ScreenType(SCREEN_PERCENT), m_ScreenTex(-1), 
		m_ScreenW(0), m_ScreenH(0), m_ID("image")
		//m_Start(NO_TIME), m_Finish(NO_TIME), m_Loop(false), m_MoviePtr(NULL)
	{
	}

	~CTextureLayer() { clean(); }

	void cleanObjects() { m_Model.clean(); }
	void clean() 
	{
		cleanObjects();
		if (m_ScreenTex != -1) deleteTexture(m_ScreenTex); m_ScreenTex = -1;
		//if (m_MoviePtr) delete m_MoviePtr; m_MoviePtr = NULL;
	}

	CTextureLayer(CTextureLayer const &t)
	{
		*this = t;
		editGradient().clearDataPtrs();
		editModel().clearDataPtrs();
		m_ScreenTex = -1;
		//m_MoviePtr = NULL;
		setLoaded(false);
	}

	CTextureLayer & operator = (const CTextureLayer & t)
	{
		CLayer::operator =(t);
		setReload(getReload() || (m_Transparency != t.getTransparency()));

		if (getIsImage() && getReload())
		{
			deleteTexture(m_ScreenTex);
			m_ScreenTex = -1;
		}
		m_ID = t.getID();
		m_ImageType = t.getImageType();
		m_Transparency = t.getTransparency();
		m_ForceTransparency = t.getForceTransparency();
		m_Vertical = t.getVertical();
		m_VertScale = t.getVertScale();
		m_MinLevel = t.getMinLevel();
		m_MaxLevel = t.getMaxLevel();
		m_LayerType = t.getLayerType();
		m_ScreenType = t.getScreenType();

		m_Start = t.getStart();
		m_Finish = t.getFinish();
		m_Loop = t.getLoop();

		//force gradient to update if used
		if (!m_Gradient.isSame(t.getGradient()))
		{
			m_Gradient.clean();
			m_Gradient = CGradient(t.getGradient());
			setReload(true);
		}
		return *this;
	}

	void Render()
	{
		if (getActive()) {
			getModel().Render();
		}
	}

	const C3DModel & getModel() { return m_Model; }
	C3DModel & editModel() { return m_Model; }

	int	getMaxLevel() const { return m_MaxLevel; }
	void setMaxLevel(int i)	{ m_MaxLevel = i; }
	int	getMinLevel() const { return m_MinLevel; }
	void setMinLevel(int i)	{ m_MinLevel = i; }

	int	getLayerType() const { return m_LayerType; }
	void setLayerType(int i)	{ m_LayerType = i; }
	int	getScreenType() const { return m_ScreenType; }
	void setScreenType(int i)	{ m_ScreenType = i; }
	int	getScreenTex() const { return m_ScreenTex; }
	void setScreenTex(int i)	{ m_ScreenTex = i; }
	int	getScreenW() const { return m_ScreenW; }
	void setScreenW(int i)	{ m_ScreenW = i; }
	int	getScreenH() const { return m_ScreenH; }
	void setScreenH(int i)	{ m_ScreenH = i; }

	double getTransparency() const { return m_Transparency; }
	void setTransparency(double t) { m_Transparency = t; }
	bool getForceTransparency() const { return m_ForceTransparency; }
	void setForceTransparency(bool b) { m_ForceTransparency = b; }

	bool getVertical() const { return m_Vertical; }
	void setVertical(bool b) { m_Vertical = b; }
	double	getVertScale() const { return m_VertScale; }
	void setVertScale(double d)	{ m_VertScale = d; }

	string getID() const { return m_ID; }
	void setID(string str) { m_ID = str; }

	string getImageType() const { return m_ImageType; }
	void setImageType(string str) { m_ImageType = str; }
	string getImageExt() const { return getImageType() == "image/dds" ? "dds" : 
		getImageType() == "image/jpeg" ? "jpg" : 
		getImageType() == "image/png" ? "png": "";
	}

	const CGradient & getGradient() const { return m_Gradient; }
	CGradient & editGradient() const { return const_cast<CGradient &>(m_Gradient); }

	bool setActive(bool b);

	bool update(string name, string link, string id, bool bActive);

	bool getIsImage() const { return getID() == "image"; }
	bool getIsGrad() const { return getID() == "grad"; }
	bool getIsScreen() const { return getID() == "screen"; }
	bool getIsTiled() const { return !getIsImage() && !getIsGrad() && !getIsScreen(); }

	bool getEarthImage() const { return (m_MaxPos[0] - m_MinPos[0] == 180.0)
		&& (m_MaxPos[1] - m_MinPos[1] == 360.0); }

	bool getBackground() const 
	{ 
		return (getEarthImage() && m_Transparency == 1.0);
	}

	void setSizeToTerrain(CTerrainSet & Terrain)
	{
		m_MinPos = Terrain.getMinPos();
		m_MaxPos = Terrain.getMaxPos();
	}
	bool getImageTexturePath(string &path, int level) const;
	bool getTiledTexturePath(string &path, int level, int row, int col) const;
	bool getTiledTextureURL(string &path, int level, int row, int col);

	double getStart() const { return m_Start; }
	void setStart(double t) { m_Start = t; }
	double getFinish() const { return m_Finish; }
	void setFinish(double t) { m_Finish = t; }
	bool getLoop() const { return m_Loop; }
	void setLoop(bool b) { m_Loop = b; }

	bool	openVideo(string FileName);
	bool	setVideoFrame(double dtime);
	//const CMovie* getMoviePtr() const { return m_MoviePtr; }
	//CMovie* editMovie() const { return m_MoviePtr; }
};

typedef CTextureLayer * CTextureLayerPtr;

class CTextureSet
{
private:
	vector<CTextureLayerPtr> m_TextureLayerList;

	t_scale	m_Scale;
	double	m_Shading;

public:
	CTextureSet() {}
	~CTextureSet()
	{
		while (getTextureLayerCnt())
			delTextureLayer(0);
	}

	void Render()
	{
		for (int i = 0; i < getTextureLayerCnt(); i++)
		{
			if (!getTextureLayer(i).getActive()) {
				continue;
			}
			getTextureLayer(i).editModel().setScale(m_Scale);
			getTextureLayer(i).editModel().setShading(m_Shading);
			getTextureLayer(i).Render();
		}
	}

	t_scale getScale() { return m_Scale; }
	void setScale(t_scale scale)
	{
		m_Scale = scale;
	}
	float getShading() { return m_Shading; }
	void setShading(float shading)
	{
		m_Shading = shading;
	}

	bool setActive(int i, bool b) { return getTextureLayer(i).setActive(b); }

	int getCurrentBackground() const
	{
		for (int i = getTextureLayerCnt()-1; i >= 0 ; i--)
			if (getTextureLayer(i).getActive() && getTextureLayer(i).getBackground())
				return i;
		return -1;
	}

	int getCurGradIndex() const
	{
		for (int i = getTextureLayerCnt()-1; i >= 0 ; i--)
			if (getTextureLayer(i).getActive() && getTextureLayer(i).getIsGrad())
				return i;
		return -1;
	}

	int getTextureLayerCnt() const { return m_TextureLayerList.size(); }
	bool getTextureLayerValid(int i) const { return i >= 0 && i < getTextureLayerCnt(); }

	CTextureLayer & getTextureLayer(int iNdx) const
	{ 
		assert(getTextureLayerValid(iNdx));
		return *m_TextureLayerList[iNdx];
	}
	int getTextureLayerIndex(string name) const
	{ 
		for (int i = 0; i < getTextureLayerCnt(); i++)
			if (name == getTextureLayer(i).getName())
				return i;
		return -1;
	}
	int getTextureLayerIndex(const CTextureLayerPtr hTextureLayer) const
	{ 
		for (int i = 0; i < getTextureLayerCnt(); i++)
			if (hTextureLayer == getTextureLayerPtr(i))
				return i;
		return -1;
	}
	const CTextureLayerPtr getTextureLayerPtr(int iNdx) const
	{ 
		if (!getTextureLayerValid(iNdx))
			return NULL;
		return m_TextureLayerList[iNdx];
	}

	int unloadMaterials()
	{ 
		for (int i = 0; i < getTextureLayerCnt(); i++)
			getTextureLayer(i).cleanObjects();
		return -1;
	}

	int setDirty()
	{ 
		for (int i = 0; i < getTextureLayerCnt(); i++)
			if (getTextureLayer(i).getActive())
				getTextureLayer(i).editModel().setDirty(true);
		return -1;
	}

	void prepareTextures(double dtime)
	{
		for (int i = 0; i < getTextureLayerCnt(); i++)
			if (getTextureLayer(i).getActive())
				getTextureLayer(i).setVideoFrame(dtime);
	}

	bool getMovieActive() const
	{
		for (int i = 0; i < getTextureLayerCnt(); i++)
			if (getTextureLayer(i).getActive() && getExt(getTextureLayer(i).getLink()) == ".mp4")
				return true;
		return false;
	}

	int getActiveCnt() const
	{
		int iActive = 0;
		for (int i = 0; i < getTextureLayerCnt(); i++)
			if (getTextureLayer(i).getActive())
				iActive++;
		return iActive;
	}

	bool addTextureLayer(CTextureLayer & TextureLayer, int iNdx = -1)
	{
		if (!getTextureLayerValid(iNdx))
		{
			m_TextureLayerList.push_back(new CTextureLayer());
			iNdx = getTextureLayerCnt() - 1;
		}
		else 
		{
			vector<CTextureLayerPtr> :: iterator iter = m_TextureLayerList.begin();
			for (int i = 0; iter != m_TextureLayerList.end() && i < iNdx; iter++, i++);
			m_TextureLayerList.insert(iter, new CTextureLayer());
		}
		return editTextureLayer(TextureLayer, iNdx);
	}

	bool editTextureLayer(CTextureLayer & TextureLayer, int iNdx)
	{
		if (!getTextureLayerValid(iNdx))
			return false;
		getTextureLayer(iNdx) = TextureLayer;
		return true;
	}

	bool delTextureLayer(int iNdx)
	{ 
		if (!getTextureLayerValid(iNdx))
			return false;
		delete (m_TextureLayerList[iNdx]);
		m_TextureLayerList.erase(m_TextureLayerList.begin() + iNdx);
		return true;
	}

	bool moveTextureLayerUp(int iNdx)
	{ 
		if (!getTextureLayerValid(iNdx))
			return false;
		string strGroup = getTextureLayer(iNdx).getGroup();
		int iType = getTextureLayer(iNdx).getIsTiled() ? 0 : getTextureLayer(iNdx).getIsGrad() ? 1 : 2;
		int iSwap = -1;
		for (int i = 0; i < getTextureLayerCnt(); i++)
		{
			if (i == iNdx)	break;
			int iterType = getTextureLayer(i).getIsTiled() ? 0 : getTextureLayer(i).getIsGrad() ? 1 : 2;
			if (strGroup != getTextureLayer(i).getGroup() || iType != iterType)
				continue;
			iSwap = i;
		}
		if (iSwap == -1)
			return false;
		CTextureLayerPtr tmp = m_TextureLayerList[iNdx];
		m_TextureLayerList[iNdx] = m_TextureLayerList[iSwap];
		m_TextureLayerList[iSwap] = tmp;
		return true;
	}

};

const int MESH_GRID=48;

class CTerrainTile
{
private:
	CTerrainSet * m_TerrainSetPtr;
	CTextureSet * m_TextureSetPtr;
	CTerrainTile * m_Parent;

	vector<CTerrainTile> m_Child;
	int		m_ChildRows;
	int		m_ChildCols;

	vector<C3DObjectPtr> m_TileObjects;
	vector<CTextureLayerPtr> m_TexLayerPtr;
	int		m_MeshRows;
	int		m_MeshCols;
	bool	m_Skirt;

	double	m_LonMin, m_LonMax;
	double	m_LatMin, m_LatMax;
	double	m_MinDepth, m_MaxDepth;

	int		m_Level;
	int		m_MaxLevel;
	int		m_TileCount;
	int		m_TileRow;
	int		m_TileCol;

	Vec3f	m_BBoxCenter;
	float	m_BBoxRadius;
	float	m_BBoxScale[2];
	Vec3f	m_BBoxCorners[4];

	double	m_LastDrawn;

	float * m_BmpTerrain;
	int		m_BmpSize;

	vector<int>		m_TextureStatus;
	vector<int>		m_TryCount;
	bool			m_NeedUpdate;

public:
	CTerrainTile(CTerrainTile *pParent = NULL, int level = 0, int row = 0, int col = 0)
	{
		init(pParent, level, row, col);
		m_ChildRows = m_ChildCols = 0;
		m_MaxLevel = -1;
		m_MinDepth = m_MaxDepth = 0;
		m_BBoxScale[0] = m_BBoxScale[1] = -1;
		m_Skirt = false;

		m_ChildRows = 2;
		m_ChildCols = 2;
		m_MeshCols = MESH_GRID+1;
		m_MeshRows = MESH_GRID+1;

		m_BmpTerrain = NULL;
		m_BmpSize = 0;

		m_NeedUpdate = true;

	}
	void init(CTerrainTile *pParent, int level, int row, int col)
	{
		m_Parent = pParent;
		m_Level = level;
		m_TileRow = row;
		m_TileCol = col;
		if (m_Parent)
			setEnvironment(getParent().m_TerrainSetPtr, getParent().m_TextureSetPtr);
		else
			setEnvironment(NULL, NULL);
	}
	void setEnvironment(CTerrainSet *pTerrain, CTextureSet *pTexture)
	{
		m_TerrainSetPtr = pTerrain;
		m_TextureSetPtr = pTexture;
	}

	~CTerrainTile()
	{
		//remove tile objects from texture layers
		for (int i = 0; i < getTileObjectCnt(); i++)
		{
			CTextureLayerPtr pTex = getTextureLayerPtr(i);
			if (m_TextureSetPtr->getTextureLayerIndex(pTex) != -1)
				pTex->editModel().delObject(m_TileObjects[i]);
		}
		if (m_BmpTerrain)
			delete [] m_BmpTerrain;
		deleteChildren();
	}

	int getChildCnt() const { return m_Child.size(); }
	void deleteChildren();
	const CTerrainTile & getChild(int i) const { return m_Child[i]; }
	const CTerrainTile & getParent() const { return *m_Parent; }
	CTerrainTile & editParent() const { return *m_Parent; }
	CTerrainSet & getTerrainSet() const { return *m_TerrainSetPtr; }
	CTextureSet & getTextureSet() { return *m_TextureSetPtr; }
	bool getTerrainValid() const { return m_TerrainSetPtr && m_TextureSetPtr; }

	void setBounds(double lonmin, double lonmax, double latmin, double latmax)
	{
		m_LonMin = lonmin;
		m_LonMax = lonmax;
		m_LatMin = latmin;
		m_LatMax = latmax;
		float fdepth[4];
		getTerrainSet().getPosHeight(m_LatMin, m_LonMin, fdepth[0]);
		getTerrainSet().getPosHeight(m_LatMin, m_LonMax, fdepth[1]);
		getTerrainSet().getPosHeight(m_LatMax, m_LonMin, fdepth[2]);
		getTerrainSet().getPosHeight(m_LatMax, m_LonMax, fdepth[3]);
		setDepthMinMax(fdepth, 4);
		m_MaxLevel = getTerrainSet().getMaxLevel(m_LonMin,m_LatMin,m_LonMax,m_LatMax);
	}
	void setChildRows(int i) { m_ChildRows = i; }
	bool setDepthMinMax(float *pPnts, int iCnt);
	void setChildCols(int i) { m_ChildCols = i; }
	void setMeshRows(int i) { m_MeshRows = i; }
	void setMeshCols(int i) { m_MeshCols = i; }

	int	getTextureStatus(int ndx) const { return m_TextureStatus[ndx]; }
	void setTextureStatus(int ndx, int i) { m_TextureStatus[ndx] = i; }
	int	getTryCount(int ndx) const { return m_TryCount[ndx]; }
	void setTryCount(int ndx, int i) { m_TryCount[ndx] = i; }
	bool getSkirt() const { return m_Skirt; }
	void setSkirt(bool b) { m_Skirt = b; }

	double getMinDepth() const { return m_MinDepth; }
	double getMaxDepth() const { return m_MaxDepth; }
	bool getNoTerrain() const { return m_MaxLevel == -1; }
	int getLevel() const { return m_Level; }

	bool createTerrainTexture(unsigned char * &pTerrainData, int iCols, int iRows, bool bShading, bool bFlip);
	bool updateTerrainTexture(int iObj);
	void updateTextureLayers();
	void resetTexCoords(int iObj);
	float get1DTexCoord(int iObj, float fDepth);
	void setSkirtHeight(int iObj, bool bSkirt);

	int getTileObjectCnt() const { return (int)m_TileObjects.size(); }
	bool getTileObjectValid(int i) const { return i >= 0 && i < getTileObjectCnt(); }
	void clearTileObject(int i)
	{ 
		m_TileObjects.erase(m_TileObjects.begin()+i);
		m_TexLayerPtr.erase(m_TexLayerPtr.begin()+i);
	}
	void addTileObject(C3DObjectPtr pObj, CTextureLayerPtr pTex)
	{
		m_TileObjects.push_back(pObj);
		m_TexLayerPtr.push_back(pTex);
	}

	C3DObject & getTileObject(int iNdx) const
	{ 
		assert(getTileObjectValid(iNdx));
		return *m_TileObjects[iNdx];
	}
	C3DObjectPtr getTileObjectPtr(int iNdx) const
	{ 
		assert(getTileObjectValid(iNdx));
		return m_TileObjects[iNdx];
	}

	CTextureLayerPtr getTextureLayerPtr(int iNdx) const
	{ 
		assert(getTileObjectValid(iNdx));
		return m_TexLayerPtr[iNdx];
	}
	int getTextureLayerIndex(CTextureLayerPtr hTx) const
	{
		for (int i = 0; i < getTileObjectCnt(); i++)
			if (getTextureLayerPtr(i) == hTx)
				return i;
		return -1;
	}
	CTextureLayer & getTextureLayer(int iNdx) const
	{ 
		CTextureLayerPtr hTx = getTextureLayerPtr(iNdx);
		return *hTx;
	}

	C3DObjectPtr createMesh();
	double getLonOffset();
	void reproject();
	void cull();
	double distToTile() const;
	bool getFacingAway() const;
	void setBoundingSphere();

	bool getHide() const
	{
		return (getTileObjectCnt() && getTileObject(0).getHide());
	}
	void setHide(bool bSet)
	{
		for (int i = 0; i < getTileObjectCnt(); i++)
			m_TileObjects[i]->setHide(bSet);
	}
	void setHideR(bool bSet)
	{
		setHide(bSet);
		for (int i = 0; i < getChildCnt(); i++)
			m_Child[i].setHideR(bSet);
	}
	bool getNeedUpdate() const { return m_NeedUpdate; }
	void setNeedUpdate()
	{
		m_NeedUpdate = true;
		for (int i = 0; i < getChildCnt(); i++)
			m_Child[i].setNeedUpdate();
	}

	void unloadMaterials()
	{
		m_TileObjects.clear();
		for (int i = 0; i < getChildCnt(); i++)
			m_Child[i].unloadMaterials();
	}

	void removeTextureLayer(CTextureLayerPtr hTx, bool bForceUpdate = false)
	{
		int iLayer = -1;
		for (int i = 0; i < getTileObjectCnt(); i++)
			if (getTextureLayerPtr(i) == hTx)
			{
				hTx->editModel().delObject(m_TileObjects[i]);
				clearTileObject(i);
				iLayer = i;
				break;
			}
		if (iLayer == 0 || bForceUpdate)
			m_NeedUpdate = true;
		if (iLayer == 0)
		{
			if (m_BmpTerrain) delete m_BmpTerrain; m_BmpTerrain = NULL;
		}
		for (int i = 0; i < getChildCnt(); i++)
			m_Child[i].removeTextureLayer(hTx);
	}

	bool getTileInfo(Vec3d pos, int &iLevel, int &iRow, int &iCol) const;
	bool cacheTileTexture(int iObj);
};

typedef CTerrainTile * CTerrainTilePtr;

class CTerrainTree
{
private:
	CTerrainTile m_Root;

public:
	CTerrainTree() {}
	~CTerrainTree()	{}

	CTerrainTile & getRoot() { return m_Root; }
	void clean() { m_Root = CTerrainTile(); }
	CTerrainTile & getTile(CTerrainTilePtr hTile) { return *hTile; }

	void create(CTerrainSet *pTerrain, CTextureSet *pTextures);

	//this will turn each leaf off or on based on their visibility and distance to camera
	void cull()
	{
		m_Root.setHideR(true);
		if (m_Root.getTerrainValid())
			m_Root.cull();
	}

	void flush();

	void unloadMaterials()	{ m_Root.unloadMaterials(); }
	void setNeedUpdate() { m_Root.setNeedUpdate(); }
	void removeTextureLayer(CTextureLayerPtr hTx, bool bForceUpdate = false) { m_Root.removeTextureLayer(hTx, bForceUpdate); }
	bool getTileInfo(Vec3d pos, int &iLevel, int &iRow, int &iCol) const
	{		return m_Root.getTileInfo(pos, iLevel, iRow, iCol); }
};

enum {PROJ_GLOBE, PROJ_GEO, PROJ_MERC, PROJ_MAX, PROJ_ELLIPSE};

// The thing that actually creates meshes based on the terrain data
//  and knows how to display them
class CTerrain {

private:
	CTerrainTree * m_Tree;
	CTerrainSet * m_TerrainSet;
	CTextureSet * m_TextureSet;

	bool	m_NeedTerrainUpdate;
	bool	m_TileNoCull;

public:

	//---[ constructor/destructor ]------------------------

	CTerrain() 
	{
		m_NeedTerrainUpdate = false;
		m_TileNoCull = false;
		m_TerrainSet = new CTerrainSet();
		m_TextureSet = new CTextureSet();
		m_Tree = new CTerrainTree();
	}
	~CTerrain()
	{
		delete m_Tree;
		delete m_TextureSet;
		delete m_TerrainSet;
	}

	CTerrainSet & getTerrainSet() const { return *m_TerrainSet; }
	CTextureSet & getTextureSet() const { return *m_TextureSet; }
	CTerrainTree & getTree() const { return *m_Tree; }

	//EXPOSED PROPERTIES

	void updateTextures() { getTree().setNeedUpdate(); }	//force call to updateTextureLayers
	bool getTileNoCull() const { return m_TileNoCull; }
	void setTileNoCull(bool b) { m_TileNoCull = b; }

	double getTileFactor() const { return getTerrainSet().getTileFactor(); }
	void setTileFactorSilent(double d) { getTerrainSet().setTileFactor(d); }
	void setTileFactor(double d)
	{ 
		if (d==getTerrainSet().getTileFactor())
			return;
		getTerrainSet().setTileFactor(d);
	}
	double getTileScale() const { return getTerrainSet().getTileScale(); }
	void setTileScale(double d) { getTerrainSet().setTileScale(d); }
	bool getShowSkirts() const { return getTerrainSet().getShowSkirts(); }
	void setShowSkirts(bool b)
	{ 
		if (b==getTerrainSet().getShowSkirts())
			return;
		getTerrainSet().setShowSkirts(b);
		updateTextures(); 
	}

	t_scale getScale() const { return getTextureSet().getScale(); }
	void setScale(t_scale scale) 
	{
		if (getTextureSet().getScale() == scale)
			return;
		getTextureSet().setScale(scale);
		setNeedTerrainUpdate();
	}
	double getShading() const { return getTextureSet().getShading(); }
	void setShading(double shading)
	{
		if (getTextureSet().getShading() == shading)
			return;
		getTextureSet().setShading(shading);  //reset normals on final tiles
	}

	int getProjection() const;
	void setProjection(int iProj);
	bool setMeshCenterGPS(Vec3d vPos) 
	{ 
		if (vPos==getTerrainSet().getMeshCenterGPS())
			return true;
		bool bRet = getTerrainSet().setMeshCenterGPS(vPos);
		if (bRet) 
			setNeedTerrainUpdate();
		return bRet;
	}
	Vec3d getMeshCenterGPS() { return getTerrainSet().getMeshCenterGPS(); }

	//BUILDING AND DRAWING TERRAIN
	void Render();
	void cullTiles() { getTree().cull(); }
	void flushTiles() { getTree().flush(); }
	void unloadMaterials()	
	{ 
		getTree().unloadMaterials();
		getTextureSet().unloadMaterials();
		updateTextures(); 
	}

	bool getNeedRedraw();
	bool getNeedUpdate() { return m_NeedTerrainUpdate; }
	void setNeedTerrainUpdate() { m_NeedTerrainUpdate = true; }

	void setFog(double factor, color clr);
	void drawLLGrid(Vec3d vCenter, double fScale, double fRes, bool bLabels, color clr);
	void drawUTMGrid(Vec3d vCenter, double fScale, double fRes, bool bLabels, color clr);
	void drawOutlines();
	void drawOutlineList(vector<Vec3d> vList, color clr);

	bool updateTerrain();

	//TERRAIN SET
	CTerrainLayer & getTerrain(int i) const { return getTerrainSet().getTerrainLayer(i); }
	int getTerrainLayerCnt() { return getTerrainSet().getTerrainLayerCnt(); }
	bool getTerrainLayerValid(int i) { return i >= 0 && i < getTerrainLayerCnt(); }

	bool addTerrain(CTerrainLayer & Terrain, int iNdx)
	{ 
		getTerrainSet().addTerrainLayer(Terrain, iNdx);
		return true;
	}
	bool editTerrain(CTerrainLayer & Terrain, int iNdx)
	{
		bool bRebuild = (Terrain.getScale() != getTerrainSet().getTerrainLayer(iNdx).getScale()
			|| Terrain.getOffset() != getTerrainSet().getTerrainLayer(iNdx).getOffset()); 
		bool bRet = getTerrainSet().editTerrainLayer(Terrain, iNdx);
		if (bRet && bRebuild) 
			setNeedTerrainUpdate();
		return bRet;
	}
	bool delTerrain(int iNdx)
	{ 
		if (!getTerrainLayerValid(iNdx))
			return false;
		bool bActive = getTerrainSet().getTerrainLayer(iNdx).getActive();
		bool bRet = getTerrainSet().delTerrainLayer(iNdx);
		if (bRet && bActive)
			setNeedTerrainUpdate();
		return bRet;
	}	
	bool setTerrainActive(int i, bool b)
	{
		bool bRet = getTerrainSet().setActive(i, b);
		if (bRet) 
			setNeedTerrainUpdate();
		return bRet;
	}
	bool getPosHeight( double dLat, double dLon, float &fDepth) { return getTerrainSet().getPosHeight(dLat, dLon, fDepth); }

	//TEXTURES SET
	CTextureLayer & getTexture(int i) const { return getTextureSet().getTextureLayer(i); }
	int getTextureLayerCnt() { return getTextureSet().getTextureLayerCnt(); }
	bool getTextureLayerValid(int i) { return i >= 0 && i < getTextureLayerCnt(); }

	bool addTexture(CTextureLayer & Texture, int iNdx) { return getTextureSet().addTextureLayer(Texture, iNdx); }
	bool editTexture(CTextureLayer & Texture, int iNdx)
	{ 
		bool bRet = getTextureSet().editTextureLayer(Texture, iNdx);
		bool bActive = getTextureSet().getTextureLayer(iNdx).getActive();
		if (bRet && bActive && getTextureSet().getTextureLayer(iNdx).getReload())
			clearTextureLayer(iNdx);
		return bRet;
	}

	bool delTexture(int iNdx)
	{
		if (!getTextureLayerValid(iNdx))
			return false;
		if (getTextureSet().getTextureLayer(iNdx).getActive())
			setTextureActive(iNdx, false);
		return getTextureSet().delTextureLayer(iNdx);
	}

	bool setTextureActive(int iNdx, bool b) 
	{
		int iOldBackground = getTextureSet().getCurrentBackground();
		bool bRet = getTextureSet().setActive(iNdx, b);
		if (!b)
			clearTextureLayer(iNdx);
		else
		{
			if (iOldBackground != getTextureSet().getCurrentBackground()) 
				clearTextureLayer(iOldBackground);
			updateTextures();
		}
		return bRet;
	}

	void clearTextureLayer(int iNdx)
	{ 
		if (!getTextureSet().getTextureLayerValid(iNdx))
			return;
		getTextureSet().getTextureLayer(iNdx).editModel().clean();
		getTree().removeTextureLayer(getTextureSet().getTextureLayerPtr(iNdx));
	}

	//MISC
	bool getTileInfo(Vec3d pos, int &iLevel, int &iRow, int &iCol) const { return getTree().getTileInfo(pos, iLevel, iRow, iCol); }
	bool saveTerrainBitmap(string pName, Vec3d minpos, Vec3d maxpos, int maxSize);

	void rebuildTextureTiles();
	void rebuildTextureTilesR(CTextureLayer &Texture, int iLevel, int iRow, int iCol);
	
};
