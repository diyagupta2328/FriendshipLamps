/*
 * ============================================================
 * FRIENDSHIP LAMP — ESP32 Firmware for LAMP 2
 * ============================================================
 *
 * IDENTICAL to Lamp 1 except swapped MQTT topics:
 *   Publishes to:   friendship-lamp/lamp2
 *   Subscribes to:  friendship-lamp/lamp1
 *
 * Everything else — wiring, I2S, melody, UART — identical.
 *
 * *** FILL IN WIFI CREDENTIALS BELOW ***
 * ============================================================
 */

#include <WiFi.h>
#include <PubSubClient.h>
#include <driver/i2s.h>
#include <math.h>

/* ── WiFi credentials ─────────────────────────────────────── */
const char* WIFI_SSID     = "Tyler";      /* <-- CHANGE */
const char* WIFI_PASSWORD = "Mosasaurus";   /* <-- CHANGE */

/* ── MQTT ─────────────────────────────────────────────────── */
const char* MQTT_BROKER     = "broker.hivemq.com";
const int   MQTT_PORT       = 1883;
const char* TOPIC_PUBLISH   = "friendship-lamp/lamp2";  /* swapped */
const char* TOPIC_SUBSCRIBE = "friendship-lamp/lamp1";  /* swapped */

/* ── I2S pins ────────────────────────────────────────────── */
#define I2S_BCLK_PIN   26
#define I2S_LRCLK_PIN  25
#define I2S_DATA_PIN   27

/* ── Audio config ────────────────────────────────────────── */
#define SAMPLE_RATE     44100
#define AUDIO_BUF_SIZE  512

/* ── State ───────────────────────────────────────────────── */
WiFiClient   wifiClient;
PubSubClient mqttClient(wifiClient);
volatile bool playMusicFlag  = false;
volatile bool sendButtonFlag = false;

/* ────────────────────────────────────────────────────────── */
/* MELODY — same tune as Lamp 1                               */
/* ────────────────────────────────────────────────────────── */
struct Note { int freq; int dur; };

static const Note melody[] = {
    {523, 400}, {587, 400}, {659, 400}, {698, 400},
    {784, 600}, {0,   200},
    {784, 400}, {740, 300}, {698, 300}, {659, 400},
    {587, 600}, {0,   200},
    {523, 300}, {587, 300}, {659, 300}, {784, 300},
    {880, 500}, {784, 300}, {0,   150},
    {698, 400}, {659, 400}, {587, 400}, {523, 600},
    {0,   300},
    {659, 350}, {698, 350}, {784, 350}, {880, 350},
    {988, 500}, {0,   200},
    {880, 350}, {784, 350}, {698, 350}, {659, 350},
    {587, 500}, {0,   200},
    {523, 300}, {659, 300}, {784, 300}, {880, 300},
    {1047,500}, {880, 300}, {784, 300},
    {698, 400}, {659, 400}, {587, 600}, {523, 800},
    {0,   500},
};
static const int MELODY_LEN = sizeof(melody) / sizeof(melody[0]);

/* ────────────────────────────────────────────────────────── */
/* I2S SETUP                                                  */
/* ────────────────────────────────────────────────────────── */
void setupI2S()
{
    i2s_config_t cfg = {
        .mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate          = SAMPLE_RATE,
        .bits_per_sample      = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format       = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count        = 8,
        .dma_buf_len          = AUDIO_BUF_SIZE,
        .use_apll             = false,
        .tx_desc_auto_clear   = true,
    };
    i2s_pin_config_t pins = {
        .bck_io_num   = I2S_BCLK_PIN,
        .ws_io_num    = I2S_LRCLK_PIN,
        .data_out_num = I2S_DATA_PIN,
        .data_in_num  = I2S_PIN_NO_CHANGE,
    };
    i2s_driver_install(I2S_NUM_0, &cfg, 0, NULL);
    i2s_set_pin(I2S_NUM_0, &pins);
    i2s_zero_dma_buffer(I2S_NUM_0);
}

/* ────────────────────────────────────────────────────────── */
/* TONE GENERATION                                            */
/* ────────────────────────────────────────────────────────── */
void playTone(int freq_hz, int dur_ms)
{
    int samples = (SAMPLE_RATE * dur_ms) / 1000;
    int16_t buf[AUDIO_BUF_SIZE * 2];
    float phase = 0.0f;
    float inc   = (freq_hz > 0) ? (2.0f * M_PI * freq_hz / SAMPLE_RATE) : 0;
    int written = 0;
    while (written < samples)
    {
        int chunk = min(AUDIO_BUF_SIZE, samples - written);
        for (int i = 0; i < chunk; i++)
        {
            int16_t s = (freq_hz > 0) ? (int16_t)(8000 * sinf(phase)) : 0;
            buf[i*2] = buf[i*2+1] = s;
            phase += inc;
            if (phase > 2.0f * M_PI) phase -= 2.0f * M_PI;
        }
        size_t bw;
        i2s_write(I2S_NUM_0, buf, chunk * 4, &bw, portMAX_DELAY);
        written += chunk;
    }
}

