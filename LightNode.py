# LightNodes.py (MicroPython for M5StickC Plus / ESP32)
# Acts as a BLE Peripheral exposing LIGHT_NAME and LIGHT_STATE

import bluetooth
from ble_advertising import advertising_payload
from micropython import const
import struct
import time

_LIGHT_NAME_UUID = bluetooth.UUID("11111111-1111-1111-1111-111111111111")
_LIGHT_STATE_UUID = bluetooth.UUID("22222222-2222-2222-2222-222222222222")
_SERVICE_UUID = bluetooth.UUID("da1a59b9-1125-4bd4-b72b-d15dc8057c53")

# Define flags for read/write
_FLAG_READ = const(0x0002)
_FLAG_WRITE = const(0x0008)

# Create BLE instance
ble = bluetooth.BLE()
ble.active(True)

light_name = b"LightNode-1"
light_state = bytearray(b"0")  # OFF by default

def on_write(attr_handle, data):
    global light_state
    light_state[:] = data
    print("State changed to:", "ON" if data == b"1" else "OFF")

# Register the GATT server
services = (
    (_SERVICE_UUID,
     (
        (_LIGHT_NAME_UUID, _FLAG_READ),
        (_LIGHT_STATE_UUID, _FLAG_READ | _FLAG_WRITE),
     )),
)

handles = ble.gatts_register_services(services)

# Get handles
light_name_handle = handles[0][0]
light_state_handle = handles[0][1]

# Write initial values
ble.gatts_write(light_name_handle, light_name)
ble.gatts_write(light_state_handle, light_state)

# Set write callback
ble.irq(handler=lambda event, data: on_write(*data) if event == 3 else None)

# Advertise
payload = advertising_payload(name="LightNode-1", services=[_SERVICE_UUID])
ble.gap_advertise(100_000, adv_data=payload)

print("LightNode advertising...")

# Keep the script running
while True:
    time.sleep(1)
