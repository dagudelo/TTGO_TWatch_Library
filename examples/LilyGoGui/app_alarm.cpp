#include "app_alarm.h"
#include "app_typedef.h"
#include "ui.h"
#include "lvgl.h"
#include <LilyGoLib.h>

static lv_style_t style_frameless;
void alarm_event_handler(lv_event_t *e);
void timer_event_handler(lv_event_t *e);
void cronometer_event_handler(lv_event_t *e);
extern void common_back_button_event_handler(lv_event_t *e);
static lv_obj_t *set_alarm_button, *label, *day_roller, *hour_roller, *minute_roller;
void printDateTime();

SensorPCF8563 rtc;

void app_alarm_load(lv_obj_t *cont)
{
    lv_obj_t *alarm_page = lv_obj_create(cont);

    lv_obj_set_size(alarm_page, 240, 220);
    lv_style_set_border_width(&style_frameless, 0);
    lv_style_set_radius(&style_frameless, 0);
    lv_obj_add_style(alarm_page, &style_frameless, 0);
    lv_obj_center(alarm_page);
    // lv_obj_t *label = lv_label_create(cont);
    // lv_label_set_text(label, "Alarm");
    // lv_obj_align(label,  LV_ALIGN_CENTER, 0, 0);

    day_roller = lv_roller_create(alarm_page);
    lv_roller_set_options(day_roller, "Mon\nTue\nWed\nThu\nFri\nSat\nSun", LV_ROLLER_MODE_INFINITE);
    lv_obj_set_size(day_roller, 80, 180);
    lv_obj_align(day_roller, LV_ALIGN_OUT_TOP_LEFT, -5, 10);

    // Create the hour roller
    hour_roller = lv_roller_create(alarm_page);
    lv_roller_set_options(hour_roller, "00\n01\n02\n03\n04\n05\n06\n07\n08\n09\n10\n11\n12\n13\n14\n15\n16\n17\n18\n19\n20\n21\n22\n23", LV_ROLLER_MODE_INFINITE);
    lv_obj_set_size(hour_roller, 80, 180);
    lv_obj_align(hour_roller, LV_ALIGN_OUT_TOP_LEFT, 75, 10);

    // Create the minute roller
    minute_roller = lv_roller_create(alarm_page);
    lv_roller_set_options(minute_roller, "00\n01\n02\n03\n04\n05\n06\n07\n08\n09\n10\n11\n12\n13\n14\n15\n16\n17\n18\n19\n20\n21\n22\n23\n24\n25\n26\n27\n28\n29\n30\n31\n32\n33\n34\n35\n36\n37\n38\n39\n40\n41\n42\n43\n44\n45\n46\n47\n48\n49\n50\n51\n52\n53\n54\n55\n56\n57\n58\n59\n60", LV_ROLLER_MODE_INFINITE);
    lv_obj_set_size(minute_roller, 80, 180);
    lv_obj_align(minute_roller, LV_ALIGN_OUT_TOP_LEFT, 155, 10);

    // Create the set alarm button
    set_alarm_button = lv_btn_create(alarm_page);
    lv_obj_set_size(set_alarm_button, 150, 20);
    lv_obj_align(set_alarm_button, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_add_event_cb(set_alarm_button, alarm_event_handler, LV_EVENT_CLICKED, NULL);
    label = lv_label_create(set_alarm_button);
    lv_label_set_text(label, "Set Alarm");

    lv_obj_t *return_btn = lv_btn_create(alarm_page);
    lv_obj_align(return_btn, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_set_style_bg_opa(return_btn, LV_OPA_0, 0);
    lv_obj_set_style_outline_color(return_btn, lv_color_white(), LV_STATE_FOCUS_KEY);
    lv_obj_set_size(return_btn, 40, 40);
    lv_obj_t *text = lv_label_create(return_btn);
    lv_obj_center(text);
    lv_label_set_text(text, LV_SYMBOL_LEFT);
    lv_obj_add_event_cb(return_btn, common_back_button_event_handler, LV_EVENT_CLICKED, &app_alarm);
}
void alarm_event_handler(lv_event_t *e)
{
    lv_label_set_text(label, "Clicked");
    RTC_Alarm alarm = watch.getAlarm();
    rtc.resetAlarm();
    Serial.printf("Alarm was set for: %02d:%02d:%02d\n", alarm.hour, alarm.minute, alarm.day, alarm.week);
     Serial.printf("Alarm input day %d, hour %d min %d\n", lv_roller_get_selected(day_roller), lv_roller_get_selected(hour_roller), lv_roller_get_selected(minute_roller));
    

   
    // watch.setAlarmByDays(0);
   
    RTC_DateTime datetime = watch.getDateTime();
    Serial.printf("Current time: %02d:%02d:%02d\n", datetime.hour, datetime.minute, datetime.second);
    rtc.setAlarm(static_cast<uint8_t>(lv_roller_get_selected(hour_roller)),static_cast<uint8_t>( lv_roller_get_selected(minute_roller)), datetime.day, static_cast<uint8_t>(lv_roller_get_selected(day_roller)));
    rtc.enableAlarm();
    Serial.println(rtc.isAlarmActive());
    

    alarm = rtc.getAlarm();
    Serial.printf("Alarm set for: hour: %d: Minute: %d Day:%d Week:%d\n", alarm.hour, alarm.minute, alarm.day, alarm.week);
    // lv_obj_t *alarm = lv_label_create(lv_scr_act());

    // lv_label_set_text_fmt(alarm, "Alarm set for %s %s:%s", lv_roller_get_selected(day_roller), lv_roller_get_selected(hour_roller),  lv_roller_get_selected(minute_roller));
    // lv_obj_align(alarm, LV_ALIGN_TOP_MID, 0, 0);
    // printDateTime();
}

void printDateTime()
{
    unsigned long lastMillis = 0;
    if (millis() - lastMillis > 1000)
    {
        /**
        /// Format output time*
        Option:
            DATETIME_FORMAT_HM
            DATETIME_FORMAT_HMS
            DATETIME_FORMAT_YYYY_MM_DD
            DATETIME_FORMAT_MM_DD_YYYY
            DATETIME_FORMAT_DD_MM_YYYY
            DATETIME_FORMAT_YYYY_MM_DD_H_M_S
        default:   DATETIME_FORMAT_YYYY_MM_DD_H_M_S_WEEK
        */
        const char *str = watch.strftime();
        Serial.println(str);
        lastMillis = millis();
    }
    lv_task_handler();
    delay(5);
}
app_t app_alarm = {
    .setup_func_cb = app_alarm_load,
    .exit_func_cb = nullptr,
    .user_data= nullptr,
};