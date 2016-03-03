 /* =============================================================================
	File: scene_mgr.cpp

  =============================================================================== */

#pragma warning(push)
#pragma warning(disable : 4312)
#pragma warning(disable : 4311)

#ifdef WIN32
#define _GDI32_
#include <GL/gl.h>
#include <GL/glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif

//#include "glext.h"
#pragma warning(pop)
#include <vector>

#include "utility.h"
#include "scene_mgr.h"

#include <vl/VLgl.h>

//  Functions 
#ifdef WIN32
PFNGLBINDBUFFERPROC					glBindBuffer = NULL;
PFNGLBUFFERDATAPROC					glBufferData = NULL;
PFNGLBUFFERSUBDATAPROC				glBufferSubData = NULL;
PFNGLDELETEBUFFERSPROC				glDeleteBuffers = NULL;
PFNGLGENBUFFERSPROC					glGenBuffers = NULL;
PFNGLMAPBUFFERPROC					glMapBuffer = NULL;
PFNGLUNMAPBUFFERPROC				glUnmapBuffer = NULL;

//  ARB_texture_compression
PFNGLCOMPRESSEDTEXIMAGE2DARBPROC	glCompressedTexImage2DARB = NULL;
PFNGLGETCOMPRESSEDTEXIMAGEARBPROC	glGetCompressedTexImageARB = NULL;

#else
typedef void (*glBindBufferPtr) (GLenum, GLuint);
typedef void (*glBufferDataPtr) (GLenum, GLsizeiptr, const GLvoid *, GLenum);
typedef void (*glBufferSubDataPtr) (GLenum, GLintptr, GLsizeiptr, const GLvoid *);
typedef void (*glDeleteBuffersPtr) (GLsizei, const GLuint *);
typedef void (*glGenBuffersPtr) (GLsizei, GLuint *);
typedef GLvoid * (*glMapBufferPtr) (GLenum, GLenum);
typedef GLboolean (*glUnmapBufferPtr) (GLenum);
typedef void (*glCompressedTexImage2DARBPtr) (GLenum, GLint, GLenum, GLsizei, GLsizei, GLint, GLsizei, const GLvoid *);
typedef void (*glGetCompressedTexImageARBPtr) (GLenum, GLint, GLvoid *);

glBindBufferPtr					pfnglBindBuffer = NULL;
glBufferDataPtr					pfnglBufferData = NULL;
glBufferSubDataPtr				pfnglBufferSubData = NULL;
glDeleteBuffersPtr				pfnglDeleteBuffers = NULL;
glGenBuffersPtr					pfnglGenBuffers = NULL;
glMapBufferPtr					pfnglMapBuffer = NULL;
glUnmapBufferPtr				pfnglUnmapBuffer = NULL;
glCompressedTexImage2DARBPtr	pfnglCompressedTexImage2DARB = NULL;
glGetCompressedTexImageARBPtr	pfnglGetCompressedTexImageARB = NULL;

#undef glBindBuffer
#undef glBufferData
#undef glBufferSubData
#undef glDeleteBuffers
#undef glGenBuffers
#undef glMapBuffer
#undef glUnmapBuffer
#undef glCompressedTexImage2DARB
#undef glGetCompressedTexImageARB
#define glBindBuffer				(*pfnglBindBuffer)
#define glBufferData				(*pfnglBufferData)
#define glBufferSubData				(*pfnglBufferSubData)
#define glDeleteBuffers				(*pfnglDeleteBuffers)
#define glGenBuffers				(*pfnglGenBuffers)
#define glMapBuffer					(*pfnglMapBuffer)
#define glUnmapBuffer				(*pfnglUnmapBuffer)
#define glCompressedTexImage2DARB	(*pfnglCompressedTexImage2DARB)
#define glGetCompressedTexImageARB	(*pfnglGetCompressedTexImageARB)
#endif
#ifndef WIN32
#include <dlfcn.h>

/* =============================================================================
 =============================================================================== */

void *glGetProcAddress(const char *name)
{
	void	*pLib = dlopen(NULL, RTLD_LAZY);
	if (!pLib)
	{
		cout << ("Error loading OpenGL lib for mac: " + (string) dlerror());
		return NULL;
	}

	void	*symbol = dlsym(pLib, name);
	dlclose(pLib);
	return symbol;
}
#endif

