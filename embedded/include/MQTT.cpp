#include <PubSubClient.h>
#include <WiFi.h>

// hotspot
constexpr const char *userName = Username;
constexpr const char *passWord = Password;

WiFiClient espClient;

void setup_wifi() {
  Serial.print("connecting it brochacho");

  WiFi.disconnect(true);
  WiFi.mode(WIFI_STA);

  WiFi.begin(userName, passWord);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nwifi is connected dude");
  Serial.print("this is the ip dude:");
  Serial.println(WiFi.localIP());
}
