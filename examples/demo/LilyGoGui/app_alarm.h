#pragma once

#include "app_typedef.h"
#include "lvgl.h"

extern app_t app_alarm;

void app_alarm_load(lv_obj_t *cont);
void check_alarm();