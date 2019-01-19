/*
   Easiest way to make it work for you:
   1. Uncomment and modify the sunblindManualInitialConfiguration() with:
    - Unique remote number
    - Choose a starting point for the rolling code. Any unsigned int works, 1 is a good start
    - Upload the sketch to the sunblindManualInitialConfiguration() is executed
    - Comment the sunblindManualInitialConfiguration() and upload the sketch.
   2. Long-press the program button of YOUR ACTUAL REMOTE until your blind goes up and down slightly
    - send {"status":{"action":"prog"}} to the MQTT channel of the remote
 */
#include <Arduino.h>
#include <ArduinoJson.h>
#include <MqttClient.h>
#include <FotaClient.h>
#include <ESPWifiClient.h>
#include <RemotePrint.h>
#include <SimpleTimer.h>
#include <RCSwitch.h>
#include "settings.h"
#include "ParadoxControlPanel/ParadoxControlPanel.h"

SimpleTimer espRestartTimer;
RCSwitch    rcReceiver    = RCSwitch();
ESPWifiClient *wifiClient = new ESPWifiClient(WIFI_SSID, WIFI_PASS);
FotaClient    *fotaClient = new FotaClient(DEVICE_NAME);
MqttClient    *mqttClient = NULL;

ParadoxControlPanel *controlPanel =
  new ParadoxControlPanel(ALARM_MODULE_HOSTNAME,
                          ALARM_MODULE_PASSWORD,
                          ALARM_USER_PIN);


long   alarmLastAttempt            = 0;
long   sunblindLastStatusMsgSentAt = 0;
String topics[TOPICS_COUNT]; // one additional topic for the Alarm

// TODO: Move the method below to utils class
void getAllTopics(String action, String topics[], const char *alarmTopic) {
  int i = 0;

  for (i = 0; i < BLINDS_COUNT; i++) {
    SomfyBlind *blind = &blinds[i];
    String tmp;
    tmp       = action + blind->mqttTopic;
    topics[i] = tmp;
  }

  // The index is already incremented
  topics[i] = String(alarmTopic);
}

// TODO: Move the method below to utils class
SomfyBlind* getBlindFromTopic(String topic) {
  String *lowerTopic = new String(topic.c_str());

  lowerTopic->toLowerCase();

  for (int i = 0; i < TOPICS_COUNT; i++) {
    String *lowerTopicNext = new String(blinds[i].mqttTopic.c_str());
    lowerTopicNext->toLowerCase();

    if (*lowerTopic == *lowerTopicNext) {
      return &blinds[i];
    }
  }
  PRINT("BLIND: Could not find Blind with topic: ");
  PRINTLN(topic);
  return NULL;
}

void restartEsp() {
  PRINTLN("ESP: Restaring ESP due to schedule");
  ESP.restart();
}

void sunblindPublishStatus(bool forcePublish = false,
                           const char *messageId = NULL, String topic = "") {
  long now = millis();

  if ((forcePublish) or (now - sunblindLastStatusMsgSentAt >
                         MQTT_SUNBLIND_PUBLISH_STATUS_INTERVAL)) {
    int count = BLINDS_COUNT;

    if (topic.length()) {
      count = 1;
    }

    for (int i = 0; i < count; i++) {
      SomfyBlind *blind = NULL;

      if (topic.length()) {
        blind = getBlindFromTopic(topic);
      } else {
        blind = &blinds[i];
      }

      const size_t bufferSize = JSON_OBJECT_SIZE(1) + JSON_OBJECT_SIZE(3);
      DynamicJsonBuffer jsonBuffer(bufferSize);
      JsonObject& root   = jsonBuffer.createObject();
      JsonObject& status = root.createNestedObject("status");

      if (messageId != NULL) {
        root["messageId"] = messageId;
      }

      root["name"]      = blind->name;
      status["powerOn"] = blind->blindPowerOn;

      // convert to String
      String outString;
      root.printTo(outString);

      // publish the message
      String topic = String("get");
      topic.concat(blind->mqttTopic);
      mqttClient->publish(topic, outString);
    }
    sunblindLastStatusMsgSentAt = now;
  }
}

