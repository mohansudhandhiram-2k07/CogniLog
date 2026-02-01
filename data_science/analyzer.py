import pandas as pd
import matplotlib.pyplot as plt

FILENAME = "focus_data.csv"

try:
    # 1. Load data, skipping lines that aren't properly formatted CSV
    df = pd.read_csv(FILENAME, on_bad_lines='skip')
    
    # 2. Fix the Column Mismatch
    # We rename 'Unix_Time' to 'Unix_Timestamp' if it exists, or vice versa
    if 'Unix_Time' in df.columns:
        df.rename(columns={'Unix_Time': 'Unix_Timestamp'}, inplace=True)
    
    # 3. Clean the data (ensure we only have numbers)
    df['Unix_Timestamp'] = pd.to_numeric(df['Unix_Timestamp'], errors='coerce')
    df['Duration_Sec'] = pd.to_numeric(df['Duration_Sec'], errors='coerce')
    df = df.dropna() # Remove any rows that weren't numbers

    # 4. Convert to Readable Time
    df['Time'] = pd.to_datetime(df['Unix_Timestamp'], unit='s')

    # 5. Plotting
    plt.style.use('seaborn-v0_8-darkgrid')
    plt.figure(figsize=(10, 5))
    plt.plot(df['Time'], df['Duration_Sec'], marker='o', color='#007acc', linewidth=2)
    
    plt.title('CogniLog: Productivity Analysis', fontsize=14)
    plt.xlabel('Session Date/Time', fontsize=12)
    plt.ylabel('Focus Duration (Seconds)', fontsize=12)
    plt.xticks(rotation=30)
    plt.tight_layout()

    # Save and Show
    plt.savefig("focus_analytics.png")
    print("[*] Analysis Success! Check 'focus_analytics.png' for your graph.")
    plt.show()

except Exception as e:
    print(f"[!] Error: {e}")
    print("Tip: Open your CSV in Notepad and make sure the first line is exactly:")
    print("Session_ID,Unix_Timestamp,Duration_Sec")