/* ────────────────────────────────────────────────────────── */
/* PLAY FULL MELODY (~15 seconds)                             */
/* ────────────────────────────────────────────────────────── */
void playMelody()
{
    for (int i = 0; i < MELODY_LEN; i++)
        playTone(melody[i].freq, melody[i].dur);
    i2s_zero_dma_buffer(I2S_NUM_0);
}

/* ────────────────────────────────────────────────────────── */
/* WIFI                                                        */
/* ────────────────────────────────────────────────────────── */
void connectWiFi()
{
    Serial.print("Connecting to WiFi...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.print("Ping test: ");
    Serial.println(WiFi.gatewayIP());
    Serial.print("WiFi connected. IP: ");
    Serial.println(WiFi.localIP());
}

/* ────────────────────────────────────────────────────────── */
/* MQTT CALLBACK                                              */
/* Called when partner (Lamp 1) pressed their button         */
/* ────────────────────────────────────────────────────────── */
void mqttCallback(char* topic, byte* payload, unsigned int length)
{
    String msg = "";
    for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];
    Serial.print("[MQTT RX] topic=");
    Serial.print(topic);
    Serial.print(" msg=");
    Serial.println(msg);

    if (msg == "PRESS")
    {
        Serial.println("[ACTION] Partner pressed button!");
        Serial.println("[ACTION] Sending ACTIVE to MSPM0...");
        Serial.println("ACTIVE\r\n");
        Serial.println("[ACTION] Setting playMusicFlag...");
        playMusicFlag = true;
    }
}

/* ────────────────────────────────────────────────────────── */
/* MQTT CONNECT                                               */
/* ────────────────────────────────────────────────────────── */
void connectMQTT()
{
    while (!mqttClient.connected())
    {
        Serial.print("Connecting to MQTT...");
        String clientId = "lamp2-" + WiFi.macAddress();
        clientId.replace(":", "");
        if (mqttClient.connect(clientId.c_str()))
        {
            Serial.println(" connected.");
            mqttClient.subscribe(TOPIC_SUBSCRIBE);
            Serial.print("Subscribed to: ");
            Serial.println(TOPIC_SUBSCRIBE);
        }
        else
        {
            Serial.print(" failed rc=");
            Serial.print(mqttClient.state());
            Serial.println(" retrying in 3s...");
            delay(3000);
        }
    }
}

/* ────────────────────────────────────────────────────────── */
/* SETUP                                                      */
/* ────────────────────────────────────────────────────────── */
void setup()
{
    Serial.begin(115200);
    delay(500);

    setupI2S();
    connectWiFi();

    mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
    mqttClient.setCallback(mqttCallback);
    mqttClient.setSocketTimeout(10);
    connectMQTT();

    Serial.println("Lamp 2 ready.");
}

/* ────────────────────────────────────────────────────────── */
/* MAIN LOOP                                                  */
/* ────────────────────────────────────────────────────────── */

void loop()
{
    if (!mqttClient.connected())
    {
        Serial.println("[WARN] MQTT disconnected — reconnecting...");
        connectMQTT();
    }
    mqttClient.loop();

    static String uartBuf = "";
    while (Serial.available())
    {
        char c = (char)Serial.read();
        if (c == '\n')
        {
            uartBuf.trim();
            Serial.print("[UART RX] received: '");
            Serial.print(uartBuf);
            Serial.println("'");
            if (uartBuf == "BTN")
            {
                Serial.println("[ACTION] BTN detected — publishing PRESS...");
                sendButtonFlag = true;
            }
            uartBuf = "";
        }
        else if (c != '\r')
        {
            uartBuf += c;
        }
    }

    if (sendButtonFlag)
    {
        sendButtonFlag = false;
        if (mqttClient.publish(TOPIC_PUBLISH, "PRESS"))
            Serial.println("[MQTT TX] Published PRESS successfully");
        else
            Serial.println("[MQTT TX] PUBLISH FAILED!");
    }

    if (playMusicFlag)
    {
        Serial.println("[MUSIC] Starting melody...");
        playMusicFlag = false;
        playMelody();
        Serial.println("[MUSIC] Melody done.");
    }

    delay(10);
}