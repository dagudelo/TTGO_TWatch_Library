#include "app_alarm.h"
#include "app_typedef.h"
#include "lvgl.h"
#include <LilyGoLib.h>
#include <LittleFS.h>
#include <vector>

// --- Sound system ---
extern const unsigned char mp3_array[16509];
extern unsigned char mp3_ring_setup[86144];
extern const uint8_t boot_music[4365];

#include <AudioFileSourcePROGMEM.h>
#include <AudioGeneratorMP3.h>
#include <AudioOutputI2S.h>

static AudioFileSourcePROGMEM *alarm_file = nullptr;
static AudioGeneratorMP3 *alarm_mp3 = nullptr;
static AudioOutputI2S *alarm_out = nullptr;
static bool alarm_audio_ready = false;

// --- Global alarm state ---
static std::vector<AlarmData> g_alarms;
static uint16_t g_next_id = 1;

// --- Timer / stopwatch ---
struct StopwatchData {
    bool running; unsigned long start_time, elapsed_time;
    unsigned long lap_times[10]; int lap_count; bool sound_on;
};
struct TimerData {
    bool running; unsigned long duration_ms, start_time;
    uint8_t sound; bool sound_on;
};
static StopwatchData g_sw = {false,0,0,{0},0,true};
static TimerData     g_tmr = {false,0,0,SOUND_BEEP,true};
static bool timer_done = false;
static int  triggered_id = -1;

// ==================== LittleFS BINARY STORAGE ====================
#define ALARM_FILE  "/alarms.bin"

static void storage_save() {
    File f = LittleFS.open(ALARM_FILE, FILE_WRITE);
    if (!f) { Serial.println("Alarm save failed"); return; }
    uint32_t magic = 0x4D524C41; // "ALRM"
    uint16_t ver = 1, count = g_alarms.size();
    f.write((uint8_t*)&magic, 4);
    f.write((uint8_t*)&ver, 2);
    f.write((uint8_t*)&count, 2);
    for (auto &a : g_alarms) f.write((uint8_t*)&a, sizeof(AlarmData));
    f.close();
}

static void storage_load() {
    g_alarms.clear();
    if (!LittleFS.exists(ALARM_FILE)) return;
    File f = LittleFS.open(ALARM_FILE, FILE_READ);
    if (!f) return;
    uint32_t magic; uint16_t ver, count;
    f.read((uint8_t*)&magic, 4);
    if (magic != 0x4D524C41) { f.close(); return; }
    f.read((uint8_t*)&ver, 2);
    f.read((uint8_t*)&count, 2);
    for (int i = 0; i < count; i++) {
        AlarmData a;
        if (f.read((uint8_t*)&a, sizeof(AlarmData)) == sizeof(AlarmData)) {
            g_alarms.push_back(a);
            if (a.id >= g_next_id) g_next_id = a.id + 1;
        }
    }
    f.close();
}

static void alarm_add(uint8_t recur, uint8_t hour, uint8_t minute,
                      uint8_t days_mask, uint8_t month, uint8_t day,
                      uint8_t sound) {
    AlarmData a{};
    a.id = g_next_id++;
    a.set_enabled(true);
    a.set_sound_on(true);
    a.set_vibrate(true);
    a.set_recurrence(recur);
    a.hour = hour;
    a.minute = minute;
    a.days_mask = days_mask;
    a.target_month = month;
    a.target_day = day;
    a.sound_type = sound;
    g_alarms.push_back(a);
    storage_save();
}

static void alarm_toggle(int id) {
    for (auto &a : g_alarms)
        if (a.id == id) { a.set_enabled(!a.enabled()); storage_save(); return; }
}
static void alarm_delete(int id) {
    for (auto it = g_alarms.begin(); it != g_alarms.end(); ++it)
        if (it->id == id) { g_alarms.erase(it); storage_save(); return; }
}

