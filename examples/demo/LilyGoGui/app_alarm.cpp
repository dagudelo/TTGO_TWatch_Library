#include "app_alarm.h"
#include "app_typedef.h"
#include "lvgl.h"
#include <LilyGoLib.h>
#include <AudioFileSourcePROGMEM.h>
#include <AudioGeneratorMP3.h>
#include <AudioOutputI2S.h>

// Sound system - external MP3 arrays
extern const unsigned char mp3_array[16509];
extern unsigned char mp3_ring_setup[86144];
extern const uint8_t boot_music[4365];

// Audio objects for alarm sounds
static AudioFileSourcePROGMEM *alarm_file = nullptr;
static AudioGeneratorMP3 *alarm_mp3 = nullptr;
static AudioOutputI2S *alarm_out = nullptr;
static bool alarm_audio_initialized = false;

enum AlarmSound {
    SOUND_VIBRATE_ONLY = 0,
    SOUND_BEEP = 1,
    SOUND_RING = 2,
    SOUND_MELODY = 3
};

enum RecurrenceType {
    RECUR_ONCE = 0,
    RECUR_DAILY = 1,
    RECUR_WEEKDAYS = 2,
    RECUR_WEEKENDS = 3,
    RECUR_CUSTOM = 4
};

// Alarm data structure
struct AlarmData {
    bool enabled;
    uint8_t hour;
    uint8_t minute;
    RecurrenceType recurrence;
    bool days[7]; // Mon-Sun
    AlarmSound sound;
    bool vibrate;
    bool sound_enabled;
};

// Stopwatch data
struct StopwatchData {
    bool running;
    unsigned long start_time;
    unsigned long elapsed_time;
    unsigned long lap_times[10];
    int lap_count;
    bool sound_enabled;
};

// Timer data
struct TimerData {
    bool running;
    unsigned long duration_ms;
    unsigned long start_time;
    AlarmSound sound;
    bool sound_enabled;
};

// Global state
static AlarmData g_alarm = {false, 0, 0, RECUR_ONCE, {false}, SOUND_BEEP, true, true};
static StopwatchData g_stopwatch = {false, 0, 0, {0}, 0, true};
static TimerData g_timer = {false, 0, 0, SOUND_BEEP, true};

// UI objects
static lv_obj_t *main_tabview = nullptr;
static lv_style_t style_frameless;
SensorPCF8563 rtc;

// Forward declarations
void create_alarm_tab(lv_obj_t *parent);
void create_stopwatch_tab(lv_obj_t *parent);
void create_timer_tab(lv_obj_t *parent);
void init_alarm_audio();
void play_alarm_sound(AlarmSound sound, int repeat_count = 3);

