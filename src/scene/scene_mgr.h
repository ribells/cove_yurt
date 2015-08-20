#pragma once

#include <vector>
#include <string>
using namespace std;

#include <assert.h>

#include "vl/VLf.h"
#include "vl/VLd.h"
#include "const.h"

#include "movie.h"
#include "xmlParser.h"

#define GL_CLAMP_TO_EDGE	0x812F

namespace C3DSceneMgr
{
	static bool	m_Wireframe = false;
	static bool	m_Lighting = true;
	static bool	m_TextureCompression = true;

	enum { TX_CLAMP_NONE, TX_CLAMP_EDGE, TX_CLAMP_BORDER };

	bool initSceneManager();
	string getOpenGLVersion();

	int createTexture(string strFileName, int &width, int &height, int iTxMode = 0);
	int createTexture(const unsigned char* pData, int sizeX, int sizeY, int iTypeIn, int iTypeOut, int iTxMode);
	int createUITexture(string strFileName, int &width, int &height, int &txWidth);
	int createVideoTexture(CMovie * pMovie);
	bool updateVideoTexture(CMovie * pMovie, double movie_tm);
	void deleteTexture(int iTex);

	void createNewTileTexture(string strFileName);
	bool getDDSTileSupport();
	void setDDSTileSupportOff();

	int getPrimType(string strType);
	string getPrimType(int i);
	int getDataPrimType(string strType);
	string getDataPrimType(int i);
	void drawPrimType(int i);
	void drawIcon(double scale, int iTex);
	void drawArrow(Vec3f pos, Vec3f vec, float fRad);
	void clearQuadrics();
};

using namespace C3DSceneMgr;

// This is is used for indexing into the vertex and texture coordinate arrays.
class CFace
{
private:
	int		m_VertIndex[3];		// indicies for the verts that make up this triangle
	int		m_CoordIndex[3];	// indicies for the tex coords to texture this face
	int		m_NormalIndex[3];	// indicies for the vertex normals for this face
public:
	CFace() { setFace(0, 0, 0); }
	CFace(int v0, int v1, int v2) { setFace(v0, v1, v2); }
	CFace(int *pVertIndex, int *pCoordIndex, int *pVNormIndex) 
	{
		setFace(pVertIndex, pCoordIndex, pVNormIndex);
	}
	void setFace(int v0, int v1, int v2) 
	{
		m_VertIndex[0] = m_CoordIndex[0] = m_NormalIndex[0] = v0;
		m_VertIndex[1] = m_CoordIndex[1] = m_NormalIndex[1] = v1;
		m_VertIndex[2] = m_CoordIndex[2] = m_NormalIndex[2] = v2;
	}
	void setFace(int *pVertIndex, int *pCoordIndex, int *pVNormIndex) 
	{
		for (int i = 0; i < 3; i++)
		{
			m_VertIndex[i] = pVertIndex[i];
			m_CoordIndex[i] = pCoordIndex[i];
			m_NormalIndex[i] = pVNormIndex[i];
		}
	}
	void offsetIndices(int vertexOffset, int textureOffset, int vnormalOffset)
	{
		for (int i = 0; i < 3; i++)
		{
			m_VertIndex[i] += vertexOffset;
			m_CoordIndex[i] += textureOffset;
			m_NormalIndex[i] += vnormalOffset;
		}
	}
	int	getVertIndex(int i) const { return m_VertIndex[i]; }
	void setVertIndex(int i, int ndx) { m_VertIndex[i] = ndx; }
	int	getCoordIndex(int i) const { return m_CoordIndex[i]; }
	void setCoordIndex(int i, int ndx) { m_CoordIndex[i] = ndx; }
	int	getNormalIndex(int i) const { return m_NormalIndex[i]; }
	void setNormalIndex(int i, int ndx) { m_NormalIndex[i] = ndx; }

};


// This holds the information for a material.  It may be a texture map or a color.
class CMaterial
{
private:
	string	m_Name;	
	string	m_Link;
	color	m_Color;
	int		m_TextureId;
	int		m_Width;
	int		m_Height;		//if height == 1 then this is a 1D texture

