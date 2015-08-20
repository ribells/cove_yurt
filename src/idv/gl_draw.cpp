/* =============================================================================
	File: gl_draw.cpp

 =============================================================================== */

#pragma warning(push)
#pragma warning(disable : 4312)
#pragma warning(disable : 4311)
#ifdef WIN32
#include <GL/gl.h>
#include <GL/glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif
//#include "glext.h"
#pragma warning(pop)
#include <string.h>
#include "timer.h"

#include "utility.h"
#include "world.h"

#include "gl_draw.h"
#include "settings.h"
#include "web/web_file.h"

#include <vl/VLgl.h>

CDraw	g_Draw;

//------[ function prototypes ]------//

void	d_updateSliderPos();
Vec3d	getCameraFromGPS(Vec3d pos, t_scale scale);
Vec3d	getGPSFromCamera(Vec3d pos, t_scale scale);

/* =============================================================================
    [ intersectLinePlane ]
 =============================================================================== */
bool intersectLinePlane(Vec3d LiP0, Vec3d LiP1, Vec3d PnV0, Vec3d PnN, Vec3d &I)
{
	Vec3d	u = LiP1 - LiP0;
	Vec3d	w = LiP0 - PnV0;
	double	D = dot(PnN, u);
	double	N = -dot(PnN, w);

	if (fabs(D) < 1e-6){	//  segment is parallel to plane
		if (N == 0)	{		//  segment lies in plane
			I = LiP0;
			return true;
		}
		else {
			return false;	//  no intersection
		}
	}

	double	sI = N / D;
	if (sI < 0)
		return false;	//  intersection behind camera

	I = LiP0 + sI * u;	//  compute segment intersect point
	return true;
}

/* =============================================================================
    [ intersectUnitSphere ]
 =============================================================================== */
bool intersectSphere(Vec3d &localRayStart, Vec3d &localRayEnd, double radius, Vec3d *localIsect, double *dist)
{
	Vec3d	dir = (localRayEnd - localRayStart);
	double	a = sqrlen(dir);
	double	b = dot(localRayStart, dir);
	double	c = dot(localRayStart, localRayStart) - radius * radius;
	double	disc = b * b - a * c;

	if (disc <= 0)
		return false;

	disc = sqrt(disc);

	double	t0 = (-b + disc) / a;
	if (t0 <= 0.000001)
		return false;

	double	t1 = (-b - disc) / a;

	if (t1 > 0.000001)
	{
		*localIsect = localRayStart + t1 * dir;
		*dist = t1;
	}
	else
	{
		*localIsect = localRayStart + t0 * dir;
		*dist = t0;
	}

	return true;
}

/* =============================================================================
 =============================================================================== */
void getSceneRay(int x, int y, Vec3d &rayStart, Vec3d &rayEnd)
{
	double	projMat[16];
	double	mvMat[16];
	GLint	viewport[4];
	double	wx, wy, wz;

	glGetDoublev(GL_PROJECTION_MATRIX, projMat);
	glGetDoublev(GL_MODELVIEW_MATRIX, mvMat);
	glGetIntegerv(GL_VIEWPORT, viewport);

	gluUnProject(x, y, 0.0, mvMat, projMat, viewport, &wx, &wy, &wz);
	rayStart = Vec3d(wx, wy, wz);

	gluUnProject(x, y, 1.0, mvMat, projMat, viewport, &wx, &wy, &wz);
	rayEnd = Vec3d(wx, wy, wz);
}

/* =============================================================================
    assumes that sphere center is origin
 =============================================================================== */
bool getSphereIntersect(int x, int y, Vec3d Center, double radius, Vec3d &pnt)
{
	Vec3d	rayStart, rayEnd;
	getSceneRay(x, y, rayStart, rayEnd);

	double	dist;
	double	rad = min(radius, len(rayStart));
	return intersectSphere(rayStart, rayEnd, rad, &pnt, &dist);
}

/* =============================================================================
 =============================================================================== */
bool getPlaneIntersect(int x, int y, Vec3d *verts, Vec3d &pnt)
{
	Vec3d	rayStart, rayEnd;
	getSceneRay(x, y, rayStart, rayEnd);

	Vec3d	vNorm = norm(cross(verts[1] - verts[0], verts[2] - verts[0]));
	return intersectLinePlane(rayStart, rayEnd, verts[0], vNorm, pnt);
}

/* =============================================================================
    [ get a string with the position ]
 =============================================================================== */
string getStringFromPosition(Vec3d pos, bool bHTML)
{
	string	strPos;
	char	pch[64] = "";
	if (g_Set.m_PositionUnits == 4)
	{
		double	zx, zy;
		double	utm_e, utm_n;
		double	j_e, j_n;
		if (getUTMfromLL(g_Set.m_LocalOrigin[0], g_Set.m_LocalOrigin[1], zx, zy, utm_e, utm_n)
			&&	getUTMfromLL(pos[0], pos[1], zx, zy, j_e, j_n))
			sprintf(pch, "%.2lf  %.2lf", j_e - utm_e, j_n - utm_n);
	}
	else if (g_Set.m_PositionUnits == 3)
	{
		double	zx, zy;
		double	utm_e, utm_n;
		if (getUTMfromLL(pos[0], pos[1], zx, zy, utm_e, utm_n))
			sprintf(pch, "%i%c  %.2lf  %.2lf", (int) zx, (char) zy, utm_e, utm_n);
	}
	else if (g_Set.m_PositionUnits == 2)
	{
		strPos = getDegMinSecString(pos[0], true, bHTML);
		strPos += pos[0] >= 0 ? " N   " : " S  ";
		strPos += getDegMinSecString(pos[1], true, bHTML);
		strPos += pos[1] >= 0 ? " E" : " W";
	}
	else if (g_Set.m_PositionUnits == 1)
	{
		strPos = getDegMinSecString(pos[0], false, bHTML);
		strPos += pos[0] >= 0 ? " N   " : " S  ";
		strPos += getDegMinSecString(pos[1], false, bHTML);
		strPos += pos[1] >= 0 ? " E" : " W";
	}
	else
	{
		if (bHTML) {
			sprintf(pch, "%.6lf  %.6lf", pos[0], pos[1]);
		} else {
			sprintf(pch, "%.6lf%c  %.6lf%c", pos[0], 0xba, pos[1], 0xba);
		}
	}

	return strlen(pch) > 0 ? pch : strPos;
}

