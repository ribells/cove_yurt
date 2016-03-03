/* =============================================================================
	File: terrain_table.cpp

 =============================================================================== */
#include <FL/Fl_JPEG_Image.H>
#include <FL/Fl_PNG_Image.H>
#include <FL/Fl_BMP_Image.H>

#include "string.h"
#include "utility.h"
#include "xmlParser.h"
#include "scene/image.h"
#include "web/web_file.h"

#include "terrain.h"

void	setMeshCenterGPS(Vec3d v);
Vec3d	getMeshCenterGPS();

/* =============================================================================
 =============================================================================== */

bool CTextureLayer::update(string name, string link, string id, bool bActive)
{
	setName(!name.size() ? getFileNameBase(link) : name);
	setLink(link);
	setID(id);
	return setActive(bActive);
}

/* =============================================================================
 =============================================================================== */
bool CTextureLayer::setActive(bool bActive)
{
	string	strSupported[] = { ".dds", ".bmp", ".jpg", ".jpeg", ".png", ".mp4", "" };
	string	strLocal;

	if (!bActive)
	{
		setLoaded(false);
		clean();
		return true;
	}

	//  if this is a gradient update the map
	if (getIsGrad())
	{
		if (!editGradient().updateMap())
			return false;
	}
	else if (getIsTiled())	//  if this is tiled, then download base image
	{
		string	strLocal, strURL;

		//  download overall map ahead of time
		if (!getTiledTextureURL(strURL, 0, 0, 0))
			return false;
		if (!getTiledTexturePath(strLocal, 0, 0, 0))
			return false;
		if (strURL.size() && strLocal.size()) 
			getWebFile(strURL, strLocal);
	}
	else	//  download image file to local cache
	{
		g_Env.m_DownloadName = m_Name;

		string	strTmp = cacheWebFiles(m_Link, "textures", strSupported);
		g_Env.m_DownloadName = "";
		if (!strTmp.size())
			return false;
		strLocal = strTmp;
		if (getExt(strLocal) == ".mp4")
			openVideo(strLocal);
	}

	setLocalFilePath(strLocal);

	if (strLocal == "") strLocal = m_Name;
	//cout << "\nFrom terrain_table.cpp - loading texture: " + strLocal;

	setLoaded(true);
	return true;
}

/* =============================================================================
 =============================================================================== */
bool CTextureLayer::getImageTexturePath(string &path, int level) const
{
	//  set the local texture path name
	if (!getIsImage() || level > 0)
		return false;
	if (getLink().substr(0, 7) == "datasvr" || getLink().substr(0, 5) == "http:")
		path = g_Env.m_LocalCachePath + "/textures/" + getFileName(getLink());
	else
		path = getLocalFilePath();
	return true;
}

/* =============================================================================
 =============================================================================== */
bool CTextureLayer::getTiledTexturePath(string &path, int level, int row, int col) const
{
	if (!getIsTiled())
		return false;
	if (level == 0)
	{
		if (getLink().find("http:") == string::npos && getEarthImage())
			path = g_Env.m_LocalCachePath + "/textures/" + getID() + "/" + getID() + "." + getImageExt();
	}
	else
	{
		if (level - 1 > getMaxLevel() || level - 1 < getMinLevel())
			return false;
		else
		{
			char	buf[1024];
			string	strpath;
			if (getIsLocalFile(getLink()))
				strpath = getLink() + "/%s/%i/%.4i/%.4i_%.4i." + getImageExt();
			else
				strpath = g_Env.m_LocalCachePath + "/textures/%s/%i/%.4i/%.4i_%.4i." + getImageExt();
			sprintf(buf, strpath.c_str(), getID().c_str(), level - 1, row, row, col);
			path = buf;
		}
	}

	return true;
}

/* =============================================================================
    only called for tiled image sets
 =============================================================================== */
