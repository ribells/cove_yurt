/*
 * timeWidget.cpp
 *
 *  Created on: Sep 4, 2015
 *      Author: Bruce Campbell
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

#include "timeWidget.h"

//----[ static global variables ]----//
static string g_TimeString;
static int g_TimeSel = 0;
static int g_CurTime = 0;
static int g_LeadSel = 0;
static int g_TrailSel = 0;
static bool g_Animating = true;
static bool g_Looping = true;
static Vec3f g_TWLocation = Vec3f(0.0, 0.0, 0.0);
static Vec3f g_TWRotation = Vec3f(0.0, 0.0, 0.0);
static int time_ui_active_index = 9;
static float active[] = { 0.75, 1.0, 0.75, 1.0 };

const static int CURRENT_TIME = 0;
const static int START_TIME = 1;
const static int END_TIME = 2;
const static int LEAD_TIME = 3;
const static int TRAIL_TIME = 4;
const static int CURRENT_SPEED = 5;
const static int SLOW_DOWN = 6;
const static int PLAY_REVERSE = 7;
const static int STOP_PLAY = 8;
const static int PLAY_FORWARD = 9;
const static int SPEED_UP = 10;
const static int LOOP = 11;
const static int ANIMATE = 12;

/* =============================================================================
 Time Interface
 =============================================================================== */

/* =============================================================================
 =============================================================================== */
void tm_time_ui(int iType, float x, float y, float z, float yaw, float pitch,
		float roll) {
	bool bStart = (iType == 1);

	vector<long int> vTime;
	int iCnt = 0;
	for (int i = 0; i < g_World.getDataSet().getDataLayerCnt(); i++) {
		if (g_World.getDataLayer(i).getActive()) {
			CDataLayer &DataLayer = g_World.getDataLayer(i);
			vTime.push_back(
					bStart ? DataLayer.getOffsetStart()
							: DataLayer.getOffsetFinish());
		}
	}

	if (g_TimeSel < 0) {
		return;
	}

	long int tm = 0;
	g_TimeString = getNiceDateTimeString(g_World.getTimeLine().getTime());
	/*
	 double start = 0.0;
	 double finish = 0.0;
	 g_World.getDataSet().getTimeBounds(start, finish);
	 g_TimeString = getNiceDateTimeString(start);
	 */
	if (g_TimeSel == vTime.size()) {
		if (g_TimeString == "") {
			return;
		}
		tm = scanDateTime(g_TimeString);
		if (tm <= 0) {
			cout << "Unable to read the entered time";
			return;
		}
	} else {
		tm = vTime[g_TimeSel];
	}

	if (time_ui_active_index >= 0) {
		g_TWLocation = Vec3f(-1 * x, -1 * y, -1 * z);
		//cout << "creating GUI at " << pitch << ":" << yaw << ":" << roll << "\n";
		glPushMatrix();
		glRotatef(-1 * pitch, 1, 0, 0);
		glRotatef(-1 * roll, 0, 0, 1);
		glRotatef(-1 * yaw, 0, 1, 0);
		glTranslatef(0.0, 4.0, -10.0);
		glLineWidth(2.0f);
		draw_current_time_ui();
		draw_lead_time_ui();
		draw_trail_time_ui();
		draw_speed_ui();
		draw_play_controls_ui();
		draw_labels();
		glLineWidth(1.0f);
		glPopMatrix();
	}

	//Set the trail to be the max time initially
	if (g_TrailSel == 0) {
		g_World.getTimeLine().setTrail(
				g_World.getTimeLine().getFinish()
						- g_World.getTimeLine().getSelStart());
	}
	//Set the lead to be the max time initially
	if (g_LeadSel == 0) {
		g_World.getTimeLine().setLead(
				g_World.getTimeLine().getFinish()
						- g_World.getTimeLine().getSelStart());
	}
	//TO DO: Draw Labels for Current Values
	//g_Draw.draw3DText("TESTING", Vec3f(130.0,3.0,47.0), 0, 0, true);
	/*
	 g_World.getTimeLine().updateBounds(bStart ? tm : NO_TIME, bStart ? NO_TIME : tm);
	 g_World.getDataSet().setSelStart(g_World.getTimeLine().getSelStart());
	 g_World.getDataSet().setSelFinish(g_World.getTimeLine().getSelFinish());
	 g_Set.m_NeedRedraw = true;
	 */
}

int get_time_ui_current_time() {
	return g_CurTime;
}

int set_time_ui_active_index(int i) {
	time_ui_active_index = i;
	return i;
}

