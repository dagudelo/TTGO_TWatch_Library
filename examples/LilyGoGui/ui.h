#pragma once
#include "app_configuration.h"
#include "app_controller.h"
#include "app_fft.h"
#include "app_music.h"
#include "app_wireless.h"
#include "lvgl.h"
#include "BleCompositeHID.h"
#include "KeyboardDevice.h"
#include "MouseDevice.h"

extern BleCompositeHID bleComposite;
extern KeyboardDevice* keyboard;
extern MouseDevice* mouse;

#define LV_DELAY(x)                                                                                                                                  \
  do {                                                                                                                                               \
    uint32_t t = x;                                                                                                                                  \
    while (t--) {                                                                                                                                    \
      lv_timer_handler();                                                                                                                            \
      delay(1);                                                                                                                                      \
    }                                                                                                                                                \
  } while (0);

void ui_init();
void common_back_button_event_handler(lv_event_t *e);
void ui_boot_anim();

/**
 * @brief A brief
 *  string of information appears at the top of the page.
 * 
 * @param str String to display
 * @param delay Display time
 */
void ui_send_msg(char *str, uint32_t delay);