bool CTextureLayer::getTiledTextureURL(string &path, int level, int row, int col)
{
	if (getID() == "" || getLink() == "" || getName() == "")
		return false;
	if (level > 0 && (level - 1 > getMaxLevel() || level - 1 < getMinLevel())) 
		return false;

	char	buf[2048] = "";
	if (getLink().find("http:") != string::npos)	//  WMS map server
	{
		if (getLink().find("worldwind25") != string::npos)	//  NASA map server
		{
			if (getImageType() == "") 
				setImageType("image/jpeg");
			if (level != 0)
				sprintf(buf, "%s?T=%s&L=%i&X=%i&Y=%i", getLink().c_str(), getID().c_str(), level - 1, col, row);
			path = buf;
			return true;
		}
		//  figure out image type if not set
		if (getImageType() == "")
		{

			//  getCapabilities from server
			string	strPath = g_Env.m_LocalCachePath + "/textures/" + getName() + "/capabilities.xml";
			if (!WMS_GetCapabilities(getLink(), strPath))
			{
				cout << ("Map Server does not appear to be available");
				return false;
			}

			//  find format to use
			string	strImageType;
			if (!WMS_GetImageFormat(strPath, strImageType))
			{
				cout << ("Cannot find image format");
				return false;
			}

			setImageType(strImageType);

			vector<string>	strTitles;
			vector<string>	strNames;
			bool			bFound = false;
			if (WMS_GetMapNames(strPath, strTitles, strNames))
			{
				for (int i = 0; i < strNames.size() && !bFound; i++)
					if (strNames[i] == getID()) bFound = true;
			}

			if (!bFound)
			{
				cout << ("Could not find map at web map server site.");
				return false;
			}
		}

		//  request the map tile
		string	strRequest = "&SERVICE=WMS&VERSION=1.1.1&REQUEST=GetMap&SRS=EPSG:4326&TRANSPARENT=TRUE&FORMAT=" +
			getImageType();
		if (getID().find("STYLES=") == string::npos) strRequest += "&STYLES=";

		double	minLon = -180, maxLon = 180;
		double	minLat = -90, maxLat = 90;
		int		width = 1024, height = 512;
		if (level > 0)
		{
			double	fOff = 36.0 / (1 << (level - 1));
			minLon = -180 + col * fOff;
			maxLon = minLon + fOff;
			minLat = -90 + row * fOff;
			maxLat = minLat + fOff;
			width = 512;
		}

		sprintf(buf,"%s?LAYERS=%s%s&WIDTH=%i&HEIGHT=%i&BBOX=%lf,%lf,%lf,%lf",
			getLink().c_str(),getID().c_str(),strRequest.c_str(),width,	height,	minLon,	minLat,	maxLon,	maxLat);
	}
	else
	{
		if (getEarthImage())
			setImageType("image/dds");
		else
			setImageType("image/png");

		if (level == 0)
			sprintf(buf, "%s/%s/%s.%s", getLink().c_str(), getID().c_str(), getID().c_str(), getImageExt().c_str());
		else
			sprintf(buf, "%s/%s/%i/%.4i/%.4i_%.4i.%s", getLink().c_str(), getID().c_str(), level - 1, row, row, col, getImageExt().c_str());
	}

	path = buf;
	return true;
}


/* =============================================================================
 =============================================================================== */
bool CTextureLayer::openVideo(string FileName)
{
	/*
	//do one call to get the real size of the movie
	CMovie * tmpMoviePtr = new CMovie();
	if (!tmpMoviePtr->OpenMovie(FileName.c_str(), 256, 256))
		return false;
	int w, h;
	tmpMoviePtr->getMovieSize(w, h);
	delete tmpMoviePtr;

	//now load the real movie
	m_MoviePtr = new CMovie();
	if (!m_MoviePtr->OpenMovie(FileName.c_str(), w, h))
	{
		delete m_MoviePtr;
		m_MoviePtr = NULL;
		return false;
	}
	return true;
	*/
	return false;
}

bool CTextureLayer::setVideoFrame(double dtime)
{
	/*
	if (!getIsImage() || m_MoviePtr == NULL)
		return false;

	double movie_tm = 0.0;
	if (getStart() != NO_TIME || getFinish() != NO_TIME)
	{
		double dDur = getFinish()-getStart();
		if (getLoop())
			dtime = getStart() + fmod(dtime, dDur);
		else
			dtime = min(getFinish(), max(getStart(), dtime));
		movie_tm = (dtime-getStart()) / (getFinish()-getStart());
		movie_tm  = movie_tm * m_MoviePtr->getMovieDuration();
	}
	return updateVideoTexture(m_MoviePtr, movie_tm);
	*/
	return false;
}

/* =============================================================================
 =============================================================================== */
bool CGradient::addFile(string file, double fMin, double fMax)
{
	if (!file.size())
		return false;
	m_File.push_back(file);
	m_Min.push_back(fMin);
	m_Max.push_back(fMax);
	return true;
}

