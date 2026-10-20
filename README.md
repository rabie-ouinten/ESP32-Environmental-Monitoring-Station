# ESP32 Environmental Monitoring Station

## Overview

This project consists of an environmental monitoring station based on the ESP32 microcontroller. It measures temperature, relative humidity, and light intensity, with local data visualization and remote monitoring through a web interface.

The project combines embedded programming, sensor interfacing, web technologies, and PCB design using KiCad.

## Features

- Temperature and humidity measurement
- Light intensity monitoring using an LDR
- Local display using a 16×2 LCD with I²C interface
- Web interface for monitoring measurements
- Data recording and export in CSV format
- Custom electronic circuit and PCB design using KiCad

## Hardware Components

- ESP32 development board (ESP-WROOM-32)
- DHT11 temperature and humidity sensor
- Light-dependent resistor (LDR)
- 16×2 LCD with I²C interface
- Additional electronic components for the circuit

## Software and Tools

- Arduino IDE
- C++ / Arduino framework
- ESP32 Wi-Fi web server
- HTML, CSS, and JavaScript
- KiCad for schematic capture and PCB design

## Project Structure

- `firmware/` — ESP32 source code
- `hardware/` — KiCad schematic and PCB design files

## Getting Started

1. Install the Arduino IDE.
2. Install the ESP32 board support package.
3. Install the required libraries for the DHT11 sensor and I²C LCD.
4. Open the Arduino source file.
5. Configure the Wi-Fi access point and upload the program to the ESP32.
6. Connect to the ESP32 access point and open the web interface.

## PCB Design

The electronic schematic and printed circuit board are designed using KiCad.

The hardware design files are available in the `hardware/` directory.

## Future Improvements

- Improve light intensity calibration
- Enhance the web interface
- Add new sensors and monitoring features
- Optimize the PCB design

## License

This project is distributed under the license specified in the `LICENSE` file.

