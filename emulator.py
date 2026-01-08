import serial
import time
import random
from datetime import datetime

PORT = 'COM3'
BAUDRATE = 9600
INTERVAL = 2

def generate_temperature():
    return round(22.0 + random.uniform(-2.0, 2.0), 2)

def emulate_sensor():
    try:
        with serial.Serial(PORT, BAUDRATE, timeout=1) as ser:
            print(f"Эмулятор запущен на {PORT}")
            while True:
                temp = generate_temperature()
                timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
                message = f"T:{temp:.2f} @ {timestamp}\n"
                ser.write(message.encode('utf-8'))
                print(f"Отправлено: {message.strip()}")
                time.sleep(INTERVAL)
    except serial.SerialException as e:
        print(f"Ошибка порта: {e}")
    except KeyboardInterrupt:
        print("\nОстановлено.")

if __name__ == "__main__":
    emulate_sensor()