int set_time_ui_value(int i) {
	double dtime;
	float timestep = (g_World.getTimeLine().getFinish()
			- g_World.getTimeLine().getStart()) / 1256.0;
	switch (time_ui_active_index) {
	case CURRENT_TIME:
		if (i > 0) {
			dtime = g_World.getTimeLine().getTime() + timestep;
			if (dtime < g_World.getTimeLine().getSelFinish()) {
				g_World.getTimeLine().setCurTime(dtime);
				g_CurTime += timestep;
			}
		} else {
			dtime = g_World.getTimeLine().getTime() - timestep;
			if (dtime > g_World.getTimeLine().getSelStart()) {
				g_World.getTimeLine().setCurTime(dtime);
				g_CurTime -= timestep;
			}
		}
		break;
	case START_TIME:
		if (i > 0) { //move start time ahead
			dtime = g_World.getTimeLine().getSelStart() + timestep;
			if (dtime < g_World.getTimeLine().getSelFinish()) {
				g_World.getTimeLine().setSelStart(dtime);
				//g_World.getDataSet().setSelStart(dtime);
				if (dtime > g_World.getTimeLine().getTime()) {
					g_World.getTimeLine().setCurTime(dtime);
					g_CurTime += timestep;
				}
			}
		} else { //move start time back
			dtime = g_World.getTimeLine().getSelStart() - timestep;
			if (dtime >= g_World.getTimeLine().getStart()) {
				g_World.getTimeLine().setSelStart(dtime);
				//g_World.getDataSet().setSelStart(dtime);
			}
		}
		break;
	case END_TIME:
		if (i > 0) { //move end time ahead
			dtime = g_World.getTimeLine().getSelFinish() + timestep;
			if (dtime <= g_World.getTimeLine().getFinish()) {
				g_World.getTimeLine().setSelFinish(dtime);
				//g_World.getDataSet().setSelFinish(dtime);
			}
		} else { //move end time back
			dtime = g_World.getTimeLine().getSelFinish() - timestep;
			if (dtime > g_World.getTimeLine().getSelStart()) {
				g_World.getTimeLine().setSelFinish(dtime);
				//g_World.getDataSet().setSelFinish(dtime);
				if (dtime < g_World.getTimeLine().getTime()) {
					g_World.getTimeLine().setCurTime(dtime);
					g_CurTime -= timestep;
				}
			}
		}
		break;
	case LEAD_TIME:
		if (i > 0) { //decrease the lead time
			dtime = g_World.getTimeLine().getLead() - timestep;
			g_LeadSel = 1;
			if (g_World.getTimeLine().getLead()
					> g_World.getTimeLine().getTime()
							- g_World.getTimeLine().getStart()) {
				g_World.getTimeLine().setLead(
						g_World.getTimeLine().getTime()
								- g_World.getTimeLine().getStart() - timestep);
			} else {
				g_World.getTimeLine().setLead(dtime);
				g_World.getDataSet().setLead(dtime);
			}
			if (dtime >= g_World.getTimeLine().getSelStart()) {
				g_World.getTimeLine().setLead(dtime);
				g_World.getDataSet().setLead(dtime);
			}
		} else { //increase the lead time
			dtime = g_World.getTimeLine().getLead() + timestep;
			g_LeadSel = 1;
			//don't let the lead get ahead of current time
			if (dtime <= g_World.getTimeLine().getTime()
					- g_World.getTimeLine().getStart()) {
				g_World.getTimeLine().setLead(dtime);
				g_World.getDataSet().setLead(dtime);
			}
		}
		break;
	case TRAIL_TIME:
		if (i > 0) { //increase the trail time
			dtime = g_World.getTimeLine().getTrail() + timestep;
			g_TrailSel = 1;
			if (dtime <= g_World.getTimeLine().getSelFinish()) {
				g_World.getTimeLine().setTrail(dtime);
				g_World.getDataSet().setTrail(dtime);
			}
		} else { //decrease the trail time
			dtime = g_World.getTimeLine().getTrail() - timestep;
			g_TrailSel = 1;
			if (g_World.getTimeLine().getTrail()
					>= g_World.getTimeLine().getFinish()
							- g_World.getTimeLine().getTime()) {
				g_World.getTimeLine().setTrail(
						g_World.getTimeLine().getFinish()
								- g_World.getTimeLine().getTime() - timestep);
			} else {
				g_World.getTimeLine().setTrail(dtime);
				g_World.getDataSet().setTrail(dtime);
			}
			if (dtime <= g_World.getTimeLine().getSelFinish()) {
				g_World.getTimeLine().setTrail(dtime);
				g_World.getDataSet().setTrail(dtime);
			}
		}
		break;
	case CURRENT_SPEED:
		if (i > 0) {
			g_World.getTimeLine().setFaster();
		} else {
			g_World.getTimeLine().setSlower();
		}
		//cout << "Speed is " << g_World.getTimeLine().getSpeed() << "\n";
		break;
	case SLOW_DOWN:
		if (i > 0) {
			g_World.getTimeLine().setSlower();
		} else {
			g_World.getTimeLine().setSlower();
		}
		//cout << "Speed is " << g_World.getTimeLine().getSpeed() << "\n";
		break;
	case PLAY_REVERSE:
		g_CurTime = (int) g_World.getTimeLine().getTime();
		g_World.getTimeLine().setPlay(false, true);
		g_Animating = false;
		g_World.getTimeLine().setTime(g_CurTime);
		break;
	case STOP_PLAY:
		g_CurTime = (int) g_World.getTimeLine().getTime();
		g_World.getTimeLine().setPlay(false, false);
		g_World.getTimeLine().setCurTime(g_CurTime);
		break;
	case PLAY_FORWARD:
		g_CurTime = (int) g_World.getTimeLine().getTime();
		g_World.getTimeLine().setPlay(true, false);
		g_World.getTimeLine().setTime(g_CurTime);
		break;
	case SPEED_UP:
		if (i > 0) {
			g_World.getTimeLine().setFaster();
		} else {
			g_World.getTimeLine().setFaster();
		}
		//cout << "Speed is " << g_World.getTimeLine().getSpeed() << "\n";
		break;
	case LOOP:
		if (i > 0) {
			g_Looping = true;
		} else {
			g_Looping = false;
		}
		//cout << "Speed is " << g_World.getTimeLine().getSpeed() << "\n";
		break;
	case ANIMATE:
		if (i > 0) {
			g_Animating = true;
		} else {
			g_Animating = false;
		}
		//cout << "Speed is " << g_World.getTimeLine().getSpeed() << "\n";
		break;
	}
}

