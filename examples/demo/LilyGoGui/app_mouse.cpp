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
static lv_obj_t *ble_status_label;  // BLE connection status
static lv_obj_t *center_label_static;  // Center action/gesture label
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
  lv_obj_t *btn_back;
  lv_obj_t *btn_home;
  lv_obj_t *btn_recent;
} icon_param;

// Touch gesture detection
static uint32_t press_start_time = 0;
static uint32_t last_click_time = 0;
static bool long_press_handled = false;
static bool is_dragging = false;
static bool double_click_waiting = false;
static lv_point_t press_start_point = {0, 0};
static lv_point_t last_click_point = {0, 0};
static int click_count = 0;

// Gesture thresholds
#define LONG_PRESS_TIME 500      // 500ms for long press
#define DOUBLE_CLICK_TIME 300    // 300ms window for double-click
#define DRAG_THRESHOLD 15        // 15px movement to start drag
#define CLICK_TOLERANCE 10       // 10px max movement for click
#define SCROLL_SENSITIVITY 3     // Scroll wheel sensitivity
#define TOUCH_AREA_HEIGHT 180    // Upper area for touch interaction

// Touch modes
typedef enum {
  TOUCH_MODE_CURSOR,      // Move cursor only
  TOUCH_MODE_DIRECT,      // Direct touch like touchscreen
  TOUCH_MODE_SCROLL       // Scroll mode
} touch_mode_t;

static touch_mode_t current_mode = TOUCH_MODE_DIRECT;
static lv_obj_t *mode_label;

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
static void back_event_cb(lv_event_t *event);
static void home_event_cb(lv_event_t *event);
static void recent_event_cb(lv_event_t *event);
static void mode_switch_event_cb(lv_event_t *event);
static void update_label_task(lv_timer_t *timer);
static void handle_swipe_gesture(lv_point_t start, lv_point_t end);
static void handle_pinch_zoom(int direction);

