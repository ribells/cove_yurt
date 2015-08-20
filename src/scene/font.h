#pragma once

//*********************************************************
//
//Copyright (c) 1998 Brad Fish
//Copyright (c) 2002 Henri Kyrki
//
//*********************************************************

#include <vector>
#include <string>
using namespace std;

#ifdef WIN32
#include <GL/gl.h>
#else
#include <OpenGL/gl.h>
#endif

namespace GLFontError
{
	struct InvalidFile{};
	struct InvalidFont{};
}

class GLFontBase
{
public:
	GLFontBase() : m_txID(-1) {};
	void Begin();
	~GLFontBase() { clean(); }

private:
	void clean()
	{
		m_CharList.clear();
		if (m_txID != -1) 
		{
			//GLuint	textureID = m_txID;
			//glDeleteTextures(1, &textureID);
			m_txID = -1;
		}
	}

protected:

	void CreateImpl(string fileName, bool bPixelPerfect = true);

	typedef struct
	{
		float dx;
		float dy;
		float tx1, ty1;
		float tx2, ty2;
	} GLFONTCHAR;

	typedef struct
	{
		int Tex;
		int TexWidth, TexHeight;
		int IntStart, IntEnd;
		void *ptr;
	} GLFONT;

	vector<GLFONTCHAR>	m_CharList;
	int		m_txID;
	GLFONT	m_Font;
	bool	m_PixelPerfect;
};

class GLFont : public GLFontBase
{
public:
	GLFont() {};
	void Create(string fileName, bool bPixelPerfect = true);
	void TextOut (string strText, float x, float y, float z);
	void getExtent(string strText, float &dx, float &dy);
};



