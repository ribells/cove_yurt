#pragma once

#include "terrain.h"
#include "object.h"
#include "line.h"
#include <sstream>

#include "data_mgr.h"

//==========[ class CData ]==================================================

enum {LINE_ALL, LINE_PREV, LINE_TRAILS, LINE_CURRENT};

enum {VIEW_POINTS, VIEW_VECTORS, VIEW_LINES, VIEW_PLANES, VIEW_ISOSURFACES, VIEW_MAX};

enum {CONVERT_NONE, CONVERT_TERRAIN, CONVERT_SCALE, CONVERT_VALUE};

enum {LOAD_IMMEDIATE, LOAD_ACTIVATION, LOAD_SCHEDULE};

enum {PLANES_LAT, PLANES_LON, PLANES_DEPTH, PLANES_CUSTOM, PLANES_MODEL};

enum {PLANES_CUSTOM_VSLICE, PLANES_CUSTOM_TSLICE, PLANES_CUSTOM_PLOT, PLANES_CUSTOM_MAX};

enum {SLICE_DEPTH, SLICE_LAT, SLICE_LON, SLICE_VSECT};

typedef struct multi_file_t
{
	string strPath;
	double dMinTime;
	double dMaxTime;
} multi_file;

#define ISO_MAX 3
#define DIM_MAX 3

class CDataLayer : public CLayer
{
private:
	CDataPtr		m_DataPtr;

	C3DModel		m_Model;
	C3DModel		m_PlotModel;
	CGradient		m_Gradient;

	CDataLayer *	m_PrevLayer;
	CDataLayer *	m_NextLayer;

	bool	m_Dirty;

	bool	m_MultiFile;
	bool	m_ShareFile;
	bool	m_UpdateFile;
	int		m_UpdateFileType;
	dtime	m_UpdateRepeatDur;
	dtime	m_UpdateRepeatTime;
	bool	m_RerunWF;

	bool	m_CycleData;
	dtime	m_CycleDataDur;

	vector<multi_file>	m_MultiFileData;
	int		m_MultiFileCur;

	int		m_CurVar;
	string	m_CurVarName;	//used to remember variable when dataset loaded

	float	m_CurVarMin;
	float	m_CurVarMax;
	int		m_ConvertDepths;
	double	m_DisplaceFactor;

	int		m_TimeFormat;

	//timeline
	dtime	m_Time;
	dtime	m_Start;
	dtime	m_Finish;
	dtime	m_SelStart;
	dtime	m_SelFinish;
	dtime	m_Lead;
	dtime	m_Trail;
	dtime	m_FrameTime;
	dtime	m_TimeStart;
	dtime	m_TimeStep;

	//filtering
	bool	m_UseDisplayStride;
	int		m_DisplayStride;
	bool	m_UseDisplayMinPos;
	bool	m_UseDisplayMaxPos;
	Vec3d	m_DisplayMinPos;
	Vec3d	m_DisplayMaxPos;

	bool	m_ViewDimActive[DIM_MAX];
	bool	m_ViewDimGrid[DIM_MAX]; //false = Plane , true = Grid
	int		m_ViewPlaneValue[DIM_MAX];
	int		m_ViewGridValue[DIM_MAX];

	//view types
	int		m_ViewType;

	//coloring
	color	m_Color;
	string	m_GradientFile;
	bool	m_GradientBins;
	bool	m_GradientLine;
	bool	m_GradientFlip;
	bool	m_GradientLog;
	int		m_Contours;
	bool	m_UseGradient;
	bool	m_GradientLegend;

	double  m_Transparency;

	bool	m_UseDisplayMinVal;
	bool	m_UseDisplayMaxVal;
	bool	m_ClampDisplayMinVal;
	bool	m_ClampDisplayMaxVal;
	float	m_DisplayMinVal;
	float	m_DisplayMaxVal;

	//points
	int		m_PrimType;
	float	m_PointSize;
	bool	m_ShowScaleSize[3];
	Vec3d	m_ScaleValue;
	bool	m_MagnifyCurrent;
	bool	m_ColorByTime;

	//lines
	float	m_LineSize;
	int		m_LineStride;
	bool	m_FadeLines;
	bool	m_AttachObj;
	string	m_AttachObjID;
	CObjectPtr m_AttachObjPtr;
	bool	m_CamAttached;
	int		m_CamType;

	//vectors
	float	m_VectorLength;
	float	m_VectorWidth;
	int		m_ShowVectorDims;
	bool	m_ShowArrows;

	//particles
	CDataPtr		m_ParticleData;
	bool			m_ParticlesDirty;
	bool			m_ParticlesRebuild;
	bool			m_ParticleForward;
	bool			m_ParticleTimeRelease;
	double			m_ParticleStep;
	int				m_ParticleValue;
	bool			m_EditAutoUpdate;

	vector<Vec3d>	m_ParticlePosList;
	vector<Vec3d>	m_ParticleUpdateList;
	vector<CObject>	m_PointEditObjects;
	CObjectTypePtr	m_PointObjectType;
	bool			m_PointEdit;

