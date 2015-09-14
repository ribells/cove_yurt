/* =============================================================================
	File: terrain_draw.cpp

 =============================================================================== */

#ifdef WIN32
#include <GL/gl.h>
#else
#include <GL/gl.h>
#endif

#include "utility.h"
#include "web/web_file.h"

#include "terrain.h"

#include "gl_draw.h"

#include <vl/VLgl.h>

/* =============================================================================
    vector to track downloads properly
 =============================================================================== */
struct TileDownload
{
	CTerrainTilePtr	hTile;
	int					iStatus;
	int					iObject;
	string				strURL;
};

typedef TileDownload		*TileDownloadPtr;

vector<TileDownloadPtr>	g_TileDownload;

TileDownload &getTileDownload(int i) { return *g_TileDownload[i]; }
int getTileDownloadCnt() { return g_TileDownload.size(); }

/* =============================================================================
 =============================================================================== */
void removeDownload(int i)
{
	delete g_TileDownload[i];
	g_TileDownload.erase(g_TileDownload.begin() + i);
}

/* =============================================================================
 =============================================================================== */
void clearDownloadTile(CTerrainTilePtr hTile, int iObj = -1)
{
	for (int i = 0; i < getTileDownloadCnt(); i++)
	{
		if (iObj >= 0 && getTileDownload(i).iObject != iObj)
			continue;
		if (getTileDownload(i).hTile == hTile) 
			getTileDownload(i).hTile = NULL;
	}
}

/* =============================================================================
 =============================================================================== */
void clearAllDownloadTiles()
{
	while(getTileDownloadCnt()) removeDownload(0);
}

/* =============================================================================
 =============================================================================== */
bool checkTileDownload(CTerrainTilePtr hTile, int iObj)
{
	for (int i = 0; i < getTileDownloadCnt(); i++)
		if (hTile == getTileDownload(i).hTile && getTileDownload(i).iObject == iObj)
			return true;
	return false;
}

/* =============================================================================
 =============================================================================== */
bool CTerrain::getNeedRedraw()
{
	return checkNewDownload();
}

/* =============================================================================
 =============================================================================== */
bool CTerrainTile::cacheTileTexture(int iObj)
{
	string	strURL;
	string	strPath;

	//  check if tile already on download list
	if (checkTileDownload(this, iObj))
		return true;
	if (getTextureLayerPtr(iObj) == 0)
		return false;

	CTextureLayer	&table = *(getTextureLayerPtr(iObj));
	if (!table.getTiledTextureURL(strURL, m_Level, m_TileRow, m_TileCol)) 
		return false;
	if (!table.getTiledTexturePath(strPath, m_Level, m_TileRow, m_TileCol)) 
		return false;
	setTextureStatus(iObj, THREAD_WAIT);

	//  create new tile download record
	TileDownloadPtr	hDownload = new TileDownload;
	TileDownload		&Download = *hDownload;
	Download.strURL = strURL;
	Download.iObject = iObj;
	Download.iStatus = 0;
	Download.hTile = this;

	if (getWebFile(strURL, strPath, "", &(Download.iStatus)))
	{
		g_TileDownload.push_back(hDownload);
		return true;
	}

	setTextureStatus(iObj, THREAD_DONE);	//  otherwise stop trying web
	return false;
}

/* =============================================================================
 =============================================================================== */
bool CTerrain::updateTerrain()
{
	if (!m_NeedTerrainUpdate)
		return false;
	m_NeedTerrainUpdate = false;
	//cout << " - updating terrain\n";

	//  create the tree
	clearAllDownloadTiles();
	getTree().clean();
	int alc = getTerrainSet().getActiveLayerCnt();
	if (getTerrainSet().getActiveLayerCnt() == 0)
		return true;

	getTree().create(m_TerrainSet, m_TextureSet);
	return true;
}

/* =============================================================================
 =============================================================================== */
void CTerrain::Render()
{
	int temp1 = g_TileDownload.size();
	//  look at download list and prep tiles if they have new data
	for (int i = (int) g_TileDownload.size() - 1; i >= 0; i--)
	{
		if (getTileDownload(i).iStatus == THREAD_WAIT) {
			continue;
		}
		CTerrainTilePtr	hTile = getTileDownload(i).hTile;
		int				iObj = getTileDownload(i).iObject;
		int				iStatus = getTileDownload(i).iStatus;
		string			strURL = getTileDownload(i).strURL;
		removeDownload(i);

		if (!hTile) {
			continue;
		}
		CTerrainTile	&Tile = getTree().getTile(hTile);

        if (!Tile.getTileObjectValid(iObj))
            continue;
            
		if (iStatus == THREAD_SUCCEED)
		{
            Tile.setTextureStatus(iObj, THREAD_DONE);
            Tile.removeTextureLayer(Tile.getTextureLayerPtr(iObj), true);
		}
		else if (iStatus == THREAD_FAIL)
		{
			if (Tile.getTryCount(iObj) + 1 == DOWNLOAD_TRY_MAX)
			{
				Tile.setTextureStatus(iObj, THREAD_DONE);
				cout << ("Texture request failed:  " + strURL);
			}
			else
			{
				Tile.setTryCount(iObj, Tile.getTryCount(iObj) + 1);
				Tile.cacheTileTexture(iObj);
			}
		}
	}

	//  figure out tiles to show
	if (!getTileNoCull()) {
		cullTiles();
	}

	//  and draw all the active terrain tiles
	getTextureSet().Render();
}

