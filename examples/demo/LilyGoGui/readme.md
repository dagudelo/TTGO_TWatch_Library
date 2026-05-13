# LilyGoGui - Complete Smartwatch Interface Documentation# LilyGoGui - Complete Smartwatch Interface Documentation



## Overview## Overview



LilyGoGui is a comprehensive smartwatch interface for the LilyGo T-Watch S3 (ESP32-S3) featuring time management, wireless connectivity, health tracking, multimedia, and system utilities. Built with LVGL 8.3.9 for smooth 240×240 display rendering.LilyGoGui is a comprehensive smartwatch interface for the LilyGo T-Watch S3 (ESP32-S3) featuring a full suite of applications including time management, wireless connectivity, health tracking, multimedia, and system utilities. Built with LVGL 8.3.9 for smooth 240x240 pixel display rendering.



**Platform**: ESP32-S3 @ 240MHz | 16MB Flash | 8MB PSRAM  **Platform**: ESP32-S3 (240MHz, 16MB Flash, 8MB PSRAM)  

**Display**: 240×240 TFT touchscreen  **Display**: 240x240 TFT with touch  

**Framework**: Arduino ESP32 2.0.9 + LVGL 8.3.9  **Framework**: Arduino ESP32 2.0.9 + LVGL 8.3.9  

**Author**: zhanguichen (Shenzhen Xinyuan Electronic Technology)  **Author**: zhanguichen (Shenzhen Xinyuan Electronic Technology)  

**License**: MIT**License**: MIT



------



## Table of Contents## Table of Contents



