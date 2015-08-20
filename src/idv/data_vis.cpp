/* =============================================================================
	File: data_draw.cpp

 =============================================================================== */

#pragma warning(push)
#pragma warning(disable : 4312)
#pragma warning(disable : 4311)
#ifdef WIN32
#include <GL/gl.h>
#else
#include <GL/gl.h>
#include <GL/glut.h>
#endif
//#include "glext.h"
#pragma warning(pop)
#include <stdlib.h>
#include <memory.h>

//#include "netcdf.h"

#include "utility.h"
#include "scene/scene_mgr.h"
#include "scene/movie.h"
#include "web/web_file.h"
#include "gl_draw.h"
#include "data_layer.h"
#include "world.h"

#include <vl/VLgl.h>

Vec3d			getCameraFromGPS(Vec3d pos, float scale[2]);
Vec3d			getMeshCenter();
Vec3d			resetLookAt(Vec3d pnt);

//  BSP DEBUGGING
vector<Vec3d>	g_BSPCellCoords;

/* =============================================================================
 =============================================================================== */
void bsp_debug_cell_coords(Vec3d *pCrnr)
{
	if (pCrnr == NULL)
	{
		g_BSPCellCoords.clear();
		return;
	}

	const int	tri[4][3] = { { 0, 1, 3 }, { 0, 3, 2 }, { 4, 5, 7 }, { 4, 7, 6 } };
	for (int j = 0; j < 4; j++)
		for (int n = 0; n < 3; n++)
			g_BSPCellCoords.push_back(pCrnr[tri[j][n]]);
}

/* =============================================================================
 =============================================================================== */
void bsp_debug_draw_cells()
{
	if (g_BSPCellCoords.size() == 0)
		return;
	glLineWidth(1);
	glColor4f(1, 1, 1, 1);
	for (int i = 0; i < g_BSPCellCoords.size();)
	{
		glBegin(GL_LINE_LOOP);
		for (int j = 0; j < 3; j++, i++)
			glVertex(g_BSPCellCoords[i]);
		glEnd();
	}
}

//  UTILITY ROUTINES
#define REAL				double
#define ANSI_DECLARATORS	1
#ifndef VOID
#define VOID	void
#endif
#include "data_vis_triangulate.h"

/* =============================================================================
 =============================================================================== */

bool triangulate_grid(vector<Vec3d> &posIn, int iStart, int iCnt, int **triOut, int *triCnt)
{
	struct triangulateio	in, out;

	//  Define input points.
	memset(&in, 0, sizeof(triangulateio));
	memset(&out, 0, sizeof(triangulateio));
	in.numberofpoints = iCnt;
	if (!mem_alloc(in.pointlist, iCnt * 2))
		return false;

	double	*dp = in.pointlist;
	for (int i = iStart; i < iStart+iCnt; i++)
	{
		*dp++ = posIn[i][0];
		*dp++ = posIn[i][1];
	}

	// Triangulate the points. Switches are chosen to quietly number everything from
	triangulate("zQ", &in, &out, NULL);

	//  Free all allocated arrays, including those allocated by Triangle.
	assert(in.numberofpoints == out.numberofpoints);
	free(in.pointlist);
	free(out.pointlist);
	free(out.pointmarkerlist);

	*triOut = out.trianglelist;
	*triCnt = out.numberoftriangles;

	return true;
}

//  VIDEO
const int	MVTEXSIZE = 512;

/* =============================================================================
 =============================================================================== */

void CDataLayer::clearVideoTexture()
{
	if (m_Vid_txid != -1)
		deleteTexture(m_Vid_txid);
	m_Vid_txid = -1;
}

/* =============================================================================
 =============================================================================== */
bool CDataLayer::openVideo(string FileName)
{
	/*
	if (m_MoviePtr) delete m_MoviePtr;
	m_MoviePtr = new CMovie();

	if (!getMovie().OpenMovie(FileName.c_str(), MVTEXSIZE, MVTEXSIZE))
	{
		delete m_MoviePtr;
		m_MoviePtr = NULL;
		return false;
	}

	clearVideoTexture();
	GLuint	textureID;
	glGenTextures(1, &textureID);
	m_Vid_txid = textureID;
	glBindTexture(GL_TEXTURE_2D, m_Vid_txid);

	unsigned char	*pData = getMovie().GetMovieFrame(0.0);
	GLenum  glType = getIntel() ? GL_UNSIGNED_BYTE : GL_UNSIGNED_INT_8_8_8_8_REV;
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, MVTEXSIZE, MVTEXSIZE, 0, GL_BGRA_EXT, glType, pData);
	return true;
	*/
	return false;
}

/* =============================================================================
 =============================================================================== */
void CDataLayer::closeVideo()
{
	m_ImageVar = -1;
	m_ImagePathNew = "";
	m_ImagePathCur = "";
	//if (m_MoviePtr) delete m_MoviePtr;
	//m_MoviePtr = NULL;
	//clearVideoTexture();
}

/* =============================================================================
 =============================================================================== */
bool CDataLayer::getShowVideo() const
{
	//return m_MoviePtr != NULL || m_Vid_txid != -1;
	return false;
}

/* =============================================================================
 =============================================================================== */
void CDataLayer::getVideoSize(int &w, int &h) const
{
	w = m_Vid_w;
	h = m_Vid_h;
}

/* =============================================================================
 =============================================================================== */
void CDataLayer::attachVideo()
{
	m_ImageVar = -1;
	m_ImagePathCur = "";
	m_ImagePathNew = "";
	setDirty();
	if (!m_AttachVideo)
		return;
	if (m_AttachVideoFile)
	{
		m_ImagePathNew = m_AttachVideoPath;
		return;
	}
	else if (getDataLoaded())
	{

		//  if no file then see if there is an image variable in the data
		for (int i = 0; i < getData().getVarCnt(); i++)
		{
			if (getData().getVarType(i) != DATA_ENUM)
				continue;

			string	strVarName = lwrstr(getData().getVarName(i));
			if (strVarName == "image" || strVarName == "video")
			{
				m_ImageVar = i;
				return;
			}
		}
	}
}

/* =============================================================================
 =============================================================================== */
bool CDataLayer::updateVideoTexture()
{

	//  update bitmap based on current time
	if (m_ImagePathNew == "")
	{
		m_ImagePathCur = "";
		clearVideoTexture();
		return false;
	}

	string	strSupported[] = { "" };
	if (getExt(m_ImagePathNew) == ".mp4")
	{
		if (m_ImagePathNew != m_ImagePathCur)
		{
			clearVideoTexture();

			string	strPath = cacheWebFiles(m_ImagePathNew, "datasets", strSupported);
			m_ImagePathCur = m_ImagePathNew;
			if (!openVideo(strPath))
			{
				cout << ("Unable to load the image from data file: " + strPath);
				return false;
			}
		}

		//  update video frame based on current time
		/*
		if (getMovieLoaded())
		{
			double	v_tm = 0;
			if (m_Time > getStart())
				v_tm = (double) (m_Time - getStart()) + m_FrameTime;

			unsigned char	*pData = getMovie().GetMovieFrame(v_tm);
			getMovie().getMovieSize(m_Vid_w, m_Vid_h);
			if (pData)
			{
				glBindTexture(GL_TEXTURE_2D, m_Vid_txid);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);	//  Set Texture Max Filter
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);	//  Set Texture Min Filter
				GLenum	glType = getIntel() ? GL_UNSIGNED_BYTE : GL_UNSIGNED_INT_8_8_8_8_REV;
				glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, MVTEXSIZE, MVTEXSIZE, GL_BGRA_EXT, glType, pData);
				return true;
			}
		}
		*/
	}
	else
	{
		if (m_ImagePathNew != m_ImagePathCur)
		{
			clearVideoTexture();

			int		w, h;
			string	strPath = cacheWebFiles(m_ImagePathNew, "datasets", strSupported);
			m_ImagePathCur = m_ImagePathNew;
			m_Vid_txid = createTexture(strPath, w, h);
			if (m_Vid_txid == -1)
			{
				cout << ("Unable to load the image from data file: " + strPath);
				return false;
			}

			m_Vid_w = w;
			m_Vid_h = h;
		}

		glBindTexture(GL_TEXTURE_2D, m_Vid_txid);
		return true;
	}

	return false;
}

/* =============================================================================
 =============================================================================== */
void CDataLayer::delDataWin()
{
	if (m_DataWinPtr == NULL)
		return;
	g_Draw.getDataWindowSet().delWindow((CWindow *) m_DataWinPtr);
	m_DataWinPtr = NULL;
}

/* =============================================================================
 =============================================================================== */
void *CDataLayer::getDataWin()
{

	//  if no window, then create new window
	if (m_DataWinPtr == NULL)
		m_DataWinPtr = g_Draw.getDataWindowSet().addWindow(m_dw_x, m_dw_y, m_dw_w, m_dw_h);

	//  save the window position
	CWindow *pWindow = (CWindow *) m_DataWinPtr;
	pWindow->getPos(m_dw_x, m_dw_y, m_dw_w, m_dw_h);
	return pWindow;
}

/* =============================================================================
 =============================================================================== */
void CDataLayer::drawVideo()
{
	/*
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glColor4f(1, 1, 1, 1);

	delDataWin();

	Vec3f	pos = getLastInterpPos();
	Vec3f	dir = getLastInterpDir();
	if (dir == vl_0)
		return;

	Vec3f	rot = Vec3f(0, 180, 0);
	glPushMatrix();

	float	scale = 15 / MetersPerDegree;	//  determines size of video image
	glTranslatef(pos[0], pos[1], pos[2]);
	glScalef(scale, scale, scale);

	Vec3d	srot = getSceneRot(pos);
	glRotatef(srot[1], 0, 1, 0);
	glRotatef(srot[2], 0, 0, 1);

	glRotatef(rot[1] - (180 - getObjectRot()), 0, 1, 0);

	Vec3f	pt[4];
	pt[0] = Vec3f(-1, 0, 1);
	pt[1] = Vec3f(1, 0, 1);
	pt[2] = Vec3f(1, 0, -1);
	pt[3] = Vec3f(-1, 0, -1);

	glBegin(GL_QUADS);
	glTexCoord2f(0, 1);
	glVertex(pt[0]);
	glTexCoord2f(1, 1);
	glVertex(pt[1]);
	glTexCoord2f(1, 0);
	glVertex(pt[2]);
	glTexCoord2f(0, 0);
	glVertex(pt[3]);
	glEnd();
	glDisable(GL_TEXTURE_2D);

	glPopMatrix();
	*/
}

/* =============================================================================
    ATTACH CAMERA & OBJECTS Figure out interpolated vectors and positions for
    attached objects and cameras to tracks
 =============================================================================== */