	bool	m_Shared;
	bool	m_Dirty;
	bool	m_MipMap;

public:
	CMaterial() : m_TextureId(-1), m_Color(0), m_Dirty(true), m_MipMap(true) {}
	~CMaterial() 
	{
		if (!m_Shared && m_TextureId != -1)
			deleteTexture(m_TextureId);
	}

	string getName() const { return m_Name; }
	void setName(string str) { m_Name = str; }
	string getLink() const { return m_Link; }
	void setLink(string str) { m_Link = str; }
	color getColor() const { return m_Color; }
	void setColor(color clr) { m_Color = clr;}

	int getTextureId() const { return m_TextureId; }
	void setTextureId(int i) { m_TextureId = i; }
	int getWidth() const { return m_Width; }
	void setWidth(int i) { m_Width = i; }
	int getHeight() const { return m_Height; }
	void setHeight(int i) { m_Height = i; }

	bool getShared() const { return m_Shared; }
	void setShared(bool b) { m_Shared = b; }
	bool getDirty() const { return m_Dirty; }
	void setDirty(bool b) { m_Dirty = b; }
	bool getMipMap() const { return m_MipMap; }
	void setMipMap(bool b) { m_MipMap = b; }
} ;

typedef CMaterial * CMaterialPtr;

class t_scale
{
public:
	float	land;
	float	sea;
	bool operator == (t_scale &s) const { return land == s.land && sea == s.sea; }
	t_scale() { land = 1.0; sea = 1.0; }
};


// This holds all the information for our model/scene. 
class C3DObject 
{
private:
	string	m_Name;				// The name of the object
	vector<Vec3f>	m_vVerts;			// The object's vertices
	vector<Vec3f>	m_vNormals;		// The object's vertex normals
	vector<Vec3f>	m_vTexVerts;		// The texture's UV coordinates
	vector<color>	m_vClrVerts;		// The color value for the vert

	vector<CFace>	m_vFaces;			// The faces information of the object
	CMaterialPtr	m_pMaterial;	// The texture to use

	bool	m_TriangleStrip;
	bool	m_Line;
	bool	m_Dirty;
	bool	m_Lighted;
	bool	m_Reprojected;
	bool	m_Hide;
	int		m_Layering;
	float	m_Offset;

	double	m_Transparency;
	bool	m_CheckNoData;
	float	m_NoDataValue;
	Vec3d	m_PlaneDir;

	double	m_XTxMin, m_XTxMax;
	double	m_YTxMin, m_YTxMax;

	void	*m_pGLArray;
	int		*m_pGLIndices;
	int		m_BufferIDArray;
	int		m_BufferIDIndices;

public:
	C3DObject() : m_pGLArray(NULL), m_pGLIndices(NULL), m_BufferIDArray(-1), m_BufferIDIndices(-1),
		m_pMaterial(NULL), m_Dirty(true), m_TriangleStrip(false), m_Line(false), m_Reprojected(false), m_Hide(false),
		m_Name(""), m_Lighted(true), m_CheckNoData(false), m_NoDataValue(NODATA), m_Layering(0), m_Offset(0)
	{
		m_XTxMax = 1.0; m_XTxMin = 0.0;
		m_YTxMax = 1.0; m_YTxMin = 0.0;
		m_Transparency = 1.0;
		m_PlaneDir = vl_0;
	}
	~C3DObject();

	C3DObject(C3DObject const &t)
	{
		*this = t;
		m_Dirty = true;
		m_pMaterial = NULL;
		m_pGLArray = NULL;
		m_pGLIndices = NULL;
		m_BufferIDArray = m_BufferIDIndices = -1;
	}

	string getName() const { return m_Name; }
	void setName(string str) { m_Name = str; }
	bool getDirty() const { return m_Dirty; }
	void setDirty(bool b) { m_Dirty = b; }

	bool getHasNormals() const { return getNumNormals() > 0; }
	bool getHasTextureVerts() const { return getNumTexVerts() > 0; }
	bool getHasColorVerts() const { return (m_CheckNoData || getNumClrVerts() > 0); }

	bool getTriangleStrip() const { return m_TriangleStrip; }
	void setTriangleStrip(bool b) { m_TriangleStrip = b; }
	bool getLine() const { return m_Line; }
	void setLine(bool b) { m_Line = b; }