void app_mouse_load(lv_obj_t *cont)
{
  // Check if BLE is enabled
  extern bool bleEnabled;
  if (!bleEnabled) {
    lv_obj_t *warning_label = lv_label_create(cont);
    lv_label_set_text(warning_label, "BLE is disabled\nEnable it from\nmain screen");
    lv_obj_set_style_text_align(warning_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(warning_label);
    return;
  }
  
  // Mode switch button (top-left, below return button to avoid overlap)
  lv_obj_t *mode_btn = lv_btn_create(cont);
  lv_obj_set_size(mode_btn, 35, 35);
  lv_obj_align(mode_btn, LV_ALIGN_TOP_LEFT, 5, 48);  // Positioned below return button (40px button + 8px gap)
  lv_obj_add_event_cb(mode_btn, mode_switch_event_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *mode_btn_label = lv_label_create(mode_btn);
  lv_label_set_text(mode_btn_label, LV_SYMBOL_REFRESH);
  lv_obj_center(mode_btn_label);
  lv_obj_set_style_text_font(mode_btn_label, &lv_font_montserrat_12, 0);
  
  // Mode display label (top-right, smaller)
  mode_label = lv_label_create(cont);
  lv_label_set_text(mode_label, "Direct");
  lv_obj_align(mode_label, LV_ALIGN_TOP_RIGHT, -5, 8);
  lv_obj_set_style_text_font(mode_label, &lv_font_montserrat_10, 0);
  
  // Combined status label in center - shows both BLE status and current action
  center_label_static = lv_label_create(cont);
  lv_label_set_long_mode(center_label_static, LV_LABEL_LONG_WRAP);
  lv_label_set_recolor(center_label_static, true);
  lv_obj_set_width(center_label_static, 180);
  lv_obj_set_style_text_align(center_label_static, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(center_label_static, LV_ALIGN_CENTER, 0, 0);
  
  // Set initial combined status
  if (bleHID.isConnected()) {
    lv_label_set_text(center_label_static, "#0000FF " LV_SYMBOL_BLUETOOTH " Connected#\nTouch Area");
  } else {
    lv_label_set_text(center_label_static, "#808080 " LV_SYMBOL_BLUETOOTH " Disconnected#\nTouch Area");
  }
  
  // Keep reference for BLE updates (reuse center label)
  ble_status_label = center_label_static;
  
  // Android navigation buttons at the bottom
  icon_param.btn_back = create_mouse_btn(cont, LV_ALIGN_BOTTOM_LEFT, 10, -10, LV_SYMBOL_LEFT);
  lv_obj_add_event_cb(icon_param.btn_back, back_event_cb, LV_EVENT_CLICKED, NULL);
  
  icon_param.btn_home = create_mouse_btn(cont, LV_ALIGN_BOTTOM_MID, 0, -10, LV_SYMBOL_HOME);
  lv_obj_add_event_cb(icon_param.btn_home, home_event_cb, LV_EVENT_CLICKED, NULL);
  
  icon_param.btn_recent = create_mouse_btn(cont, LV_ALIGN_BOTTOM_RIGHT, -10, -10, LV_SYMBOL_LIST);
  lv_obj_add_event_cb(icon_param.btn_recent, recent_event_cb, LV_EVENT_CLICKED, NULL);
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

  indev = lv_indev_get_next(NULL);

  timer = lv_timer_create(update_label_task, 100, NULL);
  //bleMouse.begin();

  lv_task_handler();
}
static void update_label_task(lv_timer_t *timer)
{
  static lv_point_t point2 = {0, 0};
  static uint32_t last_status_update = 0;
  lv_point_t point = {0, 0};
  
  // Update BLE status every second (only if not showing other action)
  uint32_t now = lv_tick_get();
  if (now - last_status_update > 1000 && indev->proc.state != LV_INDEV_STATE_PRESSED) {
    if (center_label_static) {
      if (bleHID.isConnected()) {
        lv_label_set_text(center_label_static, "#0000FF " LV_SYMBOL_BLUETOOTH " Connected#\nTouch Area");
      } else {
        lv_label_set_text(center_label_static, "#808080 " LV_SYMBOL_BLUETOOTH " Disconnected#\nTouch Area");
      }
    }
    last_status_update = now;
  }
  
  // Get the first input device (touchpad)
  if (indev == NULL) {
    indev = lv_indev_get_next(NULL);
    if (indev == NULL) return;
  }
  
  // Check if input device is pressed
  if (indev->proc.state == LV_INDEV_STATE_PRESSED)
  {
    lv_indev_get_point(indev, &point);
    
    // Detect initial press
    if (point2.x == 0 && point2.y == 0) {
      press_start_time = now;
      press_start_point = point;
      long_press_handled = false;
      is_dragging = false;
      point2 = point;
    }
    
    if (bleHID.isConnected() && point.y < TOUCH_AREA_HEIGHT)
    {
      int16_t dx = point.x - press_start_point.x;
      int16_t dy = point.y - press_start_point.y;
      int16_t move_dist = sqrt(dx*dx + dy*dy);
      
      // Check if we've started dragging
      if (!is_dragging && move_dist > DRAG_THRESHOLD) {
        is_dragging = true;
      }
      
      // Handle different touch modes
      if (current_mode == TOUCH_MODE_DIRECT) {
        // Direct touch mode - acts like touchscreen
        if (!long_press_handled && (now - press_start_time) >= LONG_PRESS_TIME && move_dist < CLICK_TOLERANCE) {
          // Long press = right click
          bleHID.mouseClick(MOUSE_RIGHT);
          long_press_handled = true;
          lv_label_set_text(center_label_static, "Right Click");
        } else if (is_dragging) {
          // Dragging - move cursor
          int16_t delta_x = point.x - point2.x;
          int16_t delta_y = point.y - point2.y;
          if (delta_x != 0 || delta_y != 0) {
            bleHID.mouseMove(delta_x, delta_y);
            point2 = point;
            lv_label_set_text_fmt(center_label_static, "Drag: %d,%d", dx, dy);
          }
        } else {
          lv_label_set_text_fmt(center_label_static, "Touch: %d,%d", point.x, point.y);
        }
        
      } else if (current_mode == TOUCH_MODE_CURSOR) {
        // Cursor mode - always moves cursor
        int16_t delta_x = point.x - point2.x;
        int16_t delta_y = point.y - point2.y;
        if (delta_x != 0 || delta_y != 0) {
          bleHID.mouseMove(delta_x, delta_y);
          point2 = point;
          lv_label_set_text_fmt(center_label_static, "Cursor: %d,%d", point.x, point.y);
        }
        
      } else if (current_mode == TOUCH_MODE_SCROLL) {
        // Scroll mode - vertical movement scrolls
        int16_t delta_y = point.y - point2.y;
        if (abs(delta_y) > 5) {
          int8_t scroll = (delta_y > 0) ? -1 : 1;
          bleHID.mouseMove(0, 0, scroll * SCROLL_SENSITIVITY);
          point2 = point;
          lv_label_set_text_fmt(center_label_static, "Scroll: %d", delta_y);
        }
      }
    }
  } else {
    // Touch released
    if (bleHID.isConnected() && press_start_time > 0 && press_start_point.y < TOUCH_AREA_HEIGHT) {
      uint32_t press_duration = now - press_start_time;
      int16_t dx = abs(press_start_point.x - point2.x);
      int16_t dy = abs(press_start_point.y - point2.y);
      int16_t move_dist = sqrt(dx*dx + dy*dy);
      
      // Handle tap gestures
      if (!long_press_handled && !is_dragging && move_dist < CLICK_TOLERANCE) {
        if (press_duration < LONG_PRESS_TIME) {
          // Check for double-click
          if (double_click_waiting && (now - last_click_time) < DOUBLE_CLICK_TIME) {
            // Double-click detected
            bleHID.mouseClick(MOUSE_LEFT);
            delay(50);
            bleHID.mouseClick(MOUSE_LEFT);
            lv_label_set_text(center_label_static, "Double-Click");
            double_click_waiting = false;
            click_count = 0;
          } else {
            // Single click
            bleHID.mouseClick(MOUSE_LEFT);
            lv_label_set_text(center_label_static, "Click");
            double_click_waiting = true;
            last_click_time = now;
            last_click_point = press_start_point;
          }
        }
      } else if (is_dragging && current_mode == TOUCH_MODE_DIRECT) {
        // End of drag - release mouse button if held
        lv_label_set_text(center_label_static, "Drag End");
      } else if (move_dist > 100) {
        // Swipe gesture detected
        handle_swipe_gesture(press_start_point, point2);
      }
    }
    
    // Reset state
    point2.x = 0;
    point2.y = 0;
    press_start_time = 0;
    long_press_handled = false;
    is_dragging = false;
    
    // Clear double-click waiting after timeout
    if (double_click_waiting && (now - last_click_time) > DOUBLE_CLICK_TIME) {
      double_click_waiting = false;
    }
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

// Android navigation button callbacks
static void back_event_cb(lv_event_t *event)
{
  if (bleHID.isConnected())
  {
    bleHID.pressKey(KEY_ESC);
    delay(50);
    bleHID.releaseKey(KEY_ESC);
    lv_label_set_text(center_label_static, "Back");
  }
}

static void home_event_cb(lv_event_t *event)
{
  if (bleHID.isConnected())
  {
    bleHID.pressKey(KEY_LEFT_GUI);
    delay(50);
    bleHID.releaseKey(KEY_LEFT_GUI);
    lv_label_set_text(center_label_static, "Home");
  }
}

static void recent_event_cb(lv_event_t *event)
{
  if (bleHID.isConnected())
  {
    bleHID.pressKey(KEY_LEFT_ALT);
    bleHID.pressKey(KEY_TAB);
    delay(50);
    bleHID.releaseKey(KEY_TAB);
    bleHID.releaseKey(KEY_LEFT_ALT);
    lv_label_set_text(center_label_static, "Recent Apps");
  }
}

// Mode switch callback
static void mode_switch_event_cb(lv_event_t *event)
{
  current_mode = (touch_mode_t)((current_mode + 1) % 3);
  
  switch(current_mode) {
    case TOUCH_MODE_DIRECT:
      lv_label_set_text(mode_label, "Direct");
      lv_label_set_text(center_label_static, "Mode: Direct");
      break;
    case TOUCH_MODE_CURSOR:
      lv_label_set_text(mode_label, "Cursor");
      lv_label_set_text(center_label_static, "Mode: Cursor");
      break;
    case TOUCH_MODE_SCROLL:
      lv_label_set_text(mode_label, "Scroll");
      lv_label_set_text(center_label_static, "Mode: Scroll");
      break;
  }
}

// Handle swipe gestures
static void handle_swipe_gesture(lv_point_t start, lv_point_t end)
{
  if (!bleHID.isConnected()) return;
  
  int16_t dx = end.x - start.x;
  int16_t dy = end.y - start.y;
  
  // Determine swipe direction
  if (abs(dx) > abs(dy)) {
    // Horizontal swipe
    if (dx > 0) {
      // Swipe right - Forward (browser)
      bleHID.pressKey(KEY_LEFT_ALT);
      bleHID.pressKey(KEY_RIGHT_ARROW);
      delay(50);
      bleHID.releaseKey(KEY_RIGHT_ARROW);
      bleHID.releaseKey(KEY_LEFT_ALT);
      lv_label_set_text(center_label_static, "Swipe Right");
    } else {
      // Swipe left - Back (browser)
      bleHID.pressKey(KEY_LEFT_ALT);
      bleHID.pressKey(KEY_LEFT_ARROW);
      delay(50);
      bleHID.releaseKey(KEY_LEFT_ARROW);
      bleHID.releaseKey(KEY_LEFT_ALT);
      lv_label_set_text(center_label_static, "Swipe Left");
    }
  } else {
    // Vertical swipe
    if (dy > 0) {
      // Swipe down - Page Down
      bleHID.pressKey(KEY_PAGE_DOWN);
      delay(50);
      bleHID.releaseKey(KEY_PAGE_DOWN);
      lv_label_set_text(center_label_static, "Swipe Down");
    } else {
      // Swipe up - Page Up
      bleHID.pressKey(KEY_PAGE_UP);
      delay(50);
      bleHID.releaseKey(KEY_PAGE_UP);
      lv_label_set_text(center_label_static, "Swipe Up");
    }
  }
}

// Handle pinch zoom (placeholder for future multi-touch)
static void handle_pinch_zoom(int direction)
{
  if (!bleHID.isConnected()) return;
  
  // Ctrl + Plus/Minus for zoom
  bleHID.pressKey(KEY_LEFT_CTRL);
  if (direction > 0) {
    bleHID.pressKey('=');  // Zoom in
  } else {
    bleHID.pressKey('-');  // Zoom out
  }
  delay(50);
  bleHID.releaseAllKeys();
  lv_label_set_text(center_label_static, direction > 0 ? "Zoom In" : "Zoom Out");
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