// ==================== AUDIO HELPERS ====================
static void audio_init() {
    if (alarm_audio_ready) return;
    alarm_out = new AudioOutputI2S(1, AudioOutputI2S::EXTERNAL_I2S);
    alarm_out->SetPinout(BOARD_DAC_IIS_BCK, BOARD_DAC_IIS_WS, BOARD_DAC_IIS_DOUT);
    alarm_out->SetGain(2.0);
    alarm_mp3 = new AudioGeneratorMP3();
    alarm_audio_ready = true;
}

static void play_sound(uint8_t type) {
    extern LilyGoLib watch;
    watch.setWaveform(0,47); watch.setWaveform(1,47);
    watch.setWaveform(2,47); watch.setWaveform(3,0);
    watch.run();
    if (type == SOUND_VIBRATE_ONLY) return;
    audio_init();
    const uint8_t *src = nullptr; size_t sz = 0;
    switch (type) {
        case SOUND_BEEP:   src = boot_music;      sz = sizeof(boot_music);      break;
        case SOUND_RING:   src = mp3_ring_setup;   sz = sizeof(mp3_ring_setup);  break;
        case SOUND_MELODY: src = mp3_array;        sz = sizeof(mp3_array);       break;
        default: return;
    }
    for (int i = 0; i < 5; i++) {
        if (alarm_mp3->isRunning()) alarm_mp3->stop();
        if (alarm_file) delete alarm_file;
        alarm_file = new AudioFileSourcePROGMEM(src, sz);
        if (alarm_mp3->begin(alarm_file, alarm_out))
            while (alarm_mp3->isRunning())
                if (!alarm_mp3->loop()) { alarm_mp3->stop(); break; }
        if (i < 4) { delay(300); watch.setWaveform(0,47); watch.setWaveform(1,0); watch.run(); }
    }
}

// ==================== UI: ALARM LIST ====================
static lv_obj_t *list_cont = nullptr;

static void refresh_list();

