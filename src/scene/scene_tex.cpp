 /* =============================================================================
	File: scene_tex.cpp

  =============================================================================== */

#ifdef WIN32
#include <GL/gl.h>
#include <GL/glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif
//#include "glext.h"

#include <stdlib.h>
#include <string.h>
#include <fstream>
#include <vector>

#include <FL/Fl_JPEG_Image.H>
#include <FL/Fl_PNG_Image.H>
#include <FL/Fl_BMP_Image.H>

#include "utility.h"
#include "scene_mgr.h"
#include "image.h"

#ifdef WIN32
extern PFNGLCOMPRESSEDTEXIMAGE2DARBPROC glCompressedTexImage2DARB;
extern PFNGLGETCOMPRESSEDTEXIMAGEARBPROC glGetCompressedTexImageARB;

#else
typedef void (*glCompressedTexImage2DARBPtr) (GLenum, GLint, GLenum, GLsizei, GLsizei, GLint, GLsizei, const GLvoid *);
typedef void (*glGetCompressedTexImageARBPtr) (GLenum, GLint, GLvoid *);
extern glCompressedTexImage2DARBPtr pfnglCompressedTexImage2DARB;
extern glGetCompressedTexImageARBPtr pfnglGetCompressedTexImageARB;
#undef glCompressedTexImage2DARB
#undef glGetCompressedTexImageARB
#define glCompressedTexImage2DARB	(*pfnglCompressedTexImage2DARB)
#define glGetCompressedTexImageARB	(*pfnglGetCompressedTexImageARB)
#endif
#ifndef MAKEFOURCC
#define MAKEFOURCC(ch0, ch1, ch2, ch3) \
		( \
			(int) (unsigned char) (ch0) | ((int) (unsigned char) (ch1) << 8) | ((int) (unsigned char) (ch2) << 16) | \
				((int) (unsigned char) (ch3) << 24) \
		)
#endif //  defined(MAKEFOURCC)

//  FOURCC codes for DX compressed-texture pixel formats
#define FOURCC_DXT1 (MAKEFOURCC('D', 'X', 'T', '1'))
#define FOURCC_DXT2 (MAKEFOURCC('D', 'X', 'T', '2'))
#define FOURCC_DXT3 (MAKEFOURCC('D', 'X', 'T', '3'))
#define FOURCC_DXT4 (MAKEFOURCC('D', 'X', 'T', '4'))
#define FOURCC_DXT5 (MAKEFOURCC('D', 'X', 'T', '5'))

typedef struct _DDCOLORKEY
{
	long int	dwColorSpaceLowValue;	//  low boundary of color space that is to be treated as Color Key, inclusive
	long int	dwColorSpaceHighValue;	//  high boundary of color space that is to be treated as Color Key, inclusive
} DDCOLORKEY;

typedef struct _DDSCAPS2
{
	long int	dwCaps; //  capabilities of surface wanted
	long int	dwCaps2;
	long int	dwCaps3;
	union
	{
		long int	dwCaps4;
		long int	dwVolumeDepth;
	} DUMMYUNIONNAMEN_1;
} DDSCAPS2;

/* -----------------------------------------------------------------------------
    DDPIXELFORMAT
 ------------------------------------------------------------------------------- */
typedef struct _DDPIXELFORMAT
{
	long int	dwSize;			//  size of structure
	long int	dwFlags;		//  pixel format flags
	long int	dwFourCC;		//  (FOURCC code)
	long int	dwRGBBitCount;	//  how many bits per pixel
	long int	dwRBitMask;		//  mask for red bit
	long int	dwGBitMask;		//  mask for green bits
	long int	dwBBitMask;		//  mask for blue bits
	long int	dwRGBAlphaBitMask;	//  mask for alpha channel
} DDPIXELFORMAT;

/* -----------------------------------------------------------------------------
    DDSURFACEDESC2
 ------------------------------------------------------------------------------- */