	bool getLighted() const { return m_Lighted; }
	void setLighted(bool b) { m_Lighted = b; }
	bool getReprojected() const { return m_Reprojected; }
	void setReprojected(bool b) { m_Reprojected = b; }
	bool getHide() const { return m_Hide; }
	void setHide(bool b) { m_Hide = b; }
	double getTransparency() const { return m_Transparency; }
	void setTransparency(double d) { m_Transparency = d; m_Dirty = true; }

	void setNoData(float d) { m_NoDataValue = d; }

	void setLayering(int i) { m_Layering = i; }
	void setOffset(float off) { m_Offset = off; }

	bool getHasMaterial() const { return m_pMaterial != 0; }
	CMaterial &	getMaterial() const { return *m_pMaterial; }
	const CMaterialPtr getMaterialPtr() const { return m_pMaterial; }
	void setMaterial(const CMaterialPtr pMat) { m_pMaterial = pMat; }

	void setCheckNoData(bool b) { m_CheckNoData = b; }
	double getXTxMin() const { return m_XTxMin; }
	void setXTxMin(double d) { m_XTxMin = d; }
	double getXTxMax() const { return m_XTxMax; }
	void setXTxMax(double d) { m_XTxMax = d; }
	double getYTxMin() const { return m_YTxMin; }
	void setYTxMin(double d) { m_YTxMin = d; }
	double getYTxMax() const { return m_YTxMax; }
	void setYTxMax(double d) { m_YTxMax = d; }

	void setPlaneDir(Vec3d v) { m_PlaneDir = v; }
	Vec3d getPlaneDir() const { return m_PlaneDir; }

	int	getNumVerts() const { return (int)m_vVerts.size(); }
	int	getNumNormals() const { return (int)m_vNormals.size(); }
	int	getNumTexVerts() const { return (int)m_vTexVerts.size(); }
	int	getNumClrVerts() const { return (int)m_vClrVerts.size(); }
	int	getNumFaces() const { return (int)m_vFaces.size(); }

	const vector<Vec3f> & getVerts() const { return m_vVerts; }
	vector<Vec3f> & editVerts() const { return const_cast<vector<Vec3f> &>(m_vVerts); }
	const vector<Vec3f> & getNormals() const { return m_vNormals; }
	vector<Vec3f> & editNormals() const { return const_cast<vector<Vec3f> &>(m_vNormals); }
	const vector<Vec3f> & getTexVerts() const { return m_vTexVerts; }
	vector<Vec3f> & editTexVerts() const { return const_cast<vector<Vec3f> &>(m_vTexVerts); }
	const vector<color> & getClrVerts() const { return m_vClrVerts; }
	vector<color> & editClrVerts() const { return const_cast<vector<color> &>(m_vClrVerts); }
	const vector<CFace> & getFaces() const { return m_vFaces; }
	vector<CFace> & editFaces() const { return const_cast<vector<CFace> &>(m_vFaces); }
	const CFace & getFace(int i) const { return m_vFaces[i]; }
	CFace & editFace(int i) const { return const_cast<CFace &>(m_vFaces[i]); }
	void addFace(int v0, int v1, int v2);

	void alignObjArrays();
	void unloadGLArrays();
	void createGLArray(bool bTexture, const t_scale scale, float shading);

	int getBufferIDArray() const { return m_BufferIDArray; }
	int getBufferIDIndices() const { return m_BufferIDIndices; }
	const void * getGLArray() const { return m_pGLArray; }
	const int * getGLIndices() const { return m_pGLIndices; }

	bool initArrays(int numVert, int numNorm, int numTexVert, int numClrVert, int numFaces);
	void FillInObjectInfo( vector<Vec3f> & pVerts, vector<Vec3f> & pNormals, vector<Vec3f> & pTexVerts, 
							vector<color> & pClrVerts, vector<CFace> & pFaces);
	void ComputeVertexNormals();
};

typedef C3DObject * C3DObjectPtr;

// This holds our model information.
class C3DModel 
{
private:
	string	m_ModelName;

	vector<CMaterialPtr> Materials;	// The list of material information (Textures and colors)
	vector<C3DObjectPtr> Objects;	// The object list for our model

	bool	m_ReverseOrder;
	bool	m_Hide;
	bool	m_Selected;
	int		m_SelectID;

