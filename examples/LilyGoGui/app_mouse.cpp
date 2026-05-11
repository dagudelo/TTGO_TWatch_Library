#include <lvgl.h>
#include "app_mouse.h"
#include <Arduino.h>
#include "ui.h"

// Create a new screen
static lv_obj_t *screen;
static lv_obj_t *label1;
static lv_obj_t *label_coords;

lv_indev_t *indev;
lv_timer_t *timer;
static lv_point_t last_point;
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
static void update_label_task(lv_timer_t *timer);
static lv_timer_t *mouse_timer;

static void update_mouse_label_task(lv_timer_t *timer)
{
  if (bleComposite.isConnected())
  {
    lv_label_set_text(label1, "Trackpad Ready");
  }
  else
  {
    lv_label_set_text(label1, "#ff0000 Not Connected#");
  }
}

void app_mouse_load(lv_obj_t *cont)
{
  icon_param.icon_left = create_mouse_btn(cont, LV_ALIGN_BOTTOM_MID, -20, -10, LV_SYMBOL_OK);
  lv_obj_add_event_cb(icon_param.icon_left, left_event_cb, LV_EVENT_CLICKED, NULL);

  icon_param.icon_right = create_mouse_btn(cont, LV_ALIGN_OUT_RIGHT_MID, 40, 0, LV_SYMBOL_LIST);
  lv_obj_add_event_cb(icon_param.icon_right, right_event_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_align_to(icon_param.icon_right, icon_param.icon_left, LV_ALIGN_OUT_RIGHT_MID, 40, 0);

  label1 = lv_label_create(cont);
  lv_label_set_long_mode(label1, LV_LABEL_LONG_WRAP);
  lv_label_set_recolor(label1, true);
  if (bleComposite.isConnected()) {
      lv_label_set_text(label1, "Trackpad Ready");
  } else {
      lv_label_set_text(label1, "#ff0000 Not Connected#");
  }
  lv_obj_set_width(label1, 150);
  lv_obj_set_style_text_align(label1, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(label1, LV_ALIGN_TOP_MID, 0, 10);

  label_coords = lv_label_create(cont);
  lv_label_set_text(label_coords, "");
  lv_obj_set_style_text_align(label_coords, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(label_coords, LV_ALIGN_CENTER, 0, 0);

  indev = lv_indev_get_next(NULL);

  timer = lv_timer_create(update_label_task, 100, NULL);
  mouse_timer = lv_timer_create(update_mouse_label_task, 500, NULL);
  last_point.x = -1; // Initialize last point to an invalid state
  last_point.y = -1;
}
static void update_label_task(lv_timer_t *timer)
{
  lv_point_t point;
  lv_indev_get_point(indev, &point);
  
  if (indev->proc.state == LV_INDEV_STATE_PRESSED)
  {
    if (bleComposite.isConnected() && point.y < 200) // Use y<200 as active trackpad area
    {
      lv_label_set_text_fmt(label_coords, "X:%d Y:%d", point.x, point.y);
      if (last_point.x != -1) { // If we have a valid last point
        int8_t dx = point.x - last_point.x;
        int8_t dy = point.y - last_point.y;
        if (dx != 0 || dy != 0) {
          mouse->mouseMove(dx, dy);
        }
      }
      last_point = point; // Update last point
    }
  } else {
    last_point.x = -1; // Invalidate last point when touch is released
    last_point.y = -1;
    lv_label_set_text(label_coords, ""); // Clear coordinate label
  }
}

static void left_event_cb(lv_event_t *event)
{
  if (bleComposite.isConnected())
  {
    mouse->mouseClick(MOUSE_LOGICAL_LEFT_BUTTON);
    lv_label_set_text(label_coords, "Left Click");
  }
}
static void right_event_cb(lv_event_t *event)
{
  if (bleComposite.isConnected())
  {
    mouse->mouseClick(MOUSE_LOGICAL_RIGHT_BUTTON);
    lv_label_set_text(label_coords, "Right Click");
  }
}

// Return to the main menu
void return_to_main_menu(lv_obj_t *cont)
{
  // Destroy the screen and all its children
  lv_obj_clean(cont);
  lv_timer_del(mouse_timer);
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