	//planes
	bool			m_PlanesShade; //shade the slices
	bool			m_PlanesWindow;
	int				m_PlanesType;
	int				m_PlanesCustomType;
	Vec3d			m_PlanesPresetPos;

	vector<Vec3d>	m_PlanesVertSliceList;
	vector<Vec3d>	m_PlanesTimePlotList;
	Vec3d			m_PlanesTimeSlicePos;
	CLineTypePtr	m_PlanesLineType;
	CLine			m_LineEditLine;
	bool			m_LineEdit;

	CDataPtr		m_PlaneData;
	bool			m_PlanesDirty;
	Vec3d			m_PlaneMin;
	Vec3d			m_PlaneMax;

	//isosurfaces
	float	m_IsoValue[ISO_MAX];
	bool	m_IsoValueActive[ISO_MAX];
	bool	m_IsoShadeFlip;

	//internal display variables
	int		m_DisplayCnt;
	int		m_DisplayCur;
	int		m_IndexCur;
	vector<int> m_DisplayNdx;

	int		m_LastPntCur;
	vector<float> m_DataPlane;	//data points
	vector<float> m_DataPlane2;	//2D vector data points
	vector<float> m_DataPlane3;	//3D vector data points

	vector<Vec3f> m_ScnPlane;	//caches locations for point/vecs/lines
	vector<Vec3f> m_ScnPlane2;	//caches end locations vectors

	Vec3f	m_LastInterpPos;
	Vec3f	m_LastInterpDir;

	int		m_ImageVar;
	string	m_ImagePathNew;
	string	m_ImagePathCur;
	bool	m_AttachVideo;
	bool	m_AttachVideoFile;
	string	m_AttachVideoPath;
	bool	m_AttachVideoWindow;

	//CMovie *m_MoviePtr;
	int		m_Vid_txid;
	int		m_Vid_h;
	int		m_Vid_w;

	void *	m_DataWinPtr;
	int		m_dw_x, m_dw_y, m_dw_w, m_dw_h;

public:
	CDataLayer();
	~CDataLayer() {	clean(); }

	void clean();

