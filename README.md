# Room_Environment_Manager

ESP32-based environment manager with Slack



## About

This project can do

* measure temperature, humidity, atmospheric pressure, and CO2 concentration
* show room environment on display
* send room environment on Slack every 1 hour
* send room environment graph (bitmap image) on Slack
* control an air conditioner via Slack

Graphs:

![graph](image/graph.png)

## Hardware

You need

* Main Module:
  * [ESP32 Dev Module (4MB)](https://akizukidenshi.com/catalog/g/g115673/)
  * [SHT31](https://akizukidenshi.com/catalog/g/g112125/)
    * Temperature & Humidity Sensor
    * I2C

  * [BME680](https://akizukidenshi.com/catalog/g/g114469/)
    * Temperature & Humidity & Atmospheric Pressure Sensor
    * I2C

  * [MH-Z19C](https://akizukidenshi.com/catalog/g/g116142/)
    * CO2 Sensor
    * Serial (RX=16, TX=17)

  * [SC2004CSLB](https://akizukidenshi.com/catalog/g/g100036/)
    * LCD Character Module (20x4)

* Air Conditioner Control Module:
  * [XIAO ESP32C3](https://akizukidenshi.com/catalog/g/g117454/)
  * Infrared LED
  * 2SC1815 Transistor
  * Some Resistors

* Additional Sensor Module 1:
  * [XIAO ESP32C3](https://akizukidenshi.com/catalog/g/g117454/)
  * [SHT40I](https://akizukidenshi.com/catalog/g/g130207/)
    * Temperature & Humidity Sensor
    * I2C

* Additional Sensor Module 2:
  * [XIAO ESP32C3](https://akizukidenshi.com/catalog/g/g117454/)
  * [BME280](https://akizukidenshi.com/catalog/g/g109421/)
    * Temperature & Humidity & Atmospheric Pressure Sensor
    * I2C
* Air Conditioner
  * I used Panasonic's one



## Hardware (Lite version)

* [XIAO ESP32C3](https://akizukidenshi.com/catalog/g/g117454/)
* [BME280](https://akizukidenshi.com/catalog/g/g109421/)
  * Temperature & Humidity & Atmospheric Pressure Sensor
  * I2C
* Infrared LED
* 2SC1815 Transistor
* Some Resistors



## Slack

You need

* Slack bot token
  * channels:history
  * channels:write
  * files:write