void CDataLayer::setAttachData(int iPntCur)
{
	if (iPntCur == -1 || getData().getTimeSteps() != getData().getVarSize(m_CurVar))
		return;

	Vec3f	pnts[4];
	float	head[2] = { 0, 0 };
	int		iHeadVar = getData().findVar("heading");
	if (iHeadVar == -1)
		iHeadVar = getData().findVar("head");

	// figure out the 4 points of the curve
	// find previous 2 points
	int iAddPrev = 0;
	int iPrev = -1;
	for (int i = iPntCur; i >= 0 && iAddPrev < 2; i--)
	{
		if (editData().getVarFDataValue(m_CurVar, i) == getData().getVarNoValue(m_CurVar)
			||	!editData().isDataPosValid(m_CurVar, editData().getDataPos(m_CurVar, i)))
			continue;
		pnts[1 - iAddPrev] = getScenePos(getDataNdxPos(i));
		if (iAddPrev == 0)
		{
			iPrev = i;
			if (iHeadVar != -1)
				head[0] = editData().getVarFDataValue(iHeadVar, i);
		}

		iAddPrev++;
	}

	//  find trailing 2 points
	int iAddNext = 0;
	int iNext = -1;
	int iSize = getData().getVarSize(m_CurVar);
	for (int i = iPntCur + 1; i < iSize && iAddNext < 2; i++)
	{
		if (editData().getVarFDataValue(m_CurVar, i) == (float)
			getData().getVarNoValue(m_CurVar)
			||	!editData().isDataPosValid(m_CurVar, editData().getDataPos(m_CurVar, i)))
			continue;
		pnts[2 + iAddNext] = getScenePos(getDataNdxPos(i));
		if (iAddNext == 0)
		{
			iNext = i;
			if (iHeadVar != -1)
				head[1] = editData().getVarFDataValue(iHeadVar, i);
		}

		iAddNext++;
	}

	//  if no valid points set to 0 and leave
	if (iAddPrev == 0 && iAddNext == 0)
	{
		m_LastInterpPos = m_LastInterpDir = vl_0;
		return;
	}

	//  fill in for end cases
	if (iAddPrev == 0)
	{
		pnts[1] = pnts[2];
		head[0] = head[1];
		iPrev = iNext;
	}

	if (iAddNext == 0)
	{
		pnts[2] = pnts[1];
		head[1] = head[0];
		iNext = iPrev;
	}

	if (iAddPrev == 1) pnts[0] = pnts[1];
	if (iAddNext == 1) pnts[3] = pnts[2];

	//  figure out the time offset
	double	dPrevTime = getData().getDataTime(iPrev);
	double	dNextTime = getData().getDataTime(iNext);
	double	dTime = min(dNextTime, max(dPrevTime, m_Time + m_FrameTime));
	double	dPos = dNextTime == dPrevTime ? 0 : (dTime - dPrevTime) / (dNextTime - dPrevTime);

	//  now do a spline to get tangent and position on curve
	Vec3f	pos, dir;
	CatmullRomPnt(pos, dir, dPos, pnts);
	if (iHeadVar != -1)
		dir = xform(HRot4f(Vec3f(0, 1, 0), -(head[1] + head[0]) * vl_pi / 360.0), Vec3f(0, 0, -1));

	if (len(pos - m_LastInterpPos) > (1e-5))
	{
		m_LastInterpPos = pos;
		m_LastInterpDir = dir;
	}
}

/* =============================================================================
    DATA GRADIENT ROUTINES
 =============================================================================== */
void CDataLayer::setGradientTex()
{
	unsigned char	*pBmp = NULL;
	CGradient		&Grad = editGradient();
	int				w = 256, h = 256;

	m_UseGradient = true;
	if (getUseGradient() && Grad.getGradMap() == NULL) {
		if (!Grad.updateMap()) {
			setUseGradient(false);
		}
	}
	if (getUseGradient()) {
		w = Grad.getMapWidth();
		pBmp = Grad.createGradBitmap(NULL, m_Contours, m_GradientLine, m_GradientBins, m_GradientFlip, m_GradientLog);
	} else {
		color	c = getColor();
		if (!mem_alloc(pBmp, w * h * 4))
			return;

		unsigned char	*pTmp = pBmp;
		for (int i = 0; i < w * h; i++)
		{
			*pTmp++ = clrR(c);
			*pTmp++ = clrG(c);
			*pTmp++ = clrB(c);
			*pTmp++ = clrA(c);
		}
	}

	int txID = createTexture(pBmp, w, h, GL_RGBA, GL_RGBA, TX_CLAMP_EDGE);
	Grad.setTexID(txID);
	if (!getUseGradient())
		delete[] pBmp;

	setDirty();
}

/* =============================================================================
 =============================================================================== */
float CDataLayer::getGradOffset(float fData) const
{
	float	maxval = m_UseDisplayMaxVal ? m_DisplayMaxVal : m_CurVarMax;
	float	minval = m_UseDisplayMinVal ? m_DisplayMinVal : m_CurVarMin;
	if (fData < minval && m_UseDisplayMinVal && m_ClampDisplayMinVal) fData = minval;
	if (fData > maxval && m_UseDisplayMaxVal && m_ClampDisplayMaxVal) fData = maxval;

	float	gval = (maxval == minval) ? 0.5 : (fData - minval) / (maxval - minval);
	return gval;
}

/* =============================================================================
 =============================================================================== */
void CDataLayer::setPointColor(int iPnt, int iSelectID, double dTrans)
{
	if (iSelectID != -1) {
		glColor4ub(iPnt & 0xff, (iPnt >> 8) & 0xff, (iPnt >> 16) & 0xff, 0xff);
	} else {
		color	clr = 0x00000000;
		float dval = 0.5;
		if (getShowPoints() && getColorByTime()) {
			vector<double> tm = getData().getTimeMinMax();
			double dtime = getData().getDataTime(m_DisplayNdx[iPnt]);
			dval = (tm[1] == tm[0]) ? 0.5 : (dtime - tm[0]) / (tm[1] - tm[0]);
		} else {
			dval = getGradOffset(m_DataPlane[m_DisplayNdx[iPnt]]);
		}
		dTrans = min(1.0, dTrans);
		if (dval >= 0 && dval <= 1.0) {
			clr = getUseGradient() ? getGradient().getColor(dval) : getColor();
		}

		float tempr = float(clr >> 24) / (float)0xFF;
		float tempg = float(0xFF & (clr >> 16)) / (float)0xFF;
		float tempb = float(0xFF & (clr >> 8)) / (float)0xFF;

		float colr[] = { tempr, tempb, tempg, 0.5};
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, colr);

		//USED OTHER ENDIAN:
		//glColor4ub(clrR(clr), clrG(clr), clrB(clr), dTrans * clrTrans(clr) * getTransparency() * 255.0);
	}
}

/* =============================================================================
    BUILD MESH FOR DATA SLICE
 =============================================================================== */
Vec3f CDataLayer::getScenePos(Vec3d pos) const
{
	bool	bScaled = getConvertDepths() < CONVERT_SCALE || getShowIsoSurfaces();
	return g_Draw.getScenePntFromGPS(recenterGPS(pos), bScaled);
}

/* =============================================================================
 =============================================================================== */
Vec3d CDataLayer::getDataNdxPos(int ndx) const
{
	Vec3d	pos = editData().getDataPos(m_CurVar, ndx);
	if (!editData().isDataPosValid(m_CurVar, pos))
		return Vec3d(NODATA, NODATA, NODATA);
	if (getConvertDepths() == CONVERT_TERRAIN)
	{
		g_Draw.setTerrainHeight(pos);
		pos[2] *= -1;
	}
	else if (getConvertDepths() == CONVERT_SCALE)
		pos[2] *= m_DisplaceFactor;
	else if (getConvertDepths() == CONVERT_VALUE && !getShowIsoSurfaces())
	{
		float	maxval = m_UseDisplayMaxVal ? m_DisplayMaxVal : m_CurVarMax;
		float	minval = m_UseDisplayMinVal ? m_DisplayMinVal : m_CurVarMin;
		double	dFactor = -m_DisplaceFactor * 1000.0 / (maxval - minval);
		pos[2] = (m_DataPlane[ndx] - minval) * dFactor;
	}

	pos = pos * Vec3d(1, 1, -1) + getPosOffset();
	return pos;
}

/* =============================================================================
 =============================================================================== */
bool CDataLayer::createSliceNdxList(int iSliceType, int iSlice, vector<float> &vSlice, int &iRows, int &iCols)
{
	int iDimCnt = getData().getVarDims(m_CurVar).size();
	//int pSize[NC_MAX_DIMS];
	int pSize[10];
	vector<int> dims(iDimCnt, -1);

	for (int i = 0; i < iDimCnt; i++)
	{
		pSize[i] = getData().getDimSize(getData().getVarDims(m_CurVar)[i]);
		if (iDimCnt - i > 3) dims[i] = 0;
	}

	switch(iSliceType)
	{
	case SLICE_DEPTH:	//  depth
		if (getData().getLatCnt() == 1 || getData().getLonCnt() == 1)
			return false;
		iRows = pSize[iDimCnt - 2];
		iCols = pSize[iDimCnt - 1];
		dims[iDimCnt - 3] = iSlice;
		break;

	case SLICE_LAT:		//  latitude
		if (getData().getLonCnt() == 1 || getData().getDepthCnt() == 1)
			return false;
		iRows = pSize[iDimCnt - 3];
		iCols = pSize[iDimCnt - 1];
		dims[iDimCnt - 2] = iSlice;
		break;

	case SLICE_LON:		//  longitude
		if (getData().getLatCnt() == 1 || getData().getDepthCnt() == 1)
			return false;
		iRows = pSize[iDimCnt - 3];
		iCols = pSize[iDimCnt - 2];
		dims[iDimCnt - 1] = iSlice;
		break;
	}

	if (iRows <= 1 || iCols <= 1)
		return false;
	if (!editData().getVarSlice(m_CurVar, vSlice, dims, true))
		return false;
	return true;
}

/* =============================================================================
    given a position cube return a 3d model for a slice
 =============================================================================== */