typedef struct _DDSURFACEDESC2
{
	long int		dwSize;			//  size of the DDSURFACEDESC structure
	long int		dwFlags;		//  determines what fields are valid
	long int		dwHeight;		//  height of surface to be created
	long int		dwWidth;		//  width of input surface
	long int		dwLinearSize;	//  Formless late-allocated optimized surface size

	long int		dwDepth;		//  the depth if this is a volume texture

	long int		dwMipMapCount;	//  number of mip-map levels requestde
	long int		dwAlphaBitDepth;	//  depth of alpha buffer requested
	long int		dwReserved;			//  reserved
	long int		lpSurface;			//  pointer to the associated surface memory
	DDCOLORKEY		dwEmptyFaceColor;	//  Physical color for empty cubemap faces
	DDCOLORKEY		ddckCKDestBlt;		//  color key for destination blt use
	DDCOLORKEY		ddckCKSrcOverlay;	//  color key for source overlay use
	DDCOLORKEY		ddckCKSrcBlt;		//  color key for source blt use
	_DDPIXELFORMAT	ddpfPixelFormat;	//  pixel format description of the surface
	DDSCAPS2		ddsCaps;			//  direct draw surface capabilities
	long int		dwTextureStage;		//  stage in multitexture cascade
} DDSURFACEDESC2;

struct DDS_IMAGE_DATA
{
	GLsizei sizeX;
	GLsizei sizeY;
	GLint	components;
	GLenum	format;
	int		numMipMaps;
	GLubyte *data;
};

/* =============================================================================
 =============================================================================== */

DDS_IMAGE_DATA *loadDDSTextureFile(string filename)
{
	DDSURFACEDESC2	ddsd;
	char			filecode[4] = "   ";
	FILE			*pFile;
	int				factor;
	int				bufferSize;

	static DDS_IMAGE_DATA	*pDDSImageData = NULL;
	static int		prevBufferSize = 0;

	//  Open the file
	pFile = fopen(filename.c_str(), "rb");

	if (pFile == NULL)
	{
		cout << ("loadDDSTextureFile couldn't find, or failed to load " + filename);
		return NULL;
	}

	//  Verify the file is a true .dds file
	if (!fread(filecode, 4, 1, pFile) || strncmp(filecode, "DDS ", 4) != 0)
	{
		cout << ("The file " + filename + " doesn't appear to be a valid .dds file!" );
		fclose(pFile);
		return NULL;
	}

	//  Get the surface descriptor
	if (!fread(&ddsd, sizeof(ddsd), 1, pFile))
	{
		fclose(pFile);
		return NULL;
	}

	swapEndian(true, &ddsd, sizeof(long int), sizeof(ddsd) / sizeof(long int));

		// This .dds loader supports the loading of compressed formats DXT1, DXT3 and DXT5.
	GLenum format;
	switch(ddsd.ddpfPixelFormat.dwFourCC)
	{
	case FOURCC_DXT1:   // DXT1's compression ratio is 8:1
		format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
		factor = 2;
		break;

	case FOURCC_DXT3:	// DXT3's compression ratio is 4:1
		pDDSImageData->format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
		factor = 4;
		break;

	case FOURCC_DXT5:	// DXT5's compression ratio is 4:1
		pDDSImageData->format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
		factor = 4;
		break;

	default:
		cout << ("The file " + filename + " doesn't appear to be compressed using DXT1, DXT3, or DXT5!");
		fclose(pFile);
		return NULL;
	}

	if (ddsd.dwLinearSize == 0)
		ddsd.dwLinearSize = ddsd.dwWidth * ddsd.dwHeight / factor;

	if (ddsd.dwMipMapCount > 1)
		bufferSize = ddsd.dwLinearSize * factor;
	else
		bufferSize = ddsd.dwLinearSize;

	if (bufferSize > prevBufferSize || prevBufferSize > 512*512)
	{
		if (pDDSImageData) delete pDDSImageData;
		pDDSImageData = (DDS_IMAGE_DATA *) malloc(sizeof(DDS_IMAGE_DATA) + bufferSize * sizeof(unsigned char));
		memset(pDDSImageData, 0, sizeof(DDS_IMAGE_DATA));
		pDDSImageData->data = (unsigned char *) pDDSImageData + sizeof(DDS_IMAGE_DATA);
		prevBufferSize = bufferSize;
	}

	pDDSImageData->format = format;
	pDDSImageData->sizeX = ddsd.dwWidth;
	pDDSImageData->sizeY = ddsd.dwHeight;
	pDDSImageData->numMipMaps = ddsd.dwMipMapCount > 0 ? ddsd.dwMipMapCount : 1;

	if (ddsd.ddpfPixelFormat.dwFourCC == FOURCC_DXT1)
		pDDSImageData->components = 3;
	else
		pDDSImageData->components = 4;

	fread(pDDSImageData->data, bufferSize, 1, pFile);
	fclose(pFile);
		
	return pDDSImageData;
}

