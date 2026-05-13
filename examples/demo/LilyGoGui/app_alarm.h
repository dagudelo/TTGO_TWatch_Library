#pragma once

#include "app_typedef.h"
#include "lvgl.h"
#include <LittleFS.h>
#include <vector>

// --- Recurrence types ---
enum RecurrenceType : uint8_t {
    RECUR_ONCE          = 0,
    RECUR_DAILY         = 1,
    RECUR_WEEKDAYS      = 2,
    RECUR_WEEKENDS      = 3,
    RECUR_CUSTOM_DAYS   = 4,
    RECUR_SPECIFIC_DATE = 5
};

// --- Alarm sound types ---
enum AlarmSound : uint8_t {
    SOUND_VIBRATE_ONLY = 0,
    SOUND_BEEP         = 1,
    SOUND_RING         = 2,
    SOUND_MELODY       = 3
};

// --- Binary alarm record (20 bytes, packed) ---
struct AlarmData {
    uint16_t id;
    uint8_t  flags;
    uint8_t  hour;
    uint8_t  minute;
    uint8_t  days_mask;
    uint8_t  target_month;
    uint8_t  target_day;
    uint8_t  sound_type;
    uint8_t  _pad[11];

    inline bool     enabled()       const { return flags & 0x01; }
    inline bool     sound_on()      const { return flags & 0x02; }
    inline bool     vibrate()       const { return flags & 0x04; }
    inline uint8_t  recurrence()    const { return (flags >> 3) & 0x07; }

    inline void set_enabled(bool v)       { v ? (flags |= 1) : (flags &= ~1); }
    inline void set_sound_on(bool v)      { v ? (flags |= 2) : (flags &= ~2); }
    inline void set_vibrate(bool v)       { v ? (flags |= 4) : (flags &= ~4); }
    inline void set_recurrence(uint8_t r) { flags = (flags & 0xC7) | ((r & 0x07) << 3); }
};

static_assert(sizeof(AlarmData) == 20, "AlarmData must be 20 bytes");

// --- Public API ---
extern app_t app_alarm;

void app_alarm_load(lv_obj_t *cont);
void check_alarm();
void check_timer();
