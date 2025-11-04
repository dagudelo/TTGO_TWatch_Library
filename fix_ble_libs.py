#!/usr/bin/env python3
"""
Script to fix ESP32 BLE Keyboard/Mouse library conflicts with NimBLE-Arduino.
This script modifies the BleKeyboard.h and BleMouse.h files to prevent macro
redefinition conflicts.
"""

Import("env")
import os

def patch_ble_headers(source, target, env):
    """Patch the BLE Keyboard and Mouse header files to prevent conflicts."""
    
    # Get the library dependencies path
    libdeps_dir = env.subst("$PROJECT_LIBDEPS_DIR/$PIOENV")
    
    # Paths to the files that need patching
    ble_keyboard_h = os.path.join(libdeps_dir, "ESP32 BLE Keyboard", "BleKeyboard.h")
    ble_mouse_h = os.path.join(libdeps_dir, "ESP32 BLE Mouse", "BleMouse.h")
    
    files_to_patch = [ble_keyboard_h, ble_mouse_h]
    
    for file_path in files_to_patch:
        if not os.path.exists(file_path):
            continue
            
        print(f"Patching {file_path}...")
        
        with open(file_path, 'r') as f:
            content = f.read()
        
        # Check if already patched
        if "// PATCHED TO PREVENT CONFLICTS" in content:
            print(f"{file_path} already patched, skipping...")
            continue
        
        # Remove the conflicting macro definitions
        new_content = content.replace(
            '#define BLEDevice                  NimBLEDevice\n'
            '#define BLEServerCallbacks         NimBLEServerCallbacks\n'
            '#define BLECharacteristicCallbacks NimBLECharacteristicCallbacks\n'
            '#define BLEHIDDevice               NimBLEHIDDevice\n'
            '#define BLECharacteristic          NimBLECharacteristic\n'
            '#define BLEAdvertising             NimBLEAdvertising\n'
            '#define BLEServer                  NimBLEServer\n'
            '#define BLERemoteCharacteristics   NimBLERemoteCharacteristics',
            '// PATCHED TO PREVENT CONFLICTS WITH NIMBLE-ARDUINO\n'
            '// Use NimBLE classes directly instead of macros'
        )
        
        # Also remove individual defines if not in a block
        defines = [
            ('#define BLEDevice                  NimBLEDevice', 'using BLEDevice = NimBLEDevice;'),
            ('#define BLEServerCallbacks         NimBLEServerCallbacks', 'using BLEServerCallbacks = NimBLEServerCallbacks;'),
            ('#define BLECharacteristicCallbacks NimBLECharacteristicCallbacks', 'using BLECharacteristicCallbacks = NimBLECharacteristicCallbacks;'),
            ('#define BLEHIDDevice               NimBLEHIDDevice', 'using BLEHIDDevice = NimBLEHIDDevice;'),
            ('#define BLECharacteristic          NimBLECharacteristic', 'using BLECharacteristic = NimBLECharacteristic;'),
            ('#define BLEAdvertising             NimBLEAdvertising', 'using BLEAdvertising = NimBLEAdvertising;'),
            ('#define BLEServer                  NimBLEServer', 'using BLEServer = NimBLEServer;'),
            ('#define BLERemoteCharacteristics   NimBLERemoteCharacteristics', 'using BLERemoteCharacteristics = NimBLERemoteCharacteristics;'),
        ]
        
        for old, new in defines:
            if old in new_content:
                new_content = new_content.replace(old, f'// {old}  // PATCHED')
        
        with open(file_path, 'w') as f:
            f.write(new_content)
        
        print(f"Successfully patched {file_path}")

env.AddPreAction("buildprog", patch_ble_headers)
