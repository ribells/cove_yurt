/* =============================================================================
	File: terrain.cpp

 =============================================================================== */

#ifdef WIN32
#include <GL/gl.h>
#else
#include <GL/gl.h>
#endif
#include "terrain.h"
#include "utility.h"
#include "timer.h"
#include "web/web_file.h"

#include "gl_draw.h"

void	clearDownloadTile(CTerrainTilePtr hTile, int iObj = -1);

const double  RADIUS = 60.0;

int		g_Projection = PROJ_GEO;
Vec3d	gMeshCenterGPS = vl_0;
Vec3d	gMeshCenter = vl_0;
Vec3d	gMeshCenterProj = vl_0;

inline Vec3d	projectPnt(Vec3d pnt_in);

/* =============================================================================
 =============================================================================== */

void setMeshCenter(Vec3d v)
{
	gMeshCenter = v;
}

/* =============================================================================
 =============================================================================== */
Vec3d getMeshCenter()
{
	return gMeshCenter;
}

/* =============================================================================
 =============================================================================== */
Vec3d getMeshCenterGPS()
{
	return gMeshCenterGPS;
}

/* =============================================================================
 =============================================================================== */
void setMeshCenterGPS(Vec3d v)
{
	gMeshCenterGPS = v;
	gMeshCenter = Vec3d(v[1], 0, -v[0]);
	gMeshCenterProj = vl_0;
	gMeshCenterProj = projectPnt(gMeshCenter);
}

/* =============================================================================
 =============================================================================== */
int CTerrain::getProjection() const
{
	return g_Projection;
}

/* =============================================================================
 =============================================================================== */
void CTerrain::setProjection(int iProj)
{
	if (iProj == g_Projection)
		return;
	g_Projection = iProj;
	gMeshCenterProj = vl_0;
	gMeshCenterProj = projectPnt(gMeshCenter);
	setNeedTerrainUpdate();
}

/* =============================================================================
 =============================================================================== */
Vec3d resetLookAt(Vec3d pnt)
{
	if (g_Projection == PROJ_GLOBE)
	{
		if (pnt[0] < -180) pnt[0] += 360.0;
		if (pnt[0] > 180) pnt[0] -= 360.0;
	}
	else
	{
		if (pnt[0] < g_Draw.getGeoMin()) pnt[0] = g_Draw.getGeoMin();
		if (pnt[0] > g_Draw.getGeoMax()) pnt[0] = g_Draw.getGeoMax();
	}

	double	dMax = g_Projection == PROJ_MERC ? 72 : 90;
	double	dMin = g_Projection == PROJ_MERC ? -72 : -90;
	if (pnt[2] < dMin) pnt[2] = dMin;
	if (pnt[2] > dMax) pnt[2] = dMax;
	return pnt;
}

/* =============================================================================
 =============================================================================== */
inline Vec3d projectPnt(Vec3d pnt_in)
{
	Vec3d	pnt_out;
	switch(g_Projection)
	{
	case PROJ_GLOBE:
		{
			Mat3d	m1, m2, m;
			double	fRadius = (RADIUS + pnt_in[1]);
			m1.MakeRot(Vec3d(0, 1, 0), pnt_in[0] * vl_pi / 180.);
			m2.MakeRot(Vec3d(0, 0, 1), -pnt_in[2] * vl_pi / 180.);
			m = m1 * m2;
			pnt_out = fRadius * m * Vec3d(1, 0, 0);
		}
		break;

	case PROJ_MERC:
		{
			double	dval = min(72.0, max(-72.0, pnt_in[2]));
			pnt_in[2] = 90.0 * log(tan(vl_pi / 4.0 + dval * vl_pi / 360.0));
			pnt_out = pnt_in;
		}
		break;

	case PROJ_ELLIPSE:
		{
			double	alpha = pnt_in[2] * vl_pi / 180.0;
			pnt_in[0] *= min(1.0, max(0.0, sqrt(cos(alpha))));
			pnt_out = pnt_in;
		}
		break;

	default:	//  PROJ_GEO
		pnt_out = pnt_in;
		break;
	}

	return pnt_out - gMeshCenterProj;
}

/* =============================================================================
 =============================================================================== */
inline Vec3d unprojectPnt(Vec3d pnt_in)
{
	Vec3d	pnt_out;
	pnt_in += gMeshCenterProj;
	switch(g_Projection)
	{
	case PROJ_GLOBE:
		{
			double	fRadius = len(pnt_in);
			Vec3d	pnt = (pnt_in) / fRadius;
			pnt_out[1] = (fRadius - RADIUS);
			pnt_out[2] = asin(-pnt[1]) * 180. / vl_pi;
			pnt = norm(Vec3d(pnt[0], 0, pnt[2]));
			pnt_out[0] = -acos(pnt[0]) * 180. / vl_pi;
			if (pnt[2] < 0) pnt_out[0] *= -1;
		}
		break;

	case PROJ_MERC:
		{
			pnt_in[2] = atan(sinh(pnt_in[2] / 90.0)) * 180.0 / vl_pi;
			pnt_out = pnt_in;
		}
		break;

	case PROJ_ELLIPSE:
		{
			double	alpha = pnt_in[2] * vl_pi / 180.0;
			pnt_in[0] /= min(1.0, max(0.0, sqrt(cos(alpha))));
			pnt_out = pnt_in;
		}
		break;

	default:	//  PROJ_GEO
		pnt_out = pnt_in;
		break;
	}

	return pnt_out;
}

/* =============================================================================
 =============================================================================== */
Vec3d projectCamera(Vec3d pnt, Vec3d cam)
{
	Vec3d	pnt_out;
	switch(g_Projection)
	{
	case PROJ_GLOBE:
		{
			Vec3d	newpnt = projectPnt(pnt);
			Vec3d	dir = norm(projectPnt(0.01 * norm(cam - pnt) + pnt) - newpnt);
			pnt_out = newpnt + dir * len(cam - pnt);
		}
		break;

	default:
		pnt_out = projectPnt(cam);
		break;
	}

	return pnt_out;
}

