#include "app_music.h"
#include "SD_MMC.h"
#include "ui.h"

static struct {
  lv_obj_t *music_list;

  lv_obj_t *play;
  lv_obj_t *pause;
  lv_obj_t *stop;
  lv_obj_t *progress_bar;
  lv_obj_t *time_label;
} music_param;

extern QueueHandle_t play_music_queue;
extern QueueHandle_t play_time_queue;

static lv_obj_t *create_music_btn(lv_obj_t *parent, lv_align_t align, lv_coord_t x_ofs, lv_coord_t y_ofs, const char *symbol);

static void play_event_cb(lv_event_t *e);
static void pause_event_cb(lv_event_t *e);
static void stop_event_cb(lv_event_t *e);
extern void common_back_button_event_handler(lv_event_t *e);

void app_music_load(lv_obj_t *cont) {

  /*Create a list*/
  lv_obj_t *music_list = music_param.music_list = lv_roller_create(cont);
  lv_obj_set_size(music_list, LV_PCT(95), 120);
  lv_obj_align(music_list, LV_ALIGN_TOP_RIGHT, -5, 45);
  lv_obj_set_style_outline_color(music_list, lv_color_white(), LV_STATE_FOCUS_KEY);

  lv_list_add_text(music_list, "Music name");
  String name_list;
  
  name_list += "mp3_array";
  name_list += "\n";
  name_list += "mp3_ring_setup";
  name_list += "\n";
   name_list += "boot_music";
  name_list += "\n";

  lv_roller_set_options(music_list, name_list.c_str(), LV_ROLLER_MODE_NORMAL);

  music_param.play = create_music_btn(cont, LV_ALIGN_LEFT_MID, 65, 70, LV_SYMBOL_PLAY);
  music_param.pause = create_music_btn(cont, LV_ALIGN_LEFT_MID, 105, 70, LV_SYMBOL_PAUSE);
  music_param.stop = create_music_btn(cont, LV_ALIGN_LEFT_MID, 145, 70, LV_SYMBOL_STOP);
  lv_obj_set_style_bg_color(music_param.stop, lv_palette_main(LV_PALETTE_PINK), 0);

  lv_obj_add_event_cb(music_param.play, play_event_cb, LV_EVENT_CLICKED, music_list);
  lv_obj_add_event_cb(music_param.pause, pause_event_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_add_event_cb(music_param.stop, stop_event_cb, LV_EVENT_CLICKED, NULL);

  lv_obj_t *return_btn = lv_btn_create(cont);
  lv_obj_align(return_btn, LV_ALIGN_TOP_LEFT, 10, 10);
  lv_obj_set_style_bg_opa(return_btn, LV_OPA_0, 0);
  lv_obj_set_style_outline_color(return_btn, lv_color_white(), LV_STATE_FOCUS_KEY);
  lv_obj_set_size(return_btn, 40, 40);
  lv_obj_t *text = lv_label_create(return_btn);
  lv_obj_center(text);
  lv_label_set_text(text, LV_SYMBOL_LEFT);
  lv_obj_add_event_cb(return_btn, common_back_button_event_handler, LV_EVENT_CLICKED, &app_music);
}

static lv_obj_t *create_music_btn(lv_obj_t *parent, lv_align_t align, lv_coord_t x_ofs, lv_coord_t y_ofs, const char *symbol) {
  lv_obj_t *btn = lv_btn_create(parent);
  lv_obj_set_size(btn, 30, 30);
  lv_obj_set_style_radius(btn, 90, 0);
  lv_obj_align(btn, align, x_ofs, y_ofs);
  lv_obj_set_style_outline_color(btn, lv_color_white(), LV_STATE_FOCUS_KEY);

  lv_obj_t *label = lv_label_create(btn);
  lv_obj_center(label);
  lv_label_set_text(label, symbol);

  return btn;
}

static void play_event_cb(lv_event_t *e) {
  lv_obj_t *list = (lv_obj_t *)lv_event_get_user_data(e);
  char play_music_path[50] = {0};
  lv_roller_get_selected_str(list, play_music_path, sizeof(play_music_path));
  std::string *pStr = new std::string(play_music_path);
  //Serial.println(path);
  xQueueSend(play_music_queue, &pStr, (TickType_t)10);
}

static void pause_event_cb(lv_event_t *e) {
    LV_UNUSED(e);
    // Placeholder for pause functionality
    // xEventGroupSetBits(global_event_group, RING_PAUSE);
}

static void stop_event_cb(lv_event_t *e) {
    LV_UNUSED(e);
    // Placeholder for stop functionality
    // xEventGroupSetBits(global_event_group, RING_STOP);
}


app_t app_music = {
    .setup_func_cb = app_music_load,
    .exit_func_cb = nullptr,
    .user_data= nullptr,
};
