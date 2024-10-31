#include <FastLED.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "secrets.h"

#define LED_PIN     16
#define NUM_LEDS    300
#define BRIGHTNESS  100
#define LED_TYPE    WS2812
#define COLOR_ORDER RGB
CRGB leds[NUM_LEDS];

const char* ssid = WIFI_SSID;
const char* password = WIFI_PSK;
const char* mqtt_server = MQTT_HOST;

void callback(char* topic, byte* payload, unsigned int length) {
  if(strcmp(topic, "/erase") == 0){
    // Serial.printf("Got erase\n");
    for(uint16_t i=0;i<NUM_LEDS;i++){
      leds[i] = CRGB::Black;
    }
    FastLED.show();
  }else if(strcmp(topic, "/changes") == 0){
    // Serial.printf("Got changes, length: %d\n", length);
    if (length % 5 == 0) {      
      for (uint16_t i = 0; i < length; i = i+5) {
        uint16_t pixel = payload[i] | (payload[i + 1] << 8);
        leds[pixel].r = payload[i + 2];
        leds[pixel].g = payload[i + 3];
        leds[pixel].b = payload[i + 4];
        // Serial.printf("Setting pixel %d: %d, %d, %d\n", pixel, payload[i + 2], payload[i + 3], payload[i + 4]);
      }
      FastLED.show();
    }
  }
}

WiFiClientSecure wifi;
PubSubClient client(mqtt_server, 8883, callback, wifi);

void connectToWiFi() {
  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
}

void WiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info){
  Serial.println("Connected to AP successfully!");
}

void WiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info){
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info){
  Serial.println("Disconnected from WiFi access point");
  Serial.print("WiFi lost connection. Reason: ");
  Serial.println(info.wifi_sta_disconnected.reason);
  Serial.println("Trying to Reconnect");
  WiFi.begin(ssid, password);
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("lite-fright-esp32", MQTT_USER, MQTT_PASS)) {
      Serial.println("mqtt connected");
      client.subscribe("/erase");
      client.subscribe("/changes");
      client.publish("/erase","");
    } else {
      Serial.print("mqtt connect failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(9600);
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(BRIGHTNESS);

  leds[0] = CRGB::Red;
  FastLED.show();

  connectToWiFi();

  leds[0] = CRGB::Green;
  leds[1] = CRGB::Red;
  FastLED.show();

  wifi.setInsecure();
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}