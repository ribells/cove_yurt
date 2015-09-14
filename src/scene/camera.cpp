 /* =============================================================================
	File: camera.cpp

  =============================================================================== */

#pragma warning(push)
#pragma warning(disable : 4312)
#pragma warning(disable : 4311)
#include <GL/gl.h>
#pragma warning(pop)
#include "camera.h"
#include "timer.h"
#include "const.h"

const double	CamMin = 0.001 / KmPerDegree;	// 1 Meter
const double	CamMax = 5000.0 / KmPerDegree;	// 5000.0 KM

double			kMouseRotationSensitivity = 0.25 / 180.0;
double			kMouseTranslationXSensitivity = 0.1;
double			kMouseTranslationYSensitivity = 0.1;
double			kMouseZoomSensitivity = 0.1;

Vec3d			projectPnt(Vec3d pnt);
Vec3d			projectUp(Vec3d pnt);
Vec3d			projectCamera(Vec3d pnt, Vec3d cam);
Vec3d			resetLookAt(Vec3d pnt);

/* =============================================================================
 =============================================================================== */

void Camera::setLookAt(const Vec3d lookAt)
{
	m_LookAt = lookAt;
	m_DirtyTransform = true;
}

/* =============================================================================
 =============================================================================== */
Vec3d Camera::getLookAt() const
{
	return m_LookAt;
}

/* =============================================================================
 =============================================================================== */
Vec3d Camera::getPos() const
{
	return m_Position;
}

/* =============================================================================
 =============================================================================== */
double Camera::getDist()
{
	return len(m_Position - m_LookAt);
}

/* =============================================================================
 =============================================================================== */
void Camera::calculateViewingTransformParameters()
{
	m_UpVector = Vec3d(0, 1, 0);
	m_LookAt = resetLookAt(m_LookAt);
	if (m_Azimuth > vl_pi)
		m_Azimuth -= vl_pi*2;
	else if (m_Azimuth < -vl_pi)
		m_Azimuth += vl_pi*2;

	Mat4d	originXform = HTrans4d(m_LookAt);
	Mat4d	dollyXform = HTrans4d(Vec3d(0, 0, m_Dolly));
	Mat4d	azimXform = HRot4d(Vec3d(0, 1, 0), m_Azimuth);
	Mat4d	elevXform = HRot4d(Vec3d(1, 0, 0), m_Elevation);
	Mat4d	twistXform = vl_1;

	//  get camera offset from LookAt
	m_Position = xform(azimXform, xform(elevXform, xform(dollyXform, Vec3d(0, 0, 0))));

	//  offset LookAt from origin
	m_Position = xform(originXform, m_Position);

	m_DirtyTransform = false;
}

/* =============================================================================
 =============================================================================== */
void Camera::setViewingTransform()
{
	Vec3d	lookat = projectPnt(m_LookAt);
	Vec3d	up = projectUp(m_LookAt);
	Vec3d	pos = projectCamera(m_LookAt, m_Position);

		//gluLookAt code
    Vec3d forward = norm(lookat - pos);
    Vec3d side = norm(cross(forward, up));
    up = cross(side, forward);
    Mat4d m(side[0], up[0], -forward[0], 0, side[1], up[1], -forward[1], 0, side[2], up[2], -forward[2], 0, 0,0,0,1);
    //glMultMatrixd(&m[0][0]);
    //glTranslated(-pos[0], -pos[1], -pos[2]);
}

/* =============================================================================
 =============================================================================== */
Camera::Camera()
{
	m_Elevation = m_Azimuth = m_Twist = 0.0f;
	setDolly(200);
	setElevation(-1.6);
	setAzimuth(0);
	setLookAt(vl_0);
	m_CurrentMouseAction = kActionNone;
	m_Animating = false;

	m_Momentum = false;
	m_Impulse = vl_0;
	m_Spin = 0;

	calculateViewingTransformParameters();
}

/* =============================================================================
 =============================================================================== */
void Camera::setSensitivity()
{
	double	camdist = getDist();
	double	factor = max(CamMin, min(CamMax, camdist)) * 0.002;

	kMouseTranslationXSensitivity = factor;
	kMouseTranslationYSensitivity = factor;
	kMouseZoomSensitivity = factor;
}