/* =============================================================================
 =============================================================================== */
string C3DSceneMgr::getOpenGLVersion()
{
	const GLubyte	*version = glGetString(GL_VERSION);
	return version ? (char *) version : "";
}

/* =============================================================================
 =============================================================================== */
bool C3DSceneMgr::initSceneManager()
{
#ifndef WIN32
	pfnglBindBuffer = (glBindBufferPtr) glGetProcAddress("glBindBuffer");
	pfnglBufferData = (glBufferDataPtr) glGetProcAddress("glBufferData");
	pfnglBufferSubData = (glBufferSubDataPtr) glGetProcAddress("glBufferSubData");
	pfnglDeleteBuffers = (glDeleteBuffersPtr) glGetProcAddress("glDeleteBuffers");
	pfnglGenBuffers = (glGenBuffersPtr) glGetProcAddress("glGenBuffers");
	pfnglMapBuffer = (glMapBufferPtr) glGetProcAddress("glMapBuffer");
	pfnglUnmapBuffer = (glUnmapBufferPtr) glGetProcAddress("glUnmapBuffer");
	pfnglCompressedTexImage2DARB = (glCompressedTexImage2DARBPtr) glGetProcAddress("glCompressedTexImage2DARB");
	pfnglGetCompressedTexImageARB = (glGetCompressedTexImageARBPtr) glGetProcAddress("glGetCompressedTexImageARB");
#else
	glBindBuffer = (PFNGLBINDBUFFERPROC) wglGetProcAddress("glBindBuffer");
	glBufferData = (PFNGLBUFFERDATAPROC) wglGetProcAddress("glBufferData");
	glBufferSubData = (PFNGLBUFFERSUBDATAPROC) wglGetProcAddress("glBufferSubData");
	glDeleteBuffers = (PFNGLDELETEBUFFERSPROC) wglGetProcAddress("glDeleteBuffers");
	glGenBuffers = (PFNGLGENBUFFERSPROC) wglGetProcAddress("glGenBuffers");
	glMapBuffer = (PFNGLMAPBUFFERPROC) wglGetProcAddress("glMapBuffer");
	glUnmapBuffer = (PFNGLUNMAPBUFFERPROC) wglGetProcAddress("glUnmapBuffer");
	glCompressedTexImage2DARB = (PFNGLCOMPRESSEDTEXIMAGE2DARBPROC) wglGetProcAddress("glCompressedTexImage2DARB");
	glGetCompressedTexImageARB = (PFNGLGETCOMPRESSEDTEXIMAGEARBPROC) wglGetProcAddress("glGetCompressedTexImageARB");
#endif

	// test case for VBO not available ;
	// glBindBuffer = NULL;
	// glMapBuffer = NULL;
	// glGenBuffers = NULL;

	if (glBindBuffer &&	glBufferData && glBufferSubData && glDeleteBuffers && glGenBuffers
		&&	glMapBuffer	&&	glUnmapBuffer && glCompressedTexImage2DARB && glGetCompressedTexImageARB)
		//cout << ("From scene_mgr.cpp: Using Vertex Buffer Objects and compressed textures.");
	if (!glBindBuffer) 
		cout << ("Vertex Buffer Objects are not available.");
	if (!glCompressedTexImage2DARB)
	{
		cout << ("Texture compression is not available.  Deep Blue tiles will be changed to a terrain gradient and some texture sets may not appear correctly.");
	}

	return true;
}

/* =============================================================================
 =============================================================================== */
bool C3DSceneMgr::getDDSTileSupport()
{
	return glCompressedTexImage2DARB != NULL;
}

void C3DSceneMgr::setDDSTileSupportOff()
{
#ifndef WIN32
	pfnglCompressedTexImage2DARB = NULL;
#else
	glCompressedTexImage2DARB = NULL;
#endif
}

static const string strPrimType[] =
{
	"model",
	"icon",
	"sphere",
	"cube",
	"cylinder",
	"cone",
	"inverted cone",
	"pyramid",
	"diamond"
};
static const string strDataPrimType[] =
{
	"",
	"point",
	"sphere",
	"cube",
	"cylinder",
	"cone",
	"inverted cone",
	"pyramid",
	"diamond"
};
static const int	iPrimMax = 9;

