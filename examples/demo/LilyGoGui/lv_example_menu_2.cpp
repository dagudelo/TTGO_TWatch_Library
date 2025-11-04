#include <lvgl.h>
#include "lv_example_menu_2.h"
#include "WiFi.h"
#include "global_flags.h"
#include "ui.h"
#include <stack>
#include <time.h>

char options[128] = ""; // Buffer to hold the options
lv_obj_t *roller1 = nullptr;
lv_obj_t *roller2 = nullptr;
lv_obj_t *roller3 = nullptr;
lv_obj_t *roller4 = nullptr;
lv_obj_t *day_roller = nullptr;
lv_obj_t *hour_slider = nullptr;
lv_obj_t *minute_slider = nullptr;

lv_obj_t *lv_cronometer_page_create(lv_obj_t *cont);
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
    /*
        lv_obj_set_size(menu, lv_disp_get_hor_res(NULL), lv_disp_get_ver_res(NULL));
        lv_obj_center(menu);
        /*Create a main page
        lv_obj_t *main_page =lv_menu_page_create(menu, "main Menu");
        lv_obj_set_size(main_page, lv_disp_get_hor_res(NULL), lv_disp_get_ver_res(NULL)-40);
        lv_obj_align(main_page, LV_ALIGN_TOP_MID, 0, 40);

        lv_obj_t *main_page_cont = lv_menu_cont_create(main_page);
        lv_obj_align(main_page_cont, LV_ALIGN_TOP_MID, 50, 10); // Adjust the position to prevent overlap with the back button
        label = lv_label_create(main_page);
        lv_label_set_text(label, "Alarm");
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
        lv_obj_add_event_cb(main_page_cont, alarm_event_handler, LV_EVENT_CLICKED, main_page);

        main_page_cont = lv_menu_cont_create(main_page);
        lv_obj_align(main_page_cont, LV_ALIGN_CENTER, 50, 0); // Adjust the position to prevent overlap with the back button
        label = lv_label_create(main_page_cont);
        lv_label_set_text(label, "cronometer");
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
        lv_obj_add_event_cb(main_page_cont, cronometer_event_handler, LV_EVENT_CLICKED, main_page);

        main_page_cont = lv_menu_cont_create(main_page);
        lv_obj_align(main_page_cont, LV_ALIGN_CENTER, 50, 0); // Adjust the position to prevent overlap with the back button
        label = lv_label_create(main_page_cont);
        lv_label_set_text(label, "Timer");
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
        lv_obj_add_event_cb(main_page_cont, timer_event_handler, LV_EVENT_CLICKED, main_page);
        lv_menu_set_page(menu, main_page);
        */
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

    // Get the current time
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    int current_year = tm->tm_year + 1900; // Years are counted from 1900

    int days_in_month = get_days_in_month(selected_month, current_year);

    char options[128] = ""; // Buffer to hold the options
    for (int i = 1; i <= days_in_month; i++)
    {
        char str[4];
        sprintf(str, "%d\n", i);
        strcat(options, str);
    }

    // lv_roller_set_options(roller2, options, LV_ROLLER_MODE_INFINITE);
}