	t_scale	m_Scale;
	float	m_Shading;

	Vec3d	trans;			// The model translation
	Vec3d	rot;			// The model rotation in x y z order
	Vec3d	scale;			// The model scale


	// The variables below are only used for file loading 
	string	m_Filepath;
	string	m_matName;			// The name of material for the currently loading object
	string	m_objName;			// The name of material for the currently loading object

	vector<Vec3f>	m_vVerts;
	vector<Vec3f>	m_vTexVerts;
	vector<color>	m_vClrVerts;
	vector<Vec3f>	m_vNormals;
	vector<CFace>	m_vFaces;

	//used when loading obj files
	int		m_vertexOffset;
	int		m_textureOffset;
	int		m_vnormalOffset;

public:
	C3DModel() : m_vertexOffset(0), m_textureOffset(0), m_vnormalOffset(0), m_ReverseOrder(false),
		m_Hide(false), m_Selected(false), m_SelectID(-1), m_Shading(1.0)
	{
		trans = vl_0;
		rot = vl_0;
		scale = vl_1;
		m_Scale.land = m_Scale.sea = 1.0;
	}
	~C3DModel() { clean(); }

	void clean()
	{
		while (getObjectCnt())
			delObject(0);
		while (getMaterialCnt())
			delMaterial(0);
	}

	void Render() const;

	bool Import(string strFileName);

	int getMaterialCnt() const { return Materials.size(); }
	bool getMaterialValid(int i) const { return i >= 0 && i < getMaterialCnt(); }
	const CMaterial & getMaterial(int i) const { return *Materials[i]; }
	CMaterial & editMaterial(int i) const { return *Materials[i]; }
	const CMaterialPtr getMaterialPtr(int i) const { return getMaterialValid(i) ? Materials[i] : NULL; }
	void addMaterial(const CMaterialPtr pMat) { Materials.push_back(pMat); }

	int getObjectCnt() const { return Objects.size(); }
	bool getObjectValid(int i) const { return i >= 0 && i < getObjectCnt(); }
	const C3DObject & getObject(int i) const { return *Objects[i]; }
	C3DObject & editObject(int i) const { return *Objects[i]; }
	const C3DObjectPtr getObjectPtr(int i) const { return getObjectValid(i) ? Objects[i] : NULL; }

	int getObjectIndex(const C3DObjectPtr pObj) const
	{ 
		for (int i = 0; i < getObjectCnt(); i++)
			if (pObj == Objects[i])
				return i;
		return -1;
	}
	void addObject(C3DObjectPtr pObj) { Objects.push_back(pObj); }
	void delObject(int i)
	{
		if (Objects[i]->getMaterialPtr() && !Objects[i]->getMaterial().getShared())
			delMaterial(Objects[i]->getMaterialPtr());
		delete Objects[i];
		Objects.erase(Objects.begin()+i);
	}
	void delObject(C3DObjectPtr pObj)
	{
		int ndx = getObjectIndex(pObj);
		if (ndx != -1)
			delObject(ndx);
	}
	bool getDirty(int i) const { return (getObjectValid(i)) ? getObject(i).getDirty() : false; }
	void setDirty(int i, bool b) { if (getObjectValid(i)) editObject(i).setDirty(b); }
	void setDirty(bool b) {for (int i = 0; i < getObjectCnt(); i++) setDirty(i, b); }

	int addMaterial(string strName, string strFile, color clr, int iTxMode);
	int addMaterial(string strName, const unsigned char* pData, int sizeX, int sizeY, color clr, int iTxMode);
	int addMaterial(string strName, int txID, int sizeX, int sizeY, color clr, bool bMipMap);

	void unloadMaterials(bool bClearMaterial = false);
	void reloadMaterials();

	void clearDataPtrs()
	{
		Objects.clear();
		Materials.clear();
	}

	bool getHide() const { return m_Hide; }
	void setHide(bool b) { m_Hide = b; }
	int  getSelected() const { return m_Selected; }
	void setSelected(bool b) { m_Selected = b; }
	int  getSelectID() const { return m_SelectID; }
	void setSelectID(int i) { m_SelectID = i; }

	string getModelName() { return m_ModelName; }
	string getFilepath() const { return m_Filepath; }

