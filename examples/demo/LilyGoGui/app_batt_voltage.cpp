#include "app_batt_voltage.h"
#include "Arduino.h"
#include <WiFi.h>
#include <LilyGoLib.h>

lv_obj_t * batt_voltage_label = NULL;
lv_obj_t * power_analysis_table = NULL;

LV_FONT_DECLARE(robot_ightItalic_16);
LV_FONT_DECLARE(lv_font_montserrat_14);
LV_FONT_DECLARE(lv_font_montserrat_12);

extern LilyGoLib watch;
extern lv_obj_t *setupGUI();
extern bool bleEnabled;
extern bool wifiEnabled;
extern bool loraEnabled;

// Battery discharge tracking
static float last_battery_percent = -1.0f;
static unsigned long last_battery_time = 0;
static float discharge_rate_percent_per_hour = 0.0f;
const float BATTERY_CAPACITY_MAH = 470.0f; // 470mAh battery

void app_batt_voltage_load(lv_obj_t *cont) {

    // Create scrollable container - Leave space for back button (40px at bottom)
    lv_obj_t * obj = lv_obj_create(cont);
    lv_obj_set_size(obj, 240, 200);  // 200px height leaves 40px for back button
    lv_obj_align(obj, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x000000), 0);
    lv_obj_set_style_border_width(obj, 0, 0);  
    lv_obj_set_style_pad_all(obj, 0, 0);
    
    // Enable vertical scrolling
    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_scroll_dir(obj, LV_DIR_VER);
    
    int y_pos = 5;

    // Battery voltage label at top
    batt_voltage_label = lv_label_create(obj);
    lv_label_set_recolor(batt_voltage_label, true);
    lv_obj_set_pos(batt_voltage_label, 50, y_pos);
    lv_obj_set_style_text_color(batt_voltage_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(batt_voltage_label, &lv_font_montserrat_14, 0);
    lv_label_set_text(batt_voltage_label, "#FF0000 Battery Info#");
    y_pos += 35;  // Increased from 25 to 35 for more space

    // Power Analysis Table - more compact
    power_analysis_table = lv_table_create(obj);
    lv_obj_set_size(power_analysis_table, 230, 300);  // Reduced height to fit better
    lv_obj_set_pos(power_analysis_table, 5, y_pos);
    lv_obj_set_style_bg_color(power_analysis_table, lv_color_hex(0x000000), 0);
    lv_obj_set_style_text_color(power_analysis_table, lv_color_white(), LV_PART_ITEMS);
    lv_obj_set_style_text_font(power_analysis_table, &lv_font_montserrat_12, LV_PART_ITEMS);  // Smaller font
    lv_obj_set_style_pad_all(power_analysis_table, 3, LV_PART_ITEMS);
    lv_obj_set_style_pad_top(power_analysis_table, 1, LV_PART_ITEMS);
    lv_obj_set_style_pad_bottom(power_analysis_table, 1, LV_PART_ITEMS);
    
    // Table headers - adjusted widths for better fit
    lv_table_set_col_cnt(power_analysis_table, 3);
    lv_table_set_col_width(power_analysis_table, 0, 75);   // Module name
    lv_table_set_col_width(power_analysis_table, 1, 70);   // Status
    lv_table_set_col_width(power_analysis_table, 2, 55);   // Power
    
    lv_table_set_cell_value(power_analysis_table, 0, 0, "Module");
    lv_table_set_cell_value(power_analysis_table, 0, 1, "Status");
    lv_table_set_cell_value(power_analysis_table, 0, 2, "Power");
    
    // Header styling
    for (int i = 0; i < 3; i++) {
        lv_obj_set_style_bg_color(power_analysis_table, lv_color_hex(0x333333), LV_PART_ITEMS | LV_STATE_DEFAULT);
    }
    
    // Power consumption estimates (in mA)
    lv_table_set_cell_value(power_analysis_table, 1, 0, "Display");
    lv_table_set_cell_value(power_analysis_table, 1, 1, "ON");
    lv_table_set_cell_value(power_analysis_table, 1, 2, "40mA");
    
    lv_table_set_cell_value(power_analysis_table, 2, 0, "WiFi");
    lv_table_set_cell_value(power_analysis_table, 2, 1, wifiEnabled ? "#00FF00 ON#" : "#808080 OFF#");
    lv_table_set_cell_value(power_analysis_table, 2, 2, wifiEnabled ? "80mA" : "0mA");
    
    lv_table_set_cell_value(power_analysis_table, 3, 0, "BLE");
    lv_table_set_cell_value(power_analysis_table, 3, 1, bleEnabled ? "#00FF00 ON#" : "#808080 OFF#");
    lv_table_set_cell_value(power_analysis_table, 3, 2, bleEnabled ? "15mA" : "0mA");
    
    lv_table_set_cell_value(power_analysis_table, 4, 0, "LoRa");
    lv_table_set_cell_value(power_analysis_table, 4, 1, loraEnabled ? "#00FF00 ON#" : "#808080 OFF#");
    lv_table_set_cell_value(power_analysis_table, 4, 2, loraEnabled ? "30mA" : "0mA");
    
    lv_table_set_cell_value(power_analysis_table, 5, 0, "CPU");
    lv_table_set_cell_value(power_analysis_table, 5, 1, "Active");
    lv_table_set_cell_value(power_analysis_table, 5, 2, "20mA");
    
    // Calculate total power
    int total_power = 40 + 20; // Display + CPU always on
    if (wifiEnabled) total_power += 80;
    if (bleEnabled) total_power += 15;
    if (loraEnabled) total_power += 30;
    
    lv_table_set_cell_value(power_analysis_table, 6, 0, "Total");
    lv_table_set_cell_value(power_analysis_table, 6, 1, "Estimated");
    char total_text[16];
    sprintf(total_text, "%dmA", total_power);
    lv_table_set_cell_value(power_analysis_table, 6, 2, total_text);
    
    // Add discharge rate row (initially empty)
    lv_table_set_cell_value(power_analysis_table, 7, 0, "Rate");
    lv_table_set_cell_value(power_analysis_table, 7, 1, "---");
    lv_table_set_cell_value(power_analysis_table, 7, 2, "---");
    
    // Add remaining time row (initially empty)
    lv_table_set_cell_value(power_analysis_table, 8, 0, "Time Left");
    lv_table_set_cell_value(power_analysis_table, 8, 1, "---");
    lv_table_set_cell_value(power_analysis_table, 8, 2, "---");
    
    // Add battery capacity row
    lv_table_set_cell_value(power_analysis_table, 9, 0, "Capacity");
    lv_table_set_cell_value(power_analysis_table, 9, 1, "470mAh");
    lv_table_set_cell_value(power_analysis_table, 9, 2, "---");
    
    // Make last rows have different background
    lv_obj_set_style_bg_color(power_analysis_table, lv_color_hex(0x1a1a1a), LV_PART_ITEMS);
    
    y_pos += 310;
    
    // Set content height for scrolling (taller than visible area)
    lv_obj_set_height(obj, y_pos);
}

