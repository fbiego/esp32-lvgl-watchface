
// File generated by bin2lvgl
// developed by fbiego. 
// https://github.com/fbiego
// Watchface: RADAR

#ifndef _FACE_RADAR_H
#define _FACE_RADAR_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "lvgl.h"

#define ENABLE_FACE_RADAR // (Radar) uncomment to enable or define it elsewhere

#ifdef ENABLE_FACE_RADAR
    extern lv_obj_t *face_radar;
	extern lv_obj_t *face_radar_0_984;
	extern lv_obj_t *face_radar_1_58768;
	extern lv_obj_t *face_radar_17_119773;
	extern lv_obj_t *face_radar_33_212563;


	LV_IMG_DECLARE(face_radar_dial_img_0_984_0);
	LV_IMG_DECLARE(face_radar_dial_img_1_58768_0);
	LV_IMG_DECLARE(face_radar_dial_img_17_119773_0);
	LV_IMG_DECLARE(face_radar_dial_img_33_212563_0);
	LV_IMG_DECLARE(face_radar_dial_img_preview_0);


#endif
    void onFaceEvent(lv_event_t * e);

    void init_face_radar(void (*callback)(const char*, const lv_img_dsc_t *, lv_obj_t **));
    void update_time_radar(int second, int minute, int hour, bool mode, bool am, int day, int month, int year, int weekday);
    void update_weather_radar(int temp, int icon);
    void update_status_radar(int battery, bool connection);
    void update_activity_radar(int steps, int distance, int kcal);
    void update_health_radar(int bpm, int oxygen);
    void update_all_radar(int second, int minute, int hour, bool mode, bool am, int day, int month, int year, int weekday, 
                int temp, int icon, int battery, bool connection, int steps, int distance, int kcal, int bpm, int oxygen);
    void update_check_radar(lv_obj_t *root, int second, int minute, int hour, bool mode, bool am, int day, int month, int year, int weekday, 
                int temp, int icon, int battery, bool connection, int steps, int distance, int kcal, int bpm, int oxygen);


#ifdef __cplusplus
}
#endif

#endif