int get_time_ui_active_index() {
	return time_ui_active_index;
}

int get_next_ui_active_index() {
	if (time_ui_active_index == ANIMATE) {
		return set_time_ui_active_index(CURRENT_TIME);
	} else {
		return ++time_ui_active_index;
	}
}

int get_previous_ui_active_index() {
	if (time_ui_active_index == CURRENT_TIME) {
		return set_time_ui_active_index(SPEED_UP);
	} else {
		return --time_ui_active_index;
	}
}

bool tm_get_should_animate() {
	return g_Animating;
}

bool tm_get_should_loop() {
	return g_Looping;
}

/* =============================================================================
 =============================================================================== */
void show_timeline(bool b) {
	g_Set.m_ShowTimeline = b;
}

/* =============================================================================
 =============================================================================== */
void show_layout_timeline(bool bShow) {
	if (bShow) {
		time_ui_active_index = STOP_PLAY;
	} else {
		time_ui_active_index = -1;
	}
}

void draw_current_time_ui() {
	float white[] = { 1.0, 1.0, 1.0, 0.5 };
	float yellow[] = { 1.0, 1.0, 0.0, 0.5 };
	GLUquadricObj *sphereObj;
	sphereObj = gluNewQuadric();

	glPushMatrix();
	glTranslatef(g_TWLocation[0], g_TWLocation[1], g_TWLocation[2]);
	//Line for range of time
	glBegin(GL_LINES);
	glVertex3f(0.0f, 0.0f, 0.0f);
	glVertex3f(2.5f, 0.0f, 0.0f);
	glEnd();
	glScalef(0.05, 0.05, 0.05);
	glPushMatrix();
	float part_done = (g_World.getTimeLine().getSelStart()
			- g_World.getTimeLine().getStart())
			/ (g_World.getTimeLine().getFinish()
					- g_World.getTimeLine().getStart());
	float loop_end = 1.0;
	if (part_done > 1.0) {
		part_done = 1.0;
	}
	if (part_done < 0.0) {
		part_done = 0.0;
	}
	glTranslatef(50.0 * part_done, 0.0, 0.0);
	if (time_ui_active_index == START_TIME) {
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, active);
	} else {
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, yellow);
	}
	draw_diamond();
	glPopMatrix();
	glPushMatrix();
	part_done = (g_World.getTimeLine().getSelFinish()
			- g_World.getTimeLine().getStart())
			/ (g_World.getTimeLine().getFinish()
					- g_World.getTimeLine().getStart());
	if (part_done > 1.0) {
		part_done = 1.0;
	}
	if (part_done < 0.0) {
		part_done = 0.0;
	}
	glTranslatef(50.0 * part_done, 0.0, 0.0);
	if (time_ui_active_index == END_TIME) {
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, active);
	} else {
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, yellow);
	}
	draw_diamond();
	glPopMatrix();
	glTranslatef(6.0, 1.9, 0.0);
	glScalef(4.0, 4.0, 4.0);
	draw_date();
	glScalef(0.25, 0.25, 0.25);
	glTranslatef(-46.0, -1.9, 0.0);
	part_done = (g_World.getTimeLine().getTime()
			- g_World.getTimeLine().getStart())
			/ (g_World.getTimeLine().getFinish()
					- g_World.getTimeLine().getStart());
	loop_end = (g_World.getTimeLine().getSelFinish()
			- g_World.getTimeLine().getStart())
			/ (g_World.getTimeLine().getFinish()
					- g_World.getTimeLine().getStart());
	//cout << part_done << endl;
	if (part_done >= loop_end) {
		if (g_Looping) {
			g_World.getTimeLine().setTime(g_World.getTimeLine().getSelStart());
			part_done = (g_World.getTimeLine().getFinish()
					- g_World.getTimeLine().getSelStart())
					/ (g_World.getTimeLine().getFinish()
							- g_World.getTimeLine().getStart());
		} else {
			part_done = 1.0;
		}
	}
	if (part_done < 0.0) {
		part_done = 0.0;
	}
	glTranslatef(50.0 * part_done, 0.0, 0.0);
	if (time_ui_active_index == CURRENT_TIME) {
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, active);
	} else {
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, white);
	}
	gluSphere(sphereObj, 1.6, 8, 8);
	glPopMatrix();
}

