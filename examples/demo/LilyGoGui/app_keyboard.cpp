#include <lvgl.h>
#include "app_keyboard.h"
#include "global_flags.h"
#include <LV_Helper.h>
#include <Arduino.h>
#include "BleCompositeHID.h"

// Use shared composite HID instance from main file
extern BleCompositeHID bleHID;
static lv_obj_t *label1;

// Create the icon
static struct
{
  lv_obj_t *play;
  lv_obj_t *pause;
  lv_obj_t *stop;
  lv_obj_t *progress_bar;
  lv_obj_t *time_label;
  lv_obj_t *forward;
  lv_obj_t *backward;
  lv_obj_t *volume_up;
  lv_obj_t *volume_down;
  lv_obj_t *media;
} keyboard_param;

static lv_obj_t *create_keyboard_btn(lv_obj_t *parent, lv_align_t align, lv_coord_t x_ofs, lv_coord_t y_ofs, const char *symbol, lv_palette_t color);
static void play_event_cb(lv_event_t *e);
// static void drag_music_time_event_cb(lv_event_t *e);
static void pause_event_cb(lv_event_t *e);
static void stop_event_cb(lv_event_t *e);
static void forward_event_cb(lv_event_t *e);
static void backward_event_cb(lv_event_t *e);
static void button_event_cb(lv_event_t *e);