void CDataLayer::createSliceModel
(
	int				iSliceType,
	vector<float>	&vSlice,
	int				iRows,
	int				iCols,
	bool			bWindow,
	bool			bNormals
)
{

	//  create new object
	int cnt = vSlice.size();
	if (!editModel().createNewObject(cnt, cnt, cnt, cnt, 0))
		return;

	C3DObject		&Object = getModel().editObject(getModel().getObjectCnt() - 1);
	vector<Vec3f>	&vVerts = Object.editVerts();
	vector<Vec3f>	&vTexVerts = Object.editTexVerts();
	vector<Vec3f>	&vNormals = Object.editNormals();
	vector<color>	&vClrVerts = Object.editClrVerts();

	float			fNoVal = getData().getVarNoValue(m_CurVar);

	vector<double>	p_x;
	double			dXStart = 0.0, dYStart = 0.0;;

	double	dXRange = 1.0, dYRange = 1.0;
	if (bWindow)
	{
		bNormals = false;
		Object.setReprojected(false);

		Vec3d	vmin(90, 360, EARTHMAX);
		Vec3d	vmax(-90, -360, EARTHMIN);
		for (int i = 0; i < iCols * iRows; i++)
		{
			Vec3d	pos = getDataNdxPos(vSlice[i]);
			if (m_DataPlane[vSlice[i]] == fNoVal)
				continue;
			for (int j = 0; j < 3; j++)
			{
				if (pos[j] > vmax[j]) vmax[j] = pos[j];
				if (pos[j] < vmin[j]) vmin[j] = pos[j];
			}
		}

		setPlaneMin(vmin);
		setPlaneMax(vmax);

		if (iSliceType == SLICE_DEPTH)
		{
			dXStart = vmin[1];
			dXRange = vmax[1] - vmin[1];
			dYStart = vmin[0];
			dYRange = vmax[0] - vmin[0];
		}

		if (iSliceType > SLICE_DEPTH)
		{
			int iCnt = (iSliceType == SLICE_VSECT) ? iRows : iCols;
			int iStep = (iSliceType == SLICE_VSECT) ? iCols : 1;
			p_x.resize(iCnt);

			double	dDist = 0.0;
			p_x[0] = 0.0;

			Vec3d	lastPos = getDataNdxPos(vSlice[0]);
			for (int i = 1; i < iCnt; i++)
			{
				int		ndx = vSlice[i * iStep];
				Vec3d	newPos = getDataNdxPos(ndx);
				dDist += max(1e-10, len((newPos - lastPos) * Vec3d(1, 1, 0)));
				p_x[i] = dDist;
				lastPos = newPos;
			}

			for (int i = 0; i < iCnt; i++)
				p_x[i] /= dDist;
			dYStart = vmin[2];
			dYRange = vmax[2] - vmin[2];
		}
	}

	//  create vertices
	int iCnt = 0;
	for (int r = 0; r < iRows; r++)
	{
		Vec3d	firstPos = vl_0;
		Vec3d	lastPos = vl_0;
		for (int c = 0; c < iCols; c++, iCnt++)
		{
			int		ndx = vSlice[iCnt];
			color	clr = 0;
			float	dval = 0;
			Vec3d	pos = getDataNdxPos(ndx);
			if (m_DataPlane[ndx] != fNoVal && !getCropped(pos))
			{
				if (Object.getPlaneDir() == vl_0)
				{
					if (firstPos == vl_0)
						firstPos = pos;
					else
						lastPos = pos;
				}

				dval = getGradOffset(m_DataPlane[ndx]);
				if (dval >= 0 && dval <= 1.0)
					clr = CLR_WHITE;
			}

			if (bWindow)
			{
				if (iSliceType == SLICE_DEPTH)
					vVerts[iCnt] = Vec3f((pos[1] - dXStart) / dXRange, (pos[0] - dYStart) / dYRange, 0.0);
				else if (iSliceType == SLICE_VSECT)
					vVerts[iCnt] = Vec3f(p_x[r], (pos[2] - dYStart) / dYRange, 0.0);
				else
					vVerts[iCnt] = Vec3f(p_x[c], (pos[2] - dYStart) / dYRange, 0.0);
			}
			else
				vVerts[iCnt] = getScenePos(pos);
			vNormals[iCnt] = Vec3f(0, 1, 0);
			vTexVerts[iCnt] = Vec3f(dval, 0, 0);
			vClrVerts[iCnt] = clr;
		}

		if (Object.getPlaneDir() == vl_0)
		{
			Vec3d	v = lastPos - firstPos;
			Object.setPlaneDir(norm(Vec3d(v[1], -v[0], 0)));
		}
	}

	//  create faces
	int w = iCols;
	for (int r = 0; r < iRows - 1; r++)
	{
		for (int c = 0; c < iCols - 1; c++)
		{
			int iCnt = Object.getNumFaces();
			if (vClrVerts[(r + 1) * w + c] && vClrVerts[r * w + c] && vClrVerts[(r + 1) * w + c + 1])
				Object.addFace((r + 1) * w + c, r * w + c, (r + 1) * w + c + 1);
			if (vClrVerts[(r + 1) * w + c + 1] && vClrVerts[r * w + c] && vClrVerts[r * w + c + 1])
				Object.addFace((r + 1) * w + c + 1, r * w + c, r * w + c + 1);
			if (iCnt != Object.getNumFaces())
				continue;

			//  wind the other way to try to add triangles
			if (vClrVerts[(r + 1) * w + c] && vClrVerts[r * w + c + 1] && vClrVerts[(r + 1) * w + c + 1])
				Object.addFace((r + 1) * w + c, r * w + c + 1, (r + 1) * w + c + 1);
			if (vClrVerts[(r + 1) * w + c] && vClrVerts[r * w + c] && vClrVerts[r * w + c + 1])
				Object.addFace((r + 1) * w + c, r * w + c, r * w + c + 1);
		}
	}

	Object.setTriangleStrip(false);
	Object.setReprojected(false);
	if (bNormals)
	{
		Object.ComputeVertexNormals();
		Object.setLighted(true);
	}
	else
		Object.setLighted(false);
	Object.setDirty(true);

		//if a window, move it to the plot window model
	if (bWindow)
	{
		editPlotModel().addObject(editModel().getObjectPtr(0));
		editModel().clearDataPtrs();
	}
}

/* =============================================================================
    given a position cube return a 3d model for a slice of it
 =============================================================================== */
void CDataLayer::createStreamModel(int iTimeSlice)
{
	int iConVar = getData().findVar("connect_point_mask");
	if (iConVar == -1)
		return;

	CData	&Data = editData();
	Data.loadVar(iConVar);

	//  create new object
	bool	bTimeSteps = Data.getTimeSteps() > 1;
	int		iSize = bTimeSteps ? Data.getTimeSliceSize(m_CurVar) : Data.getVarSize(m_CurVar);
	int		iStart = 0;
	int		cnt = iSize * 4;
	if (!editModel().createNewObject(cnt, cnt, cnt, cnt, 0))
		return;

	C3DObject		&Object = getModel().editObject(getModel().getObjectCnt() - 1);
	vector<Vec3f>	&vVerts = Object.editVerts();
	vector<Vec3f>	&vTexVerts = Object.editTexVerts();
	vector<Vec3f>	&vNormals = Object.editNormals();
	vector<color>	&vClrVerts = Object.editClrVerts();

	//  create color values and normals
	float			fNoVal = Data.getVarNoValue(m_CurVar);
	int				iCnt = 0;
	for (int i = 0, ndx = iStart; i < iSize; i++, ndx++)
	{
		color	clr = 0;
		float	dval = 0;
		if (m_DataPlane[ndx] != fNoVal)
		{
			dval = getGradOffset(m_DataPlane[ndx]);
			if (dval >= 0 && dval <= 1.0)
				clr = CLR_WHITE;
		}

		int iTot = (i == 0 || i == iSize - 1) ? 2 : 4;
		for (int j = 0; j < iTot; j++, iCnt++)
		{
			vNormals[iCnt] = Vec3f(0, 1, 0);
			vTexVerts[iCnt] = Vec3f(dval, 0, 0);
			vClrVerts[iCnt] = clr;
		}
	}

	//  create vertices
	vector<float>	&pConData = Data.getVarFData(iConVar);
	int				iWidVar = getData().findVar("channel_width");
	if (iWidVar != -1)
		editData().loadVar(iWidVar);
	iCnt = 0;

	Vec3d	pos0 = getDataNdxPos(iStart), pos1;
	for (int i = 0, iData = iStart; i < iSize - 1; i++, iData++)
	{
		pos1 = getDataNdxPos(iData + 1);

		bool	bSeg = (vClrVerts[4 * i] && vClrVerts[4 * i + 2] && pConData[iData] == 1);
		Vec3d	vec = (bSeg) ? pos1 - pos0 : vl_0;
		vec[2] = 0;
		if (vec != vl_0)
			normalise(vec);

		Vec3d	vecVert = Vec3d(vec[1], -vec[0], 0);
		vecVert *= (iWidVar != -1 ? Data.getVarFData(iWidVar)[iData] : 10.0) * m_LineSize / MetersPerDegree;
		vVerts[iCnt++] = getScenePos(pos0 + vecVert);
		vVerts[iCnt++] = getScenePos(pos0 - vecVert);
		vVerts[iCnt++] = getScenePos(pos1 + vecVert);
		vVerts[iCnt++] = getScenePos(pos1 - vecVert);
		pos0 = pos1;
	}

	//  create faces
	bool	bPrev = false;
	for (int i = 0, iData = iStart; i < iSize - 1; i++, iData++)
	{
		if (!(vClrVerts[4 * i] && vClrVerts[4 * i + 2] && pConData[iData] == 1))
		{
			bPrev = false;
			continue;
		}

		if (bPrev)
		{
			Object.addFace(4 * i - 1, 4 * i - 2, 4 * i + 1);
			Object.addFace(4 * i + 1, 4 * i - 2, 4 * i);
		}

		Object.addFace(4 * i + 1, 4 * i, 4 * i + 3);
		Object.addFace(4 * i + 3, 4 * i, 4 * i + 2);
		bPrev = true;
	}

	Object.setTriangleStrip(false);
	Object.setReprojected(false);
	Object.setLighted(false);
	Object.setDirty(true);
}

/* =============================================================================
    given a position cube return a 3d model for a slice of it
 =============================================================================== */
void CDataLayer::createShapeModel(int iTimeSlice)
{
	int iShapeVar = getData().findVar("shapes");
	if (iShapeVar == -1)
		return;

	editData().loadVar(iShapeVar);
	vector<float> vShapes = editData().getVarFData(iShapeVar);

	int cnt = m_DataPlane.size()*3;
	if (!editModel().createNewObject(cnt, cnt, cnt, cnt, 0))
		return;

	C3DObject		&Object = getModel().editObject(getModel().getObjectCnt() - 1);
	vector<Vec3f>	&vVerts = Object.editVerts();
	vector<Vec3f>	&vTexVerts = Object.editTexVerts();
	vector<Vec3f>	&vNormals = Object.editNormals();
	vector<color>	&vClrVerts = Object.editClrVerts();

	float			fNoVal = getData().getVarNoValue(m_CurVar);

	int iStart = 0, iCount = 0;
	for (int s = 0; s < vShapes.size(); s++, iStart += iCount)
	{
		iCount = vShapes[s];
		vector<Vec3d> vIn;
		vector<int> vOut;
		for (int i = iStart; i < iStart+iCount; i++)
		{
			color	clr = 0;
			float	dval = 0;
			if (m_DataPlane[i] != fNoVal)
			{
				dval = getGradOffset(m_DataPlane[i]);
				if (dval >= 0 && dval <= 1.0) clr = CLR_WHITE;
			}
			Vec3d pos = getDataNdxPos(i);
			vIn.push_back(pos);
			vVerts[i] = getScenePos(pos);
			vNormals[i] = Vec3f(0, 1, 0);
			vTexVerts[i] = Vec3f(dval, 0, 0);
			vClrVerts[i] = clr;
		}

		Triangulate::Process(vIn, vOut);

		//  create faces
		for (int i = 0; i < vOut.size(); i += 3)
			Object.addFace(vOut[i]+iStart, vOut[i+1]+iStart, vOut[i+2]+iStart);
	}

	Object.setTriangleStrip(false);
	Object.setReprojected(false);
	Object.setLighted(false);
	Object.setDirty(true);
}