/* =============================================================================
    called to reproject the tile based on the current projection
 =============================================================================== */
void projectVert(Vec3f &v, Vec3f &n)
{
	switch(g_Projection)
	{
	case PROJ_GLOBE:
		{
			Mat3d	m1, m2, m;
			m1.MakeRot(Vec3d(0, 1, 0), (v[0] + gMeshCenter[0]) * vl_pi / 180.);
			m2.MakeRot(Vec3d(0, 0, 1), -(v[2] + gMeshCenter[2]) * vl_pi / 180.);
			m = m1 * m2;
			v = getVec3f((RADIUS + v[1] + gMeshCenter[1]) * m * Vec3d(1, 0, 0) - gMeshCenterProj);
			
			static Mat3d m0 = vl_0;
			if (m0[0][0] == 0.0)
				m0.MakeRot(Vec3d(0, 0, 1), -vl_pi / 2.);
			n = getVec3f(m * (m0 * Vec3d(n[0], n[1], n[2])));
		}
		break;

	case PROJ_GEO:
		break;

	default:
		v = getVec3f(projectPnt(getVec3d(v) + getMeshCenter()));
		break;
	}
}

/* =============================================================================
 =============================================================================== */
Vec3d recenterGPS(Vec3d pos)
{
	if (g_Projection == PROJ_GEO && g_Draw.getGeoCenter() != 0.0)
	{
		if (pos[1] < g_Draw.getGeoMin())
			pos[1] += 360.0;
		else if (pos[1] > g_Draw.getGeoMax())
			pos[1] -= 360.0;
	}

	return pos;
}

/* =============================================================================
 =============================================================================== */
Vec3d getCameraFromGPS(Vec3d pos, t_scale scale)
{
	float	s = (pos[2] > 0) ? scale.land : scale.sea;
	pos[2] = s ? (pos[2]) * s / MetersPerDegree : 0;
	return Vec3d(pos[1], pos[2], -pos[0]);
}

/* =============================================================================
 =============================================================================== */
Vec3d getGPSFromCamera(Vec3d pos, t_scale scale)
{
	pos = Vec3d(-pos[2], pos[0], pos[1]);

	float	s = (pos[2] > 0) ? scale.land : scale.sea;
	pos[2] = s ? (pos[2]) * MetersPerDegree / s : 0;
	return pos;
}

/* =============================================================================
 =============================================================================== */
Vec3f getScenePntFromGPS(Vec3d pos, t_scale scale)
{
	pos = getCameraFromGPS(pos, scale);

	Vec3d	pnt = projectPnt(pos);
	return getVec3f(pnt);
}

/* =============================================================================
 =============================================================================== */
Vec3d getGPSFromScenePnt(Vec3f pos_in, t_scale scale)
{
	Vec3d	pos = unprojectPnt(getVec3d(pos_in));
	Vec3d	pnt = getGPSFromCamera(pos, scale);
	return pnt;
}

/* =============================================================================
 =============================================================================== */
double getSceneDistScale(Vec3f pos1, Vec3f pos2)
{
	if (g_Projection == PROJ_GLOBE)
		return 1.0;

	t_scale	scale;
	double	sceneDist = len(pos1 - pos2) * KmPerDegree;
	double	realDist = getKMfromGPS(getGPSFromScenePnt(pos1, scale), getGPSFromScenePnt(pos2, scale));
	return sceneDist / realDist;
}

/* =============================================================================
 =============================================================================== */
Vec3d projectUp(Vec3d pnt)
{
	return g_Projection == PROJ_GLOBE ? norm(projectPnt(pnt) + gMeshCenterProj) : Vec3d(0, 1, 0);
}

/* =============================================================================
 =============================================================================== */
Vec3d getSceneRotFromGPS(Vec3d pos)
{
	return g_Projection == PROJ_GLOBE ? Vec3d(0, pos[1], -90 + pos[0]) : vl_0;
}

/* =============================================================================
 =============================================================================== */
Vec3d getSceneRot(Vec3f pos_in)
{
	if (g_Projection != PROJ_GLOBE)
		return vl_0;

	Vec3d	pos = unprojectPnt(getVec3d(pos_in));
	t_scale	scale;
	pos = getGPSFromCamera(pos, scale);
	return Vec3d(0, pos[1], -90 + pos[0]);
}

/* =============================================================================
 =============================================================================== */
bool getFacingAway(Vec3f cam, Vec3f pnt)
{
	if (g_Projection != PROJ_GLOBE)
		return false;

	Vec3f	offset = getVec3f(gMeshCenterProj);
	return(dot(norm(cam + offset), norm(pnt + offset)) < 0.0);
}

/* =============================================================================
 =============================================================================== */
double getObjectRot()
{
	return g_Projection == PROJ_GLOBE ? 90 : 0;
}

/* =============================================================================
 =============================================================================== */
bool CTerrainTile::getTileInfo(Vec3d pos, int &iLevel, int &iRow, int &iCol) const
{
	if (pos[0] > m_LatMax || pos[0] < m_LatMin || pos[1] > m_LonMax || pos[1] < m_LonMin) 
		return false;

	if (!getHide())
	{
		iLevel = m_Level;
		iRow = m_TileRow;
		iCol = m_TileCol;
		return true;
	}

	//  find the child tile
	if (getChildCnt() == 0)
		return false;
	iRow = (int) floor(m_ChildRows * (pos[0] - m_LatMin) / (m_LatMax - m_LatMin));
	iCol = (int) floor(m_ChildCols * (pos[1] - m_LonMin) / (m_LonMax - m_LonMin));
	assert(iRow < m_ChildRows && iCol < m_ChildCols);
	return m_Child[iRow * m_ChildCols + iCol].getTileInfo(pos, iLevel, iRow, iCol);
}

/* =============================================================================
 =============================================================================== */
