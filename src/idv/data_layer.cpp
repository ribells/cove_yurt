/* =============================================================================
	File: data_layer.cpp

 =============================================================================== */

#pragma warning(push)
#pragma warning(disable : 4312)
#pragma warning(disable : 4311)
#ifdef WIN32
#include <GL/gl.h>
#else
#include <OpenGL/gl.h>
#endif
//#include "glext.h"
#pragma warning(pop)
#include <stdlib.h>
#include <memory.h>

//#include "netcdf.h"

#include "utility.h"
#include "scene/scene_mgr.h"
#include "soap/wfservices.h"
#include "web/web_file.h"
#include "data_layer.h"

bool	getWFFile_proxy(string strWFPath, string strPath, bool bRerun);

/* =============================================================================
    Data Handling Class
 =============================================================================== */
CDataLayer::CDataLayer()
{
	clearLayerPtrs();
	m_PrevLayer = NULL;
	m_NextLayer = NULL;

	setLoaded(false);
	m_Dirty = true;
	m_CurVar = -1;
	m_CurVarName = "";

	m_LastPntCur = -1;
	m_DisplayCnt = 0;

	m_TimeFormat = TIME_FMT_MDY;
	m_ConvertDepths = CONVERT_NONE;
	m_DisplaceFactor = 1.0;

	m_DataWinPtr = NULL;
	m_dw_x = m_dw_y = m_dw_w = m_dw_h = 0;

	//  file loading
	m_ShareFile = true;
	m_MultiFile = false;
	m_UpdateFile = false;
	m_UpdateFileType = LOAD_IMMEDIATE;
	m_UpdateRepeatDur = DUR_DAY;
	m_UpdateRepeatTime = NO_TIME;
	m_RerunWF = false;

	m_CycleData = false;
	m_CycleDataDur = DUR_DAY;

	//  tracking time
	m_Time = getCurrentTime();
	m_Start = m_Finish = NO_TIME;
	m_SelStart = m_SelFinish = NO_TIME;
	m_Lead = NO_TIME;
	m_Trail = 0;
	m_TimeStart = NO_TIME;
	m_TimeStep = 1;

	//  filtering
	m_UseDisplayStride = false;
	m_DisplayStride = 1;
	m_DisplayMinPos = m_DisplayMaxPos = vl_0;
	m_UseDisplayMinPos = m_UseDisplayMaxPos = false;
	for (int i = 0; i < 3; i++)
	{
		m_ViewDimActive[i] = (i == 0);
		m_ViewDimGrid[i] = false;
		m_ViewPlaneValue[i] = 0;
		m_ViewGridValue[i] = 5;
	}

	//  color
	m_Color = CLR_WHITE;
	m_GradientBins = false;
	m_GradientLine = false;
	m_GradientFlip = false;
	m_GradientLog = false;
	m_GradientLegend = true;
	m_Contours = 20;
	m_Transparency = 1.0;
	m_UseDisplayMinVal = m_UseDisplayMaxVal = false;
	m_ClampDisplayMinVal = m_ClampDisplayMaxVal = false;
	m_DisplayMinVal = m_DisplayMaxVal = 0;
	setGradientFile("system/gradients/rainbow_1.bmp");
	m_UseGradient = true;

	//  points
	m_ViewType = VIEW_POINTS;
	m_PrimType = 1;
	m_PointSize = 1;
	for (int i = 0; i < 3; i++) {
		m_ShowScaleSize[i] = false;
	}
	m_ScaleValue = vl_1;
	m_MagnifyCurrent = false;
	m_ColorByTime = false;

	//  vectors
	m_VectorLength = 1.0;
	m_VectorWidth = 1.0;
	m_ShowArrows = true;
	m_ShowVectorDims = 2;

	//  lines
	m_LineSize = 2;
	m_FadeLines = false;
	m_LineStride = 1;

	//  attached objects
	m_CamAttached = false;
	m_CamType = 0;
	m_AttachObj = false;
	m_AttachObjID = "";
	m_AttachObjPtr = NULL;
	m_LastInterpPos = vl_0;
	m_LastInterpDir = vl_0;

	//  attached video
	//m_MoviePtr = NULL;
	m_Vid_txid = -1;
	m_Vid_h = 0;
	m_Vid_w = 0;
	m_ImageVar = -1;
	m_ImagePathCur = "";
	m_ImagePathNew = "";
	m_AttachVideo = true;
	m_AttachVideoFile = false;
	m_AttachVideoPath = "";
	m_AttachVideoWindow = false;

	m_EditAutoUpdate = false;

	//  particles
	m_ParticlesDirty = true;
	m_ParticlesRebuild = true;
	m_ParticleData = NULL;
	m_ParticleForward = true;
	m_ParticleTimeRelease = false;
	m_ParticleStep = DUR_HOUR;
	m_ParticleValue = PART_SPEED;
	m_PointObjectType = NULL;
	m_PointEdit = false;

	//  planes
	m_PlanesDirty = true;
	m_PlaneData = NULL;
	m_PlanesType = PLANES_MODEL;
	m_PlanesCustomType = PLANES_CUSTOM_VSLICE;
	m_PlanesShade = false;
	m_PlanesWindow = false;
	m_PlanesPresetPos = vl_0;
	m_LineEdit = false;
	m_PlanesLineType = NULL;
	m_PlaneMin = vl_0;
	m_PlaneMax = vl_0;
	m_PlanesTimeSlicePos = Vec3d(NODATA, NODATA, NODATA);

	//  iso surfaces
	for (int i = 0; i < ISO_MAX; i++)
	{
		m_IsoValue[i] = 0;
		m_IsoValueActive[i] = false;
	}

	m_IsoShadeFlip = false;
}

