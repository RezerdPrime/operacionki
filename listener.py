import serial

PORT = 'COM4'      # парный к тому, куда пишет эмулятор
BAUDRATE = 9600

try:
    with serial.Serial(PORT, BAUDRATE, timeout=1) as ser:
        print(f"Слушаю порт {PORT} ({BAUDRATE} бод)...")
        while True:
            line = ser.readline()
            if line:
                print(line.decode('utf-8').strip())
except serial.SerialException as e:
    print(f"Ошибка: {e}")
except KeyboardInterrupt:
    print("\nПриём остановлен.")