/* =============================================================================
 =============================================================================== */

static void myCube()
{
	static GLfloat	n[6][3] =
	{
		{ -1.0, 0.0, 0.0 },
		{ 0.0, 1.0, 0.0 },
		{ 1.0, 0.0, 0.0 },
		{ 0.0, -1.0, 0.0 },
		{ 0.0, 0.0, 1.0 },
		{ 0.0, 0.0, -1.0 }
	};
	static GLint	faces[6][4] =
	{
		{ 0, 4, 5, 1 },
		{ 1, 5, 7, 3 },
		{ 3, 7, 6, 2 },
		{ 2, 6, 4, 0 },
		{ 6, 7, 5, 4 },
		{ 3, 2, 0, 1 }
	};
	static GLfloat	v[8][3] =
	{
		{ -0.5, -0.5, 0 },
		{ -0.5, 0.5, 0 },
		{ 0.5, -0.5, 0 },
		{ 0.5, 0.5, 0 },
		{ -0.5, -0.5, 1 },
		{ -0.5, 0.5, 1 },
		{ 0.5, -0.5, 1 },
		{ 0.5, 0.5, 1 },
	};
	for (int i = 5; i >= 0; i--)
	{
		glBegin(GL_QUADS);
		glNormal3fv(&n[i][0]);
		glVertex3fv(&v[faces[i][0]][0]);
		glVertex3fv(&v[faces[i][1]][0]);
		glVertex3fv(&v[faces[i][2]][0]);
		glVertex3fv(&v[faces[i][3]][0]);
		glEnd();
	}
}

/* =============================================================================
 =============================================================================== */
static void myPyramid()
{
	static GLfloat	n[5][3] =
	{
		{ -.894, 0.0, .447 },
		{ 0.0, .894, .447 },
		{ .894, 0.0, .447 },
		{ 0.0, -.894, .447 },
		{ 0.0, 0.0, -1.0 }
	};
	static GLint	faces[5][4] = { { 0, 4, 5, 1 }, { 1, 5, 7, 3 }, { 3, 7, 6, 2 }, { 2, 6, 4, 0 }, { 3, 2, 0, 1 } };
	static GLfloat	v[8][3] =
	{
		{ -0.5, -0.5, 0 },
		{ -0.5, 0.5, 0 },
		{ 0.5, -0.5, 0 },
		{ 0.5, 0.5, 0 },
		{ -0.0, -0.0, 1 },
		{ -0.0, 0.0, 1 },
		{ 0.0, -0.0, 1 },
		{ 0.0, 0.0, 1 },
	};

	for (int i = 4; i >= 0; i--)
	{
		glBegin(GL_QUADS);
		glNormal3fv(&n[i][0]);
		glVertex3fv(&v[faces[i][0]][0]);
		glVertex3fv(&v[faces[i][1]][0]);
		glVertex3fv(&v[faces[i][2]][0]);
		glVertex3fv(&v[faces[i][3]][0]);
		glEnd();
	}
}

/* =============================================================================
 =============================================================================== */
static void myDiamond()
{
	static GLfloat	n[8][3] =
	{
		{ -.894, 0.0, .447 },
		{ 0.0, .894, .447 },
		{ .894, 0.0, .447 },
		{ 0.0, -.894, .447 },
		{ -.894, 0.0, -.447 },
		{ 0.0, .894, -.447 },
		{ .894, 0.0, -.447 },
		{ 0.0, -.894, -.447 },
	};
	static GLint	faces[8][4] =
	{
		{ 0, 4, 1 },
		{ 1, 4, 3 },
		{ 3, 4, 2 },
		{ 2, 4, 0 },
		{ 0, 1, 5 },
		{ 1, 3, 5 },
		{ 3, 2, 5 },
		{ 2, 0, 5 }
	};
	static GLfloat	v[6][3] =
	{
		{ -0.5, -0.5, 0 },
		{ -0.5, 0.5, 0 },
		{ 0.5, -0.5, 0 },
		{ 0.5, 0.5, 0 },
		{ 0.0, 0.0, 1.0 },
		{ 0.0, 0.0, -1.0 }
	};

	for (int i = 7; i >= 0; i--)
	{
		glBegin(GL_TRIANGLES);
		glNormal3fv(&n[i][0]);
		glVertex3fv(&v[faces[i][0]][0]);
		glVertex3fv(&v[faces[i][1]][0]);
		glVertex3fv(&v[faces[i][2]][0]);
		glEnd();
	}
}

