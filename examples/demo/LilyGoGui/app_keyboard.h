#pragma once

#include "app_typedef.h"
#include "lvgl.h"
#include "global_flags.h"
#include <LV_Helper.h>  
#include <Arduino.h>



extern app_t app_keyboard;

void app_keyboard_load(lv_obj_t *cont);
void app_keyboard_exit(lv_obj_t *cont);

