#include "app_alarm.h"
#include "app_typedef.h"
#include "lvgl.h"
#include <LilyGoLib.h>
#include <AudioFileSourcePROGMEM.h>
#include <AudioGeneratorMP3.h>
#include <AudioOutputI2S.h>
#include <SD_MMC.h>
#include <vector>

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

// Alarm data structure - now with unique ID for storage
struct AlarmData {
    int id;  // Unique alarm ID
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
static std::vector<AlarmData> g_alarms;  // List of all alarms
static int next_alarm_id = 1;  // Counter for unique alarm IDs
static StopwatchData g_stopwatch = {false, 0, 0, {0}, 0, true};
static TimerData g_timer = {false, 0, 0, SOUND_BEEP, true};
static bool timer_just_completed = false;  // Flag to track if timer completed while screen was off
static int triggered_alarm_id = -1;  // ID of alarm that just triggered

// UI objects
static lv_obj_t *main_tabview = nullptr;
static lv_style_t style_frameless;
SensorPCF8563 rtc;

// Forward declarations
void create_alarm_tab(lv_obj_t *parent);
void create_stopwatch_tab(lv_obj_t *parent);
void create_timer_tab(lv_obj_t *parent);
void load_alarms_from_storage();
void save_alarms_to_storage();
void add_new_alarm(uint8_t hour, uint8_t minute, RecurrenceType recur, bool days[7], AlarmSound sound, bool sound_enabled);
void delete_alarm(int alarm_id);
void refresh_alarm_list_ui();
void init_alarm_audio();
void play_alarm_sound(AlarmSound sound, int repeat_count = 3);

#define ALARM_STORAGE_FILE "/alarms.txt"

// ==================== PERSISTENT STORAGE ====================
void load_alarms_from_storage() {
    g_alarms.clear();
    
    File file = SD_MMC.open(ALARM_STORAGE_FILE, FILE_READ);
    if (!file) {
        Serial.println("No alarm file found, starting fresh");
        return;
    }
    
    while (file.available()) {
        String line = file.readStringUntil('\n');
        line.trim();
        if (line.length() == 0) continue;
        
        // Parse format: id,enabled,hour,minute,recur,days[7],sound,sound_enabled
        int idx = 0;
        AlarmData alarm;
        alarm.id = line.substring(idx, line.indexOf(',', idx)).toInt();
        idx = line.indexOf(',', idx) + 1;
        
        alarm.enabled = line.substring(idx, line.indexOf(',', idx)).toInt();
        idx = line.indexOf(',', idx) + 1;
        
        alarm.hour = line.substring(idx, line.indexOf(',', idx)).toInt();
        idx = line.indexOf(',', idx) + 1;
        
        alarm.minute = line.substring(idx, line.indexOf(',', idx)).toInt();
        idx = line.indexOf(',', idx) + 1;
        
        alarm.recurrence = (RecurrenceType)line.substring(idx, line.indexOf(',', idx)).toInt();
        idx = line.indexOf(',', idx) + 1;
        
        for (int i = 0; i < 7; i++) {
            alarm.days[i] = line.substring(idx, line.indexOf(',', idx)).toInt();
            idx = line.indexOf(',', idx) + 1;
        }
        
        alarm.sound = (AlarmSound)line.substring(idx, line.indexOf(',', idx)).toInt();
        idx = line.indexOf(',', idx) + 1;
        
        alarm.sound_enabled = line.substring(idx).toInt();
        alarm.vibrate = true;  // Always vibrate
        
        g_alarms.push_back(alarm);
        if (alarm.id >= next_alarm_id) {
            next_alarm_id = alarm.id + 1;
        }
    }
    
    file.close();
    Serial.printf("Loaded %d alarms from storage\n", g_alarms.size());
}

void save_alarms_to_storage() {
    File file = SD_MMC.open(ALARM_STORAGE_FILE, FILE_WRITE);
    if (!file) {
        Serial.println("Failed to open alarm file for writing");
        return;
    }
    
    for (const auto& alarm : g_alarms) {
        file.printf("%d,%d,%d,%d,%d,", alarm.id, alarm.enabled, alarm.hour, alarm.minute, alarm.recurrence);
        for (int i = 0; i < 7; i++) {
            file.printf("%d,", alarm.days[i]);
        }
        file.printf("%d,%d\n", alarm.sound, alarm.sound_enabled);
    }
    
    file.close();
    Serial.printf("Saved %d alarms to storage\n", g_alarms.size());
}

void add_new_alarm(uint8_t hour, uint8_t minute, RecurrenceType recur, bool days[7], AlarmSound sound, bool sound_enabled) {
    AlarmData new_alarm;
    new_alarm.id = next_alarm_id++;
    new_alarm.enabled = true;
    new_alarm.hour = hour;
    new_alarm.minute = minute;
    new_alarm.recurrence = recur;
    for (int i = 0; i < 7; i++) {
        new_alarm.days[i] = days[i];
    }
    new_alarm.sound = sound;
    new_alarm.vibrate = true;
    new_alarm.sound_enabled = sound_enabled;
    
    g_alarms.push_back(new_alarm);
    save_alarms_to_storage();
    Serial.printf("Added alarm #%d for %02d:%02d\n", new_alarm.id, hour, minute);
}

void delete_alarm(int alarm_id) {
    for (auto it = g_alarms.begin(); it != g_alarms.end(); ++it) {
        if (it->id == alarm_id) {
            Serial.printf("Deleting alarm #%d\n", alarm_id);
            g_alarms.erase(it);
            save_alarms_to_storage();
            return;
        }
    }
}

void toggle_alarm(int alarm_id) {
    for (auto& alarm : g_alarms) {
        if (alarm.id == alarm_id) {
            alarm.enabled = !alarm.enabled;
            save_alarms_to_storage();
            Serial.printf("Alarm #%d %s\n", alarm_id, alarm.enabled ? "enabled" : "disabled");
            return;
        }
    }
}

// ==================== ALARM TAB ====================
static lv_obj_t *alarm_list_container = nullptr;
static lv_obj_t *parent_alarm_tab = nullptr;

void refresh_alarm_list_ui() {
    if (!alarm_list_container || !parent_alarm_tab) return;
    
    // Clear existing list
    lv_obj_clean(alarm_list_container);
    
    int y_pos = 5;
    
    // Show all alarms
    for (const auto& alarm : g_alarms) {
        lv_obj_t *alarm_item = lv_obj_create(alarm_list_container);
        lv_obj_set_size(alarm_item, 220, 50);
        lv_obj_set_pos(alarm_item, 0, y_pos);
        lv_obj_clear_flag(alarm_item, LV_OBJ_FLAG_SCROLLABLE);
        
        // Time label (large)
        lv_obj_t *time_label = lv_label_create(alarm_item);
        char time_str[10];
        sprintf(time_str, "%02d:%02d", alarm.hour, alarm.minute);
        lv_label_set_text(time_label, time_str);
        lv_obj_set_style_text_font(time_label, &lv_font_montserrat_24, 0);
        lv_obj_set_pos(time_label, 5, 5);
        
        // Recurrence info (small)
        lv_obj_t *recur_label = lv_label_create(alarm_item);
        const char *recur_text[] = {"Once", "Daily", "Weekdays", "Weekends", "Custom"};
        lv_label_set_text(recur_label, recur_text[alarm.recurrence]);
        lv_obj_set_style_text_font(recur_label, &lv_font_montserrat_10, 0);
        lv_obj_set_pos(recur_label, 5, 30);
        
        // Enable/Disable switch
        lv_obj_t *toggle = lv_switch_create(alarm_item);
        lv_obj_set_size(toggle, 40, 20);
        lv_obj_set_pos(toggle, 90, 8);
        if (alarm.enabled) lv_obj_add_state(toggle, LV_STATE_CHECKED);
        lv_obj_set_user_data(toggle, (void*)(intptr_t)alarm.id);
        lv_obj_add_event_cb(toggle, [](lv_event_t *e) {
            int id = (int)(intptr_t)lv_obj_get_user_data(lv_event_get_target(e));
            toggle_alarm(id);
        }, LV_EVENT_VALUE_CHANGED, NULL);
        
        // Delete button
        lv_obj_t *del_btn = lv_btn_create(alarm_item);
        lv_obj_set_size(del_btn, 50, 36);
        lv_obj_set_pos(del_btn, 160, 7);
        lv_obj_t *del_label = lv_label_create(del_btn);
        lv_label_set_text(del_label, "Del");
        lv_obj_center(del_label);
        lv_obj_set_user_data(del_btn, (void*)(intptr_t)alarm.id);
        lv_obj_add_event_cb(del_btn, [](lv_event_t *e) {
            int id = (int)(intptr_t)lv_obj_get_user_data(lv_event_get_target(e));
            delete_alarm(id);
            refresh_alarm_list_ui();
        }, LV_EVENT_CLICKED, NULL);
        
        y_pos += 55;
    }
    
    // Update container height
    lv_obj_set_height(alarm_list_container, y_pos + 10);
}

void create_alarm_tab(lv_obj_t *parent) {
    parent_alarm_tab = parent;
    
    // Create scrollable container
    lv_obj_set_scrollbar_mode(parent, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_scroll_dir(parent, LV_DIR_VER);
    
    int y_pos = 0;
    
    // Title
    lv_obj_t *title_label = lv_label_create(parent);
    lv_label_set_text(title_label, "Alarms");
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_20, 0);
    lv_obj_set_pos(title_label, 80, y_pos);
    y_pos += 28;
    
    // Quick Add section title
    lv_obj_t *quick_title = lv_label_create(parent);
    lv_label_set_text(quick_title, "Quick Add:");
    lv_obj_set_style_text_font(quick_title, &lv_font_montserrat_12, 0);
    lv_obj_set_pos(quick_title, 5, y_pos);
    y_pos += 18;
    
    // Quick-add button 1: Morning Alarm (7:00 AM Daily)
    lv_obj_t *add_btn1 = lv_btn_create(parent);
    lv_obj_set_size(add_btn1, 105, 32);
    lv_obj_set_pos(add_btn1, 0, y_pos);
    lv_obj_t *add_label1 = lv_label_create(add_btn1);
    lv_label_set_text(add_label1, "7:00 AM");
    lv_obj_set_style_text_font(add_label1, &lv_font_montserrat_10, 0);
    lv_obj_center(add_label1);
    lv_obj_add_event_cb(add_btn1, [](lv_event_t *e) {
        bool days[7] = {false};
        add_new_alarm(7, 0, RECUR_DAILY, days, SOUND_BEEP, true);
        refresh_alarm_list_ui();
    }, LV_EVENT_CLICKED, NULL);
    
    // Quick-add button 2: Work Alarm (8:30 AM Weekdays)
    lv_obj_t *add_btn2 = lv_btn_create(parent);
    lv_obj_set_size(add_btn2, 105, 32);
    lv_obj_set_pos(add_btn2, 115, y_pos);
    lv_obj_t *add_label2 = lv_label_create(add_btn2);
    lv_label_set_text(add_label2, "8:30 Work");
    lv_obj_set_style_text_font(add_label2, &lv_font_montserrat_10, 0);
    lv_obj_center(add_label2);
    lv_obj_add_event_cb(add_btn2, [](lv_event_t *e) {
        bool days[7] = {false};
        add_new_alarm(8, 30, RECUR_WEEKDAYS, days, SOUND_RING, true);
        refresh_alarm_list_ui();
    }, LV_EVENT_CLICKED, NULL);
    
    y_pos += 36;
    
    // Quick-add button 3: Afternoon Reminder (12:00 PM Daily)
    lv_obj_t *add_btn3 = lv_btn_create(parent);
    lv_obj_set_size(add_btn3, 105, 32);
    lv_obj_set_pos(add_btn3, 0, y_pos);
    lv_obj_t *add_label3 = lv_label_create(add_btn3);
    lv_label_set_text(add_label3, "12:00 PM");
    lv_obj_set_style_text_font(add_label3, &lv_font_montserrat_10, 0);
    lv_obj_center(add_label3);
    lv_obj_add_event_cb(add_btn3, [](lv_event_t *e) {
        bool days[7] = {false};
        add_new_alarm(12, 0, RECUR_DAILY, days, SOUND_BEEP, true);
        refresh_alarm_list_ui();
    }, LV_EVENT_CLICKED, NULL);
    
    // Quick-add button 4: Evening Reminder (6:00 PM Daily)
    lv_obj_t *add_btn4 = lv_btn_create(parent);
    lv_obj_set_size(add_btn4, 105, 32);
    lv_obj_set_pos(add_btn4, 115, y_pos);
    lv_obj_t *add_label4 = lv_label_create(add_btn4);
    lv_label_set_text(add_label4, "6:00 PM");
    lv_obj_set_style_text_font(add_label4, &lv_font_montserrat_10, 0);
    lv_obj_center(add_label4);
    lv_obj_add_event_cb(add_btn4, [](lv_event_t *e) {
        bool days[7] = {false};
        add_new_alarm(18, 0, RECUR_DAILY, days, SOUND_BEEP, true);
        refresh_alarm_list_ui();
    }, LV_EVENT_CLICKED, NULL);
    
    y_pos += 36;
    
    // Quick-add button 5: Bedtime Reminder (10:00 PM Daily)
    lv_obj_t *add_btn5 = lv_btn_create(parent);
    lv_obj_set_size(add_btn5, 105, 32);
    lv_obj_set_pos(add_btn5, 0, y_pos);
    lv_obj_t *add_label5 = lv_label_create(add_btn5);
    lv_label_set_text(add_label5, "10:00 PM");
    lv_obj_set_style_text_font(add_label5, &lv_font_montserrat_10, 0);
    lv_obj_center(add_label5);
    lv_obj_add_event_cb(add_btn5, [](lv_event_t *e) {
        bool days[7] = {false};
        add_new_alarm(22, 0, RECUR_DAILY, days, SOUND_BEEP, true);
        refresh_alarm_list_ui();
    }, LV_EVENT_CLICKED, NULL);
    
    // Quick-add button 6: Weekend Alarm (9:00 AM Weekends)
    lv_obj_t *add_btn6 = lv_btn_create(parent);
    lv_obj_set_size(add_btn6, 105, 32);
    lv_obj_set_pos(add_btn6, 115, y_pos);
    lv_obj_t *add_label6 = lv_label_create(add_btn6);
    lv_label_set_text(add_label6, "9:00 Weekend");
    lv_obj_set_style_text_font(add_label6, &lv_font_montserrat_10, 0);
    lv_obj_center(add_label6);
    lv_obj_add_event_cb(add_btn6, [](lv_event_t *e) {
        bool days[7] = {false};
        add_new_alarm(9, 0, RECUR_WEEKENDS, days, SOUND_BEEP, true);
        refresh_alarm_list_ui();
    }, LV_EVENT_CLICKED, NULL);
    
    y_pos += 40;
    
    // Alarm list container
    alarm_list_container = lv_obj_create(parent);
    lv_obj_set_size(alarm_list_container, 220, 130);  // Fixed height for list
    lv_obj_set_pos(alarm_list_container, 0, y_pos);
    lv_obj_set_scrollbar_mode(alarm_list_container, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_scroll_dir(alarm_list_container, LV_DIR_VER);
    
    // Populate alarm list
    refresh_alarm_list_ui();
    
    y_pos += 135;
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
    lv_roller_set_options(min_roller, "00\n01\n02\n03\n04\n05\n06\n07\n08\n09\n10\n11\n12\n13\n14\n15\n16\n17\n18\n19\n20\n21\n22\n23\n24\n25\n26\n27\n28\n29\n30\n31\n32\n33\n34\n35\n36\n37\n38\n39\n40\n41\n42\n43\n44\n45\n46\n47\n48\n49\n50\n51\n52\n53\n54\n55\n56\n57\n58\n59", LV_ROLLER_MODE_INFINITE);
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
            uint16_t mins = lv_roller_get_selected(rollers->min);  // Now single minutes, not * 5
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
        lv_obj_t *label = (lv_obj_t*)timer->user_data;
        if (g_timer.running) {
            unsigned long elapsed = millis() - g_timer.start_time;
            if (elapsed >= g_timer.duration_ms) {
                // Timer finished - mark as completed
                g_timer.running = false;
                timer_just_completed = true;  // Set flag for check_timer to handle wake-up
                
                lv_label_set_text(label, "#FF0000 DONE!#");
                lv_label_set_recolor(label, true);
                
                Serial.println("TIMER FINISHED! Flagged for screen wake-up.");
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
    // Load alarms from persistent storage
    load_alarms_from_storage();
    
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

// Check timer completion in main loop - wakes screen when timer finishes
void check_timer() {
    extern LilyGoLib watch;
    
    // Check if timer just completed
    if (timer_just_completed) {
        timer_just_completed = false;  // Reset flag
        
        Serial.println("Timer completed - waking screen and triggering alert!");
        
        // Wake up the screen
        lv_disp_trig_activity(NULL);
        
        // Set brightness to visible level
        watch.incrementalBrightness(128);  // Medium brightness
        
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
        
        // Note: The timer tab will already show "DONE!" from the LVGL timer update
    }
}

// Check alarm in main loop - call this from LilyGoGui.ino loop()
void check_alarm() {
    extern LilyGoLib watch;
    
    // First, check if an alarm just triggered (wake up screen)
    if (triggered_alarm_id >= 0) {
        Serial.printf("Alarm #%d triggered - waking screen!\n", triggered_alarm_id);
        
        // Wake up the screen
        lv_disp_trig_activity(NULL);
        watch.incrementalBrightness(128);  // Medium brightness
        
        // Find the alarm
        for (auto& alarm : g_alarms) {
            if (alarm.id == triggered_alarm_id) {
                // Strong vibration
                watch.setWaveform(0, 47);
                watch.setWaveform(1, 47);
                watch.setWaveform(2, 47);
                watch.setWaveform(3, 0);
                watch.run();
                
                // Play sound if enabled
                if (alarm.sound_enabled) {
                    play_alarm_sound(alarm.sound, 5);
                }
                
                // If once, disable it
                if (alarm.recurrence == RECUR_ONCE) {
                    alarm.enabled = false;
                    save_alarms_to_storage();
                }
                break;
            }
        }
        
        triggered_alarm_id = -1;  // Reset flag
        refresh_alarm_list_ui();  // Update UI
        return;
    }
    
    // Check all enabled alarms
    RTC_DateTime now = watch.getDateTime();
    static uint8_t last_minute = 255;
    
    // Check once per minute
    if (now.minute == last_minute) return;
    last_minute = now.minute;
    
    for (auto& alarm : g_alarms) {
        if (!alarm.enabled) continue;
        if (alarm.hour != now.hour || alarm.minute != now.minute) continue;
        
        // Check recurrence
        bool should_trigger = false;
        switch (alarm.recurrence) {
            case RECUR_ONCE:
                should_trigger = true;
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
                should_trigger = alarm.days[now.week];
                break;
        }
        
        if (should_trigger) {
            triggered_alarm_id = alarm.id;  // Set flag for wake-up
            Serial.printf("Alarm #%d should trigger!\n", alarm.id);
            return;  // Handle in next loop iteration
        }
    }
}

app_t app_alarm = {
    .setup_func_cb = app_alarm_load,
    .exit_func_cb = nullptr,
    .user_data = nullptr,
};