/* =============================================================================
 =============================================================================== */
int C3DSceneMgr::getPrimType(string strType)
{
	for (int i = 0; i < iPrimMax; i++)
		if (strPrimType[i] == strType) 
			return i;
	return -1;
}

/* =============================================================================
 =============================================================================== */
string C3DSceneMgr::getPrimType(int i)
{
	if (i < 0 || i >= iPrimMax) 
		return "";
	return strPrimType[i];
}

/* =============================================================================
 =============================================================================== */
int C3DSceneMgr::getDataPrimType(string strType)
{
	for (int i = 1; i < iPrimMax; i++)
		if (strDataPrimType[i] == strType) 
			return i;
	return -1;
}

/* =============================================================================
 =============================================================================== */
string C3DSceneMgr::getDataPrimType(int i)
{
	if (i < 0 || i >= iPrimMax) 
		return "";
	return strDataPrimType[i];
}

/* =============================================================================
 =============================================================================== */

GLUquadricObj	*prim_quadric = NULL;
GLUquadricObj	*data_quadric = NULL;

void C3DSceneMgr::clearQuadrics()
{
	if (prim_quadric)
	{
		gluDeleteQuadric(prim_quadric);
		prim_quadric = NULL;
	}

	if (data_quadric)
	{
		gluDeleteQuadric(data_quadric);
		data_quadric = NULL;
	}
}

/* =============================================================================
 =============================================================================== */
void C3DSceneMgr::drawPrimType(int i)
{
	if (i > 1)
	{
		glPushMatrix();
		glRotatef(-90, 1, 0, 0);
	}

	if (prim_quadric == NULL)
	{
		prim_quadric = gluNewQuadric();
		gluQuadricOrientation(prim_quadric, GLU_OUTSIDE);
		gluQuadricDrawStyle(prim_quadric, GLU_FILL);
		gluQuadricTexture(prim_quadric, GL_TRUE);
		gluQuadricNormals(prim_quadric, GLU_SMOOTH);
	}

	switch(i)
	{
	case 0: 
		cout << ("ERROR: missing model");
		break;

	case 1:
		glBegin(GL_POINTS);
		glVertex3d(0, 0, 0);
		glEnd();
		break;

	case 2:
		gluSphere(prim_quadric, .5, 10, 10);
		break;

	case 3:
		myCube();
		break;

	case 4:
		gluCylinder(prim_quadric, .5, .5, 1, 10, 1);
		gluQuadricOrientation(prim_quadric, GLU_INSIDE);
		gluDisk(prim_quadric, 0, .5, 10, 1);
		glTranslatef(0, 0, 1);
		gluQuadricOrientation(prim_quadric, GLU_OUTSIDE);
		gluDisk(prim_quadric, 0, .5, 10, 1);
		break;

	case 5:
		gluCylinder(prim_quadric, .5, 0.0, 1, 10, 5);
		gluDisk(prim_quadric, 0, .5, 10, 1);
		break;

	case 6:
		gluCylinder(prim_quadric, 0.0, .5, 1, 10, 5);
		glTranslatef(0, 0, 1);
		gluDisk(prim_quadric, 0, .5, 10, 1);
		break;

	case 7:
		myPyramid();
		break;

	case 8:
		myDiamond();
		break;
	}

	if (i > 1) 
		glPopMatrix();
}

/* =============================================================================
 =============================================================================== */
void orientTx(Vec3f pnt0, Vec3f pnt1)
{
	glTranslatef(pnt0[0], pnt0[1], pnt0[2]);

	Vec3f	dir = pnt1 - pnt0;
	dir = norm(dir);

	double	xylen = len(Vec3d(dir[0], dir[1], 0));
	Mat4d	rotxz = vl_1;
	Mat4d	rot2z = vl_1;
	if (xylen > 1e-6)
	{
		rotxz[0][0] = dir[0] / xylen;
		rotxz[0][1] = dir[1] / xylen;
		rotxz[1][0] = -dir[1] / xylen;
		rotxz[1][1] = dir[0] / xylen;
		glMultMatrixd(rotxz.Ref());
	}

	rot2z[0][0] = dir[2];
	rot2z[0][2] = -xylen;
	rot2z[2][0] = xylen;
	rot2z[2][2] = dir[2];
	glMultMatrixd(rot2z.Ref());
}