static void create_alarm_list(lv_obj_t *parent) {
    lv_obj_set_scrollbar_mode(parent, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_scroll_dir(parent, LV_DIR_VER);

    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Alarms");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_pos(title, 85, 5);

    // Add button
    lv_obj_t *add_btn = lv_btn_create(parent);
    lv_obj_set_size(add_btn, 120, 32);
    lv_obj_set_pos(add_btn, 55, 32);
    lv_obj_t *add_lbl = lv_label_create(add_btn);
    lv_label_set_text(add_lbl, "+ Add Alarm");
    lv_obj_center(add_lbl);

    list_cont = lv_obj_create(parent);
    lv_obj_set_size(list_cont, 225, 155);
    lv_obj_set_pos(list_cont, 2, 70);
    lv_obj_set_scrollbar_mode(list_cont, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_scroll_dir(list_cont, LV_DIR_VER);
    lv_obj_clear_flag(list_cont, LV_OBJ_FLAG_SCROLL_ELASTIC);

    refresh_list();

    lv_obj_set_height(parent, 240);
}

static void refresh_list() {
    if (!list_cont) return;
    lv_obj_clean(list_cont);
    int y = 2;
    const char *recur_names[] = {"Once","Daily","Weekdays","Weekends","Custom","Date"};
    for (auto &a : g_alarms) {
        lv_obj_t *item = lv_obj_create(list_cont);
        lv_obj_set_size(item, 215, 48);
        lv_obj_set_pos(item, 2, y);
        lv_obj_clear_flag(item, LV_OBJ_FLAG_SCROLLABLE);

        char buf[16];
        sprintf(buf, "%02d:%02d", a.hour, a.minute);
        lv_obj_t *time_lbl = lv_label_create(item);
        lv_label_set_text(time_lbl, buf);
        lv_obj_set_style_text_font(time_lbl, &lv_font_montserrat_22, 0);
        lv_obj_set_pos(time_lbl, 4, 2);

        lv_obj_t *recur_lbl = lv_label_create(item);
        lv_label_set_text(recur_lbl, recur_names[a.recurrence()]);
        lv_obj_set_style_text_font(recur_lbl, &lv_font_montserrat_10, 0);
        lv_obj_set_pos(recur_lbl, 4, 26);

        lv_obj_t *sw = lv_switch_create(item);
        lv_obj_set_size(sw, 38, 20);
        lv_obj_set_pos(sw, 75, 5);
        if (a.enabled()) lv_obj_add_state(sw, LV_STATE_CHECKED);
        lv_obj_set_user_data(sw, (void*)(intptr_t)a.id);
        lv_obj_add_event_cb(sw, [](lv_event_t *e){
            alarm_toggle((int)(intptr_t)lv_obj_get_user_data(lv_event_get_target(e)));
            refresh_list();
        }, LV_EVENT_VALUE_CHANGED, nullptr);

        lv_obj_t *del = lv_btn_create(item);
        lv_obj_set_size(del, 50, 34);
        lv_obj_set_pos(del, 155, 7);
        lv_obj_t *dl = lv_label_create(del);
        lv_label_set_text(dl, "Del");
        lv_obj_center(dl);
        lv_obj_set_user_data(del, (void*)(intptr_t)a.id);
        lv_obj_add_event_cb(del, [](lv_event_t *e){
            alarm_delete((int)(intptr_t)lv_obj_get_user_data(lv_event_get_target(e)));
            refresh_list();
        }, LV_EVENT_CLICKED, nullptr);

        y += 50;
    }
    lv_obj_set_height(list_cont, y + 4);
}

// ==================== UI: ADD ALARM SCREEN ====================
static lv_obj_t *add_screen = nullptr;
static uint8_t add_recur = RECUR_CUSTOM_DAYS;
static uint8_t add_days_mask = 0x7F;
static uint8_t add_hour = 7, add_min = 0;
static uint8_t add_month = 1, add_day = 1;

static const char *day_labels[] = {"S","M","T","W","T","F","S"};
static lv_obj_t *day_btns[7];

static void update_day_btn_styles() {
    for (int i = 0; i < 7; i++) {
        if (day_btns[i]) {
            lv_obj_set_style_bg_color(day_btns[i],
                (add_days_mask & (1 << i)) ? lv_color_hex(0x2196F3) : lv_color_hex(0x555555), 0);
        }
    }
}

static void recur_btn_cb(lv_event_t *e) {
    if (e) add_recur = (uint8_t)(intptr_t)lv_event_get_user_data(e);
    lv_obj_t *day_row = (lv_obj_t*)lv_obj_get_user_data(add_screen);
    if (!day_row) return;
    lv_obj_t *hr = (lv_obj_t*)lv_obj_get_user_data(lv_obj_get_child(add_screen, 2));
    lv_obj_t *mn = (lv_obj_t*)lv_obj_get_user_data(lv_obj_get_child(add_screen, 3));
    lv_obj_t *mo = (lv_obj_t*)lv_obj_get_user_data(lv_obj_get_child(add_screen, 4));
    lv_obj_t *dy = (lv_obj_t*)lv_obj_get_user_data(lv_obj_get_child(add_screen, 5));

    bool week = (add_recur == RECUR_CUSTOM_DAYS);
    bool date = (add_recur == RECUR_SPECIFIC_DATE);

    week ? lv_obj_clear_flag(day_row, LV_OBJ_FLAG_HIDDEN) : lv_obj_add_flag(day_row, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(hr, LV_OBJ_FLAG_HIDDEN);
    (!week && !date) ? lv_obj_add_flag(mn, LV_OBJ_FLAG_HIDDEN) : lv_obj_clear_flag(mn, LV_OBJ_FLAG_HIDDEN);
    date ? lv_obj_clear_flag(mo, LV_OBJ_FLAG_HIDDEN) : lv_obj_add_flag(mo, LV_OBJ_FLAG_HIDDEN);
    date ? lv_obj_clear_flag(dy, LV_OBJ_FLAG_HIDDEN) : lv_obj_add_flag(dy, LV_OBJ_FLAG_HIDDEN);
}

static void show_add_screen() {
    if (add_screen) lv_obj_del(add_screen);
    add_screen = lv_obj_create(lv_layer_top());
    lv_obj_set_size(add_screen, 240, 240);
    lv_obj_set_style_bg_color(add_screen, lv_color_hex(0x111111), 0);
    lv_obj_set_style_pad_all(add_screen, 4, 0);

    lv_obj_t *ttl = lv_label_create(add_screen);
    lv_label_set_text(ttl, "New Alarm");
    lv_obj_set_style_text_font(ttl, &lv_font_montserrat_18, 0);
    lv_obj_set_pos(ttl, 65, 4);

    // Recurrence row
    lv_obj_t *recur_row = lv_obj_create(add_screen);
    lv_obj_set_size(recur_row, 232, 34);
    lv_obj_set_pos(recur_row, 2, 28);
    lv_obj_clear_flag(recur_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(recur_row, 2, 0);

    const char *rnames[] = {"Week","Day","Date"};
    uint8_t rvals[] = {RECUR_CUSTOM_DAYS, RECUR_DAILY, RECUR_SPECIFIC_DATE};
    for (int i = 0; i < 3; i++) {
        lv_obj_t *b = lv_btn_create(recur_row);
        lv_obj_set_size(b, 72, 28);
        lv_obj_set_pos(b, 2 + i*76, 2);
        lv_obj_t *l = lv_label_create(b);
        lv_label_set_text(l, rnames[i]);
        lv_obj_center(l);
        lv_obj_add_event_cb(b, recur_btn_cb, LV_EVENT_CLICKED, (void*)(intptr_t)rvals[i]);
    }

    // Day buttons row
    lv_obj_t *day_row = lv_obj_create(add_screen);
    lv_obj_set_size(day_row, 232, 32);
    lv_obj_set_pos(day_row, 2, 66);
    lv_obj_clear_flag(day_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(day_row, 1, 0);
    for (int i = 0; i < 7; i++) {
        day_btns[i] = lv_btn_create(day_row);
        lv_obj_set_size(day_btns[i], 29, 28);
        lv_obj_set_pos(day_btns[i], 1 + i*32, 1);
        lv_obj_t *lbl = lv_label_create(day_btns[i]);
        lv_label_set_text(lbl, day_labels[i]);
        lv_obj_center(lbl);
        uint8_t mask = 1 << i;
        lv_obj_add_event_cb(day_btns[i], [](lv_event_t *e){
            uint8_t m = (uint8_t)(intptr_t)lv_event_get_user_data(e);
            add_days_mask ^= m;
            update_day_btn_styles();
        }, LV_EVENT_CLICKED, (void*)(intptr_t)mask);
    }
    update_day_btn_styles();
    lv_obj_set_user_data(add_screen, day_row);

    // Hour roller
    lv_obj_t *hr = lv_roller_create(add_screen);
    lv_roller_set_options(hr, "00\n01\n02\n03\n04\n05\n06\n07\n08\n09\n10\n11\n12\n13\n14\n15\n16\n17\n18\n19\n20\n21\n22\n23", LV_ROLLER_MODE_INFINITE);
    lv_obj_set_size(hr, 70, 80);
    lv_obj_set_pos(hr, 5, 104);
    lv_roller_set_visible_row_count(hr, 3);
    lv_roller_set_selected(hr, 7, LV_ANIM_OFF);

    // Minute roller
    lv_obj_t *mn = lv_roller_create(add_screen);
    lv_roller_set_options(mn, "00\n05\n10\n15\n20\n25\n30\n35\n40\n45\n50\n55", LV_ROLLER_MODE_INFINITE);
    lv_obj_set_size(mn, 70, 80);
    lv_obj_set_pos(mn, 85, 104);
    lv_roller_set_visible_row_count(mn, 3);

    // Month roller (hidden by default)
    lv_obj_t *mo = lv_roller_create(add_screen);
    lv_roller_set_options(mo, "Jan\nFeb\nMar\nApr\nMay\nJun\nJul\nAug\nSep\nOct\nNov\nDec", LV_ROLLER_MODE_NORMAL);
    lv_obj_set_size(mo, 70, 80);
    lv_obj_set_pos(mo, 5, 104);
    lv_roller_set_visible_row_count(mo, 3);
    lv_obj_add_flag(mo, LV_OBJ_FLAG_HIDDEN);

    // Day roller (hidden by default)
    lv_obj_t *dy = lv_roller_create(add_screen);
    lv_roller_set_options(dy, "1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n11\n12\n13\n14\n15\n16\n17\n18\n19\n20\n21\n22\n23\n24\n25\n26\n27\n28\n29\n30\n31", LV_ROLLER_MODE_INFINITE);
    lv_obj_set_size(dy, 70, 80);
    lv_obj_set_pos(dy, 85, 104);
    lv_roller_set_visible_row_count(dy, 3);
    lv_obj_add_flag(dy, LV_OBJ_FLAG_HIDDEN);

    // Stash refs for visibility toggle
    lv_obj_set_user_data(lv_obj_get_child(add_screen, 2), hr);
    lv_obj_set_user_data(lv_obj_get_child(add_screen, 3), mn);
    lv_obj_set_user_data(lv_obj_get_child(add_screen, 4), mo);
    lv_obj_set_user_data(lv_obj_get_child(add_screen, 5), dy);

    // Sound dropdown
    lv_obj_t *snd = lv_dropdown_create(add_screen);
    lv_dropdown_set_options(snd, "Vibrate\nBeep\nRing\nMelody");
    lv_dropdown_set_selected(snd, SOUND_BEEP);
    lv_obj_set_size(snd, 140, 30);
    lv_obj_set_pos(snd, 45, 190);

    // Save button
    lv_obj_t *save = lv_btn_create(add_screen);
    lv_obj_set_size(save, 100, 32);
    lv_obj_set_pos(save, 65, 225);
    lv_obj_t *sv_lbl = lv_label_create(save);
    lv_label_set_text(sv_lbl, "Save");
    lv_obj_center(sv_lbl);
    lv_obj_set_user_data(save, snd);
    lv_obj_add_event_cb(save, [](lv_event_t *e){
        lv_obj_t *snd_dd = (lv_obj_t*)lv_event_get_user_data(e);
        uint8_t hour = lv_roller_get_selected(lv_obj_get_child(add_screen, 2));
        uint8_t min  = (add_recur == RECUR_DAILY) ? 0 : lv_roller_get_selected(lv_obj_get_child(add_screen, 3)) * 5;
        uint8_t month = (add_recur == RECUR_SPECIFIC_DATE) ? lv_roller_get_selected(lv_obj_get_child(add_screen, 4)) + 1 : 0;
        uint8_t day   = (add_recur == RECUR_SPECIFIC_DATE) ? lv_roller_get_selected(lv_obj_get_child(add_screen, 5)) + 1 : 0;
        uint8_t sound = lv_dropdown_get_selected(snd_dd);
        alarm_add(add_recur, hour, min, add_days_mask, month, day, sound);
        lv_obj_del(add_screen);
        add_screen = nullptr;
        refresh_list();
    }, LV_EVENT_CLICKED, save);

    // Cancel
    lv_obj_t *cancel = lv_btn_create(add_screen);
    lv_obj_set_size(cancel, 100, 32);
    lv_obj_set_pos(cancel, 65, 261);
    lv_obj_t *cn_lbl = lv_label_create(cancel);
    lv_label_set_text(cn_lbl, "Cancel");
    lv_obj_center(cn_lbl);
    lv_obj_add_event_cb(cancel, [](lv_event_t *e){
        lv_obj_del(add_screen);
        add_screen = nullptr;
    }, LV_EVENT_CLICKED, nullptr);

    recur_btn_cb(nullptr);
}

static void wire_add_button() {
    lv_obj_t *parent = lv_obj_get_parent(list_cont);
    lv_obj_t *add_btn = lv_obj_get_child(parent, 1);
    if (add_btn) {
        lv_obj_add_event_cb(add_btn, [](lv_event_t *e){
            show_add_screen();
        }, LV_EVENT_CLICKED, nullptr);
    }
}

// ==================== STOPWATCH TAB ====================
static void create_stopwatch_tab(lv_obj_t *parent) {
    lv_obj_set_scrollbar_mode(parent, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_scroll_dir(parent, LV_DIR_VER);

    lv_obj_t *time_lbl = lv_label_create(parent);
    lv_obj_set_style_text_font(time_lbl, &lv_font_montserrat_48, 0);
    lv_label_set_text(time_lbl, "00:00.00");
    lv_obj_set_pos(time_lbl, 25, 5);

    lv_obj_t *btn_cont = lv_obj_create(parent);
    lv_obj_set_size(btn_cont, 220, 44);
    lv_obj_set_pos(btn_cont, 5, 60);
    lv_obj_clear_flag(btn_cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(btn_cont, 2, 0);

    lv_obj_t *ss_btn = lv_btn_create(btn_cont);
    lv_obj_set_size(ss_btn, 102, 40);
    lv_obj_align(ss_btn, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_bg_color(ss_btn, lv_color_hex(0x0000FF), 0);
    lv_obj_t *ss_lbl = lv_label_create(ss_btn);
    lv_label_set_text(ss_lbl, "Start");
    lv_obj_center(ss_lbl);

    lv_obj_t *lr_btn = lv_btn_create(btn_cont);
    lv_obj_set_size(lr_btn, 102, 40);
    lv_obj_align(lr_btn, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_t *lr_lbl = lv_label_create(lr_btn);
    lv_label_set_text(lr_lbl, "Lap");
    lv_obj_center(lr_lbl);

    lv_obj_t *lap_list = lv_list_create(parent);
    lv_obj_set_size(lap_list, 220, 145);
    lv_obj_set_pos(lap_list, 5, 110);

    lv_obj_add_event_cb(ss_btn, [](lv_event_t *e){
        lv_obj_t *btn = lv_event_get_target(e);
        lv_obj_t *lbl = lv_obj_get_child(btn, 0);
        if (!g_sw.running) {
            g_sw.running = true;
            g_sw.start_time = millis() - g_sw.elapsed_time;
            lv_label_set_text(lbl, "Stop");
            lv_obj_set_style_bg_color(btn, lv_color_hex(0xFF0000), 0);
        } else {
            g_sw.running = false;
            g_sw.elapsed_time = millis() - g_sw.start_time;
            lv_label_set_text(lbl, "Start");
            lv_obj_set_style_bg_color(btn, lv_color_hex(0x0000FF), 0);
        }
    }, LV_EVENT_CLICKED, nullptr);

    lv_obj_add_event_cb(lr_btn, [](lv_event_t *e){
        lv_obj_t *list = (lv_obj_t*)lv_event_get_user_data(e);
        if (g_sw.running && g_sw.lap_count < 10) {
            g_sw.lap_times[g_sw.lap_count++] = millis() - g_sw.start_time;
            char buf[48];
            unsigned long t = g_sw.lap_times[g_sw.lap_count-1];
            sprintf(buf, "L%d: %02lu:%02lu.%02lu", g_sw.lap_count, t/60000, (t/1000)%60, (t/10)%100);
            lv_obj_t *btn = lv_list_add_btn(list, LV_SYMBOL_RIGHT, buf);
            lv_obj_scroll_to_view(btn, LV_ANIM_ON);
            if (g_sw.sound_on) play_sound(SOUND_BEEP);
        } else if (!g_sw.running) {
            g_sw.elapsed_time = 0; g_sw.lap_count = 0;
            lv_obj_clean(list);
        }
    }, LV_EVENT_CLICKED, lap_list);

    lv_obj_set_height(parent, 265);

    lv_timer_create([](lv_timer_t *t){
        lv_obj_t *lbl = (lv_obj_t*)t->user_data;
        if (g_sw.running) {
            unsigned long e = millis() - g_sw.start_time;
            char b[16]; sprintf(b, "%02lu:%02lu.%02lu", e/60000, (e/1000)%60, (e/10)%100);
            lv_label_set_text(lbl, b);
        }
    }, 50, time_lbl);
}

// ==================== TIMER TAB ====================
static void create_timer_tab(lv_obj_t *parent) {
    lv_obj_set_scrollbar_mode(parent, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_scroll_dir(parent, LV_DIR_VER);

    lv_obj_t *setup_lbl = lv_label_create(parent);
    lv_label_set_text(setup_lbl, "Set Duration:");
    lv_obj_set_pos(setup_lbl, 65, 2);

    lv_obj_t *time_cont = lv_obj_create(parent);
    lv_obj_set_size(time_cont, 220, 80);
    lv_obj_set_pos(time_cont, 0, 24);
    lv_obj_clear_flag(time_cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(time_cont, 2, 0);

    lv_obj_t *hr = lv_roller_create(time_cont);
    lv_roller_set_options(hr, "00\n01\n02\n03\n04\n05\n06\n07\n08\n09\n10\n11\n12", LV_ROLLER_MODE_NORMAL);
    lv_obj_set_size(hr, 65, 72);
    lv_obj_align(hr, LV_ALIGN_LEFT_MID, 2, 0);
    lv_roller_set_visible_row_count(hr, 3);

    lv_obj_t *mn = lv_roller_create(time_cont);
    lv_roller_set_options(mn, "00\n01\n02\n03\n04\n05\n06\n07\n08\n09\n10\n11\n12\n13\n14\n15\n16\n17\n18\n19\n20\n21\n22\n23\n24\n25\n26\n27\n28\n29\n30\n31\n32\n33\n34\n35\n36\n37\n38\n39\n40\n41\n42\n43\n44\n45\n46\n47\n48\n49\n50\n51\n52\n53\n54\n55\n56\n57\n58\n59", LV_ROLLER_MODE_INFINITE);
    lv_obj_set_size(mn, 65, 72);
    lv_obj_center(mn);
    lv_roller_set_visible_row_count(mn, 3);

    lv_obj_t *sc = lv_roller_create(time_cont);
    lv_roller_set_options(sc, "00\n15\n30\n45", LV_ROLLER_MODE_INFINITE);
    lv_obj_set_size(sc, 65, 72);
    lv_obj_align(sc, LV_ALIGN_RIGHT_MID, -2, 0);
    lv_roller_set_visible_row_count(sc, 3);

    lv_obj_t *hms = lv_label_create(parent);
    lv_label_set_text(hms, "  HH        MM        SS");
    lv_obj_set_pos(hms, 20, 108);
    lv_obj_set_style_text_font(hms, &lv_font_montserrat_10, 0);

    lv_obj_t *timer_lbl = lv_label_create(parent);
    lv_obj_set_style_text_font(timer_lbl, &lv_font_montserrat_48, 0);
    lv_label_set_text(timer_lbl, "00:00:00");
    lv_obj_set_pos(timer_lbl, 15, 125);

    lv_obj_t *start_btn = lv_btn_create(parent);
    lv_obj_set_size(start_btn, 160, 40);
    lv_obj_set_pos(start_btn, 35, 180);
    lv_obj_t *st_lbl = lv_label_create(start_btn);
    lv_label_set_text(st_lbl, "Start Timer");
    lv_obj_center(st_lbl);

    struct TR { lv_obj_t *h; lv_obj_t *m; lv_obj_t *s; };
    static TR rollers;
    rollers.h = hr; rollers.m = mn; rollers.s = sc;

    lv_obj_add_event_cb(start_btn, [](lv_event_t *e){
        TR *r = (TR*)lv_event_get_user_data(e);
        lv_obj_t *btn = lv_event_get_target(e);
        lv_obj_t *lbl = lv_obj_get_child(btn, 0);
        if (!g_tmr.running) {
            uint16_t h = lv_roller_get_selected(r->h);
            uint16_t m = lv_roller_get_selected(r->m);
            uint16_t s = lv_roller_get_selected(r->s) * 15;
            g_tmr.duration_ms = (h*3600 + m*60 + s) * 1000;
            if (g_tmr.duration_ms > 0) {
                g_tmr.running = true;
                g_tmr.start_time = millis();
                lv_label_set_text(lbl, "Cancel");
                lv_obj_set_style_bg_color(btn, lv_color_hex(0xFF0000), 0);
            }
        } else {
            g_tmr.running = false;
            lv_label_set_text(lbl, "Start Timer");
            lv_obj_set_style_bg_color(btn, lv_color_hex(0x0000FF), 0);
        }
    }, LV_EVENT_CLICKED, &rollers);

    lv_obj_set_height(parent, 230);

    lv_timer_create([](lv_timer_t *t){
        lv_obj_t *lbl = (lv_obj_t*)t->user_data;
        if (g_tmr.running) {
            unsigned long rem = g_tmr.duration_ms - (millis() - g_tmr.start_time);
            if ((long)rem <= 0) {
                g_tmr.running = false;
                timer_done = true;
                lv_label_set_text(lbl, "#FF0000 DONE!#");
                lv_label_set_recolor(lbl, true);
            } else {
                char b[16]; sprintf(b, "%02lu:%02lu:%02lu", rem/3600000, (rem/60000)%60, (rem/1000)%60);
                lv_label_set_text(lbl, b);
            }
        }
    }, 100, timer_lbl);
}

// ==================== ALARM TRIGGER ====================
void check_alarm() {
    extern LilyGoLib watch;
    if (triggered_id >= 0) {
        lv_disp_trig_activity(nullptr);
        watch.incrementalBrightness(128);
        for (auto &a : g_alarms) {
            if (a.id == triggered_id) {
                watch.setWaveform(0,47); watch.setWaveform(1,47);
                watch.setWaveform(2,47); watch.setWaveform(3,0);
                watch.run();
                if (a.sound_on()) play_sound(a.sound_type);
                if (a.recurrence() == RECUR_ONCE || a.recurrence() == RECUR_SPECIFIC_DATE) {
                    a.set_enabled(false);
                    storage_save();
                }
                break;
            }
        }
        triggered_id = -1;
        refresh_list();
        return;
    }

    RTC_DateTime now = watch.getDateTime();
    static uint8_t last_min = 255;
    if (now.minute == last_min) return;
    last_min = now.minute;

    for (auto &a : g_alarms) {
        if (!a.enabled()) continue;
        if (a.hour != now.hour || a.minute != now.minute) continue;
        bool fire = false;
        switch (a.recurrence()) {
            case RECUR_ONCE:          fire = true; break;
            case RECUR_DAILY:         fire = true; break;
            case RECUR_WEEKDAYS:      fire = (now.week >= 1 && now.week <= 5); break;
            case RECUR_WEEKENDS:      fire = (now.week == 0 || now.week == 6); break;
            case RECUR_CUSTOM_DAYS:   fire = (a.days_mask & (1 << now.week)); break;
            case RECUR_SPECIFIC_DATE: fire = (a.target_month == now.month && a.target_day == now.day); break;
        }
        if (fire) { triggered_id = a.id; return; }
    }
}

void check_timer() {
    extern LilyGoLib watch;
    if (!timer_done) return;
    timer_done = false;
    lv_disp_trig_activity(nullptr);
    watch.incrementalBrightness(128);
    watch.setWaveform(0,47); watch.setWaveform(1,47);
    watch.setWaveform(2,47); watch.setWaveform(3,0);
    watch.run();
    if (g_tmr.sound_on) play_sound(g_tmr.sound);
}

// ==================== MAIN ENTRY ====================
void app_alarm_load(lv_obj_t *cont) {
    storage_load();
    lv_obj_t *tv = lv_tabview_create(cont, LV_DIR_TOP, 26);
    lv_obj_set_size(tv, 240, 220);
    lv_obj_align(tv, LV_ALIGN_TOP_MID, 0, 0);

    lv_obj_t *tab_a = lv_tabview_add_tab(tv, "Alarm");
    lv_obj_t *tab_s = lv_tabview_add_tab(tv, "Stopwatch");
    lv_obj_t *tab_t = lv_tabview_add_tab(tv, "Timer");

    create_alarm_list(tab_a);
    wire_add_button();
    create_stopwatch_tab(tab_s);
    create_timer_tab(tab_t);
}

app_t app_alarm = {
    .setup_func_cb = app_alarm_load,
    .exit_func_cb = nullptr,
    .user_data = nullptr,
};