/* =============================================================================
 =============================================================================== */
bool getPositionFromString(string strPos, Vec3d &pos)
{
	pos = vl_0;

	bool	bSuccess = false;
	double	lond, latd;
	for (int i = 0; i < strPos.length(); i++)
		if (strPos[i] == '\'') 
			strPos[i] = ' ';
	if (g_Set.m_PositionUnits == 4)
	{
		double	j_e, j_n;
		int		iRet = sscanf(strPos.c_str(), "%lf  %lf", &j_e, &j_n);
		if (iRet == 2)
		{
			double	zx, zy;
			double	utm_e, utm_n;
			if (getUTMfromLL(g_Set.m_LocalOrigin[0], g_Set.m_LocalOrigin[1], zx, zy, utm_e, utm_n))
			{
				getLLfromUTM(zx, zy, utm_e + j_e, utm_n + j_n, latd, lond);
				bSuccess = true;
			}
		}
	}
	else if (g_Set.m_PositionUnits == 3)
	{
		int		zx;
		char	zy;
		double	utm_e, utm_n;
		int		iRet = sscanf(strPos.c_str(), "%i%c  %lf  %lf", &zx, &zy, &utm_e, &utm_n);
		if (iRet == 4)
		{
			getLLfromUTM(zx, zy, utm_e, utm_n, latd, lond);
			bSuccess = true;
		}
	}
	else if (g_Set.m_PositionUnits == 2)
	{
		char	EW, NS;
		double	lonm, lons;
		double	latm, lats;
		int		iRet = sscanf
			(
				strPos.c_str(),
				"%lf %lf %lf %c  %lf %lf %lf %c",
				&latd,
				&latm,
				&lats,
				&NS,
				&lond,
				&lonm,
				&lons,
				&EW
			);
		if (iRet == 8)
		{
			bSuccess = true;
			latd += latm / 60.0 + lats / 3600.0;
			if (toupper(NS) == 'S') latd *= -1;
			lond += lonm / 60.0 + lons / 3600.0;
			if (toupper(EW) == 'W') lond *= -1;
		}
	}
	else if (g_Set.m_PositionUnits == 1)
	{
		char	EW, NS;
		double	lonm;
		double	latm;
		int		iRet = sscanf(strPos.c_str(), "%lf %lf %c  %lf %lf %c", &latd, &latm, &NS, &lond, &lonm, &EW);
		if (iRet == 6)
		{
			bSuccess = true;
			latd += latm / 60.0;
			if (toupper(NS) == 'S') latd *= -1;
			lond += lonm / 60.0;
			if (toupper(EW) == 'W') lond *= -1;
		}
	}
	else
	{
		int iRet = sscanf(strPos.c_str(), "%lf  %lf", &latd, &lond);
		if (iRet == 2) bSuccess = true;
	}

	if (bSuccess)
		pos = Vec3d(latd, lond, 0);
	else
		cout << ("ERROR: invalid position");
	return bSuccess;
}

/* =============================================================================
 =============================================================================== */
string getDistanceString(double dist)
{
	string	strOut;
	if (dist < 5.0)
		strOut = davesCommas(dist * 1000.0f) + " m";
	else if (g_Set.m_DistanceUnits == 0)
		strOut = davesCommas(dist, true) + " km";
	else if (g_Set.m_DistanceUnits == 1)
		strOut = davesCommas(dist * 0.621371192, true) + " mi";
	else if (g_Set.m_DistanceUnits == 2)
		strOut = davesCommas(dist * 0.539956803, true) + " nm";
	return strOut;
}

/* =============================================================================
 =============================================================================== */
Vec3f CDraw::get2DPoint(Vec3f pos) const
{
	double	projMat[16];
	double	mvMat[16];
	GLint	viewport[4];
	double	x, y, z;

	glGetIntegerv(GL_VIEWPORT, viewport);
	glGetDoublev(GL_PROJECTION_MATRIX, projMat);
	glGetDoublev(GL_MODELVIEW_MATRIX, mvMat);

	gluProject(pos[0], pos[1], pos[2], mvMat, projMat, viewport, &x, &y, &z);
	return Vec3f(x, m_Height - y, z);
}

/* =============================================================================
 =============================================================================== */
Vec3f CDraw::get2DPointGPS(Vec3d pos) const
{
	return get2DPoint(g_Draw.getScenePntFromGPS(pos));
}

/* =============================================================================
 =============================================================================== */
Vec3f CDraw::get3DPoint(int x2, int y2, double depth) const
{
	double	projMat[16];
	double	mvMat[16];
	GLint	viewport[4];

	glGetIntegerv(GL_VIEWPORT, viewport);
	glGetDoublev(GL_PROJECTION_MATRIX, projMat);
	glGetDoublev(GL_MODELVIEW_MATRIX, mvMat);

	double	x, y, z;
	gluUnProject(x2, m_Height - y2, depth, mvMat, projMat, viewport, &x, &y, &z);
	return Vec3f(x, y, z);
}

/* =============================================================================
    [ initGL ]
 =============================================================================== */