	CDataLayer(CDataLayer const &t)
	{
		*this = t;
		editGradient().clearDataPtrs();
		editModel().clearDataPtrs();
		editPlotModel().clearDataPtrs();
		clearLayerPtrs();
		setLoaded(false);
	}
	CDataLayer & operator = (const CDataLayer & t)
	{
		CLayer::operator =(t);
		setReload(getReload() || t.getShareFile()!=getShareFile()
			|| t.getMultiFile()!=getMultiFile() || t.getUpdateFile()!=getUpdateFile()); 
		m_PrevLayer = t.m_PrevLayer;
		m_NextLayer = t.m_NextLayer;
		m_MultiFileData = t.m_MultiFileData;
		m_MultiFileCur = t.m_MultiFileCur;
		m_TimeFormat = t.m_TimeFormat;

		m_ConvertDepths = t.getConvertDepths();
		m_DisplaceFactor = t.getDisplaceFactor();
		m_CurVarName = t.getCurVarName();

		m_MultiFile = t.getMultiFile();
		m_ShareFile = t.getShareFile();
		m_UpdateFile = t.getUpdateFile();
		m_UpdateFileType = t.getUpdateFileType();
		m_UpdateRepeatDur = t.getUpdateRepeatDur();
		m_UpdateRepeatTime = t.getUpdateRepeatTime();
		m_RerunWF = t.getRerunWF();
		m_CycleData = t.getCycleData();
		m_CycleDataDur = t.getCycleDataDur();

		m_Color = t.getColor();
		m_GradientBins = t.getGradientBins();
		m_GradientLine = t.getGradientLine();
		m_GradientFlip = t.getGradientFlip();
		m_GradientLegend = t.getGradientLegend();
		m_GradientLog = t.getGradientLog();
		m_Contours = t.getContours();
		m_Transparency = t.getTransparency();
		m_GradientFile = t.getGradientFile();
		m_UseDisplayMinVal = t.getUseDisplayMinVal();
		m_UseDisplayMaxVal = t.getUseDisplayMaxVal();
		m_ClampDisplayMinVal = t.getClampDisplayMinVal();
		m_ClampDisplayMaxVal = t.getClampDisplayMaxVal();
		m_DisplayMinVal = t.getDisplayMinVal();
		m_DisplayMaxVal = t.getDisplayMaxVal();
		m_UseGradient = t.getUseGradient();

		m_UseDisplayMinPos = t.getUseDisplayMinPos();
		m_DisplayMinPos = t.getDisplayMinPos();
		m_UseDisplayMaxPos = t.getUseDisplayMaxPos();
		m_DisplayMaxPos = t.getDisplayMaxPos();
		m_UseDisplayStride = t.getUseDisplayStride();
		m_DisplayStride = t.getDisplayStride();
		for (int i=0; i < DIM_MAX; i++)
		{
			m_ViewDimActive[i] = t.getViewDimActive(i);
			m_ViewDimGrid[i] = t.getViewDimGrid(i);
			m_ViewPlaneValue[i] = t.getViewPlaneValue(i);
			m_ViewGridValue[i] = t.getViewGridValue(i);
		}

		m_ViewType = t.getViewType();

		m_PrimType = t.getPrimType();
		m_PointSize = t.getPointSize();
		for (int i=0; i < 3; i++)
			m_ShowScaleSize[i] = t.getShowScaleSize(i);
		m_ScaleValue = t.getScaleValue();
		m_MagnifyCurrent = t.getMagnifyCurrent();
		m_ColorByTime = t.getColorByTime();

		m_VectorLength = t.getVectorLength();
		m_VectorWidth = t.getVectorWidth();
		m_ShowVectorDims = t.getShowVectorDims();
		m_ShowArrows = t.getShowArrows();

		m_LineSize = t.getLineSize();
		m_FadeLines = t.getFadeLines();
		m_CamAttached = t.getCamAttached();
		m_CamType = t.getCamType();
		m_AttachObj = t.getAttachObjAttached();
		m_AttachObjID = t.getAttachObjID();
		m_AttachVideo = t.getAttachVideo();
		m_AttachVideoFile = t.getAttachVideoFile();
		m_AttachVideoPath = t.getAttachVideoPath();
		m_AttachVideoWindow = t.getAttachVideoWindow();

		m_EditAutoUpdate = t.m_EditAutoUpdate;
		m_ParticlePosList = t.m_ParticlePosList;
		m_ParticleForward = t.m_ParticleForward;
		m_ParticleTimeRelease = t.m_ParticleTimeRelease;
		m_ParticleStep = t.m_ParticleStep;
		m_ParticleValue = t.m_ParticleValue;
		m_PointEdit = t.getPointEdit();

		m_PlanesShade = t.getPlanesShade();
		m_PlanesWindow = t.getPlanesWindow();
		m_PlanesType = t.getPlanesType();
		m_PlanesCustomType = t.getPlanesCustomType();
		m_PlanesPresetPos = t.getPlanesPresetPos();
		m_PlanesVertSliceList = t.getPlanesVertSliceList();
		m_PlanesTimeSlicePos = t.getPlanesTimeSlicePos();
		m_PlanesTimePlotList = t.getPlanesTimePlotList();
		m_LineEdit = t.getLineEdit();
		m_PlaneMin = t.m_PlaneMin;
		m_PlaneMax = t.m_PlaneMax;

		for (int i=0; i < ISO_MAX; i++)
		{
			m_IsoValueActive[i] = t.getIsoValueActive(i);
			m_IsoValue[i] = t.getIsoValue(i);
		}
		m_IsoShadeFlip = t.getIsoShadeFlip();

		m_Time = t.m_Time;
		m_Start = t.m_Start;
		m_Finish = t.m_Finish;
		m_TimeStart = t.m_TimeStart;
		m_TimeStep = t.m_TimeStep;

		m_SelStart = t.m_SelStart;
		m_SelFinish = t.m_SelFinish;
		m_Trail = t.m_Trail;
		m_Lead = t.m_Lead;

		m_dw_x = t.m_dw_x;
		m_dw_y = t.m_dw_y;
		m_dw_w = t.m_dw_w;
		m_dw_h = t.m_dw_h;
		m_DataWinPtr = t.m_DataWinPtr;

		setDirty();
		if (getActive())
			initActivate();

		return *this;
	}

	const CData & getData() const { return *m_DataPtr; }
	CData & editData() const { return *m_DataPtr; }
	const CDataPtr getDataPtr() const { return m_DataPtr; }
	bool getDataLoaded() const { return m_DataPtr != NULL; }

	CData & getParticleData() { return *m_ParticleData; }
	bool getParticleDataLoaded() { return m_ParticleData != NULL; }
	CData & getPlaneData() { return *m_PlaneData; }
	bool getPlaneDataLoaded() { return m_PlaneData != NULL; }

	const C3DModel & getModel() const { return m_Model; }
	C3DModel & editModel() { return m_Model; }
	const C3DModel & getPlotModel() const { return m_PlotModel; }
	C3DModel & editPlotModel() { return m_PlotModel; }

	const CGradient & getGradient() const { return m_Gradient; }
	CGradient & editGradient() { return m_Gradient; }

	//CMovie & getMovie() const { return *m_MoviePtr; }
	//bool getMovieLoaded() { return m_MoviePtr != NULL; }