/* =============================================================================
 =============================================================================== */
void Camera::clickMouse(MouseAction_t action, double x, double y)
{
	m_CurrentMouseAction = action;
	m_LastMousePosition[0] = x;
	m_LastMousePosition[1] = y;
}

/* =============================================================================
 =============================================================================== */
void Camera::dragMouse(double x, double y, bool bDataCam)
{
	Vec3d	mouseDelta = Vec3d(x, y, 0.0f) - m_LastMousePosition;
	m_LastMousePosition = Vec3d(x, y, 0.0f);
	m_Animating = false;

	switch(m_CurrentMouseAction)
	{
	case kActionTranslate:
		if (!bDataCam)
		{
			calculateViewingTransformParameters();

			double	xTrack = -mouseDelta[0] * kMouseTranslationXSensitivity;
			double	yTrack = mouseDelta[1] * kMouseTranslationYSensitivity;

			Vec3d	transXAxis = cross(m_UpVector, (m_Position - m_LookAt));
			normalise(transXAxis);

			Vec3d	transYAxis = cross(m_UpVector, transXAxis);
			normalise(transYAxis);

			Vec3d	offset = transXAxis * xTrack + transYAxis * yTrack;

			m_LookAt += offset;
			m_Impulse = offset;
			m_Spin = 0;
			break;
		}

	case kActionRotate:
		{
			double	dAzimuth = -mouseDelta[0] * kMouseRotationSensitivity;
			double	dElevation = -mouseDelta[1] * kMouseRotationSensitivity;

			setAzimuth(getAzimuth() + dAzimuth);
			setElevation(getElevation() + dElevation);
			m_Impulse = vl_0;
			m_Spin = dAzimuth;
			break;
		}

	case kActionZoom:
		{
			double	dDolly = -mouseDelta[1] * kMouseZoomSensitivity;
			setDolly(max(getDolly() + dDolly, (double) CamMin));
			m_Impulse = vl_0;
			m_Spin = 0;
			break;
		}

	case kActionTwist:
	default:
		break;
	}
}

/* =============================================================================
 =============================================================================== */
void Camera::releaseMouse(double x, double y)
{
	m_CurrentMouseAction = kActionNone;
}

/* =============================================================================
 =============================================================================== */
void resetCamTimer()
{
	g_Timers.startTimer(CAMERA_TIMER);
}

/* =============================================================================
 =============================================================================== */
void normRotate(double &r)
{
	while(r > vl_pi) r -= vl_pi * 2;
	while(r < -vl_pi) r += vl_pi * 2;
}

/* =============================================================================
 =============================================================================== */
void Camera::moveCamera(Vec3d pos, double elev, double azim, double dolly, double fTime, bool bCurve)
{
	if (fTime <= 0.0)
	{
		m_Animating = false;
		setLookAt(pos);
		setElevation(elev);
		setAzimuth(azim);
		setDolly(dolly);
		return;
	}

	if (dolly <= 0) 
		dolly = m_Dolly;
	m_MoveDur = fTime;
	m_MoveTime = 0;
	m_MovePos = pos - m_LookAt;
	m_MoveDolly = dolly - m_Dolly;
	m_MoveDollyPeak = bCurve ? len(m_MovePos) * .3 : 0;
	m_MoveElev = elev - m_Elevation;
	normRotate(m_Azimuth);
	m_MoveAzim = azim - m_Azimuth;
	if (m_MoveDur < 10.0) 
		normRotate(m_MoveAzim);
	g_Timers.startTimer(CAMERA_TIMER);
	m_Animating = true;
	m_Impulse = vl_0;
	m_Spin = 0;
}

/* =============================================================================
 =============================================================================== */