void CDraw::initState()
{
	//  create light
	GLfloat lightDiffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	GLfloat lightAmbient[] = { 0.1f, 0.1f, 0.1f, 1.0f };
	GLfloat lightSpecular[] = { 0.1f, 0.1f, 0.1f, 0.1f };

	glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);
	glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpecular);

	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);

	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);

	//  create material
	GLfloat matSpecular[] = { 0.1f, 0.1f, 0.1f, 1.0f };
	GLfloat matShininess[] = { 5.0f };

	glMaterialfv(GL_FRONT, GL_SPECULAR, matSpecular);
	glMaterialfv(GL_FRONT, GL_SHININESS, matShininess);

	glEnable(GL_COLOR_MATERIAL);
	glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

	//  smooth points and lines
	glEnable(GL_POINT_SMOOTH);
	glHint(GL_POINT_SMOOTH_HINT, GL_FASTEST);
	glEnable(GL_LINE_SMOOTH);
	glHint(GL_LINE_SMOOTH_HINT, GL_FASTEST);

	glDisable(GL_BLEND);
	glAlphaFunc(GL_GREATER, 0.01);
	glEnable(GL_ALPHA_TEST);

	glDepthFunc(GL_LEQUAL);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glCullFace(GL_BACK);
	glEnable(GL_CULL_FACE);

	glHint(GL_TEXTURE_COMPRESSION_HINT,GL_NICEST);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT,GL_NICEST);

	//  we use scale xforms, so renormalize the normals
	glEnable(GL_RESCALE_NORMAL);
}

/* =============================================================================
    [ resize ]
 =============================================================================== */
void setPerspective(GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar)
{
	GLdouble xmin, xmax, ymin, ymax;

	ymax = zNear * tan(fovy * vl_pi / 360.0);
	ymin = -ymax;
	xmin = ymin * aspect;
	xmax = ymax * aspect;

	glFrustum(xmin, xmax, ymin, ymax, zNear, zFar);
}

void CDraw::resize(int w, int h)
{
	//  set up the viewport and projection matrix
	if (w && h)
	{
		glViewport(0, 0, w, h);
		setScreenExtents(w, h);
	}

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	//  figure out far and near based on camera dist
	float	f_Aspect = float(m_Width) / float(m_Height);
	float	camdist = getCamera().getDist();
	m_Far_P = min(max(camdist * 10.0, 1.0), 1e4);
	m_Near_P = max(min(camdist / 100.0, 1.0), 1e-6);

	if (g_Set.m_MapView) //orthographic camera
	{
		float	d = camdist * tan(vl_pi * 0.5 * m_FOV / 180);
		glOrtho(-d * f_Aspect, d * f_Aspect, -d, d, m_Near_P, m_Far_P);
	}
	else {
		setPerspective(m_FOV, f_Aspect, m_Near_P, m_Far_P);
	}
}

/* =============================================================================
 =============================================================================== */
void CDraw::resize_glwindow(int w, int h)
{
	resize(w, h);
	if (g_World.getInvalid()) {
		clear_screen();
	}
	resetWindows();
	g_Set.m_NeedRedraw = true;
}

//  Utility Functions
static const double g_GotoTime = 3.0;

/* =============================================================================
 =============================================================================== */

bool CDraw::getMinCellSize(double m_LonMin, double m_LatMin, double m_LonMax, double m_LatMax, double &fMinCell) const
{
	return g_World.getTerrainSet().getMinCellSize(m_LonMin, m_LatMin, m_LonMax, m_LatMax, fMinCell);
}

/* =============================================================================
 =============================================================================== */

bool CDraw::GetTerrainHeight(double dLat, double dLong, double &dDepth) const
{
	float fDepth;
	bool bRet = g_World.getTerrain().getPosHeight(dLat, dLong, fDepth);
	dDepth = fDepth;
	return bRet;
}

/* =============================================================================
 =============================================================================== */
bool CDraw::setTerrainHeight(Vec3d &pos) const
{
	float	fDepth;
	bool	bRet = g_World.getTerrain().getPosHeight(pos[0], pos[1], fDepth);
	if (bRet) 
		pos[2] = fDepth;
	return bRet;
}

/* =============================================================================
 =============================================================================== */
void CDraw::resetLookAtHeight()
{
	//  check that lookat isn't below surface
	Vec3d	pos = getLookAtGPS();
	if (GetTerrainHeight(pos[0], pos[1], pos[2])) 
		setLookAtGPS(pos);

	//  check that camera position isn't below surface
	getCamera().setElevation(min(-.04, getCamera().getElevation()));
}

/* =============================================================================
 =============================================================================== */
Vec3f CDraw::getScenePntFromGPS(Vec3d pntGPS, bool bScaled) const
{
	t_scale	scale;
	return ::getScenePntFromGPS(pntGPS, bScaled ? g_World.getScale() : scale);
}

/* =============================================================================
 =============================================================================== */
Vec3d CDraw::getGPSFromScenePnt(Vec3f pos) const
{
	return ::getGPSFromScenePnt(pos, g_World.getScale());
}

/* =============================================================================
 =============================================================================== */
Vec3d CDraw::getCameraFromGPS(Vec3d pntGPS) const
{
	return ::getCameraFromGPS(pntGPS, g_World.getScale());
}

/* =============================================================================
 =============================================================================== */
Vec3d CDraw::getGPSFromCamera(Vec3d pnt) const
{
	return ::getGPSFromCamera(pnt, g_World.getScale());
}

/* =============================================================================
 =============================================================================== */
Vec3d CDraw::getLookAtGPS() const
{
	return getGPSFromCamera(getCamera().getLookAt());
}

/* =============================================================================
 =============================================================================== */
void CDraw::setLookAtGPS(const Vec3d lookAt)
{
	getCamera().setLookAt(g_Draw.getCameraFromGPS(lookAt));
}

/* =============================================================================
 =============================================================================== */
Vec3f CDraw::getCameraPosition() const
{
	return getScenePntFromGPS(getGPSFromCamera(getCamera().getPos()));
}

/* =============================================================================
 =============================================================================== */
Vec3d CDraw::getCameraPositionGPS() const
{
	return getGPSFromCamera(getCamera().getPos());
}