/* =============================================================================
 =============================================================================== */
void CDataLayer::clean()
{
	if (getDataLoaded())
	{
		editData().setUseCount(getData().getUseCount() - 1);

		//  delete if no one else is using data layer
		if (getData().getUseCount() == 0)
			delete m_DataPtr;
		m_DataPtr = NULL;
	}

	clearLayer();
	delDataWin();
	editModel().clean();
	editPlotModel().clean();
	editGradient().clean();

	m_LastPntCur = -1;
	m_CurVar = -1;

	m_DisplayNdx.clear();
	std::vector<int> ().swap(m_DisplayNdx);
	m_DataPlane.clear();
	std::vector<float> ().swap(m_DataPlane);
	m_DataPlane2.clear();
	std::vector<float> ().swap(m_DataPlane2);
	m_DataPlane3.clear();
	std::vector<float> ().swap(m_DataPlane3);
	m_ScnPlane.clear();
	std::vector<Vec3f> ().swap(m_ScnPlane);
	m_ScnPlane2.clear();
	std::vector<Vec3f> ().swap(m_ScnPlane2);

	m_Start = m_Finish = m_Time = getCurrentTime();
}

/* =============================================================================
 =============================================================================== */
void CDataLayer::resetDimFilter()
{
	for (int i = 0; i < 3; i++)
	{
		int iMax = getViewDimMax(i);
		m_ViewPlaneValue[i] = min(iMax - 1, m_ViewPlaneValue[i]);
		m_ViewGridValue[i] = min(iMax - 1, m_ViewGridValue[i]);
	}
}

/* =============================================================================
 =============================================================================== */
void CDataLayer::initCurVar()
{
	int idx = getData().findVar(m_CurVarName);
	if (idx == -1) 
		idx = getData().getLargestOfType(DATA_SCALAR);
	if (idx == -1) 
		cout << ("WARNING: No variables found to view");
	setCurVar(idx);
}

/* =============================================================================
 =============================================================================== */