bool CTerrainTile::createTerrainTexture(unsigned char * &pTerrainBmp, int iCols, int iRows, bool bShading, bool bFlip)
{
	if (!getTerrainSet().getActiveLayerCnt())
		return false;

	int iGrad = getTextureSet().getCurGradIndex();
	if (iGrad == -1)
		return false;

	const CGradient	&Grad = getTextureSet().getTextureLayer(iGrad).getGradient();

	if (!mem_alloc(pTerrainBmp, iRows * iCols * 4))
		return false;

	Vec3f	*pTrnNormals = NULL;

	//  get the subsection of terrain values
	if ((m_BmpSize != iRows) || !m_BmpTerrain)
	{
		m_BmpSize = iRows;

		int iSize = (iRows + 1) * (iCols + 1);
		if (m_BmpTerrain) delete[] m_BmpTerrain;
		if (!mem_alloc(m_BmpTerrain, iSize))
		{
			delete[] pTerrainBmp;
			return false;
		}

		//  get normals if looking at slope
		if (bShading)
		{
			if (!mem_alloc(pTrnNormals, iSize))
				return false;
		}

		double	newLonMax = m_LonMax + (m_LonMax - m_LonMin) / (iCols - 1);
		double	newLatMin = m_LatMin - (m_LatMax - m_LatMin) / (iRows - 1);
		bool	bRet = getTerrainSet().getGrid(m_LonMin, newLonMax, newLatMin, m_LatMax,
											m_BmpTerrain, pTrnNormals, iCols + 1, iRows + 1);
		if (!bRet)
		{
			delete[] m_BmpTerrain;
			m_BmpTerrain = NULL;
			return false;
		}
	}

	//  create a bitmap based on the gradient colors and subsection of terrain
	float			*pTmp = m_BmpTerrain;
	unsigned char	*pBmp = pTerrainBmp;
	int				w = Grad.getMapWidth();

	int				iMin = (Grad.getCnt() > 2) ? Grad.getMin(2) : EARTHMIN;
	int				iMax = (Grad.getCnt() > 2) ? Grad.getMax(2) : EARTHMAX;

	for (int r = 0; r < iRows; r++)
	{
		pBmp = pTerrainBmp + (bFlip ? (iRows - r - 1) : r) * iCols * 4;
		for (int c = 0; c < iCols; c++)
		{
			double	fDepth = *pTmp++;

			if (ISNODATA(fDepth))
			{
				for (int j = 0; j < 4; j++) 
					*pBmp++ = 0;
				continue;
			}

			float	dVal = get1DTexCoord(-1, fDepth) * w;
			int		idx = (int) floor(dVal);
			float	x = dVal - idx;
			if (idx < 0 || idx >= w - 1)
			{
				idx = min(Grad.getMapWidth() - 1, max(0, idx));
				x = 0.0;
			}

			float	fFactor = 1.0;

			float	iContourStep = Grad.getContour();
			if (fDepth > iMin && fDepth < iMax && iContourStep > 0)
			{
				int iContour11 = floor(fDepth / iContourStep);
				int iContour12 = floor(*pTmp / iContourStep);
				int iContour21 = floor(*(pTmp + iCols) / iContourStep);
				int iContour22 = floor(*(pTmp + iCols + 1) / iContourStep);
				if ((iContour11 != iContour12) || (iContour11 != iContour21) || (iContour11 != iContour22))
				{
					fFactor = .75;
					x = 0.0;
				}

				if (Grad.getGradBins())
				{
					idx = (int) (get1DTexCoord(-1, iContour11 * iContourStep + iContourStep / 2) * (float) w);
					x = 0.0;
				}
			}

			unsigned char	*clr0 = Grad.getGradMap() + idx * 4;
			unsigned char	*clr1 = (x == 0.0) ? clr0 : clr0 + 4;

			for (int j = 0; j < 4; j++, clr0++, clr1++)
			{
				if (j == 3) fFactor = 1.0;
				*pBmp++ = (unsigned char) ((*clr0 * (1.0 - x) +*clr1 * x) * fFactor);
			}
		}

		pTmp++;
	}

	//  shade the bitmap based on normals derived from the terrain subsection
	if (bShading && pTrnNormals)
	{
		Vec3f	*pN = pTrnNormals;
		Vec3d	lookat = g_Draw.getLookAtGPS();
		Vec3d	lpos = g_Draw.getLightPosition();
		Vec3d	lvec = g_Draw.getGPSFromCamera(g_Draw.getLightPosition());
		Vec3f	vLight = norm(getVec3f(lvec) * Vec3f(-1, -1, 10.0 / MetersPerDegree));
		float	fShading = getTextureSet().getShading();
		for (int r = 0; r < iRows; r++)
		{
			pBmp = pTerrainBmp + (bFlip ? (iRows - r - 1) : r) * iCols * 4;
			for (int c = 0; c < iCols; c++, pN++)
			{
				Vec3f	n = norm(*pN * Vec3f(1, 1, 1 / fShading));
				float	fLight = dot(n, vLight);
				fLight = min(1.0f, max(0.0f, fLight));
				for (int j = 0; j < 3; j++) 
					pBmp[j] = (unsigned char) (((float) pBmp[j]) * fLight);
				pBmp += 4;
			}

			pN++;
		}

		delete[] pTrnNormals;
	}

	return true;
}

/* =============================================================================
 =============================================================================== */