/* =============================================================================
 =============================================================================== */
double CDraw::getCameraDistKM() const
{
	return getCamera().getDist() * KmPerDegree;
}

/* =============================================================================
 =============================================================================== */
double CDraw::getCameraDistKM(Vec3f pos) const
{
	return len(getCameraPosition() - pos) * KmPerDegree;
}

/* =============================================================================
 =============================================================================== */
double CDraw::getCameraDistKM_GPS(Vec3d pos) const
{
	return getCameraDistKM(getScenePntFromGPS(pos));
}

/* =============================================================================
 =============================================================================== */
void CDraw::gotoCameraView(Vec3d pntGPS, double elev, double azim, double dolly, double time)
{
	if (time == -1.0) 
		time = g_GotoTime;
	if (!g_Set.m_AnimateCamera) 
		time = 0.0;
	m_ValidPos = true;
	g_Set.m_HoldCursorHeight = false;

	double	depth;
	if (GetTerrainHeight(pntGPS[0], pntGPS[1], depth))
		if (depth + 100 < pntGPS[2]) 
			g_Set.m_HoldCursorHeight = true;	//  simple check to see if off bottom
	getCamera().setAnimating(false);
	if (!g_Set.m_AnimateCamera || g_Set.m_ServerMode) 
		time = 0.0;

	//  wrap around the globe
	if (time != 0.0 && g_Set.m_Projection == PROJ_GLOBE)
	{
		Vec3d	cur = getLookAtGPS();
		Vec3d	vec = pntGPS - cur;
		if (vec[1] > 180) cur[1] += 360;
		if (vec[1] < -180) cur[1] -= 360;
		setLookAtGPS(cur);
	}

	getCamera().moveCamera(getCameraFromGPS(pntGPS), elev, azim, dolly, time);
	m_TerrainPos = pntGPS;
	m_ValidPos = true;
	g_Set.m_NeedRedraw = true;
}

/* =============================================================================
 =============================================================================== */
void CDraw::gotoCameraViewRange(Vec3d min_pos, Vec3d max_pos)
{
	Vec3d	pos = (min_pos + max_pos) / 2;
	pos[2] *= -1;

	double	dolly = min(max(len((max_pos - min_pos) * Vec3d(1, 1, 0)), .001), 200.0);
	double	elev = -.55 - ((max_pos[1]-min_pos[1])/360.0); //range from -1.55 to -.55 based on height
	double	azim = getCamera().getAzimuth();
	gotoCameraView(pos, elev, azim, dolly);
}

/* =============================================================================
 =============================================================================== */
void CDraw::gotoGPSPoint(Vec3d pnt, double fDistKm)
{
	double	dist = fDistKm > 0 ? fDistKm / KmPerDegree : getCamera().getDolly();
	gotoCameraView(pnt, getCamera().getElevation(), getCamera().getAzimuth(), dist, 2);
	setTerrainPos(pnt);
}

/* =============================================================================
 =============================================================================== */
void CDraw::gotoGPSRange(Vec3d pnt1, Vec3d pnt2, double fDist)
{
	if (fDist <= 0.0) 
		fDist = len((pnt1 - pnt2) * Vec3d(1, 1, 0)) * KmPerDegree;
	gotoGPSPoint((pnt1 + pnt2) / 2, fDist);
}

/* =============================================================================
 =============================================================================== */
void CDraw::rotateCamera(double time, bool bCW)
{
	if (getCamera().getAnimating() || time <= 0.0)
		getCamera().setAnimating(false);
	else if (g_Set.m_Projection == PROJ_GLOBE && getCamera().getDist() > 30.0)
	{
		getCamera().moveCamera
			(
				getCamera().getLookAt() + Vec3d(360 * (bCW ? -1 : 1), 0, 0),
				getCamera().getElevation(),
				getCamera().getAzimuth(),
				getCamera().getDolly(),
				time,
				false
			);
	}
	else
	{
		getCamera().moveCamera
			(
				getCamera().getLookAt(),
				getCamera().getElevation(),
				getCamera().getAzimuth() + 2 * vl_pi * (bCW ? -1 : 1),
				getCamera().getDolly(),
				time
			);
	}
}

/* =============================================================================
 =============================================================================== */
void CDraw::getCameraView(Vec3d &pos, double &elev, double &azim, double &dolly)
{
	pos = getLookAtGPS();
	dolly = getCamera().getDolly();
	elev = getCamera().getElevation();
	azim = getCamera().getAzimuth();
}

/* =============================================================================
 =============================================================================== */
bool CDraw::inTopView() const
{
	return((vl_pi / 2 - fabs(getCamera().getElevation())) < 1e-3 && getCamera().getAzimuth() == 0.0);
}

/* =============================================================================
 =============================================================================== */
void CDraw::setViewAngle(int iType)
{
	Vec3d	pos;
	double	elev, azim, dolly;
	getCameraView(pos, elev, azim, dolly);

	switch(iType)
	{
	case ANGLE_TOP:
		{
			static Vec3d	pos_save = vl_0;
			static double	elev_save, azim_save, dolly_save;
			if (inTopView())
			{
				if (pos_save == vl_0)	//  starting in top view - do nothing
					return;
				pos = pos_save;
				elev = elev_save;
				azim = azim_save;
				dolly = dolly_save;
			}
			else
			{
				getCameraView(pos_save, elev_save, azim_save, dolly_save);	//  store old view
				elev = -vl_pi / 2;
				azim = 0.0;
			}
		}
		break;

	case ANGLE_LEFT:
		azim -= vl_pi / 2;
		if (azim < -vl_pi)
			azim += vl_pi * 2;
		break;

	case ANGLE_RIGHT:
		azim += vl_pi / 2;
		if (azim > vl_pi) 
			azim -= vl_pi * 2;
		break;

	case ANGLE_BACK:
		azim += vl_pi;
		if (azim > vl_pi)
			azim -= vl_pi * 3;
		break;

	case ANGLE_ZOOMIN:
		dolly /= 2;
		break;

	case ANGLE_ZOOMOUT:
		dolly *= 2;
		break;

	default:
		return;
	}

	gotoCameraView(pos, elev, azim, dolly, 0);
}

