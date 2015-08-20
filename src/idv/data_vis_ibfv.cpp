/* =============================================================================
	File: data_ibfv.cpp

 =============================================================================== */

/*----------------------------------------------------*/
/* ibfv_sample.c - Image Based Flow Visualization */
/* */
/* Jarke J. van Wijk, 2002 */
//  Technische Universiteit Eindhoven
/*----------------------------------------------------*/
/*
#pragma warning(push)
#pragma warning(disable : 4312)
#pragma warning(disable : 4311)
#include <GL/glut.h>
#pragma warning(pop)

#include <stdlib.h>
#include <math.h>

#define NPN 64
#define NMESH 100
#define DM ((float) (1.0/(NMESH-1.0)))
#define NPIX 512
#define SCALE 4.0
int iframe = 0;
const int Npat = 32;
int alpha = (int)(0.12*255);
float sa;
float tmax = NPIX/(SCALE*NPN);
float dmax = SCALE/NPIX;

void initGL(void)
{
	glViewport(0, 0, (GLsizei) NPIX, (GLsizei) NPIX);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glTranslatef(-1.0, -1.0, 0.0);
	glScalef(2.0, 2.0, 1.0);

	glTexParameteri(GL_TEXTURE_2D,	GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D,	GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D,	GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,	GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glEnable(GL_TEXTURE_2D);

	glShadeModel(GL_FLAT);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glClear(GL_COLOR_BUFFER_BIT);
}

GLubyte pat[Npat][NPN][NPN][4];

void makePatterns(void)
{
	int lut[256];
	int phase[NPN][NPN];
	int i, j, k, t;

	for (i = 0; i < 256; i++)
		lut[i] = i < 127 ? 0 : 255;
	for (i = 0; i < NPN; i++)
		for (j = 0; j < NPN; j++)
			phase[i][j] = rand() % 256;
	for (k = 0; k < Npat; k++)
	{
		t = k*256/Npat;
		for (i = 0; i < NPN; i++)
			for (j = 0; j < NPN; j++)
			{
				pat[k][i][j][0] =
				pat[k][i][j][1] =
				pat[k][i][j][2] = lut[(t + phase[i][j]) % 256];
				pat[k][i][j][3] = alpha;
			}
	}
}

void getDP(float x, float y, float *px, float *py)
{
	float dx, dy, vx, vy, r;
	dx = x - 0.5f;
	dy = y - 0.5f;
	r = dx*dx + dy*dy;
	if (r < 0.0001f) r = 0.0001f;
	vx = sa*dx/r + 0.02f;
	vy = sa*dy/r;
	r = vx*vx + vy*vy;
	if (r > dmax*dmax)
	{
		r = sqrt(r);
		vx *= dmax/r;
		vy *= dmax/r;
	}
	*px = x + vx;
	*py = y + vy;
}

void display(void)
{
	int i, j;
	float x1, x2, y, px, py;
	sa = .01f;
	for (i = 0; i < NMESH-1; i++)
	{
		x1 = DM*i;
		x2 = x1 + DM;
		glBegin(GL_QUAD_STRIP);
		for (j = 0; j < NMESH; j++) 
		{
			y = DM*j;

			glTexCoord2f(x1, y);
			getDP(x1, y, &px, &py);
			glVertex2f(px, py);

			glTexCoord2f(x2, y);
			getDP(x2, y, &px, &py);
			glVertex2f(px, py);
		}
		glEnd();
	}
	iframe = iframe + 1;
	glEnable(GL_BLEND);

	int k = iframe % Npat;
	glTexImage2D(GL_TEXTURE_2D, 0, 4, NPN, NPN, 0, GL_RGBA, GL_UNSIGNED_BYTE, pat[k]);

	glBegin(GL_QUAD_STRIP);
	glTexCoord2f(0.0, 0.0); glVertex2f(0.0, 0.0);
	glTexCoord2f(0.0, tmax); glVertex2f(0.0, 1.0);
	glTexCoord2f(tmax, 0.0); glVertex2f(1.0, 0.0);
	glTexCoord2f(tmax, tmax); glVertex2f(1.0, 1.0);
	glEnd();

	glDisable(GL_BLEND);
	glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 0, 0, NPIX, NPIX, 0);
	glutSwapBuffers();
}

int ibfv_main()
{
	char *str = "appname";
	int cnt = 1;
	glutInit(&cnt, &str );
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
	glutInitWindowSize(NPIX, NPIX);
	glutCreateWindow(str);
	glutDisplayFunc(display);
	glutIdleFunc(display);
	initGL();
	makePatterns();
	glutMainLoop();
	return 0;
}
*/