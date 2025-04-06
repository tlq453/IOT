import asyncio
from bleak import BleakScanner, BleakClient
from database import add_light_status

# UUIDs (must match M5StickCPlus firmware)
SERVICE_UUID = "35e65a71-b2d6-43f7-b3be-719ce5744884"  # M5StickCPlus service
MOTION_STATE_UUID = "36bd104c-a23f-4dcf-b808-a1bc4516314e"
LIGHT_STATE_UUID = "c033e6dc-1355-4935-abb1-6e46b1a84c36"  # LED state characteristic

# Track connected devices
connected_devices = {}
max_connected_devices = 3  # Set a limit for connected devices


def motion_notification_handler(sender, data, device):
    """Callback when M5StickCPlus sends Motion state updates."""
    state = int.from_bytes(data, byteorder="little")
    print(f"📢 {device.name}: Motion {'DETECTED' if state else 'NO MOTION'}")
    # Find the device by address (which we know is device.address)
    for addr, device_info in connected_devices.items():
        asyncio.create_task(
            write_to_device(
                device_info["client"], "ON" if state else "OFF"  # Send proper command
            )
        )
    else:
        print(f"⚠️ Device {device.address} not found in connected_devices")


def led_notification_handler(sender, data, device):
    """Callback when M5StickCPlus sends LED state updates."""
    state = int.from_bytes(data, byteorder="little")
    add_light_status(device.name, state, "BLE")
    print(f"💡 {device.name}: LED {'ON' if state else 'OFF'}")


async def connect_to_device(device, max_retries=3):
    retry_count = 0
    while retry_count < max_retries:
        try:
            async with BleakClient(
                device, timeout=30.0
            ) as client:  # Increased timeout to 30s
                print(f"✅ Connected to {device.name}")

                # Add small delay for connection stabilization
                await asyncio.sleep(0.5)

                # Verify services exist first
                try:
                    services = await client.get_services()
                    has_motion = MOTION_STATE_UUID.lower() in {
                        str(c.uuid).lower() for s in services for c in s.characteristics
                    }
                    has_led = LIGHT_STATE_UUID.lower() in {
                        str(c.uuid).lower() for s in services for c in s.characteristics
                    }

                    # Subscribe to notifications if characteristics exist
                    if has_motion:
                        await client.start_notify(
                            MOTION_STATE_UUID,
                            lambda s, d: motion_notification_handler(s, d, device),
                        )
                        print(f"🔔 Subscribed to motion notifications")

                    if has_led:
                        await client.start_notify(
                            LIGHT_STATE_UUID,
                            lambda s, d: led_notification_handler(s, d, device),
                        )
                        print(f"💡 Subscribed to LED state notifications")

                except Exception as e:
                    print(f"⚠️ Service discovery failed: {str(e)}")
                    raise

                # Store device info
                connected_devices[device.address] = {
                    "client": client,
                    "light_state": LIGHT_STATE_UUID,
                    "name": device.name,
                    "has_motion": has_motion,
                    "has_led": has_led,
                }
                print(
                    f"📋 Connected devices ({len(connected_devices)}): {[d['name'] for d in connected_devices.values()]}"
                )

                while client.is_connected:
                    await asyncio.sleep(1)
                return  # Exit on clean disconnect

        except Exception as e:
            retry_count += 1
            print(f"⚠️ Attempt {retry_count}/{max_retries} failed: {str(e)}")
            await asyncio.sleep(2)  # Wait before retry

    print(f"❌ Max retries reached for {device.address}")


async def write_to_device(client, message):
    """Write data to a connected device"""
    try:
        if client.is_connected:
            byte_to_send = b"\x01" if message == "ON" else b"\x00"
            await client.write_gatt_char(LIGHT_STATE_UUID, byte_to_send)
            print(f"📤 Sent to {client}: {message}")
            return True
        else:
            print(f"⚠️ Device {client.address} not connected")
            return False
    except Exception as e:
        print(f"⚠️ Write failed to {client.address}: {str(e)}")
        return False


async def main():
    global connected_devices
    scanner = None
    global max_connected_devices

    while True:
        if len(connected_devices) < max_connected_devices:
            print("🔍 Scanning for devices...")
            try:
                scanner = BleakScanner(
                    detection_callback=lambda d, _: (
                        SERVICE_UUID.lower() in d.metadata.get("uuids", [])
                    ),
                    scanning_mode="active",
                )
                await scanner.start()

                test_connected_device = []
                # Continuous scanning until max reached
                while len(connected_devices) < max_connected_devices:
                    devices = await scanner.get_discovered_devices()
                    new_devices = [
                        d
                        for d in devices
                        if SERVICE_UUID.lower() in d.metadata.get("uuids", [])
                        and d.address not in connected_devices
                    ]

                    for device in new_devices:
                        if len(connected_devices) >= max_connected_devices:
                            break
                        print(f"⚡ Found: {device.name}")
                        # Non-blocking connection attempt
                        if device.name not in test_connected_device:
                            test_connected_device.append(device.name)
                            asyncio.create_task(connect_to_device(device))

                    await asyncio.sleep(1)  # Short interval between scans

            except Exception as e:
                print(f"⚠️ Scan error: {str(e)}")
            finally:
                if scanner:
                    await scanner.stop()

        else:
            # Maintain existing connections
            for addr, info in list(connected_devices.items()):
                if not info["client"].is_connected:
                    del connected_devices[addr]
                    print(f"♻️ Disconnected: {addr}")

            await asyncio.sleep(1)


if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\n🛑 Server stopped.")
