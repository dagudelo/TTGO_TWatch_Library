#include <lvgl.h>
#include "lv_example_menu_2.h"
#include "WiFi.h"
#include "global_flags.h"
#include "ui.h"
#include <stack>

char options[128] = ""; // Buffer to hold the options
lv_obj_t *roller1 = nullptr;
lv_obj_t *roller2 = nullptr;
lv_obj_t *roller3 = nullptr;
lv_obj_t *roller4 = nullptr;
lv_obj_t *day_roller = nullptr;
lv_obj_t *hour_slider = nullptr;
lv_obj_t *minute_slider = nullptr;

extern void alarm_event_handler(lv_event_t *e);
void timer_event_handler(lv_event_t *e);
void cronometer_event_handler(lv_event_t *e);
void set_alarm_event_handler(lv_event_t *e);
lv_obj_t *create_back_button(lv_obj_t *parent);

static void back_event_handler(lv_event_t *e)
{
    lv_obj_t *obj = lv_event_get_target(e);
    lv_obj_t *menu = lv_obj_create(static_cast<lv_obj_t *>(e->user_data));

    if (lv_menu_back_btn_is_root(menu, obj))
    {
        lv_obj_t *mbox1 = lv_msgbox_create(NULL, "Hello", "Root back btn click.", NULL, true);
        lv_obj_center(mbox1);
    }
}

void lv_example_menu_2_load(lv_obj_t *cont)
{
    // lv_obj_t *label;
    // lv_obj_t *menu = lv_menu_create(cont);
    lv_obj_t *list = lv_list_create(cont);
    lv_obj_set_size(list, lv_pct(100), lv_pct(80));
    lv_obj_align(list, LV_ALIGN_CENTER, 0, 10);

    lv_obj_t *btn;

    btn = lv_list_add_btn(list, LV_SYMBOL_HOME, "Alarm");
    lv_obj_add_event_cb(btn, alarm_event_handler, LV_EVENT_CLICKED, NULL);

    btn = lv_list_add_btn(list, LV_SYMBOL_HOME, "Cronometer");
    lv_obj_add_event_cb(btn, cronometer_event_handler, LV_EVENT_CLICKED, NULL);

    btn = lv_list_add_btn(list, LV_SYMBOL_HOME, "Timer");
    lv_obj_add_event_cb(btn, timer_event_handler, LV_EVENT_CLICKED, NULL);
}
int get_days_in_month(int month, int year)
{
    if (month == 2)
    {
        // February
        if (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0))
        {
            // Leap year
            return 29;
        }
        else
        {
            // Non-leap year
            return 28;
        }
    }
    else if (month == 4 || month == 6 || month == 9 || month == 11)
    {
        // April, June, September, November
        return 30;
    }
    else
    {
        // January, March, May, July, August, October, December
        return 31;
    }
}

void roller1_event_handler(lv_event_t *e)
{
    lv_obj_t *roller1 = lv_event_get_target(e);
    int selected_month = lv_roller_get_selected(roller1) + 1; // Months are 1-indexed

    int current_year = 2023; // Placeholder for year logic

    int days_in_month = get_days_in_month(selected_month, current_year);

    char options[128] = ""; // Buffer to hold the options
    for (int i = 1; i <= days_in_month; i++)
    {
        char str[4];
        sprintf(str, "%d\n", i);
        strcat(options, str);
    }
}

void set_alarm_event_handler(lv_event_t *e)
{
    // Get the selected day, hour, and minute
    char day[3] = "";
    lv_roller_get_selected_str(day_roller, day, 3);
    int selected_hour = lv_slider_get_value(hour_slider);
    int selected_minute = lv_slider_get_value(minute_slider);

    // Set the alarm
    // Note: This is a placeholder. Replace this with the actual code to set the alarm.
    // TTGOClass *ttgo = TTGOClass::getWatch();
    // ttgo->rtc->setAlarm(selected_day, selected_hour, selected_minute);
}

void cronometer_event_handler(lv_event_t *e)
{
    lv_obj_t *menu = lv_event_get_target(e);
    lv_obj_add_flag(menu, LV_OBJ_FLAG_HIDDEN);
    lv_obj_t *cronometer_page = lv_obj_create(lv_scr_act());
    lv_obj_clear_flag(cronometer_page, LV_OBJ_FLAG_HIDDEN);
    // Show the cronometer page
    // Set the cronometer page as the current page
    lv_menu_set_page(menu, cronometer_page);
}

void timer_event_handler(lv_event_t *e)
{
    lv_obj_t *menu = lv_event_get_target(e);
    lv_obj_add_flag(menu, LV_OBJ_FLAG_HIDDEN);
    lv_obj_t *timer_page = lv_obj_create(lv_scr_act());
    lv_obj_clear_flag(timer_page, LV_OBJ_FLAG_HIDDEN);
    // Show the timer page
    // Set the timer page as the current page
    lv_menu_set_page(menu, timer_page);
}
lv_obj_t *create_back_button(lv_obj_t *parent)
{
    lv_obj_t *back_button = lv_btn_create(parent);

    // Create a style for the button
    static lv_style_t style;
    lv_style_init(&style);
    lv_style_set_bg_color(&style, lv_color_white());
    lv_style_set_radius(&style, 10);
    lv_obj_add_style(back_button, &style, 0);

    lv_obj_set_size(back_button, 50, 50);
    lv_obj_align(back_button, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_add_event_cb(back_button, back_event_handler, LV_EVENT_CLICKED, NULL);

    lv_obj_t *back_label = lv_label_create(back_button);

    // Create a style for the label
    static lv_style_t label_style;
    lv_style_init(&label_style);
    lv_style_set_text_color(&label_style, lv_color_black());
    lv_obj_add_style(back_label, &label_style, 0);

    lv_label_set_text(back_label, "<");

    return back_button;
}

app_t lv_example_menu_2 = {
    .setup_func_cb = lv_example_menu_2_load,
    .exit_func_cb = nullptr,
    .user_data = nullptr,
};
