import serial
import serial.tools.list_ports
import csv
import os

def find_esp32_port():
    ports = serial.tools.list_ports.comports()
    for port in ports:
        # Most ESP32s use CP210x or CH340 drivers
        if "CP210" in port.description or "USB" in port.description:
            return port.device
    return None

def run_harvester():
    port = find_esp32_port()
    if not port:
        print("[!] No ESP32 detected. Check connection!")
        return

    print(f"[*] Auto-detected ESP32 on {port}...")
    
    try:
        # Open Serial Port
        ser = serial.Serial(port, 115200, timeout=1)
        
        if not os.path.exists("focus_data.csv"):
            with open("focus_data.csv", 'w', newline='') as f:
                writer = csv.writer(f)
                writer.writerow(["Session_ID", "Unix_Timestamp", "Duration_Sec"])

        print("[*] Ready! Trigger 'DATA DUMP' on your OLED now...")

        while True:
            if ser.in_waiting > 0:
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                if "," in line and "Session_ID" not in line:
                    parts = line.split(",")
                    with open("focus_data.csv", 'a', newline='') as f:
                        writer = csv.writer(f)
                        writer.writerow(parts)
                    print(f"[+] Saved Session: {parts}")
    except Exception as e:
        print(f"[!] Access Denied! Close VS Code Monitor first. Error: {e}")

if __name__ == "__main__":
    run_harvester()

# --- CONFIG ---
PORT = 'COM11'  # Match this to your PlatformIO port
BAUD = 115200
FILENAME = "focus_data.csv"

def run_harvester():
    print(f"[*] Initializing Harvester on {PORT}...")
    
    try:
        # Open Serial Port
        ser = serial.Serial(PORT, BAUD, timeout=1)
        
        # Prepare CSV file with headers if it's new
        if not os.path.exists(FILENAME):
            with open(FILENAME, 'w', newline='') as f:
                writer = csv.writer(f)
                writer.writerow(["Session_ID", "Unix_Timestamp", "Duration_Sec"])

        print("[*] Ready! Trigger 'DATA DUMP' on your ESP32 now...")

        while True:
            if ser.in_waiting > 0:
                # Read line from ESP32
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                
                # If the line looks like CSV data (e.g., "0,1706271234,30")
                if "," in line and "Session_ID" not in line:
                    parts = line.split(",")
                    with open(FILENAME, 'a', newline='') as f:
                        writer = csv.writer(f)
                        writer.writerow(parts)
                    print(f"[+] Saved Session: {parts}")

    except Exception as e:
        print(f"[!] Error: {e}")
    finally:
        print("[*] Closing Harvester.")

if __name__ == "__main__":
    run_harvester()