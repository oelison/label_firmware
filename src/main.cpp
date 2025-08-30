#include <Arduino.h>
#include "driver/ledc.h"
#include "driver/rmt.h"
#include <SPI.h>
#include "WebPage.h"
#include "NVMData.h"
#include "DynamicData.h"

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  ((byte) & 0x80 ? '1' : '0'), \
  ((byte) & 0x40 ? '1' : '0'), \
  ((byte) & 0x20 ? '1' : '0'), \
  ((byte) & 0x10 ? '1' : '0'), \
  ((byte) & 0x08 ? '1' : '0'), \
  ((byte) & 0x04 ? '1' : '0'), \
  ((byte) & 0x02 ? '1' : '0'), \
  ((byte) & 0x01 ? '1' : '0') 


WebPage webPage;

#define PIN_MOTOR_PWM  25   // Motor PWM output
#define PIN_SENSOR     32   // light barrier input
#define PIN_5V_EN      23   // 5V enable output
#define PIN_LATCH      15   // SPI Chip Select
#define PIN_HEAT       17   // Write pattern
#define PIN_SCK         0   // SPI Clock
#define PIN_MISO        2   // SPI MISO (not used)
#define PIN_MOSI        4   // SPI MOSI (data out)
#define PIN_SS          5   // SPI Slave Select (not used)

const int delayTime_ms = 10;
const int SECONDS_1 = 1000 / delayTime_ms;
const int SECONDS_2 = 2000 / delayTime_ms;
const int SECONDS_5 = 5000 / delayTime_ms;
const int SECONDS_10 = 10000 / delayTime_ms;
const int SECONDS_100 = 100000 / delayTime_ms;
const int MILLISECONDS_50 = 50 / delayTime_ms;
const int MILLISECONDS_500 = 500 / delayTime_ms;

volatile uint32_t pulseCount = 0;
volatile uint32_t errorCount = 0;
volatile bool timerFlag = false;
volatile unsigned long lastMicros = 0;
volatile uint32_t periodMicros = 0;

hw_timer_t* timer = NULL;

void IRAM_ATTR onPulse() {
  unsigned long now = micros();
  unsigned long tryPeriodMicros = now - lastMicros;
  if(tryPeriodMicros > 2000) { // Entprellen: mind. 1ms Abstand
    lastMicros = now;
    periodMicros = tryPeriodMicros;
    pulseCount++;
  } else {
    errorCount++;
  }
}

void IRAM_ATTR onTimerISR() {
  timerFlag = true;
}

void heatPulse(int pinStrobe, int us) {
  digitalWrite(pinStrobe, LOW);
  delayMicroseconds(us);
  digitalWrite(pinStrobe, HIGH);
}

void setupWiFi()
{
  String hostname = "CasioKL780";
  if (NVMData::get().NetDataValid() == false)
  {
    Serial.println("Starting AP mode");
    DynamicData::get().setNewNetwork = true;
    Serial.println(WiFi.macAddress());
    WiFi.mode(WIFI_AP);
    Serial.println("Starting AP: " + hostname);
    WiFi.softAP(hostname.c_str(), "123Power");
    Serial.println("AP started");
    delay(1000);
    Serial.println(WiFi.softAPIP());
    DynamicData::get().ipaddress = WiFi.softAPIP().toString();
  }
  else
  {
    Serial.println("Starting STA mode");
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(hostname.c_str());
    //Serial.printf("SSID = %s\n", NVMData::get().GetNetName().c_str());
    WiFi.begin(NVMData::get().GetNetName().c_str(), NVMData::get().GetNetPassword().c_str());
    //Serial.println(WiFi.macAddress());
    int maxWaitForNet = 0;
    while (WiFi.status() != WL_CONNECTED)
    {
      delay(delayTime_ms);
      if (maxWaitForNet < SECONDS_1)
      {
        maxWaitForNet++;
      }
      else
      {
        //Serial.print(".");
        WiFi.begin(NVMData::get().GetNetName().c_str(), NVMData::get().GetNetPassword().c_str());
        DynamicData::get().incErrorCounter("Wifi startup");
        maxWaitForNet = 0;
      }
    }
    //Serial.println("");
    Serial.println("wiFi connected");
    //Serial.println(WiFi.localIP());
    //Serial.println(WiFi.dnsIP());
    DynamicData::get().ipaddress = WiFi.localIP().toString();
  }
  Serial.printf("IP address: %s\n", DynamicData::get().ipaddress.c_str());
}