/* =============================================================================
 =============================================================================== */
void C3DSceneMgr::drawArrow(Vec3f pos, Vec3f vec, float fRad)
{
	if (data_quadric == NULL)
	{
		data_quadric = gluNewQuadric();
		gluQuadricOrientation(data_quadric, GLU_OUTSIDE);
		gluQuadricDrawStyle(data_quadric, GLU_FILL);
		gluQuadricTexture(data_quadric, GL_TRUE);
		gluQuadricNormals(data_quadric, GLU_SMOOTH);
	}

	glPushMatrix();
	orientTx(pos, vec);

	double	len1 = len(vec - pos);
	glScalef(fRad, fRad, len1);
	gluCylinder(data_quadric, 1, 1, 1, 8, 1);
	gluQuadricOrientation(data_quadric, GLU_INSIDE);
	gluDisk(data_quadric, 0, 1, 8, 1);
	glTranslatef(0, 0, 1);
	glScalef(2, 2, 4 * fRad / len1);
	gluQuadricOrientation(data_quadric, GLU_OUTSIDE);
	gluCylinder(data_quadric, 1, 0, 1, 8, 1);
	gluQuadricOrientation(data_quadric, GLU_INSIDE);
	gluDisk(data_quadric, 0, 1, 8, 1);
	glPopMatrix();
}

/* =============================================================================
 =============================================================================== */
void C3DSceneMgr::drawIcon(double scale, int iTex)
{
	float	modelview[16];
	int		i, j;

	glPushMatrix();
	glGetFloatv(GL_MODELVIEW_MATRIX, modelview);

	// undo all rotations - beware all scaling is lost, translation is preserved
	for (i = 0; i < 3; i++)
		for (j = 0; j < 3; j++)
		{
			if (i == j)
				modelview[i * 4 + j] = 1.0;
			else
				modelview[i * 4 + j] = 0.0;
		}

	glLoadMatrixf(modelview);

	scale *= 5;
	glScalef(scale, scale, scale);

	glDisable(GL_LIGHTING);
	if (iTex >= 0)
	{
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, iTex);
	}

	glBegin(GL_QUADS);
	glTexCoord2f(0, 1);
	glVertex3f(-.5, 0, 0);
	glTexCoord2f(1, 1);
	glVertex3f(.5, 0, 0);
	glTexCoord2f(1, 0);
	glVertex3f(.5, 1, 0);
	glTexCoord2f(0, 0);
	glVertex3f(-.5, 1, 0);
	glEnd();
	glDisable(GL_TEXTURE_2D);

	glPopMatrix();
}

/* =============================================================================
 =============================================================================== */
C3DObject::~C3DObject()
{
	if (m_pGLArray) delete[] (GLfloat *) m_pGLArray;
	if (m_pGLIndices) delete[] m_pGLIndices;
	if (m_BufferIDArray != -1) glDeleteBuffers(1, (GLuint *) &(m_BufferIDArray));
	if (m_BufferIDIndices != -1) glDeleteBuffers(1, (GLuint *) &(m_BufferIDIndices));
}

/* =============================================================================
 =============================================================================== */
void C3DObject::alignObjArrays()
{
	if (getNumNormals() == 0 || m_Line) 
		return;	//  no vertex normals to fix up

	if ((getNumVerts() == getNumTexVerts()) && (getNumVerts() == getNumNormals())) 
		return;

	int				iNewCnt = getNumFaces() * 3;
	vector<Vec3f>	vVertsNew;
	vector<Vec3f>	vNormalsNew;
	vector<Vec3f>	vTexVertsNew;
	try
	{
		vVertsNew.resize(iNewCnt);
		vNormalsNew.resize(iNewCnt);
		vTexVertsNew.resize(iNewCnt);
	}
	catch(std::bad_alloc const &)
	{
		return;
	}

	//  Go through all of the faces in the object
	int idxVert = 0;
	for (int f = 0; f < getNumFaces(); f++)
	{
		CFace	&face = editFace(f);
		for (int v = 0; v < 3; v++, idxVert++)
		{
			vVertsNew[idxVert] = m_vVerts[face.getVertIndex(v)];
			face.setVertIndex(v, idxVert);

			if (getNumNormals())
			{
				vNormalsNew[idxVert] = m_vNormals[face.getNormalIndex(v)];
				face.setNormalIndex(v, idxVert);
			}

			if (getNumTexVerts())
			{
				vTexVertsNew[idxVert] = m_vTexVerts[face.getCoordIndex(v)];
				face.setCoordIndex(v, idxVert);
			}
		}
	}

	m_vVerts = vVertsNew;
	if (getNumNormals()) 
		m_vNormals = vNormalsNew;
	if (getNumTexVerts()) 
		m_vTexVerts = vTexVertsNew;
}

