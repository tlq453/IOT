# CentralServer.py
# Runs on Raspberry Pi 4B
# Connects to multiple BLE LightNodes, reads LIGHT_NAME and LIGHT_STATE

import asyncio
from bleak import BleakScanner, BleakClient

# UUIDs for the BLE characteristics
SERVICE_UUID = "da1a59b9-1125-4bd4-b72b-d15dc8057c53"
LIGHT_NAME_UUID = "11111111-1111-1111-1111-111111111111"
LIGHT_STATE_UUID = "22222222-2222-2222-2222-222222222222"

device_list = {}

async def handle_device(device):
    async with BleakClient(device) as client:
        if not client.is_connected:
            print(f"Failed to connect to {device.name}")
            return

        try:
            light_name = await client.read_gatt_char(LIGHT_NAME_UUID)
            light_state = await client.read_gatt_char(LIGHT_STATE_UUID)

            name_str = light_name.decode('utf-8')
            state_str = light_state.decode('utf-8')

            print(f"{name_str}: {'ON' if state_str == '1' else 'OFF'}")
        except Exception as e:
            print(f"Error reading from {device.address}: {e}")

async def main():
    while True:
        
        '''
        print("🔍 Scanning for LightNodes...")
        devices = await BleakScanner.discover(timeout=5.0)

        for d in devices:
            print(f"Found: {d.name} ({d.address}) - {d.metadata}")
        
        
        '''    
        print("🔍 Scanning for LightNodes...")
        devices = await BleakScanner.discover(timeout=5.0)

        
        tasks = []
        for d in devices:
            # Use d.advertisement_data.service_uuids if updating for bleak 0.22+
            if SERVICE_UUID.lower() in [s.lower() for s in d.metadata.get("uuids", [])]:
                print(f"📡 Found LightNode: {d.name} ({d.address})")
                tasks.append(handle_device(d))

        if tasks:
            await asyncio.gather(*tasks)
        else:
            print("🚫 No LightNodes found.")

        print("⏳ Waiting before next scan...\n")
        await asyncio.sleep(10)  # Delay between scans
        #'''

if __name__ == "__main__":
    asyncio.run(main())