bool CTerrainTile::updateTerrainTexture(int iObj)
{
	string			strPath = "";
	unsigned char	*pTerrainBmp = NULL;
	int				txID2 = -1;
	int				w, h;
	bool			bMipMap = true;
	bool			bUseParentTexture = false;

	
	CTextureLayer & Layer = getTextureLayer(iObj);
	C3DObject	&Object = getTileObject(iObj);

	//  used to generate 1D gradient textures
	if (Layer.getIsGrad())
	{
		CGradient	&Grad = Layer.editGradient();
		if (Grad.getGradMap() == NULL)
			Grad.updateMap();
		w = Grad.getMapWidth();
		h = 256;
		if (Grad.getTexID() == -1)
		{
			double	iCnt = 0;
			float	*pGradVals = NULL;
			int		iStep = Grad.getContour();
			if (iStep > 0)
			{
				int iMin = (Grad.getCnt() > 2) ? ceil(Grad.getMin(2) / (double) iStep) * iStep : EARTHMIN;
				int iMax = (Grad.getCnt() > 2) ? floor(Grad.getMax(2) / (double) iStep) * iStep : EARTHMAX;
				if (iMax < iMin) iMax = iMin;
				if (!mem_alloc(pGradVals, (iMax - iMin) / iStep + 16))
					return false;

				float	*pCur = pGradVals;
				for (int i = iMin; i <= iMax; iCnt++)
				{
					*pCur++ = get1DTexCoord(iObj, i);
					if (i == iMax)
						break;
					i = min(iMax, i + iStep);
				}
			}

			unsigned char	*pData = Grad.createGradBitmap(pGradVals, iCnt, true, Grad.getGradBins(), false, false);
			int				txID = createTexture(pData, w, h, GL_RGBA, GL_RGBA, TX_CLAMP_EDGE);
			if (pGradVals) delete[] pGradVals;
			Grad.setTexID(txID);
		}

		if (!Grad.getGradHiRes())
		{
			if (m_Parent)
				bUseParentTexture = true;
			else
				txID2 = Grad.getTexID();
		}
		else
		{
#ifdef _DEBUG
			w = h = 128;
#else
			w = h = 256;
#endif
			if (!createTerrainTexture(pTerrainBmp, w, h, false, true)) 
				bUseParentTexture = true;
		}
	}
	else	//  Load a file
	{
		if (getTextureStatus(iObj) != THREAD_TRY && getTextureStatus(iObj) != THREAD_DONE)
			return false;

		if (
			Layer.getImageTexturePath(strPath, m_Level)
		||	Layer.getTiledTexturePath(strPath, m_Level, m_TileRow, m_TileCol)
		)
		{
			string	strSupported[] = { ".dds", ".bmp", ".jpg", ".jpeg", ".png", "" };
			if (strPath.size() && !findfile(strPath, strSupported))
			{
				//  get file from the web if it's available
				if (getTextureStatus(iObj) == THREAD_TRY) 
					cacheTileTexture(iObj);
				strPath = "";	//  clear the path so that parent texture is used
			}
		}

		if (!strPath.size() && m_Parent)	//  no file found || waiting for download
			bUseParentTexture = true;
	}

	if (bUseParentTexture)
	{
		//  go to parent and get texture id
		int iPObj = getParent().getTextureLayerIndex(getTextureLayerPtr(iObj));
		if (iPObj < 0 || !getTileObject(iObj).getHasMaterial())
			editParent().updateTextureLayers();

		//  if not parent texture then drop out
		iPObj = getParent().getTextureLayerIndex(getTextureLayerPtr(iObj));
		editParent().setHide(true);
		if (iPObj == -1)
			return false;

		//  now sure parent has a texture id
		if (!getParent().getTileObject(iPObj).getHasMaterial())
			return false;

		CMaterial	&Material = getParent().getTileObject(iPObj).getMaterial();
		txID2 = Material.getTextureId();
		w = Material.getWidth();
		h = Material.getHeight();
		bMipMap = Material.getMipMap();
	}

	color	clr = CLR_WHITE;
	bool	bOverlay = Layer.getIsImage();
	int		iTxRet = -1;
	if (strPath.size())
		iTxRet = Layer.editModel().addMaterial("image surface", strPath, clr, bOverlay ? TX_CLAMP_BORDER : TX_CLAMP_EDGE	);
	else if (pTerrainBmp)
		iTxRet = Layer.editModel().addMaterial("grad surface", pTerrainBmp, w, h, clr, TX_CLAMP_EDGE);
	else if (txID2 != -1)
		iTxRet = Layer.editModel().addMaterial("parent surface", txID2, w, h, clr, bMipMap);
	else
		return false;
	/*
	if (iTxRet == -1 && Layer.getMoviePtr())
	{
		int w, h;
		Layer.getMoviePtr()->getMovieSize(w, h);
		txID2 = createVideoTexture(Layer.editMovie());
		iTxRet = Layer.editModel().addMaterial("parent surface", txID2, w, h, clr, false);
	}
	*/
	if (iTxRet == -1)
	{
		cout << "Error loading texture for terrain";
		if (strPath.size() && filesize(strPath) < 10000) 
			delfile(strPath);
		return false;
	}

	int mat_index = Layer.getModel().getMaterialCnt() - 1;
	Object.setMaterial(Layer.getModel().getMaterialPtr(mat_index));

	//  reset texture bounds based on texture range and lonmin/lonmax, etc
	double	xtxmin = 0, xtxmax = 1, ytxmin = 0, ytxmax = 1;
	if (bUseParentTexture)
	{
		int			iPObj = getParent().getTextureLayerIndex(getTextureLayerPtr(iObj));
		const C3DObject	&PObject = getParent().getTileObject(iPObj);
		double		size = PObject.getXTxMax() - PObject.getXTxMin();
		double		min = PObject.getXTxMin();
		xtxmin = min + size * (double) (m_TileCol % getParent().m_ChildCols) / (double) getParent().m_ChildCols;
		xtxmax = min + size * (double) (m_TileCol % getParent().m_ChildCols + 1) / (double) getParent().m_ChildCols;
		size = PObject.getYTxMax() - PObject.getYTxMin();
		min = PObject.getYTxMin();
		ytxmin = min + size * (double) (m_TileRow % getParent().m_ChildRows) / (double) getParent().m_ChildRows;
		ytxmax = min + size * (double) (m_TileRow % getParent().m_ChildRows + 1) / (double) getParent().m_ChildRows;
	}
	else if (m_Level == 0)
	{
		double w = Layer.getMaxPos()[1] - Layer.getMinPos()[1];
		double h = Layer.getMaxPos()[0] - Layer.getMinPos()[0];
		xtxmin = (m_LonMin - Layer.getMinPos()[1]) / w;
		xtxmax = (m_LonMax - Layer.getMinPos()[1]) / w;
		ytxmin = (m_LatMin - Layer.getMinPos()[0]) / h;
		ytxmax = (m_LatMax - Layer.getMinPos()[0]) / h;
	}

	Object.setXTxMin(xtxmin);
	Object.setXTxMax(xtxmax);
	Object.setYTxMin(ytxmin);
	Object.setYTxMax(ytxmax);
	Object.setTransparency(getTextureLayer(iObj).getTransparency());

	if (pTerrainBmp) 
		delete[] pTerrainBmp;
	resetTexCoords(iObj);
	return true;
}