/* =============================================================================
 =============================================================================== */
void CTerrain::setFog(double factor, color clr)
{
	float	dist = g_Draw.getCameraDistKM();
	float	distFactor = min(1.0, 10000.0 / (dist * dist));
	factor *= distFactor;
	if (factor < 0.001)
		return;

	Vec3f	vclr = clrVec(clr);
	GLfloat fogColor[4] = { vclr[0], vclr[1], vclr[2], clrTrans(clr) }; //  Fog Color
	glFogi(GL_FOG_MODE, GL_EXP);		//  Fog Mode - GL_EXP, GL_EXP2, GL_LINEAR
	glFogfv(GL_FOG_COLOR, fogColor);	//  Set Fog Color
	glFogf(GL_FOG_DENSITY, factor);		//  How Dense Will The Fog Be
	glHint(GL_FOG_HINT, GL_DONT_CARE);	//  Fog Hint Value
	glFogf(GL_FOG_START, .0f);			//  Fog Start Depth
	glFogf(GL_FOG_END, 20.0f);			//  Fog End Depth
	glEnable(GL_FOG);	//  Enables GL_FOG
}

/* =============================================================================
 =============================================================================== */
void CTerrain::drawUTMGrid(Vec3d vCenter, double fScale, double fRes, bool bLabels, color clr)
{
	//  cached values to be fast on redraw
	static Vec3f	*pRows = NULL;
	static Vec3f	*pCols = NULL;
	static Vec3d	lastminpos = vl_0;
	static Vec3d	lastmaxpos = vl_0;
	static double	lastScale = 0;
	static double	lastSize = 0;

	//  utm not valid above 84 north and below 80 south
	if (fabs(vCenter[0]) > 80.0)
		return;

	//  force rebuild next time
	if (fScale == 0.0)
	{
		lastScale = 0.0;
		return;
	}

	Vec3d	posd = vCenter + Vec3d(-1, -1, 0) * (fScale / fRes) * 2 / MetersPerDegree;
	g_Draw.setTerrainHeight(posd);

	double	dCamDist = g_Draw.getCameraDistKM();
	float	text_scale = dCamDist / MetersPerDegree * .8;

	if (bLabels)
	{
		posd[2] += max(1.0, min(10000.0, dCamDist));

		Vec3f	pos = g_Draw.getScenePntFromGPS(posd);
		char	buf[1024];
		if (fScale < 1000)
			sprintf(buf, "Grid: %.1lfm", (fScale / fRes));
		else
			sprintf(buf, "Grid: %.1lfkm", (fScale / fRes) / 1000);
		g_Draw.drawScaledText(buf, clr, pos, text_scale);
	}

	glColor4ub(clrR(clr), clrG(clr), clrB(clr), clrA(clr)/2);
	glLineWidth(2);

	int size = 10 * fRes;
	int lres = 10;
	int cntRows = size * 2 + 1;
	int cntCols = size * 2 + 1;
	int cntPnts = size * 2 * lres + 1;

	//  rebuild grid if position or dist has changed
	vCenter *= Vec3d(1, 1, 0);

	double	zx = 0, zy = 0;
	getUTMfromLL(vCenter[0], vCenter[1], zx, zy, vCenter[1], vCenter[0]);

	Vec3d	extent = Vec3d(1, 1, 0) * size * (fScale / fRes);
	Vec3d	minpos = vCenter - extent;
	Vec3d	maxpos = vCenter + extent;
	if (minpos != lastminpos || maxpos != lastmaxpos || fScale != lastScale || size != lastSize)
	{
		if (pRows) delete pRows;
		pRows = new Vec3f[cntRows * cntPnts];
		if (pCols) delete pCols;
		pCols = new Vec3f[cntCols * cntPnts];

		float	fStep = (fScale / fRes);
		float	fResStep = fStep / lres;
		double	dLat, dLon, y, x;
		float	fDepth;
		int		r, c, i;
		int		ndx = 0;
		for (r = 0, y = minpos[0]; r < cntRows; y += fStep, r++)
		{
			for (i = 0, x = minpos[1]; i < cntPnts; x += fResStep, i++)
			{
				getLLfromUTM(zx, zy, x, y, dLat, dLon);
				getTerrainSet().getPosHeight(dLat, dLon, fDepth = 0.0);
				pRows[ndx++] = g_Draw.getScenePntFromGPS(Vec3d(dLat, dLon, fDepth));
			}
		}

		ndx = 0;
		for (c = 0, x = minpos[1]; c < cntCols; x += fStep, c++)
		{
			for (i = 0, y = minpos[0]; i < cntPnts; y += fResStep, i++)
			{
				getLLfromUTM(zx, zy, x, y, dLat, dLon);
				getTerrainSet().getPosHeight(dLat, dLon, fDepth = 0.0);
				pCols[ndx++] = g_Draw.getScenePntFromGPS(Vec3d(dLat, dLon, fDepth));
			}
		}

		lastminpos = minpos;
		lastmaxpos = maxpos;
		lastScale = fScale;
		lastSize = size;
	}

	int ndx = 0;
	for (int r = 0; r < cntRows; r++)
	{
		glBegin(GL_LINE_STRIP);
		for (int i = 0; i < cntPnts; i++) 
			glVertex(pRows[ndx++]);
		glEnd();
	}

	ndx = 0;
	for (int c = 0; c < cntCols; c++)
	{
		glBegin(GL_LINE_STRIP);
		for (int i = 0; i < cntPnts; i++) 
			glVertex(pCols[ndx++]);
		glEnd();
	}
}

