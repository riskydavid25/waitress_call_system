#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiManager.h>
#include <BlynkSimpleEsp32.h>

// Konfigurasi MQTT Broker
const char* mqtt_server = "broker.emqx.io"; // Contoh: "broker.hivemq.com"
const int mqtt_port = 1883;
const char* mqtt_client_id = "ESP32_Sender3";

// Konfigurasi Blynk
#define BLYNK_PRINT Serial
#define BLYNK_AUTH "cjqSPyXSlzm32sMLG0JOfH7ANlSvqe8M"  // Ganti dengan token dari Blynk

// Pin konfigurasi
const int callButton = 33;
const int billButton = 25;
const int resetButton = 26;
const int greenLed = 12;  // LED Call
const int blueLed = 13;   // LED Bill
const int wifiLed = 14;   // LED Indikator WiFi

int lastCallState = HIGH;
int lastBillState = HIGH;
int lastResetState = HIGH;
bool wifiConnected = false;

WiFiClient espClient;
PubSubClient client(espClient);
WiFiManager wifiManager;

void setup() {
  Serial.begin(115200);
  pinMode(callButton, INPUT_PULLUP);
  pinMode(billButton, INPUT_PULLUP);
  pinMode(resetButton, INPUT_PULLUP);
  pinMode(greenLed, OUTPUT);
  pinMode(blueLed, OUTPUT);
  pinMode(wifiLed, OUTPUT);

  digitalWrite(greenLed, LOW);
  digitalWrite(blueLed, LOW);
  digitalWrite(wifiLed, HIGH); // LED merah menyala jika WiFi belum tersambung

  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  Blynk.begin(BLYNK_AUTH, WiFi.SSID().c_str(), WiFi.psk().c_str()); // Koneksi ke Blynk dengan WiFi yang sudah tersambung
}

void setup_wifi() {
  wifiManager.setTimeout(180);
  if (!wifiManager.autoConnect("Sender3_AP")) {
    Serial.println("Failed to connect and hit timeout");
    ESP.restart();
  }
  Serial.println("WiFi connected");
  Serial.println(WiFi.localIP());

  wifiConnected = true;
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(mqtt_client_id)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

// Fungsi reset dari Blynk
BLYNK_WRITE(V0) {
  int resetState = param.asInt();
  if (resetState == 1) {
    resetSystem();
  }
}

// Fungsi reset sistem (untuk tombol fisik & Blynk)
void resetSystem() {
  digitalWrite(greenLed, LOW);
  digitalWrite(blueLed, LOW);
  client.publish("waitress/sender3/call", "false");
  client.publish("waitress/sender3/bill", "false");
  Blynk.virtualWrite(V1, 0);
  Blynk.virtualWrite(V2, 0);
  Blynk.virtualWrite(V3, 1);
}

void loop() {
  Blynk.run();
  
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
  } else {
    wifiConnected = false;
  }

  // Blinking LED WiFi jika tersambung
  static unsigned long lastBlinkTime = 0;
  if (wifiConnected) {
    if (millis() - lastBlinkTime > 500) { // Set interval blinking 500ms
      digitalWrite(wifiLed, !digitalRead(wifiLed));
      lastBlinkTime = millis();
    }
  } else {
    digitalWrite(wifiLed, HIGH); // LED merah menyala terus jika WiFi putus
  }

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  int callState = digitalRead(callButton);
  int billState = digitalRead(billButton);
  int resetState = digitalRead(resetButton);

  if (callState == LOW && lastCallState == HIGH) {
    digitalWrite(greenLed, HIGH);
    digitalWrite(blueLed, LOW);
    client.publish("waitress/sender3/call", "true");
    client.publish("waitress/sender3/bill", "false");
    Blynk.virtualWrite(V1, 1);
    Blynk.virtualWrite(V2, 0);
    Blynk.virtualWrite(V3, 0);
    delay(200);
  }

  if (billState == LOW && lastBillState == HIGH) {
    digitalWrite(greenLed, LOW);
    digitalWrite(blueLed, HIGH);
    client.publish("waitress/sender3/call", "false");
    client.publish("waitress/sender3/bill", "true");
    Blynk.virtualWrite(V1, 0);
    Blynk.virtualWrite(V2, 1);
    Blynk.virtualWrite(V3, 0);
    delay(200);
  }

  if (resetState == LOW && lastResetState == HIGH) {
    resetSystem();
    delay(200);
  }

  lastCallState = callState;
  lastBillState = billState;
  lastResetState = resetState;
}
