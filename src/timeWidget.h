/*
 * timeWidget.h
 *
 *  Created on: Sep 7, 2015
 *      Author: ribells
 */

#ifndef TIMEWIDGET_H_
#define TIMEWIDGET_H_

void tm_time_ui(int iType, float x, float y, float z, float yaw, float pitch, float roll);
void show_timeline(bool b);
int get_time_ui_active_index();
int get_next_ui_active_index();
int get_previous_ui_active_index();
int get_time_ui_current_time();
int set_time_ui_active_index(int i);
int set_time_ui_value(int i);
bool tm_get_should_animate();
bool tm_get_should_loop();
void show_layout_timeline(bool bShow);

void draw_current_time_ui();
void draw_lead_time_ui();
void draw_trail_time_ui();
void draw_speed_ui();
void draw_play_controls_ui();

void draw_cube();
void draw_pyramid();
void draw_diamond();

void draw_labels();

void draw_date();
void draw_hyphen();
void draw_one();
void draw_two();
void draw_three();
void draw_four();
void draw_five();
void draw_six();
void draw_seven();
void draw_eight();
void draw_nine();
void draw_zero();
void draw_L();
void draw_A();
void draw_S();
void draw_P();
void draw_E();
void draw_D();
void draw_T();
void draw_R();
void draw_I();
void draw_N();

#endif /* TIMEWIDGET_H_ */