/* =============================================================================
 =============================================================================== */
color CGradient::getColor(double dVal) const
{
	dVal = dVal * (double) (getBmpWidth() - 1);

	unsigned char	*pClr = getBmp() + ((int) floor(dVal)) * 4;
	color			clr = pClr ? clrRGBA(pClr[0], pClr[1], pClr[2], pClr[3]) : CLR_WHITE;
	return clr;
}

/* =============================================================================
 =============================================================================== */
bool CGradient::updateMap()
{
	vector<Fl_RGB_Image *>	img;

	m_Depth.clear();
	m_MapWidth = m_MapHeight = 0;
	if (m_Map) delete[] m_Map;
	m_Map = NULL;
	m_TxVal.clear();

	//  load the images
	int	iWidth = 0;
	Fl_RGB_Image	*pImg;
	for (int i = 0; i < getCnt(); i++)
	{
		string	strSupported[] = { ".bmp", ".jpg", ".png", "" };
		string	strLocal = cacheWebFiles(m_File[i], "textures", strSupported);
		//cout << "\nFrom terrain_table.cpp: - loading gradient: " + strLocal;

		if (getExt(strLocal) == ".jpg" || getExt(strLocal) == ".jpeg")
			pImg = new Fl_JPEG_Image(strLocal.c_str());
		else if (getExt(strLocal) == ".png")
			pImg = new Fl_PNG_Image(strLocal.c_str());
		else
			pImg = new Fl_BMP_Image(strLocal.c_str());
		if (pImg->d() == 0)
		{
			cout << "\nError loading gradient file: " + m_File[i];
			break;
		}

		iWidth += pImg->w();
		img.push_back(pImg);
	}

	if (img.size() < getCnt() || iWidth == 0 || (!mem_alloc(m_Map, iWidth * 4)))
	{
		for (int i = 0; i < getCnt(); i++) {
			delete img[i];
		}
		return false;
	}

	m_MapHeight = 1;

	vector<double>	fOffset;
	for (int i = 0; i < getCnt(); i++)
	{
		if (img[i]->d() == 0)
			continue;

		int	w = img[i]->w();
		const unsigned char *pData = (const unsigned char *) img[i]->data()[0];
		if (img[i]->d() == 3) 
			pData = makeRGBA(pData, img[i]->w(), img[i]->h());

		double	fMin = m_Min[i] == -32000 ? EARTHMIN : m_Min[i];
		double	fMax = m_Max[i] == 32000 ? EARTHMAX : m_Max[i];
		fMin = min(fMax, fMin);

		// first figure out index of min and max for this gradient in current gradient
		int idxIn = 0;
		for (idxIn = 0; idxIn < getMapCnt(); idxIn++)
			if (fMin < getMapDepth(idxIn))
				break;

		int offIn;
		if (idxIn >= getMapCnt())
			offIn = m_MapWidth;
		else if (idxIn == 0 && fMin < getMapDepth(0))
			offIn = 0;
		else
		{
			offIn = (int)(fOffset[idxIn - 1] +
				((fOffset[idxIn] - fOffset[idxIn - 1]) * (fMin - getMapDepth(idxIn - 1)) 
				/ (getMapDepth(idxIn) - getMapDepth(idxIn - 1))));
		}

		int idxOut = 0;
		for (idxOut = 0; idxOut < getMapCnt(); idxOut++)
			if (fMax <= getMapDepth(idxOut))
				break;

		int offOut;
		if (idxOut >= getMapCnt())
			offOut = m_MapWidth;
		else if (idxOut == 0 && fMax <= getMapDepth(0))
			offOut = 0;
		else
		{
			offOut = (int)(fOffset[idxOut - 1] +
				((fOffset[idxOut] - fOffset[idxOut - 1]) * (fMax - getMapDepth(idxOut - 1))
				/ (getMapDepth(idxOut) - getMapDepth(idxOut - 1))));
		}

		//  create opening for new gradient and copy it in
		memmove(m_Map + (offIn + w) * 4, m_Map + offOut * 4, (m_MapWidth - offOut) * 4);
		memmove(m_Map + offIn * 4, pData, w * 4);

		int iInsertSize = (w - (offOut - offIn));
		m_MapWidth += iInsertSize;

		//  update depths and indices arrays
		fOffset.insert(fOffset.begin() + idxIn, offIn);
		fOffset.insert(fOffset.begin() + idxIn + 1, offIn + w);

		m_Depth.insert(m_Depth.begin() + idxIn, fMin);
		m_Depth.insert(m_Depth.begin() + idxIn + 1, fMax);

		for (int j = idxIn + 2; j < getMapCnt(); j++) 
			fOffset[j] += iInsertSize;

		//  collapse any redundant values
		for (int j = getMapCnt() - 1; j > 0; j--)
		{
			if (getMapDepth(j) <= getMapDepth(j - 1))
			{
				fOffset.erase(fOffset.begin() + j);
				m_Depth.erase(m_Depth.begin() + j);
			}
		}

		if (img[i]->d() == 3) delete pData;
	}

	for (int i = 0; i < getMapCnt(); i++) 
		m_TxVal.push_back(fOffset[i] / m_MapWidth);
	for (int i = 0; i < getCnt(); i++) 
		delete img[i];

	return true;
}

