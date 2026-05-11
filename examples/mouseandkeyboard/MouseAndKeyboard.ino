// This dummy function declaration helps the Arduino preprocessor handle includes correctly.
void setup();

#include <LilyGoLib.h>
#include <LV_Helper.h>

#include <BleKeyboard.h>
#include <BleMouse.h>

BleKeyboard bleKeyboard("T-Watch Keyboard", "LilyGo", 100);
BleMouse bleMouse("T-Watch Mouse", "LilyGo", 100);

// LVGL UI elements
static lv_obj_t *status_label;
static lv_obj_t *device_info_label;

void setup()
{
    Serial.begin(115200);
    
    watch.begin();

    //Initialize LVGL
    beginLvglHelper(false);

    // Create a screen and add labels
    lv_obj_t *scr = lv_scr_act();
    if (scr == NULL) {
        Serial.println("Error: lv_scr_act() returned NULL");
        while(1);
    }

    status_label = lv_label_create(scr);
    if (status_label == NULL) {
        Serial.println("Error: lv_label_create(scr) for status_label returned NULL");
        while(1);
    }
    lv_label_set_text(status_label, "Disconnected");
    lv_obj_align(status_label, LV_ALIGN_TOP_MID, 0, 10);

    device_info_label = lv_label_create(scr);
    if (device_info_label == NULL) {
        Serial.println("Error: lv_label_create(scr) for device_info_label returned NULL");
        while(1);
    }
    lv_label_set_text(device_info_label, "");
    lv_obj_align(device_info_label, LV_ALIGN_CENTER, 0, 0);

    // Start the BLE keyboard and mouse services
    bleKeyboard.begin();
    bleMouse.begin();

    Serial.println("Bluetooth HID services started. Waiting for connection..."); 
}

void loop()
{
    static bool last_keyboard_connected_state = false;
    static bool last_mouse_connected_state = false;

    bool current_keyboard_connected_state = bleKeyboard.isConnected();
    bool current_mouse_connected_state = bleMouse.isConnected();

    // Update connection status label if state changes
    if (current_keyboard_connected_state != last_keyboard_connected_state) {
        if (current_keyboard_connected_state) {
            lv_label_set_text(status_label, "Keyboard Connected");
            Serial.println("Keyboard connected!");
        } else {
            lv_label_set_text(status_label, "Keyboard Disconnected");
            Serial.println("Keyboard disconnected!");
        }
        last_keyboard_connected_state = current_keyboard_connected_state;
    }
    
    if (current_mouse_connected_state != last_mouse_connected_state) {
        if (current_mouse_connected_state) {
            // You can update the label for mouse connection as well
            // For simplicity, we are just printing to serial
            Serial.println("Mouse connected!");
        } else {
            Serial.println("Mouse disconnected!");
        }
        last_mouse_connected_state = current_mouse_connected_state;
    }

    // If both are connected, send some commands
    if (bleKeyboard.isConnected() && bleMouse.isConnected()) {

        // Example of sending media keys
        // In a real application, you would trigger these actions based on button presses or touch events.
        Serial.println("Sending Play/Pause");
        bleKeyboard.write(KEY_MEDIA_PLAY_PAUSE);
        delay(2000);

        Serial.println("Sending Next Track");
        bleKeyboard.write(KEY_MEDIA_NEXT_TRACK);
        delay(2000);

        Serial.println("Sending Previous Track");
        bleKeyboard.write(KEY_MEDIA_PREVIOUS_TRACK);
        delay(2000);

        Serial.println("Sending Volume Up");
        bleKeyboard.write(KEY_MEDIA_VOLUME_UP);
        delay(2000);

        Serial.println("Sending Volume Down");
        bleKeyboard.write(KEY_MEDIA_VOLUME_DOWN);
        delay(2000);

        // Example of moving the mouse
        Serial.println("Moving mouse");
        bleMouse.move(10, 10, 0, 0); // Move mouse right and down
        delay(1000);
    }

    // LVGL task handler
    lv_task_handler();

    delay(5); 
}