lv_obj_t *lv_cronometer_page_create(lv_obj_t *cont)
{
    lv_obj_t *page = lv_obj_create(cont);
    lv_obj_set_size(page, 230, 230);
    // page->flags |= LV_OBJ_FLAG_HIDDEN; // Hide the page initially
    int roller_width = 230 / 4;       // Divide the width equally among the 4 rollers
    int roller_height = 230 - 10 - 5; // Subtract the top and bottom padding
    roller1 = lv_roller_create(page);
    lv_roller_set_options(roller1,
                          "Jan\n"
                          "Feb\n"
                          "Mar\n"
                          "Apr\n"
                          "May\n"
                          "Jun\n"
                          "Jul\n"
                          "Aug\n"
                          "Sep\n"
                          "Oct\n"
                          "Nov\n"
                          "Dec",
                          LV_ROLLER_MODE_INFINITE);

    lv_roller_set_visible_row_count(roller1, 4);
    lv_obj_center(roller1);
    lv_obj_add_event_cb(roller1, roller1_event_handler, LV_EVENT_ALL, NULL);
    lv_obj_set_size(roller1, roller_width, roller_height);
    // Position at the left edge with 10 px top padding
    lv_obj_align(roller1, LV_ALIGN_TOP_LEFT, 0, 10); // Align to the top left with 10 px top padding

    roller2 = lv_roller_create(page);
    lv_roller_set_options(roller2,
                          options,
                          LV_ROLLER_MODE_INFINITE);

    lv_roller_set_visible_row_count(roller2, 4);
    lv_obj_center(roller2);
    // lv_obj_add_event_cb(roller2, event_handler, LV_EVENT_ALL, NULL);
    lv_obj_set_size(roller2, roller_width, roller_height);
    lv_obj_align_to(roller2, roller1, LV_ALIGN_OUT_RIGHT_MID, 0, 0);

    roller3 = lv_roller_create(page);
    lv_roller_set_options(roller3,
                          "1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n11\n12\n13\n14\n15\n16\n17\n18\n19\n20\n21\n22\n23\n24",
                          LV_ROLLER_MODE_INFINITE);

    lv_roller_set_visible_row_count(roller3, 4);
    lv_obj_center(roller3);
    // lv_obj_add_event_cb(roller3, event_handler, LV_EVENT_ALL, NULL);
    lv_obj_set_size(roller3, roller_width, roller_height);
    lv_obj_align_to(roller3, roller2, LV_ALIGN_OUT_RIGHT_MID, 0, 0);

    roller4 = lv_roller_create(page);
    lv_roller_set_options(roller4,
                          "1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n11\n12\n13\n14\n15\n16\n17\n18\n19\n20\n21\n22\n23\n24",
                          LV_ROLLER_MODE_INFINITE);

    lv_roller_set_visible_row_count(roller4, 4);
    lv_obj_center(roller4);
    lv_obj_set_size(roller4, roller_width, roller_height);
    lv_obj_align_to(roller4, roller3, LV_ALIGN_OUT_RIGHT_MID, 0, 0); // Align to the right of roller3
    return page;
}

// void alarm_event_handler(lv_event_t *e)
// {
//     lv_obj_t *menu = lv_event_get_target(e);
//     lv_obj_add_flag(menu, LV_OBJ_FLAG_HIDDEN);
//     lv_obj_t *alarm_page = lv_obj_create(lv_scr_act());
//     lv_obj_center(alarm_page);
//     create_back_button(alarm_page);

//     lv_obj_clear_flag(alarm_page, LV_OBJ_FLAG_HIDDEN);
//     //  Hide the current page
//     //  Show the alarm page
//     //  Set the alarm page as the current page

//     /// Create the day roller
//     day_roller = lv_roller_create(alarm_page);
//     lv_roller_set_options(day_roller, "Mon\nTue\nWed\nThu\nFri\nSat\nSun", LV_ROLLER_MODE_INFINITE);
//     lv_obj_align(day_roller, LV_ALIGN_TOP_LEFT, 10, 10);

//     // Create the hour slider
//     hour_slider = lv_slider_create(alarm_page);
//     lv_slider_set_range(hour_slider, 0, 23);
//     lv_obj_align(hour_slider, LV_ALIGN_LEFT_MID, 10, 0);
//     lv_slider_set_mode(hour_slider, LV_SLIDER_MODE_SYMMETRICAL); // Make the slider vertical

//     // Create the minute slider
//     minute_slider = lv_slider_create(alarm_page);
//     lv_slider_set_range(minute_slider, 0, 59);
//     lv_obj_align(minute_slider, LV_ALIGN_LEFT_MID, 10, 50);
//     lv_slider_set_mode(minute_slider, LV_SLIDER_MODE_SYMMETRICAL);
//     // Create the set alarm button
//     lv_obj_t *set_alarm_button = lv_btn_create(alarm_page);
//     lv_obj_set_size(set_alarm_button, 100, 50);
//     lv_obj_align(set_alarm_button, LV_ALIGN_BOTTOM_MID, 0, -10);
//     lv_obj_add_event_cb(set_alarm_button, set_alarm_event_handler, LV_EVENT_CLICKED, NULL);
//     lv_obj_t *label = lv_label_create(set_alarm_button);
//     lv_label_set_text(label, "Set Alarm");
// }

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