/* =============================================================================
    INITIALIZE DATA FOR VIEWING set position of data points based on display time
 =============================================================================== */
void CDataLayer::initData(double dTime, double dFrameTime)
{
	if (m_CurVar == -1) {
		return;
	}
	//  convert from time in data set
	dTime = getDataTimefromWorld(dTime);

	if (getCycleData() && getCycleDataDur() > 0 && !getShowParticles())
	{
		double	offset = fmod(dTime - m_Start, getCycleDataDur());
		dTime = offset >= 0 ? m_Start + offset : m_Start + getCycleDataDur() + offset;
	}

	//  no change in time, so reshow current data
	if (!m_Dirty && m_Time == dTime && m_FrameTime == dFrameTime) {
		return;
	}
	//  update the layer time
	m_FrameTime = dFrameTime;
	m_Time = dTime;

	//  check if need to load a new file for a multi-file set
	static bool bResetFile = false;
	if (bResetFile)
		return;
	if (getMultiFile() && m_MultiFileData.size())
	{
		int iNewFile = 0;
		for (int i = 1; i < m_MultiFileData.size(); i++)
		{
			if (m_MultiFileData[i].dMinTime > m_Time)
				break;
			iNewFile++;
		}

		if (iNewFile != m_MultiFileCur)	//  load new file - (create list at activate by loading/discarding)
		{
			bResetFile = true;
			if (getShareFile() && getSharedDataLayer(getLink()))
			{
				CDataLayer	*pShareLayer = getSharedDataLayer(getLink());
				m_DataPtr = pShareLayer->getDataPtr();
				if (pShareLayer->m_MultiFileCur == iNewFile) //  new file has been reset by shared layer
					bResetFile = false;
			}

			if (bResetFile)
			{
				//  clean out old file
				int	iCurVar = m_CurVar;
				CDataPtr hOldData = getDataPtr();
				m_DataPtr = new CData();
				m_DataPtr->setTimeFormat(getTimeFormat());

				editData().setBSPTree(hOldData);
				hOldData->setUseCount(hOldData->getUseCount() - 1);
				if (hOldData->getUseCount() == 0) {
					delete hOldData;
				}

				//  load the new file
				string	strLocal = m_MultiFileData[iNewFile].strPath;
				cout << (" - updating multifile data to " + strLocal);

				bool	bSuccess = false;
				if (getExt(strLocal) == ".csv" || getExt(strLocal) == ".txt")
					bSuccess = editData().readCSV(strLocal);
				else
					bSuccess = editData().readNetCDF(strLocal);

				if (false)
				{
					m_DataPtr = NULL;
					cout << ("ERROR: unable to load multifile data");
				}
				else
				{
					editData().setUseCount(1);
					editData().sortTime();

					//  set the curvar to make sure data is loaded
					m_CurVar = -1;
					setCurVar(iCurVar);

					//  set indicators to correct file
					m_MultiFileCur = iNewFile;
					setLocalFilePath(strLocal);
					setDirty();
					setPlanesDirty();
				}
			}
		}
	}

	if (!getDataLoaded()) {
		return;
	}

	editData().setCoordVars(m_CurVar);

	int	iFormat = getData().getDataFormat();

	// create particle data set and just replace it in getData if model and showing line
	CDataPtr curData = NULL;
	int		curVar = -1;

	if (m_ParticleData && m_ParticlesDirty && m_ParticlesRebuild)
	{
		delete m_ParticleData;
		m_ParticleData = NULL;
	}

	if (getShowParticles())
	{
		if (m_ParticleData == NULL || m_ParticlesDirty || bResetFile)
		{
			bool	bMulti = bResetFile;
			if (m_ParticleData == NULL)
			{
				m_ParticleData = new CData();
				bMulti = false;
			}

			//  set up vars to create particle data based on model
			vector<int> iFlowVars;
			iFlowVars.push_back(m_CurVar + 1);
			iFlowVars.push_back(m_CurVar);
			if (getShowVectorDims() == 3)
				iFlowVars.push_back(m_CurVar + 2);

			vector<double> time(2);
			time[0] = m_SelStart;
			time[1] = m_SelFinish;

			dtime	cycle = getCycleData() ? getCycleDataDur() : NO_TIME;

			if (m_ParticlesRebuild || bResetFile)
				m_ParticleUpdateList = m_ParticlePosList;

			vector<double>	vpos;
			for (int i = 0; i < m_ParticleUpdateList.size(); i++)
				for (int j = 0; j < 3; j++)
					vpos.push_back(m_ParticleUpdateList[i][j]);

			m_ParticleData->createParticleData(editData(),iFlowVars,vpos,time,cycle,
					m_ParticleStep,	m_ParticleValue,m_ParticleForward,m_ParticleTimeRelease,bMulti);
			m_ParticleUpdateList.resize(0);
			m_ParticleUpdateList.resize(m_ParticlePosList.size(), Vec3d(NODATA, NODATA, NODATA));
			m_ParticlesDirty = false;
			m_ParticlesRebuild = false;
		}

		curData = m_DataPtr;
		m_DataPtr = m_ParticleData;

		//  set current var in the particle data
		curVar = m_CurVar;
		m_CurVar = -1;
		if (m_DataPtr->getVarCnt() > 4)
		{
			setCurVar(4);
			iFormat = DFORMAT_PATH;
		}
	}

	//  create custom plane
	if (m_PlanesDirty && m_PlaneData)
	{
		delete m_PlaneData;
		m_PlaneData = NULL;
	}

	if (getShowCustomPlane() || getShowVertSect() || (getShowPlanes() && iFormat == DFORMAT_2DSURFACE))
	{
		if (m_PlaneData == NULL || m_PlanesDirty)
		{
			m_PlaneData = new CData();

			if (iFormat == DFORMAT_PATH)
			{
				m_PlaneData->createVertSectData(getData(), m_CurVar);
			}
			else
			{

				//  set up vars to create new plane based on model
				int				iTimeVar = getData().getTimeVar();
				vector<double>	time;
				if (iTimeVar == -1)
					time.push_back(0);
				else
					time = getData().getVar(iTimeVar).getDData();

				vector<int>		vDims = getData().getVarDims(m_CurVar);
				int				iLevels = vDims.size() < 3 ? 1 : getData().getDimSize(vDims[vDims.size() - 3]);
				int				iYCoord = getData().getDimSize(vDims[vDims.size() - 2]);
				int				iXCoord = getData().getDimSize(vDims[vDims.size() - 1]);
				vector<double>	lat, lon;
				vector<float>	depth;
				if (m_PlanesType == PLANES_DEPTH || iFormat == DFORMAT_2DSURFACE)
				{
					int iSize = iXCoord * iYCoord;
					lat.resize(iSize);
					lon.resize(iSize);
					depth.resize(iSize);
					for (int i = 0; i < iXCoord * iYCoord; i++)
					{
						Vec3d	pos = getDataNdxPos(i);
						lat[i] = pos[0];
						lon[i] = pos[1];
						depth[i] = -pos[2];
						if (iFormat != DFORMAT_2DSURFACE)
							depth[i] = getPlanesPresetPos(PLANES_DEPTH);
					}

					iLevels = 1;
				}
				else
				{
					vector<double>	dpos = getData().getPosMinMax();
					Vec3d			setpos = getPlanesPresetPos();
					vector<Vec3d>	poslist;
					if (m_PlanesType == PLANES_LAT)
					{
						poslist.push_back(Vec3d(setpos[0], dpos[1], 0));
						poslist.push_back(Vec3d(setpos[0], dpos[4], 0));
					}
					else if (m_PlanesType == PLANES_LON)
					{
						poslist.push_back(Vec3d(dpos[0], setpos[1], 0));
						poslist.push_back(Vec3d(dpos[3], setpos[1], 0));
					}
					else if (m_PlanesCustomType == PLANES_CUSTOM_VSLICE)
						poslist = getPlanesVertSliceList();
					else
					{

						//  fix up time
						double	t_inc = (m_SelFinish - m_SelStart) / 99;
						time.resize(100);
						for (int i = 0; i < 100; i++)
							time[i] = m_SelStart + t_inc * (double) i;
						if (getShowTimeSlice())
							poslist.push_back(getPlanesTimeSlicePos());
						else
						{
							poslist = getPlanesTimePlotList();
							iLevels = 1;
						}
					}

					int iPlanes = poslist.size() - 1;
					int iSteps = max(50, 100 - 5 * iPlanes);
					int iTotCols = getShowTimeSlice() || getShowTimePlot() ? poslist.size() : iPlanes * iSteps;
					lat.resize(iTotCols);
					lon.resize(iTotCols);
					depth.resize(iTotCols * iLevels);

					if (getShowTimeSlice())
					{
						Vec3d	pos = poslist[0];
						g_Draw.setTerrainHeight(pos);
						lat[0] = pos[0];
						lon[0] = pos[1];
						depth[iLevels - 1] = -pos[2];
					}
					else if (getShowTimePlot())
					{
						for (int p = 0; p < iTotCols; p++)
						{
							lat[p] = poslist[p][0];
							lon[p] = poslist[p][1];
							depth[p] = poslist[p][2];
						}
					}
					else
					{
						for (int p = 0; p < iPlanes; p++)
						{
							double	latInc = (poslist[p + 1][0] - poslist[p][0]) / (double) (iSteps - 1);
							double	lonInc = (poslist[p + 1][1] - poslist[p][1]) / (double) (iSteps - 1);
							for (int i = 0; i < iSteps; i++)
							{
								Vec3d	pos = Vec3d(poslist[p][0] + (double) i * latInc,
													poslist[p][1] + (double) i * lonInc,
													0);
								g_Draw.setTerrainHeight(pos);
								lat[p * iSteps + i] = pos[0];
								lon[p * iSteps + i] = pos[1];
								depth[iTotCols * (iLevels - 1) + p * iSteps + i] = -pos[2];
							}
						}
					}

					//  fill in depths
					for (int i = 0; i < iTotCols; i++)
					{
						float	fDepth = depth[iTotCols * (iLevels - 1) + i];
						float	depthInc = max(0.0f, fDepth) / (float) (iLevels - 1);
						for (int j = 0; j < iLevels - 1; j++)
							depth[iTotCols * j + i] = (float) j * depthInc;
					}

					iXCoord = iTotCols;
					iYCoord = 1;
				}

				if (m_PlaneData->createModelGrid(time, iYCoord, iXCoord, iLevels, lat, lon, depth))
				{
					vDims.clear();
					for (int i = 0; i < 4; i++)
						vDims.push_back(i);
					m_PlaneData->newVar(getData().getVarName(m_CurVar), DATA_SCALAR, vDims);
					if (iFormat == DFORMAT_2DSURFACE)
						m_PlaneData->setVarFData(4, editData().getVarFData(m_CurVar));
					else
						m_PlaneData->sampleModel(editData(), m_CurVar, 4);
				}
			}

			m_PlanesDirty = false;
		}

		curData = m_DataPtr;
		m_DataPtr = m_PlaneData;

		//  set current var in the particle data
		curVar = m_CurVar;
		m_CurVar = 4;	//  set directly to avoid resetting min and max
		if (getShowTimeSlice() || getShowTimePlot()) {
			iFormat = DFORMAT_PATH;
		}
	}

	int		iPntCur = -1;
	double	dLead = NO_TIME, dTrail = NO_TIME;
	int		iPntLead = -1, iPntTrail = -1;
	int		iStartPnt = 0, iEndPnt = 0;
	bool	bTimeSlice = false;
	int		iSize = -1, iCurSize = -1;
	int		*pFilterNdxList = NULL;

	bool	bStorePnts = (getShowPoints() || getShowLines() || getShowVectors());

	editData().setCoordVars(m_CurVar);

	int			iDimCnt = 0;
	vector<int> vDims = getData().getVarDims(m_CurVar);
	for (int i = 0; i < vDims.size(); i++)
		if (getData().getDimSize(vDims[i]) > 1)
			iDimCnt++;

	//check to see if we have any data to show
	bool	bNoData =(getData().getVarCnt() == 0
						||	m_CurVar == -1
						||	getData().getLatVar() == -1
						||	getData().getLonVar() == -1);

	//  check if current time is in this data's window
	bNoData |= ((m_Time < m_SelStart || m_Time > m_SelFinish) && m_SelStart != m_SelFinish);
	if (bNoData)
	{
		m_DisplayCnt = 0;
		m_DisplayCur = -1;
		goto cleanup;
	}
	//BDC temporary test here...
	m_Lead = NO_TIME;
	//m_Trail = 1279667;
	m_Trail = 12796674;
	//  figure out cur point location window based on time
	dLead = m_Lead == NO_TIME ? m_Time : m_Time - m_Lead;
	dTrail = m_Trail == NO_TIME ? m_Time : m_Time + m_Trail;
	iPntLead = m_Lead == -1 ? 0 : -1;
	iPntTrail = m_Trail == -1 ? getData().getTimeSteps() : -1;
	for (int i = getData().getTimeSteps() - 1; i != 0; i--)
	{
		double	dTime = getData().getDataTime(i);
		if (iPntTrail == -1 && dTime <= dTrail)
			iPntTrail = i;
		if (iPntCur == -1 && dTime <= m_Time)
			iPntCur = i;
		if (iPntLead == -1 && dTime <= dLead)
		{
			iPntLead = i;
			break;
		}
	}

	iPntTrail = max(0, iPntTrail);
	iPntCur = max(0, iPntCur);
	iPntLead = max(0, iPntLead);

	//  check if at same data point as last time
	if (!m_Dirty && iPntCur == m_LastPntCur)
		goto cleanup;

	//  update the display buffer size if necessary
	m_DisplayCnt = 0;
	m_DisplayCur = -1;
	bTimeSlice = (iFormat == DFORMAT_SHAPES || iFormat == DFORMAT_STREAMNETWORK || iFormat == DFORMAT_2DSURFACE || iFormat == DFORMAT_3DMODEL);
	iSize = bTimeSlice ? getData().getTimeSliceSize(m_CurVar) : getData().getVarSize(m_CurVar);
	iCurSize = m_DisplayNdx.size();

	bStorePnts = (getShowPoints() || getShowLines() || getShowVectors() || getShowTimeSlice() || getShowTimePlot());

	if (bStorePnts && (iSize > iCurSize || iSize < iCurSize - 0x10000))
	{
		int iNewSize = ((iSize / 1024) + 1) * 1024;
		try
		{
			m_DisplayNdx.clear();
			m_DisplayNdx.resize(iNewSize);
		}
		catch(std::bad_alloc const &)
		{
			cout << "Memory allocation failure!" << endl;
			return;
		}
	}

	if (m_Dirty || iDimCnt > 1 || m_LastPntCur == -1)
	{

		//  allocate and load data
		if (iSize > m_DataPlane.size())
		{
			try
			{
				m_DataPlane.clear();
				m_DataPlane.resize(iSize);
			}
			catch(std::bad_alloc const &)
			{
				cout << "Memory allocation failure!" << endl;
				return;
			}
		}

		int iSlice = bTimeSlice ? iPntCur : -1;
		editData().getTimeSlice(m_CurVar, iSlice, m_DataPlane);

		//  allocate and load vector data
		if (getShowVectors())
		{
			try
			{
				if (iSize > m_DataPlane2.size())
				{
					m_DataPlane2.clear();
					m_DataPlane2.resize(iSize);
				}

				if (getShowVectorDims() == 3 && iSize > m_DataPlane3.size())
				{
					m_DataPlane3.clear();
					m_DataPlane3.resize(iSize);
				}
			}
			catch(std::bad_alloc const &)
			{
				cout << "Memory allocation failure!" << endl;
				return;
			}

			int iVar2 = min(m_CurVar + 1, getData().getVarCnt() - 1);
			if (getData().getVarSize(iVar2) != getData().getVarSize(m_CurVar))
				iVar2 = m_CurVar;
			editData().getTimeSlice(iVar2, iSlice, m_DataPlane2);
			if (getShowVectorDims() == 3)
			{
				int iVar3 = min(iVar2 + 1, getData().getVarCnt() - 1);
				if (getData().getVarSize(iVar3) != getData().getVarSize(m_CurVar))
					iVar3 = iVar2;
				editData().getTimeSlice(iVar3, iSlice, m_DataPlane3);
			}
		}
		else
		{
			m_DataPlane2.clear();
			std::vector<float> ().swap(m_DataPlane2);
			m_DataPlane3.clear();
			std::vector<float> ().swap(m_DataPlane3);
		}
	}

	//  reset start and point count based on display type
	iStartPnt = 0;
	iEndPnt = iSize;
	m_LineStride = 1;
	editModel().clean();
	editPlotModel().clean();

	if (iFormat == DFORMAT_PATH)
	{
		if (getData().getTimeSteps() > 1)
		{
			m_LineStride = getData().getTimeSliceSize(m_CurVar);
			if (iFormat == DFORMAT_STREAMNETWORK || iFormat == DFORMAT_SHAPES)
			{
				if (bTimeSlice)
					iPntCur = 0;
				iPntLead = iPntTrail = iPntCur;
			}

			iStartPnt = (iPntLead) * m_LineStride;
			iEndPnt = min((iPntTrail + 1) * m_LineStride, iSize);
		}
	}

	//  vertical section on an auv or ctd set
	if (getShowVertSect())
	{
		int iRows = max(1, iPntTrail - iPntLead);
		int iCols = getData().getDimSize(1);
		vector<float> vSlice(iRows * iCols);
		for (int i = iPntLead, ndx = 0; i < iPntTrail; i++)
			for (int j = 0; j < iCols; j++, ndx++)
				vSlice[ndx] = i * iCols + j;
		if (m_PlanesWindow)
			createSliceModel(SLICE_VSECT, vSlice, iRows, iCols, true, m_PlanesShade);
		createSliceModel(SLICE_VSECT, vSlice, iRows, iCols, false, m_PlanesShade);
		iEndPnt = 0;
	}

	//  stream networks
	if (getShowPlanes() && iFormat == DFORMAT_STREAMNETWORK)
	{
		createStreamModel(iPntCur);
		iEndPnt = 0;
	}

	//  shape list
	if (getShowPlanes() && iFormat == DFORMAT_SHAPES)
	{
		createShapeModel(iPntCur);
		iEndPnt = 0;
	}
	//  iso surfaces
	if (getShowIsoSurfaces())
	{
		for (int i = 0; i < 3; i++)
		{
			if (!m_IsoValueActive[i]) {
				continue;
			}
			m_IsoValue[i] = min(m_CurVarMax, max(m_CurVarMin, m_IsoValue[i]));
			CreateIsoSurfaceModel(m_IsoValue[i], m_IsoShadeFlip);
		}
	}

	//  show 2d surface
	if (getShowPlanes() && iFormat == DFORMAT_2DSURFACE)
	{
		int iRows, iCols;
		vector<float> vSlice;
		if (createSliceNdxList(0, 0, vSlice, iRows, iCols))
		{
			if (m_PlanesWindow)
				createSliceModel(SLICE_DEPTH, vSlice, iRows, iCols, true, m_PlanesShade);
			createSliceModel(SLICE_DEPTH, vSlice, iRows, iCols, false, m_PlanesShade);
		}
	}

	//  show custom planes
	if (getShowCustomPlane())
	{
		if (getShowTimeSlice())
		{
			int iRows = getData().getDimSize(0);
			int iCols = getData().getDimSize(1);
			vector<float> vSlice(iRows * iCols);
			for (int i = 0; i < iRows * iCols; i++) {
				vSlice[i] = i;
			}
			createSliceModel(SLICE_VSECT, vSlice, iRows, iCols, true, m_PlanesShade);
			createSliceModel(SLICE_VSECT, vSlice, iRows, iCols, false, m_PlanesShade);
			iStartPnt = iPntCur * m_LineStride;
			iEndPnt = (iPntCur + 1) * m_LineStride;
			m_LineStride = 1;
		}
		else if (getShowVertSlice())
		{
			int iRows, iCols;
			vector<float> vSlice;
			int iSliceType = m_PlanesType == PLANES_DEPTH ? SLICE_DEPTH : SLICE_LAT;
			if (createSliceNdxList(iSliceType, 0, vSlice, iRows, iCols))
			{
				if (m_PlanesWindow)
					createSliceModel(iSliceType, vSlice, iRows, iCols, true, m_PlanesShade);
				createSliceModel(iSliceType, vSlice, iRows, iCols, false, m_PlanesShade);
			}
		}
	}

	//  figure out slice data to show from filter
	if (iFormat == DFORMAT_3DMODEL && (getShowModelPlanes() || getShowPoints() || getShowVectors()))
	{
		vector<vector<float> *> vNdxVec;
		int iTotal = 0;

		for (int iSliceType = 0; iSliceType < 3; iSliceType++)	//  slice types 0, 1, 2
		{
			if (!m_ViewDimActive[iSliceType])
				continue;

			int iStart = m_ViewDimGrid[iSliceType] ? 0 : m_ViewPlaneValue[iSliceType];
			int iInc = m_ViewDimGrid[iSliceType] ? m_ViewGridValue[iSliceType] : 1;
			int iEnd = m_ViewDimGrid[iSliceType] ? getViewDimMax(iSliceType) : iStart + 1;
			for (int iSlice = iStart; iSlice < iEnd; iSlice += iInc)
			{
				int iRows, iCols;
				vector<float> *pvSlice = new vector<float>;
				if (!createSliceNdxList(iSliceType, iSlice, *pvSlice, iRows, iCols))
					continue;
				if (getShowPoints() || getShowVectors())
				{
					vNdxVec.push_back(pvSlice);
					iTotal += (int) pvSlice->size();
				}
				else
				{
					if (m_PlanesWindow && getModel().getObjectCnt() == 0)
						createSliceModel(iSliceType, *pvSlice, iRows, iCols, true, m_PlanesShade);
					createSliceModel(iSliceType, *pvSlice, iRows, iCols, false, m_PlanesShade);
					delete pvSlice;
					if (iSliceType != SLICE_DEPTH)
					{
						C3DObject	&object = getModel().editObject(getModel().getObjectCnt() - 1);
						object.setName(iSliceType == SLICE_LAT ? "1" : "2");
						if (iSliceType == SLICE_LAT)
							object.setPlaneDir(object.getPlaneDir() * -1);
					}
				}
			}
		}

		iEndPnt = 0;
		if (iTotal > 0 && (getShowPoints() || getShowVectors()))
		{

			//  create list of points to show
			if (!mem_alloc(pFilterNdxList, iTotal))
				return;

			int *pTmp = pFilterNdxList;
			for (int i = 0; i < vNdxVec.size(); i++)
			{
				int size = (int) vNdxVec[i]->size();
				float	*pf = &(*vNdxVec[i])[0];
				for (int j = 0; j < size; j++)
					*pTmp++ = *pf++;
			}

			iEndPnt = iTotal;
		}
	}

	//  put point data in the display buffer
	if (bStorePnts)
	{
		int dim1 = getData().getDimSize(vDims[vDims.size() - 1]);
		int dim2 = (vDims.size() >= 2) ? getData().getDimSize(vDims[vDims.size() - 2]) : 1;
		m_ScnPlane.resize(iEndPnt - iStartPnt);
		m_ScnPlane2.resize(getShowVectors() ? iEndPnt - iStartPnt : 0);
		if (m_ImageVar != -1)
			m_ImagePathNew = "";

		float fNoVal = getData().getVarNoValue(m_CurVar);
		for (int i = iStartPnt; i < iEndPnt; i++)
		{
			int ndx = pFilterNdxList ? pFilterNdxList[i] : i;
			Vec3d gps_pos = getDataNdxPos(ndx);
			if ((m_DataPlane[ndx] == fNoVal)
					|| gps_pos == Vec3d(NODATA, NODATA, NODATA)
					|| (getShowVectors() && (m_DataPlane2[ndx] == fNoVal || (getShowVectorDims() == 3 && m_DataPlane3[ndx] == fNoVal)))
					|| getCropped(gps_pos)
					|| getStrideFilter(ndx, dim1, dim2))
				m_DataPlane[ndx] = NODATA;
			else
			{
				if (m_DisplayCur == -1 && ndx >= iPntCur)
				{
					m_DisplayCur = m_DisplayCnt;
					m_IndexCur = ndx;
				}

				if (m_ImageVar != -1 && ndx <= m_IndexCur)
				{
					int image_idx = getData().getVar(m_ImageVar).getEnumValue(ndx);
					if (image_idx != -1)
						m_ImagePathNew = getData().getVar(m_ImageVar).getEnumString(image_idx);
				}

				if (getShowVectors())
				{
					//  figure out flow vector
					Vec3d vecd = Vec3d(m_DataPlane2[ndx], m_DataPlane[ndx], getShowVectorDims() == 3 ? m_DataPlane3[ndx] : 0);
					Vec3d gps_vec = gps_pos;
					if (len(vecd) > 5.0)
						m_DataPlane[ndx] = (float) NODATA;
					else
					{
						m_DataPlane[ndx] = len(vecd);
						gps_vec = gps_pos + vecd * .05 * getVectorLength();
					}

					m_ScnPlane2[m_DisplayCnt] = getScenePos(gps_vec);
				}

				m_ScnPlane[m_DisplayCnt] = getScenePos(gps_pos);
			}

			m_DisplayNdx[m_DisplayCnt] = ndx;
			m_DisplayCnt++;
		}
	}

	if (pFilterNdxList) delete[] pFilterNdxList;

cleanup:
	bResetFile = false;
	m_Dirty = false;
	m_LastPntCur = iPntCur;
	if (iFormat == DFORMAT_PATH && !getShowParticles())
		setAttachData(iPntCur);
	editModel().setTransparency(getTransparency());
	if (m_DisplayCnt == 0)
	{
		m_ScnPlane.clear();
		std::vector<Vec3f> ().swap(m_ScnPlane);
		m_ScnPlane2.clear();
		std::vector<Vec3f> ().swap(m_ScnPlane2);
	}

	if (curData)
	{
		m_DataPtr = curData;
		m_CurVar = -1;
		setCurVar(curVar);
	}
}

