# FriendshipLamps
ELEC327 Project

Things to Program: MSPM0, ESP32, LED, Amplifier, Button

Let's separate tasks in different folders for now. 
- UART Communication RX and TX between MSPM0 and ESP32
- I2S Communication from ESP32 to Amplifier
- SPI Communication from MSPM0 to LED Strip and Button
- Wifi communication between two different ESP32's


UART Connection Notes

- Test ESP32 booting: open PuTTY at Serial, COM7, 115200 baud rate

What I was currently working on:
Sending the string 'AT\r\n' from MSPM0. In CCS if you watch atResult, uartResult, and rxBuffer in the Watch window, we see MSPM0's own transmitted string is just echoed back. Still no 'OK' from the ESP32. The MSPM0 can transmit fine (we see echoes) but receives nothing back from the ESP32.

Also right now this is the wiring setup I'm using:
LP PA10 (TX)  ──→  ESP32 IO16  
LP PA11 (RX)  ←──  ESP32 IO17  
LP GND        ────  ESP32 GND
LP 3.3V       _____ ESP32 3V3

I'm not using the ESP32 GPIO 1 and 3 (which are the RX TX pins on the ESP32) because the factory default is GPIO 16 and 17 for uart. I tried to change that configuration locally through MSPM0 config files but it didn't flash correctly so I'm gonna leave out those local files. I know it doesnt work because on boot (check stream in PuTTY), ESP32 still says its UART pins are 16 (RX) and 17 (TX). So we have to check ESP32 TX to RX pin on MSPM0. That's what I was doing now. 