/* =============================================================================
    CREATE TEXTURE - This creates a texture in OpenGL that we can texture map CREATE TEXTURE
 =============================================================================== */
int createTextureDDS(DDS_IMAGE_DATA *pDDSImageData)
{
	if (!glCompressedTexImage2DARB) 
		return -1;

	//  Generate a texture with the associative texture ID stored in the array
	GLuint	textureID;
	glGenTextures(1, &textureID);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// use anisotropic filtering if available
	float fLargest;
	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fLargest);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, fLargest);

	int nHeight = pDDSImageData->sizeY;
	int nWidth = pDDSImageData->sizeX;
	int nNumMipMaps = pDDSImageData->numMipMaps;
	int nBlockSize = (pDDSImageData->format == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT) ? 8 : 16;

	//  Load the mip-map levels
	int nSize;
	int nOffset = 0;
	for (int i = 0; i < nNumMipMaps; ++i)
	{
		if (nWidth == 0) nWidth = 1;
		if (nHeight == 0) nHeight = 1;

		nSize = ((nWidth + 3) / 4) * ((nHeight + 3) / 4) * nBlockSize;

		glCompressedTexImage2DARB(GL_TEXTURE_2D, i, pDDSImageData->format, 
			nWidth, nHeight, 0, nSize, pDDSImageData->data + nOffset);

		nOffset += nSize;

		//  Half the image size for the next mip-map level...
		nWidth = (nWidth / 2);
		nHeight = (nHeight / 2);
	}

	return textureID;
}

bool saveDDSTextureFile(string filename, int txID)
{
	FILE			*pFile;
	char			filecode[5] = "DDS ";
	DDSURFACEDESC2	ddsd;
	int				width, height, size;

	//  Open the file
	pFile = fopen(filename.c_str(), "wb");

	if (pFile == NULL)
	{
		cout << ("loadDDSTextureFile failed to open " + filename);
		return false;
	}

	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);

	//write the header
	fwrite(filecode, 4, 1, pFile);
	memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
	ddsd.dwSize = 124;
	ddsd.dwFlags = 0x00021007;
	ddsd.dwWidth = width;
	ddsd.dwHeight = height;
	ddsd.ddpfPixelFormat.dwSize =32;
	ddsd.ddpfPixelFormat.dwFlags = 0x4;
	ddsd.ddpfPixelFormat.dwFourCC = FOURCC_DXT1;
	ddsd.ddsCaps.dwCaps = 0x00401008;
	int nNumMipMaps = 1+floor(log((double)max(width, height))/log(2.0));
	ddsd.dwMipMapCount = nNumMipMaps;

	swapEndian(true, &ddsd, sizeof(long int), sizeof(ddsd) / sizeof(long int));
	fwrite(&ddsd, sizeof(ddsd), 1, pFile);

	//write the mipmaps
	GLubyte *buffer = (GLubyte *)malloc(width*height*4);

	for (int i = 0; i < nNumMipMaps; ++i)
	{
		glGetTexLevelParameteriv(GL_TEXTURE_2D, i, GL_TEXTURE_WIDTH, &width);
		glGetTexLevelParameteriv(GL_TEXTURE_2D, i, GL_TEXTURE_HEIGHT, &height);
		glGetTexLevelParameteriv(GL_TEXTURE_2D, i, GL_TEXTURE_COMPRESSED_IMAGE_SIZE, &size);
		glGetCompressedTexImageARB(GL_TEXTURE_2D, i, buffer);
		fwrite(buffer, size, 1, pFile);
	}

	fclose(pFile);
	delete buffer;
	return true;
}