/* =============================================================================
    DISPLAY DATA
 =============================================================================== */
void CDataLayer::RenderData(double dSpeed, int iSelectID)
{
	if (m_DataPtr == NULL || m_CurVar == -1)
		return;

	if ((iSelectID == -1 || getShowPoints()) && m_DisplayCnt != 0)
	{
		//glEnable(GL_DEPTH_TEST);
		//glDisable(GL_LIGHTING);
		//glColor4f(1, 1, 1, 1);

		//  draw lines
		if (getShowLines() || getShowTimeSlice())
		{
			CDataPtr curData = NULL;
			int curVar = -1;
			double dTimeSteps = m_DisplayCnt / m_LineStride;
			double dInc = 1 / (dTimeSteps - 1);
			int iFadeStep = 0;
			double dFadeInc = 0;
			if (getShowParticles() && m_ParticleData)
			{
				curData = m_DataPtr;
				m_DataPtr = m_ParticleData;
				curVar = m_CurVar;
				m_CurVar = 4;
				setCurVarMinMax();
				if (m_ParticleTimeRelease)
				{
					iFadeStep = m_ParticlePosList.size();

					double dTotTimeSteps = getData().getDimSize(0);
					double dParticleCnt = getData().getDimCnt() == 2 ? getData().getDimSize(1) : 1;
					dFadeInc = 1.0 / (dParticleCnt / iFadeStep);
					dFadeInc *= (dTotTimeSteps / dTimeSteps);
				}
			}

			int iSize = getShowTimeSlice() ? 3 : m_LineSize;
			glLineWidth(iSize);
			glPointSize(iSize);
			glBegin(GL_LINES);
			for (int s = 0; s < m_LineStride; s++)			//  each particle
			{
				for (int i = dTimeSteps - 2; i >= 0; i--)	//  each timestep
				{
					int ndx0 = s + i * m_LineStride;
					int ndx1 = ndx0 + m_LineStride;
					if (	ISNODATA(m_DataPlane[m_DisplayNdx[ndx0]])
						||	(ndx1 < m_DisplayCnt && ISNODATA(m_DataPlane[m_DisplayNdx[ndx1]])))
						continue;

					double dTrans = !m_FadeLines ? 1.0 : iFadeStep ? (s / iFadeStep) *
						dFadeInc : (dInc * ndx0) /
						(double) m_LineStride;
					setPointColor(ndx0, iSelectID, dTrans);
					glVertex(m_ScnPlane[ndx0]);
					setPointColor(ndx1, iSelectID, dTrans + dInc);
					glVertex(m_ScnPlane[ndx1]);
				}
			}

			if (m_LineStride == 1 && m_Trail == 0 && !ISNODATA(m_DataPlane[m_DisplayNdx[m_DisplayCnt - 1]]))
			{
				setPointColor(m_DisplayCnt - 1, iSelectID, 1.0);
				glVertex(m_ScnPlane[m_DisplayCnt - 1]);
				glVertex(m_LastInterpPos);
			}

			glEnd();

			//  to create rounded corners
			glBegin(GL_POINTS);
			for (int s = 0; s < m_LineStride; s++)			//  each particle
			{
				for (int i = dTimeSteps - 2; i >= 0; i--)	//  each timestep
				{
					int ndx0 = s + i * m_LineStride;
					if (ISNODATA(m_DataPlane[m_DisplayNdx[ndx0]]))
						continue;

					double dTrans = !m_FadeLines ? 1.0 : iFadeStep ? (s / iFadeStep) *
						dFadeInc : (dInc * ndx0) /
						(double) m_LineStride;
					setPointColor(ndx0, iSelectID, dTrans);
					glVertex(m_ScnPlane[ndx0]);
				}
			}

			glEnd();

			//  put larger point on end of particles lines
			if (m_LineStride > 1)
			{
				glPointSize(m_LineSize * 2);
				glBegin(GL_POINTS);
				for (int s = 0; s < m_LineStride; s++)		//  each particle
				{
					int ndx0 = s + (dTimeSteps - 1) * m_LineStride;
					if (ISNODATA(m_DataPlane[m_DisplayNdx[ndx0]]))
						continue;

					double dTrans = !m_FadeLines ? 1.0 : iFadeStep ? (s / iFadeStep) *
						dFadeInc : (dInc * ndx0) /
						(double) m_LineStride;
					setPointColor(ndx0, iSelectID, dTrans);
					glVertex(m_ScnPlane[ndx0]);
				}

				glEnd();
			}

			if (curData) {
				m_DataPtr = curData;
				m_CurVar = curVar;
				setCurVarMinMax();
			}
		}

		//  draw arrows
		if (getShowVectors()) {
			glLineWidth(m_VectorWidth);

			float size_factor = .001;

			for (int i = 0; i < m_DisplayCnt; i++)
			{
				if (ISNODATA(m_DataPlane[m_DisplayNdx[i]]))
					continue;

				setPointColor(i, iSelectID);

				if (!m_ShowArrows)
				{
					glBegin(GL_LINES);
					glVertex(m_ScnPlane2[i]);
					setPointColor(i, iSelectID, 0.0);
					glVertex(m_ScnPlane[i]);
					glEnd();
					continue;
				}

				if (iSelectID == -1) glEnable(GL_LIGHTING);

				drawArrow(m_ScnPlane[i], m_ScnPlane2[i], size_factor * getVectorWidth());
			}
		}

		if (getShowTimePlot()) {
			glEnable(GL_LIGHTING);
			for (int i = 0; i < m_LineStride; i++)	//  each point
			{
				color clr = getPlotColor(i, m_LineStride);
				glColor4ub(clrR(clr), clrG(clr), clrB(clr), 255.0);
				glPushMatrix();

				Vec3f pos = m_ScnPlane[i];
				glTranslatef(pos[0], pos[1], pos[2]);

				float fsize = 0.02;
				glScalef(fsize, fsize, fsize);
				drawPrimType(2);
				glPopMatrix();
			}
		}

		//  draw points
		if (getShowPoints()) {
			bool bGlyph = (m_PrimType != 1);
			if (bGlyph && iSelectID == -1) {
				glEnable(GL_LIGHTING);
			}
			if (!bGlyph) {
				glBegin(GL_POINTS);
			}

			Vec3d baseSize = m_ScaleValue * m_PointSize;
			double dLastSize = -1;
			m_MagnifyCurrent = true;
			m_Time = g_World.getTimeLine().getTime();

			for (int i = 0; i < m_DisplayCnt; i++)
			{
				if (ISNODATA(m_DataPlane[m_DisplayNdx[i]])) {
					continue;
				}
				float dval = getGradOffset(m_DataPlane[m_DisplayNdx[i]]);
				if (dval < 0 || dval > 1.0) {
					continue;
				}
				Vec3d size = baseSize;
				for (int b = 0; b < 3; b++) {
					//if (m_ShowScaleSize[b]) {
						size[b] *= dval;
					//}
				}
				int ndx = m_IndexCur - (m_DisplayCur - i);
				if (m_MagnifyCurrent &&
					//m_DisplayCur != -1 && i <= m_DisplayCur &&
					(m_Time - dSpeed) > getData().getDataTime(ndx))
				{
					double dFade = 5.0f - max(1.0, min(4.0, (double) (m_Time - getData().getDataTime(ndx)) / (dSpeed)));
					//double dFade = ((rand()%100)/100.0);
					size *= 0.4 * dFade;
				} else { //Hasn't errupted yet in timeline
					size *= 0.2;
				}
				setPointColor(i, iSelectID);
				Vec3f pos = m_ScnPlane[i];

				if (bGlyph)
				{
					glPushMatrix();
					glTranslatef(pos[0], 0.08+pos[1]*1000.0, pos[2]);
					Vec3d rot = getSceneRot(pos);
					glRotatef(rot[1], 0, 1, 0);
					glRotatef(rot[2], 0, 0, 1);
					glRotatef(getObjectRot(), 0, 1, 0);
					size *=  0.10;
					glScalef(size[0], size[2], size[1]);
					drawPrimType(m_PrimType);
					glPopMatrix();
				} else {
					if (size[0] != dLastSize) {
						glEnd();
						dLastSize = size[0];
						glPointSize(max(1.0, size[0] * 7.0));
						glBegin(GL_POINTS);
					}
					glVertex3d(pos[0], pos[1], pos[2]);
				}
			}
			if (!bGlyph)
				glEnd();
		}
	}

	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);

	if (getLineEdit())
	{
		m_LineEditLine.setSelectID(iSelectID);
		m_LineEditLine.Render();
	}

	if (getPointEdit())
	{
		for (int i = 0; i < m_PointEditObjects.size(); i++)
		{
			m_PointEditObjects[i].setSelectID(iSelectID == -1 ? -1 : i);
			m_PointEditObjects[i].Render(false);
		}
	}

	if (iSelectID == -1)
	{
		//glEnable(GL_DEPTH_TEST);
		if (m_DataWinPtr) ((CWindow *)
			m_DataWinPtr)->setShow(false);

		//  draw models
		if (getModel().getObjectCnt() > 0)
		{
			int txID = getGradient().getTexID();
			if (getModel().getMaterialCnt() == 0)
				editModel().addMaterial("surface", txID, 0, 0, CLR_WHITE, true);
			else
				getModel().editMaterial(0).setTextureId(txID);

			CMaterialPtr pMat = getModel().getMaterialPtr(0);
			for (int i = 0; i < getModel().getObjectCnt(); i++)
				getModel().editObject(i).setMaterial(pMat);

			//  draw to a window
			glDisable(GL_CULL_FACE);
			if (getShowPlanes() && getPlanesWindow())
				renderPlotWindow();
			if (getShowPlanes())
				editModel().sortPlaneObjects(g_Draw.getCameraPositionGPS() - g_Draw.getLookAtGPS());
			glEnable(GL_LIGHTING);
			getModel().Render();
			glDisable(GL_LIGHTING);
			glEnable(GL_CULL_FACE);
		}

		if (getShowTimePlot()) {
			renderPlotWindow();
		}

		//  draw attached objects
		glEnable(GL_LIGHTING);
		if (getAttachObjAttached() && getAttachObjPtr())
		{
			getAttachObj().setHide(false);
			getAttachObj().Render(false);
		}

		//  draw video
		if (updateVideoTexture())
		{
			if (getAttachVideoWindow())
			{
				glDisable(GL_LIGHTING);
				glDisable(GL_DEPTH_TEST);
				g_Draw.enterOrtho2D();

				CWindow *pWindow = (CWindow *) getDataWin();
				pWindow->setTexWindow(0.0, 1.0, 0.0, 1.0);
				pWindow->draw();
				g_Draw.exitOrtho2D();
			}
			else
				drawVideo();
		}
	}

	//glEnable(GL_DEPTH_TEST);
	//glEnable(GL_LIGHTING);
}