/* =============================================================================
 =============================================================================== */
void CTerrain::drawLLGrid(Vec3d vCenter, double fScale, double fRes, bool bLabels, color clr)
{
	//  cached values to be fast on redraw
	static Vec3f	*pRows = NULL;
	static Vec3f	*pCols = NULL;
	static Vec3d	lastminpos = vl_0;
	static Vec3d	lastmaxpos = vl_0;
	static double	lastScale = 0;
	static double	lastSize = 0;

	int size = 10 * fRes;
	int lres = 25;
	int cntRows = size * 2 + 1;
	int cntCols = size * 2 + 1;
	int cntPnts = size * 2 * lres + 1;

	vCenter[2] = 0;

	Vec3d	extent = Vec3d(1, 1, 0) * size * (fScale / fRes);
	Vec3d	minpos = vCenter - extent;
	Vec3d	maxpos = vCenter + extent;
	Vec3d	minTPos = getTerrainSet().getMinPos();
	Vec3d	maxTPos = getTerrainSet().getMaxPos();
	if (maxTPos[1] - minTPos[1] > 359.0)
	{
		minTPos[1] = g_Draw.getGeoMin();
		maxTPos[1] = g_Draw.getGeoMax();
	}

	minpos = Vec3d(max(minpos[0], minTPos[0]),
		getProjection() == PROJ_GLOBE ? minpos[1] : max(minpos[1], minTPos[1]),	0);
	maxpos = Vec3d(min(maxpos[0], maxTPos[0]),
			getProjection() == PROJ_GLOBE ? maxpos[1] : min(maxpos[1], maxTPos[1]),	0);

	//  offset to on lat/lon line
	minpos = Vec3d(minpos[0] - fmod(minpos[0], (fScale / fRes)), minpos[1] - fmod(minpos[1], (fScale / fRes)), 0);

	float	fStep = (fScale / fRes);
	float	fResStep = fStep / lres;

	if (minpos != lastminpos || maxpos != lastmaxpos || fScale != lastScale || size != lastSize)
	{
		if (pRows) delete pRows;
		pRows = new Vec3f[cntRows * cntPnts];
		if (pCols) delete pCols;
		pCols = new Vec3f[cntCols * cntPnts];

		double	y, x;
		float	fDepth;
		int		r, c, i;
		int		ndx = 0;
		for (r = 0, y = minpos[0]; r < cntRows; y += fStep, r++)
		{
			for (i = 0, x = minpos[1]; i < cntPnts; x += fResStep, i++)
			{
				double	dLat = y, dLon = x;
				getTerrainSet().getPosHeight(dLat, dLon, fDepth = 0.0);
				pRows[ndx++] = g_Draw.getScenePntFromGPS(Vec3d(dLat, dLon, fDepth));
			}
		}

		ndx = 0;
		for (c = 0, x = minpos[1]; c < cntCols; x += fStep, c++)
		{
			for (i = 0, y = minpos[0]; i < cntPnts; y += fResStep, i++)
			{
				double	dLat = y, dLon = x;
				getTerrainSet().getPosHeight(dLat, dLon, fDepth = 0.0);
				pCols[ndx++] = g_Draw.getScenePntFromGPS(Vec3d(dLat, dLon, fDepth));
			}
		}

		lastminpos = minpos;
		lastmaxpos = maxpos;
		lastScale = fScale;
		lastSize = size;
	}

	if (bLabels)
	{
		double	dCamDist = g_Draw.getCameraDistKM();
		float	text_scale = dCamDist / MetersPerDegree * .8;

		float	fStep = (fScale / fRes);
		double label_r_ndx = (double)((int)((vCenter[1]-minpos[1])/fStep));
		double label_c_ndx = (double)((int)((vCenter[0]-minpos[0])/fStep));

		double	y = minpos[0], x = minpos[1];
		int ndx = label_r_ndx * lres;
		for (int r = 0; r < cntRows; y += fStep, r++, ndx += cntPnts)
		{
			if (pRows[ndx] == NODATA || r == label_c_ndx)
				continue;
			string	strLabel = getDegMinSecString(y, true, false, true);
			g_Draw.drawScaledText(strLabel, clr, pRows[ndx], text_scale);
		}

		ndx = label_c_ndx * lres;
		for (int c = 0; c < cntCols; x += fStep, c++, ndx += cntPnts)
		{
			if (pCols[ndx] == NODATA)
				continue;
			string	strLabel = getDegMinSecString(x, true, false, true);
			g_Draw.drawScaledText(strLabel, clr, pCols[ndx], text_scale);
		}
	}

	glColor4ub(clrR(clr), clrG(clr), clrB(clr), clrA(clr)/2);
	glLineWidth(2);

	int ndx = 0;
	for (int r = 0; r < cntRows; r++)
	{
		glBegin(GL_LINE_STRIP);
		for (int c = 0; c < cntPnts; c++)
			if (pRows[ndx][2] != NODATA)
				glVertex(pRows[ndx++]);
		glEnd();
	}

	ndx = 0;
	for (int c = 0; c < cntCols; c++)
	{
		glBegin(GL_LINE_STRIP);
		for (int r = 0; r < cntPnts; r++)
			if (pCols[ndx][2] != NODATA)
				glVertex(pCols[ndx++]);
		glEnd();
	}

}