/* =============================================================================
 =============================================================================== */
unsigned char *CGradient::createGradBitmap(float *pGradVals, int iCnt, bool bLine, bool bBins, bool bFlip, bool bLog)
{
	unsigned char	*pData = getGradMap();
	int				w = m_BmpWidth = getMapWidth();
	int				h = m_BmpHeight = 256;

	if (w <= 0 || !pData)
		return NULL;
	if (m_Bmp) delete m_Bmp;
	if (!mem_alloc(m_Bmp, w * h * 4))
		return NULL;

	//  fill in first line of bitmap
	if (bFlip)
	{
		for (int i = 0; i < w; i++)
			for (int j = 0; j < 4; j++) 
				m_Bmp[(w - i - 1) * 4 + j] = *pData++;
	}
	else
		memcpy(m_Bmp, pData, w * 4);

	if (bLog)
	{
		for (int i = 0; i < w; i++)
		{
			int ndx = (1.0f - pow((float) (w - i) / (float) w, 2.71828f)) * w;
			for (int j = 0; j < 4; j++) 
				m_Bmp[i * 4 + j] = m_Bmp[ndx * 4 + j];
		}
	}

	//  fix up lines and bins
	if (bLine || bBins)
	{
		bool	bEvenSpace = false;
		if (!pGradVals && iCnt > 0)
		{
			bEvenSpace = true;
			if (!mem_alloc(pGradVals, iCnt + 1))
				return false;

			float	fStep = 1.0f / (float) (iCnt);
			float	fVal = 0;
			for (int i = 0; i < iCnt; i++, fVal += fStep) 
				pGradVals[i] = fVal;
			pGradVals[iCnt] = 1.0;
		}

		if (pGradVals)
		{
			int prevNdx = -1;
			for (int i = 0; i <= iCnt; i++)
			{
				int ndxBase = (int) (pGradVals[i] * (float) w);
				int ndxCur = max(0, min(w - 1, ndxBase));

				if (bBins && prevNdx != -1)
				{
					int				ndxMid = (ndxCur - prevNdx) / 2 + prevNdx;
					unsigned char	*clr = m_Bmp + ndxMid * 4;

					for (int n = prevNdx + 1; n <= ndxCur; n++)
						for (int j = 0; j < 4; j++) 
							*(m_Bmp + (n * 4) + j) = clr[j];
				}

				float	fFactor = .75;
				if (bLine && ndxBase > 0 && ndxBase < w - 1)
				{
					for (int j = 0; j < 3; j++)
						*(m_Bmp + (ndxCur * 4) + j) *= fFactor;
				}

				prevNdx = ndxCur;
			}

			if (bEvenSpace && pGradVals) delete pGradVals;
		}
	}

	//  fill out entire bitmap
	unsigned char	*pch = m_Bmp + w * 4;
	for (int i = 1; i < h; i++, pch += w * 4) 
		memcpy(pch, m_Bmp, w * 4);
	return m_Bmp;
}

/* =============================================================================
 =============================================================================== */
bool CTerrainSet::setMeshCenterGPS(Vec3d vPos)
{
	if (vPos == ::getMeshCenterGPS() && vPos == m_MeshCenterGPS)
		return false;
	m_MeshCenterGPS = vPos;
	::setMeshCenterGPS(vPos);
	return true;
}

/* =============================================================================
 =============================================================================== */
Vec3d CTerrainSet::getMeshCenterGPS() const
{
	return m_MeshCenterGPS;
}

/* =============================================================================
 =============================================================================== */