void sunblindSendToBlind(SomfyBlind *blind, String payload) {
  PRINTLN("BLIND: Send to blind '" + blind->name + "''");

  const size_t bufferSize = JSON_OBJECT_SIZE(1) + JSON_OBJECT_SIZE(3) + 50;
  DynamicJsonBuffer jsonBuffer(bufferSize);
  JsonObject& root   = jsonBuffer.parseObject(payload);
  JsonObject& status = root.get<JsonObject&>("status");

  if (!status.success()) {
    PRINTLN_E("BLIND: JSON with \"status\" key not received.");
    PRINTLN_E(payload);
    return;
  }

  const char *powerOn = status.get<const char *>("powerOn");

  // Publish the status here to have quck feedback
  const char *messageId = root.get<const char *>("messageId");

  if (powerOn) { // check if powerOn is present in the payload
    PRINT("BLIND: Power On: ");
    PRINTLN(powerOn);

    if (strcasecmp(powerOn, "true") == 0) {
      blind->blindPowerOn = true;
      sunblindPublishStatus(true, messageId, blind->mqttTopic);
      blind->remoteButtonUp();
    } else {
      blind->blindPowerOn = false;
      sunblindPublishStatus(true, messageId, blind->mqttTopic);
      blind->remoteButtonDown();
    }
  }
  const char *action = status.get<const char *>("action");

  if (action) {
    if (strcasecmp(action, "up") == 0) {
      blind->blindPowerOn = true;
      blind->remoteButtonUp();
    } else if (strcasecmp(action, "down") == 0) {
      blind->blindPowerOn = false;
      blind->remoteButtonDown();
    } else if (strcasecmp(action, "stop") == 0) {
      blind->remoteButtonStop();
    } else if (strcasecmp(action, "prog") == 0) {
      blind->remoteButtonProgram();
    } else {
      PRINT("BLIND: Unknown action '");
      PRINT(action);
      PRINTLN("'");
    }
  }
  sunblindPublishStatus(true, messageId, blind->mqttTopic);
}

void sunblindMqttCallback(char *topic, String payload) {
  PRINTLN("Blind: Callback called.");

  char *ptr = strchr(topic, '/');

  if (ptr != NULL) {
    String lowerTopic = String(ptr);
    lowerTopic.toLowerCase();
    SomfyBlind *blind = getBlindFromTopic(lowerTopic);

    if (blind == NULL) {
      // Blind not found
      return;
    }
    sunblindSendToBlind(blind, payload);
  }
}

void sunblindManualInitialConfiguration() {
  SomfyBlind *blind = NULL;

  // blind                       = &blinds[0];
  // blind->remoteControllSerial = 0x111000;
  // blind->rollingCode          = 10;
  // PRINTLN("rollingCode: " + String(blind->rollingCode));
  // PRINTLN("remoteControllSerial: " + String(blind->remoteControllSerial));
  // blind->save();

  // ========================
  // blind                       = &blinds[1];
  // blind->remoteControllSerial = 0x111001;
  // blind->rollingCode          = 10;
  // PRINTLN("rollingCode: " + String(blind->rollingCode));
  // PRINTLN("remoteControllSerial: " + String(blind->remoteControllSerial));
  // blind->save();

  // ========================
  // blind                       = &blinds[2];
  // blind->remoteControllSerial = 0x111002;
  // blind->rollingCode          = 10;
  // PRINTLN("rollingCode: " + String(blind->rollingCode));
  // PRINTLN("remoteControllSerial: " + String(blind->remoteControllSerial));
  // blind->save();

  // ========================
  // blind                       = &blinds[3];
  // blind->remoteControllSerial = 0x111003;
  // blind->rollingCode          = 10;
  // PRINTLN("rollingCode: " + String(blind->rollingCode));
  // PRINTLN("remoteControllSerial: " + String(blind->remoteControllSerial));
  // blind->save();


  // ========================
  // blind                       = &blinds[4];
  // blind->remoteControllSerial = 0x111004;
  // blind->rollingCode          = 10;
  // PRINTLN("rollingCode: " + String(blind->rollingCode));
  // PRINTLN("remoteControllSerial: " + String(blind->remoteControllSerial));
  // blind->save();

  // ========================
  blind                       = &blinds[5];
  blind->remoteControllSerial = 0x111005;
  blind->rollingCode          = 10;
  PRINTLN("rollingCode: " + String(blind->rollingCode));
  PRINTLN("remoteControllSerial: " + String(blind->remoteControllSerial));
  blind->save();

  for (int i = 0; i < BLINDS_COUNT; i++) {
    SomfyBlind *blind = &blinds[i];
    blind->load();
    PRINTLN("rollingCode: " + String(blind->rollingCode));
    PRINTLN("remoteControllSerial: " + String(blind->remoteControllSerial));
  }
}