void CDataLayer::initActivate()
{
	initCurVar();

	//  check display setting
	int iFormat = getData().getDataFormat();
	int iView = getViewType();
	bool bValid[DFORMAT_MAX][VIEW_MAX] =
	{
		{ true, true, false, false, false },
		{ true, true, true, true, false },
		{ true, false, false, true, false },
		{ true, true, true, true, false },
		{ true, true, true, true, true },
		{ true, false, false, true, false }
	};
	if (!bValid[iFormat][iView])
		setViewType(VIEW_POINTS);

	vector<double> dpos = getData().getPosMinMax();
	setMinPos(Vec3d(dpos[0], dpos[1], -dpos[5]));
	setMaxPos(Vec3d(dpos[3], dpos[4], -dpos[2]));

	createGradient();
	attachVideo();

	if (iFormat == DFORMAT_3DMODEL || iFormat == DFORMAT_2DSURFACE)
	{
		Vec3d center = (Vec3d(dpos[0], dpos[1], 0) + Vec3d(dpos[3], dpos[4], 0)) / 2.0;
		if (m_ParticlePosList.size() == 0)
		{
			double dist = abs(dpos[3] - dpos[0]) / 20.0;
			addPoints(center, dist, 3, 0);
			updatePointList();
		}

		if (iFormat == DFORMAT_3DMODEL)
		{
			if (m_PlanesVertSliceList.size() == 0)
			{
				vector<Vec3d> poslist;
				Vec3d vec = Vec3d(dpos[0], dpos[1], dpos[2]) - center;
				poslist.push_back(center + vec / 2);
				poslist.push_back(center);
				poslist.push_back(center - vec / 2);
				setPlanesVertSliceList(poslist);
			}

			if (m_PlanesTimePlotList.size() == 0)
			{
				vector<Vec3d> poslist;
				poslist.push_back(center);
				setPlanesTimePlotList(poslist);
			}

			if (m_PlanesTimeSlicePos == Vec3d(NODATA, NODATA, NODATA))
				setPlanesTimeSlicePos(center);
		}
	}

	if (g_Env.m_Verbose)
	{
		cout << (getData().getHeader());
		cout << (getData().getGridExtents());
	}

	setParticlesRebuild();
	setPlanesDirty();
	setDirty();
}

/* =============================================================================
 =============================================================================== */