// ==================== ALARM TAB ====================
void create_alarm_tab(lv_obj_t *parent) {
    static lv_obj_t *hour_roller, *minute_roller, *recur_dropdown, *sound_dropdown;
    static lv_obj_t *status_label;
    static lv_obj_t *day_checkboxes[7];
    
    // Create scrollable container
    lv_obj_set_scrollbar_mode(parent, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_scroll_dir(parent, LV_DIR_VER);
    
    int y_pos = 0;
    
    // Status label at top
    status_label = lv_label_create(parent);
    lv_label_set_text(status_label, g_alarm.enabled ? "#00FF00 Alarm Active#" : "#808080 Alarm Off#");
    lv_label_set_recolor(status_label, true);
    lv_obj_set_pos(status_label, 50, y_pos);
    y_pos += 22;
    
    // Time selectors - BIGGER rollers
    lv_obj_t *time_cont = lv_obj_create(parent);
    lv_obj_set_size(time_cont, 220, 85);
    lv_obj_set_pos(time_cont, 0, y_pos);
    lv_obj_set_style_pad_all(time_cont, 2, 0);
    lv_obj_clear_flag(time_cont, LV_OBJ_FLAG_SCROLLABLE);
    
    hour_roller = lv_roller_create(time_cont);
    lv_roller_set_options(hour_roller, "00\n01\n02\n03\n04\n05\n06\n07\n08\n09\n10\n11\n12\n13\n14\n15\n16\n17\n18\n19\n20\n21\n22\n23", LV_ROLLER_MODE_INFINITE);
    lv_obj_set_size(hour_roller, 80, 78);
    lv_obj_align(hour_roller, LV_ALIGN_LEFT_MID, 2, 0);
    lv_roller_set_selected(hour_roller, g_alarm.hour, LV_ANIM_OFF);
    lv_roller_set_visible_row_count(hour_roller, 3);
    
    lv_obj_t *colon = lv_label_create(time_cont);
    lv_label_set_text(colon, ":");
    lv_obj_set_style_text_font(colon, &lv_font_montserrat_32, 0);
    lv_obj_center(colon);
    
    minute_roller = lv_roller_create(time_cont);
    lv_roller_set_options(minute_roller, "00\n05\n10\n15\n20\n25\n30\n35\n40\n45\n50\n55", LV_ROLLER_MODE_INFINITE);
    lv_obj_set_size(minute_roller, 80, 78);
    lv_obj_align(minute_roller, LV_ALIGN_RIGHT_MID, -2, 0);
    lv_roller_set_selected(minute_roller, g_alarm.minute / 5, LV_ANIM_OFF);
    lv_roller_set_visible_row_count(minute_roller, 3);
    
    y_pos += 90;
    
    // Recurrence selector
    lv_obj_t *recur_label = lv_label_create(parent);
    lv_label_set_text(recur_label, "Repeat:");
    lv_obj_set_pos(recur_label, 0, y_pos + 3);
    
    recur_dropdown = lv_dropdown_create(parent);
    lv_dropdown_set_options(recur_dropdown, "Once\nDaily\nWeekdays\nWeekends\nCustom");
    lv_obj_set_size(recur_dropdown, 170, 30);
    lv_obj_set_pos(recur_dropdown, 50, y_pos);
    lv_dropdown_set_selected(recur_dropdown, g_alarm.recurrence);
    
    y_pos += 38;
    
    // Sound selector
    lv_obj_t *sound_label = lv_label_create(parent);
    lv_label_set_text(sound_label, "Sound:");
    lv_obj_set_pos(sound_label, 0, y_pos + 3);
    
    sound_dropdown = lv_dropdown_create(parent);
    lv_dropdown_set_options(sound_dropdown, "Vibrate\nBeep\nRing\nMelody");
    lv_obj_set_size(sound_dropdown, 170, 30);
    lv_obj_set_pos(sound_dropdown, 50, y_pos);
    lv_dropdown_set_selected(sound_dropdown, g_alarm.sound);
    
    y_pos += 38;
    
    // Sound ON/OFF toggle
    lv_obj_t *sound_toggle = lv_switch_create(parent);
    lv_obj_set_size(sound_toggle, 50, 25);
    lv_obj_set_pos(sound_toggle, 0, y_pos);
    if (g_alarm.sound_enabled) lv_obj_add_state(sound_toggle, LV_STATE_CHECKED);
    
    lv_obj_t *sound_toggle_label = lv_label_create(parent);
    lv_label_set_text(sound_toggle_label, "Sound ON");
    lv_obj_set_pos(sound_toggle_label, 55, y_pos + 3);
    
    lv_obj_add_event_cb(sound_toggle, [](lv_event_t *e) {
        g_alarm.sound_enabled = lv_obj_has_state(lv_event_get_target(e), LV_STATE_CHECKED);
        Serial.printf("Alarm sound: %s\n", g_alarm.sound_enabled ? "ON" : "OFF");
    }, LV_EVENT_VALUE_CHANGED, NULL);
    
    y_pos += 33;
    
    // Day checkboxes (for custom recurrence)
    lv_obj_t *days_label = lv_label_create(parent);
    lv_label_set_text(days_label, "Custom Days:");
    lv_obj_set_pos(days_label, 0, y_pos);
    y_pos += 22;
    
    const char *day_names[] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
    for (int i = 0; i < 7; i++) {
        day_checkboxes[i] = lv_checkbox_create(parent);
        lv_checkbox_set_text(day_checkboxes[i], day_names[i]);
        lv_obj_set_size(day_checkboxes[i], 55, 24);
        lv_obj_set_pos(day_checkboxes[i], 0 + (i % 4) * 56, y_pos + (i / 4) * 28);
        if (g_alarm.days[i]) lv_obj_add_state(day_checkboxes[i], LV_STATE_CHECKED);
    }
    
    y_pos += 62;
    
    // Buttons container
    lv_obj_t *btn_cont = lv_obj_create(parent);
    lv_obj_set_size(btn_cont, 220, 42);
    lv_obj_set_pos(btn_cont, 0, y_pos);
    lv_obj_set_style_pad_all(btn_cont, 2, 0);
    lv_obj_clear_flag(btn_cont, LV_OBJ_FLAG_SCROLLABLE);
    
    // Set Alarm button
    lv_obj_t *set_btn = lv_btn_create(btn_cont);
    lv_obj_set_size(set_btn, 102, 36);
    lv_obj_align(set_btn, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_t *set_label = lv_label_create(set_btn);
    lv_label_set_text(set_label, g_alarm.enabled ? "Disable" : "Enable");
    lv_obj_center(set_label);
    
    lv_obj_add_event_cb(set_btn, [](lv_event_t *e) {
        lv_obj_t *hour_r = (lv_obj_t*)lv_event_get_user_data(e);
        lv_obj_t *parent = lv_obj_get_parent(lv_event_get_target(e));
        lv_obj_t *minute_r = (lv_obj_t*)lv_obj_get_user_data(parent);
        
        g_alarm.enabled = !g_alarm.enabled;
        lv_obj_t *label = lv_obj_get_child(lv_event_get_target(e), 0);
        lv_label_set_text(label, g_alarm.enabled ? "Disable" : "Enable");
        
        if (g_alarm.enabled) {
            g_alarm.hour = lv_roller_get_selected(hour_r);
            g_alarm.minute = lv_roller_get_selected(minute_r) * 5;
            Serial.printf("Alarm set for %02d:%02d\n", g_alarm.hour, g_alarm.minute);
        }
    }, LV_EVENT_CLICKED, hour_roller);
    lv_obj_set_user_data(btn_cont, minute_roller);
    
    // Test Sound button
    lv_obj_t *test_btn = lv_btn_create(btn_cont);
    lv_obj_set_size(test_btn, 102, 36);
    lv_obj_align(test_btn, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_t *test_label = lv_label_create(test_btn);
    lv_label_set_text(test_label, "Test");
    lv_obj_center(test_label);
    
    lv_obj_add_event_cb(test_btn, [](lv_event_t *e) {
        if (g_alarm.sound_enabled) {
            play_alarm_sound(g_alarm.sound, 2);  // Test with 2 repetitions
        }
    }, LV_EVENT_CLICKED, NULL);
    
    // Set content height for scrolling
    y_pos += 48;
    lv_obj_set_height(parent, y_pos);
}

// ==================== STOPWATCH TAB ====================
void create_stopwatch_tab(lv_obj_t *parent) {
    static lv_obj_t *time_label, *lap_list, *start_stop_btn, *lap_reset_btn;
    
    // Enable scrolling
    lv_obj_set_scrollbar_mode(parent, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_scroll_dir(parent, LV_DIR_VER);
    
    int y_pos = 5;
    
    // Time display
    time_label = lv_label_create(parent);
    lv_obj_set_style_text_font(time_label, &lv_font_montserrat_48, 0);
    lv_label_set_text(time_label, "00:00.00");
    lv_obj_set_pos(time_label, 25, y_pos);
    y_pos += 55;
    
    // Sound toggle for stopwatch
    lv_obj_t *sw_sound_toggle = lv_switch_create(parent);
    lv_obj_set_size(sw_sound_toggle, 50, 25);
    lv_obj_set_pos(sw_sound_toggle, 5, y_pos);
    if (g_stopwatch.sound_enabled) lv_obj_add_state(sw_sound_toggle, LV_STATE_CHECKED);
    
    lv_obj_t *sw_sound_label = lv_label_create(parent);
    lv_label_set_text(sw_sound_label, "Beep on Lap");
    lv_obj_set_pos(sw_sound_label, 60, y_pos + 3);
    
    lv_obj_add_event_cb(sw_sound_toggle, [](lv_event_t *e) {
        g_stopwatch.sound_enabled = lv_obj_has_state(lv_event_get_target(e), LV_STATE_CHECKED);
        Serial.printf("Stopwatch sound: %s\n", g_stopwatch.sound_enabled ? "ON" : "OFF");
    }, LV_EVENT_VALUE_CHANGED, NULL);
    
    y_pos += 32;
    
    // Button container
    lv_obj_t *btn_cont = lv_obj_create(parent);
    lv_obj_set_size(btn_cont, 220, 50);
    lv_obj_set_pos(btn_cont, 0, y_pos);
    lv_obj_set_style_pad_all(btn_cont, 2, 0);
    lv_obj_clear_flag(btn_cont, LV_OBJ_FLAG_SCROLLABLE);
    
    // Start/Stop button
    start_stop_btn = lv_btn_create(btn_cont);
    lv_obj_set_size(start_stop_btn, 102, 44);
    lv_obj_align(start_stop_btn, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_bg_color(start_stop_btn, lv_color_hex(0x0000FF), 0);
    lv_obj_t *ss_label = lv_label_create(start_stop_btn);
    lv_label_set_text(ss_label, "Start");
    lv_obj_center(ss_label);
    
    lv_obj_add_event_cb(start_stop_btn, [](lv_event_t *e) {
        lv_obj_t *btn = lv_event_get_target(e);
        lv_obj_t *label = lv_obj_get_child(btn, 0);
        
        if (!g_stopwatch.running) {
            // Start
            g_stopwatch.running = true;
            g_stopwatch.start_time = millis() - g_stopwatch.elapsed_time;
            lv_label_set_text(label, "Stop");
            lv_obj_set_style_bg_color(btn, lv_color_hex(0xFF0000), 0);
        } else {
            // Stop
            g_stopwatch.running = false;
            g_stopwatch.elapsed_time = millis() - g_stopwatch.start_time;
            lv_label_set_text(label, "Start");
            lv_obj_set_style_bg_color(btn, lv_color_hex(0x0000FF), 0);
        }
    }, LV_EVENT_CLICKED, NULL);
    
    // Lap/Reset button
    lap_reset_btn = lv_btn_create(btn_cont);
    lv_obj_set_size(lap_reset_btn, 102, 44);
    lv_obj_align(lap_reset_btn, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_t *lr_label = lv_label_create(lap_reset_btn);
    lv_label_set_text(lr_label, "Lap");
    lv_obj_center(lr_label);
    
    // Lap times list - Create BEFORE setting callback
    lap_list = lv_list_create(parent);
    lv_obj_set_size(lap_list, 220, 250);  // Larger to accommodate scrolling
    lv_obj_set_pos(lap_list, 0, y_pos + 55);
    
    lv_obj_add_event_cb(lap_reset_btn, [](lv_event_t *e) {
        lv_obj_t *list = (lv_obj_t*)lv_event_get_user_data(e);
        
        if (g_stopwatch.running) {
            // Record lap
            if (g_stopwatch.lap_count < 10) {
                g_stopwatch.lap_times[g_stopwatch.lap_count++] = millis() - g_stopwatch.start_time;
                
                // Add to list - FIXED: Use lv_list_add_text instead
                char lap_text[48];
                unsigned long lap_ms = g_stopwatch.lap_times[g_stopwatch.lap_count - 1];
                sprintf(lap_text, "Lap %d: %02lu:%02lu.%02lu", g_stopwatch.lap_count,
                        lap_ms / 60000, (lap_ms / 1000) % 60, (lap_ms / 10) % 100);
                
                lv_obj_t *btn = lv_list_add_btn(list, LV_SYMBOL_RIGHT, lap_text);
                lv_obj_set_style_text_font(btn, &lv_font_montserrat_14, 0);
                
                // Scroll to bottom
                lv_obj_scroll_to_view(btn, LV_ANIM_ON);
                
                Serial.println(lap_text);  // Debug output
                
                // Play beep sound if enabled
                if (g_stopwatch.sound_enabled) {
                    play_alarm_sound(SOUND_BEEP, 1);  // Single beep for lap
                }
            }
        } else {
            // Reset
            g_stopwatch.elapsed_time = 0;
            g_stopwatch.lap_count = 0;
            lv_obj_clean(list);
        }
    }, LV_EVENT_CLICKED, lap_list);
    
    y_pos += 310;
    
    // Set content height for scrolling
    lv_obj_set_height(parent, y_pos);
    
    // Update timer for stopwatch
    static lv_timer_t *update_timer = nullptr;
    if (update_timer) lv_timer_del(update_timer);
    update_timer = lv_timer_create([](lv_timer_t *timer) {
        lv_obj_t *label = (lv_obj_t*)timer->user_data;
        if (g_stopwatch.running) {
            unsigned long elapsed = millis() - g_stopwatch.start_time;
            char time_str[16];
            sprintf(time_str, "%02lu:%02lu.%02lu", elapsed / 60000, (elapsed / 1000) % 60, (elapsed / 10) % 100);
            lv_label_set_text(label, time_str);
        }
    }, 50, time_label);
}

// ==================== TIMER TAB ====================
void create_timer_tab(lv_obj_t *parent) {
    static lv_obj_t *hour_roller, *min_roller, *sec_roller, *timer_label, *start_btn;
    
    // Enable scrolling
    lv_obj_set_scrollbar_mode(parent, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_scroll_dir(parent, LV_DIR_VER);
    
    int y_pos = 0;
    
    // Label for setup
    lv_obj_t *setup_label = lv_label_create(parent);
    lv_label_set_text(setup_label, "Set Duration:");
    lv_obj_set_pos(setup_label, 65, y_pos);
    y_pos += 22;
    
    // Time selectors - BIGGER
    lv_obj_t *time_cont = lv_obj_create(parent);
    lv_obj_set_size(time_cont, 220, 80);
    lv_obj_set_pos(time_cont, 0, y_pos);
    lv_obj_set_style_pad_all(time_cont, 2, 0);
    lv_obj_clear_flag(time_cont, LV_OBJ_FLAG_SCROLLABLE);
    
    hour_roller = lv_roller_create(time_cont);
    lv_roller_set_options(hour_roller, "00\n01\n02\n03\n04\n05\n06\n07\n08\n09\n10\n11\n12", LV_ROLLER_MODE_NORMAL);
    lv_obj_set_size(hour_roller, 65, 74);
    lv_obj_align(hour_roller, LV_ALIGN_LEFT_MID, 2, 0);
    lv_roller_set_visible_row_count(hour_roller, 3);
    
    min_roller = lv_roller_create(time_cont);
    lv_roller_set_options(min_roller, "00\n05\n10\n15\n20\n25\n30\n35\n40\n45\n50\n55", LV_ROLLER_MODE_INFINITE);
    lv_obj_set_size(min_roller, 65, 74);
    lv_obj_center(min_roller);
    lv_roller_set_visible_row_count(min_roller, 3);
    
    sec_roller = lv_roller_create(time_cont);
    lv_roller_set_options(sec_roller, "00\n15\n30\n45", LV_ROLLER_MODE_INFINITE);
    lv_obj_set_size(sec_roller, 65, 74);
    lv_obj_align(sec_roller, LV_ALIGN_RIGHT_MID, -2, 0);
    lv_roller_set_visible_row_count(sec_roller, 3);
    
    y_pos += 85;
    
    // HH:MM:SS labels
    lv_obj_t *hms_label = lv_label_create(parent);
    lv_label_set_text(hms_label, "  HH        MM        SS");
    lv_obj_set_pos(hms_label, 20, y_pos);
    lv_obj_set_style_text_font(hms_label, &lv_font_montserrat_10, 0);
    y_pos += 22;
    
    // Sound toggle for timer
    lv_obj_t *timer_sound_toggle = lv_switch_create(parent);
    lv_obj_set_size(timer_sound_toggle, 50, 25);
    lv_obj_set_pos(timer_sound_toggle, 5, y_pos);
    if (g_timer.sound_enabled) lv_obj_add_state(timer_sound_toggle, LV_STATE_CHECKED);
    
    lv_obj_t *timer_sound_label = lv_label_create(parent);
    lv_label_set_text(timer_sound_label, "Sound ON");
    lv_obj_set_pos(timer_sound_label, 60, y_pos + 3);
    
    lv_obj_add_event_cb(timer_sound_toggle, [](lv_event_t *e) {
        g_timer.sound_enabled = lv_obj_has_state(lv_event_get_target(e), LV_STATE_CHECKED);
        Serial.printf("Timer sound: %s\n", g_timer.sound_enabled ? "ON" : "OFF");
    }, LV_EVENT_VALUE_CHANGED, NULL);
    
    y_pos += 32;
    
    // Timer display
    timer_label = lv_label_create(parent);
    lv_obj_set_style_text_font(timer_label, &lv_font_montserrat_48, 0);
    lv_label_set_text(timer_label, "00:00:00");
    lv_obj_set_pos(timer_label, 15, y_pos);
    y_pos += 58;
    
    // Start/Stop button
    start_btn = lv_btn_create(parent);
    lv_obj_set_size(start_btn, 160, 44);
    lv_obj_set_pos(start_btn, 30, y_pos);
    lv_obj_t *start_label = lv_label_create(start_btn);
    lv_label_set_text(start_label, "Start Timer");
    lv_obj_center(start_label);
    
    struct TimerRollers {
        lv_obj_t *hour;
        lv_obj_t *min;
        lv_obj_t *sec;
    };
    static TimerRollers rollers;
    rollers.hour = hour_roller;
    rollers.min = min_roller;
    rollers.sec = sec_roller;
    
    lv_obj_add_event_cb(start_btn, [](lv_event_t *e) {
        TimerRollers *rollers = (TimerRollers*)lv_event_get_user_data(e);
        lv_obj_t *btn = lv_event_get_target(e);
        lv_obj_t *label = lv_obj_get_child(btn, 0);
        
        if (!g_timer.running) {
            // Start timer
            uint16_t hours = lv_roller_get_selected(rollers->hour);
            uint16_t mins = lv_roller_get_selected(rollers->min) * 5;
            uint16_t secs = lv_roller_get_selected(rollers->sec) * 15;
            g_timer.duration_ms = (hours * 3600 + mins * 60 + secs) * 1000;
            
            if (g_timer.duration_ms > 0) {
                g_timer.running = true;
                g_timer.start_time = millis();
                lv_label_set_text(label, "Cancel");
                lv_obj_set_style_bg_color(btn, lv_color_hex(0xFF0000), 0);
            }
        } else {
            // Cancel timer
            g_timer.running = false;
            lv_label_set_text(label, "Start Timer");
            lv_obj_set_style_bg_color(btn, lv_color_hex(0x0000FF), 0);
        }
    }, LV_EVENT_CLICKED, &rollers);
    
    y_pos += 52;
    
    // Set content height for scrolling
    lv_obj_set_height(parent, y_pos);
    
    // Update timer
    static lv_timer_t *update_timer = nullptr;
    if (update_timer) lv_timer_del(update_timer);
    update_timer = lv_timer_create([](lv_timer_t *timer) {
        extern LilyGoLib watch;
        lv_obj_t *label = (lv_obj_t*)timer->user_data;
        if (g_timer.running) {
            unsigned long elapsed = millis() - g_timer.start_time;
            if (elapsed >= g_timer.duration_ms) {
                // Timer finished - BEEP AND VIBRATE!
                g_timer.running = false;
                
                // Strong vibration pattern
                watch.setWaveform(0, 47);  // Strong buzz
                watch.setWaveform(1, 47);  // Strong buzz again
                watch.setWaveform(2, 47);  // Strong buzz again
                watch.setWaveform(3, 0);
                watch.run();
                
                // Play alarm sound multiple times if enabled
                if (g_timer.sound_enabled) {
                    play_alarm_sound(g_timer.sound, 5);  // Play 5 times for timer
                }
                
                lv_label_set_text(label, "#FF0000 DONE!#");
                lv_label_set_recolor(label, true);
                
                Serial.println("TIMER FINISHED!");
            } else {
                unsigned long remaining = g_timer.duration_ms - elapsed;
                char time_str[16];
                sprintf(time_str, "%02lu:%02lu:%02lu", remaining / 3600000, (remaining / 60000) % 60, (remaining / 1000) % 60);
                lv_label_set_text(label, time_str);
            }
        }
    }, 100, timer_label);
}

// Initialize audio system for alarms
void init_alarm_audio() {
    if (alarm_audio_initialized) return;
    
    alarm_out = new AudioOutputI2S(1, AudioOutputI2S::EXTERNAL_I2S);
    alarm_out->SetPinout(BOARD_DAC_IIS_BCK, BOARD_DAC_IIS_WS, BOARD_DAC_IIS_DOUT);
    alarm_out->SetGain(2.0);  // High volume for alarms (10x louder than music 0.2)
    alarm_mp3 = new AudioGeneratorMP3();
    alarm_audio_initialized = true;
    Serial.println("Alarm audio initialized with gain 2.0");
}

// Play alarm sound using MP3 system (like app_music.cpp)
void play_alarm_sound(AlarmSound sound, int repeat_count) {
    extern LilyGoLib watch;
    
    // Vibrate first - stronger pattern
    watch.setWaveform(0, 47); // Strong buzz
    watch.setWaveform(1, 47); // Strong buzz
    watch.setWaveform(2, 47); // Strong buzz again
    watch.setWaveform(3, 0);
    watch.run();
    
    Serial.printf("Playing alarm sound: %d (repeat %d times)\n", sound, repeat_count);
    
    // Initialize audio if not done
    init_alarm_audio();
    
    // Select audio data based on sound type
    const unsigned char *audio_data = nullptr;
    size_t audio_size = 0;
    
    switch(sound) {
        case SOUND_VIBRATE_ONLY:
            // Already vibrated above
            return;
            
        case SOUND_BEEP:
            audio_data = boot_music;
            audio_size = sizeof(boot_music);
            break;
            
        case SOUND_RING:
            audio_data = mp3_ring_setup;
            audio_size = sizeof(mp3_ring_setup);
            break;
            
        case SOUND_MELODY:
            audio_data = mp3_array;
            audio_size = sizeof(mp3_array);
            break;
    }
    
    if (audio_data == nullptr) return;
    
    // Play sound multiple times for better audibility
    for (int i = 0; i < repeat_count; i++) {
        // Stop previous playback if any
        if (alarm_mp3 && alarm_mp3->isRunning()) {
            alarm_mp3->stop();
        }
        
        // Create new audio source
        if (alarm_file) delete alarm_file;
        alarm_file = new AudioFileSourcePROGMEM(audio_data, audio_size);
        
        // Start playback
        if (alarm_mp3->begin(alarm_file, alarm_out)) {
            Serial.printf("Playing sound iteration %d/%d\n", i+1, repeat_count);
            
            // Play until complete
            while (alarm_mp3->isRunning()) {
                if (!alarm_mp3->loop()) {
                    alarm_mp3->stop();
                    break;
                }
                delay(1);
            }
        }
        
        // Small delay between repetitions
        if (i < repeat_count - 1) {
            delay(300);
            
            // Additional vibration between sounds
            watch.setWaveform(0, 47);
            watch.setWaveform(1, 0);
            watch.run();
        }
    }
    
    Serial.println("Alarm sound playback completed");
}

void app_alarm_load(lv_obj_t *cont) {
    // Create tabview with 3 tabs - Extended by 20px for more room
    main_tabview = lv_tabview_create(cont, LV_DIR_TOP, 26);
    lv_obj_set_size(main_tabview, 240, 220);  // 220px height leaves 20px for back button
    lv_obj_align(main_tabview, LV_ALIGN_TOP_MID, 0, 0);
    
    // Create tabs
    lv_obj_t *tab_alarm = lv_tabview_add_tab(main_tabview, "Alarm");
    lv_obj_t *tab_stopwatch = lv_tabview_add_tab(main_tabview, "Stopwatch");
    lv_obj_t *tab_timer = lv_tabview_add_tab(main_tabview, "Timer");
    
    // Populate tabs
    create_alarm_tab(tab_alarm);
    create_stopwatch_tab(tab_stopwatch);
    create_timer_tab(tab_timer);
}

// Check alarm in main loop - call this from LilyGoGui.ino loop()
void check_alarm() {
    extern LilyGoLib watch;
    
    if (!g_alarm.enabled) return;
    
    RTC_DateTime now = watch.getDateTime();
    static uint8_t last_minute = 255;
    
    // Check once per minute
    if (now.minute == last_minute) return;
    last_minute = now.minute;
    
    if (now.hour == g_alarm.hour && now.minute == g_alarm.minute) {
        // Check recurrence
        bool should_trigger = false;
        switch (g_alarm.recurrence) {
            case RECUR_ONCE:
                should_trigger = true;
                g_alarm.enabled = false; // Disable after triggering
                break;
            case RECUR_DAILY:
                should_trigger = true;
                break;
            case RECUR_WEEKDAYS:
                should_trigger = (now.week >= 1 && now.week <= 5);
                break;
            case RECUR_WEEKENDS:
                should_trigger = (now.week == 0 || now.week == 6);
                break;
            case RECUR_CUSTOM:
                should_trigger = g_alarm.days[now.week];
                break;
        }
        
        if (should_trigger) {
            if (g_alarm.sound_enabled) {
                play_alarm_sound(g_alarm.sound, 5);  // Repeat 5 times for actual alarm
            }
            Serial.println("ALARM TRIGGERED!");
        }
    }
}

app_t app_alarm = {
    .setup_func_cb = app_alarm_load,
    .exit_func_cb = nullptr,
    .user_data = nullptr,
};