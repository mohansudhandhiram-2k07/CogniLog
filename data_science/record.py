import serial
import wave
import time
import os

# --- CONFIGURATION ---
PORT = 'COM11'      # Must match your ESP32 port
BAUD = 921600       # The new high-speed baud rate
SAMPLE_RATE = 16000 # 16kHz TinyML standard
RECORD_SECONDS = 2  # Length of each audio clip
CHANNELS = 1        # Mono audio
SAMPLE_WIDTH = 2    # 16-bit audio = 2 bytes per sample

# Create a folder for the dataset
if not os.path.exists("dataset"):
    os.makedirs("dataset")

print(f"Connecting to {PORT} at {BAUD} baud...")
try:
    ser = serial.Serial(PORT, BAUD)
except Exception as e:
    print(f"Error opening port. Make sure Teleplot/PlatformIO monitors are CLOSED. \n{e}")
    exit()

def record_sample(filename):
    print(f"🔴 RECORDING {RECORD_SECONDS}s... Speak now!")
    frames = bytearray()
    bytes_to_read = SAMPLE_RATE * RECORD_SECONDS * SAMPLE_WIDTH

    ser.reset_input_buffer() # Flush old data sitting in the wire
    
    while len(frames) < bytes_to_read:
        waiting = ser.in_waiting
        if waiting > 0:
            # Snatch the raw binary chunks as they arrive
            chunk = ser.read(min(waiting, bytes_to_read - len(frames)))
            frames.extend(chunk)

    # Package the raw binary into a standard .wav file
    with wave.open(filename, 'wb') as wf:
        wf.setnchannels(CHANNELS)
        wf.setsampwidth(SAMPLE_WIDTH)
        wf.setframerate(SAMPLE_RATE)
        wf.writeframes(frames)
        
    print(f"✅ Saved to {filename}\n")

# --- THE RECORDING LOOP ---
try:
    print("\n--- COGNILOG DATASET COLLECTOR ---")
    while True:
        input("Press ENTER to record a sample (or Ctrl+C to quit)...")
        
        # Create a unique filename based on the current time
        timestamp = int(time.time())
        filepath = f"dataset/wakeword_{timestamp}.wav"
        
        record_sample(filepath)

except KeyboardInterrupt:
    print("\nDataset collection stopped. Close the serial port.")
    ser.close()