import asyncio
import sqlite3
import time
from bleak import BleakClient, BleakScanner

# --- BLE Configuration ---
SMARTLIGHT_SERVICE_UUID = "12345678-1234-5678-1234-56789abcdef0"
MOTION_CHARACTERISTIC_UUID = "12345678-1234-5678-1234-56789abcdef1"
LED_CONTROL_CHARACTERISTIC_UUID = "12345678-1234-5678-1234-56789abcdef2"

# --- Inactivity Timing (in seconds) ---
standby_timeout = 5   # After 5 seconds of motion inactivity -> standby (2 LEDs)
off_timeout = 10      # After 10 seconds of no motion -> turn off lights

# --- SQLite Configuration ---
DB_NAME = "smartlighting.db"

# Set up the SQLite database and table if they don't exist
def init_db():
    conn = sqlite3.connect(DB_NAME)
    c = conn.cursor()
    c.execute('''
        CREATE TABLE IF NOT EXISTS events (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp REAL,
            event TEXT,
            details TEXT
        )
    ''')
    conn.commit()
    conn.close()

def log_event(event_type, details):
    """Log an event to the SQLite database for later Grafana visualization."""
    conn = sqlite3.connect(DB_NAME)
    c = conn.cursor()
    c.execute('INSERT INTO events (timestamp, event, details) VALUES (?, ?, ?)',
              (time.time(), event_type, details))
    conn.commit()
    conn.close()
    print(f"Logged event: {event_type}, {details}")

# Global variable to track last motion time
last_motion_time = None

async def notification_handler(sender, data):
    """Handle motion sensor notifications from the peripheral."""
    global last_motion_time
    # Data is a byte array: 1 indicates motion, 0 indicates no motion.
    motion = data[0]
    print(f"Received motion sensor data: {motion}")
    log_event("motion", f"Motion value: {motion}")
    if motion == 1:
        last_motion_time = asyncio.get_event_loop().time()

async def main():
    global last_motion_time
    init_db()  # Initialize the database
    print("Scanning for SmartLight peripherals...")
    devices = await BleakScanner.discover(timeout=5.0)
    target = None

    for d in devices:
        uuids = d.metadata.get("uuids", [])
        if any(SMARTLIGHT_SERVICE_UUID.lower() in u.lower() for u in uuids):
            target = d
            break

    if not target:
        print("No SmartLight peripheral found.")
        return

    print(f"Found device: {target.address} - {target.name}")
    async with BleakClient(target.address) as client:
        print("Connected to peripheral")
        await client.start_notify(MOTION_CHARACTERISTIC_UUID, notification_handler)
        last_motion_time = asyncio.get_event_loop().time()

        while True:
            await asyncio.sleep(1)
            current_time = asyncio.get_event_loop().time()
            elapsed = current_time - last_motion_time

            if elapsed < standby_timeout:
                # Recent motion: send full lighting command (4 LEDs on)
                command = bytearray([2])
                command_str = "Full mode (4 LEDs)"
            elif elapsed < off_timeout:
                # Intermediate inactivity: send standby command (2 LEDs on)
                command = bytearray([1])
                command_str = "Standby mode (2 LEDs)"
            else:
                # Extended inactivity: turn off lights
                command = bytearray([0])
                command_str = "Lights off"

            try:
                await client.write_gatt_char(LED_CONTROL_CHARACTERISTIC_UUID, command)
                log_event("led_command", command_str)
            except Exception as e:
                print(f"Failed to send command: {e}")

if __name__ == "__main__":
    asyncio.run(main())