/* =============================================================================
 =============================================================================== */
unsigned char *padOut(const unsigned char *pData, int sizeX, int sizeY, int sizeTx)
{
	int iSize = sizeTx * sizeTx * 4;
	if (iSize == 0)
		return NULL;

	unsigned char	*pNew = NULL;
	if (!mem_alloc(pNew, iSize))
		return NULL;

	unsigned char	*p = pNew;
	for (int i = 0; i < sizeTx; i++)
	{
		if ((i >= sizeTx - sizeY) && (sizeX > sizeTx))
			pData += (sizeX - sizeTx) * 4;
		for (int j = 0; j < sizeTx * 4; j++) 
			*p++ = ((j < sizeX * 4) && (i >= sizeTx - sizeY)) ? *pData++ : 0;
	}

	return pNew;
}

//=============================================================================
Fl_Image * openTextureFile(string strFileName)
{
	Fl_Image *img = NULL;
	try
	{
		if (getExt(strFileName) == ".jpg" || getExt(strFileName) == ".jpeg")
			img = new Fl_JPEG_Image(strFileName.c_str());
		else if (getExt(strFileName) == ".png")
			img = new Fl_PNG_Image(strFileName.c_str());
		else
			img = new Fl_BMP_Image(strFileName.c_str());
	}
	catch(std::bad_alloc x)
	{
		mem_alloc_failure(__FILE__, __LINE__);
		return NULL;
	}

	//  return if no image
	if (img->d() == 0 || img->h() == 0 || img->w() == 0)
	{
		delete img;
		return NULL;
	}

	//  downsize if image is too big
	if (img->h() > 4096 || img->w() > 4096)
	{
		cout << ("Downsizing image to maximum of 4096 a side");
		int h = img->h() >= img->w() ? 4096 : 4096 * img->h() / img->w();
		int w = img->w() >= img->h() ? 4096 : 4096 * img->w() / img->h();
		Fl_Image *img2 = img->copy(w, h);
		delete img;
		img = img2;
	}
	
	return img;
}
//=============================================================================== */

/* =============================================================================
 =============================================================================== */
int C3DSceneMgr::createUITexture(string strFileName, int &width, int &height, int &txWidth)
{
	Fl_Image * img = openTextureFile(strFileName);
	if (img == NULL) {
		return -1;
	}

	width = img->w();
	height = img->h();

	//  need to add an alpha channel to properly overlay on existing textures
	const unsigned char *pData = NULL;
	if (img->d() == 3)
		pData = makeRGBA((const unsigned char *) img->data()[0], width, height);
	else
		pData = (const unsigned char *) img->data()[0];

	GLuint	textureID;
	glGenTextures(1, &textureID);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

	int txHeight = height;
	txWidth = (width <= 64) ? 64 : (width <= 128) ? 128 : width;

	const unsigned char *pDataNew = NULL;
	if (width != txWidth)
	{
		pDataNew = padOut(pData, width, height, txWidth);
		txHeight = txWidth;
	}

	glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE); 
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, txWidth, txHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, pDataNew ? pDataNew : pData);

	if (pDataNew) delete[] pDataNew;
	if (img->d() == 3) delete[] pData;
	delete img;

	return textureID;
}

/* =============================================================================
 =============================================================================== */
int C3DSceneMgr::createTexture(const unsigned char *pData, int sizeX, int sizeY, int iTypeIn, int iTypeOut, int iTxMode)
{
	//  Generate a texture with the associated texture ID stored in the array
	GLuint	textureID;
	glGenTextures(1, &textureID);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

	//  use anisotropic filtering if available
	float	fLargest;
	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fLargest);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, fLargest);
	if (iTxMode == TX_CLAMP_EDGE)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
	else
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	}

	if (m_TextureCompression && glCompressedTexImage2DARB)
		iTypeOut = (iTypeOut == GL_RGBA) ? GL_COMPRESSED_RGBA : GL_COMPRESSED_RGB;
	else
		iTypeOut = (iTypeOut == GL_RGBA) ? GL_RGBA8 : GL_RGB8;

	if (!isPowerOfTwo(sizeX*sizeY))
		gluBuild2DMipmaps(GL_TEXTURE_2D, iTypeOut, sizeX, sizeY, iTypeIn, GL_UNSIGNED_BYTE, pData);
	else
	{
		glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE); 
		glTexImage2D(GL_TEXTURE_2D, 0, iTypeOut, sizeX, sizeY, 0, iTypeIn, GL_UNSIGNED_BYTE, pData);
	}
	return textureID;
}