void app_keyboard_load(lv_obj_t *cont)
{
  // BLE Status indicator at top
  lv_obj_t *ble_status = lv_label_create(cont);
  lv_label_set_text(ble_status, bleHID.isConnected() ? LV_SYMBOL_BLUETOOTH " Connected" : "#808080 " LV_SYMBOL_BLUETOOTH " Disconnected#");
  lv_label_set_recolor(ble_status, true);
  lv_obj_align(ble_status, LV_ALIGN_TOP_MID, 0, 5);

  // generate the ui
  keyboard_param.play = create_keyboard_btn(cont, LV_ALIGN_CENTER, -65, -45, LV_SYMBOL_PLAY, LV_PALETTE_GREEN);
  lv_obj_add_event_cb(keyboard_param.play, button_event_cb, LV_EVENT_CLICKED, NULL);

  keyboard_param.pause = create_keyboard_btn(cont, LV_ALIGN_CENTER, 0, -50, LV_SYMBOL_PAUSE, LV_PALETTE_YELLOW);
  lv_obj_add_event_cb(keyboard_param.pause, button_event_cb, LV_EVENT_CLICKED, NULL);

  keyboard_param.stop = create_keyboard_btn(cont, LV_ALIGN_CENTER, 65, -45, LV_SYMBOL_STOP, LV_PALETTE_PINK);
  lv_obj_add_event_cb(keyboard_param.stop, button_event_cb, LV_EVENT_CLICKED, NULL);

  keyboard_param.backward = create_keyboard_btn(cont, LV_ALIGN_CENTER, -35, 15, LV_SYMBOL_LEFT, LV_PALETTE_BLUE);
  lv_obj_add_event_cb(keyboard_param.backward, button_event_cb, LV_EVENT_CLICKED, NULL);

  keyboard_param.forward = create_keyboard_btn(cont, LV_ALIGN_CENTER, 35, 15, LV_SYMBOL_RIGHT, LV_PALETTE_RED);
  lv_obj_add_event_cb(keyboard_param.forward, button_event_cb, LV_EVENT_CLICKED, NULL);

  keyboard_param.volume_up = create_keyboard_btn(cont, LV_ALIGN_CENTER, 65, 70, LV_SYMBOL_PLUS, LV_PALETTE_GREY);
  lv_obj_add_event_cb(keyboard_param.volume_up, button_event_cb, LV_EVENT_CLICKED, NULL);

  keyboard_param.volume_down = create_keyboard_btn(cont, LV_ALIGN_CENTER, -65, 70, LV_SYMBOL_MINUS, LV_PALETTE_GREY);
  lv_obj_add_event_cb(keyboard_param.volume_down, button_event_cb, LV_EVENT_CLICKED, NULL);

  keyboard_param.media = create_keyboard_btn(cont, LV_ALIGN_CENTER, 0, 85, LV_SYMBOL_AUDIO, LV_PALETTE_GREY);
  lv_obj_add_event_cb(keyboard_param.media, button_event_cb, LV_EVENT_CLICKED, NULL);

  label1 = lv_label_create(cont);
  lv_label_set_long_mode(label1, LV_LABEL_LONG_WRAP);
  lv_label_set_recolor(label1, true);
  lv_label_set_text(label1, "Media Controls");
  lv_obj_set_width(label1, 150);
  lv_obj_set_style_text_align(label1, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(label1, LV_ALIGN_TOP_MID, 0, 10);                         // Align the label to the top of the container
  lv_obj_set_style_bg_color(label1, lv_color_make(0x80, 0x80, 0x80), 0); // Set the background color to gray
  lv_obj_set_style_shadow_color(label1, lv_color_black(), 0);            // Set the shadow color to black
  lv_obj_set_style_shadow_width(label1, 5, 0);                           // Set the shadow width
  lv_task_handler();
}
static void update_label_task(lv_timer_t *timer)
{
}
static void button_event_cb(lv_event_t *e)
{
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *btn = lv_event_get_target(e);

  if (code == LV_EVENT_CLICKED && bleHID.isConnected())
  {
    if (btn == keyboard_param.play)
    {
      bleHID.writeMediaKey(KEY_MEDIA_PLAY_PAUSE);
      lv_label_set_text_fmt(label1, "Play/Pause");
      //bleKeyboard.getArtistAndTrackName();
      // Obtain a reference to the service we are after
    }
    else if (btn == keyboard_param.pause)
    {
      bleHID.writeMediaKey(KEY_MEDIA_PLAY_PAUSE);
      lv_label_set_text_fmt(label1, "Play/Pause");
    }
    else if (btn == keyboard_param.stop)
    {
      bleHID.writeMediaKey(KEY_MEDIA_STOP);
      lv_label_set_text_fmt(label1, "Stop");
    }
    else if (btn == keyboard_param.forward)
    {
      bleHID.writeMediaKey(KEY_MEDIA_NEXT_TRACK);
      lv_label_set_text_fmt(label1, "Next Track");
    }
    else if (btn == keyboard_param.backward)
    {
      bleHID.writeMediaKey(KEY_MEDIA_PREVIOUS_TRACK);
      lv_label_set_text_fmt(label1, "Previous Track");
    }
    else if (btn == keyboard_param.volume_up)
    {
      bleHID.writeMediaKey(KEY_MEDIA_VOLUME_UP);
      lv_label_set_text_fmt(label1, "Volume Up");
    }
    else if (btn == keyboard_param.volume_down)
    {
      bleHID.writeMediaKey(KEY_MEDIA_VOLUME_DOWN);
      lv_label_set_text_fmt(label1, "Volume Down");
    }
    else if (btn == keyboard_param.media)
    {
      bleHID.writeMediaKey(KEY_MEDIA_WWW_HOME);
      lv_label_set_text_fmt(label1, "Media");
    }
  }
  else
  {
    lv_label_set_text_fmt(label1, "Not connected");
    // generate the code to wait for a connection
  }
}

// Return to the main menu

static lv_obj_t *create_keyboard_btn(lv_obj_t *parent, lv_align_t align, lv_coord_t x_ofs, lv_coord_t y_ofs, const char *symbol, lv_palette_t color)
{
  lv_obj_t *btn = lv_btn_create(parent);
  lv_obj_set_size(btn, 60, 60);
  lv_obj_set_style_radius(btn, 180, 0);
  lv_obj_align(btn, align, x_ofs, y_ofs);
  lv_obj_set_style_outline_color(btn, lv_color_white(), LV_STATE_FOCUS_KEY);
  lv_obj_set_style_bg_color(btn, lv_palette_main(color), 0); // Set the background color

  lv_obj_t *label = lv_label_create(btn);
  lv_obj_center(label);
  lv_label_set_text(label, symbol);

  return btn;
}

app_t app_keyboard = {
    .setup_func_cb = app_keyboard_load,
    .exit_func_cb = nullptr,
    .user_data = nullptr,
};