/* =============================================================================
 =============================================================================== */
float CTerrainTile::get1DTexCoord(int iObj, float fDepth)
{
	int iGrad = iObj < 0 ? getTextureSet().getCurGradIndex() : getTextureLayer(iObj).getIsGrad() ? 1 : -1;
	if (iGrad == -1)
		return 0;

	CGradient &Grad = iObj < 0 ? getTextureSet().getTextureLayer(iGrad).editGradient() : getTextureLayer(iObj).editGradient();

	bool		bFlatLand = getTextureSet().getScale().land == 0;
	bool		bFlatWater = getTextureSet().getScale().sea == 0;

	if (ISNODATA(fDepth))
		return NODATA;

	if (fDepth > 0 && bFlatLand)
	{
		fDepth = 10.0;
	}
	else if (fDepth < 0 && bFlatWater)
	{
		fDepth = -10.0;
	}

	int ndx;
	if (Grad.getGradMap() == NULL)
		if (!Grad.updateMap())
			return 1.0;

	double	prevVal = Grad.getMapVal(0);
	double	prevDepth = Grad.getMapDepth(0);
	double	curDepth, curVal;
	for (ndx = 1; ndx < Grad.getMapCnt(); ndx++)
	{
		curDepth = Grad.getMapDepth(ndx);
		curVal = Grad.getMapVal(ndx);
		if (fDepth < curDepth)
			break;
		prevDepth = curDepth;
		prevVal = curVal;
	}

	float	dVal = 1.0;
	if (ndx < Grad.getMapCnt()) 
		dVal = prevVal + (fDepth - prevDepth) * (curVal - prevVal) / (curDepth - prevDepth);
	return dVal;
}

/* =============================================================================
 =============================================================================== */
void CTerrainTile::resetTexCoords(int iObj)
{
	if (getTileObjectCnt() <= iObj || getTextureLayerPtr(iObj) == NULL)
		return;

	bool			bGrad = (getTextureLayer(iObj).getIsGrad()) && !getTextureLayer(iObj).getGradient().getGradHiRes();

	C3DObject		&Object = getTileObject(iObj);
	vector<Vec3f>	&vTex = Object.editTexVerts();
	double			fTexYInc = (Object.getYTxMax() - Object.getYTxMin()) / (double) (m_MeshRows - 1);
	double			fTexXInc = (Object.getXTxMax() - Object.getXTxMin()) / (double) (m_MeshCols - 1);
	double			fTexY = Object.getYTxMin();
	int				iTex = 0;
	for (int r = 0; r < m_MeshRows + 2; r++, fTexY += fTexYInc)
	{
		double	fTexX = Object.getXTxMin();
		for (int c = 0; c < m_MeshCols; c++, fTexX += fTexXInc)
		{
			Vec3f	tx;
			if (bGrad)
				tx = Vec3f(get1DTexCoord(iObj, vTex[iTex][2]), 0, vTex[iTex][2]);
			else
				tx = Vec3f(fTexX, 1.0 - fTexY, vTex[iTex][2]);
			if (c == 0) 
				vTex[iTex++] = tx;
			vTex[iTex++] = tx;
			if (c == m_MeshCols - 1) 
				vTex[iTex++] = tx;
		}

		if (r == 0 || r == m_MeshRows) fTexY -= fTexYInc;
	}

	getTileObject(iObj).setDirty(true);
}

/* =============================================================================
 =============================================================================== */
void CTerrainTile::setSkirtHeight(int iObj, bool bSkirt)
{
	if (getTileObjectCnt() <= iObj)
		return;

	setSkirt(bSkirt);

	C3DObject		&Object = getTileObject(iObj);
	vector<Vec3f>	&vVerts = Object.editVerts();
	int				iCols = m_MeshCols + 2;
	int				iRows = m_MeshRows + 2;
	float			sk_ht = bSkirt ? len(vVerts[iCols + 2] - vVerts[iCols + 3]) / 2.0 : 0;
	for (int r = 1; r < iRows - 1; r++)
	{
		vVerts[r * iCols][1] = vVerts[r * iCols + 1][1] - sk_ht;
		vVerts[(r + 1) * iCols - 1][1] = vVerts[(r + 1) * iCols - 2][1] - sk_ht;
	}

	for (int c = 0; c < iCols; c++)
	{
		float	ht = (c == 0 || c == iCols - 1) ? 0 : sk_ht;
		vVerts[c][1] = vVerts[c + iCols][1] - ht;
		vVerts[(iRows - 1) * iCols + c][1] = vVerts[(iRows - 2) * iCols + c][1] - ht;
	}

	getTileObject(iObj).setDirty(true);
}

/* =============================================================================
 =============================================================================== */