/* =============================================================================
 =============================================================================== */
int CDraw::addCurrentView()
{
	CView	View;
	g_World.getViewSet().addView(View);

	int ndx = g_World.getViewSet().getCnt() - 1;
	updateView(ndx);
	return ndx;
}

/* =============================================================================
 =============================================================================== */
void CDraw::updateView(int ndx)
{
	if (ndx == -1)
		return;
	g_World.getViewSet().updateView
		(
			ndx,
			getLookAtGPS(),
			getCamera().getElevation(),
			getCamera().getAzimuth(),
			getCamera().getDolly()
		);
	g_World.getViewSet().updateViewTime
		(
			ndx,
			g_World.getTimeLine().getStart(),
			g_World.getTimeLine().getFinish(),
			g_World.getTimeLine().getSelStart(),
			g_World.getTimeLine().getSelFinish(),
			g_World.getTimeLine().getLead(),
			g_World.getTimeLine().getTrail(),
			g_World.getTimeLine().getTime()
		);
}

/* =============================================================================
 =============================================================================== */
bool CDraw::getCameraExtent(Vec3d &minpos, Vec3d &maxpos)
{
	Vec3d	crnr[] =
	{
		Vec3d(m_Width - 1, 1, 0),
		Vec3d(m_Width - 1, m_Height - 1, 0),
		Vec3d(1, 1, 0),
		Vec3d(1, m_Height - 1, 0)
	};
	for (int i = 0; i < 4; i++)
	{
		Vec3d	pos;
		if (!getTerrainIntersect(crnr[i][0], crnr[i][1], pos)) 
			return false;
		if (i == 0)
		{
			minpos = maxpos = pos;
		}

		//  for globe case near dateline
		if (i >= 2)
		{
			if (pos[1] - 180.0 > minpos[1]) 
				pos[1] -= 360;
		}

		for (int j = 0; j < 2; j++)
		{
			if (minpos[j] > pos[j]) minpos[j] = pos[j];
			if (maxpos[j] < pos[j]) maxpos[j] = pos[j];
		}
	}

	if (minpos[0] == maxpos[0] || minpos[1] == maxpos[1])
		return false;
	return true;
}

/* =============================================================================
 =============================================================================== */
void CDraw::resetScreenBounds()
{
	int h = 0;
	int realtop = 0;
	if (g_Set.m_ShowDB)
	{
		h += DB_H;
		if (g_Set.m_ShowDBTop) 
			realtop += DB_H;
	}

	if (g_Set.m_ShowVB)
	{
		h += VB_H;
		if (g_Set.m_ShowVBTop) 
			realtop += VB_H;
	}

	setScreenOffsets(realtop, (h - realtop));
}

/* =============================================================================
    [ follow playing data ]
 =============================================================================== */
Vec3f SetObjectRotations(Vec3f pos, Vec3f dir)
{
	double	xRot, yRot;
	if (dir[2] > 1.0 - 1e-4)
	{
		yRot = 0;
		xRot = 90;
	}
	else
	{
		Vec3f	tmpdir = dir * Vec3f(1, 0, -1);
		if (tmpdir == vl_0) 
			tmpdir = dir;

		Vec3f	lldir = norm(tmpdir);
		yRot = asin(lldir[0]);
		yRot = lldir[2] < 0 ? yRot : lldir[0] > 0 ? -vl_pi - yRot : vl_pi - yRot;
		xRot = -asin(dir[1]);
	}

	return Vec3f(xRot, yRot, 0);
}

/* =============================================================================
 =============================================================================== */
void SetObjectPosition(CObjectPtr pObject, Vec3f pos, Vec3f dir)
{
	if (pObject == 0)
		return;

	CObject &Object = *pObject;
	Vec3f	rot = SetObjectRotations(pos, dir);
	Object.setPosition(g_Draw.getGPSFromScenePnt(pos));
	Object.setSceneTx(pos);
	Object.setRotation(getVec3d(rot * 180 / vl_pi));
}

/* =============================================================================
 =============================================================================== */
CObjectPtr SetObjectFromName(string strID)
{
	CObjectTypePtr	pType = g_World.getLayoutTypes().getObjectTypePtr(strID);
	if (pType == NULL)
		return NULL;

	CObjectPtr	pObject = new CObject();
	CObject			&Object = *pObject;
	Object.setObjectType(pType);
	Object.setName(Object.getObjectType().getName());
	return pObject;
}

/* =============================================================================
 =============================================================================== */
void CDraw::SetCameraPosition(Vec3f pos, Vec3f dir)
{
	setDataCamPos(getGPSFromScenePnt(pos));
	setLookAtGPS(getDataCamPos());
	if (dir == vl_0)
		return;

	// if a follow camera then, get current amount that is real camera ;
	// double elev = getCamera().getElevation() + m_DataCamOffset[0];
	double	azim = getCamera().getAzimuth() - m_DataCamOffset[1];

	//  now get new rotation and add it in
	m_DataCamOffset = SetObjectRotations(pos, dir);
	m_DataCamOffset[1] -= vl_pi;

	// getCamera().setElevation(-m_DataCamOffset[0]+elev);
	getCamera().setAzimuth(m_DataCamOffset[1] + azim);
}

/* =============================================================================
 =============================================================================== */
Vec3d CDraw::getLightPosition() const
{
	Mat4d	dirXform = HRot4d(Vec3d(0, 1, 0), getLightDir() * vl_pi / 180.0);
	Mat4d	angleXform = HRot4d(Vec3d(1, 0, 0), -getLightAngle() * vl_pi / 180.0);
	Vec3d	m_Position = xform(dirXform, xform(angleXform, Vec3d(0, 0, 1)));
	return m_Position;
}

