/* =============================================================================
	File: image.cpp

 =============================================================================== */

#pragma warning(disable : 4311)
#pragma warning(disable : 4312)
#include <stdio.h>
#include <string.h>

#ifndef WIN32
#define cimg_display_type	0
#endif
#define cimg_use_png
#define cimg_use_jpeg

#include "CImg.h"

using namespace cimg_library;

#include "image.h"
#include "utility.h"

/* =============================================================================
    [ Functions ]
 =============================================================================== */
unsigned char *loadImage(string filename, int &width, int &height)
{
	CImg < unsigned char > image(filename.c_str());
	width = image.width;
	height = image.height;

	int iSize = image.width * image.height * 3;
	if (iSize == 0) 
		return NULL;

	unsigned char	*data = NULL;
	if (!mem_alloc(data, iSize)) 
		return NULL;
	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
			for (int rgb = 0; rgb < 3; rgb++)
				data[(height - 1 - y) * 3 * width + 3 * x + rgb] = image.data[rgb * width * height + y * width + x];
	}

	return data;
}

/* =============================================================================
 =============================================================================== */
void saveImage(string filename, const unsigned char *imageBuffer, int width, int height, int quality)
{
	int iSize = 3 * width * height;
	if (iSize == 0) 
		return;

	unsigned char	*shuffled = NULL;
	if (!mem_alloc(shuffled, iSize)) 
		return;

	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			for (int rgb = 0; rgb < 3; rgb++)
				shuffled[rgb * width * height + y * width + x] = imageBuffer[(height - 1 - y) * 3 * width + 3 * x + rgb];
		}
	}

	CImg < unsigned char > image(shuffled, width, height, 1, 3, false);
	if (getExt(filename) == ".jpg")
		image.save_jpeg(filename.c_str(), quality);
	else if (getExt(filename) == ".png")
		image.save_png(filename.c_str());
	else
		image.save_bmp(filename.c_str());

	delete[] shuffled;
}