void alarmPublishStatus(const char *messageId = "", bool force = false, const char *areaName = "") {
  if (force == true) {
    String areasInfo = controlPanel->getAreasInfoForArm(areaName, messageId);
    mqttClient->publish(MQTT_TOPIC_ALARM_GET, areasInfo);
  } else {
    String areasInfo = controlPanel->getLatestAreasInfo();

    if (areasInfo.length() != 0) {
      mqttClient->publish(MQTT_TOPIC_ALARM_GET, areasInfo);
    }
  }
}

void alarmMqttCallback(String payload) {
  PRINTLN("Paradox: Callback called.");

  const size_t bufferSize = JSON_OBJECT_SIZE(1) + JSON_OBJECT_SIZE(2) + 90;
  DynamicJsonBuffer jsonBuffer(bufferSize);
  JsonObject& root   = jsonBuffer.parseObject(payload);
  JsonObject& status = root.get<JsonObject&>("status");

  if (!root.success() || !status.success()) {
    PRINTLN("Paradox: Invalid JSON received.");
  #ifdef DEBUG_ENABLED
    root.prettyPrintTo(Serial);
  #endif // ifdef DEBUG_ENABLED
    return;
  }
  const char *armChar = status.get<const char *>("arm");

  if (armChar) {
    QueueItem item;
    item.areaName = armChar;
    item.action   = Action::armArea;
    controlPanel->queueActionAdd(item);

    const char *messageId = root.get<const char *>("messageId");
    alarmPublishStatus(messageId, true, armChar);

    // TODO: We need the loop here to make sure that the message will be sent asap
    mqttClient->loop();
  }
}

void mqttCallback(char *topic, byte *payload, unsigned int length) {
  PRINT("MQTT Message arrived [");
  PRINT(topic);
  PRINTLN("] ");

  // Convert the payload to string
  char spayload[length + 1];
  memcpy(spayload, payload, length);
  spayload[length] = '\0';
  String payloadString = String(spayload);

  if (strcmp(topic, MQTT_TOPIC_ALARM_SET) == 0) {
    return alarmMqttCallback(payloadString);
  } else {
    sunblindMqttCallback(topic, payloadString);
  }
}

void alarmGetControlPanelStatus() {
  if  ((alarmLastAttempt != 0) &&
       (millis() <= alarmLastAttempt + (unsigned int)MQTT_ALARM_PUBLISH_STATUS_INTERVAL)) {
    return;
  }
  QueueItem item;
  item.areaName = "";
  item.action   = Action::getStatus;
  controlPanel->queueActionAdd(item);
  alarmLastAttempt = millis();
}

void setup() {
  wifiClient->init();
  getAllTopics("set", topics, MQTT_TOPIC_ALARM_SET);
  mqttClient = new MqttClient(MQTT_SERVER,
                              MQTT_SERVER_PORT,
                              DEVICE_NAME,
                              MQTT_USERNAME,
                              MQTT_PASS,
                              topics,
                              TOPICS_COUNT,
                              MQTT_SERVER_FINGERPRINT,
                              mqttCallback);
  fotaClient->init();
  espRestartTimer.setInterval(DEVICE_RESTART_ESP_TIME, restartEsp);

  // sunblindManualInitialConfiguration();

  rcReceiver.enableReceive(PIN_RF_RECEIVER_DATA);
}