void setup() {
    Serial.begin(115200);
    Serial.println("Starting...");
    NVMData::get().Init();
    setupWiFi();
    webPage.Init();
    // PWM einrichten: 15 kHz, 8 Bit Duty
    ledcSetup(0, 15000, 10); // channel 0, 15kHz, 10-bit resolution
    ledcAttachPin(PIN_MOTOR_PWM, 0);
    ledcWrite(0, 0); // stop Motor
    //Serial.println("LEDC test: 50% duty on GPIO25");
    // Lichtschranke als Input mit Interrupt
    pinMode(PIN_SENSOR, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(PIN_SENSOR), onPulse, RISING);

    // Timer für 100 ms Messfenster
    timer = timerBegin(0, 80, true); // 1 µs ticks @ 80 MHz
    timerAttachInterrupt(timer, [](){ onTimerISR(); }, true);
    timerAlarmWrite(timer, 100000, true); // 100ms
    timerAlarmEnable(timer);
    SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI, PIN_SS); // SCK=0, MISO=2, MOSI=4, SS=5
    SPI.setFrequency(2000000); // 2 MHz
    SPI.setDataMode(SPI_MODE0);
    SPI.setBitOrder(MSBFIRST);
    pinMode(PIN_5V_EN, OUTPUT);
    digitalWrite(PIN_5V_EN, LOW); // 5V für Sensor ausschalten
    pinMode(PIN_LATCH, OUTPUT);
    digitalWrite(PIN_LATCH, HIGH);
    pinMode(PIN_HEAT, OUTPUT);
    digitalWrite(PIN_HEAT, HIGH);
}

void checkNetworkSet()
{
  if(DynamicData::get().setNewNetwork == true)
  {
    if(webPage.newNetworkSet == true)
    {
      DynamicData::get().setNewNetwork = false;
      webPage.newNetworkSet = false;
      NVMData::get().StoreNetData();
    }
  }
  else
  {
    DynamicData::get().setNewNetwork = false;
  }
}

void pulseLow(int pin, int us) {
  digitalWrite(pin, LOW);
  delayMicroseconds(us);
  digitalWrite(pin, HIGH);
}

volatile uint32_t printCountState = 0;
static float freqFilt = 220.0;
static bool printing = false;


static uint8_t buffer[12] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

void loop() {
    static uint32_t lastCount = 0;
    static float integral = 0;
    static uint32_t lastPrint = 0;
    uint32_t p = periodMicros;
    uint32_t actCount = pulseCount;
    uint32_t duty = 0;
    if (webPage.startPrinting == true) {
        printing = true;
        webPage.startPrinting = false;
        digitalWrite(PIN_5V_EN, HIGH); // 5V für Sensor einschalten
        ledcWrite(0, 250);
        printCountState = 0;
        Serial.println("Printing started");
    }
    //pulseCount++;
    // Frequenz berechnen (wenn gültig)
    if (actCount != lastCount) {
        if (lastCount + 1 < actCount) {
            webPage.lostSteps += (actCount - lastCount - 1);
        }
        if(actCount % 4 == 0) {
            if (printing == true) {
                printCountState++;
                if ((printCountState >= 150) && (printCountState < webPage.length + 150)) {
                    for (int i = 0; i < 12; i++) {
                        buffer[i] = webPage.printBuffer[12 * (printCountState - 150) + i];
                        //Serial.printf(BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(buffer[i]));
                    }
                    //Serial.println();
                    SPI.writeBytes(buffer, sizeof(buffer));
                    pulseLow(PIN_LATCH, 10); // 10us low pulse to latch data
                    pulseLow(PIN_HEAT, 3000); // nach 4 Pulsen schreiben
                }
            }
        }
        lastCount = actCount;
        float freq = 0.0;
        if (p > 0) { // 1 kHz ... 1 Hz
            freq = 1e6f / (float)p;   // Hz
        }
        
        // Tiefpassfilter gegen Jitter
        freqFilt = 0.9f * freqFilt + 0.1f * freq;

        // Regler
        float error = 220.0f - freqFilt;
        integral += error * 0.1f; // angenommen: loop alle 10ms
        float control = 200 + 1.5*error + 0.05*integral;
        
        duty = constrain((uint32_t)control, 0, 1023);
        if (printCountState > webPage.length + 300) {
            // print done
            duty = 0;
            printing = false;
            digitalWrite(PIN_5V_EN, LOW); // 5V für Sensor ausschalten
        }
        if (printing == false) {
            duty = 0;
            integral = 0;
        }
        ledcWrite(0, duty);

        // Debug-Ausgabe
        
        
    }
    if (millis() - lastPrint > 500) {
        lastPrint = millis();
        //Serial.printf("filt=%.2f Hz, duty= %d, count=%d, errors=%d\n", freqFilt, duty, actCount, errorCount);
    }
    if (printing == false) {
        webPage.loop();
        checkNetworkSet();
        delay(delayTime_ms);
    }
}