/* =============================================================================
 =============================================================================== */

void C3DObject::unloadGLArrays()
{
	if (m_BufferIDArray != -1)
	{
		glDeleteBuffers(1, (GLuint *) &(m_BufferIDArray));
		m_BufferIDArray = -1;
	}

	if (m_BufferIDIndices != -1)
	{
		glDeleteBuffers(1, (GLuint *) &(m_BufferIDIndices));
		m_BufferIDIndices = -1;
	}

	if (m_pGLArray)
	{
		delete[] (GLfloat *) m_pGLArray;
		m_pGLArray = NULL;
	}

	if (m_pGLIndices)
	{
		delete[] m_pGLIndices;
		m_pGLIndices = NULL;
	}
}

#define MAX_COORD	(12)

/* =============================================================================
 =============================================================================== */

void	projectVert(Vec3f &v, Vec3f &n);

void C3DObject::createGLArray(bool bTexture, const t_scale scale, float shading)
{
	if ((getNumTexVerts() && (getNumVerts() != getNumTexVerts()))
		&&	(getNumClrVerts() && (getNumVerts() != getNumClrVerts())))
		return;

	if (getNumFaces() == 0 || getNumVerts() == 0 || m_Line) 
		return;

	unloadGLArrays();

	GLfloat			*pArray = NULL;
	unsigned int	*pIndices = NULL;
	int				indiceCnt = m_TriangleStrip ? getNumFaces() + 2 : getNumFaces() * 3;
	if (glBindBuffer)
	{
		glGetError();
		if (m_BufferIDArray == -1) 
			glGenBuffers(1, (GLuint *) &(m_BufferIDArray));
		glBindBuffer(GL_ARRAY_BUFFER, m_BufferIDArray);
		glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * MAX_COORD * getNumVerts(), NULL, GL_STATIC_DRAW);

		if (m_BufferIDIndices == -1) 
			glGenBuffers(1, (GLuint *) &(m_BufferIDIndices));
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_BufferIDIndices);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(int) * (indiceCnt), NULL, GL_STATIC_DRAW);

		if (glGetError() != GL_NO_ERROR)
			unloadGLArrays();
		else
		{
			pArray = (GLfloat *) glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
			pIndices = (unsigned int *) glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);
		}
	}

	if (m_BufferIDArray == -1)
	{
		pArray = new GLfloat[MAX_COORD * getNumVerts()];
		m_pGLArray = pArray;
		mem_alloc(pIndices, indiceCnt);
		m_pGLIndices = (int *) pIndices;
	}

	assert(pArray != NULL);
	assert(pIndices != NULL);

	//  fill in the vertex info in the buffer
	bool bReproject = m_Reprojected && (scale.land >= 0 || m_Offset != 0);
	bool bNormals = getNumNormals() > 0;
	for (int i = 0; i < getNumVerts(); i++)
	{
		Vec3f	v = m_vVerts[i];
		Vec3f	n = bNormals ? m_vNormals[i] : Vec3f(0, 1, 0);

		if (m_Layering > 0)
		{
			if (m_Layering == 1)
				v[1] = 0;
			else if (m_Layering == 2 && v[1] < 0)
				v[1] = 0;
			else if (m_Layering == 3 && v[1] > 0)
				v[1] = 0;

			n = Vec3f(0, 1, 0);
		}

		if (bReproject)
		{
			v[1] = (v[1] + m_Offset) * (v[1] > 0 ? scale.land : scale.sea); //scale the offset
			n = norm(n * Vec3f(1, 1 / shading, 1));
			projectVert(v, n);
		}

		//  texture coordinates
		if (bTexture)
		{
			if (getNumTexVerts() != 0)
			{
				for (int j = 0; j < 2; j++) 
					*pArray++ = m_vTexVerts[i][j];
			}

			//  vertex color
			if (getNumClrVerts() != 0)
			{
				unsigned int	c = m_vClrVerts[i];
				Vec3f			clr = clrVec(c);
				float			alpha = clrTrans(c) * getTransparency();
				for (int j = 0; j < 3; j++) 
					*pArray++ = clr[j];
				*pArray++ = alpha;
			}
			else if (m_CheckNoData)
			{
				float	fClr = m_vTexVerts[i][2] == m_NoDataValue ? 0.0 : 1.0;
				for (int j = 0; j < 3; j++)
					*pArray++ = fClr;

				float	alpha = m_vTexVerts[i][2] == m_NoDataValue? 0.0 : getTransparency();
				*pArray++ = alpha;
			}
		}

		//  normal
		if (bNormals)
			for (int j = 0; j < 3; j++) 
				*pArray++ = n[j];

		//  vertex
		for (int j = 0; j < 3; j++) 
			*pArray++ = v[j];
	}

	if (m_TriangleStrip)
	{
		*pIndices++ = getFace(0).getVertIndex(0);
		*pIndices++ = getFace(0).getVertIndex(1);
		for (int i = 0; i < getNumFaces(); i++) 
			*pIndices++ = getFace(i).getVertIndex(2);
	}
	else
	{
		for (int i = 0; i < getNumFaces(); i++)
			for (int v = 0; v < 3; v++) 
				*pIndices++ = getFace(i).getVertIndex(v);
	}

	if (m_BufferIDArray != -1)
	{
		glUnmapBuffer(GL_ARRAY_BUFFER);
		glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
	}
}

