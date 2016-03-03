/* =============================================================================
	File: terrain_io.cpp

 =============================================================================== */

//#include "netcdf.h"

#include <string.h>
#include "terrain.h"
#include "utility.h"
#include "scene/image.h"
#include "web/web_file.h"

#include <FL/Fl_BMP_Image.H>

/* =============================================================================
 =============================================================================== */

bool CTerrainLayer::completeLoad()
{
	CData	&Data = getData();
	if (Data.getVarCnt() != 3)
	{
		cout << ("Bad terrain format - unable to continue loading terrain.");
		return false;
	}

	//  set scale and offset attributes
	int iAttScale = Data.findAtt(2, "scale_factor");
	if (iAttScale != -1)
	{
		m_Scale *= Data.getAttData(2, iAttScale)[0];
		Data.delAtt(2, iAttScale);
	}

	int iAttOffset = Data.findAtt(2, "add_offset");
	if (iAttOffset != -1)
	{
		m_Offset += Data.getAttData(2, iAttOffset)[0];
		Data.delAtt(2, iAttOffset);
	}

	m_NoData = Data.getVar(2).getNoValue();

	int xd = Data.findDim("x");
	m_Cols = Data.getDimSize(xd);

	int yd = Data.findDim("y");
	m_Rows = Data.getDimSize(yd);

	double	lonMin = Data.getVarDDataValue(0, 0);
	double	lonMax = Data.getVarDDataValue(0, m_Cols - 1);
	m_CellSizeX = (lonMax - lonMin) / (double) (m_Cols);

	double	latMin = Data.getVarDDataValue(1, m_Rows - 1);
	double	latMax = Data.getVarDDataValue(1, 0);
	m_CellSizeY = (latMax - latMin) / (double) (m_Rows);

	m_NoData = Data.getVarNoValue(2);

	//fill in the row data
	if (!mem_alloc(m_RowData, m_Rows)) {
			return false;
	}
	for (int i = 0; i < m_Rows; i++) {
		m_RowData[i] = &(Data.m_Var[2].editTrnDataRow(i)[0]);
	}
	m_MinPos = Vec3d(latMax - (double) m_Rows * m_CellSizeY, lonMin, Data.m_Var[2].getMin());
	m_MaxPos = Vec3d(latMax, lonMin + (double) m_Cols * m_CellSizeX, Data.m_Var[2].getMax());
	setWrap((fabs(m_MaxPos[1] - 180.) < m_CellSizeX && fabs(m_MinPos[1] + 180.) < m_CellSizeX));

	// clean up attributes - remove offset/scale and put in ranges if not there
	int index[3] = { 1, 0, 2 };
	for (int i = 0; i < 3; i++) {
		int ndx = index[i];
		int id = Data.findAtt(ndx, "actual_range");
		if (id == -1) {
			id = Data.newAtt(ndx, "actual_range");

			vector<double>	pd;
			pd.resize(2);
			pd[0] = m_MinPos[ndx];
			pd[1] = m_MaxPos[ndx];
			Data.setAttData(ndx, id, pd);
		}
	}

	//  send out the location to debug
	//if (g_Env.m_Verbose) {
		ostringstream	s;
		s << "\n   Terrain Details: " << endl;
		s << "    - lat range: " << m_MinPos[0] << "," << m_MaxPos[0] << endl;
		s << "    - long range: " << m_MinPos[1] << "," << m_MaxPos[1] << endl;
		s << "    - depth range: " << m_MinPos[2] << "," << m_MaxPos[2];
		cout << (s.str());
	//}

	return true;
}

/* =============================================================================
 =============================================================================== */
bool CTerrainLayer::readTerrainFile(string fileName)
{
	m_DataPtr = new CData();
	getData().setLoadAsTerrain(true);
	if (!getData().readTerrain(fileName))
	{
		delete m_DataPtr;
		m_DataPtr = 0;
		return false;
	}

	//  store the data and metadata
	return completeLoad();
}

/* =============================================================================
 =============================================================================== */
bool CTerrainLayer::writeBILFile(string fileName)
{
	setExt(fileName, ".bil");

	if (!writeBilFile(fileName, getRowData(), m_Rows, m_Cols, false))
		return false;

	short	iNoData = (short) max(-32768.0f, min(32767.0f, m_NoData));
	if	(!writeBilFileHeader(fileName,m_Rows,m_Cols,iNoData,m_MinPos[1],
		m_MinPos[0] + (double) m_Rows * m_CellSizeY,m_CellSizeX,m_CellSizeY	))
		return false;

	return true;
}

/* =============================================================================
 =============================================================================== */
bool writeBilFile(string fileName, float **pData, int iRows, int iCols, bool bFlip)
{
	if (iCols == 0 || iRows == 0)
		return false;

	FILE	*fp;
	if ((fp = fopen(fileName.c_str(), "wb")) == NULL)
		return false;

	short	*pBuf = NULL;
	if (!mem_alloc(pBuf, iCols))
		return false;

	float	*pf;
	for (int i = 0; i < iRows; i++)
	{
		pf = pData[(bFlip ? (iRows - i - 1) : i)];
		for (int j = 0; j < iCols; j++, pf++) 
			pBuf[j] = (short) max(-32768.0f, min(32767.0f, *pf));

		//  flip the data to intel format if not on intel machine
		swapEndian(true, pBuf, sizeof(short), iCols);
		fwrite(pBuf, sizeof(short) * iCols, 1, fp);
	}

	delete[] pBuf;
	fclose(fp);
	return true;
}

/* =============================================================================
    write the .hdr file and .prj files for the bil file
 =============================================================================== */