	void setPrevLayer(CDataLayer *pLayer) { m_PrevLayer = pLayer; }
	CDataLayer * getPrevLayer() const { return m_PrevLayer; }
	void setNextLayer(CDataLayer *pLayer) { m_NextLayer = pLayer; }
	CDataLayer * getNextLayer() const { return m_NextLayer; }
	CDataLayer * getSharedDataLayer(string strPath)
	{
		CDataLayer *pLayer = NULL;
		for (pLayer = getPrevLayer(); pLayer; pLayer = pLayer->getPrevLayer())
			if (pLayer->getActive() && pLayer->getShareFile() && 
				lwrstr(pLayer->getLink()) == lwrstr(strPath))
				break;
		if (pLayer == NULL)
		{
			for (pLayer = getNextLayer(); pLayer; pLayer = pLayer->getNextLayer())
				if (pLayer->getActive() && pLayer->getShareFile() && 
					lwrstr(pLayer->getLink()) == lwrstr(strPath))
					break;
		}
		if (pLayer == NULL)
			return NULL;
		return pLayer;
	}
	bool isSharedData() const
	{
		if (!getShareFile() || !getDataLoaded())
			return false;
		CDataLayer *pLayer;
		for (pLayer = getPrevLayer(); pLayer; pLayer = pLayer->getPrevLayer())
			if (pLayer->getActive() && pLayer->getShareFile() && pLayer->getDataPtr()==getDataPtr())
				return true;
		for (pLayer = getNextLayer(); pLayer; pLayer = pLayer->getNextLayer())
			if (pLayer->getActive() && pLayer->getShareFile() && pLayer->getDataPtr()==getDataPtr())
				return true;
		return false;
	}

	Vec3f getScenePos(Vec3d pos) const;
	Vec3d getDataNdxPos(int ndx) const;

	bool getTimeFormat() const { return m_TimeFormat; }
	void setTimeFormat(int i) { m_TimeFormat = i; }

	void clearLayerPtrs()
	{
		m_DataPtr = NULL;
		m_AttachObjPtr = NULL;
		//m_MoviePtr = NULL;
		m_ParticleData = NULL;
		m_PlaneData = NULL;
		m_PointObjectType = NULL;
	}
	void clearLayer()
	{
		if (m_DataPtr) delete m_DataPtr;
		if (m_AttachObjPtr) delete m_AttachObjPtr;
		//if (m_MoviePtr) delete m_MoviePtr;
		if (m_ParticleData) delete m_ParticleData;
		if (m_PlaneData) delete m_PlaneData;
		if (m_PointObjectType) delete m_PointObjectType;
		clearLayerPtrs();
	}
	void initLayer()
	{
		clearLayer();
		//m_MoviePtr = new CMovie();
		m_AttachObjPtr = NULL;
		m_ParticleData = NULL;
		m_PlaneData = NULL;
		m_PointObjectType = NULL;
	}

	bool setActive(bool b);
	void initCurVar();
	void initActivate();

	dtime getTime() const { return m_Time; }
	dtime getStart() const { return m_Start; }
	void setStart(dtime t) { m_Start = t; }
	dtime getFinish() const { return m_Finish; }
	void setFinish(dtime t) { m_Finish = t; }

	dtime getTimeStart() const {return m_TimeStart;}
	void setTimeStart(dtime t) { m_TimeStart = t; }
	dtime getTimeStep() const {return m_TimeStep;}
	void setTimeStep(dtime offset) { m_TimeStep = offset; }

	dtime getWorldTimefromData(double dt) const
	{ 
		dtime start = m_TimeStart == NO_TIME ? 0 : m_TimeStart;
		return max(0.0, start + dt * m_TimeStep);
	}
	dtime getDataTimefromWorld(double t) const
	{ 
		dtime start = m_TimeStart == NO_TIME ? 0 : m_TimeStart;
		return max(0.0, (t + start) / m_TimeStep );
	}
	dtime getOffsetStart() const { return getWorldTimefromData(m_Start); }
	dtime getOffsetFinish() const { return getWorldTimefromData(m_Finish); }

	//called by ui so covert time
	void setSelStart(double t) { m_SelStart = getDataTimefromWorld(t); }
	void setSelFinish(double t) { m_SelFinish = getDataTimefromWorld(t); }
	void setLead(double t) { m_Lead = t/m_TimeStep; }
	void setTrail(double t) { m_Trail = t/m_TimeStep; }

	bool getMultiFile() const { return m_MultiFile; }
	void setMultiFile(bool b) { m_MultiFile = b; if (m_MultiFile) m_CycleData = false; } //can't mix for now
	bool getShareFile() const { return m_ShareFile; }
	void setShareFile(bool b) { m_ShareFile = b; }
	bool getUpdateFile() const { return m_UpdateFile; }
	void setUpdateFile(bool b) { m_UpdateFile = b; }
	int getUpdateFileType() const { return m_UpdateFileType; }
	void setUpdateFileType(int i) { m_UpdateFileType = i; }
	dtime getUpdateRepeatDur() const { return m_UpdateRepeatDur; }
	void setUpdateRepeatDur(dtime d) { m_UpdateRepeatDur = d; }
	dtime getUpdateRepeatTime() const { return m_UpdateRepeatTime; }
	void setUpdateRepeatTime(dtime d) { m_UpdateRepeatTime = d; }
	bool getRerunWF() const { return m_RerunWF; }
	void setRerunWF(bool b) { m_RerunWF = b; }