/* =============================================================================
 =============================================================================== */
int C3DSceneMgr::createTexture(string strFileName, int &width, int &height, int iTxMode)
{
	if (strFileName == "") //  Return from the function if no file name was passed in
		return -1;
	if (!fileexists(strFileName))
		return -1;

	//  Load the image and store the data
	int textureID = -1;
	if (getExt(strFileName) == ".dds")
	{
		DDS_IMAGE_DATA	*pDDSImageData = NULL;
		if ((pDDSImageData = loadDDSTextureFile(strFileName)) != NULL)
		{
			textureID = createTextureDDS(pDDSImageData);
			height = pDDSImageData->sizeY;
			width = pDDSImageData->sizeX;
		}
		else	//  case where worldwind wraps jpegs in dds files
		{
			Fl_RGB_Image *img = new Fl_JPEG_Image(strFileName.c_str());
			if (img->d() == 0)
				return -1;
			width = img->w();
			height = img->h();
			const unsigned char *pData = (const unsigned char *) img->data()[0];
			textureID = createTexture(pData, width, height, GL_RGB, GL_RGB, iTxMode);
			delete img;
		}
	}
	else
	{
		Fl_Image * img = openTextureFile(strFileName);
		if (img == NULL)
			return -1;
		width = img->w();
		height = img->h();
		const unsigned char *pData = (const unsigned char *) img->data()[0];
		textureID = createTexture(pData, width, height, img->d() == 4 ? GL_RGBA : GL_RGB, GL_RGBA, iTxMode);
		delete img;
	}

	return textureID;
}

void C3DSceneMgr::createNewTileTexture(string strFileName)
{
	Fl_Image * img = openTextureFile(strFileName);
	if (img == NULL)
		return;
	int width = img->w();
	int height = img->h();
	const unsigned char *pOld = (const unsigned char *) img->data()[0];

	//image processing on water
	unsigned char	*pData = NULL;
	if (!mem_alloc(pData, width * height * 3))
		return;

	unsigned char	*p = pData;
	for (int i = 0; i < width * height; i++, p+=3, pOld+=3)
	{
		p[0] = pOld[0];		//red
		p[1] = pOld[1];		//green
		if (pOld[2] < 220 && pOld[2] > pOld[1]*5/4 && pOld[2] > pOld[0]*5/4)
			p[2] = pOld[2] + (220-pOld[2])/4;
		else
			p[2] = pOld[2];		//blue
	}

	string strnew = strFileName;
	setExt(strnew, ".png");
	saveImage(strnew, pData, width, height);
	int txID = createTexture(pData, width, height, GL_RGB, GL_RGB, TX_CLAMP_EDGE);

	delete pData;
	delete img;

	setExt(strFileName, ".dds");
	saveDDSTextureFile(strFileName, txID);
	deleteTexture(txID);
}

/* =============================================================================
 =============================================================================== */
int C3DSceneMgr::createVideoTexture(CMovie* pMovie)
{
	/*
	GLuint	textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);
	int w, h;
	pMovie->getMovieSize(w, h);

	unsigned char	*pData = pMovie->GetMovieFrame(0.0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	GLenum			glType = getIntel() ? GL_UNSIGNED_BYTE : GL_UNSIGNED_INT_8_8_8_8_REV;
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_BGRA_EXT, glType, pData);
	pMovie->setMovieTex(textureID);
	return textureID;
	*/
	return -1;
}

/* =============================================================================
 =============================================================================== */