	bool getReverseOrder() const { return m_ReverseOrder; }
	void setReverseOrder(bool b) { m_ReverseOrder = b; }

	void getTransform(Vec3d &t, Vec3d &r, Vec3d &s) const	{ t = trans; r = rot; s = scale; }
	void setTransform(Vec3d t, Vec3d r, Vec3d s)	{ trans = t; rot = r; scale = s; }

	void setTransparency(double d) 
	{
		for (int i = 0; i < getObjectCnt(); i++) 
			editObject(i).setTransparency(d);
	}

	t_scale getScale() const { return m_Scale; }
	void setScale(t_scale s)
	{
		if (s == m_Scale)
			return;
		m_Scale = s;
		setDirty(true);
	}

	float getShading() const { return m_Shading; }
	void setShading(float f)
	{
		f = max(f, 1.0f);
		if (f == m_Shading)
			return;
		m_Shading = f;
		setDirty(true);
	}

	// This is called when we are finished loading in the face information
	void FillInObjectInfo(bool bOffset, bool bLine, bool bClearData);
	bool createNewObject(int numVert, int numNorm, int numTexVert, int numClrVert, int numFaces);

	void resetOffsets() { 	m_vertexOffset = m_textureOffset = m_vnormalOffset = 0; }
	void getLoadVecPtrs(vector<Vec3f> * &pVerts, vector<Vec3f> * &pTex, vector<color> * &pClr, vector<Vec3f> * &pVNrm, vector<CFace> * &pFaces)
	{
		pVerts = &m_vVerts;
		pTex = &m_vTexVerts;
		pClr = &m_vClrVerts;
		pVNrm = &m_vNormals;
		pFaces = &m_vFaces;
	}

	void sortPlaneObjects(Vec3d cameraDir);

private:
	void RenderObject(int iObj) const;
	void RenderNormals(int iObj) const;  //!!! todo - find in old code

	bool ReadOBJFile(string strFilePath);
	void ReadOBJVertexInfo(FILE * filePointer);
	void ReadOBJFaceInfo(FILE * filePointer);
	bool ReadMTLFile(string strFileName);

	bool CreateDAEMaterials(XMLNode &xn0, string strPath);
	bool CreateDAEObjects(XMLNode &xn0, Mat4f tx, string strID, vector<string> &strSyms, vector<string> &strTargets);
	bool RecurseDAEFileNodes(XMLNode &xn0, XMLNode &xml_in, string strID, Mat4f tx);
	bool ReadDAEFile(string strFileName);
	bool ReadKMZFile(string strFileName);
	bool OpenKMZFile(string strFileName, string &strTempPath);

	void delMaterial(CMaterialPtr pMat)
	{
		for (int i = 0; i < getMaterialCnt(); i++)
			if (Materials[i] == pMat || pMat == NULL)
			{
				delete Materials[i];
				Materials.erase(Materials.begin() + i);
				return;
			}
	}
};

typedef C3DModel * C3DModelPtr;


class C3DScene
{
public:
	vector<C3DModelPtr> Models;			// The model list to display, shared ptrs

	C3DScene() {}
	~C3DScene() {}  //Models constructed and deconstructed elsewhere

	int getModelCnt() const { return Models.size(); }
	bool getModelValid(int i) const { return i >= 0 && i < getModelCnt(); }
	const C3DModel & getModel(int i) const { return *Models[i]; }

	void addModel(C3DModelPtr pMod) { Models.push_back(pMod); }
	void delModel(int i) {  Models.erase(Models.begin()+i);	}
	void clearModels() {  Models.clear();	}
	int getModelIndex(C3DModelPtr pMod) const
	{ 
		for (int i = 0; i < getModelCnt(); i++)
			if (pMod == Models[i])
				return i;
		return -1;
	}
	void delModel(C3DModelPtr pMod)
	{
		int ndx = getModelIndex(pMod);
		if (ndx != -1)
			delModel(ndx);
	}

	void Render(bool bSelect = false) const
	{
		for (int i = 0; i < getModelCnt(); i++)
		{
			if (bSelect) 
				Models[i]->setSelectID(i);
			Models[i]->Render();
			Models[i]->setSelectID(-1);
		}
	}
};