	bool getCycleData() const { return m_CycleData; }
	void setCycleData(bool b) { m_CycleData = b; }
	dtime getCycleDataDur() const { return m_CycleDataDur; }
	void setCycleDataDur(dtime t) { m_CycleDataDur = t; }

	void setExtents();

	void setDirty() {m_Dirty = true;}
	void setParticlesDirty() {m_ParticlesDirty = true; setDirty(); }
	void setParticlesAllDirty() {m_ParticlesRebuild = true; m_ParticlesDirty = true; setDirty(); }
	void setParticlesRebuild() { m_ParticlesRebuild = true; } 
	void setPlanesDirty() {m_PlanesDirty = true; setDirty(); }
	//bool getMovieActive() const { return m_MoviePtr != NULL; }

	void resetDimFilter();

	string getCurVarName() const { return m_CurVarName; }
	void setCurVarName(string str) { m_CurVarName = str; }

	int getCurVar() const { return m_CurVar; }
	void setCurVar(int iVar);
	float getCurVarMin() const { return m_CurVarMin; }
	float getCurVarMax() const { return m_CurVarMax; }
	int	getCurVarSize() const { return !getDataLoaded() || m_CurVar==-1 ? 0 : getData().getVarSize(m_CurVar); }
	int	getCurVarTimeSteps() const { return !getDataLoaded() || m_CurVar==-1 ? 1 : getData().getTimeSteps(); }
	int	getCurVarDepthSteps() const { return !getDataLoaded() || m_CurVar==-1 ? 1 : getData().getDepthCnt(); }
	void setCurVarMinMax();

	int getConvertDepths() const { return m_ConvertDepths; }
	void setConvertDepths(int iType) { m_ConvertDepths = iType; }
	double getDisplaceFactor() const { return m_DisplaceFactor; }
	void setDisplaceFactor(double d) { m_DisplaceFactor = d; }

	// display
	void initData(double startTime, double dCurTime);
	void RenderData(double dSpeed, int iSelectID = -1);
	void renderPlotWindow();
	color getPlotColor(int i, int icnt);

	int	getDataMax() const { return m_DisplayCnt; }

	float getGradOffset(float fval) const;
	void setAttachData(int iPntCur);

	void createSliceModel(int iSliceType, vector<float> & vSlice, int iRows, int iCols, bool bWindow, bool bNormals);
	bool createSliceNdxList(int iSliceType, int Slice, vector<float> & vSlice, int & iRows, int & iCols);

	void CreateIsoSurfaceModel(float value, bool bShadeFlip);
	void createStreamModel(int iTimeSlice);
	void createShapeModel(int iTimeSlice);

	string getInfoText(int iPnt) const;
	Vec3d getDataPos(int iPnt) const;

	void setViewType(int iType)
	{ 
		int iOldType = m_ViewType;
		m_ViewType = iType;
		if (m_ViewType==VIEW_VECTORS) 
			setShowVectorDims(m_ShowVectorDims); 
		if (m_ViewType==VIEW_VECTORS || iOldType==VIEW_VECTORS ) 
			setCurVarMinMax();
		setDirty();
	}
	int getViewType() const { return m_ViewType; }

	bool getShowPoints() const { return m_ViewType == VIEW_POINTS; }
	float getPointSize() const { return m_PointSize; }
	void setPointSize(float fSize) { m_PointSize = fSize; setDirty(); }
	int getPrimType() const { return m_PrimType; }
	void setPrimType(int i) { m_PrimType = i; setDirty(); }
	void setPrimType(string str) { m_PrimType = getDataPrimType(str); }

	bool getShowScaleSize(int i) const { return m_ShowScaleSize[i]; }
	void setShowScaleSize(int i, bool b) {  m_ShowScaleSize[i] = b; setDirty(); }
	void setShowScaleSize(bool b0, bool b1, bool b2) {  
		m_ShowScaleSize[0] = b0; 
		m_ShowScaleSize[1] = b1; 
		m_ShowScaleSize[2] = b2; 
		setDirty(); }
	Vec3d getScaleValue() const { return m_ScaleValue; }
	void setScaleValue(Vec3d size) { m_ScaleValue = size; setDirty(); }
	bool getMagnifyCurrent() const { return m_MagnifyCurrent; }
	void setMagnifyCurrent(bool b) {  m_MagnifyCurrent = b; setDirty(); }
	bool getColorByTime() const { return m_ColorByTime; }
	void setColorByTime(bool b) {  m_ColorByTime = b; setDirty(); }

	bool getShowVectors() const { return m_ViewType == VIEW_VECTORS; }
	float getVectorLength() const { return m_VectorLength; }
	void setVectorLength(float fSize) { m_VectorLength = fSize; setDirty(); }
	float getVectorWidth() const { return m_VectorWidth; }
	void setVectorWidth(float fSize) { m_VectorWidth = fSize; setDirty(); }
	int getShowVectorDims() const { return m_ShowVectorDims; }
	void setShowVectorDims(int i);
	bool getShowArrows() const { return m_ShowArrows; }
	void setShowArrows(bool b) { m_ShowArrows = b; setDirty(); }

