#include <lvgl.h>
#include "app_mouse.h"
#include "global_flags.h"
#include <LV_Helper.h>
#include <Arduino.h>
#include "BleCompositeHID.h"

// Use shared composite HID instance from main file
extern BleCompositeHID bleHID;
// Create a new screen
static lv_obj_t *screen;
static lv_obj_t *label1;
lv_indev_t *indev_touchpad;
lv_indev_t *indev;
lv_timer_t *timer;
// Create the icons


static struct
{
  lv_obj_t *icon_up;
  lv_obj_t *icon_up_right;
  lv_obj_t *icon_up_left;
  lv_obj_t *icon_down;
  lv_obj_t *icon_down_right;
  lv_obj_t *icon_down_left;
  lv_obj_t *icon_right;
  lv_obj_t *icon_left;
  lv_obj_t *icon_right_click;
  lv_obj_t *icon_left_click;
} icon_param;

static lv_obj_t *create_mouse_btn(lv_obj_t *parent, lv_align_t align, lv_coord_t x_ofs, lv_coord_t y_ofs, const char *symbol);
static void left_event_cb(lv_event_t *event);
static void right_event_cb(lv_event_t *event);
static void down_event_cb(lv_event_t *event);

static void up_event_cb(lv_event_t *event);
static void up_right_event_cb(lv_event_t *event);
static void up_left_event_cb(lv_event_t *event);
static void down_right_event_cb(lv_event_t *event);
static void down_left_event_cb(lv_event_t *event);
static void right_click_event_cb(lv_event_t *event);
static void left_click_event_cb(lv_event_t *event);
static void update_label_task(lv_timer_t *timer);

void app_mouse_load(lv_obj_t *cont)
{
  // BLE Status indicator
  label1 = lv_label_create(cont);
  lv_label_set_text(label1, bleHID.isConnected() ? LV_SYMBOL_BLUETOOTH " Connected" : "#808080 " LV_SYMBOL_BLUETOOTH " Disconnected#");
  lv_label_set_recolor(label1, true);
  lv_obj_align(label1, LV_ALIGN_TOP_MID, 0, 5);
  
  icon_param.icon_left = create_mouse_btn(cont, LV_ALIGN_BOTTOM_MID, -20, -10, LV_SYMBOL_OK);
  lv_obj_add_event_cb(icon_param.icon_left, left_event_cb, LV_EVENT_CLICKED, NULL);

  icon_param.icon_right = create_mouse_btn(cont, LV_ALIGN_OUT_RIGHT_MID, 40, 0, LV_SYMBOL_LIST);
  lv_obj_add_event_cb(icon_param.icon_right, right_event_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_align_to(icon_param.icon_right, icon_param.icon_left, LV_ALIGN_OUT_RIGHT_MID, 40, 0);
  /*
  icon_param.icon_left = create_mouse_btn(cont, LV_ALIGN_LEFT_MID, 10, 0, LV_SYMBOL_LEFT);
  icon_param.icon_up_left = create_mouse_btn(cont, LV_ALIGN_TOP_LEFT, 20, 25, LV_SYMBOL_LEFT);
  icon_param.icon_up_right = create_mouse_btn(cont, LV_ALIGN_TOP_RIGHT, -10, 25, LV_SYMBOL_RIGHT);
  icon_param.icon_down_left = create_mouse_btn(cont, LV_ALIGN_BOTTOM_LEFT, 10, -10, LV_SYMBOL_LEFT);
  icon_param.icon_down_right = create_mouse_btn(cont, LV_ALIGN_BOTTOM_RIGHT, -10, -10, LV_SYMBOL_RIGHT);
  icon_param.icon_right_click = create_mouse_btn(cont, LV_ALIGN_CENTER, -15, 0, LV_SYMBOL_LIST);
  icon_param.icon_left_click = create_mouse_btn(cont, LV_ALIGN_CENTER, 15, 0, LV_SYMBOL_OK);

  // Add event callbacks



  lv_obj_add_event_cb(icon_param.icon_left, left_event_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_add_event_cb(icon_param.icon_right_click, right_click_event_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_add_event_cb(icon_param.icon_left_click, left_click_event_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_add_event_cb(icon_param.icon_up_left, up_left_event_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_add_event_cb(icon_param.icon_up_right, up_right_event_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_add_event_cb(icon_param.icon_down_left, down_left_event_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_add_event_cb(icon_param.icon_down_right, down_right_event_cb, LV_EVENT_CLICKED, NULL);

  */

  label1 = lv_label_create(cont);
  lv_label_set_long_mode(label1, LV_LABEL_LONG_WRAP);
  lv_label_set_recolor(label1, true);
  lv_label_set_text(label1, "0000");
  lv_obj_set_width(label1, 150);
  lv_obj_set_style_text_align(label1, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(label1, LV_ALIGN_CENTER, 0, 0);

  indev = lv_indev_get_next(NULL);

  timer = lv_timer_create(update_label_task, 100, NULL);
  //bleMouse.begin();

  lv_task_handler();
}
static void update_label_task(lv_timer_t *timer)
{
  static lv_point_t point2 = {0, 0};  // Fixed: make static to persist across calls
  lv_point_t point = {0, 0};
  
  // Get the first input device (touchpad) - cache it
  if (indev == NULL) {
    indev = lv_indev_get_next(NULL);
    if (indev == NULL) return;  // No input device available
  }
  
  // Check if input device is pressed
  if (indev->proc.state == LV_INDEV_STATE_PRESSED)
  {
    /* Get the point of the last pressed position */
    lv_indev_get_point(indev, &point);
    lv_label_set_text_fmt(label1, "X:%d Y:%d",
                          point.x, point.y);
    if (bleHID.isConnected() && point.y < 200)
    { 
      // Calculate relative movement
      int16_t delta_x = point.x - point2.x;
      int16_t delta_y = point.y - point2.y;
      
      // Only move if there's actual movement
      if (delta_x != 0 || delta_y != 0) {
        bleHID.mouseMove(delta_x, delta_y);
        point2.x = point.x;
        point2.y = point.y;
      }
    }
  } else {
    // Reset last point when touch is released to avoid drift
    point2.x = 0;
    point2.y = 0;
  }
}

static void left_event_cb(lv_event_t *event)
{
  if (bleHID.isConnected())
  {
    bleHID.mouseClick(MOUSE_LEFT);
  }
}
static void right_event_cb(lv_event_t *event)
{
  if (bleHID.isConnected())
  {
    bleHID.mouseClick(MOUSE_RIGHT);
  }
}

// Return to the main menu
void return_to_main_menu(lv_obj_t *cont)
{
  // Destroy the screen and all its children
  lv_obj_clean(cont);
  lv_timer_del(timer);
  // Call the main menu function or perform any other necessary actions
}
static lv_obj_t *create_mouse_btn(lv_obj_t *parent, lv_align_t align, lv_coord_t x_ofs, lv_coord_t y_ofs, const char *symbol)
{
  lv_obj_t *btn = lv_btn_create(parent);
  lv_obj_set_size(btn, 60, 60);
  lv_obj_set_style_radius(btn, 180, 0);
  lv_obj_align(btn, align, x_ofs, y_ofs);
  lv_obj_set_style_outline_color(btn, lv_color_white(), LV_STATE_FOCUS_KEY);

  lv_obj_t *label = lv_label_create(btn);
  lv_obj_center(label);
  lv_label_set_text(label, symbol);

  return btn;
}

app_t app_mouse = {
    .setup_func_cb = app_mouse_load,
    .exit_func_cb = (app_func_t)return_to_main_menu,
    .user_data = nullptr,
};