/* =============================================================================
    RENDER SCENE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\* This function renders the
    entire scene. RENDER SCENE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*
 =============================================================================== */
void C3DModel::RenderObject(int iObj) const
{
	//  Get the object that we are displaying
	if (!getObjectValid(iObj)) 
		return;

	C3DObject	&Object = editObject(iObj);
	if (m_Hide || Object.getNumVerts() == 0 || Object.getHide()) 
		return;

	bool	bTexture =	(Object.getHasTextureVerts() 
		&&	(Object.getHasMaterial())
		&&	(Object.getMaterial().getTextureId() != -1));

	//  repost mesh to card if necessary
	if (Object.getDirty() || (Object.getBufferIDArray() == -1 && Object.getGLArray() == NULL))
	{
		Object.createGLArray(bTexture, m_Scale, m_Shading);
		Object.setDirty(false);
		if (!Object.getLine() && Object.getBufferIDArray() == -1 && Object.getGLArray() == NULL) 
			return;
	}

	// Check to see if this object has a texture map, if so bind the texture to it.
	if (m_SelectID == -1)
	{
		if (Object.getHasMaterial())
		{
			Vec3f	clr = m_Selected ? Vec3f(75., .25, .25) : clrVec(Object.getMaterial().getColor());
			glColor4f(clr[0], clr[1], clr[2], Object.getTransparency());

			if (m_Wireframe || !m_Lighting || !Object.getLighted())
				glDisable(GL_LIGHTING);
			else
				glEnable(GL_LIGHTING);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glEnable(GL_BLEND);
			m_Wireframe ? glPolygonMode(GL_FRONT, GL_LINE) : glPolygonMode(GL_FRONT, GL_FILL);
		}
		else if (Object.getNumNormals() > 0)
			return;  //  case where waiting for texture for a tile
	}
	else
	{
		int iVal = m_SelectID;
		glColor4ub(iVal & 0xff, (iVal >> 8) & 0xff, (iVal >> 16) & 0xff, 255);
		glPolygonMode(GL_FRONT, GL_FILL);
	}

	//  set up the texture parameters
	if (m_Wireframe || !bTexture || Object.getLine() || m_SelectID != -1)
		glDisable(GL_TEXTURE_2D);
	else
	{
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, Object.getMaterial().getTextureId());
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		if (Object.getMaterial().getMipMap())
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		else
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	}

	if (Object.getLine())
	{
		glLineWidth(1);
		glBegin(GL_LINES);
		for (int i = 0; i < Object.getFaces().size(); i++)
		{
			glVertex(Object.getVerts()[Object.getFaces()[i].getVertIndex(0)]);
			glVertex(Object.getVerts()[Object.getFaces()[i].getVertIndex(1)]);
		}

		glEnd();
		return;
	}

	//  draw the object
	glEnableClientState(GL_VERTEX_ARRAY);
	if (Object.getHasNormals()) 
		glEnableClientState(GL_NORMAL_ARRAY);
	if (Object.getHasTextureVerts())
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	if (Object.getHasColorVerts()) 
		glEnableClientState(GL_COLOR_ARRAY);

	GLenum	type = 0;
	if (bTexture)
		type = Object.getHasColorVerts() ? GL_T2F_C4F_N3F_V3F : GL_T2F_N3F_V3F;
	else
		type = Object.getHasNormals() ? GL_N3F_V3F : GL_V3F;

	if (Object.getBufferIDArray() != -1)
	{
		glBindBuffer(GL_ARRAY_BUFFER, Object.getBufferIDArray());
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Object.getBufferIDIndices());
		glInterleavedArrays(type, 0, 0);
		if (Object.getTriangleStrip())
			glDrawElements(GL_TRIANGLE_STRIP, Object.getNumFaces() + 2, GL_UNSIGNED_INT, 0);
		else
			glDrawElements(GL_TRIANGLES, Object.getNumFaces() * 3, GL_UNSIGNED_INT, 0);
	}
	else if (Object.getGLArray())
	{
		glInterleavedArrays(type, 0, Object.getGLArray());
		if (Object.getTriangleStrip())
			glDrawElements(GL_TRIANGLE_STRIP, Object.getNumFaces() + 2, GL_UNSIGNED_INT, Object.getGLIndices());
		else
			glDrawElements(GL_TRIANGLES, Object.getNumFaces() * 3, GL_UNSIGNED_INT, Object.getGLIndices());
	}
	else
		cout << ("Invalid model state!");
}

