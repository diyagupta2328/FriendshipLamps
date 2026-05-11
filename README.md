# Friendship Lamps
ELEC327 Project
Team: Diya Gupta, Devin Von Arx, Tyler Do

A pair of connected lamps that communicate over WiFi — when a button is pressed on one lamp, the other lamp lights up and plays music. Built using an MSPM0G3507 microcontroller and an ESP32-WROOM-32E WiFi module.

---

## How It Works

1. User presses the button on Lamp 1
2. MSPM0 detects the press and sends a command to ESP32 #1 over UART
3. ESP32 #1 publishes a message to an MQTT broker over WiFi
4. ESP32 #2 (anywhere in the world on any WiFi) receives the message
5. ESP32 #2 sends a command to MSPM0 #2 and plays music through the amplifier
6. MSPM0 #2 lights up the LED strip for 15 seconds
7. The same happens in reverse when Lamp 2's button is pressed

---

## Hardware Components

| Component | Purpose |
|---|---|
| MSPM0G3507 LaunchPad | Main microcontroller — button, LEDs, UART |
| ESP32-WROOM-32E | WiFi module — MQTT communication, audio playback |
| MAX98357A I2S Amplifier | Drives the speaker from digital I2S audio |
| SK6812 NeoPixel LED Strip (30 LEDs) | Visual indicator — lights up for 15 seconds |
| 30mm Illuminated Arcade Button | User input with built-in LED |
| Speaker | Audio output |

---

## Wiring

### MSPM0 LaunchPad
| Signal | Pin | Connects to |
|---|---|---|
| UART TX | PA10 (J4) | ESP32 GPIO3 |
| UART RX | PA11 (J4) | ESP32 GPIO1 |
| LED strip data | PA27 | SK6812 DIN (white wire) |
| Button input | PA13 | Button switch pin |
| 3.3V | Header pin 1 | Button switch pin 2 |
| 5V | Header pin 21 | Button LED+, LED strip VDD |
| GND | Header pin 22 | Button LED−, LED strip GND, ESP32 GND |

**Important:** J21 and J22 jumpers must be in **BP position**

### ESP32 DevKit
| Signal | Pin | Connects to |
|---|---|---|
| UART RX | GPIO3 | MSPM0 PA10 |
| UART TX | GPIO1 | MSPM0 PA11 |
| I2S BCLK | GPIO26 | MAX98357A BCLK |
| I2S LRCLK | GPIO25 | MAX98357A LRC |
| I2S Data | GPIO27 | MAX98357A DIN |

### MAX98357A Amplifier
| Pin | Connects to |
|---|---|
| Vin | 5V |
| GND | GND |
| SD | 3.3V (always on, left channel) |
| GAIN | Leave unconnected (9dB) |
| OUT+/OUT− | Speaker terminals |

---

## Software

### Files
| File | Target | Description |
|---|---|---|
| `lamp_main.c` | MSPM0 (both lamps) | Button detection, LED control, UART |
| `esp1_test.ino` | ESP32 Lamp 1 | WiFi, MQTT, I2S audio |
| `espt2_test.ino` | ESP32 Lamp 2 | WiFi, MQTT, I2S audio (swapped topics) |

### Dependencies
- **CCS (Code Composer Studio)** with MSPM0 SDK 2.x
- **Arduino IDE** with ESP32 board support
- **PubSubClient** library (Arduino) — install via Library Manager

### Configuration
Before flashing, update WiFi credentials in each ESP32 sketch:
```cpp
const char* WIFI_SSID     = "YOUR_WIFI_NAME";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
```

**Note:** ESP32 only supports 2.4GHz WiFi. If using an iPhone hotspot, enable **Maximize Compatibility** in Personal Hotspot settings.

---

## Communication Protocols

| Link | Protocol | Details |
|---|---|---|
| Button → MSPM0 | GPIO input | Active HIGH, pull-down, debounced |
| MSPM0 ↔ ESP32 | UART | 115200 baud, 8N1, PA10/PA11 |
| ESP32 → Amplifier | I2S | 44100 Hz, 16-bit stereo |
| MSPM0 → LED strip | NeoPixel 1-wire | SK6812 GRB, bit-banged on PA27 |
| ESP32 ↔ ESP32 | WiFi + MQTT | broker.hivemq.com, port 1883 |

---

## Flashing Instructions

### MSPM0
1. Open CCS, import `uart_echo_interrupts_standby` project example
2. Replace `main.c` with provided file
3. Verify SysConfig: 115200 baud UART, PA27 output, PA13 input with pull-down
4. Build and flash via XDS110 debugger (onboard LP) or external SWD debugger for PCB

### ESP32
1. **Disconnect MSPM0 UART wires before flashing** (they interfere with programming)
2. Open `.ino` file in Arduino IDE
3. Select board: ESP32 Dev Module, port: your COM port
4. Fill in WiFi credentials
5. Click Upload
6. Reconnect UART wires after flashing

---

## Troubleshooting

| Symptom | Fix |
|---|---|
| ESP32 won't flash | Disconnect LP wires from GPIO1/GPIO3 first |
| No MQTT connection | Check WiFi has internet access, use 2.4GHz, enable hotspot Maximize Compatibility |
| LEDs don't light | Confirm J21/J22 in BP position, PA27 PINCM60, 5V to strip |
| Button not detected | Check PA13 PINCM35, pull-down configured, switch wired to 3.3V |
| No sound | SD pin on MAX98357A tied to 3.3V? 5V to Vin? GPIO26/25/27 correct? |
| Only one direction works | Each lamp needs its own MSPM0 + ESP32 fully wired |
