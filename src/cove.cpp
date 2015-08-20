/*
 * cove.cpp
 *
 * Author: Bruce Donald Campbell
 * Created: June 25, 2015
 */

#include <GL/gl.h>
#include <GL/glu.h>
#include <stdlib.h>
#include "idv/world.h"
#include "idv/gl_draw.h"
//#include "scene/scene_mgr.h"
#include "scene/image.h"
#include <FL/Fl_JPEG_Image.H>

void draw(void);

void init_cove(void)
{
	g_Draw.initState();

	string strFileName = "Axial_Textures_4-13-15.003.jpg";
	//string strFileName = "Axial_bottom_terrain2_1_1.jpg";
	Fl_Image * img = new Fl_JPEG_Image (strFileName.c_str());
	if (img->d() == 0) {
		return;
	}
	int width = img->w();
	int height = img->h();
	const unsigned char *pData = (const unsigned char *) img->data()[0];
	int textureID = createTexture(pData, width, height, GL_RGB, GL_RGB, TX_CLAMP_EDGE);
	/*
	static GLuint texid;
	glGenTextures(textureID, &texid);
	glBindTexture(GL_TEXTURE_2D,texid);
	glTexEnvi(GL_TEXTURE_2D, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
	*/

	//string strFileName = "Axial_Textures_4-13-15.003.jpg";
	strFileName = "Axial_bottom_terrain2_1_1.jpg";
	img = new Fl_JPEG_Image (strFileName.c_str());
	if (img->d() == 0) {
		return;
	}
	width = img->w();
	height = img->h();
	const unsigned char *pData2 = (const unsigned char *) img->data()[0];
	textureID = createTexture(pData2, width, height, GL_RGB, GL_RGB, TX_CLAMP_EDGE);
	delete img;
	/*
	static GLuint texid2;
	glGenTextures(textureID, &texid2);
	glBindTexture(GL_TEXTURE_2D,texid2);
	*/
}

#define SPEED 10.0f