/* =============================================================================
 =============================================================================== */
color CDataLayer::getPlotColor(int i, int icnt)
{
	static CGradient	*pGrad = NULL;
	if (!pGrad)
	{
		pGrad = new CGradient();
		pGrad->addFile("system/gradients/rainbow_1.bmp", 0, 1.0);
		pGrad->updateMap();
		pGrad->createGradBitmap(NULL, false, false, false, false, false);
	}

	return pGrad->getColor((double) i / (double) (icnt - 1));
}

/* =============================================================================
 =============================================================================== */
void CDataLayer::renderPlotWindow()
{
	glDisable(GL_LIGHTING);
	g_Draw.enterOrtho2D();

	CWindow *pWindow = (CWindow *) getDataWin();
	pWindow->setShow(true);
	pWindow->setTexWindow(0.0, 0.0, 0.0, 0.0);
	pWindow->draw();
	glPushMatrix();

	int w_x, w_y, w_w, w_h;
	pWindow->getPos(w_x, w_y, w_w, w_h);
	glTranslatef(w_x + 10, g_Draw.getHeight() - (w_y + w_h - 10), 0);
	glScalef(w_w - 60, w_h - 20, 0);

	float fmin = 0.0, fmax = 1.0;
	const char	*pchFmt = "%.0lf";

	if (!getShowTimePlot())	//  show created slice
	{
		if (getPlotModel().getMaterialCnt() == 0)
			editPlotModel().addMaterial("surface", getGradient().getTexID(), 0, 0, CLR_WHITE, true);
		getPlotModel().editObject(0).setMaterial(getPlotModel().getMaterialPtr(0));
		getPlotModel().Render();

		Vec3d vmin = getPlaneMin();
		Vec3d vmax = getPlaneMax();
		if (vmin[2] == vmax[2])
		{
			fmin = vmin[0];
			fmax = vmax[0];
			pchFmt = "%.2lf";
		}
		else
		{
			fmin = vmin[2];
			fmax = vmax[2];
		}

		glColor4f(0.75, 0.75, 0.75, 0.1);
	}
	else	//  plot lines from data
	{
		glColor4f(0.9, 0.9, 0.9, 1.0);
		glBegin(GL_QUADS);
		glVertex2f(0.0, 0.0);
		glVertex2f(1.0, 0.0);
		glVertex2f(1.0, 1.0);
		glVertex2f(0.0, 1.0);
		glEnd();
		glLineWidth(2);

		double dTimeSteps = m_DisplayCnt / m_LineStride;
		for (int p = 0; p < m_LineStride; p++)		//  each point
		{
			color clr = getPlotColor(p, m_LineStride);
			glColor4ub(clrR(clr), clrG(clr), clrB(clr), 255.0);
			glBegin(GL_LINES);
			for (int i = 0; i < dTimeSteps - 1; i++) //  each timestep
			{
				int ndx0 = p + i * m_LineStride;
				int ndx1 = ndx0 + m_LineStride;
				if (ISNODATA(m_DataPlane[m_DisplayNdx[ndx0]]) || ISNODATA(m_DataPlane[m_DisplayNdx[ndx1]]))
					continue;

				float dval1 = getGradOffset(m_DataPlane[m_DisplayNdx[ndx0]]);
				float dval2 = getGradOffset(m_DataPlane[m_DisplayNdx[ndx1]]);
				if (dval1 < 0 || dval1 > 1.0 || dval2 < 0 || dval2 > 1.0)
					continue;
				glVertex2f((double) i / (dTimeSteps - 1), dval1);
				glVertex2f((double) (i + 1) / (dTimeSteps - 1), dval2);
			}

			glEnd();
		}

		fmax = m_UseDisplayMaxVal ? m_DisplayMaxVal : m_CurVarMax;
		fmin = m_UseDisplayMinVal ? m_DisplayMinVal : m_CurVarMin;
		pchFmt = "%.2lf";
		glColor4f(0.1, 0.1, 0.1, 0.3);
	}

	//  draw a faint grid in the background
	glLineWidth(1);
	glBegin(GL_LINES);
	float fval = 0.0;
	for (int i = 0; i <= 4; i++, fval += 0.25)
	{
		glVertex2f(0.0, fval);
		glVertex2f(1.0, fval);
		glVertex2f(fval, 0.0);
		glVertex2f(fval, 1.0);
	}
	glEnd();

	//  add line for current time if a time slice
	if (getShowTimeSlice() || getShowTimePlot())
	{
		if (getShowTimeSlice())
			glColor4f(0.85, 0.85, 0.85, 1.0);
		else
			glColor4f(0.15, 0.15, 0.15, 1.0);
		glLineWidth(1);

		float ftime = (m_Time - m_SelStart) / (m_SelFinish - m_SelStart);
		glBegin(GL_LINES);
		glVertex2f(ftime, 0.0);
		glVertex2f(ftime, 1.0);
		glEnd();
	}

	glPopMatrix();

	//  draw text on side of graph
	pWindow->getPos(w_x, w_y, w_w, w_h);

	char buf[256];
	sprintf(buf, pchFmt, fmax, 0xba);
	g_Draw.glDrawText(buf, 0xff7fffff, w_x + w_w - 47, g_Draw.getHeight() - (w_y + 23));
	sprintf(buf, pchFmt, fmin + (fmax - fmin) / 2, 0xba);
	g_Draw.glDrawText(buf, 0xff7fffff, w_x + w_w - 47, g_Draw.getHeight() - (w_y + w_h / 2 + 6));
	sprintf(buf, pchFmt, fmin, 0xba);
	g_Draw.glDrawText(buf, 0xff7fffff, w_x + w_w - 47, g_Draw.getHeight() - (w_y + w_h - 9));

	g_Draw.exitOrtho2D();
}