/* =============================================================================
 =============================================================================== */
void CTerrain::drawOutlines()
{
	vector<Vec3d>	vList;

	for (int i = 0; i < getTerrainSet().getActiveLayerCnt(); i++)
	{
		CTerrainLayer	&Table = getTerrainSet().getActiveLayer(i);
		vList.push_back(Table.getMinPos());
		vList.push_back(Table.getMaxPos());
	}

	drawOutlineList(vList, 0xcc008888);
}

/* =============================================================================
 =============================================================================== */
void CTerrain::drawOutlineList(vector<Vec3d> vList, color clr)
{
	glColor4ub(clrR(clr), clrG(clr), clrB(clr), clrA(clr));
	glLineWidth(2);

	for (int t = 0; t < vList.size() / 2; t++)
	{
		Vec3d	minPos = vList[t * 2];
		Vec3d	maxPos = vList[t * 2 + 1];

		for (int i = 0; i < 2; i++)
		{
			double	dLat = i == 0 ? minPos[0] : maxPos[0];
			double	dInc = (maxPos[1] - minPos[1]) / 10.;
			if (fabs(dInc) < 1e-10)
				continue;
			glBegin(GL_LINE_STRIP);
			for (double dLon = minPos[1]; dLon <= maxPos[1] + dInc / 2; dLon += dInc)
			{
				float	fDepth = 0.0;
				getTerrainSet().getPosHeight(dLat, dLon, fDepth);

				Vec3f	pos = g_Draw.getScenePntFromGPS(Vec3d(dLat, dLon, fDepth));
				glVertex(pos);
			}

			glEnd();
		}

		for (int i = 0; i < 2; i++)
		{
			double	dLon = i == 0 ? minPos[1] : maxPos[1];
			double	dInc = (maxPos[0] - minPos[0]) / 10.;
			if (fabs(dInc) < 1e-10)
				continue;
			glBegin(GL_LINE_STRIP);
			for (double dLat = minPos[0]; dLat <= maxPos[0] + dInc / 2; dLat += dInc)
			{
				float	fDepth = 0.0;
				getTerrainSet().getPosHeight(dLat, dLon, fDepth);

				Vec3f	pos = g_Draw.getScenePntFromGPS(Vec3d(dLat, dLon, fDepth));
				glVertex(pos);
			}

			glEnd();
		}
	}
}