void CTerrainSet::updateActiveLayerList()
{
	//create a list of active layers to speed up terrain creation
	m_ActiveLayerList.clear();
	for (int i = 0; i < getTerrainLayerCnt(); i++)
	{
		if (!getTerrainLayer(i).getActive())
			continue;
			//order the layers from highest resolution to lowest resolution
		bool bInserted = false;
		for (int j = 0; j < m_ActiveLayerList.size() && !bInserted; j++)
			if (getTerrainLayer(i).getCellSizeX() < m_ActiveLayerList[j]->getCellSizeX())
			{
				m_ActiveLayerList.insert(m_ActiveLayerList.begin()+j, getTerrainLayerPtr(i));
				bInserted = true;
			}
		if (!bInserted)
			m_ActiveLayerList.push_back(getTerrainLayerPtr(i));
	}

	//  reset minpos and maxpos
	m_MaxPos = Vec3d(-1e8, -1e8, -1e8);
	m_MinPos = Vec3d(1e8, 1e8, 1e8);
	for (int i = 0; i < getActiveLayerCnt(); i++)
	{
		CTerrainLayer	&hLayer = getActiveLayer(i);

		Vec3d	minTmp = hLayer.getMinPos();
		Vec3d	maxTmp = hLayer.getMaxPos();
		for (int j = 0; j < 3; j++)
		{
			if (minTmp[j] < m_MinPos[j]) m_MinPos[j] = minTmp[j];
			if (maxTmp[j] > m_MaxPos[j]) m_MaxPos[j] = maxTmp[j];
		}
	}

	if (m_MaxPos == Vec3d(-1e8, -1e8, -1e8))
		m_MinPos = m_MaxPos = vl_0;
}

/* =============================================================================
 =============================================================================== */
bool CTerrainSet::getMinCellSize(double	m_LonMin, double m_LatMin, double m_LonMax, double m_LatMax, double &fMinCell, int *pTable)
{
	fMinCell = 100.0;
	for (int i = 0; i < getActiveLayerCnt(); i++)
	{
		CTerrainLayer	&hLayer = getActiveLayer(i);

		Vec3d	minPos = hLayer.getMinPos();
		Vec3d	maxPos = hLayer.getMaxPos();
		if ((m_LonMin <= maxPos[1] && m_LonMax >= minPos[1] && m_LatMin <= maxPos[0] && m_LatMax >= minPos[0])
			&&	(hLayer.getCellSizeX() < fMinCell || hLayer.getCellSizeY() < fMinCell))
		{
			fMinCell = min(hLayer.getCellSizeX(), hLayer.getCellSizeY());
			if (pTable) *pTable = i;
		}
	}

	return(fMinCell < 100.0);
}

/* =============================================================================
 =============================================================================== */
int CTerrainSet::getMaxLevel(double m_LonMin, double m_LatMin, double m_LonMax, double m_LatMax)
{
	double	cell_size;
	if (!getMinCellSize(m_LonMin, m_LatMin, m_LonMax, m_LatMax, cell_size))
		return -1;

	int   max_level;
	float grid_size = 20.0;
	for (max_level = 0; grid_size > cell_size; grid_size /= 2, max_level++);
	return max_level;
}

/* =============================================================================
 =============================================================================== */
inline float CatmullRomVal(float t, float *p)
{
	if (ISNODATA(p[1]) || ISNODATA(p[2]))
		return NODATA;

	if (ISNODATA(p[0])) p[0] = p[1];
	if (ISNODATA(p[3])) p[3] = p[2];

	float	t2 = t * t;
	float	t3 = t2 * t;
	return	0.5 *	((2.0 * p[1]) 
				+ t * (-p[0] + p[2]) 
				+ t2 * (2.0 * p[0] - 5.0 * p[1] + 4.0 * p[2] - p[3]) 
				+ t3 * (-p[0] + 3.0 * p[1] - 3.0 * p[2] + p[3]));
}

/* =============================================================================
 =============================================================================== */
bool CTerrainSet::getGridPosHeight(double dLat, double dLon, float &fDepth)
{
	fDepth = NODATA;
	if (dLon < -180) dLon += 360;
	if (dLon > 180) dLon -= 360;

	for (int i = 0; i < getActiveLayerCnt(); i++)
	{
		CTerrainLayer	&hLayer = getActiveLayer(i);

		Vec3d	minPos = hLayer.getMinPos();
		Vec3d	maxPos = hLayer.getMaxPos();
		if (hLayer.getWrap() || (dLon <= maxPos[1] && dLon >= minPos[1] && dLat <= maxPos[0] && dLat >= minPos[0]))
		{
			if (hLayer.getPosHeight(dLat, dLon, fDepth))
				break;
		}
	}

	return !ISNODATA(fDepth);
}