bool writeBilFileHeader(string	fileName, int iRows, int iCols, short iNoData, 
						float ulxmap, float ulymap, float fCellSizeX, float fCellSizeY)
{
	setExt(fileName, ".hdr");

	FILE	*fp;
	if ((fp = fopen(fileName.c_str(), "wb")) == NULL)
		return false;
	fprintf(fp, "byteorder I\n");
	fprintf(fp, "layout BIL\n");
	fprintf(fp, "ncols %i\n", iCols);
	fprintf(fp, "nrows %i\n", iRows);
	fprintf(fp, "nbands 1\n");
	fprintf(fp, "nbits 16\n");
	fprintf(fp, "nodata %i\n", iNoData);
	fprintf(fp, "ulxmap %lf\n", ulxmap);
	fprintf(fp, "ulymap %lf\n", ulymap);
	fprintf(fp, "xdim %f\n", fCellSizeX);
	fprintf(fp, "ydim %f\n", fCellSizeY);
	fclose(fp);

	setExt(fileName, ".prj");
	if ((fp = fopen(fileName.c_str(), "wb")) == NULL)
		return false;
	fprintf(fp,
		"Projection GEODETIC\n""Datum WGS84\n""Zunits METERS\n""Units DD\n""Spheroid WGS84\n""Xshift 0.0000000000\n"
		"Yshift 0.0000000000\n");
	fclose(fp);

	return true;
}

/* =============================================================================
    save a bitmap at the resolution of the current active terrain set
 =============================================================================== */
bool CTerrain::saveTerrainBitmap(string pName, Vec3d minpos, Vec3d maxpos, int maxSize)
{
	int iGrad = getTextureSet().getCurGradIndex();
	if (!pName.size() || iGrad == -1)
		return false;

	CTerrainTile	Tile;
	Tile.setEnvironment(m_TerrainSet, m_TextureSet);

	Tile.setBounds(minpos[1], maxpos[1], minpos[0], maxpos[0]);

	if (maxSize == 0) maxSize = 4096;

	float	w = maxpos[1] - minpos[1];
	float	h = maxpos[0] - minpos[0];
	if (h == 0.0f || w == 0.0f)
		return false;
	if (w > h)
	{
		h = (maxSize * h) / w;
		w = maxSize;
	}
	else
	{
		w = (maxSize * w) / h;
		h = maxSize;
	}

	unsigned char	*pTerrainBmp;
	if (Tile.createTerrainTexture(pTerrainBmp, w, h, true, false))
	{
		cout << ("  saving bitmap " + pName);

		unsigned char	*pRGB = makeRGB(pTerrainBmp, w, h);
		delete[] pTerrainBmp;
		saveImage(pName, pRGB, w, h);
		delete[] pRGB;
	}

	return true;
}

Fl_Image * openTextureFile(string fileName);

void CTerrain::rebuildTextureTilesR(CTextureLayer &Texture, int iLevel, int iRow, int iCol)
{
	string	strLocal, strURL;
	Texture.getTiledTextureURL(strURL, iLevel, iRow, iCol);
	Texture.getTiledTexturePath(strLocal, iLevel, iRow, iCol);
	if (strURL == "" || strLocal == "" || !getWebFile(strURL, strLocal)) {
		cout << ("error creating tile");
		return;
	}
	createNewTileTexture(strLocal);
//	delfile(strLocal);

	if (iLevel == Texture.getMaxLevel()+1)
		return;

	for (int y = 0; y < 2; y++)
		for (int x = 0; x < 2; x++)
			rebuildTextureTilesR(Texture, iLevel + 1, 
								iRow * (iLevel ? 2 : 1) + y, 
								iCol * (iLevel ? 2 : 1) + x);
}

/* =============================================================================
 =============================================================================== */
void CTerrain::rebuildTextureTiles()
{
	CTextureLayer Texture;
	Texture.setLink("http://worldwind25.arc.nasa.gov/tile/tile.aspx");
	Texture.setID("bmng.topo.bathy.200406");
	Texture.setName("temp");
	Texture.setImageType("image/jpeg");
	Texture.setMinLevel(0);
	Texture.setMaxLevel(4);

	m_TextureCompression = true;

	for (int y = 0; y < 5; y++)
		for (int x = 0; x < 10; x++)
		{
			rebuildTextureTilesR(Texture, 1, y, x);
			cout << ".";
		}
	cout << endl;

	//make overview tile
	Texture.setLink("datasvr/GIS");
	string strOverview;
	Texture.getTiledTexturePath(strOverview, 0, 0, 0);
	if (!fileexists(strOverview))
	{
		unsigned char	*pDest = NULL;
		int w = 512, h = 512;
		if (!mem_alloc(pDest, w*5 * h*10 * 3))
			return;
		for (int y = 0; y < 5; y++) {
			for (int x = 0; x < 10; x++)
			{
				string fileName;
				Texture.getTiledTexturePath(fileName, 1, y, x);

				Fl_Image * img = openTextureFile(fileName);
				const unsigned char *pSrc = (const unsigned char *) img->data()[0];

				for (int i = 0; i < h; i++)
					memcpy(pDest + (h*(y+1)-i-1)*w*10*3 + (w*3*x), pSrc + (w*3*i), w*3);

				delete img;
			}
		}
		Fl_RGB_Image base_img(pDest, 512*10, 512*5);
		Fl_Image * img = base_img.copy(2048, 1024);
		saveImage(strOverview, (const unsigned char *)img->data()[0], img->w(), img->h());
	}
	createNewTileTexture(strOverview);
}