bool CTerrainTile::setDepthMinMax(float *pTerrain, int iCnt)
{
	m_MinDepth = 1e10;
	m_MaxDepth = -1e10;
	for (int i = 0; i < iCnt; i++)
	{
		float	depth = pTerrain[i];
		if (ISNODATA(depth))
			continue;
		if (depth < EARTHMIN || depth > EARTHMAX)
			continue;
		if (depth < m_MinDepth) m_MinDepth = depth;
		if (depth > m_MaxDepth) m_MaxDepth = depth;
	}

	bool	bSet = true;
	if (m_MinDepth == 1e10)
	{
		if (!m_Parent)
		{
			m_MinDepth = EARTHMIN;
			m_MaxDepth = EARTHMAX;
		}
		else
		{
			m_MinDepth = getParent().getMinDepth();
			m_MaxDepth = getParent().getMaxDepth();
		}

		bSet = false;
	}

	setBoundingSphere();
	return bSet;
}

/* =============================================================================
 =============================================================================== */
C3DObjectPtr CTerrainTile::createMesh()
{
	//  create object to display
	float	*pTerrain = NULL;
	Vec3f	*pNormals = NULL;
	if (!mem_alloc(pTerrain, m_MeshCols * m_MeshRows) 
		|| !mem_alloc(pNormals, m_MeshRows * m_MeshCols)
		|| !getTerrainSet().getGrid(m_LonMin, m_LonMax, m_LatMin, m_LatMax, pTerrain, pNormals, m_MeshCols, m_MeshRows))
	{
		if (pTerrain) delete[] pTerrain;
		if (pNormals) delete[] pNormals;
		return NULL;
	}

	// improve min max for bounding volume and build minimal tile if no valid depth data
	if (!setDepthMinMax(pTerrain, m_MeshRows * m_MeshCols)) 
		m_MeshRows = m_MeshCols = 1;

	//  create new object
	int rows = m_MeshRows + 2;
	int cols = m_MeshCols + 2;
	int cnt = rows * cols;
	int fcnt = (rows - 1) * (cols - 1) * 2 + (rows - 2) * 3;
	C3DObjectPtr pObject = new C3DObject();
	C3DObject		&Object = *pObject;
	if (!Object.initArrays(cnt, cnt, cnt, 0, fcnt)) 
		return NULL;

	vector<Vec3f>	&vVerts = Object.editVerts();
	vector<Vec3f>	&vTexVerts = Object.editTexVerts();
	vector<Vec3f>	&vNormals = Object.editNormals();
	vector<CFace>	&vFaces = Object.editFaces();

	double			lonStart = getLonOffset();
	Vec3d			vOffset = getMeshCenter();

	double dLat = m_LatMin - (-vOffset[2]);
	double dLatInc = (m_LatMax - m_LatMin) / (double) (m_MeshRows - 1);
	double dLonInc = (m_LonMax - m_LonMin) / (double) (m_MeshCols - 1);
	int ndxSrc = 0;
	int	ndxDst = cols + 1;
	for (int r = 1; r < rows-1; r++, dLat += dLatInc, ndxDst += 2)
	{
		double	dLon = lonStart - vOffset[0];
		for (int c = 1; c < cols-1; c++, dLon += dLonInc, ndxSrc++, ndxDst++)
		{
			float	fDepth = pTerrain[ndxSrc];
			if (ISNODATA(fDepth))
				fDepth = m_MinDepth;
			//  create the mesh components
			vVerts[ndxDst] = Vec3f(dLon, fDepth/ MetersPerDegree, -dLat);
			vTexVerts[ndxDst] = Vec3f(0, 0, fDepth);
			Vec3f	n = pNormals[ndxSrc];
			vNormals[ndxDst] = Vec3f(n[0], n[2], -n[1]);
		}
	}
	delete[] pTerrain;
	delete[] pNormals;

	//add skirt vertices
	for (int r = 1, ndx = cols; r < rows-1; r++, ndx += cols)
	{
		vVerts[ndx] = vVerts[ndx+1]; vVerts[ndx+cols-1] = vVerts[ndx+cols-2];
		vTexVerts[ndx] = vTexVerts[ndx+1]; vTexVerts[ndx+cols-1] = vTexVerts[ndx+cols-2];
		vNormals[ndx] = vNormals[ndx+1]; vNormals[ndx+cols-1] = vNormals[ndx+cols-2];
	}
	for (int c = 0, ndx = cols * (m_MeshCols+1); c < cols; c++, ndx++)
	{
		vVerts[c] = vVerts[c+cols]; vVerts[ndx] = vVerts[ndx-cols];
		vTexVerts[c] = vTexVerts[c+cols]; vTexVerts[ndx] = vTexVerts[ndx-cols];
		vNormals[c] = vNormals[c+cols]; vNormals[ndx] = vNormals[ndx-cols];
	}

	//create faces as a long triangle strip  - take care of skirts by adding extra verts on each side
	int iCnt = 0;
	for (int r = 0; r < rows - 1;)
	{
		int c;
			//  head out
		for (c = 0; c < cols - 1; c++)
		{
			vFaces[iCnt++].setFace((r + 1) * cols + c, r * cols + c, (r + 1) * cols + c + 1);
			vFaces[iCnt++].setFace((r + 1) * cols + c + 1, r * cols + c, r * cols + c + 1);
		}
			//  turn around
		if (r < rows - 2)
		{
			vFaces[iCnt++].setFace((r + 1) * cols + c, r * cols + c, (r + 1) * cols + c - 1);
			vFaces[iCnt++].setFace(r * cols + c, (r + 1) * cols + c - 1, (r + 1) * cols + c);
			vFaces[iCnt++].setFace((r + 1) * cols + c - 1, (r + 1) * cols + c, (r + 2) * cols + c);
		}

		if (++r == rows - 1)
			break;

			//  head back
		for (c = cols - 2; c >= 0; c--)
		{
			vFaces[iCnt++].setFace(r * cols + c, (r + 1) * cols + c + 1, (r + 1) * cols + c);
			vFaces[iCnt++].setFace(r * cols + c + 1, (r + 1) * cols + c + 1, r * cols + c);
		}
			//  turn around
		c = 0;
		if (r < rows - 2)
		{
			vFaces[iCnt++].setFace((r + 1) * cols + c, r * cols + c, (r + 1) * cols + c + 1);
			vFaces[iCnt++].setFace(r * cols + c, (r + 1) * cols + c + 1, (r + 1) * cols + c);
			vFaces[iCnt++].setFace((r + 1) * cols + c + 1, (r + 1) * cols + c, (r + 2) * cols + c);
		}
		r++;
	}

	//  prep tile for display
	Object.setTriangleStrip(true);
	Object.setReprojected(true);
	Object.setCheckNoData(true);
	Object.setDirty(true);
	setHide(true);	//  initially hidden - culling will turn it on if necessary
	setSkirt(false);
	resetTexCoords(0);
	return pObject;
}