/* =============================================================================
 =============================================================================== */
bool CTerrainSet::getPosHeight(double dLat, double dLon, float &fDepth)
{
	fDepth = NODATA;
	if (dLon < -180) dLon += 360;
	if (dLon > 180) dLon -= 360;

	int		iTable;
	double	fMin;
	if (!getMinCellSize(dLon, dLat, dLon, dLat, fMin, &iTable))
		return false;
	CTerrainLayer	&Table = getActiveLayer(iTable);
	double lonInc = Table.getCellSizeX();
	double latInc = Table.getCellSizeY();

	double dLonStart = Table.getMinPos()[1];
	double dLonOff = (dLon - dLonStart) / lonInc;
	double lonMin = dLonStart + lonInc * (floor(dLonOff) - 1);
	float  fLonGridPos = dLonOff - floor(dLonOff);

	double dLatStart = Table.getMinPos()[0];
	double dLatOff = (dLat - dLatStart) / latInc;
	double latMin = dLatStart + latInc * (floor(dLatOff) - 1);
	float  fLatGridPos = dLatOff - floor(dLatOff);

	float row[4], col[4];
	double lat = latMin;
	for (int r = 0; r < 4; r++, lat += latInc)
	{
		double lon = lonMin;
		for (int c = 0; c < 4; c++, lon += lonInc)
			getGridPosHeight(lat, lon, row[c]);
		col[r] = CatmullRomVal(fLonGridPos, row);
	}
	fDepth = CatmullRomVal(fLatGridPos, col);
	
	return !ISNODATA(fDepth);
}

/* =============================================================================
 =============================================================================== */
