#pragma once

#include "vl/VLd.h"
#include "vl/VLf.h"
using namespace std;

//==========[ class Camera ]===================================================

typedef enum { kActionNone, kActionTranslate, kActionRotate, kActionZoom, kActionTwist,} MouseAction_t;

class Camera {

private:
	double		m_Elevation;
	double		m_Azimuth;
	double		m_Dolly;
	double		m_Twist;

	Vec3d		m_LookAt;
	Vec3d		m_Position;
	Vec3d		m_UpVector;
	bool		m_DirtyTransform;

	bool		m_Momentum;
	Vec3d		m_Impulse;
	double		m_Spin;

	bool		m_Animating;
	double		m_MoveTime;
	double		m_MoveDur;
	Vec3d		m_MovePos;
	double		m_MoveAzim;
	double		m_MoveDolly;
	double		m_MoveDollyPeak;
	double		m_MoveElev;

	Vec3d			m_LastMousePosition;
	MouseAction_t	m_CurrentMouseAction;

	void calculateViewingTransformParameters();

public:

	//---[ Constructors ]----------------------------------

	Camera();

	//---[ Settings ]--------------------------------------

	inline double getElevation() const { return m_Elevation; }
	inline void setElevation( double elevation )  //clamp between -1.6 and 1.6
	{ m_Elevation = max(min(elevation,vl_pi/2-1e-4),-vl_pi/2+1e-4); m_DirtyTransform = true; }
	inline double getAzimuth() const { return m_Azimuth; }
	inline void setAzimuth( double azimuth ) { m_Azimuth = azimuth; m_DirtyTransform = true; }
	inline double getDolly() const { return m_Dolly; }
	inline void setDolly( double dolly ) { m_Dolly = dolly; m_DirtyTransform = true; }
	inline double getTwist() const { return m_Twist; }
	inline void setTwist( double twist ) { m_Twist = twist; m_DirtyTransform = true; }

	inline Vec3d getUpVector() const { return m_UpVector; }

	bool getMomentum() { return m_Momentum; }
	void setMomentum(bool bSet) { m_Momentum = bSet; m_Impulse = vl_0; m_Spin = 0; }
	bool getAnimating() { return m_Animating || m_Momentum; }
	void setAnimating(bool b) { m_Animating = b; }

	Vec3d getLookAt() const;
	void setLookAt( const Vec3d lookAt );
	Vec3d getPos() const;
	double getDist();

	void clickMouse( MouseAction_t action, double x, double y );
	void dragMouse( double x, double y, bool bDataCam = false);
	void releaseMouse( double x, double y );

	//---[ Viewing Transform ]--------------------------------

	void applyViewingTransform();
	void setViewingTransform();
	void setSensitivity();
	void moveCamera(Vec3d pos, double elev, double azim, double dolly, double fTime, bool bCurve = true);
};