bool CDataLayer::setActive(bool bActive)
{
	string strSupported[] = { ".csv", ".txt", ".nc", ".cdf", "" };

	if (!bActive)
	{
		clean();
		if (getShowVideo()) 
			closeVideo();
		setLoaded(false);
		return true;
	}

	setLoaded(false);
	if (m_Link == "")
		return false;

	bActive = false;
	initLayer();

	//  if share is set, check if data is already loaded
	bool bShared = false;
	if (getShareFile())
	{
		CDataLayer	*pShareLayer = getSharedDataLayer(m_Link);
		if (pShareLayer)
		{
			m_DataPtr = pShareLayer->getDataPtr();
			m_MultiFileData = pShareLayer->m_MultiFileData;
			m_MultiFileCur = pShareLayer->m_MultiFileCur;
			cout << (" - using shared data file");
			setLocalFilePath(pShareLayer->getLocalFilePath());
			bShared = true;
		}
	}

	//  load the file(s)
	CDataPtr dataPtr = NULL;
	if (!bShared)
	{

		//  parse multi-file list
		vector<string> strFiles;
		if (getMultiFile())
		{
			strFiles = Tokenize(m_Link, ";");
			m_MultiFileData.clear();
			m_MultiFileCur = 0;
		}
		else
			strFiles.push_back(m_Link);

		for (int f = 0; f < strFiles.size(); f++)
		{
			//  figure out if we need to update file
			string strLink = strFiles[f];
			string strTempPath = "";
			string strCurPath = getLocalPath(strLink);
			if (getUpdateFile() && findfile(strCurPath, strSupported))
			{
				bool bForceUpdate = false;
				if (getUpdateFileType() == LOAD_SCHEDULE)
				{
					dtime file_time = filetime(strCurPath);
					if (file_time != NO_TIME)
					{
						dtime len = getUpdateRepeatDur();
						dtime start = getUpdateRepeatTime();
						dtime update_time = floor((getCurrentTime() - start) / len) * len + start;
						if (file_time < update_time) 
							bForceUpdate = true;
					}
				}

				if (bForceUpdate || getUpdateFileType() == LOAD_IMMEDIATE || getUpdateFileType() == LOAD_ACTIVATION)
				{
					strTempPath = strCurPath + ".tmp";
					renfile(strCurPath, strTempPath);
				}
			}

			//  download the file
			string strLocal;
			if (strLink.substr(0, 5) == "wfsvr")
			{
				strLocal = getLocalPath(strLink);

				bool bRet = false;
				//if (lwrstr(g_Env.m_TridentServerPath) == "proxy")
				//	bRet = getWFFile_proxy(strLink, strLocal, getRerunWF());
				//else
				//	bRet = get_ws_mswf()->getWFFile(strLink, strLocal, getRerunWF());
				if (!bRet) 
					strLocal == "";
			}
			else
			{
				g_Env.m_DownloadName = m_Name;
				strLocal = cacheWebFiles(strLink, "datasets", strSupported);
				g_Env.m_DownloadName = "";
			}

			//  if updating file then check if it worked
			if (strTempPath != "")
			{
				if (strLocal == "")	//  didn't download new one, get old one back
				{
					renfile(strTempPath, strCurPath);
					strLocal = strCurPath;
					cout << (" - ERROR: unable to update local file for " + strLink);
				}
				else
				{
					delfile(strTempPath);	//  else remove old temp file
					if (getUpdateFileType() == LOAD_IMMEDIATE) 
						setUpdateFile(false);
				}
			}

			if (strLocal == "")
				return false;

			//  try to load the data file
			bool bSuccess = false;
			CDataPtr newData = new CData();
			newData->setTimeFormat(getTimeFormat());
			cout << "\n - loading dataset: " + strLocal;
			if (getExt(strLocal) == ".bil" || getExt(strLocal) == ".grd" || getExt(strLocal) == ".asc")
			{
				newData->setLoadAsTerrain(false);
				bSuccess = newData->readTerrain(strLocal);
			}
			else if (getExt(strLocal) == ".csv" || getExt(strLocal) == ".txt")
				bSuccess = newData->readCSV(strLocal);
			else
				bSuccess = newData->readNetCDF_Raw(strLocal);

			if (f == 0 && !bSuccess)			//  if 0th or only data file init data for layer
			{
				cout << ("Error: Could not activate data set - unable to load file " + strLocal);
				delete newData;
				return false;
			}

			newData->convertNetCDF(true, false);

			if (f == 0)
			{
				dataPtr = newData;
				setLocalFilePath(strLocal);
			}

			//  add to multifile list if necessary
			if (getMultiFile())
			{
				if (f == 0 || dataPtr->isSameFormat(newData))
				{
					vector<double> vtime = newData->getTimeMinMax();
					multi_file mdata;
					mdata.strPath = strLocal;
					mdata.dMinTime = vtime[0];
					mdata.dMaxTime = vtime[1];

					//  sort and if it's first then change mainhandle
					if (f == 0)
						m_MultiFileData.push_back(mdata);
					else
					{
						int iCur = 0;
						vector<multi_file>::iterator	iter = m_MultiFileData.begin();
						for (; iter != m_MultiFileData.end(); iter++, iCur++)
							if (mdata.dMinTime < (*iter).dMinTime)
								break;
						m_MultiFileData.insert(iter, mdata);
						if (iCur == 0)
						{
							CDataPtr tmpData = dataPtr;
							dataPtr = newData;
							newData = tmpData;
							setLocalFilePath(strLocal);
						}

						delete newData;
					}
				}
				else
				{
					cout << ("Error: MultiFile data format was incorrect: " + strLocal);
					delete newData;
				}
			}
		}

		assert(dataPtr != NULL);
		dataPtr->convertNetCDF(false, true);
		dataPtr->sortTime();
		m_DataPtr = dataPtr;
	}

	editData().setUseCount(getData().getUseCount() + 1);
	initActivate();
	editData().setDirty(false);
	setLoaded(true);
	return true;
}

