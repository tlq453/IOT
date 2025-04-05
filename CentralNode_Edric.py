import asyncio
from bleak import BleakScanner, BleakClient

# UUIDs (must match M5StickCPlus firmware)
SERVICE_UUID = "35e65a71-b2d6-43f7-b3be-719ce5744884"  # M5StickCPlus service
LIGHT_STATE_UUID = "c033e6dc-1355-4935-abb1-6e46b1a84c36"  # LED state characteristic

# Track connected devices
connected_devices = {}

def notification_handler(sender, data, device_name):
    """Callback when M5StickCPlus sends LED state updates."""
    state = int.from_bytes(data, byteorder="little")
    print(f"📢 {device_name}: LED {'ON' if state else 'OFF'}")

async def connect_to_device(device, max_retries=3):
    retry_count = 0
    while retry_count < max_retries:
        try:
            async with BleakClient(device, timeout=20.0) as client:  # Increased timeout
                print(f"✅ Connected to {device.name}")
                await client.start_notify(
                    LIGHT_STATE_UUID,
                     lambda s, d: notification_handler(s, d, device.name)
                    )
                connected_devices[device.address] = client
                
                while client.is_connected:
                    await asyncio.sleep(1)
                return  # Exit on clean disconnect
                
        except Exception as e:
            retry_count += 1
            print(f"⚠️ Attempt {retry_count}/{max_retries} failed: {str(e)}")
            await asyncio.sleep(2)  # Wait before retry
    
    print(f"❌ Max retries reached for {device.address}")

# async def main():
#     global connected_devices

#     while True:
#         if not connected_devices:
#             print("🔍 Scanning for M5StickCPlus devices...")
#             devices = await BleakScanner.discover(
#                 timeout=10.0,
#                 detection_callback=lambda d, _: print(f"Found: {d.name}"),
#                 scanning_mode="active",
#                 adapter="hci0"
#             )
            
#             # Manual UUID filtering
#             valid_devices = [
#                 d for d in devices 
#                 if SERVICE_UUID.lower() in d.metadata.get("uuids", [])
#             ]
            
#             if not valid_devices:
#                 print("🚫 No valid devices found. Retrying...")
#                 await asyncio.sleep(5)
#                 continue
            
#             # Connect to first valid device
#             asyncio.create_task(connect_to_device(valid_devices[0]))
        
#         # Just wait for notifications if connected
#         else:
#             # Check if any devices are still connected
#             for addr, client in list(connected_devices.items()):
#                 if not client.is_connected:
#                     del connected_devices[addr]
#                     print(f"♻️ Disconnected: {addr}")
            
#             await asyncio.sleep(1)  # Short sleep to prevent CPU overload

async def main():
    global connected_devices
    scanner = None
    
    while True:
        if not connected_devices:
            print("🔍 Starting new scan for devices...")
            try:
                # Start scanner with our callback
                scanner = BleakScanner(
                    detection_callback=lambda d, _: (
                        SERVICE_UUID.lower() in d.metadata.get("uuids", [])
                    ),
                    scanning_mode="active"
                )
                await scanner.start()

                # Wait until we find our device
                found_device = None
                while not found_device and not connected_devices:
                    devices = await scanner.get_discovered_devices()
                    valid_devices = [d for d in devices if SERVICE_UUID.lower() in d.metadata.get("uuids", [])]
                    
                    if valid_devices:
                        found_device = valid_devices[0]
                        print(f"⚡ Found target device: {found_device.name}")
                        # Immediately attempt connection
                        await connect_to_device(found_device)
                    
                    await asyncio.sleep(0.1)

            except Exception as e:
                print(f"⚠️ Scan error: {str(e)}")
            finally:
                if scanner:
                    await scanner.stop()
                    scanner = None
                    print("🛑 Scanning completely stopped")
        
        else:
            # Connected maintenance mode - no scanning happens here
            for addr, client in list(connected_devices.items()):
                if not client.is_connected:
                    del connected_devices[addr]
                    print(f"♻️ Device disconnected: {addr}")
                    break  # Exit maintenance mode
            
            await asyncio.sleep(1)

if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\n🛑 Server stopped.")