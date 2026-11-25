#include <PubSubClient.h>
#include <WiFi.h>
#include <motor.hpp>

// Wi-Fi (hotspot) credentials
constexpr const char *userName = Username;
constexpr const char *passWord = Password;

// EMQX Public Broker settings
constexpr const char *mqtt_server = "broker.emqx.io";
constexpr int mqtt_port = 1883;
constexpr const char *subscribe_topic = "gimbal/commands";
WiFiClient espClient;
PubSubClient client(espClient);

void callback(char *topic, byte *payload, unsigned int length) {
	String msg;
	msg.reserve(length);
	for (unsigned int i = 0; i < length; i++) {
		msg += (char)payload[i];
	}

	int commaIdx = msg.indexOf(',');

	if (commaIdx == -1) {
    	Serial.println("Invalid payload: missing comma");
    	return;
 	}
	MovePos(msg.substring(0, commaIdx).toFloat(), msg.substring(commaIdx + 1).toFloat());
}

// Wifi Setup
void setup_wifi() {
  Serial.print("connecting it brochacho");

  WiFi.disconnect(true);
  WiFi.mode(WIFI_STA);

  WiFi.begin(userName, passWord);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500); // Makes sure it doesn't spam "." rapidly
    Serial.print(".");
  }

  Serial.println("\nwifi is connected dude");
  Serial.print("this is the ip dude:");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Connecting to EMQX...");
    String clientId = "ESP32S3-" + String(random(0xffff), HEX);

    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      client.subscribe(subscribe_topic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" retrying in 5s");
      delay(5000);
    }
  }
}
