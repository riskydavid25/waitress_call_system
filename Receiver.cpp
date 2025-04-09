#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiManager.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DFRobotDFPlayerMini.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define RXD2 16
#define TXD2 17
HardwareSerial mySerial(2);
DFRobotDFPlayerMini dfPlayer;

const char *mqtt_server = "broker.emqx.io";
const int mqtt_port = 1883;
const char *mqtt_client_id = "ESP32_Receiver";

#define BLYNK_PRINT Serial
#define BLYNK_AUTH "cjqSPyXSlzm32sMLG0JOfH7ANlSvqe8M"

const int wifiLed = 14;

bool wifiConnected = false;
String statusSender[6] = {"OFF", "OFF", "OFF", "OFF", "OFF", "OFF"};
unsigned long lastMessageTime[6] = {0, 0, 0, 0, 0, 0};
const unsigned long debounceDelay = 1000;

WiFiClient espClient;
PubSubClient client(espClient);
WiFiManager wifiManager;

void updateDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Meja 1, Call: " + statusSender[0] + ", Bill: " + statusSender[1]);
  display.println("Meja 2, Call: " + statusSender[2] + ", Bill: " + statusSender[3]);
  display.println("Meja 3, Call: " + statusSender[4] + ", Bill: " + statusSender[5]);
  display.display();
}

void callback(char *topic, byte *payload, unsigned int length) {
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  unsigned long currentMillis = millis();

  if (strcmp(topic, "waitress/sender1/call") == 0 && currentMillis - lastMessageTime[0] > debounceDelay) {
    statusSender[0] = (message == "true") ? "ON" : "OFF";
    if (message == "true") dfPlayer.play(1);
    lastMessageTime[0] = currentMillis;
  }
  if (strcmp(topic, "waitress/sender1/bill") == 0 && currentMillis - lastMessageTime[1] > debounceDelay) {
    statusSender[1] = (message == "true") ? "ON" : "OFF";
    if (message == "true") dfPlayer.play(2);
    lastMessageTime[1] = currentMillis;
  }
  if (strcmp(topic, "waitress/sender2/call") == 0 && currentMillis - lastMessageTime[2] > debounceDelay) {
    statusSender[2] = (message == "true") ? "ON" : "OFF";
    if (message == "true") dfPlayer.play(3);
    lastMessageTime[2] = currentMillis;
  }
  if (strcmp(topic, "waitress/sender2/bill") == 0 && currentMillis - lastMessageTime[3] > debounceDelay) {
    statusSender[3] = (message == "true") ? "ON" : "OFF";
    if (message == "true") dfPlayer.play(4);
    lastMessageTime[3] = currentMillis;
  }
  if (strcmp(topic, "waitress/sender3/call") == 0 && currentMillis - lastMessageTime[4] > debounceDelay) {
    statusSender[4] = (message == "true") ? "ON" : "OFF";
    if (message == "true") dfPlayer.play(5);
    lastMessageTime[4] = currentMillis;
  }
  if (strcmp(topic, "waitress/sender3/bill") == 0 && currentMillis - lastMessageTime[5] > debounceDelay) {
    statusSender[5] = (message == "true") ? "ON" : "OFF";
    if (message == "true") dfPlayer.play(6);
    lastMessageTime[5] = currentMillis;
  }

  updateDisplay();
}

void mqttTask(void *parameter) {
  for (;;) {
    if (!client.connected()) {
      while (!client.connected()) {
        if (client.connect(mqtt_client_id)) {
          client.subscribe("waitress/sender1/call");
          client.subscribe("waitress/sender1/bill");
          client.subscribe("waitress/sender2/call");
          client.subscribe("waitress/sender2/bill");
          client.subscribe("waitress/sender3/call");
          client.subscribe("waitress/sender3/bill");
        } else {
          vTaskDelay(3000 / portTICK_PERIOD_MS);
        }
      }
    }
    client.loop();
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void ledTask(void *parameter) {
  for (;;) {
    if (WiFi.status() == WL_CONNECTED) {
      digitalWrite(wifiLed, !digitalRead(wifiLed));
      vTaskDelay(500 / portTICK_PERIOD_MS);
    } else {
      digitalWrite(wifiLed, HIGH);
      vTaskDelay(100 / portTICK_PERIOD_MS);
    }
  }
}

void setup() {
  Serial.begin(115200);
  mySerial.begin(9600, SERIAL_8N1, RXD2, TXD2);
  pinMode(wifiLed, OUTPUT);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    while (true);
  }

  if (!dfPlayer.begin(mySerial)) {
    Serial.println("DFPlayer Mini gagal diinisialisasi!");
  } else {
    dfPlayer.volume(30);
    dfPlayer.play(7); // setup awal 0007.mp3
  }

  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(20, 0);
  display.println("WAITRESS");
  display.setCursor(40, 20);
  display.println("CALL");
  display.setCursor(34, 40);
  display.println("SYSTEM");
  display.display();
  delay(2000);

  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(20, 20);
  display.println("RISKY DAVID K");
  display.setCursor(30, 35);
  display.println("2212101134");
  display.display();
  delay(2000);

  updateDisplay();

  wifiManager.setTimeout(180);
  if (!wifiManager.autoConnect("Receiver_AP")) {
    Serial.println("Failed to connect and hit timeout");
    while (true) {
      digitalWrite(wifiLed, HIGH);
      delay(500);
    }
  }
  wifiConnected = true;
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  xTaskCreate(mqttTask, "MQTT Task", 4096, NULL, 1, NULL);
  xTaskCreate(ledTask, "LED Task", 1024, NULL, 1, NULL);
}

void loop() {
  Blynk.run();
}