void app_batt_voltage_exit(lv_obj_t *cont) {
    batt_voltage_label = NULL;
    power_analysis_table = NULL;
}

void app_batt_voltage_update() {
    if (power_analysis_table == NULL) return;
    
    // Update power consumption table
    lv_table_set_cell_value(power_analysis_table, 2, 1, wifiEnabled ? "#00FF00 ON#" : "#808080 OFF#");
    lv_table_set_cell_value(power_analysis_table, 2, 2, wifiEnabled ? "80mA" : "0mA");
    
    lv_table_set_cell_value(power_analysis_table, 3, 1, bleEnabled ? "#00FF00 ON#" : "#808080 OFF#");
    lv_table_set_cell_value(power_analysis_table, 3, 2, bleEnabled ? "15mA" : "0mA");
    
    lv_table_set_cell_value(power_analysis_table, 4, 1, loraEnabled ? "#00FF00 ON#" : "#808080 OFF#");
    lv_table_set_cell_value(power_analysis_table, 4, 2, loraEnabled ? "30mA" : "0mA");
    
    // Calculate total estimated power
    int total_power = 40 + 20; // Display + CPU always on
    if (wifiEnabled) total_power += 80;
    if (bleEnabled) total_power += 15;
    if (loraEnabled) total_power += 30;
    
    char total_text[16];
    sprintf(total_text, "%dmA", total_power);
    lv_table_set_cell_value(power_analysis_table, 6, 2, total_text);
    
    // Calculate and display discharge rate and remaining time
    extern float discharge_rate_percent_per_hour;
    float current_percent = watch.getBatteryPercent();
    bool is_charging = watch.isCharging();
    
    // Update discharge rate row (row 7)
    lv_table_set_cell_value(power_analysis_table, 7, 0, "Rate");
    if (is_charging) {
        lv_table_set_cell_value(power_analysis_table, 7, 1, "#00FF00 Charging#");
        lv_table_set_cell_value(power_analysis_table, 7, 2, "---");
    } else if (discharge_rate_percent_per_hour > 0.1f) {
        float discharge_ma = (discharge_rate_percent_per_hour * BATTERY_CAPACITY_MAH) / 100.0f;
        char rate_text[16];
        sprintf(rate_text, "%.1f%%/h", discharge_rate_percent_per_hour);
        lv_table_set_cell_value(power_analysis_table, 7, 1, rate_text);
        
        char ma_text[12];
        sprintf(ma_text, "%.0fmA", discharge_ma);
        lv_table_set_cell_value(power_analysis_table, 7, 2, ma_text);
    } else {
        lv_table_set_cell_value(power_analysis_table, 7, 1, "Wait...");
        lv_table_set_cell_value(power_analysis_table, 7, 2, "---");
    }
    
    // Update remaining time row (row 8)
    lv_table_set_cell_value(power_analysis_table, 8, 0, "Time Left");
    if (is_charging) {
        lv_table_set_cell_value(power_analysis_table, 8, 1, "#00FF00 Charging#");
        lv_table_set_cell_value(power_analysis_table, 8, 2, "---");
    } else if (discharge_rate_percent_per_hour > 0.1f && current_percent > 0) {
        float remaining_hours = current_percent / discharge_rate_percent_per_hour;
        int hours = (int)remaining_hours;
        int minutes = (int)((remaining_hours - hours) * 60);
        
        char time_text[16];
        sprintf(time_text, "%dh %dm", hours, minutes);
        lv_table_set_cell_value(power_analysis_table, 8, 1, time_text);
        
        char capacity_text[12];
        float remaining_mah = (current_percent / 100.0f) * BATTERY_CAPACITY_MAH;
        sprintf(capacity_text, "%.0fmAh", remaining_mah);
        lv_table_set_cell_value(power_analysis_table, 8, 2, capacity_text);
    } else {
        lv_table_set_cell_value(power_analysis_table, 8, 1, "Wait...");
        lv_table_set_cell_value(power_analysis_table, 8, 2, "---");
    }
    
    // Update battery capacity row (row 9) with current level
    lv_table_set_cell_value(power_analysis_table, 9, 0, "Capacity");
    char capacity_label[16];
    sprintf(capacity_label, "470mAh");
    lv_table_set_cell_value(power_analysis_table, 9, 1, capacity_label);
    
    char percent_text[16];
    sprintf(percent_text, "%.0f%%", current_percent);
    lv_table_set_cell_value(power_analysis_table, 9, 2, percent_text);
}

app_t app_batt_voltage = {
    .setup_func_cb = app_batt_voltage_load,
    .exit_func_cb = app_batt_voltage_exit,
    .user_data= nullptr,
};