void draw_lead_time_ui() {
	float white[] = { 1.0, 1.0, 1.0, 0.5 };
	glPushMatrix();
	glTranslatef(g_TWLocation[0], g_TWLocation[1] - 0.5, g_TWLocation[2]);
	//Line for range of time
	glBegin(GL_LINES);
	glVertex3f(0.0f, 0.0f, 0.0f);
	glVertex3f(2.5f, 0.0f, 0.0f);
	glEnd();
	glScalef(0.05, 0.05, 0.05);
	float part_done = (g_World.getTimeLine().getTime()
			- g_World.getTimeLine().getStart()
			- g_World.getTimeLine().getLead())
			/ (g_World.getTimeLine().getFinish()
					- g_World.getTimeLine().getStart());
	if (part_done < 0.0) {
		part_done = 0.0;
	}
	if (part_done > 1.0) {
		part_done = 1.0;
	}
	glTranslatef(50.0 * part_done, 0.0, 0.0);
	if (time_ui_active_index == LEAD_TIME) {
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, active);
	} else {
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, white);
	}
	draw_diamond();
	glPopMatrix();
}

void draw_trail_time_ui() {
	float white[] = { 1.0, 1.0, 1.0, 0.5 };
	glPushMatrix();
	glTranslatef(g_TWLocation[0], g_TWLocation[1] - 1.0, g_TWLocation[2]);
	//Line for range of time
	glBegin(GL_LINES);
	glVertex3f(0.0f, 0.0f, 0.0f);
	glVertex3f(2.5f, 0.0f, 0.0f);
	glEnd();
	glScalef(0.05, 0.05, 0.05);
	float part_done = (g_World.getTimeLine().getTime()
			- g_World.getTimeLine().getStart()
			+ g_World.getTimeLine().getTrail())
			/ (g_World.getTimeLine().getFinish()
					- g_World.getTimeLine().getStart());
	if (part_done < 0.0) {
		part_done = 0.0;
	}
	if (part_done > 1.0) {
		part_done = 1.0;
	}
	glTranslatef(50.0 * part_done, 0.0, 0.0);
	if (time_ui_active_index == TRAIL_TIME) {
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, active);
	} else {
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, white);
	}
	draw_diamond();
	glPopMatrix();
}

void draw_speed_ui() {
	float blue[] = { 0.5, 0.5, 1.0, 0.5 };
	glPushMatrix();
	glTranslatef(g_TWLocation[0], g_TWLocation[1] - 1.5, g_TWLocation[2]);
	//Line for range of time
	glBegin(GL_LINES);
	glVertex3f(0.0f, 0.0f, 0.0f);
	glVertex3f(2.5f, 0.0f, 0.0f);
	glEnd();
	float relative_speed = (g_World.getTimeLine().getSpeed() / 1000000.0);
	glTranslatef(2.7, 0.0, 0.0);
	glScalef(0.25, 0.25, 0.25);
	if (relative_speed > 512.0) {
		draw_nine();
	} else if (relative_speed > 256.0) {
		draw_eight();
	} else if (relative_speed > 128.0) {
		draw_seven();
	} else if (relative_speed > 64.0) {
		draw_six();
	} else if (relative_speed > 8.0) {
		draw_five();
	} else if (relative_speed > 4.0) {
		draw_four();
	} else if (relative_speed > 2.0) {
		draw_three();
	} else if (relative_speed > 1.0) {
		draw_two();
	} else {
		draw_one();
	}
	glTranslatef(-10.8, 0.0, 0.0);
	glScalef(0.20, 0.20, 0.20);
	if (relative_speed > 1.0) {
		glTranslatef(50.0, 0.0, 0.0);
	} else {
		glTranslatef(50.0 * relative_speed, 0.0, 0.0);
	}
	if (time_ui_active_index == CURRENT_SPEED) {
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, active);
	} else {
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, blue);
	}
	draw_diamond();
	glPopMatrix();
}

