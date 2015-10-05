/*
 * compassWidget.cpp
 *
 *  Created on: Oct 5, 2015
 *      Author: ribells
 */

#include <vrg3d/VRG3D.h>
#ifdef WIN32
#include <GL/gl.h>
#else
#include <GL/gl.h>
#endif

#include "idv/world.h"
#include "idv/gl_draw.h"
#include "idv/settings.h"

#include "compassWidget.h"

//----[ static global variables ]----//
static float g_CurrentDirection;
static int compass_ui_active_index = 1;
static Vec3f g_TWLocation = Vec3f(0.0, 0.0, 0.0);
static Vec3f g_TWRotation = Vec3f(0.0, 0.0, 0.0);
static float active[] = { 0.5, 1.0, 0.5, 1.0 };

/* =============================================================================
    Compass Interface
 =============================================================================== */

/* =============================================================================
 =============================================================================== */
void tm_compass_ui(int iType, float x, float y, float z, float yaw, float pitch, float roll)
{
	bool bStart = (iType == 1);

	g_CurrentDirection = -1*yaw;

	if(compass_ui_active_index >= 0) {
		g_TWLocation = Vec3f(-1*x, -1*y, -1*z);
		glPushMatrix();
		glRotatef(-1*pitch, 1, 0, 0);
		glRotatef(-1*roll, 0, 0, 1);
		glRotatef(-1*yaw, 0, 1, 0);
		draw_compass_ui();
		glLineWidth(1.0f);
		glPopMatrix();
	}
}

int get_compass_ui_current_direction() {
	return g_CurrentDirection;
}

void set_compass_ui_current_direction(double direction) {
	g_CurrentDirection = direction;
}

int set_compass_ui_active_index(int i) {
	compass_ui_active_index = i;
	return i;
}

void draw_compass_ui() {
	float yellow[] = { 1.0, 1.0, 0.0, 0.5 };

	glPushMatrix();
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, yellow);
	glTranslatef(g_TWLocation[0], g_TWLocation[1], g_TWLocation[2]);
	glTranslatef(0.35, 0.3, 0.0);
	glLineWidth(2.0f);
	//Face of Compass
	draw_circle(0.0f, 0.0f, 0.06f, 16);
	glPushMatrix();
	glScalef(0.02, 0.02, 0.02);
	glTranslatef(-0.5, 3.2, 0.0);
	draw_n();
	glTranslatef(3.5, -3.6, 0.0);
	draw_e();
	glTranslatef(-7.0, 0.0, 0.0);
	draw_w();
	glTranslatef(3.5, -3.8, 0.0);
	draw_s();
	glPopMatrix();
	draw_compass_needle();
	glPopMatrix();
}

void draw_compass_needle() {
	float white[] = { 1.0, 1.0, 1.0, 0.5 };

	glPushMatrix();
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, white);
	glRotatef(g_CurrentDirection, 0, 0, 1);
	glBegin(GL_LINES);
		glVertex3f(0.0f,0.0f,0.0f);
		glVertex3f(0.0f,0.06f,0.0f);
	glEnd();
	glPopMatrix();
}

void draw_circle(float cx, float cy, float r, int num_segments)
{
	float theta = 2 * 3.1415926 / float(num_segments);
	float c = cosf(theta);//precalculate the sine and cosine
	float s = sinf(theta);
	float t;

	float x = r; //we start at angle = 0
	float y = 0;

	glBegin(GL_LINE_LOOP);
	for(int ii = 0; ii < num_segments; ii++)
	{
		glVertex2f(x + cx, y + cy);//output vertex

		//apply the rotation matrix
		t = x;
		x = c * x - s * y;
		y = s * t + c * y;
	}
	glEnd();
}

void draw_n()
{
	glBegin(GL_LINE_STRIP);
		glVertex3f(0.2f,0.0f,0.0f);
		glVertex3f(0.2f,1.0f,0.0f);
		glVertex3f(0.8f,0.0f,0.0f);
		glVertex3f(0.8f,1.0f,0.0f);
	glEnd();
}

void draw_e()
{
	glBegin(GL_LINE_STRIP);
		glVertex3f(0.8f,1.0f,0.0f);
		glVertex3f(0.2f,1.0f,0.0f);
		glVertex3f(0.2f,0.5f,0.0f);
		glVertex3f(0.8f,0.5f,0.0f);
		glVertex3f(0.2f,0.5f,0.0f);
		glVertex3f(0.2f,0.0f,0.0f);
		glVertex3f(0.8f,0.0f,0.0f);
	glEnd();
}

void draw_s()
{
	glBegin(GL_LINE_STRIP);
		glVertex3f(0.8f,1.0f,0.0f);
		glVertex3f(0.2f,1.0f,0.0f);
		glVertex3f(0.2f,0.5f,0.0f);
		glVertex3f(0.8f,0.5f,0.0f);
		glVertex3f(0.8f,0.0f,0.0f);
		glVertex3f(0.2f,0.0f,0.0f);
	glEnd();
}

void draw_w()
{
	glBegin(GL_LINE_STRIP);
		glVertex3f(0.1f,1.0f,0.0f);
		glVertex3f(0.3f,0.0f,0.0f);
		glVertex3f(0.5f,1.0f,0.0f);
		glVertex3f(0.7f,0.0f,0.0f);
		glVertex3f(0.9f,1.0f,0.0f);
	glEnd();
}