bool CTerrainSet::getGrid(double dLonMin,double dLonMax,double dLatMin, double dLatMax,
						  float *pTerrain, Vec3f *pNormals, int iCols, int iRows)
{
	double lonMin, lonMax, latMin, latMax;
	double dLonInc = (dLonMax - dLonMin) / (double) (iCols - 1);
	double dLatInc = (dLatMax - dLatMin) / (double) (iRows - 1);
	lonMin = dLonMin - dLonInc;
	lonMax = dLonMax + dLonInc;
	latMin = dLatMin - dLatInc;
	latMax = dLatMax + dLatInc;

	int  iVCols = iCols + 2;
	int  iVRows = iRows + 2;

		//recalculate terrain area to sample if oversampling
	int		iTable;
	double	fMin;
	if (!getMinCellSize(dLonMin, dLatMin, dLonMax, dLatMax, fMin, &iTable))
		return false;
	CTerrainLayer	&Table = getActiveLayer(iTable);
	double cellX = Table.getCellSizeX();
	double cellY = Table.getCellSizeY();
	bool bOverSample = fMin >= ((dLonMax - dLonMin) / (double) (iCols - 1));

	if (bOverSample)
	{
		double	dLonStart = Table.getMinPos()[1];
		int		idx0 = floor((lonMin - dLonStart) / cellX) - 1;
		lonMin = dLonStart + cellX * idx0;
		int idx1 = ceil((lonMax - dLonStart) / cellX) + 1;
		lonMax = dLonStart + cellX * idx1;
		iVCols = idx1 - idx0 + 1;

		double	dLatStart = Table.getMinPos()[0];
		idx0 = floor((latMin - dLatStart) / cellY) - 1;
		latMin = dLatStart + cellY * idx0;
		idx1 = ceil((latMax - dLatStart) / cellY) + 1;
		latMax = dLatStart + cellY * idx1;
		iVRows = idx1 - idx0 + 1;
	}

	int		iNewCols = iVCols;
	int		iNewRows = iVRows;
	float	*pNew = NULL;
	if (!mem_alloc(pNew, iNewCols * iNewRows)) 
		return false;

	//  get terrain grid to work from
	float	*pTmp = pNew;
	double	latInc = (latMax - latMin) / (double) (iNewRows - 1);
	double	lonInc = (lonMax - lonMin) / (double) (iNewCols - 1);

	double lat = latMin;
	for (int r = 0; r < iNewRows; r++, lat += latInc)
	{
		double lon = lonMin;
		for (int c = 0; c < iNewCols; c++, lon += lonInc, pTmp++)
			getGridPosHeight(lat, lon, *pTmp);
	}

	//  if oversampling, need to interpolate to get real values
	if (bOverSample)
	{
		float	*pBase = pNew;
		int		iBaseCols = iNewCols;
		iNewCols = iCols + 2;
		iNewRows = iRows + 2;
		if (!mem_alloc(pNew, iNewCols * iNewRows))
		{
			delete[] pBase;
			return false;
		}

		float	col[4];

		double	dLonPos = dLonMin - dLonInc;
		for (int c = 0; c < iNewCols; c++, dLonPos += dLonInc)
		{
			double	xCell = (dLonPos - lonMin) / cellX;
			int		c_ndx = floor(xCell);
			xCell -= c_ndx;

			double	dLatPos = dLatMin - dLatInc;
			int last_r_ndx = -4;
			for (int r = 0; r < iNewRows; r++, dLatPos += dLatInc)
			{
				double	yCell = (dLatPos - latMin) / cellY;
				int		r_ndx = floor(yCell);
				yCell -= r_ndx;

				//  spline based interpolation - try to re-use row values
				int start_offset = r_ndx - last_r_ndx;
				if (start_offset > 0)
				{
					float *p_row = pBase + (r_ndx - 1) * iBaseCols + c_ndx - 1;
					for (int i = 0; i < 4; i++, p_row += iBaseCols)
						if (i < 4 - start_offset)
							col[i] = col[i + start_offset];
						else
							col[i] = CatmullRomVal(xCell, p_row);
				}
				pNew[r*iNewCols+c] = CatmullRomVal(yCell, col);

				last_r_ndx = r_ndx;
			}
		}

		delete[] pBase;
	}

	//  Copy over terrain from pNew
	for (int r = 1; r < iNewRows - 1; r++)
	{
		float	*pTmp = pNew + r * iNewCols + 1;
		for (int c = 1; c < iNewCols - 1; c++, pTmp++) 
			*pTerrain++ = *pTmp;
	}

	//  create normals for entire grid
	if (pNormals)
	{
		float fCellSize = MetersPerDegree * (dLonMax - dLonMin) / (double) (iCols);	//  Meters Per cell
		createGridNormals(pNew, iNewRows, iNewCols, fCellSize, pNormals);
	}

	delete[] pNew;

	return true;
}

/* =============================================================================
 =============================================================================== */
bool CTerrainSet::createGridNormals(float *pNew, int iNewRows, int iNewCols, float fCellSize, Vec3f *pNormals)
{
	//  build up normals from face normals
	float fac = 0.5 * fCellSize;
	float fac2 = fCellSize * fCellSize;
	for (int r = 1; r < iNewRows - 1; r++)
	{
		float	*pTmp = pNew + r * iNewCols + 1;
		for (int c = 1; c < iNewCols - 1; c++, pTmp++)
		{
			float	p0 = *pTmp;
			float	p1 = *(pTmp - iNewCols);
			float	p2 = *(pTmp + 1);
			float	p3 = *(pTmp + iNewCols);
			float	p4 = *(pTmp - 1);
			*pNormals++ = Vec3f((ISNODATA(p4)||ISNODATA(p2)) ? 0 : fac*(p4-p2),
								(ISNODATA(p1)||ISNODATA(p0)) ? 0 : fac*(p1-p0),
								fac2);
			//they will be normalized later in the projectVert call

			 //derivation  - the face normals
//			Vec3f fnrm0 = cross((Vec3f(0, -fCellSize, p1 - p0)), (Vec3f(fCellSize, 0, p2 - p0)));
//			Vec3f fnrm1 = cross((Vec3f(fCellSize, 0, p2 - p0)), (Vec3f(0, fCellSize, p3 - p0)));
//			Vec3f fnrm2 = cross((Vec3f(0, fCellSize, p3 - p0)), (Vec3f(-fCellSize, 0, p4 - p0)));
//			Vec3f fnrm3 = cross((Vec3f(-fCellSize, 0, p4 - p0)), (Vec3f(0, -fCellSize, p1 - p0)));
			 //the optimized face normals
//			Vec3f fnrm0 = Vec3f(p0-p2, p1-p0, fCellSize);
//			Vec3f fnrm1 = Vec3f(p0-p2, p3-p0, fCellSize);
//			Vec3f fnrm2 = Vec3f(p4-p0, p0-p3, fCellSize);
//			Vec3f fnrm3 = Vec3f(p4-p0, p1-p0, fCellSize);
//			*pNormals++ = (fnrm0+fnrm1+fnrm2+fnrm3) * fCellSize / 4.0;
		}
	}

	return true;
}

