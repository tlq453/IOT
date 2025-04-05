import asyncio
from bleak import BleakScanner, BleakClient

# === BLE Configuration ===
BLE_SERVER_NAMES = {
    "LightNode1": None,
    "LightNode2": None,
    "LightNode3": None,  # Optional
}

SERVICE_UUID = "da1a59b9-1125-4bd4-b72b-d15dc8057c53"
LIGHT_STATE_UUID = "22222222-2222-2222-2222-222222222222"

connected_clients = {}


async def write_to_adjacent_node(value: str):
    client = connected_clients.get("LightNode2")
    if client and client.is_connected:
        byte_val = bytes(value, 'utf-8')  # Convert char to bytes
        try:
            await client.write_gatt_char(LIGHT_STATE_UUID, byte_val)
            print(f"➡️ Sent '{value}' to AdjacentNode")
        except Exception as e:
            print(f"⚠️ Write failed: {e}")
    else:
        print("⚠️ Cannot write: AdjacentNode not connected")


def notification_handler(name):
    def handler(sender, data):
        state = chr(data[0])
        status = "ON" if state == '1' else "OFF"
        print(f"🔔 {name} LED is {status}")

        if name == "LightNode1":
            asyncio.create_task(write_to_adjacent_node(state))
        # You can expand here to update GUI or logs for AdjacentNode
    return handler


async def connect_to_device(device):
    try:
        client = BleakClient(device)
        await client.connect()
        print(f"🔗 Connected to {device.name}")

        services = await client.get_services()
        if LIGHT_STATE_UUID.lower() not in [str(c.uuid).lower() for s in services for c in s.characteristics]:
            print(f"⚠️ {device.name} has no LIGHT_STATE_UUID")
            return

        await client.start_notify(LIGHT_STATE_UUID, notification_handler(device.name))
        connected_clients[device.name] = client

    except Exception as e:
        print(f"❌ Failed to connect to {device.name}: {e}")


async def scan_and_connect():
    print("🔍 Scanning for LightNodes...")
    devices = await BleakScanner.discover()
    for d in devices:
        name = d.name
        if name in BLE_SERVER_NAMES and BLE_SERVER_NAMES[name] is None:
            BLE_SERVER_NAMES[name] = d
            print(f"✅ Found {name}")

    # Connect to available devices
    connect_tasks = []
    for name, device in BLE_SERVER_NAMES.items():
        if device:
            connect_tasks.append(connect_to_device(device))
    await asyncio.gather(*connect_tasks)


async def main():
    await scan_and_connect()

    try:
        while True:
            # Check connection status
            for name, client in list(connected_clients.items()):
                if not client.is_connected:
                    print(f"🔌 {name} disconnected. Removing from active list.")
                    del connected_clients[name]
            await asyncio.sleep(1)
    except KeyboardInterrupt:
        print("🛑 Shutting down...")
        for client in connected_clients.values():
            await client.disconnect()


if __name__ == "__main__":
    asyncio.run(main())