	bool getShowLines() const { return m_ViewType == VIEW_LINES; }
	float getLineSize() const { return m_LineSize; }
	void setLineSize(float fSize) { m_LineSize = fSize; setDirty(); }
	bool getFadeLines() const { return m_FadeLines; }
	void setFadeLines(bool b) {  m_FadeLines = b; setDirty(); }
	int getLineStride() const { return m_LineStride; }
	void setLineStride(int i) { m_LineStride = i; setDirty(); }

	bool getEditAutoUpdate() const { return m_EditAutoUpdate; }
	void setEditAutoUpdate(bool b) { m_EditAutoUpdate = b; }
	bool getShowParticles() const { return m_ViewType == VIEW_LINES && getDataLoaded()
		&& (getData().getDataFormat()==DFORMAT_2DSURFACE || getData().getDataFormat()==DFORMAT_3DMODEL);
	}
	bool getParticleForward() const { return m_ParticleForward; }
	void setParticleForward(bool b) { m_ParticleForward = b; }
	bool getParticleTimeRelease() const { return m_ParticleTimeRelease; }
	void setParticleTimeRelease(bool b) { m_ParticleTimeRelease = b; }
	double getParticleStep() const { return m_ParticleStep; }
	void setParticleStep(double t) { m_ParticleStep = t; }
	int getParticleValue() const { return m_ParticleValue; }
	void setParticleValue(int i) { m_ParticleValue = i; }

	bool getShowPlanes() const { return m_ViewType == VIEW_PLANES; }
	bool getShowPresetPlane() const { return getShowPlanes() && 
		(m_PlanesType == PLANES_DEPTH || m_PlanesType >= PLANES_LON || m_PlanesType >= PLANES_LAT); }
	bool getShowVerticalPlane() const { return getShowPlanes() && 
		(m_PlanesType == PLANES_CUSTOM || m_PlanesType >= PLANES_LON || m_PlanesType >= PLANES_LAT); }
	bool getShowCustomPlane() const { return getShowPlanes() && m_PlanesType < PLANES_MODEL; }
	bool getShowModelPlanes() const { return getShowPlanes() && m_PlanesType == PLANES_MODEL; }
	bool getShowVertSect() const { return getShowPlanes() && getData().getDataFormat() == DFORMAT_PATH; }
	bool getShowVertSlice() const { return getShowCustomPlane() && m_PlanesCustomType == PLANES_CUSTOM_VSLICE; }
	bool getShowTimeSlice() const { return getShowCustomPlane() && m_PlanesCustomType == PLANES_CUSTOM_TSLICE; }
	bool getShowTimePlot() const { return getShowCustomPlane() && m_PlanesCustomType == PLANES_CUSTOM_PLOT; }

	int getPlanesType() const { return m_PlanesType; }
	void setPlanesType(int i) { m_PlanesType = i;  setPlanesDirty(); }
	int getPlanesCustomType() const { return m_PlanesCustomType; }
	void setPlanesCustomType(int i) { m_PlanesCustomType = i;  setPlanesDirty(); }
	double getPlanesPresetPos(int i) const { return m_PlanesPresetPos[i]; }
	void setPlanesPresetPos(int i, double d) { m_PlanesPresetPos[i] = d;  setPlanesDirty(); }
	Vec3d getPlanesPresetPos() const { return m_PlanesPresetPos; }
	void setPlanesPresetPos(Vec3d pos) { m_PlanesPresetPos = pos;  setPlanesDirty(); }
	bool getPlanesShade() const { return m_PlanesShade; }
	void setPlanesShade(bool b) { m_PlanesShade = b;  setPlanesDirty(); }
	bool getPlanesWindow() const { return m_PlanesWindow; }
	void setPlanesWindow(bool b) { m_PlanesWindow = b;  setDirty(); }

	Vec3d getPlaneMin() const { return m_PlaneMin; }
	void setPlaneMin(Vec3d v) { m_PlaneMin = v; }
	Vec3d getPlaneMax() const { return m_PlaneMax; }
	void setPlaneMax(Vec3d v) { m_PlaneMax = v; }

	bool getShowIsoSurfaces() const { return m_ViewType == VIEW_ISOSURFACES; }
	float getIsoValue(int i) const { return m_IsoValue[i]; }
	void setIsoValue(int i, float fSize) { m_IsoValue[i] = fSize; setDirty(); }
	bool getIsoValueActive(int i) const { return m_IsoValueActive[i]; }
	void setIsoValueActive(int i, bool b) { m_IsoValueActive[i] = b; setDirty(); }
	bool getIsoShadeFlip() const { return m_IsoShadeFlip; }
	void setIsoShadeFlip(bool b) { m_IsoShadeFlip = b;  setPlanesDirty(); }