/* =============================================================================
 =============================================================================== */
void CDataLayer::setCurVar(int iVar)
{
	if (!getData().getVarValid(iVar))
		return;

	if (getData().isNCFile())
	{
		int iVecCnt = m_ShowVectorDims;
		if (m_CurVar != -1 && m_CurVar != iVar && !isSharedData() && getData().getVar(m_CurVar).getReloadable())
		{
			editData().clearVarFData(m_CurVar);
			setShowVectorDims(0);
		}

		editData().loadVar(iVar);
		m_CurVar = iVar;
		setShowVectorDims(iVecCnt);
	}

	m_CurVar = iVar;
	m_CurVarName = getData().getVarName(m_CurVar);

	//  reset max and min based on data
	editData().setCoordVars(iVar);
	setCurVarMinMax();
	setExtents();

//	if (g_Env.m_Verbose) { 
//		ostringstream s;
//		s << " selected variable: " << m_CurVarName << "[" << getData().getVarMin(m_CurVar) << ", " << getData().getVarMax(m_CurVar) << "]";
//		cout << (s.str());
//	}
}

/* =============================================================================
 =============================================================================== */
void CDataLayer::setShowVectorDims(int iDim)
{
	if (getDataLoaded() && getData().isNCFile())
	{
		if (m_CurVar + iDim > getData().getVarCnt())
			return;
		if (iDim < m_ShowVectorDims && !isSharedData())	//  remove unused variable data
		{
			for (int i = max(iDim, 1); i < m_ShowVectorDims && m_CurVar + i < getData().getVarCnt(); i++)
			{
				if (getData().getVarType(m_CurVar + i) != DATA_SCALAR)
					break;
				editData().clearVarFData(m_CurVar + i);
			}
		}
		else if (getShowVectors() || getShowParticles()) //  add variable data for other dimensions
		{
			for (int i = 1; i < iDim; i++) 
				editData().loadVar(m_CurVar + i);
		}
	}

	m_ShowVectorDims = iDim;
	setCurVarMinMax();
}

/* =============================================================================
 =============================================================================== */
void CDataLayer::setCurVarMinMax()
{
	if (!getData().getVarValid(m_CurVar))
	{
		m_CurVarMax = m_CurVarMin = 0.0;
		return;
	}

	if (getShowVectors())
	{
		double dMax0 = 0.0, dMax1 = 0.0;
		dMax0 = max(abs(getData().getVarMax(m_CurVar)), abs(getData().getVarMin(m_CurVar)));
		if (getData().getVarValid(m_CurVar + 1))
			dMax1 = max(abs(getData().getVarMax(m_CurVar + 1)), abs(getData().getVarMin(m_CurVar + 1)));
		m_CurVarMin = 0;
		m_CurVarMax = len(Vec3d(dMax0, dMax1, 0));
	}
	else
	{
		m_CurVarMax = getData().getVarMax(m_CurVar);
		m_CurVarMin = getData().getVarMin(m_CurVar);
	}
}

/* =============================================================================
 =============================================================================== */
void CDataLayer::setExtents()
{
	vector<double> time = getData().getTimeMinMax();
	if (getMultiFile() && m_MultiFileData.size() > 0)
	{
		time[0] = m_MultiFileData[0].dMinTime;
		time[1] = m_MultiFileData[m_MultiFileData.size() - 1].dMaxTime;
	}

	m_Start = time[0];
	m_Finish = time[1];

	vector<double> pos = getData().getPosMinMax();
	for (int i = 0; i < 3; i++)
	{
		m_MinPos[i] = pos[i];
		m_MaxPos[i] = pos[i + 3];
	}
}
