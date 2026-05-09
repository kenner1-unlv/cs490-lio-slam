# Orinbot GPS CYD Monitor

Standalone ESP32 Cheap Yellow Display GPS monitor for Orinbot.

This module reads a Waveshare / Quectel LC29H dual-band GNSS board over UART, displays live GPS status on a Cheap Yellow Display ESP32 TFT, connects to Wi-Fi, syncs local time over NTP, and broadcasts converted GPS telemetry over UDP for the Jetson Orin to receive.

## Current status

Working features:

- LC29H GPS UART input at 115200 baud
- TinyGPS++ NMEA parsing
- TFT dashboard display
- Wi-Fi connection screen
- NTP local time sync
- UDP broadcast of converted GPS data
- No raw NMEA spam over UDP

## Hardware

### GPS module

Waveshare LC29H series dual-band GPS / RTK HAT.

Useful references:

- Waveshare LC29H GPS/RTK HAT wiki: https://www.waveshare.com/wiki/LC29H%28XX%29_GPS/RTK_HAT
- Quectel LC29H / LC79H GNSS protocol specification: https://files.waveshare.com/wiki/LC29H%28XX%29-GPS-RTK-HAT/Quectel_LC29H%26LC79H_Series_GNSS_Protocol_Specification_V1.1.pdf
- Amazon product listing used for this build: https://www.amazon.com/Waveshare-Dual-Band-Positioning-Technology-Correction/dp/B0CGX8V2JF

### Display / ESP32 board

Cheap Yellow Display ESP32 board, commonly ESP32-2432S028 / ESP32-2432S028R.

Useful references:

- Random Nerd Tutorials CYD guide: https://randomnerdtutorials.com/cheap-yellow-display-esp32-2432s028r/
- CYD community documentation repo: https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display
- Original ESP32-2432S028 specification PDF mirror: https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display/blob/main/OriginalDocumentation/2-Specification/ESP32-2432S028%20Specifications-EN.pdf

## Wiring strategy

Final working approach:

- GPS board powered separately from USB during test, or from 5V/VIN for integrated build.
- GPS board jumper set to UART / Pi mode.
- LC29H TXD routed to ESP32 GPIO 27.
- Shared ground between GPS board and ESP32.
- ESP32 USB used for firmware upload and serial debugging.

Important detail:

The LC29H carrier expects 5V input. Earlier 3.3V power caused bad/noisy UART behavior.

## GPS settings

Observed working GPS UART:

- Baud: 115200
- GPS output: NMEA
- ESP32 RX pin: GPIO 27
- GPS module TXD connects to ESP32 GPIO 27
- Shared ground required

Useful NMEA sentences observed:

- GNGGA
- GNRMC
- GNGLL
- GNGSA
- GPGSV
- GLGSV
- GAGSV
- GBGSV

Example parsed fix:

- Latitude: 36.1372051
- Longitude: -115.2195476
- Altitude: 684.12 m
- Satellites: 51
- HDOP: 0.49

## UDP strategy

The ESP32 broadcasts converted GPS data over Wi-Fi UDP.

- UDP port: 5005
- Target: broadcast address 255.255.255.255
- Orin should listen on 0.0.0.0:5005

Example packet:

{"lat":36.1372051,"lon":-115.2195476,"alt":684.12,"sats":51,"hdop":0.49,"age_ms":322,"time":"20:15:03"}

The Jetson Orin can listen on UDP port 5005 and convert this into a ROS2 sensor_msgs/NavSatFix message.

## Strategy

This module keeps the ESP32 as a small IoT sensor/display node.

Data path:

LC29H GPS -> ESP32 CYD over UART -> Wi-Fi UDP -> Jetson Orin -> ROS2 bridge

This avoids running a long serial line into the Orin and keeps the GPS/display module useful even when disconnected from the robot.

## Planned next steps

- Add polished static UI backgrounds from SD card.
- Add touch keyboard screen for Wi-Fi SSID/password entry.
- Store Wi-Fi config on SD card or NVS.
- Add Orin-side UDP-to-ROS2 bridge.
- Add local CSV GPS logging to SD card.
