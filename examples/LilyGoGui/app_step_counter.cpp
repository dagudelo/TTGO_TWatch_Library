#include "app_step_counter.h"
#include "Arduino.h"
#include "ui.h"

LV_FONT_DECLARE(AlimamaShuHeiTi_Bold_54);
LV_IMG_DECLARE(step_img);
lv_obj_t * step_counter_label = NULL;

extern lv_obj_t *setupGUI();
extern void common_back_button_event_handler(lv_event_t *e);

void app_step_counter_load(lv_obj_t *cont) {
    lv_obj_t * obj = lv_obj_create(cont);
    lv_obj_set_size(obj, 240, 240);
    lv_obj_align(obj, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x000000), 0);
    lv_obj_set_style_border_width(obj, 0, 0);  
    lv_obj_set_style_pad_all(obj, 0, 0); 

    lv_obj_t * img = lv_img_create(obj);
    lv_obj_set_style_shadow_opa(img, LV_OPA_0, 0); 
    lv_obj_set_size(img, 60, 60);  
    lv_img_set_src(img, &step_img);
    lv_obj_align(img, LV_ALIGN_CENTER, -60, -10);

    step_counter_label = lv_label_create(obj);
    lv_obj_set_style_text_color(step_counter_label, lv_color_white(), 0);
    lv_obj_align(step_counter_label, LV_ALIGN_CENTER, 0, 0);
    lv_label_set_text(step_counter_label, "0");
    lv_obj_set_style_text_font(step_counter_label, &AlimamaShuHeiTi_Bold_54, 0);

    lv_obj_t *return_btn = lv_btn_create(obj);
    lv_obj_align(return_btn, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_set_style_bg_opa(return_btn, LV_OPA_0, 0);
    lv_obj_set_style_outline_color(return_btn, lv_color_white(), LV_STATE_FOCUS_KEY);
    lv_obj_set_size(return_btn, 40, 40);
    lv_obj_t *text = lv_label_create(return_btn);
    lv_obj_center(text);
    lv_label_set_text(text, LV_SYMBOL_LEFT);
    lv_obj_add_event_cb(return_btn, common_back_button_event_handler, LV_EVENT_CLICKED, &app_step_counter);
}


app_t app_step_counter = {
    .setup_func_cb = app_step_counter_load,
    .exit_func_cb = nullptr,
    .user_data= nullptr,
};