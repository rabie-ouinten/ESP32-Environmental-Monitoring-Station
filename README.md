# ESP32 Environmental Monitoring Station

## Overview

An environmental monitoring station based on the ESP32. It measures **temperature**, **relative humidity** and **light intensity**, shows the values locally on a 16×2 LCD, and serves a live **web dashboard** directly from the ESP32 (no router, no cloud).

The project combines embedded programming, sensor interfacing, a web interface, and a custom PCB designed in KiCad.

## Features

- Temperature and humidity measurement (DHT11)
- Light intensity measurement (LDR, raw 12-bit ADC value, 0–4095)
- Local display on a 16×2 LCD with I²C interface
- Built-in Wi-Fi access point: connect to the ESP32 and open the dashboard at `http://192.168.4.1`
- Live charts for temperature, humidity and light (last 20 measurements)
- Measurement every 2 seconds, stored in a circular buffer of 1000 records
- Data export in CSV format
- Custom schematic and PCB designed in KiCad (USB-C, battery charging, 3.3 V buck-boost supply)

## Hardware

### Prototype (tested)

|         Component        |       Details                  |
|     -----------------    |      -----------------         |
|      Microcontroller     | ESP32 development board (ESP-WROOM-32, "ESP32 Dev Module") |
|   Temperature / humidity |     DHT11 on GPIO 18           |
|       Light sensor       |     LDR on GPIO 34 (ADC input) |
|          Display         |     16×2 LCD with I²C backpack |

### Custom PCB (KiCad)

|       Block       | Components               |
|        ---        |     ---                  |
|  Microcontroller  | ESP32-S3-WROOM-1         |
|    USB-C input    | USB-C receptacle with CC1/CC2 resistors, fuse                       |
|     Protection    | USBLC6-2SC6 (ESD on data lines), TVS0500DRV (VBUS surge protection) |
|  Battery charging | TP4056 Li-ion charger with battery connector                        |
|   3.3 V supply    | TPS63001 buck-boost converter        |
|      Sensors      | DHT11, LDR voltage divider           |
|     Expansion     | I²C connector with pull-up resistors |
|   User controls   | BOOT and EN (reset) buttons          |

> **Note:** the firmware was developed and validated on an ESP32 development board. The PCB targets the ESP32-S3-WROOM-1; GPIO assignments may need to be adapted when moving the firmware to the custom board.

## Software and Tools

- Arduino IDE 2.3.7, C++ / Arduino framework
- ESP32 Wi-Fi web server (access point mode)
- HTML, CSS and JavaScript for the dashboard
- KiCad for schematic capture and PCB design

## Project Structure

```
.
├── firmware/    # ESP32 source code (Arduino sketch)
├── hardware/    # KiCad schematic and PCB design files
└── image/      # Screenshots, photos, project report
```

## Getting Started

1. Install the [Arduino IDE](https://www.arduino.cc/en/software).
2. Install the ESP32 board support package and select **ESP32 Dev Module**.
3. Install the required libraries: a DHT sensor library and an I²C LCD library (for example `DHT sensor library` and `LiquidCrystal_I2C`).
4. Open `firmware/station_environnementale/station_environnementale.ino`.
5. Check the pin definitions at the top of the sketch:
   ```cpp
   #define DHT_PIN   18
   #define DHT_TYPE  DHT11
   #define LDR_PIN   34
   ```
6. Upload the sketch to the ESP32.
7. Open the Serial Monitor at **9600 baud** to check the readings.
8. On your phone or computer, connect to the Wi-Fi network created by the ESP32, then open `http://192.168.4.1`.

## Wiring (prototype)

|     Signal     | ESP32 pin |
|       ---      |    ---    |
|  DHT11 data    |  GPIO 18  |
| LDR divider output | GPIO 34 |
|     LCD SDA    | GPIO 21 (default I²C) |
|     LCD SCL    | GPIO 22 (default I²C) |

Adjust the LCD pins and I²C address (commonly `0x27` or `0x3F`) to match your hardware.

## Configuration

|      Parameter       | Default |       Description        |
|         ---          |   ---   |          ---             |
| `SAMPLE_INTERVAL_MS` |   2000  |     Time between two measurements              |
|     `MAX_RECORDS`    |   1000  | Size of the circular buffer used for recording |

## Web Interface

- Live charts of temperature, humidity and light intensity
- History of the last 20 measurements
- CSV export of the recorded data

## Known Limitations

- The DHT11 has limited accuracy (about ±2 °C and ±5 % RH) and a slow response time.
- The LDR output is a raw ADC value and is **not calibrated** to lux.
- The 1000-record history is stored in RAM and is lost on reset or power loss.
- The PCB has not yet been fully validated.

## Future Improvements

- Calibrate the light measurement (ADC value to lux)
- Replace the DHT11 with a more accurate sensor such as the BME280 (I²C)
- Enhance the web interface
- Add new sensors through the I²C expansion connector
- Build and validate the custom PCB, then optimize the design

## License

This project is distributed under the license specified in the `LICENSE` file.