boolean matchRCCode(unsigned long receivedValue, unsigned long buttonArray[]) {
  for (uint8_t i = 0; i <= 3; ++i) {
    if (buttonArray[i] == receivedValue) {
      return true;
    }
  }
  return false;
}

void turnLamp(String mqttTopic, boolean isOn) {
  String payload;

  if (isOn) payload = "{\"status\":{\"powerOn\":true}}"; else {
    payload = "{\"status\":\{\"powerOn\":false}}";
  }
  mqttClient->publish(mqttTopic, payload);
}

void rcSwitchLoop() {
  if (rcReceiver.available()) {
    unsigned long receivedValue = rcReceiver.getReceivedValue();
    PRINT_D("RF Receiver: Code received: ");
    PRINTLN_D(receivedValue);
    rcReceiver.resetAvailable();

    if (matchRCCode(receivedValue, rcButtonA1)) {
      PRINTLN("RF Receiver: Button pressed: A1");
      turnLamp(MQTT_TOPIC_LAMP_4_SET, true);
    } else if (matchRCCode(receivedValue, rcButtonA2)) {
      PRINTLN("RF Receiver: Button pressed: A2");
      turnLamp(MQTT_TOPIC_LAMP_4_SET, false);
    } else if (matchRCCode(receivedValue, rcButtonB1)) {
      PRINTLN("RF Receiver: Button pressed: B1");
      turnLamp(MQTT_TOPIC_LAMP_3_SET, true);
    } else if (matchRCCode(receivedValue, rcButtonB2)) {
      PRINTLN("RF Receiver: Button pressed: B2");
      turnLamp(MQTT_TOPIC_LAMP_3_SET, false);
    } else if (matchRCCode(receivedValue, rcButtonC1)) {
      PRINTLN("RF Receiver: Button pressed: C1");
      turnLamp(MQTT_TOPIC_LAMP_2_SET, true);
    } else if (matchRCCode(receivedValue, rcButtonC2)) {
      PRINTLN("RF Receiver: Button pressed: C2");
      turnLamp(MQTT_TOPIC_LAMP_2_SET, false);
    } else if (matchRCCode(receivedValue, rcButtonD1)) {
      PRINTLN("RF Receiver: Button pressed: D1");
      turnLamp(MQTT_TOPIC_LAMP_1_SET, true);
    } else if (matchRCCode(receivedValue, rcButtonD2)) {
      PRINTLN("RF Receiver: Button pressed: D2");
      turnLamp(MQTT_TOPIC_LAMP_1_SET, false);
    } else if (matchRCCode(receivedValue, rcButtonMaster1)) {
      PRINTLN("RF Receiver: Button pressed: Master1");
      turnLamp(MQTT_TOPIC_LAMP_1_SET, true);
      turnLamp(MQTT_TOPIC_LAMP_2_SET, true);
      turnLamp(MQTT_TOPIC_LAMP_3_SET, true);
      turnLamp(MQTT_TOPIC_LAMP_4_SET, true);
    } else if (matchRCCode(receivedValue, rcButtonMaster2)) {
      PRINTLN("RF Receiver: Button pressed: Master2");
      turnLamp(MQTT_TOPIC_LAMP_1_SET, false);
      turnLamp(MQTT_TOPIC_LAMP_2_SET, false);
      turnLamp(MQTT_TOPIC_LAMP_3_SET, false);
      turnLamp(MQTT_TOPIC_LAMP_4_SET, false);
    }
  }
}

void loop() {
  wifiClient->reconnectIfNeeded();
  RemotePrint::instance()->handle();
  fotaClient->loop();
  mqttClient->loop();
  sunblindPublishStatus();

  // alarmMqttClient->loop();
  alarmGetControlPanelStatus();
  alarmPublishStatus();
  controlPanel->process();
  espRestartTimer.run();
  rcSwitchLoop();
}