bool C3DSceneMgr::updateVideoTexture(CMovie * pMovie, double movie_tm)
{
	/*
	if (pMovie == NULL)
		return false;

	int w, h;
	pMovie->getMovieSize(w, h);
	unsigned char	*pData = pMovie->GetMovieFrame(movie_tm);
	if (!pData)
		return false;

	glBindTexture(GL_TEXTURE_2D, pMovie->getMovieTex());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	GLenum	glType = getIntel() ? GL_UNSIGNED_BYTE : GL_UNSIGNED_INT_8_8_8_8_REV;
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_BGRA_EXT, glType, pData);
	return true;
	*/
	return false;
}

/* =============================================================================
 =============================================================================== */
void C3DSceneMgr::deleteTexture(int iTex)
{
	if (iTex == -1)
		return;

	GLuint	textureID = iTex;
	if (glIsTexture(iTex)) 
		glDeleteTextures(1, &textureID);
}

/* =============================================================================
    ADD MATERIAL \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\* This function adds a
    material to our model manually since .obj has no such info ADD MATERIAL
    \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*
 =============================================================================== */
int C3DModel::addMaterial(string strName, string strFile, color clr, int iTxMode)
{
	CMaterialPtr pMat = new CMaterial();
	pMat->setTextureId(-1);
	pMat->setColor(clr);
	pMat->setHeight(0);
	pMat->setWidth(0);
	pMat->setName(strName);

	// If we have a file name passed in, copy it to our material structure
	if (strFile.size())
	{
		int w, h;
		pMat->setLink(strFile);

		GLuint	txID = createTexture(pMat->getLink(), w, h, iTxMode);
		pMat->setTextureId(txID);
		pMat->setHeight(h);
		pMat->setWidth(w);
	}

	pMat->setShared(false);

	addMaterial(pMat);
	return pMat->getTextureId();
}

/* =============================================================================
 =============================================================================== */
int C3DModel::addMaterial(string strName, const unsigned char *pData, int sizeX, int sizeY, color clr, int iTxMode)
{
	CMaterialPtr pMat = new CMaterial();
	pMat->setTextureId(-1);
	pMat->setColor(clr);
	pMat->setHeight(0);
	pMat->setWidth(0);
	pMat->setName(strName);

	// If we have a file name passed in, let's copy it to our material structure
	GLuint	txID = createTexture(pData, sizeX, sizeY, GL_RGBA, GL_RGBA, iTxMode);
	pMat->setTextureId(txID);
	pMat->setShared(false);

	addMaterial(pMat);
	return pMat->getTextureId();
}

/* =============================================================================
    routine to add a material from another already created texture
 =============================================================================== */
int C3DModel::addMaterial(string strName, int txID, int sizeX, int sizeY, color clr, bool bMipMap)
{
	CMaterialPtr pMat = new CMaterial();
	pMat->setTextureId(txID);
	pMat->setColor(clr);
	pMat->setHeight(sizeY);
	pMat->setWidth(sizeX);
	pMat->setName(strName);
	pMat->setShared(true);
	pMat->setMipMap(bMipMap);
	addMaterial(pMat);
	return pMat->getTextureId();
}

/* =============================================================================
 =============================================================================== */
void C3DModel::unloadMaterials(bool bClearMaterial)
{
	for (int i = 0; i < getMaterialCnt(); i++)
	{
		if (getMaterial(i).getShared())
			continue;
		deleteTexture(getMaterial(i).getTextureId());
		editMaterial(i).setTextureId(-1);
	}

	for (int i = 0; i < getObjectCnt(); i++)
	{
		editObject(i).unloadGLArrays();
		if (bClearMaterial)
			editObject(i).setMaterial(NULL);
		editObject(i).setDirty(true);
	}
}

/* =============================================================================
 =============================================================================== */
void C3DModel::reloadMaterials()
{
	string strTempPath;
	if (!OpenKMZFile("", strTempPath))
		return;
	for (int i = 0; i < getMaterialCnt(); i++)
	{
		deleteTexture(getMaterial(i).getTextureId());

		int		w, h;
		GLuint	txID = createTexture(getMaterial(i).getLink(), w, h, TX_CLAMP_EDGE);
		editMaterial(i).setTextureId(txID);
		editMaterial(i).setHeight(h);
		editMaterial(i).setWidth(w);
	}
}