void draw_play_controls_ui() {
	float red[] = { 1.0, 0.0, 0.0, 0.5 };
	float yellow[] = { 1.0, 1.0, 0.2, 0.5 };
	float blue[] = { 0.5, 0.5, 1.0, 0.5 };
	float green[] = { 0.0, 1.0, 0.0, 0.5 };
	glPushMatrix();
	glTranslatef(g_TWLocation[0] + 1.25, g_TWLocation[1] - 2.0, g_TWLocation[2]);
	glScalef(0.25, 0.25, 0.25);
	if (time_ui_active_index == STOP_PLAY) {
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, active);
	} else {
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, red);
	}
	draw_cube();
	if (time_ui_active_index == PLAY_FORWARD) {
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, active);
	} else {
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, green);
	}
	glRotatef(90.0, 0, 1, 0);
	glTranslatef(0.0, 0.0, 2.0);
	draw_pyramid();
	if (time_ui_active_index == PLAY_REVERSE) {
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, active);
	} else {
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, green);
	}
	glRotatef(-180.0, 0, 1, 0);
	glTranslatef(0.0, 0.0, 4.0);
	draw_pyramid();
	if (time_ui_active_index == SLOW_DOWN) {
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, active);
	} else {
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, blue);
	}
	glTranslatef(0.0, 0.0, 2.0);
	draw_pyramid();
	if (time_ui_active_index == SPEED_UP) {
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, active);
	} else {
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, blue);
	}
	glRotatef(180.0, 0, 1, 0);
	glTranslatef(0.0, 0.0, 8.0);
	draw_pyramid();
	if (time_ui_active_index == LOOP) {
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, active);
	} else {
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, yellow);
	}
	glTranslatef(1.0, 0.0, 2.0);
	glRotatef(-90.0, 0, 1, 0);
	if (g_Looping) {
		glLineWidth(4.0f);
	} else {
		glLineWidth(2.0f);
	}
	draw_L();
	if (time_ui_active_index == ANIMATE) {
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, active);
	} else {
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, yellow);
	}
	glTranslatef(1.5, -0.2, 0.0);
	if (g_Animating) {
		glLineWidth(4.0f);
	} else {
		glLineWidth(2.0f);
	}
	draw_A();
	//g_World.getTimeLine().setSlower();
	glPopMatrix();
}

void draw_diamond() {
	static GLfloat n[8][3] = { { -.894, 0.0, .447 }, { 0.0, .894, .447 }, {
			.894, 0.0, .447 }, { 0.0, -.894, .447 }, { -.894, 0.0, -.447 }, {
			0.0, .894, -.447 }, { .894, 0.0, -.447 }, { 0.0, -.894, -.447 }, };
	static GLint faces[8][4] = { { 0, 4, 1 }, { 1, 4, 3 }, { 3, 4, 2 }, { 2, 4,
			0 }, { 0, 1, 5 }, { 1, 3, 5 }, { 3, 2, 5 }, { 2, 0, 5 } };
	static GLfloat v[6][3] = { { -0.5, -4.5, 0 }, { -0.5, 4.5, 0 }, { 0.5,
			-4.5, 0 }, { 0.5, 4.5, 0 }, { 0.0, 0.0, 3.0 }, { 0.0, 0.0, -3.0 } };

	for (int i = 7; i >= 0; i--) {
		glBegin(GL_TRIANGLES);
		glNormal3fv(&n[i][0]);
		glVertex3fv(&v[faces[i][0]][0]);
		glVertex3fv(&v[faces[i][1]][0]);
		glVertex3fv(&v[faces[i][2]][0]);
		glEnd();
	}
}