/* =============================================================================
 =============================================================================== */
void CTerrainTile::updateTextureLayers()
{
	//  figure out which textures need to be displayed by this tile
	vector<CTextureLayerPtr> newTexList;
	CTextureLayerPtr			pBackground = NULL;
	Vec3d						minpos, maxpos;
	for (int i = 0; i < getTextureSet().getTextureLayerCnt(); i++)
	{
		CTextureLayer	&Tex = getTextureSet().getTextureLayer(i);
		if (!Tex.getActive() || Tex.getIsScreen() || Tex.getVertical())
			continue;
		Tex.getMinMax(minpos, maxpos);

		CTextureLayerPtr hTex = getTextureSet().getTextureLayerPtr(i);
		if (Tex.getBackground())
			pBackground = hTex;
		else if (minpos[0] <= m_LatMax && maxpos[0] >= m_LatMin && minpos[1] <= m_LonMax && maxpos[1] >= m_LonMin)
			newTexList.push_back(hTex);
	}

	//  put the background texture in first
	if (pBackground) 
		newTexList.insert(newTexList.begin(), pBackground);

	// check what textures/objects are already in place and put those in our list
	vector<C3DObjectPtr> vObjects;
	for (int i = 0; i < newTexList.size(); i++)
	{
		int   iCur = getTextureLayerIndex(newTexList[i]);
		C3DObjectPtr hCurObj = NULL;
		if (iCur == -1)
		{		//  need to make a new object
			if (getTileObjectCnt() == 0)
				hCurObj = createMesh();
			else
				hCurObj = new C3DObject(getTileObject(0));
			if (hCurObj == NULL)
				continue;
			newTexList[i]->editModel().addObject(hCurObj);
		}
		else
		{
			hCurObj = getTileObjectPtr(iCur);
			clearTileObject(iCur);
		}

		vObjects.push_back(hCurObj);
		vObjects[i]->setLayering(newTexList[i]->getLayerType());
		vObjects[i]->setOffset(newTexList[i]->getPosOffset()[2] / MetersPerDegree);
	}

	//  copy over the correct object order to the model and clear texture status
	m_TileObjects.clear();
	m_TexLayerPtr.clear();
	m_TextureStatus.clear();
	m_TryCount.clear();
	for (int i = 0; i < vObjects.size(); i++)
	{
		addTileObject(vObjects[i], newTexList[i]);
		m_TextureStatus.push_back(0);
		m_TryCount.push_back(0);
	}

	for (int i = 0; i < getTileObjectCnt(); i++)
	{
		bool bSkirt = (getTextureLayer(i).getPosOffset()[2] == 0.0
			&& getTextureLayer(i).getTransparency() == 1.0 
			&& getTerrainSet().getShowSkirts());
		setSkirtHeight(i, bSkirt);

		// now redo the textures for those that have no material id
		if (!getTileObject(i).getHasMaterial())
			updateTerrainTexture(i);
	}

	m_NeedUpdate = false;
}

/* =============================================================================
 =============================================================================== */
void CTerrainTile::reproject()
{

	//  if hidden then skip reproject
	if (getHide())
		return;
	if (getNoTerrain())
		setHide(true);
	else if (getNeedUpdate())	//  if we have no texture, then load it
	{
		updateTextureLayers();
		setHide(false);
	}
}

/* =============================================================================
 =============================================================================== */
void createBoundingSphere(Vec3f *pPnt, int iCnt, Vec3f &Center, float &radius)
{
	Center = vl_0;
	for (int i = 0; i < iCnt; i++) 
		Center += pPnt[i];
	Center /= iCnt;

	radius = 0.0f;
	for (int i = 0; i < iCnt; i++)
	{
		double	r2 = sqrlen(pPnt[i] - Center);
		if (r2 > radius) radius = r2;
	}

	radius = sqrt(radius);
}

/* =============================================================================
 =============================================================================== */
double CTerrainTile::getLonOffset()
{
	double	lonmin = m_LonMin;
	if (g_Draw.getGeoCenter() > 0 && m_LonMin < -180 + g_Draw.getGeoCenter())
		lonmin += 360;
	else if (g_Draw.getGeoCenter() < 0 && m_LonMax > 180 + g_Draw.getGeoCenter())
		lonmin -= 360;
	return lonmin;
}

/* =============================================================================
 =============================================================================== */
void CTerrainTile::setBoundingSphere()
{
	t_scale scale = getTextureSet().getScale();
	m_BBoxScale[0] = scale.land;
	m_BBoxScale[1] = scale.sea;

	Vec3f	bbCorners[8];
	double	lonmin = getLonOffset();
	double	lonmax = lonmin + (m_LonMax - m_LonMin);
	Vec3d	tmp[8] =
	{
		Vec3d(m_LatMin, lonmin, m_MaxDepth),
		Vec3d(m_LatMin, lonmax, m_MaxDepth),
		Vec3d(m_LatMax, lonmax, m_MaxDepth),
		Vec3d(m_LatMax, lonmin, m_MaxDepth),
		Vec3d(m_LatMin, lonmin, m_MinDepth),
		Vec3d(m_LatMin, lonmax, m_MinDepth),
		Vec3d(m_LatMax, lonmax, m_MinDepth),
		Vec3d(m_LatMax, lonmin, m_MinDepth)
	};

	//  set corners to scene coordinates in flat projection
	int		iCurProj = g_Projection;
	if (iCurProj == PROJ_GLOBE) 
		g_Projection = PROJ_GEO;
	for (int i = 0; i < 8; i++)
		bbCorners[i] = getScenePntFromGPS(tmp[i], scale);

	createBoundingSphere(bbCorners, 8, m_BBoxCenter, m_BBoxRadius);

	Vec3d	pntGPS = g_Draw.getGPSFromScenePnt(m_BBoxCenter);
	if (iCurProj == PROJ_GLOBE) 
		g_Projection = iCurProj;
	// set bbox center and corner points based on 
	m_BBoxCenter = g_Draw.getScenePntFromGPS(pntGPS);
	for (int i = 0; i < 4; i++)
		m_BBoxCorners[i] = getScenePntFromGPS(tmp[i], scale);
}

