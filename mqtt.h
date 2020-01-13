#include <AsyncMqttClient.h>

#define MQTT_HOST IPAddress(192, 168, 1, 184)
#define MQTT_PORT 1883
#define MQTT_USER "pwnrazr"
#define MQTT_PASS "pwnrazr123"

AsyncMqttClient mqttClient;

void connectToMqtt() {
  Serial.println("Connecting to MQTT...");
  mqttClient.connect();
}

void onMqttConnect(bool sessionPresent) {
  Serial.println("Connected to MQTT.");
  Serial.print("Session present: ");
  Serial.println(sessionPresent);

  mqttClient.subscribe("esp32/beepamount", 2);
  mqttClient.subscribe("esp32/forcestopbeep", 2);
  mqttClient.subscribe("esp32/led", 2);
}

void onMqttSubscribe(uint16_t packetId, uint8_t qos) {
  Serial.println("Subscribe acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
  Serial.print("  qos: ");
  Serial.println(qos);
}

void onMqttUnsubscribe(uint16_t packetId) {
  Serial.println("Unsubscribe acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}

void onMqttPublish(uint16_t packetId) {
  Serial.println("Publish acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}
