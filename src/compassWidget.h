/*
 * compassWidget.h
 *
 *  Created on: Oct 5, 2015
 *      Author: ribells
 */

#ifndef COMPASSWIDGET_H_
#define COMPASSWIDGET_H_

void tm_compass_ui(int iType, float x, float y, float z, float yaw, float pitch, float roll);
int get_compass_ui_current_direction();
void set_compass_ui_current_direction(double direction, double animationAngle);
int set_compass_ui_active_index(int i);
void show_layout_compass(bool bShow);

void draw_compass_ui();
void draw_circle(float cx, float cy, float r, int ns);
void draw_compass_needle();

void draw_n();
void draw_e();
void draw_s();
void draw_w();

#endif /* COMPASSWIDGET_H_ */
