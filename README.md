# Friendship Lamps
ELEC327 Project

Things to Program: MSPM0, ESP32, LED, Amplifier, Button

List of Coding Tasks:
- UART Communication RX and TX between MSPM0 and ESP32
- I2S Communication from ESP32 to Amplifier 
- SPI Communication from MSPM0 to LED Strip and Button
- Wifi communication between two different ESP32's (I found a resource for this: https://esp32io.com/tutorials/communication-between-two-esp32)


## UART Connection

Setup Instructions
- download the esp32_WROOM_...... --> more instructions below for this
- embedded in tshi folder is a requirements.txt file for knowing which packages to pip install
- I used windows command prompt and CCS
- import the code as a project in CCS
- Download PuTTY

Folder Download:
Link: https://docs.espressif.com/projects/esp-at/en/latest/esp32/AT_Binary_Lists/esp_at_binaries.html#firmware-esp32-wroom-32-series
- ON this link download "v4.1.1.0 ESP32-WROOM-32-AT-V4.1.1.0.zip (Recommended)" as a zip file
- then extract that zip folder and all contents are embedded like 3 layers in but its fine it doesnt matter
- Then change a couple things in that extracted folder. 1) replace the customized_partitions folder. I have emailed you what to replace it with, 2) add the requirements.txt file. This requirements.txt file is all the packages you need to pip install. 


What I was currently working on:
Sending the string 'AT\r\n' from MSPM0. In CCS if you watch atResult, uartResult, and rxBuffer in the Watch window, we see MSPM0's own transmitted string is just echoed back. Still no 'OK' from the ESP32. The MSPM0 can transmit fine (we see echoes) but receives nothing back from the ESP32.

Side note:  To test ESP32 booting: open PuTTY at Serial, COM7, 115200 baud rate

Also right now this is the wiring setup I'm using:
- LP PA10 (TX)  ──→  ESP32 IO16  
- LP PA11 (RX)  ←──  ESP32 IO17  
- LP GND        ────  ESP32 GND
- LP 3.3V       _____ ESP32 3V3

I'm not using the ESP32 GPIO 1 and 3 (which are the RX TX pins on the ESP32) because the factory default is GPIO 16 and 17 for uart. I tried to change that configuration locally through MSPM0 config files but it didn't flash correctly so I'm gonna leave out those local files. I know it doesnt work because on boot (check stream in PuTTY), ESP32 still says its UART pins are 16 (RX) and 17 (TX). So we have to check ESP32 TX to RX pin on MSPM0. That's what I was doing now. 

Note: I also just came across this repo: https://github.com/espressif/esp-serial-flasher/tree/master. This seems very useful because it is a repo for controll ESP chips through other host microcontrollers. This doesn't include the MSPM0 but may help still. 