	color getColor() const { return m_Color; }
	void setColor(color clr) { m_Color = clr;}
	void setGradientTex();
	void setPointColor(int iPnt, int iSelectID, double dTrans = 1.0);

	void createGradient()
	{
		editGradient().clean();
		editGradient().addFile(m_GradientFile, -32000, 32000);
		setGradientTex();
	}
	string getGradientFile() const { return m_GradientFile; }
	void setGradientFile(string str) { m_GradientFile = str; }
	bool getUseGradient() const { return m_UseGradient; }
	void setUseGradient(bool b) { m_UseGradient = b; }
	bool getGradientBins() const { return m_GradientBins; }
	void setGradientBins(bool b) { m_GradientBins = b; }
	bool getGradientLine() const { return m_GradientLine; }
	void setGradientLine(bool b) { m_GradientLine = b; }
	bool getGradientFlip() const { return m_GradientFlip; }
	void setGradientFlip(bool b) { m_GradientFlip = b; }
	bool getGradientLegend() const { return m_GradientLegend; }
	void setGradientLegend(bool b) { m_GradientLegend = b; }
	bool getGradientLog() const { return m_GradientLog; }
	void setGradientLog(bool b) { m_GradientLog = b; }
	int getContours() const { return m_Contours; }
	void setContours(int i) { m_Contours = i;}
	double getTransparency() const { return m_Transparency; }
	void setTransparency(double d)
	{
		m_Transparency = d;
		editModel().setTransparency(d);
	}

	bool getUseDisplayMinVal() const { return m_UseDisplayMinVal; }
	void setUseDisplayMinVal(bool b) { m_UseDisplayMinVal = b; }
	bool getUseDisplayMaxVal() const { return m_UseDisplayMaxVal; }
	void setUseDisplayMaxVal(bool b) { m_UseDisplayMaxVal = b;}
	bool getClampDisplayMinVal() const { return m_ClampDisplayMinVal; }
	void setClampDisplayMinVal(bool b) { m_ClampDisplayMinVal = b; }
	bool getClampDisplayMaxVal() const { return m_ClampDisplayMaxVal; }
	void setClampDisplayMaxVal(bool b) { m_ClampDisplayMaxVal = b;}
	float getDisplayMinVal() const { return m_DisplayMinVal; }
	void setDisplayMinVal(float f) { m_DisplayMinVal = f;}
	float getDisplayMaxVal() const { return m_DisplayMaxVal; }
	void setDisplayMaxVal(float f) { m_DisplayMaxVal = f;}

	bool getUseDisplayStride() const { return m_UseDisplayStride; }
	void setUseDisplayStride(bool b) { m_UseDisplayStride = b;  setDirty();}
	int getDisplayStride() const { return m_DisplayStride; }
	void setDisplayStride(int i) { m_DisplayStride = i;  setDirty();}
	bool getUseDisplayMinPos() const { return m_UseDisplayMinPos; }
	void setUseDisplayMinPos(bool b) { m_UseDisplayMinPos = b;   setDirty();}
	bool getUseDisplayMaxPos() const { return m_UseDisplayMaxPos; }
	void setUseDisplayMaxPos(bool b) { m_UseDisplayMaxPos = b;  setDirty();}
	Vec3d getDisplayMinPos() const { return m_DisplayMinPos; }
	void setDisplayMinPos(Vec3d f) { m_DisplayMinPos = f;  setDirty();}
	Vec3d getDisplayMaxPos() const { return m_DisplayMaxPos; }
	void setDisplayMaxPos(Vec3d f) { m_DisplayMaxPos = f;  setDirty();}
	bool getCropped(Vec3d pos) const
	{
		if (m_UseDisplayMinPos && m_DisplayMinPos != vl_0 && 
			(pos[0] < m_DisplayMinPos[0] || pos[1] < m_DisplayMinPos[1] || pos[2] < m_DisplayMinPos[2]))
			return true;
		if (m_UseDisplayMaxPos && m_DisplayMaxPos != vl_0 && 
			(pos[0] > m_DisplayMaxPos[0] || pos[1] > m_DisplayMaxPos[1] || pos[2] > m_DisplayMaxPos[2]))
			return true;
		return false;
	}
	bool getStrideFilter(int ndx, int dim1, int dim2) const
	{
		if (!m_UseDisplayStride)
			return false;
		ndx %= dim1*dim2;
		if (((ndx/dim1)%m_DisplayStride) != 0)
			return true;
		if (((ndx%dim1)%m_DisplayStride) != 0)
			return true;
		return false;
	}