/* draw the objects. */
void draw_cove(void)
{
	g_World.updateTerrain();
	g_Draw.drawGL();

	GLUquadricObj *sphereObj, *diskObj, *cylinderObj;
	GLfloat pos[] = {-130.0, 0, -46.0, 1};
	float white[] = { 0.8, 0.8, 0.8, 0.5 };
	float yellow[] = { 0.8, 0.8, 0.2, 0.7 };
	float green[] = { 0.2, 0.8, 0.2, 1.0 };
	float blue[] = { 0.2, 0.2, 0.5, 1.0 };
	float brown[] = { 0.1, 0.1, 0.0, 1.0 };
	float red[] = { 0.8, 0.2, 0.2, 0.5 };
	glLightfv(GL_LIGHT0, GL_POSITION, pos);
	glEnable(GL_LIGHT0);

	//add some primitives to keep our bearings while we are developing
	sphereObj = gluNewQuadric();
	diskObj = gluNewQuadric();
	cylinderObj = gluNewQuadric();

	glPushMatrix();
	glTranslatef(-130.0, 0.08, -46.0);
/*
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, blue);
	glEnable(GL_TEXTURE_2D);
	glBegin(GL_QUADS);
		glTexCoord2f(0.0, 1.0); glVertex3f(0.2f,0.0f,-0.12f);
		glTexCoord2f(1.0, 1.0); glVertex3f(0.2f,0.0f,0.2f);
		glTexCoord2f(1.0, 0.0); glVertex3f(-0.12f,0.0f,0.2f);
		glTexCoord2f(0.0, 0.0); glVertex3f(-0.12f,0.0f,-0.12f);
	glEnd();
*/
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, brown);
	glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0); glVertex3f(-0.12f,-0.2f,-0.12f);
		glTexCoord2f(1.0, 0.0); glVertex3f(-0.12f,-0.2f,0.2f);
		glTexCoord2f(1.0, 1.0); glVertex3f(0.2f,-0.2f,0.2f);
		glTexCoord2f(0.0, 1.0); glVertex3f(0.2f,-0.2f,-0.12f);
	glEnd();
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, white);
	//Box for depth
	glBegin(GL_LINE_STRIP);
		glVertex3f(0.2f,0.0f,-0.12f);
		glVertex3f(0.2f,0.0f,0.2f);
		glVertex3f(-0.12f,0.0f,0.2f);
		glVertex3f(-0.12f,0.0f,-0.12f);
		glVertex3f(0.2f,0.0f,-0.12f);
	glEnd();
	glBegin(GL_LINE_STRIP);
		glVertex3f(0.2f,-0.05f,-0.12f);
		glVertex3f(0.2f,-0.05f,0.2f);
		glVertex3f(-0.12f,-0.05f,0.2f);
		glVertex3f(-0.12f,-0.05f,-0.12f);
		glVertex3f(0.2f,-0.05f,-0.12f);
	glEnd();
	glBegin(GL_LINE_STRIP);
		glVertex3f(0.2f,-0.1f,-0.12f);
		glVertex3f(0.2f,-0.1f,0.2f);
		glVertex3f(-0.12f,-0.1f,0.2f);
		glVertex3f(-0.12f,-0.1f,-0.12f);
		glVertex3f(0.2f,-0.1f,-0.12f);
	glEnd();
	glBegin(GL_LINE_STRIP);
		glVertex3f(0.2f,-0.15f,-0.12f);
		glVertex3f(0.2f,-0.15f,0.2f);
		glVertex3f(-0.12f,-0.15f,0.2f);
		glVertex3f(-0.12f,-0.15f,-0.12f);
		glVertex3f(0.2f,-0.15f,-0.12f);
	glEnd();
	glBegin(GL_LINE_STRIP);
		glVertex3f(0.2f,-0.2f,-0.12f);
		glVertex3f(0.2f,-0.2f,0.2f);
		glVertex3f(-0.12f,-0.2f,0.2f);
		glVertex3f(-0.12f,-0.2f,-0.12f);
		glVertex3f(0.2f,-0.2f,-0.12f);
	glEnd();
	//130 degrees west, 46 degrees north
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, green);
	glBegin(GL_LINES);
		glVertex3f(0.0f,0.1f,0.0f);
		glVertex3f(0.0f,-0.2f,0.0f);
	glEnd();
	//sensors
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, yellow);
	glBegin(GL_LINES);
		glVertex3f(0.008f,0.1f,0.06644f);
		glVertex3f(0.008f,-0.2f,0.06644f);
	glEnd();
	glBegin(GL_LINES);
		glVertex3f(-0.0141f,0.1f,0.06623f);
		glVertex3f(-0.0141f,-0.2f,0.06623f);
	glEnd();
	glBegin(GL_LINES);
		glVertex3f(0.2633f,0.1f,0.17982f);
		glVertex3f(0.2633f,-0.2f,0.17982f);
	glEnd();
	glBegin(GL_LINES);
		glVertex3f(-0.0089f,0.1f,0.04532f);
		glVertex3f(-0.0089f,-0.2f,0.04532f);
	glEnd();
	glBegin(GL_LINES);
		glVertex3f(0.0203f,0.1f,0.05042f);
		glVertex3f(0.0203f,-0.2f,0.05042f);
	glEnd();
	glBegin(GL_LINES);
		glVertex3f(0.0262f,0.1f,0.06033f);
		glVertex3f(0.0262f,-0.2f,0.06033f);
	glEnd();
	glBegin(GL_LINES);
		glVertex3f(0.0215f,0.1f,0.06393f);
		glVertex3f(0.0215f,-0.2f,0.06393f);
	glEnd();
	glBegin(GL_LINES);
		glVertex3f(0.022f,0.1f,0.07427f);
		glVertex3f(0.022f,-0.2f,0.07427f);
	glEnd();
/*
	glTranslatef(0.0, 0.0, -40.0);
	//gluCylinder(cylinderObj, 1.6, 1.0, 3, 10, 2);
	glTranslatef(0.0, 0.0, 80.0);
	gluDisk(diskObj, 0, 1.6, 10, 1);
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, red);
	//draw sphere where internal COVE camera's look at point is
	Vec3d look_at = g_Draw.getCamera().getLookAt();
	glTranslatef(look_at[0], look_at[1], look_at[2]);
	glScalef(10.010, 10.010, 10.010);
	gluSphere(sphereObj, 1.6, 8, 8);
*/
	glPopMatrix();
}