/* =============================================================================
 =============================================================================== */
void CDraw::drawLightVector()
{
	if (!m_ValidPos)
		return;
	glColor4f(.9, .9, .1, 0.8);

	Vec3d	pnt = getLookAtGPS();
	double	len = (getCameraDistKM() * 0.1) / KmPerDegree;
	Vec3d	vec = pnt + getGPSFromCamera(getLightPosition()) * len;

	Vec3f	pos = getScenePntFromGPS(pnt);
	Vec3f	dir = getScenePntFromGPS(vec);
	double	camdist = g_Draw.getCameraDistKM(pos);
	float	fRad = camdist * 2.0 / MetersPerDegree;

	drawArrow(pos, dir, fRad);
}

//  [ draw the GL window ]
Vec3d	reprojectLight(Vec3d pnt, Vec3d light);

/* =============================================================================
 =============================================================================== */

void CDraw::clear_screen()
{
	//  clear the GL drawing surface and depth buffer
	Vec3f	c = clrVec(g_Set.m_BackColor);
	float	trans = clrTrans(g_Set.m_BackColor);
	//glClearColor(c[0], c[1], c[2], trans);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

/* =============================================================================
 =============================================================================== */
void CDraw::drawVerticalImages()
{
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glDisable(GL_LIGHTING);
	glDisable(GL_CULL_FACE);

	for (int i = 0; i < g_World.getTextureSet().getTextureLayerCnt(); i++)
	{
		CTextureLayer	&Texture = g_World.getTextureLayer(i);
		if (!Texture.getActive() || !Texture.getVertical())
			continue;

		int iTxID = Texture.getScreenTex();
		if (iTxID == -1)
		{
			int		w, h, txw;
			string	strLocal = cacheWebFile(Texture.getLocalFilePath());

			/*
			//create video texture
			if (Texture.getMoviePtr())
			{
				Texture.getMoviePtr()->getMovieSize(w, h);
				iTxID = createVideoTexture(Texture.editMovie());
			}
			else
				iTxID = createUITexture(strLocal, w, h, txw);
			*/
			iTxID = -1;
			if (iTxID == -1)
			{
				cout << ("Unable to create texture for " + Texture.getName());
				continue;
			}

			Texture.setScreenTex(iTxID);
			Texture.setScreenW(w);
			Texture.setScreenH(h);
			Texture.setLocalFilePath(strLocal);
		}

		//if (Texture.getMoviePtr())
		//	Texture.setVideoFrame(g_World.getTimeLine().getTime() + g_World.getTimeLine().getFrameTime());
		//else
			glBindTexture(GL_TEXTURE_2D, iTxID);

		glColor4f(1, 1, 1, Texture.getTransparency());

		Vec3d	minpos = Texture.getMinPos();
		Vec3d	maxpos = Texture.getMaxPos();

		double	distMeasure = getKMfromGPS(minpos, maxpos);

		// if bottom and right are filled in then use them to set width of image
		int w = Texture.getScreenW();
		int h = Texture.getScreenH();

		if (Texture.getLayerType() != LAYER_SEALEVEL)
		{
			setTerrainHeight(minpos);
			setTerrainHeight(maxpos);
			if (Texture.getLayerType() == LAYER_POSITIVE)
			{
				minpos[2] = max(0.0,minpos[2]);
				maxpos[2] = max(0.0,maxpos[2]);
			}
			else if (Texture.getLayerType() == LAYER_NEGATIVE)
			{
				minpos[2] = min(0.0,minpos[2]);
				maxpos[2] = min(0.0,maxpos[2]);
			}
		}
		minpos[2] += Texture.getPosOffset()[2];
		maxpos[2] += Texture.getPosOffset()[2];

		Vec3f scenePos[4];
		Vec3d offset_h = Vec3d(0,0,distMeasure*1000*(double)h*Texture.getVertScale()/(double)w);
		double scale = minpos[2] > 0 ? g_World.getScale().land : g_World.getScale().sea;
		scenePos[0] = getScenePntFromGPS(recenterGPS(minpos));
		scenePos[2] = getScenePntFromGPS(recenterGPS(minpos + offset_h / scale));
		scale = maxpos[2] > 0 ? g_World.getScale().land : g_World.getScale().sea;
		scenePos[1] = getScenePntFromGPS(recenterGPS(maxpos));
		scenePos[3] = getScenePntFromGPS(recenterGPS(maxpos + offset_h / scale));

		//draw image
		//  set position, center on x, y
		glBegin(GL_QUADS);
		glTexCoord2f(0, 1);
		glVertex(scenePos[0]);
		glTexCoord2f(1, 1);
		glVertex(scenePos[1]);
		glTexCoord2f(1, 0);
		glVertex(scenePos[3]);
		glTexCoord2f(0, 0);
		glVertex(scenePos[2]);
		glEnd();
	}

	glDisable(GL_TEXTURE_2D);
	glEnable(GL_LIGHTING);
	glEnable(GL_CULL_FACE);
	glColor4f(1, 1, 1, 1);
}

/* =============================================================================
 =============================================================================== */
bool CDraw::init_drawGL()
{
	if (g_World.getInvalid()) {
		return false;
	}

	//clear_screen();

	//  switch to the model-view matrix and clear the transform
	glMatrixMode(GL_MODELVIEW);
	//glLoadIdentity();

	//  update the camera if attached to data
	if (g_Set.m_ShowData) {
		//  update all the dataset points
		Vec3f	pos, dir;
		int		cam_type;
		setCameraTracking(g_World.getDataSet().PrepareDataSets(pos, dir, cam_type));
		if (getCameraTracking()) {
			SetCameraPosition(pos, cam_type == CAM_FOLLOW ? dir : vl_0);
		}
	}

	g_World.getTextureSet().prepareTextures(g_World.getTimeLine().getTime() + g_World.getTimeLine().getFrameTime());

	//  apply the camera transform
	getCamera().applyViewingTransform();
	//TEST getCamera().setSensitivity();
	//TEST resize();

	//  switch to the model-view matrix and clear the transform
	glMatrixMode(GL_MODELVIEW);

	//  set up the lighting for projection
	Vec3d	pnt = getLookAtGPS();
	Vec3d	vec = pnt + getGPSFromCamera(getLightPosition());
	Vec3f	light = getScenePntFromGPS(vec) - getScenePntFromGPS(pnt);
	float	f[4] = { light[0], light[1], light[2], 0 };
	glLightfv(GL_LIGHT0, GL_POSITION, f);

	//  set up for culling
	ExtractFrustum();

	glEnable(GL_DEPTH_TEST);

	return true;
}

/* =============================================================================
 =============================================================================== */
bool CDraw::drawGL()
{
	static bool bDrawing = false;
	if (bDrawing) {
		return false;
	}
//	g_World.getTimeLine().setTrail(g_World.getTimeLine().getStart() + 12796674);
//	printf("Drawing world at %f\n", g_World.getTimeLine().getSpeed());
//	printf("Drawing world at %f\n", g_World.getTimeLine().getFrameTime());
	g_Timers.startPerfTimer(GPU_PERF_TIMER);
	g_Timers.startTimer(TRN_RENDER_TIMER);
	g_Timers.startTimer(DATA_RENDER_TIMER);

	double camdist = g_Draw.getCameraDistKM();
	if (g_Set.m_ShowTerrain && g_Set.m_AutoRecenter && g_Draw.getValidPos() && camdist < 25.0)
	{
		double move_dist = getKMfromGPS(g_World.getTerrain().getMeshCenterGPS(), g_Draw.getLookAtGPS());
		if (move_dist > max(250.0, camdist * 20.0))
		{
			g_World.recenterTerrain(g_Draw.getLookAtGPS());
			cout << (" - recentering terrain");
		}
	}

	if (!init_drawGL()) {
		g_Timers.stopPerfTimer(GPU_PERF_TIMER);
		return false;
	}

	bDrawing = true;

	glEnable(GL_LIGHTING);
	glShadeModel(GL_SMOOTH);
	glMatrixMode(GL_MODELVIEW);

	//  water rendering
	glDisable(GL_FOG);
	if (g_Set.m_FogFactor > 0.0) {
		g_World.getTerrain().setFog(g_Set.m_FogFactor, g_Set.m_FogColor);
	}

	//  scene rendering
	if (g_Set.m_ShowTerrain) {
		bool bScaleTileFactor = g_Set.m_AutoTileFactor;
		if (g_World.getTerrain().getNeedUpdate()) {
			g_World.getTerrain().setTileScale(1.0);
			g_World.updateTerrain();
			bScaleTileFactor = false;
		}

		static double dLastTileScale = 1.0;
		if (dLastTileScale != g_World.getTerrain().getTileScale()) {
			dLastTileScale = g_World.getTerrain().getTileScale();
			bScaleTileFactor = false;
		}

		glDepthRange(0, 1);

		double dTimeStart = g_Timers.pollTimer(COVE_TIMER);
		g_Timers.startTimer(TRN_RENDER_TIMER);
		g_World.getTerrain().Render();
		g_Timers.pollTimer(TRN_RENDER_TIMER);
		double dDur = g_Timers.pollTimer(COVE_TIMER) - dTimeStart;
		double dTileScale = g_World.getTerrain().getTileScale();

		//scale down terrain tile factor
		if (bScaleTileFactor) {
			static int iSlowCount = 0;
			if (dDur > FRAME_TIME*3.0 && dTileScale > 0.2) {
				iSlowCount++;
				if (iSlowCount > 2) { //3 slow frames in a row
					g_World.getTerrain().setTileScale(max(0.5, (g_World.getTerrain().getTileScale() - 0.5)));
					if (g_Env.m_Verbose) {
						cout << " - scaling down terrain factor";
					}
					iSlowCount = 0;
				}
			} else {
				iSlowCount = 0;
			}
			
			static int iFastCount = 0;
			if (dDur < FRAME_TIME/3.0 && dTileScale < 1.0) {
				iFastCount++;
				if (iFastCount > 10) { //10 fast frames in a row
					g_World.getTerrain().setTileScale(min(1.0, g_World.getTerrain().getTileScale()+0.1));
					if (g_Env.m_Verbose) {
						cout << (" - scaling up terrain factor");
					}
					iFastCount = 0;
				}
			} else {
				iFastCount = 0;
			}
		}

		drawVerticalImages();

		//  draw grid lines
		glDepthRange(0, 0.999);
		glDisable(GL_LIGHTING);

		double	cam_dist = getCameraDistKM();
		int		iSel;
		g_Set.m_ShowLatLong = false;
		if (g_Set.m_ShowLatLong) { // degrees
			double	fGridVals[] = { 10, 5.0, 2.5, 1.0, .5, .25, 5.0 / 60.0, 1.0 / 60.0, 1.0 / 300.0, 0 };
			for (iSel = 0; fGridVals[iSel] > 0; iSel++) {
				if (fGridVals[iSel] * 2.0 < cam_dist / KmPerDegree) {
					break;
				}
			}

			g_World.getTerrain().drawLLGrid(getLookAtGPS(),fGridVals[iSel],
					g_Set.m_GridFactor,g_Set.m_ShowGridLabels,g_Set.m_GridColor);
		}

		if (g_Set.m_ShowUTMGrid && cam_dist < 2000) { // meters
			double	fGridVals[] = { 10000, 2500, 1000, 250, 100, 25, 10, 2.5, 1, 0 };
			for (iSel = 0; fGridVals[iSel] > 0; iSel++) {
				if (fGridVals[iSel] * .01 < cam_dist) {
					break;
				}
			}

			g_World.getTerrain().drawUTMGrid(getLookAtGPS(),fGridVals[iSel],
				g_Set.m_GridFactor,g_Set.m_ShowGridLabels,g_Set.m_GridColor);
		}

		//  draw outlines
		glDepthRange(0, .9995);
		if (g_Set.m_ShowOutlines) {
			g_World.getTerrain().drawOutlines();
		}
		if (m_ListOutlines.size()) {
			g_World.getTerrain().drawOutlineList(m_ListOutlines, 0xcc006666);
		}
		glDepthRange(0, .9995);
		if (m_SelListOutlines.size()) {
			g_World.getTerrain().drawOutlineList(m_SelListOutlines, CLR_RED);
		}

		glEnable(GL_LIGHTING);
	}

	glDepthRange(0, .9998);

	g_World.getLayoutSet().Render(g_Set.m_ShowObjects, g_Set.m_ShowLines);

	//  draw the data
	glEnable(GL_NORMALIZE);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	GLfloat lightpos[] = {.5,1.,1.,0.};
	glLightfv(GL_LIGHT0, GL_POSITION, lightpos);

	if (g_Set.m_ShowData) {
		g_Timers.startTimer(DATA_RENDER_TIMER);
		double spd = g_World.getTimeLine().getSpeed();
		double temptime = (double) g_World.getTimeLine().getTime() + spd/100.0;
		g_World.getTimeLine().setCurTime(temptime);
		//printf("Drawing world at %f\n", temptime);
		g_World.getDataSet().Render(spd);
		g_Timers.pollTimer(DATA_RENDER_TIMER);
	}

	//glDisable(GL_NORMALIZE);

	//glDepthRange(0, 1);

	//if (g_Set.m_ShowNames && g_Set.m_ShowObjects) {
	//	g_World.getLayoutSet().RenderNames();
	//}

	//g_Draw.draw3DText("TESTING", Vec3f(0.0, 0.0, 30.0), 0, 0, true);

	//drawWindows();

	g_Timers.stopPerfTimer(GPU_PERF_TIMER);

	if (g_Env.m_Verbose && !g_Set.m_Printing) {
		GLenum	error;
		if ((error = glGetError()) != GL_NO_ERROR) {
			cout << ((string) "OpenGL error: " + (const char *) gluErrorString(error));
		}
	}

	bDrawing = false;

	return true;
}

/* =============================================================================
 =============================================================================== */
bool CDraw::getTerrainIntersect(int xPos, int yPos, Vec3d &pos)
{
	m_ValidPos = false;
	g_Set.m_ShowData = false;
	if (init_drawGL())
	{
		g_World.getTerrain().getTextureSet().Render();

		float	fDepth = 1.0;
		glReadPixels(xPos, m_Height - yPos, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &fDepth);
		if (fDepth != 1.0)
		{
			Vec3f	p = get3DPoint(xPos, yPos, fDepth);
			pos = getGPSFromScenePnt(p);
			GetTerrainHeight(pos[0], pos[1], pos[2]);
			m_TerrainPos = pos;
			m_ValidPos = true;
		}
	}

	g_Set.m_ShowData = true;
	return m_ValidPos;
}

/* =============================================================================
 =============================================================================== */
bool CDraw::getScreenSelect(int xPos, int yPos, int &iType, int &iLayer, int &iObj, int &iSubObj)
{
	iType = iObj = iLayer = iSubObj = -1;
	yPos = m_Height - yPos;

	unsigned char	pVal[4] = { 0 };
	int				*pSel = (int *) pVal;
	glReadBuffer(GL_BACK);

	//  clear the GL drawing surface and depth buffer and set state
	glClearColor(1, 1, 1, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glShadeModel(GL_FLAT);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	glDisable(GL_FOG);
	glDisable(GL_POINT_SMOOTH);
	glDisable(GL_LINE_SMOOTH);

	//  apply the camera transform
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	getCamera().setViewingTransform();

	//  check for objects
	*pSel = -1;
	if (g_Set.m_ShowObjects)
	{
		iType = SEL_OBJ;
		g_World.getLayoutSet().RenderObjects(true);
		glReadPixels(xPos, yPos, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pVal);
		if (*pSel != -1)
		{
			swapEndian(true, pSel, sizeof(int), 1);
			getObjectPickValue(*pSel, iLayer, iObj);
		}
	}

	//  check for lines
	if (*pSel == -1 && g_Set.m_ShowLines)
	{
		iType = SEL_LINE;
		glDisable(GL_LINE_SMOOTH);
		g_World.getLayoutSet().RenderLines(true);
		glReadPixels(xPos, yPos, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pVal);
		if (*pSel != -1)
		{
			swapEndian(true, pSel, sizeof(int), 1);
			getLinePickValue(*pSel, iLayer, iObj, iSubObj);
		}

		glEnable(GL_LINE_SMOOTH);
	}

	//  check for data
	if (*pSel == -1)
	{
		iType = SEL_DATA;
		for (int i = 0; i < g_World.getDataSet().getDataLayerCnt() && iLayer < 0; i++)
		{
			if (!g_World.getDataLayer(i).getActive())
				continue;
			g_World.getDataLayer(i).RenderData(g_World.getTimeLine().getSpeed(), i);
			glReadPixels(xPos, yPos, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pVal);
			if (*pSel != -1)
			{
				swapEndian(true, pSel, sizeof(int), 1);
				iObj = *pSel & 0x00ffffff;
				iLayer = i;
			}
		}
	}

	glEnable(GL_LIGHTING);
	glShadeModel(GL_SMOOTH);
	glEnable(GL_POINT_SMOOTH);
	glEnable(GL_LINE_SMOOTH);

	if (g_Env.m_Verbose)
	{
		ostringstream	s;
		s << " user selected type: " << iType << ", layer: " << iLayer << ", iObj: " << iObj << ", iSubObj: " << iSubObj;
		cout << (s.str());
	}

	return(*pSel != -1);
}
