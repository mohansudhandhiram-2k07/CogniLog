import socket
import time
import pygetwindow as gw

# --- CONFIGURATION ---
ESP32_IP = "10.209.209.88"  # <--- REPLACE with your ESP32's IP from Serial Monitor
PORT = 4210
INTERVAL = 2  # Check every 2 seconds

# --- HEURISTICS ---
FOCUS_APPS = ["visual studio code", "terminal", "github", "stack overflow", "cursor", "matlab"]
EDU_KEYWORDS = ["tutorial", "lecture", "course", "iitm", "embedded", "electronics", "circuit", "datascience", "math"]

def get_status():
    try:
        window = gw.getActiveWindowTitle().lower()
        
        # 1. Check if user is on YouTube
        if "youtube" in window:
            if any(key in window for key in EDU_KEYWORDS):
                return "FOCUS"  # Studying on YT
            return "DISTRACTED" # Distracted on YT
            
        # 2. Check for productivity apps
        if any(app in window for app in FOCUS_APPS):
            return "FOCUS"
            
        # 3. Default to distracted for everything else (Social Media, etc.)
        return "DISTRACTED"
    except Exception:
        return "IDLE"

# UDP Setup
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

print(f"--- CogniLog Watchdog Started ---")
print(f"Targeting ESP32 at: {ESP32_IP}")

try:
    while True:
        status = get_status()
        # Send to ESP32
        sock.sendto(status.encode(), (ESP32_IP, PORT))
        print(f"[SENT] Status: {status} | Window: {gw.getActiveWindowTitle()[:30]}...")
        time.sleep(INTERVAL)
except KeyboardInterrupt:
    print("\nWatchdog stopped.")