/* =============================================================================
 =============================================================================== */
void C3DModel::Render() const
{
	if (m_Hide || getObjectCnt() == 0)
		return;
	glPushMatrix();

	//  do model specific transformation
	glTranslatef(trans[0], trans[1], trans[2]);
	glScalef(scale[0], scale[1], scale[2]);
	if (rot[2]) glRotatef(rot[2], 0, 0, 1);
	if (rot[1]) glRotatef(rot[1], 0, 1, 0);
	if (rot[0]) glRotatef(rot[0], 1, 0, 0);

	if (m_ReverseOrder)
	{
		for (int i = getObjectCnt() - 1; i >= 0; i--) 
			RenderObject(i);
	}
	else
	{
		for (int i = 0; i < getObjectCnt(); i++)
			RenderObject(i);
	}

	glPopMatrix();

	//  clean up settings for next render
	glColor4f(0, 0, 0, 1);
	glPolygonMode(GL_FRONT, GL_FILL);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_TEXTURE_1D);
}

/* =============================================================================
 =============================================================================== */
void C3DModel::sortPlaneObjects(Vec3d cameraDir)
{

	//  if same name and camera dir is opposite of plane dirs then swap
	Vec3d	camDir = norm(cameraDir * Vec3d(1, 1, 0));
	for (int i = 0; i < getObjectCnt(); i++)
	{
		if (getObject(i).getHide() || getObject(i).getName() == "" || getObject(i).getPlaneDir() == vl_0) 
			continue;
		if (dot(getObject(i).getPlaneDir(), camDir) > 0)
			continue;

		//  figure out set to swap direction on
		Vec3d	newDir = getObject(i).getPlaneDir() * -1;
		int		iStart = i;
		for (i; i < getObjectCnt(); i++)
		{
			if (getObject(iStart).getName() != getObject(i).getName()) 
				break;

			//  swap plane dir
			editObject(i).setPlaneDir(newDir);
		}

		i--;

		//  swap the direction of rendering
		int iEnd = i;
		for (; iStart < iEnd; iStart++, iEnd--)
		{
			C3DObject	*tmp = Objects[iStart];
			Objects[iStart] = Objects[iEnd];
			Objects[iEnd] = tmp;
		}
	}
}

