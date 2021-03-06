# Toyota Expansion Board

This is a firmware source code for [Expansion Board](https://adrian-007.eu/2021/02/21/toyota-expansion-board-intro/) I made for my Toyota Corolla after replacing stock radio unit.

Firmware was written in C++14 for ATmega88 MCU and features:

* DS18B20 temperature reading
* SSD1306 OLED display driver
* Six buttons reading via ADC
* Communication with Pioneer radio unit via digital potentiometer MCP42100
* UART output (logging)