bool CTerrainTile::getFacingAway() const
{
	Vec3f campos = getRealCameraPos();
	for (int i = 0; i < 4; i++) 
		if (!::getFacingAway(campos, m_BBoxCorners[i]))
			return false;
	return true;
}
/* =============================================================================
 =============================================================================== */
double CTerrainTile::distToTile() const
{
	Vec3f grcp = getRealCameraPos();
	Vec3f mbbc = m_BBoxCenter;
	return(len(getRealCameraPos() - m_BBoxCenter)- m_BBoxRadius);
}

/* =============================================================================
 =============================================================================== */
void CTerrainTile::deleteChildren()
{
	for (int i = 0; i < getChildCnt(); i++) 
		clearDownloadTile(&m_Child[i]);
	m_Child.clear();
}

double	g_FlushTime = 5.0; //  flush undrawn tiles after n seconds

void CTerrainTree::flush()
{
	g_FlushTime = 0.0;
	cull();
	g_FlushTime = 5.0;
}

/* =============================================================================
    recursively go through tiles and see what should be shown
 =============================================================================== */
void CTerrainTile::cull()
{
	double	dTime = g_Timers.pollTimer(COVE_TIMER);
	m_TileCount++;

	if (m_Level > 0) {
		double dist = distToTile();

		//  check if in current viewing frustrum
		bool gfa = getFacingAway();
		bool sif = SphereInFrustum(m_BBoxCenter, m_BBoxRadius);
		if (dist > .0001 && (!SphereInFrustum(m_BBoxCenter, m_BBoxRadius) || getFacingAway())) {
			if (getChildCnt() && dTime > m_LastDrawn + g_FlushTime) {
				deleteChildren();	// clear out nodes below this one - effects panning speed
			}
			m_LastDrawn = dTime;
			return;
		}

		// check distance to camera to see if we're far enough away to show this one or we are at the maximum level
		double factor = getTerrainSet().getTileFactor();
		if (dist > (factor * getTerrainSet().getTileScale() * 100.0) / (double) (0x1 << m_Level)
			||	m_MaxLevel*factor <= m_Level)
		{
			setHide(false);
			reproject();
			if (getChildCnt() && dTime > m_LastDrawn + g_FlushTime) {
				deleteChildren(); // clear out nodes below this one - effects zooming speed
			}
			m_LastDrawn = dTime;
			return;
		}
	}

	//  otherwise - create children if they don't exist
	if (getChildCnt() == 0)
	{
		try
		{
			m_Child.resize(m_ChildRows * m_ChildCols);	//to assure memory doesn't move
		}
		catch(std::bad_alloc const &)
		{
			cout << "Memory allocation failure!" << endl;
			return;
		}

		int iCur = 0;
		for (double i = 0; i < m_ChildRows; i++)
		{
			for (double j = 0; j < m_ChildCols; j++, iCur++)
			{
				m_Child[iCur].init(this, m_Level + 1,
									m_TileRow * (m_Level ? 2 : 1) + i, 
									m_TileCol * (m_Level ? 2 : 1) + j);
				m_Child[iCur].setBounds (
									(m_LonMax - m_LonMin) * j / m_ChildCols + m_LonMin,
									(m_LonMax - m_LonMin) * (j + 1.) / m_ChildCols + m_LonMin,
									(m_LatMax - m_LatMin) * i / m_ChildRows + m_LatMin,
									(m_LatMax - m_LatMin) * (i + 1.) / m_ChildRows + m_LatMin);
			}
		}
	}
	if(m_Level < m_MaxLevel) {
		m_Level++;
		//  and cull the children
		for (int i = 0; i < getChildCnt(); i++) {
			m_Child[i].cull();
		}
		m_Level--;
	}
}

/* =============================================================================
 =============================================================================== */
void CTerrainTree::create(CTerrainSet *pTerrain, CTextureSet *pTextures)
{
	Vec3d	minPos = pTerrain->getMinPos();
	Vec3d	maxPos = pTerrain->getMaxPos();
	Vec3d	size = maxPos - minPos;
	int		xmin = 0, xmax = 0, ymin = 0, ymax = 0;
	if (size != vl_0)
	{
		xmin = floor((max(-180.0, minPos[1]) + 180.0) / 36.0);
		xmax = ceil((min(180.0, maxPos[1]) + 180.0) / 36.0);
		ymin = floor((max(-90.0, minPos[0]) + 90.0) / 36.0);
		ymax = ceil((min(90.0, maxPos[0]) + 90.0) / 36.0);
	}

	int cols = xmax - xmin;
	int rows = ymax - ymin;

	pTextures->unloadMaterials();	//initialize the scene
	m_Root.init(NULL, 0, ymin, xmin);
	m_Root.setEnvironment(pTerrain, pTextures);
	m_Root.setBounds(xmin * 36.0 - 180.0, xmax * 36.0 - 180.0, ymin * 36.0 - 90.0, ymax * 36.0 - 90.0);
	m_Root.setChildRows(rows);
	m_Root.setChildCols(cols);
	m_Root.setMeshRows(32);
	m_Root.setMeshCols(32);
}