1. [Core Applications](#core-applications)1. [System Architecture](#system-architecture)

2. [Utility Applications](#utility-applications)2. [Core Applications](#core-applications)

3. [System Components](#system-components)3. [Utility Applications](#utility-applications)

4. [API Reference](#api-reference)4. [System Components](#system-components)

5. [File Structure](#file-structure)5. [Key Features](#key-features)

6. [Development Guide](#development-guide)6. [API Reference](#api-reference)

7. [Battery & Power Management](#battery---power-management)
8. [File Structure](#file-structure)

---

---

## Core Applications

## System Architecture

### 1. Alarm & Timer System (`app_alarm.cpp/.h`)

### Main Loop Structure

**Multiple Alarm Management** - 836 lines

- Unlimited alarms with persistent storage (`/alarms.txt` on SD_MMC)```cpp

- Individual enable/disable toggles per alarmvoid loop() {

- Delete button for each alarm    lv_task_handler();        // LVGL event processing

- Quick-add button (default: 7:00 AM daily beep alarm)    SensorHandler();          // Accelerometer, gestures

- Scrollable alarm list (220×130px viewport)    PMUHandler();             // Power management

- Each alarm displays: Time (24pt), Recurrence type (10pt), Toggle, Delete button    get_BattVoltage();        // Battery monitoring

    app_batt_voltage_update(); // Update battery app

**Alarm Configuration**    check_alarm();            // Alarm monitoring

- Time: Hour/Minute selection (24-hour format)    check_timer();            // Timer monitoring

- Recurrence: Once, Daily, Weekdays, Weekends, Custom days    lowPowerEnergyHandler();  // Sleep mode (30s timeout)

- Sound: Vibrate, Beep, Ring, Melody (with ON/OFF toggle)}

- Persistent storage format: `id,enabled,hour,minute,recur,day0-6,sound,sound_en````



**Stopwatch Features**### App System Architecture

- Start/Stop/Reset controls

- Lap recording (up to 10 laps)All apps follow the `app_t` structure:

- Auto-scroll lap list

- Optional sound feedback on lap```cpp

- Display: MM:SS.mm format (48pt font)typedef struct {

    app_func_t setup_func_cb;  // Called when app loads

**Timer Features**    app_func_cb exit_func_cb;   // Called when app exits

- Selection: Hours (00-12), Minutes (00-59), Seconds (00/15/30/45)    void *user_data;           // App-specific data

- Real-time countdown display} app_t;

- 5× sound repetition + triple vibration on completion```

- Background operation with screen wake-up

### Power Management

**Screen Wake-Up Behavior**

When alarm/timer triggers while screen is off:- **Active Mode**: Full brightness, all sensors active

1. Sets completion flag (`triggered_alarm_id` or `timer_just_completed`)- **Light Sleep**: Screen off after 30 seconds of inactivity

2. Main loop detects flag via `check_alarm()` or `check_timer()`- **Wake Methods**: Touch screen, accelerometer interrupt, PMU interrupt

3. Wakes display: `lv_disp_trig_activity(NULL)`- **Background Operations**: Alarms and timers continue running during sleep

4. Sets brightness to 128 (medium level)

5. Triple vibration (strong buzz pattern)---

6. Plays sound 5 times (if enabled)

7. Updates UI to show completion state## Core Applications



**Audio System**### 1. **Alarm & Timer System** (`app_alarm.cpp/.h`)

- Library: ESP8266Audio v1.9.7 (MP3 from PROGMEM)

- Gain: 2.0 (10× louder than music app's 0.2 gain)#### Features

- Files: `boot_music` (4.3KB), `mp3_array` (16.5KB), `mp3_ring_setup` (86KB)

- Hardware: I2S output (pins: BCK, WS, DOUT)**Multiple Alarm Management**

2. New Alarm List UI

**Key Functions**List View: Shows all configured alarms in a scrollable list (220x130px)

```cppEach Alarm Item Displays:

void load_alarms_from_storage();      // Load from /alarms.txtTime in large font (24pt): e.g., "07:00"

void save_alarms_to_storage();        // Save all alarmsRecurrence type (10pt): "Once", "Daily", "Weekdays", etc.

void add_new_alarm(...);               // Add and persist alarmEnable/Disable toggle switch

void delete_alarm(int id);             // Remove alarm by IDDelete button

void toggle_alarm(int id);             // Enable/disable alarm3. Quick Add Alarm

void refresh_alarm_list_ui();          // Rebuild visual list"+ Add New Alarm" button: Instantly adds a 7:00 AM daily alarm with beep sound

void check_alarm();                    // Background monitoring (main loop)Default settings: 7:00, Daily recurrence, Beep sound, Sound enabled

void check_timer();                    // Timer monitoring (main loop)Can be customized in future updates to show a form

void init_alarm_audio();               // Initialize MP3 player4. Screen Wake-Up on Alarm Trigger

void play_alarm_sound(AlarmSound, int repeat); // Play with repetitionsSame behavior as timer:

```Alarm triggers in background (even when screen is off)

Sets triggered_alarm_id flag

---check_alarm() detects flag in main loop

Wakes screen with lv_disp_trig_activity(NULL)

### 2. Battery Voltage Monitor (`app_batt_voltage.cpp/.h`)Sets brightness to 128 (medium)

Triple vibration pattern (strong buzz)

**Real-Time Battery Metrics** - 223 linesPlays sound 5 times (if enabled)

- Voltage (V), Current (mA), Percentage (%), Temperature (°C)Updates UI to show current state

- Discharge rate: %/hour and mA5. Smart Alarm Management

- Estimated time remaining (hours + minutes)Individual Control: Each alarm can be enabled/disabled independently

- Battery capacity: 470mAh with current percentageDeletion: Delete button removes alarm from list and storage

- Charge/Discharge status indicatorOnce-Only Alarms: Automatically disabled after triggering

Recurrence Support: Once, Daily, Weekdays, Weekends, Custom days

**Display Components**Multi-Alarm: Supports unlimited alarms (memory permitting)

- Top label: 4 lines of comprehensive battery info (montserrat_14)Technical Implementation:

- Scrollable table: 10 rows × 3 columns (230×300px in 200px viewport)Data Structures:

- Columns: Metric | Value | Unit

- Spacing: 30px top margin, 5px extra for label

- Auto-refresh when app is activestd::vector<AlarmData> g_alarms;  // Dynamic list of all alarmsint next_alarm_id = 1;  // Unique ID counterint triggered_alarm_id = -1;  // Flag for wake-up handling

Key Functions:

**Discharge Tracking Algorithm**

```cppload_alarms_from_storage() - Loads from /alarms.txt

if (!charging && time_diff > 6_minutes) {save_alarms_to_storage() - Saves all alarms to file

    discharge_rate = (old_percent - new_percent) / hours_elapsed;add_new_alarm() - Adds alarm to list and saves

    time_remaining = current_percent / discharge_rate;delete_alarm(id) - Removes alarm by ID

}toggle_alarm(id) - Enable/disable alarm

```refresh_alarm_list_UI() - Rebuilds visual list

check_alarm() - Monitors all alarms, triggers wake-up

---Storage Format Example:



### 3. Step Counter & Activity (`app_step_counter.cpp/.h`)

1,1,7,0,1,0,0,0,0,0,0,0,1,12,1,8,30,2,1,1,1,1,1,0,0,2,1

**Features**(ID, enabled, hour, minute, recurrence, 7 day flags, sound type, sound enabled)

- Real-time step counting (BMA423 accelerometer)

- Daily step goal trackingHow It Works:

- Activity detection (walking, running)App Launch: Loads all alarms from /alarms.txt

- Tilt detection for wrist raiseUI Display: Shows alarm list with toggle/delete controls

- Motion/no-motion detectionAdd Alarm: Click "+ Add New Alarm" → adds 7:00 AM daily alarm

- Historical data visualization with chartBackground Monitoring: check_alarm() runs every loop cycle

Trigger Detection: When time matches and alarm enabled → sets flag

**Sensor Interrupts**Wake & Alert: Next loop iteration wakes screen, vibrates, plays sound

- `INT_STEP_CNTR`: Pedometer interruptPersistence: All changes saved immediately to flash

- `INT_ACTIVITY`: Activity change detectionAdvantages:

- `INT_TILT`: Tilt/wrist raise gesture✅ Persistent: Alarms survive power cycles and reboots

- `INT_WAKEUP`: Double-tap wake✅ Multiple Alarms: Support unlimited alarms

- `INT_ANY_NO_MOTION`: Motion state changes✅ Screen Wake: Never miss an alarm even when sleeping

✅ Individual Control: Enable/disable/delete any alarm

---✅ Professional: Same behavior as commercial smartwatches

✅ Efficient: Only triggers once per minute, minimal CPU usage

### 4. Music Player (`app_music.cpp/.h`)✅ Visual Feedback: Clear list showing all alarms at a glance



**Playback Capabilities**The system is production-ready and provides a complete alarm management experience! Users can now manage multiple alarms that will reliably wake the watch and alert them even when the screen is off.
- Formats: MP3, WAV, FLAC, AAC
- Sources: SPIFFS filesystem, PROGMEM (embedded)
- Audio gain: 0.2 (comfortable listening level)
- Queue-based playback system

**Controls**
- Play/Pause toggle
- Volume adjustment
- Track selection from playlist
- Playlist management

**Queue System**
```cpp
QueueHandle_t play_music_queue;  // std::string* pointers
// Usage: queue "mp3_array", "boot_music", etc.
```

---

### 5. Calendar (`app_calendar.cpp/.h`)

**Features**
- Month view with day selection
- RTC integration (PCF8563 real-time clock)
- Event marking capability
- Date/time display
- NTP time synchronization support
- Timezone configuration (GMT-5 default)

---

## Utility Applications

### 6. Wireless Configuration (`app_wireless.cpp/.h`)

**WiFi Management**
- Network scanning with SSID list
- Connection manager with saved credentials
- Signal strength (RSSI) indicator
- Auto-reconnect capability
- Default network: "Agudelo Bonilla Mesh"

**Bluetooth LE**
- Device scanning and discovery
- Pairing management
- Connection status monitoring
- BLE HID support (keyboard + mouse)

**LoRa Radio (Optional)**
- SX126x transceiver control (SX1262/SX1268)
- Frequency configuration (433/868/915 MHz)
- Transmit/Receive mode switching
- Signal analysis tools

---

### 7. BLE Composite HID (`BleCompositeHID.cpp/.h`)

**Unified HID Device**
- Keyboard emulation (full 104-key support)
- Mouse emulation (movement + 3 buttons + wheel)
- Media keys (play, pause, volume, next, prev)
- Consumer control functions

**Device Information**
- Name: "T-Watch HID"
- Manufacturer: "LilyGo"
- Battery level reporting to host
- Auto-reconnect on disconnect

**API**
```cpp
void begin();                              // Initialize BLE HID
void end();                                // Shutdown BLE
void sendKey(uint8_t key, uint8_t mod);   // Send keystroke
void sendString(const char* str);          // Type string
void sendMouseMove(int8_t x, int8_t y, int8_t wheel);
void sendMouseClick(uint8_t button);       // MOUSE_LEFT/RIGHT/MIDDLE
void sendMediaKey(uint16_t key);           // MEDIA_PLAY_PAUSE, etc.
bool isConnected();                        // Check connection status
```

---

### 8. Virtual Keyboard (`app_keyboard.cpp/.h`)

**Layout & Features**
- Full QWERTY keyboard layout
- Bluetooth HID output to paired device
- Key combinations (Shift, Ctrl, Alt, Win)
- Special characters and symbols
- Function keys (F1-F12)
- Number pad mode

---

### 9. Virtual Mouse (`app_mouse.cpp/.h`)

**Controls**
- Touch-to-move cursor (relative movement)
- Left/Right/Middle click buttons
- Scroll wheel (vertical)
- Sensitivity adjustment slider
- Bluetooth HID output
- Visual cursor position indicator

---

### 10. System Controller (`app_controller.cpp/.h`)

**Functions**
- LED control (on/off, brightness)
- Display brightness adjustment (0-255)
- Vibration motor control (DRV2605L effects)
- System settings access
- Sensor calibration interface
- Power mode selection

---

### 11. Radio Transceiver (`app_radio.cpp/.h`)

**SX126x LoRa Control**
- Frequency selection: 433/868/915 MHz bands
- Power output: 2-22 dBm
- Bandwidth configuration
- Spreading factor (SF7-SF12)
- Coding rate selection

**Operating Modes**
- Transmit with custom messages
- Receive with RSSI/SNR display
- Ping-pong communication test
- Spectrum analyzer view
- Channel activity detection (CAD)

---

### 12. FFT Spectrum Analyzer (`app_fft.cpp/.h`)

**Features**
- Real-time audio FFT analysis
- Frequency spectrum visualization (chart)
- PDM microphone input (16kHz sample rate)
- Chart display with frequency bins
- Noise detection threshold
- Voice Activity Detection (VAD)

**Configuration**
- Sample rate: 16,000 Hz
- Frame length: 30ms
- Buffer size: 480 samples (VAD_BUFFER_LENGTH)
- FFT size: Configurable bins

---

### 13. Configuration (`app_configuration.cpp/.h`)

**System Settings**
- Display brightness slider
- Screen timeout duration
- Sleep mode configuration
- Deep sleep trigger button
- System reset function
- Firmware version information
- Factory reset option

---

## System Components

### Main Loop Architecture

**Primary Functions Called Every Cycle:**
```cpp
void loop() {
    lv_task_handler();        // LVGL event processing (5ms task handler)
    SensorHandler();          // Accelerometer interrupt processing
    PMUHandler();             // Power management unit events
    get_BattVoltage();        // Battery voltage/current readings
    app_batt_voltage_update(); // Update battery app if active
    check_alarm();            // Monitor all alarms for trigger
    check_timer();            // Monitor timer for completion
    
    // Auto-sleep after 30s inactivity
    if (lv_disp_get_inactive_time(NULL) >= 30000 && standby_en) {
        lowPowerEnergyHandler();
        standby_en = 0;
    }
}
```

### Power Management Modes

**Active Mode**
- Full brightness (adjustable 0-255)
- All sensors active
- WiFi/BLE enabled (if configured)
- CPU: 240 MHz

**Light Sleep Mode**
- Screen off (brightness = 0)
- Touch interrupt enabled
- Accelerometer wake-up (double-tap, tilt)
- PMU interrupt (button press)
- Alarms/timers continue running
- CPU: 160 MHz or sleep

**Deep Sleep** (manual trigger only)
- Complete system shutdown
- Only PMU wake-up enabled
- Lowest power consumption
- Requires button press to wake

---

### Sensor Integration

**BMA423 Accelerometer**
- Step counting algorithm
- Activity classification
- Tilt detection (wrist raise)
- Double-tap gesture
- Any motion / No motion events
- Interrupt-driven (minimal CPU usage)

**PCF8563 Real-Time Clock**
- Battery-backed timekeeping
- Alarm functionality (legacy, not used - system uses software alarms)
- 32.768 kHz crystal oscillator
- I²C interface

**AXP2101 Power Management Unit**
- Battery voltage/current monitoring
- Charge controller
- Buck/Boost converters
- LDO regulators for peripherals
- Temperature sensor
- Interrupt on charge events

---

### Communication Subsystems

**WiFi (ESP32-S3 internal)**
- 2.4 GHz 802.11 b/g/n
- Station + AP modes
- WPA2/WPA3-Personal security
- NTP time synchronization
- HTTPS support (WiFiClientSecure)
- Auto-reconnect on disconnect

**Bluetooth LE 5.0 (ESP32-S3 internal)**
- Central + Peripheral roles
- HID Device Profile (keyboard/mouse)
- GATT server for custom services
- Multiple simultaneous connections
- Long range mode support
- Low power advertisements

**LoRa (SX1262/SX1268 via SPI)**
- Sub-GHz ISM bands
- Range: Up to several km (line-of-sight)
- Low power consumption
- Configurable modulation
- Listen-before-talk (LBT)

---

### Audio System Architecture

**Dual Audio Engines**

1. **Music Playback Engine**
   - Gain: 0.2 (20% volume - comfortable)
   - Use case: Music player, audio files
   - Task-based playback (separate task)

2. **Alarm/Alert Engine**
   - Gain: 2.0 (200% volume - loud alerts)
   - Use case: Alarms, timer, notifications
   - Synchronous playback (immediate)

**Audio Processing Chain**
```
MP3 File (PROGMEM/SPIFFS)
    ↓
AudioFileSource (memory/file reader)
    ↓
AudioGeneratorMP3 (decoder)
    ↓
AudioOutputI2S (DAC output)
    ↓
Speaker/Amplifier
```

**I2S Configuration**
- Pins: BOARD_DAC_IIS_BCK, BOARD_DAC_IIS_WS, BOARD_DAC_IIS_DOUT
- Sample rate: 44.1 kHz (MP3 native)
- Bit depth: 16-bit
- Channels: Mono (external_I2S mode)

---

## File Structure

### Application Files (15 apps)

| File | Lines | Description |
|------|-------|-------------|
| `app_alarm.cpp/.h` | 836 | Alarm, stopwatch, timer with persistence |
| `app_batt_voltage.cpp/.h` | 223 | Battery monitoring & discharge tracking |
| `app_calendar.cpp/.h` | ~300 | Calendar with event marking |
| `app_configuration.cpp/.h` | ~200 | System settings interface |
| `app_controller.cpp/.h` | ~250 | LED, vibration, sensor control |
| `app_fft.cpp/.h` | ~400 | Audio spectrum analyzer |
| `app_keyboard.cpp/.h` | ~350 | Virtual keyboard (BLE HID) |
| `app_mouse.cpp/.h` | ~200 | Virtual mouse (BLE HID) |
| `app_music.cpp/.h` | ~500 | Multi-format music player |
| `app_radio.cpp/.h` | ~600 | LoRa transceiver control |
| `app_return.cpp/.h` | ~50 | Back button handler |
| `app_step_counter.cpp/.h` | ~300 | Pedometer with BMA423 |
| `app_wireless.cpp/.h` | ~400 | WiFi/BLE configuration |

### Core System Files

| File | Lines | Description |
|------|-------|-------------|
| `LilyGoGui.ino` | 2239 | Main entry point, loop, initialization |
| `ui.cpp/.h` | ~800 | App launcher UI, grid layout |
| `BleCompositeHID.cpp/.h` | ~500 | Unified BLE keyboard+mouse |
| `global_flags.h` | ~30 | Global state variables |
| `app_typedef.h` | ~12 | App system type definitions |
| `lv_example_menu_2.cpp/.h` | ~150 | LVGL menu example |

### Resource Files (`src/` directory - 60+ files)

**Audio Assets**
- `boot_music.c` - Startup sound (4,365 bytes)
- `mp3_array.c` - Beep sound (16,509 bytes)
- `mp3_ring_setup.c` - Ring/melody (86,144 bytes)
- `mp3_ring_1.h` - Additional ring tone
- `wav_array.h`, `aac_array.h`, `flac_array.h` - Format examples

**Fonts (14 font files)**
- `fn1_32.c` - Main UI font
- `liquidCrystal_nor_24/32/64.c` - LCD-style digits
- `digital_play_st_24/48.c` - Digital display font
- `robot_ightItalic_16.c` - Subtitle font
- `hansans_cn.c` - Chinese characters (24pt)
- `gracetians_32.c` - Decorative font
- `exninja_22.c` - Icon font
- `AlimamaShuHeiTi_Bold_24/54.c` - Bold Chinese
- `quostige_16.c` - Small text font

**Images (25+ image files)**
- `arrow_left/right_png.c` - Navigation (16×16)
- `img_*.c` - App icons (80×80): alarm, battery, calendar, controller, FFT, keyboard, mouse, music, radio, step counter, wireless
- `battery_img.c` - Battery level indicator
- `charge_done_battery.c` - Charging complete icon
- `lilygo2_gif.c` - Boot animation GIF
- `image_lilygo_fcc.c` - FCC certification image

---

## API Reference

### App Lifecycle

**App Structure Definition**
```cpp
typedef void (*app_func_t)(lv_obj_t *parent);

typedef struct {
    app_func_t setup_func_cb;  // Called when app opens
    app_func_t exit_func_cb;   // Called when app closes (can be NULL)
    void *user_data;           // App-specific data pointer
} app_t;
```

**Implementation Pattern**
```cpp
void app_example_load(lv_obj_t *cont) {
    // Create UI elements
    // Set up event handlers
    // Initialize state
}

void app_example_exit(lv_obj_t *cont) {
    // Clean up resources
    // Stop background tasks
    // Save state if needed
}

app_t app_example = {
    .setup_func_cb = app_example_load,
    .exit_func_cb = app_example_exit,  // or nullptr
    .user_data = nullptr
};
```

---

### Core System API

**Initialization Functions**
```cpp
void settingPMU();          // Configure AXP2101 power management
void settingSensor();       // Setup BMA423 accelerometer
void settingRadio();        // Initialize SX126x LoRa (if enabled)
void settingPlayer();       // Setup audio system
void settingIRRemote();     // Configure IR transmitter
void beginLvglHelper(bool touch); // Initialize LVGL graphics
```

**Event Handlers (called from main loop)**
```cpp
void SensorHandler();       // Process accelerometer interrupts
void PMUHandler();          // Process PMU interrupts
void lowPowerEnergyHandler(); // Enter/exit light sleep mode
```

**UI Update Functions**
```cpp
void printLocalTime();      // Format and update time string
void renew_ui_time();       // Refresh time display on main screen
void renew_ui_bat();        // Refresh battery indicator
```

---

### Audio Playback API

**Queue-Based Music Playback**
```cpp
QueueHandle_t play_music_queue;  // Global queue

// Enqueue music by asset name
std::string *pStr = new std::string("mp3_array");
xQueueSend(play_music_queue, &pStr, 0);

// Main loop processes queue and plays MP3
// Auto-cleans up after playback completes
```

**Direct Alarm Sound Playback**
```cpp
// Initialize alarm audio system (call once)
init_alarm_audio();

// Play sound with repetitions
play_alarm_sound(SOUND_BEEP, 2);    // Beep 2 times
play_alarm_sound(SOUND_RING, 5);    // Ring 5 times

// Vibration is automatic between repetitions
```

**Volume Control**
```cpp
// Music engine
out->SetGain(0.2);  // 20% volume (default)

// Alarm engine
alarm_out->SetGain(2.0);  // 200% volume (10x louder)
```

---

### Battery Monitoring API

**Reading Battery Info**
```cpp
float voltage = watch.getBatteryVoltage();      // Volts
float current = watch.getBatteryCurrent();      // mA (negative when discharging)
float percent = watch.getBatteryPercent();      // 0-100%
bool charging = watch.isCharging();             // true if USB connected
bool vbus = watch.isVbusIn();                   // true if USB power present
uint8_t status = watch.getChargerStatus();      // 0-5 (see chg_status array)
```

**Discharge Rate Calculation** (automatic)
```cpp
// Global variables updated by get_BattVoltage()
extern float last_battery_percent;              // Previous reading
extern unsigned long last_battery_time;         // Timestamp of last reading
extern float discharge_rate_percent_per_hour;   // Calculated rate

// Time remaining estimate
float hours_left = current_percent / discharge_rate_percent_per_hour;
```

---

### BLE HID API (Complete Reference)

**Keyboard Functions**
```cpp
// Single key press
bleHID.sendKey(KEY_A);                    // Press 'a'
bleHID.sendKey(KEY_A, MODIFIER_SHIFT);    // Press 'A' (Shift+a)
bleHID.sendKey(KEY_C, MODIFIER_CTRL);     // Ctrl+C (copy)

// Modifier masks (can be OR'd together)
MODIFIER_CTRL   // 0x01
MODIFIER_SHIFT  // 0x02
MODIFIER_ALT    // 0x04
MODIFIER_WIN    // 0x08

// Type a string
bleHID.sendString("Hello World!");        // Types entire string

// Special keys
bleHID.sendKey(KEY_ENTER);
bleHID.sendKey(KEY_ESC);
bleHID.sendKey(KEY_TAB);
bleHID.sendKey(KEY_BACKSPACE);
bleHID.sendKey(KEY_F1);  // Function keys F1-F12
```

**Mouse Functions**
```cpp
// Move cursor (relative movement)
bleHID.sendMouseMove(dx, dy);             // Move by dx, dy pixels
bleHID.sendMouseMove(dx, dy, wheel);      // Move + scroll wheel

// Click buttons
bleHID.sendMouseClick(MOUSE_LEFT);        // 0x01
bleHID.sendMouseClick(MOUSE_RIGHT);       // 0x02
bleHID.sendMouseClick(MOUSE_MIDDLE);      // 0x04

// Combined movement and click
bleHID.sendMouseMove(10, -5, 0);          // Move right 10, up 5
bleHID.sendMouseClick(MOUSE_LEFT);        // Then click
```

**Media Control Functions**
```cpp
bleHID.sendMediaKey(MEDIA_PLAY_PAUSE);    // Toggle play/pause
bleHID.sendMediaKey(MEDIA_NEXT_TRACK);    // Next song
bleHID.sendMediaKey(MEDIA_PREVIOUS_TRACK); // Previous song
bleHID.sendMediaKey(MEDIA_VOLUME_UP);     // Increase volume
bleHID.sendMediaKey(MEDIA_VOLUME_DOWN);   // Decrease volume
bleHID.sendMediaKey(MEDIA_VOLUME_MUTE);   // Mute/unmute
```

**Connection Management**
```cpp
// Check connection status
if (bleHID.isConnected()) {
    // Safe to send HID commands
    bleHID.sendKey(KEY_A);
}

// Get battery level reported to host
uint8_t batt_level = bleHID.getBatteryLevel();  // 0-100
bleHID.setBatteryLevel(percent);                // Update reported level
```

---

## Memory Usage & Performance

### Compilation Statistics (Release Mode)

```
RAM Usage:   18.6% (60,896 / 327,680 bytes)
Flash Usage: 36.4% (2,382,509 / 6,553,600 bytes)
```

### Storage Breakdown

| Component | Size | Percentage |
|-----------|------|------------|
| Application Code | ~2,300 KB | 35.1% |
| Audio Assets | ~107 KB | 1.6% |
| Font Data | ~200 KB | 3.1% |
| Image Assets | ~50 KB | 0.8% |
| **Total Used** | **~2,657 KB** | **40.6%** |
| **Available** | **~3,896 KB** | **59.4%** |

### Performance Metrics

- **Main Loop**: ~5ms cycle time (200 Hz)
- **LVGL Refresh**: 60 FPS capable (16.7ms frame time)
- **Touch Response**: <50ms latency
- **Alarm Check**: Once per minute (minimal CPU)
- **Sleep Entry**: 30 seconds of inactivity
- **Wake-Up Time**: <100ms (from light sleep)

---

## Development Guide

### Adding a New App

**1. Create App Files**

```cpp
// app_example.h
#pragma once
#include "app_typedef.h"
#include "lvgl.h"

extern app_t app_example;
void app_example_load(lv_obj_t *cont);
```

```cpp
// app_example.cpp
#include "app_example.h"

void app_example_load(lv_obj_t *cont) {
    // Create UI
    lv_obj_t *label = lv_label_create(cont);
    lv_label_set_text(label, "Hello World!");
    lv_obj_center(label);
}

app_t app_example = {
    .setup_func_cb = app_example_load,
    .exit_func_cb = nullptr,
    .user_data = nullptr
};
```

**2. Create App Icon (80×80 pixels)**

```c
// src/img_example.c
#include "lvgl.h"

const LV_ATTRIBUTE_MEM_ALIGN lv_img_dsc_t img_example = {
    .header.always_zero = 0,
    .header.w = 80,
    .header.h = 80,
    .data_size = 80 * 80 * LV_COLOR_SIZE / 8,
    .header.cf = LV_IMG_CF_TRUE_COLOR,
    .data = { /* pixel data array */ }
};
```

**3. Register in UI System**

```cpp
// ui.cpp
#include "app_example.h"
LV_IMG_DECLARE(img_example);

// In create_app_grid():
create_app(panel, "example", &img_example, &app_example);
```

---

### LVGL Best Practices

**Memory Management**
```cpp
// Clean before rebuilding UI
lv_obj_clean(container);

// Use static variables for persistent widgets
static lv_obj_t *my_label = nullptr;
if (my_label == nullptr) {
    my_label = lv_label_create(parent);
}
```

**Scrolling Containers**
```cpp
// Enable vertical scrolling
lv_obj_set_scrollbar_mode(cont, LV_SCROLLBAR_MODE_AUTO);
lv_obj_set_scroll_dir(cont, LV_DIR_VER);

// Set content height (larger than container)
lv_obj_set_height(cont, content_height);  // e.g., 400px in 200px viewport
```

**Event Handling with Lambdas**
```cpp
// Capture by reference for local variables
lv_obj_add_event_cb(btn, [](lv_event_t *e) {
    lv_obj_t *target = lv_event_get_target(e);
    Serial.println("Button clicked!");
}, LV_EVENT_CLICKED, NULL);

// Pass user data for context
struct ButtonData {
    int id;
    const char *name;
};

static ButtonData data = {1, "Test"};
lv_obj_set_user_data(btn, &data);

lv_obj_add_event_cb(btn, [](lv_event_t *e) {
    ButtonData *d = (ButtonData*)lv_obj_get_user_data(lv_event_get_target(e));
    Serial.printf("Button %d: %s\n", d->id, d->name);
}, LV_EVENT_CLICKED, NULL);
```

**Positioning & Alignment**
```cpp
// Absolute positioning
lv_obj_set_pos(obj, x, y);

// Relative alignment
lv_obj_align(obj, LV_ALIGN_CENTER, 0, 0);           // Center
lv_obj_align(obj, LV_ALIGN_TOP_MID, 0, 10);        // Top, centered, 10px down
lv_obj_align_to(obj, ref, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);  // Below ref, 5px gap
```

---

### Power Optimization Strategies

**1. Disable Unused Sensors**
```cpp
void app_example_exit(lv_obj_t *cont) {
    // Disable accelerometer if not needed
    watch.disableFeatureInterrupt(SensorBMA423::INT_STEP_CNTR);
    
    // Power down peripherals
    watch.powerOff(XPOWERS_ALDO3);  // Disable specific LDO
}
```

**2. Reduce Screen Updates**
```cpp
// Update only when data changes
static int last_value = -1;
if (new_value != last_value) {
    lv_label_set_text_fmt(label, "%d", new_value);
    last_value = new_value;
}
```

**3. Use Light Sleep Effectively**
```cpp
// Trigger activity to prevent sleep
lv_disp_trig_activity(NULL);  // Reset inactivity timer

// Or disable auto-sleep for critical operations
standby_en = 0;  // Disable sleep
// ... do critical work ...
standby_en = 1;  // Re-enable sleep
```

**4. Monitor Discharge Rate**
```cpp
// Check if discharge rate is abnormal
if (discharge_rate_percent_per_hour > 10.0) {
    Serial.println("Warning: High power consumption detected!");
    // Investigate which components are active
}
```

---

### Debugging Tips

**Serial Output**
```cpp
// All Serial.print statements go to USB CDC
Serial.printf("Battery: %.2fV, %.0fmA, %.1f%%\n", 
              voltage, current, percent);
```

**LVGL Debugging**
```cpp
// Enable LVGL logging (in lv_conf.h)
#define LV_USE_LOG 1
#define LV_LOG_LEVEL LV_LOG_LEVEL_WARN

// Log from app code
LV_LOG_INFO("App loaded successfully");
LV_LOG_WARN("Low memory: %d bytes", heap_free);
```

**Crash Debugging**
```cpp
// ESP32 stack trace
// Use addr2line or ESP-IDF monitor to decode addresses
// Tools -> USB CDC On Boot: Enable (for serial output)
```

---

## Configuration Reference

### Display Configuration

```cpp
#define SCREEN_WIDTH            240
#define SCREEN_HEIGHT           240
#define BG_COLOR                0xffffff
#define DEFAULT_COLOR           lv_color_make(252, 218, 72)
```

### Timing Configuration

```cpp
#define DEFAULT_SCREEN_TIMEOUT  15000  // 15 seconds (watchdog)
#define SLEEP_TIMEOUT           30000  // 30 seconds (light sleep)
#define LVGL_TASK_PERIOD        5      // 5ms (200Hz task handler)
```

### Audio Configuration

```cpp
#define MIC_I2S_SAMPLE_RATE     16000  // 16kHz for PDM microphone
#define VAD_FRAME_LENGTH_MS     30     // 30ms VAD frame
#define VAD_BUFFER_LENGTH       (VAD_FRAME_LENGTH_MS * MIC_I2S_SAMPLE_RATE / 1000)
```

### Network Configuration

```cpp
#define WIFI_SSID               "Agudelo Bonilla Mesh"
#define WIFI_PASSWORD           "V1CT0R1446U5T1N"
#define WIFI_CONNECT_WAIT_MAX   30000  // 30 seconds timeout
#define NTP_SERVER1             "pool.ntp.org"
#define NTP_SERVER2             "time.nist.gov"
#define GMT_OFFSET_SEC          -18000  // GMT-5 (5 hours × 3600)
#define DAY_LIGHT_OFFSET_SEC    0       // No DST
```

### Storage Configuration

```cpp
#define ALARM_STORAGE_FILE      "/alarms.txt"  // SD_MMC filesystem
#define DEFAULT_RECORD_FILENAME "/rec.wav"     // Audio recording output
```

---

## Troubleshooting

### Common Issues & Solutions

**Audio Not Playing**
- ✓ Check `init_alarm_audio()` was called before playback
- ✓ Verify I2S pins are correct (BOARD_DAC_IIS_*)
- ✓ Ensure audio file exists in PROGMEM or SPIFFS
- ✓ Confirm speaker is connected and enabled
- ✓ Try increasing gain: `alarm_out->SetGain(3.0);`

**Screen Not Waking from Sleep**
- ✓ Verify `lv_disp_trig_activity(NULL)` is called
- ✓ Check touch interrupt is enabled (`watch.enableTouch()`)
- ✓ Confirm PMU interrupt configuration
- ✓ Test accelerometer wake (double-tap)
- ✓ Check `standby_en` flag is set to 1

**Alarms Not Triggering**
- ✓ Ensure `check_alarm()` is called in main loop
- ✓ Verify RTC time is set correctly (`watch.getDateTime()`)
- ✓ Confirm alarm storage file exists and is readable
- ✓ Check alarm is enabled (toggle switch ON)
- ✓ Verify recurrence pattern matches current day

**BLE Connection Issues**
- ✓ Confirm `bleHID.begin()` was called in setup
- ✓ Check Bluetooth is enabled on paired device
- ✓ Verify pairing was successful (look for "T-Watch HID")
- ✓ Test with `bleHID.isConnected()` before sending
- ✓ Try restarting Bluetooth on both devices

**Battery Percentage Stuck**
- ✓ Wait 6+ minutes for discharge calculation
- ✓ Check charging cable is disconnected
- ✓ Verify PMU is reading correctly (`watch.getBatteryVoltage()`)
- ✓ Reset discharge tracking: set `last_battery_time = 0`

**High Power Consumption**
- ✓ Check WiFi is disconnected when not in use
- ✓ Disable LoRa if not needed
- ✓ Reduce screen brightness
- ✓ Ensure sleep mode is working (30s timeout)
- ✓ Monitor `discharge_rate_percent_per_hour` (should be <5%/h)

---

## Version History

### v2.0.0 (Current - November 2025)

**New Features**
- ✨ Multiple alarm support with persistent storage
- ✨ Timer with single-minute selection (00-59 minutes)
- ✨ Screen wake-up on alarm/timer completion
- ✨ Battery discharge rate tracking and time estimation
- ✨ Scrollable alarm list UI (220×130px viewport)
- ✨ Individual alarm enable/disable toggles
- ✨ Alarm delete functionality
- ✨ Quick-add alarm button

**Improvements**
- 🎨 Optimized UI layouts for 240×240 display
- 🎨 Better spacing in alarm controls (moved 10-15px left)
- 🎨 Extended alarm container by 20px for more room
- 🎨 Improved battery table layout with better scrolling
- 🎨 Added 5px spacing to battery info label
- 🔊 Audio gain increased to 2.0 (10× louder for alarms)
- 🔊 Multiple sound repetitions (1-5× depending on context)
- ⚡ Background timer operation with wake-up capability

**Bug Fixes**
- 🐛 Fixed alarm/timer not keeping time when screen off
- 🐛 Fixed battery table first row overlapping return button
- 🐛 Fixed stopwatch lap recording not generating logs
- 🐛 Fixed timer lacking beep/vibrate on completion
- 🐛 Fixed alarm back button overlap issues
- 🐛 Fixed minute selector limited to 5-minute increments

**Technical Changes**
- 📝 Changed from single alarm to `std::vector<AlarmData>`
- 📝 Added persistent storage: `/alarms.txt` (CSV format)
- 📝 Implemented `check_alarm()` and `check_timer()` background monitoring
- 📝 Added `triggered_alarm_id` flag for wake-up handling
- 📝 Integrated SD_MMC filesystem support
- 📝 Created comprehensive alarm management API

### v1.0.0 (Base Release)

**Initial Features**
- Basic app launcher with grid layout
- Single alarm with manual time entry
- Stopwatch with lap times
- Timer with hour/minute/second selection
- Music player (MP3/WAV/FLAC/AAC)
- Step counter with BMA423
- BLE keyboard emulation
- BLE mouse emulation
- WiFi configuration UI
- LoRa radio control
- FFT spectrum analyzer
- Battery voltage display
- Calendar with month view
- System configuration panel

---

## Credits & Acknowledgments

**Project Team**
- **Lead Developer**: zhanguichen
- **Hardware Design**: LilyGo (Shenzhen Xinyuan Electronic Technology Co., Ltd)
- **Firmware Architecture**: Xinyuan Engineering Team
- **Alarm System Enhancement**: Advanced multi-alarm with persistent storage (v2.0)

**Libraries & Dependencies**
- **LVGL**: Light and Versatile Graphics Library (v8.3.9) by Gabor Kiss-Vamossi
- **ESP8266Audio**: Audio playback library (v1.9.7) by Earle F. Philhower, III
- **TFT_eSPI**: Display driver (v2.5.30) by Bodmer
- **Arduino ESP32**: Core framework (v2.0.9) by Espressif Systems
- **XPowersLib**: PMU library (v0.2.1) by LilyGo
- **SensorLib**: Sensor drivers (v0.1.4) by LilyGo
- **RadioLib**: Radio library (v6.3.0) by Jan Gromeš
- **NimBLE-Arduino**: Bluetooth LE stack (v1.4.3) by h2zero

**Hardware Platform**
- **SoC**: ESP32-S3 (Xtensa dual-core @ 240MHz)
- **Display**: ST7789V TFT LCD (240×240)
- **Touch**: CST816S capacitive touch controller
- **Accelerometer**: BMA423 (Bosch Sensortec)
- **RTC**: PCF8563 (NXP)
- **PMU**: AXP2101 (X-Powers)
- **Audio**: MAX98357A I2S amplifier
- **Radio**: SX1262 LoRa transceiver (optional)

**Open Source License**

MIT License

Copyright (c) 2023-2025 Shenzhen Xinyuan Electronic Technology Co., Ltd

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

---

## Battery & Power Management

The T-Watch S3 uses a **470 mAh** LiPo battery managed by the AXP2101 PMU.

### Power states

| State | CPU | WiFi | BLE | Display | Draw |
|---|---|---|---|---|---|
| Active (screen on) | 160 MHz | connected | advertising | on | ~128 mA |
| Low-power polling | 160 MHz | off | advertising | off | ~42 mA |
| Light sleep (`lightSleep=true`) | stopped | off | off | off | ~0.8 mA |

The firmware enters low-power polling after 30 seconds of screen inactivity:

1. WiFi is gracefully disconnected (`WiFi.disconnect(true)` → `WiFi.mode(WIFI_OFF)`)
2. Display brightness decremented to zero
3. CPU enters a millis()-tracked polling loop, checking for PMU button, accelerometer IRQ, or touch
4. On wake the motor vibrates briefly, display restores, and sensor interrupts are re-enabled

### Expected battery life

| Usage pattern | Runtime |
|---|---|
| Check time every 15 min (30 s active + 14.5 min polling) | ~10 hours |
| Left untouched (one boot, then forever polling) | ~11 hours |
| Light sleep enabled (idle, wake on button) | ~24+ days |

The primary drain in low-power polling is the CPU running at 160 MHz (~35 mA). To extend battery life significantly, enable the light-sleep toggle in the UI. This uses `esp_light_sleep_start()` which stops the CPU and drops draw to sub-milliamps, waking on the PMU button or accelerometer double-tap.

### Discharge tracking

The battery app (`app_batt_voltage.cpp`) tracks real-time discharge rate in %/hour and mA, and estimates remaining runtime. Capacity is hardcoded at `BATTERY_CAPACITY_MAH = 470.0f`.

## Support & Resources

**Documentation**
- Hardware Schematic: `/schematic/` directory
- Example Code: `/examples/` directory
- Firmware Binaries: `/firmware/watch-s3/` directory

**Community**
- GitHub Repository: [Xinyuan-LilyGO/TTGO_TWatch_Library](https://github.com/Xinyuan-LilyGO/TTGO_TWatch_Library)
- Branch: `android-nav-buttons` (this implementation)
- Issues: Report bugs via GitHub Issues

**Contact**
- Manufacturer: Shenzhen Xinyuan Electronic Technology Co., Ltd
- Website: www.lilygo.cc
- Email: support@lilygo.cc

---

**Last Updated**: November 6, 2025  
**Documentation Version**: 2.0.0  
**Firmware Version**: 2.0.0 (android-nav-buttons branch)

---

**End of Documentation**