void draw_cube() {
	static GLfloat n[6][3] = { { -1.0, 0.0, 0.0 }, { 0.0, 1.0, 0.0 }, { 1.0,
			0.0, 0.0 }, { 0.0, -1.0, 0.0 }, { 0.0, 0.0, 1.0 },
			{ 0.0, 0.0, -1.0 } };
	static GLint faces[6][4] = { { 0, 4, 5, 1 }, { 1, 5, 7, 3 },
			{ 3, 7, 6, 2 }, { 2, 6, 4, 0 }, { 6, 7, 5, 4 }, { 3, 2, 0, 1 } };
	static GLfloat v[8][3] = { { -0.5, -0.5, 0 }, { -0.5, 0.5, 0 }, { 0.5,
			-0.5, 0 }, { 0.5, 0.5, 0 }, { -0.5, -0.5, 1 }, { -0.5, 0.5, 1 }, {
			0.5, -0.5, 1 }, { 0.5, 0.5, 1 }, };
	for (int i = 5; i >= 0; i--) {
		glBegin(GL_QUADS);
		glNormal3fv(&n[i][0]);
		glVertex3fv(&v[faces[i][0]][0]);
		glVertex3fv(&v[faces[i][1]][0]);
		glVertex3fv(&v[faces[i][2]][0]);
		glVertex3fv(&v[faces[i][3]][0]);
		glEnd();
	}
}

void draw_pyramid() {
	static GLfloat n[5][3] = { { -.894, 0.0, .447 }, { 0.0, .894, .447 }, {
			.894, 0.0, .447 }, { 0.0, -.894, .447 }, { 0.0, 0.0, -1.0 } };
	static GLint faces[5][4] = { { 0, 4, 5, 1 }, { 1, 5, 7, 3 },
			{ 3, 7, 6, 2 }, { 2, 6, 4, 0 }, { 3, 2, 0, 1 } };
	static GLfloat v[8][3] = { { -0.5, -0.5, 0 }, { -0.5, 0.5, 0 }, { 0.5,
			-0.5, 0 }, { 0.5, 0.5, 0 }, { -0.0, -0.0, 1 }, { -0.0, 0.0, 1 }, {
			0.0, -0.0, 1 }, { 0.0, 0.0, 1 }, };

	for (int i = 4; i >= 0; i--) {
		glBegin(GL_QUADS);
		glNormal3fv(&n[i][0]);
		glVertex3fv(&v[faces[i][0]][0]);
		glVertex3fv(&v[faces[i][1]][0]);
		glVertex3fv(&v[faces[i][2]][0]);
		glVertex3fv(&v[faces[i][3]][0]);
		glEnd();
	}
}

void draw_date() {
	//g_Draw.draw3DText("TESTING", Vec3f(g_TWLocation[0], g_TWLocation[1], g_TWLocation[2]), 0, 0, true);
	GLfloat mat_emission[] = { 0.3, 0.3, 0.3, 0.0 };
	GLfloat white[] = { 1.0, 1.0, 1.0, 1.0 };
	glMaterialfv(GL_FRONT, GL_EMISSION, mat_emission);
	if (time_ui_active_index == CURRENT_TIME) {
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, active);
	} else {
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, white);
	}
	int hyphen_count = 0;
	int spaces_needed = 0;
	for (std::string::size_type i = 0; i < 10; ++i) {
		if (hyphen_count < 3) {
			switch (g_TimeString[i]) {
			case '1':
				draw_one();
				break;
			case '2':
				draw_two();
				break;
			case '3':
				draw_three();
				break;
			case '4':
				draw_four();
				break;
			case '5':
				draw_five();
				break;
			case '6':
				draw_six();
				break;
			case '7':
				draw_seven();
				break;
			case '8':
				draw_eight();
				break;
			case '9':
				draw_nine();
				break;
			case '0':
				draw_zero();
				break;
			default:
				hyphen_count++;
				if (hyphen_count < 3) {
					draw_hyphen();
				} else {
					spaces_needed = 9 - i;
				}
			}
			glTranslatef(1.0, 0.0, 0.0);
			for (int j = 0; j < spaces_needed; j++) {
				glTranslatef(1.0, 0.0, 0.0);
			}
		}
	}
	mat_emission[0] = 0.0;
	mat_emission[1] = 0.0;
	mat_emission[2] = 0.0;
	glMaterialfv(GL_FRONT, GL_EMISSION, mat_emission);
}

