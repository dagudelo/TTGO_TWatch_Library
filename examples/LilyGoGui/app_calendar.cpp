#include "app_calendar.h"
#include "Arduino.h"
#include "lvgl.h"

// This is defined in LilyGoGui.ino
extern struct tm show_timeinfo;

static lv_obj_t *calendar;
static lv_obj_t *selected_date_label;

static void event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_current_target(e);

    if (code == LV_EVENT_VALUE_CHANGED)
    {
        lv_calendar_date_t date;
        if (lv_calendar_get_pressed_date(obj, &date))
        {
            LV_LOG_USER("Clicked date: %02d.%02d.%d", date.day, date.month, date.year);
            if (selected_date_label) {
                lv_label_set_text_fmt(selected_date_label, "Selected: %d-%02d-%02d", date.year, date.month, date.day);
            }
        }
    }
}

static void create_calendar_view(lv_obj_t *parent)
{
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_set_size(cont, 240, 240);
    lv_obj_align(cont, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(cont, lv_color_hex(0x000000), 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_style_pad_all(cont, 0, 0);

    calendar = lv_calendar_create(cont);
    lv_obj_set_size(calendar, 240, 200); // Increased height a bit
    lv_obj_align(calendar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_add_event_cb(calendar, event_handler, LV_EVENT_ALL, NULL);

    if (show_timeinfo.tm_year > 100) { // Check if time is valid (year > 2000)
        lv_calendar_set_today_date(calendar, show_timeinfo.tm_year + 1900, show_timeinfo.tm_mon + 1, show_timeinfo.tm_mday);
        lv_calendar_set_showed_date(calendar, show_timeinfo.tm_year + 1900, show_timeinfo.tm_mon + 1);
    } else {
        // Fallback to a default date if RTC time is not available
        lv_calendar_set_today_date(calendar, 2025, 11, 20);
        lv_calendar_set_showed_date(calendar, 2025, 11);
    }


#if LV_USE_CALENDAR_HEADER_DROPDOWN
    lv_calendar_header_dropdown_create(calendar);
#elif LV_USE_CALENDAR_HEADER_ARROW
    lv_calendar_header_arrow_create(calendar);
#endif

    // Label to show selected date
    selected_date_label = lv_label_create(cont);
    lv_label_set_text(selected_date_label, "Select a date");
    lv_obj_set_style_text_color(selected_date_label, lv_color_white(), 0);
    lv_obj_align(selected_date_label, LV_ALIGN_BOTTOM_LEFT, 10, -10);
}


void app_calendar_load(lv_obj_t *cont) {
	  create_calendar_view(cont);
}

app_t app_calendar = {
    .setup_func_cb = app_calendar_load,
    .exit_func_cb = nullptr,
    .user_data= nullptr,
};
