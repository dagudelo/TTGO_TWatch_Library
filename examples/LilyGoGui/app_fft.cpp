#include "app_fft.h"
#include "ui.h"

extern EventGroupHandle_t global_event_group;
lv_obj_t *chart = NULL;

static void change_fft_event_cb(lv_event_t *e);
extern void common_back_button_event_handler(lv_event_t *e);

extern void resume_vadTaskHandler(void);

void app_fft_load(lv_obj_t *cont) {
  resume_vadTaskHandler();
  /* Create a diagram. */
  chart = lv_chart_create(cont);
  lv_obj_set_size(chart, 240, 180);
  lv_obj_align(chart, LV_ALIGN_BOTTOM_MID, 0, -5);
  lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
  lv_chart_set_point_count(chart, SAMPLES / 2);
  lv_obj_set_style_opa(chart, LV_OPA_0, LV_PART_INDICATOR);
  /* Set the chart range. */
  lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, -3000, 3000);
  lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_X, 0, SAMPLES);
  
  lv_chart_series_t *ser1 = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y);

  lv_obj_add_event_cb(chart, change_fft_event_cb, LV_EVENT_MSG_RECEIVED, ser1);
  lv_msg_subsribe_obj(MSG_FFT_ID, chart, NULL);
  xEventGroupSetBits(global_event_group, FFT_READY);

  lv_obj_add_event_cb(
      chart, [](lv_event_t *e) { xEventGroupSetBits(global_event_group, FFT_STOP); }, LV_EVENT_DELETE, NULL);

    lv_obj_t *return_btn = lv_btn_create(cont);
    lv_obj_align(return_btn, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_set_style_bg_opa(return_btn, LV_OPA_0, 0);
    lv_obj_set_style_outline_color(return_btn, lv_color_white(), LV_STATE_FOCUS_KEY);
    lv_obj_set_size(return_btn, 40, 40);
    lv_obj_t *text = lv_label_create(return_btn);
    lv_obj_center(text);
    lv_label_set_text(text, LV_SYMBOL_LEFT);
    lv_obj_add_event_cb(return_btn, common_back_button_event_handler, LV_EVENT_CLICKED, &app_fft);
}

static void change_fft_event_cb(lv_event_t *e) {
  lv_obj_t *chart = lv_event_get_target(e);
  lv_chart_series_t *ser = (lv_chart_series_t *)lv_event_get_user_data(e);
  lv_msg_t *m = lv_event_get_msg(e);
  uint16_t *fft = (uint16_t *)lv_msg_get_payload(m);

  //lv_coord_t *ser_array = lv_chart_get_y_array(chart, ser);

  for (int i = 0; i < SAMPLES / 2; i++) {
    ser->y_points[i] = (uint16_t)fft[i];
  }
  lv_chart_refresh(chart);
}

app_t app_fft = {
    .setup_func_cb = app_fft_load,
    .exit_func_cb = nullptr,
    .user_data = nullptr,
};