void draw_labels() {
	float green[] = { 0.4, 1.0, 0.4, 0.75 };
	glPushMatrix();
	glTranslatef(g_TWLocation[0]+1.3, g_TWLocation[1]-2.0, g_TWLocation[2]);
    if(time_ui_active_index == CURRENT_TIME ||
       time_ui_active_index == START_TIME ||
       time_ui_active_index == END_TIME) {
    	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, active);
    } else {
    	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, green);
    }
	glScalef(0.25, 0.25, 0.25);
    glTranslatef(-10.0, 8.0, 0.0);
    draw_S();
    glTranslatef(1.0, 0.0, 0.0);
    draw_P();
    glTranslatef(1.0, 0.0, 0.0);
    draw_A();
    glTranslatef(1.0, 0.0, 0.0);
    draw_N();
    glTranslatef(-3.0, -2.0, 0.0);
    if(time_ui_active_index == LEAD_TIME) {
    	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, active);
    } else {
    	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, green);
    }
    draw_L();
    glTranslatef(1.0, 0.0, 0.0);
    draw_E();
    glTranslatef(1.0, 0.0, 0.0);
    draw_A();
    glTranslatef(1.0, 0.0, 0.0);
    draw_D();
    glTranslatef(-3.8, -2.0, 0.0);
    if(time_ui_active_index == TRAIL_TIME) {
    	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, active);
    } else {
    	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, green);
    }
    draw_T();
    glTranslatef(1.0, 0.0, 0.0);
    draw_R();
    glTranslatef(1.0, 0.0, 0.0);
    draw_A();
    glTranslatef(1.0, 0.0, 0.0);
    draw_I();
    glTranslatef(1.0, 0.0, 0.0);
    draw_L();
    glTranslatef(-4.0, -2.0, 0.0);
    if(time_ui_active_index == CURRENT_SPEED) {
    	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, active);
    } else {
    	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, green);
    }
    draw_S();
    glTranslatef(1.0, 0.0, 0.0);
    draw_P();
    glTranslatef(1.0, 0.0, 0.0);
    draw_E();
    glTranslatef(1.0, 0.0, 0.0);
    draw_E();
    glTranslatef(1.0, 0.0, 0.0);
    draw_D();
    glPopMatrix();
}

void draw_hyphen() {
	glBegin(GL_LINES);
	glVertex3f(0.3f, 0.5f, 0.0f);
	glVertex3f(0.7f, 0.5f, 0.0f);
	glEnd();
}

void draw_one() {
	glBegin(GL_LINE_STRIP);
	glVertex3f(0.4f, 1.0f, 0.0f);
	glVertex3f(0.5f, 1.0f, 0.0f);
	glVertex3f(0.5f, 0.0f, 0.0f);
	glEnd();
}

void draw_two() {
	glBegin(GL_LINE_STRIP);
	glVertex3f(0.1f, 1.0f, 0.0f);
	glVertex3f(0.9f, 1.0f, 0.0f);
	glVertex3f(0.9f, 0.5f, 0.0f);
	glVertex3f(0.1f, 0.5f, 0.0f);
	glVertex3f(0.1f, 0.0f, 0.0f);
	glVertex3f(0.9f, 0.0f, 0.0f);
	glEnd();
}

void draw_three() {
	glBegin(GL_LINE_STRIP);
	glVertex3f(0.1f, 1.0f, 0.0f);
	glVertex3f(0.9f, 1.0f, 0.0f);
	glVertex3f(0.1f, 0.5f, 0.0f);
	glVertex3f(0.9f, 0.5f, 0.0f);
	glVertex3f(0.9f, 0.0f, 0.0f);
	glVertex3f(0.1f, 0.0f, 0.0f);
	glEnd();
}

void draw_four() {
	glBegin(GL_LINE_STRIP);
	glVertex3f(0.5f, 0.0f, 0.0f);
	glVertex3f(0.5f, 1.0f, 0.0f);
	glVertex3f(0.1f, 0.5f, 0.0f);
	glVertex3f(0.9f, 0.5f, 0.0f);
	glEnd();
}

void draw_five() {
	glBegin(GL_LINE_STRIP);
	glVertex3f(0.9f, 1.0f, 0.0f);
	glVertex3f(0.1f, 1.0f, 0.0f);
	glVertex3f(0.1f, 0.5f, 0.0f);
	glVertex3f(0.9f, 0.5f, 0.0f);
	glVertex3f(0.9f, 0.0f, 0.0f);
	glVertex3f(0.1f, 0.0f, 0.0f);
	glEnd();
}

void draw_six() {
	glBegin(GL_LINE_STRIP);
	glVertex3f(0.9f, 1.0f, 0.0f);
	glVertex3f(0.1f, 1.0f, 0.0f);
	glVertex3f(0.1f, 0.0f, 0.0f);
	glVertex3f(0.9f, 0.0f, 0.0f);
	glVertex3f(0.9f, 0.5f, 0.0f);
	glVertex3f(0.1f, 0.5f, 0.0f);
	glEnd();
}

void draw_seven() {
	glBegin(GL_LINE_STRIP);
	glVertex3f(0.2f, 1.0f, 0.0f);
	glVertex3f(0.8f, 1.0f, 0.0f);
	glVertex3f(0.4f, 0.0f, 0.0f);
	glEnd();
}