/* =============================================================================
    VERT SECTION EDITING
 =============================================================================== */
void CDataLayer::setLineEdit(bool b)
{
	m_LineEdit = b;
	if (m_LineEdit)
		initLineEdit();
	setDirty();
}

/* =============================================================================
 =============================================================================== */
void CDataLayer::initLineEdit()
{
	m_LineEditLine = CLine();

	//  create line type
	if (m_PlanesLineType == NULL)
	{
		m_PlanesLineType = new CLineType();
		m_PlanesLineType->setStyle(LINE_STYLE_SURVEY);
		m_PlanesLineType->setSize(2.00);
		m_PlanesLineType->setColor(0xffc03030);
	}

	//  create line
	m_LineEditLine.setLineType(m_PlanesLineType);
	if (getShowTimeSlice())
		m_LineEditLine.addControl(m_PlanesTimeSlicePos, -1);
	else
	{
		for (int i = 0; i < m_PlanesVertSliceList.size(); i++)
			m_LineEditLine.addControl(m_PlanesVertSliceList[i], -1);
	}

	m_LineEditLine.rebuildSegments();
	m_LineEditLine.setSelected(true);
}

/* =============================================================================
 =============================================================================== */
void CDataLayer::updateLineControlList()
{
	if (getShowTimeSlice())
		m_PlanesTimeSlicePos = m_LineEditLine.getControlPos(0);
	else
	{
		m_PlanesVertSliceList.clear();
		for (int i = 0; i < m_LineEditLine.getControlCnt(); i++)
			m_PlanesVertSliceList.push_back(m_LineEditLine.getControlPos(i));
	}

	setPlanesDirty();
}