	bool getViewDimActive(int i) const { return m_ViewDimActive[i]; }
	void setViewDimActive(int i, bool b) { m_ViewDimActive[i] = b;  setDirty(); }
	bool getViewDimGrid(int i) const { return m_ViewDimGrid[i]; }
	void setViewDimGrid(int i, bool b) { m_ViewDimGrid[i] = b;  setDirty(); }
	int getViewPlaneValue(int i) const { return m_ViewPlaneValue[i]; }
	void setViewPlaneValue(int i, int val) { m_ViewPlaneValue[i]= val;  setDirty(); }
	int getViewGridValue(int i) const { return m_ViewGridValue[i]; }
	void setViewGridValue(int i, int val) { m_ViewGridValue[i]= val;  setDirty(); }
	int getViewDimMax(int i) const
	{ 
		vector<int> vDims = getData().getVarDims(m_CurVar);
		i = max(0, (int)(vDims.size()-(3-i)));
		return getData().getDimSize(vDims[i]);
	}

	void delDataWin();
	void * getDataWin();
	void getDataWinPos(int &x, int &y, int &w, int &h) { x = m_dw_x; y = m_dw_y; w = m_dw_w; h = m_dw_h; }
	void setDataWinPos(int x, int y, int w, int h) { m_dw_x = x; m_dw_y = y; m_dw_w = w; m_dw_h = h; }

	bool getShowVideo() const;
	void getVideoSize(int &w, int &h) const;
	void drawVideo();
	void attachVideo();
	bool getAttachVideo() const { return m_AttachVideo; }
	void setAttachVideo(bool b) { m_AttachVideo = b; attachVideo(); }
	bool getAttachVideoWindow() const { return m_AttachVideoWindow; }
	void setAttachVideoWindow(bool b) { m_AttachVideoWindow = b; }
	bool getAttachVideoFile() const { return m_AttachVideoFile; }
	void setAttachVideoFile(bool b) { m_AttachVideoFile = b; attachVideo(); }
	string getAttachVideoPath() const { return m_AttachVideoPath; }
	void setAttachVideoPath(string s) { m_AttachVideoPath = s; attachVideo(); }

	Vec3f getLastInterpPos() const { return m_LastInterpPos; }
	Vec3f getLastInterpDir() const { return m_LastInterpDir; }

	int getCamAttached() const { return m_CamAttached; }
	void setCamAttached(int i) { m_CamAttached = i; }
	int getCamType() const { return m_CamType; }
	void setCamType(int i) { m_CamType = i; }

	bool getAttachObjAttached() const { return m_AttachObj; }
	void setAttachObjAttached(bool b) { m_AttachObj = b; }
	CObject & getAttachObj() const { return *m_AttachObjPtr; }
	string getAttachObjID() const { return m_AttachObjID; }
	void setAttachObjID(string s)
	{ 
		m_AttachObjID = s;
		setAttachObj(NULL);
	}
	const CObjectPtr getAttachObjPtr() const { return m_AttachObjPtr; }
	void setAttachObj(const CObjectPtr pObj) { if (m_AttachObjPtr) delete m_AttachObjPtr; m_AttachObjPtr = pObj; }

	vector<Vec3d> getPlanesVertSliceList() const { return m_PlanesVertSliceList; }
	void setPlanesVertSliceList(vector<Vec3d> vpos) { m_PlanesVertSliceList = vpos;  setPlanesDirty(); }
	vector<Vec3d> getPlanesTimePlotList() const { return m_PlanesTimePlotList; }
	void setPlanesTimePlotList(vector<Vec3d> vpos) { m_PlanesTimePlotList = vpos;  setPlanesDirty(); }
	Vec3d getPlanesTimeSlicePos() const { return m_PlanesTimeSlicePos; }
	void setPlanesTimeSlicePos(Vec3d pos) { m_PlanesTimeSlicePos = pos;  setPlanesDirty(); }

	bool getLineEdit() const { return m_LineEdit; }
	void setLineEdit(bool b);
	void initLineEdit();
	void updateLineControlList();
	void setLineControlNdx(int ndx);
	void updateLineControlPos(Vec3d offset);
	CLine & getLineEditLine() { return m_LineEditLine; }

	void setParticlePosList(vector<Vec3d> vpos) { m_ParticlePosList = vpos;  setParticlesDirty(); }
	vector<Vec3d> getParticlePosList() { return m_ParticlePosList; }
	void updateParticleValues()
	{
		if (getParticleDataLoaded())
			getParticleData().setParticleDataValues(m_ParticleValue, m_ParticlePosList.size());
		setDirty();
	}
	bool getPointEdit() const { return m_PointEdit; }
	void setPointEdit(bool b);
	void addPointEditObject(Vec3d pos);
	void initPointEdit();
	void updatePointList();
	void clearPoints();
	int  getSelPointCnt();
	void selPoints();
	void selPoint(int ndx, bool bMulti);
	void selPoints(int xRBMin, int xRBMax, int yRBMin, int yRBMax);
	void addPoints(Vec3d pos, double dist, int cnt, int style);
	void updatePointPos(Vec3d offset);
	void setPointDepth(float depth);
	string getInfoTextPoints();

private:
	bool updateVideoTexture();
	void clearVideoTexture();
	void closeVideo();
	bool openVideo(string FileName);
};
