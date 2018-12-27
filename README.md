# ESP8266 MT7530 Controller

Use Espressif ESP8266 to control MT7530 switches through MDIO bus so that we can get more functionality on a cheap dumb switch.

This project currently supports configuring VLAN (including saving VLAN settings) and show link status changes.

## Compiling

The code here uses the official ESP8266_RTOS_SDK which can be found here:

https://github.com/espressif/ESP8266_RTOS_SDK

I'm using the master branch (currently v3.1) and you can find compiling guide there.

## Usage

1. Set your MDIO/MDC GPIO in components/mdio_gpio/mdio_gpio.c and flash the binary into ESP8266 board.
2. There is a simple serial console available on UART0. Currently you should set everything using that.

## License

I took a lot of work from Linux kernel and OpenWrt so I guess at least the mdio and mt7530 module should be GPL.

## To-do

1. Create a webui for this.
2. Add more functions supported by MT7530 like QoS and ACL.