/* =============================================================================
 =============================================================================== */
void CDataLayer::setLineControlNdx(int ndx)
{
	if (getShowTimeSlice())
		ndx = min(ndx, 0);
	m_LineEditLine.setSelControl(ndx);
}

/* =============================================================================
 =============================================================================== */
void CDataLayer::updateLineControlPos(Vec3d offset)
{
	int ndx = m_LineEditLine.getSelControl();
	if (ndx == -1)
		return;
	m_LineEditLine.updateControl(ndx, m_LineEditLine.getControlPos(ndx) + offset);
	m_LineEditLine.updatePosition();
	if (m_EditAutoUpdate)
		updateLineControlList();
}

/* =============================================================================
    PARTICLE EDITING
 =============================================================================== */
void CDataLayer::setPointEdit(bool b)
{
	m_PointEdit = b;
	if (m_PointEdit) initPointEdit();
	setDirty();
}

/* =============================================================================
 =============================================================================== */
void CDataLayer::addPointEditObject(Vec3d pos)
{

	//  create object type
	if (m_PointObjectType == NULL)
	{
		m_PointObjectType = new CObjectType();
		m_PointObjectType->setPosType(POS_MOBILE);
		m_PointObjectType->setPrimType(2);
		m_PointObjectType->setColor(0xffc03030);
	}

	CObject obj;
	obj.setObjectType(m_PointObjectType);
	obj.setPosition(pos);
	obj.setSize(2.0);
	obj.updatePosition();
	m_PointEditObjects.push_back(obj);
}

/* =============================================================================
 =============================================================================== */
void CDataLayer::initPointEdit()
{
	m_PointEditObjects.clear();

	//  create objects
	if (getShowTimePlot())
		for (int i = 0; i < m_PlanesTimePlotList.size(); i++)
			addPointEditObject(m_PlanesTimePlotList[i]);
	else
		for (int i = 0; i < m_ParticlePosList.size(); i++)
			addPointEditObject(m_ParticlePosList[i]);
}

/* =============================================================================
 =============================================================================== */
int CDataLayer::getSelPointCnt()
{
	int iCnt = 0;
	for (int i = 0; i < m_PointEditObjects.size(); i++)
		if (m_PointEditObjects[i].getSelected())
			iCnt++;
	return iCnt;
}

/* =============================================================================
 =============================================================================== */
void CDataLayer::selPoints()
{
	for (int i = 0; i < m_PointEditObjects.size(); i++)
		m_PointEditObjects[i].setSelected(true);
}

/* =============================================================================
 =============================================================================== */
void CDataLayer::selPoints(int xRBMin, int xRBMax, int yRBMin, int yRBMax)
{
	for (int i = 0; i < m_PointEditObjects.size(); i++)
	{
		Vec3f p2DPos = g_Draw.get2DPointGPS(m_PointEditObjects[i].getPosition());
		bool bSel = (p2DPos[0] > xRBMin && p2DPos[0] < xRBMax && p2DPos[1] > yRBMin && p2DPos[1] < yRBMax);
		m_PointEditObjects[i].setSelected(bSel);
	}
}

/* =============================================================================
 =============================================================================== */
void CDataLayer::selPoint(int ndx, bool bMulti)
{
	if (!bMulti)
		for (int i = 0; i < m_PointEditObjects.size(); i++)
			m_PointEditObjects[i].setSelected(false);
	if (ndx < 0 || ndx >= m_PointEditObjects.size())
		return;

	m_PointEditObjects[ndx].setSelected(true);
}

/* =============================================================================
 =============================================================================== */
void CDataLayer::updatePointList()
{
	if (getShowTimePlot())
	{
		m_PlanesTimePlotList.clear();
		for (int i = 0; i < m_PointEditObjects.size(); i++)
			m_PlanesTimePlotList.push_back(m_PointEditObjects[i].getPosition());
		setPlanesDirty();
		return;
	}

	if (m_ParticlesRebuild)
	{
		m_ParticlePosList.clear();
		m_ParticlePosList.resize(m_PointEditObjects.size(), Vec3d(NODATA, NODATA, NODATA));
	}

	for (int i = 0; i < m_PointEditObjects.size(); i++)
	{
		Vec3d pos = m_PointEditObjects[i].getPosition();

		//  update the editing update list if not rebuilding
		if (!m_ParticlesRebuild)
		{
			if (m_ParticlePosList[i] != pos)
				m_ParticleUpdateList[i] = pos;
		}

		//  store the position
		m_ParticlePosList[i] = pos;
	}

	setParticlesDirty();
}

/* =============================================================================
 =============================================================================== */
void CDataLayer::clearPoints()
{
	for (int i = m_PointEditObjects.size() - 1; i >= 0; i--)
		if (m_PointEditObjects[i].getSelected())
			m_PointEditObjects.erase(m_PointEditObjects.begin() + i);
	setParticlesRebuild();
	if (m_EditAutoUpdate)
		updatePointList();
}

/* =============================================================================
 =============================================================================== */
void CDataLayer::setPointDepth(float depth)
{
	for (int i = 0; i < m_PointEditObjects.size(); i++)
	{
		if (m_PointEditObjects[i].getSelected())
		{
			Vec3d pos = m_PointEditObjects[i].getPosition();
			g_Draw.setTerrainHeight(pos);
			pos[2] *= -1;
			if (depth < pos[2])
				pos[2] = depth;
			m_PointEditObjects[i].setPosition(pos);
		}
	}

	setParticlesRebuild();
	if (m_EditAutoUpdate)
		updatePointList();
}

/* =============================================================================
 =============================================================================== */
void CDataLayer::addPoints(Vec3d pos, double dist, int cnt, int style)
{
	if (style == 5)
	{
		g_Draw.setTerrainHeight(pos);

		double dInc = pos[2] / (double) cnt;
		for (int i = 0; i < cnt; i++)
			addPointEditObject(Vec3d(pos[0], pos[1], dInc * (double) i));
	}
	else
	{
		if (style == 1) dist /= 2.0;
		if (style == 2) dist *= 2.0;

		int latcnt = (style == 3) ? 1 : cnt;
		int loncnt = (style == 4) ? 1 : cnt;
		double dLat = (style == 3) ? pos[0] : pos[0] - dist * cnt / 2.0;
		double dLonBase = (style == 4) ? pos[1] : pos[1] - dist * cnt / 2.0;
		double dLon = dLonBase;
		for (int i = 0; i < latcnt; i++, dLat += dist, dLon = dLonBase)
			for (int i = 0; i < loncnt; i++, dLon += dist)
				addPointEditObject(Vec3d(dLat, dLon, pos[2]));
	}

	setParticlesRebuild();
	if (m_EditAutoUpdate)
		updatePointList();
}

/* =============================================================================
 =============================================================================== */
void CDataLayer::updatePointPos(Vec3d offset)
{
	for (int i = 0; i < m_PointEditObjects.size(); i++)
	{
		if (!m_PointEditObjects[i].getSelected())
			continue;
		m_PointEditObjects[i].setPosition(m_PointEditObjects[i].getPosition() + offset * Vec3d(1, 1, 0));
		m_PointEditObjects[i].updatePosition();
	}

	if (m_EditAutoUpdate) updatePointList();
}

/* =============================================================================
 =============================================================================== */
string CDataLayer::getInfoTextPoints()
{
	ostringstream s;
	s << "<html><br>" << endl;
	for (int i = 0; i < m_PointEditObjects.size(); i++)
	{
		s << "<b>" << i + 1 << ":</b> ";
		s << getStringFromPosition(m_PointEditObjects[i].getPosition(), true);
		s << ",  " << -m_PointEditObjects[i].getPosition()[2] << " m<br>" << endl;
	}

	s << "<table></table>" << endl;
	s << "</html>" << endl;
	return s.str();
}

/* =============================================================================
    return text for a data point
 =============================================================================== */
Vec3d CDataLayer::getDataPos(int iPnt) const
{
	return editData().getDataPos(m_CurVar, iPnt) * Vec3d(1, 1, -1);
}


string CDataLayer::getInfoText(int iPnt) const
{
	ostringstream s;
	s << "<html>" << endl;
	s << "<b>Name:</b> " << getName() << "<br>" << endl;
	s << "<b>Index:</b> " << iPnt << "<br>" << endl;
	s << "<b>Time:</b> " << getNiceDateTimeString(m_Time) << "<br>" << endl;

	Vec3d pos = getDataPos(iPnt);
	if (editData().isDataPosValid(m_CurVar, pos))
	{
		s << "<b>Location:</b> " << getStringFromPosition(pos, true) << "<br>" << endl;
		s << "<b>Depth:</b> " << -pos[2] << " meters<br>" << endl;
	}

	if (getData().getVarType(m_CurVar) == DATA_ENUM)
	{
		s << "<b>" << getData().getVarName(m_CurVar) << ":</b> " << getData().getVar(m_CurVar).getEnumString(m_DataPlane[m_DisplayNdx[iPnt]]) << "<br>" << endl;
	}
	else
		s << "<b>" << getData().getVarName(m_CurVar) << ":</b> " << m_DataPlane[m_DisplayNdx[iPnt]] << "<br>" << endl;
	s << "</html>" << endl;
	return s.str();
}