void draw_eight() {
	glBegin(GL_LINE_STRIP);
	glVertex3f(0.9f, 1.0f, 0.0f);
	glVertex3f(0.1f, 1.0f, 0.0f);
	glVertex3f(0.1f, 0.0f, 0.0f);
	glVertex3f(0.9f, 0.0f, 0.0f);
	glVertex3f(0.9f, 1.0f, 0.0f);
	glVertex3f(0.9f, 0.5f, 0.0f);
	glVertex3f(0.1f, 0.5f, 0.0f);
	glEnd();
}

void draw_nine() {
	glBegin(GL_LINE_STRIP);
	glVertex3f(0.1f, 0.0f, 0.0f);
	glVertex3f(0.9f, 0.0f, 0.0f);
	glVertex3f(0.9f, 1.0f, 0.0f);
	glVertex3f(0.1f, 1.0f, 0.0f);
	glVertex3f(0.1f, 0.5f, 0.0f);
	glVertex3f(0.9f, 0.5f, 0.0f);
	glEnd();
}

void draw_zero() {
	glBegin(GL_LINE_STRIP);
	glVertex3f(0.9f, 1.0f, 0.0f);
	glVertex3f(0.1f, 1.0f, 0.0f);
	glVertex3f(0.1f, 0.0f, 0.0f);
	glVertex3f(0.9f, 0.0f, 0.0f);
	glVertex3f(0.9f, 1.0f, 0.0f);
	glEnd();
}

void draw_L()
{
	glBegin(GL_LINE_STRIP);
		glVertex3f(0.1f,1.0f,0.0f);
		glVertex3f(0.1f,0.0f,0.0f);
		glVertex3f(0.7f,0.0f,0.0f);
	glEnd();
}

void draw_A()
{
	glBegin(GL_LINE_STRIP);
		glVertex3f(0.1f,0.0f,0.0f);
		glVertex3f(0.1f,1.0f,0.0f);
		glVertex3f(0.9f,1.0f,0.0f);
		glVertex3f(0.9f,0.5f,0.0f);
		glVertex3f(0.1f,0.5f,0.0f);
		glVertex3f(0.9f,0.5f,0.0f);
		glVertex3f(0.9f,0.0f,0.0f);
	glEnd();
}
void draw_N()
{
	glBegin(GL_LINE_STRIP);
		glVertex3f(0.2f,0.0f,0.0f);
		glVertex3f(0.2f,1.0f,0.0f);
		glVertex3f(0.8f,0.0f,0.0f);
		glVertex3f(0.8f,1.0f,0.0f);
	glEnd();
}

void draw_E()
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

void draw_P()
{
	glBegin(GL_LINE_STRIP);
		glVertex3f(0.2f,0.5f,0.0f);
		glVertex3f(0.8f,0.5f,0.0f);
		glVertex3f(0.8f,1.0f,0.0f);
		glVertex3f(0.2f,1.0f,0.0f);
		glVertex3f(0.2f,0.0f,0.0f);
	glEnd();
}

void draw_R()
{
	glBegin(GL_LINE_STRIP);
		glVertex3f(0.2f,0.5f,0.0f);
		glVertex3f(0.5f,0.5f,0.0f);
		glVertex3f(0.8f,0.0f,0.0f);
		glVertex3f(0.5f,0.5f,0.0f);
		glVertex3f(0.8f,0.5f,0.0f);
		glVertex3f(0.8f,1.0f,0.0f);
		glVertex3f(0.2f,1.0f,0.0f);
		glVertex3f(0.2f,0.0f,0.0f);
	glEnd();
}

void draw_D()
{
	glBegin(GL_LINE_STRIP);
		glVertex3f(0.2f,1.0f,0.0f);
		glVertex3f(0.6f,1.0f,0.0f);
		glVertex3f(0.8f,0.7f,0.0f);
		glVertex3f(0.8f,0.3f,0.0f);
		glVertex3f(0.6f,0.0f,0.0f);
		glVertex3f(0.2f,0.0f,0.0f);
		glVertex3f(0.2f,1.0f,0.0f);
	glEnd();
}

void draw_S()
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

void draw_T()
{
	glBegin(GL_LINE_STRIP);
		glVertex3f(0.5f,0.0f,0.0f);
		glVertex3f(0.5f,1.0f,0.0f);
		glVertex3f(0.2f,1.0f,0.0f);
		glVertex3f(0.8f,1.0f,0.0f);
	glEnd();
}

void draw_I()
{
	glBegin(GL_LINE_STRIP);
		glVertex3f(0.5f,1.0f,0.0f);
		glVertex3f(0.5f,0.0f,0.0f);
	glEnd();
}
