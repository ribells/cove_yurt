/*
 * GLFONT.CPP -- glFont routines ;
 * Copyright (c) 1998 Brad Fish ;
 * Copyright (c) 2002 Henri Kyrki ;
 * See glFont.txt for terms of use ;
 * 10.5 2002 ;
 */
#include "font.h"
#include "utility.h"

/* =============================================================================
 =============================================================================== */
void GLFontBase::CreateImpl(string fileName, bool bPixelPerfect)
{
	clean();

	m_PixelPerfect = bPixelPerfect;

	//  Open font file
	FILE	*fp = NULL;
	if (!(fp = fopen(fileName.c_str(), "rb"))) 
		throw GLFontError::InvalidFile();

	//  Read glFont structure
	fread(&m_Font, sizeof(GLFONT), 1, fp);
	swapEndian(true, &(m_Font), sizeof(int), 5);

	//Save texture number ;
	//GLuint Tex;
	//glGenTextures(1, &Tex);
	//m_txID = Tex;

	//  Get number of characters
	int Num = m_Font.IntEnd - m_Font.IntStart + 1;

	m_CharList.resize(Num);

	//  Read glFont characters
	fread(&m_CharList[0], sizeof(GLFONTCHAR), Num, fp);
	swapEndian(true, &m_CharList[0], sizeof(float), 6 * Num);

	//  Get texture size
	Num = m_Font.TexWidth * m_Font.TexHeight * 2;

	vector<char> TexBytes(Num);

	//  Read texture data
	fread(&TexBytes[0], sizeof(char), Num, fp);

	//  Set texture attributes
	/*
	glBindTexture(GL_TEXTURE_2D, m_txID);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	if (m_PixelPerfect)
	{
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	}
	else
	{
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	}

	//  Create texture
	glTexImage2D(GL_TEXTURE_2D,0,2,m_Font.TexWidth,m_Font.TexHeight,0,GL_LUMINANCE_ALPHA,GL_UNSIGNED_BYTE,(void *) &TexBytes[0]);

	//  Clean up
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	*/
	fclose(fp);
}

/* =============================================================================
 =============================================================================== */
void GLFontBase::Begin()
{
	if (m_txID == -1)
		throw GLFontError::InvalidFont();
	//glBindTexture(GL_TEXTURE_2D, m_txID);
}

/* =============================================================================
 =============================================================================== */
void GLFont::Create(string fileName, bool bPixelPerfect)
{
	GLFontBase::CreateImpl(fileName, bPixelPerfect);
	if (bPixelPerfect)
	{
		for (int i = 0; i < m_Font.IntEnd - m_Font.IntStart + 1; i++)
		{
			m_CharList[i].dx = (int) ((m_CharList[i].tx2 - m_CharList[i].tx1) * m_Font.TexWidth);
			m_CharList[i].dy = (int) ((m_CharList[i].ty2 - m_CharList[i].ty1) * m_Font.TexHeight);
		}
	}
}

/* =============================================================================
 =============================================================================== */
void GLFont::TextOut(string strText, float x, float y, float z)
{
	if (m_txID == -1)
		throw GLFontError::InvalidFont();

	//  Loop through characters
	//glBegin(GL_QUADS);
	for (int i = 0; i < strText.length(); i++)
	{
			//  Get pointer to glFont character and draw the texture
		GLFONTCHAR	*Char = &m_CharList[(unsigned char) strText[i] - m_Font.IntStart];
		/*
		glTexCoord2f(Char->tx1, Char->ty1);
		glVertex3f(x, y + Char->dy, z);
		glTexCoord2f(Char->tx1, Char->ty2);
		glVertex3f(x, y, z);
		glTexCoord2f(Char->tx2, Char->ty2);
		glVertex3f(x + Char->dx, y, z);
		glTexCoord2f(Char->tx2, Char->ty1);
		glVertex3f(x + Char->dx, y + Char->dy, z);
		x += Char->dx;
		*/
	}
	//glEnd();
}

/* =============================================================================
 =============================================================================== */
void GLFont::getExtent(string strText, float &dx, float &dy)
{
	dy = 0;
	dx = 0;
	for (int i = 0; i < strText.length(); i++)
	{
		GLFONTCHAR	*Char = &m_CharList[(int) strText[i] - m_Font.IntStart];
		dx += Char->dx;
		if (Char->dy > dy) 
			dy = Char->dy;
	}
}

