#include "offset_source.hpp"
#include <Arduino.h>

#ifdef USE_FAKE_OFFSET

// ---------------------------------------------------------------------------
// Fake source (was fake_offset.cpp): ~30 offsets/s, slow sweep, 2 s dropout
// every 10 s so the lost-offset path gets exercised.
// ---------------------------------------------------------------------------
#include <math.h>

bool pollOffset(Offset &out) {
  static uint32_t last = 0;
  uint32_t now = millis();
  if (now - last < 33)
    return false;
  last = now;

  float t = now / 1000.0f;
  if (fmodf(t, 10.0f) > 8.0f)
    return false;

  out.x = 0.5f * sinf(t * 0.8f);
  out.y = 0.3f * sinf(t * 0.5f);
  return true;
}

#else

// ---------------------------------------------------------------------------
// Real source (was mqtt.cpp): offsets arrive over MQTT from the laptop.
// ---------------------------------------------------------------------------
#include <PubSubClient.h>
#include <WiFi.h>

// Credentials come from build flags, not git:
//   build_flags = -DUsername=\"mynetwork\" -DPassword=\"mypass\"
#if !defined(Username) || !defined(Password)
#error "Define Username and Password via build_flags"
#endif

constexpr const char *mqtt_server = "broker.emqx.io";
constexpr int mqtt_port = 1883;
constexpr const char *subscribe_topic = "gimbal/commands";
constexpr uint32_t RETRY_MS = 5000;

static WiFiClient espClient;
static PubSubClient client(espClient);

// callback() parks the newest offset here; pollOffset() hands it out once.
static Offset latest;
static bool fresh = false;

// Payload format assumed: plain text "x,y", e.g. "0.12,-0.30".
// Change the parsing here if the laptop sends JSON instead.
static void callback(char *topic, byte *payload, unsigned int length) {
  char buf[48];
  if (length >= sizeof(buf))
    return;
  memcpy(buf, payload, length);
  buf[length] = '\0';

  float x, y;
  if (sscanf(buf, "%f,%f", &x, &y) != 2)
    return; // garbage, ignore
  latest.x = constrain(x, -1.0f, 1.0f);
  latest.y = constrain(y, -1.0f, 1.0f);
  fresh = true;
}

// Never blocks for long: WiFi.begin() returns immediately, and reconnect
// attempts are rate-limited to one every RETRY_MS.
bool pollOffset(Offset &out) {
  static bool started = false;
  static uint32_t lastTry = 0;

  if (!started) {
    started = true;
    WiFi.mode(WIFI_STA);
    WiFi.begin(Username, Password);
    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(callback);
    client.setSocketTimeout(2); // seconds; default is 15
  }

  if (WiFi.status() != WL_CONNECTED)
    return false;

  if (!client.connected()) {
    uint32_t now = millis();
    if (now - lastTry >= RETRY_MS) {
      lastTry = now;
      String clientId = "ESP32S3-" + String(random(0xffff), HEX);
      if (client.connect(clientId.c_str())) {
        Serial.println("MQTT connected");
        client.subscribe(subscribe_topic);
      } else {
        Serial.printf("MQTT failed, rc=%d\n", client.state());
      }
    }
    return false;
  }

  client.loop(); // pumps incoming messages into callback()

  if (!fresh)
    return false;
  out = latest;
  fresh = false;
  return true;
}

#endif