/* =============================================================================
 =============================================================================== */
bool CTerrainLayer::update(string name, string link, string id, bool bActive)
{
	setName(!name.size() ? getFileNameBase(link) : name);
	setLink(link);

	return setActive(bActive);
}

/* =============================================================================
 =============================================================================== */
bool CTerrainLayer::setActive(bool bActive)
{
	string	strSupported[] = { ".bil", ".asc", ".txt", ".grd", "" };

	if (!bActive)
	{
		clean();
		setLoaded(false);
		return true;
	}

	//  Load the data based on the file extension
	g_Env.m_DownloadName = m_Name;

	string	strLocal = cacheWebFiles(m_Link, "terrain", strSupported);
	g_Env.m_DownloadName = "";
	if (!strLocal.size())
		return false;

	setLocalFilePath(strLocal);

	//  Load the data based on the file extension
	cout << " - loading terrain: " + strLocal;

	bActive = false;
	if (getExt(strLocal) == ".bil" || getExt(strLocal) == ".grd" || getExt(strLocal) == ".asc")
		bActive = readTerrainFile(strLocal);
	else
		cout << ("File format not recognized.  Aborting load.");

	if (!bActive) 
		cout << (" Failure reading file.");

	setLoaded(bActive);
	return bActive;
}

/* =============================================================================
    check if in bounds
 =============================================================================== */
bool CTerrainLayer::getPosHeight(double dLat, double dLon, float &fDepth)
{
	static CTerrainLayer	*pLast = NULL;
	bool bSame = (this == pLast);
	pLast = this;
	fDepth = NODATA;

	//  if above or below terrain bounds then return no data
	if (dLat < m_MinPos[0] - m_CellSizeY || dLat > m_MaxPos[0] + m_CellSizeY)
		return false;	//  not in object bounds

	//  same for for right or left except need to check if we can wrap
	if (dLon < m_MinPos[1] - m_CellSizeX || dLon > m_MaxPos[1] + m_CellSizeX)
	{
		if (m_MinPos[1] > -180. - m_CellSizeX / 2 && m_MaxPos[1] < 180. + m_CellSizeX / 2)
			return false;
		if (dLon < m_MinPos[1])
			dLon += 360.;
		else if (dLon > m_MaxPos[1])
			dLon -= 360.;
	}

	//  clamp values to terrain max/min
	double	xCell = max(min((dLon - m_MinPos[1]) / m_CellSizeX, (double) (m_Cols - 1)), 0.0);
	double	yCell = max(min((m_MaxPos[0] - dLat) / m_CellSizeY, (double) (m_Rows - 1)), 0.0);

	//  get grid indices
	int		new_xOff = (int) xCell;
	int		new_yOff = (int) yCell;

	//  reset these vars to be the cell offset
	xCell -= new_xOff;
	yCell -= new_yOff;

	static int xOff = NODATA;
	static int yOff = NODATA;

	float val = 0;
	static float	p[4];

	if (!bSame || new_xOff != xOff || new_yOff != yOff)
	{
		xOff = new_xOff;
		yOff = new_yOff;

		float	**pTrn = getRowData();

		//  special case for wrappable data
		int		xOff2 = xOff == m_Cols - 1 ? getWrap() ? 0 : xOff : xOff + 1;
		int		yOff2 = yOff == m_Rows - 1 ? yOff : yOff + 1;

		//  pick data points for bilinear sampling.
		p[0] = pTrn[yOff][xOff];
		p[1] = pTrn[yOff2][xOff];
		p[2] = pTrn[yOff][xOff2];
		p[3] = pTrn[yOff2][xOff2];

		for (int i = 0; i < 4; i++)
			if (p[i] == m_NoData)
				return false;
	}

	float	y0 = (p[1] - p[0]) * yCell + p[0];
	float	y1 = (p[3] - p[2]) * yCell + p[2];
	val = (y1 - y0) * xCell + y0;
	fDepth = (val * m_Scale) + m_Offset;

	return true;
}