void Camera::applyViewingTransform()
{
	if (m_Animating)
	{
		double	fTime = g_Timers.pollTimer(CAMERA_TIMER);
		if (fTime + m_MoveTime > m_MoveDur) 
			fTime = m_MoveDur - m_MoveTime;

		double	factor = fTime / m_MoveDur;
		m_LookAt += m_MovePos * factor;
		m_Azimuth += m_MoveAzim * factor;
		m_Dolly += m_MoveDolly * factor;

		//  creates a curve when jumping between camera positions
		if (m_MoveDollyPeak)
		{
			m_Dolly -= m_MoveDollyPeak * (sin(vl_pi * m_MoveTime / m_MoveDur)); //  fix from last move
			m_Dolly += m_MoveDollyPeak * (sin(vl_pi * (m_MoveTime + fTime) / m_MoveDur));
		}

		m_Elevation += m_MoveElev * factor;
		m_MoveTime += fTime;
		m_DirtyTransform = true;
		if (fabs(m_MoveTime - m_MoveDur) < 1e-5)
			m_Animating = false;
		else
			g_Timers.startTimer(CAMERA_TIMER);
	}
	else if (m_Momentum)
	{
		double	fTime = g_Timers.pollTimer(CAMERA_TIMER);
		if (m_Spin)
			m_Azimuth += m_Spin * fTime * 2.5;
		else
			m_LookAt += m_Impulse * fTime * 2.5;
		g_Timers.startTimer(CAMERA_TIMER);
		m_DirtyTransform = true;
	}

	calculateViewingTransformParameters();
	setViewingTransform();
}

// from the web to support frustum culling ;
// http://www.crownandcutlass.com/features/technicaldetails/frustum.html ;
// a plane is defined as 4 numbers: the normal vector and distance to the origin

static Vec3f	g_CameraPos;
static Vec3f	plane[6];
static float	plane_d[6];

Vec3f		getRealCameraPos() { return g_CameraPos; }

/* =============================================================================
 =============================================================================== */

void ExtractFrustum()
{
	Mat4f	proj;
	Mat4f	modl;

	glGetFloatv(GL_PROJECTION_MATRIX, proj.Ref());
	glGetFloatv(GL_MODELVIEW_MATRIX, modl.Ref());

	//  extract the camera position
	g_CameraPos = xform(inv(trans(modl)), Vec3f(0, 0, 0));

	//  extract the planes
	Mat4f	clip = modl * proj;
	float	*pf = clip.Ref();

	//  Extract the numbers for the RIGHT plane
	plane[0] = Vec3f(pf[3] - pf[0], pf[7] - pf[4], pf[11] - pf[8]);
	plane_d[0] = pf[15] - pf[12];

	//  Extract the numbers for the LEFT plane
	plane[1] = Vec3f(pf[3] + pf[0], pf[7] + pf[4], pf[11] + pf[8]);
	plane_d[1] = pf[15] + pf[12];

	//  Extract the BOTTOM plane
	plane[2] = Vec3f(pf[3] + pf[1], pf[7] + pf[5], pf[11] + pf[9]);
	plane_d[2] = pf[15] + pf[13];

	//  Extract the TOP plane
	plane[3] = Vec3f(pf[3] - pf[1], pf[7] - pf[5], pf[11] - pf[9]);
	plane_d[3] = pf[15] - pf[13];

	//  Extract the FAR plane
	plane[4] = Vec3f(pf[3] - pf[2], pf[7] - pf[6], pf[11] - pf[10]);
	plane_d[4] = pf[15] - pf[14];

	//  Extract the NEAR plane
	plane[5] = Vec3f(pf[3] + pf[2], pf[7] + pf[6], pf[11] + pf[10]);
	plane_d[5] = pf[15] + pf[14];

	//  Normalize the result
	for (int i = 0; i < 6; i++)
	{
		float	t = len(plane[i]);
		plane[i] /= t;
		plane_d[i] /= t;
	}
}

/* =============================================================================
 =============================================================================== */
bool PointInFrustum(Vec3f pnt)
{
	int p;

	for (p = 0; p < 6; p++)
		if (dot(plane[p], pnt) <= -plane_d[p]) 
			return false;
	return true;
}

/* =============================================================================
 =============================================================================== */
bool SphereInFrustum(Vec3f center, float radius)
{
	int p;

	for (p = 0; p < 6; p++)
		if (dot(plane[p], center) <= -(plane_d[p] + radius)) 
			return false;
	return true;
}
