#include <Arduino.h>
#include <SomfyBlind.h>
#include <FS.h>
#include <ArduinoJson.h>
#include <RemotePrint.h>
#include "SomfyRemote.h"


SomfyBlind::SomfyBlind() {
  this->name                 = "";
  this->mqttTopic            = "";
  this->remoteControllSerial = 0;
  this->rollingCode          = 0;
  this->blindPowerOn         = true;
}

SomfyBlind::SomfyBlind(String name,
                       String mqttTopic,
                       int    rfPin) {
  this->name                 = name;
  this->mqttTopic            = mqttTopic;
  this->rfPin                = rfPin;
  this->remoteControllSerial = 0;
  this->rollingCode          = 0;
  this->blindPowerOn         = true;
};

void SomfyBlind::remoteButtonUp() {
  PRINTLN("SOMFY BLIND: Button UP");
  this->remoteButton(REMOTE_RAISE);
}

void SomfyBlind::remoteButtonDown() {
  PRINTLN("SOMFY BLIND: Button DOWN");
  this->remoteButton(REMOTE_LOWER);
}

void SomfyBlind::remoteButtonStop() {
  PRINTLN("SOMFY BLIND: Button STOP");
  this->remoteButton(REMOTE_STOP);
}

void SomfyBlind::remoteButtonProgram() {
  PRINTLN("SOMFY BLIND: Button PROGRAM");
  this->remoteButton(REMOTE_PROG);
}

bool SomfyBlind::load() {
  String path = String("/");

  path.concat(this->name);
  path.concat(".json");

  PRINTLN("SOMFY BLIND: Loading " + path);

  bool result = SPIFFS.begin();

  if (!result) {
    PRINT("SOMFY BLIND: Cold not mount SPIFFS.");
    return false;
  }

  File blindFile = SPIFFS.open(path, "r");

  if (!blindFile) {
    PRINTLN("SOMFY BLIND: File does not exists! Path: " + path);
    return false;
  }

  String content = blindFile.readString();
  PRINTLN_D("SOMFY BLIND: File content: " + content);

  const size_t bufferSize = JSON_OBJECT_SIZE(3) + 120;

  // DynamicJsonBuffer jsonBuffer(bufferSize);
  // JsonObject& root = jsonBuffer.parseObject(content);

  DynamicJsonDocument  jsonDoc(bufferSize);
  DeserializationError error = deserializeJson(jsonDoc, content);

  if (error) {
    PRINT_E("SOMFY BLIND: Cannot deserialize the content of the file. Error: ");
    PRINTLN_E(error.c_str());
    PRINTLN_E("The file content is: ");
    PRINTLN_E(content)
    return false;
  }
  JsonObject root = jsonDoc.as<JsonObject>();

  int remoteControllSerial = root["remoteControllSerial"];
  uint16_t rollingCode     = root["rollingCode"];

  this->remoteControllSerial = remoteControllSerial;
  this->rollingCode          = rollingCode;

  blindFile.close();

  SPIFFS.end();
  return true;
}

bool SomfyBlind::save() {
  String path = String("/");

  path.concat(this->name);
  path.concat(".json");
  PRINTLN("SOMFY BLIND: Saving " + path);

  bool result = SPIFFS.begin();

  if (!result) {
    PRINT("SOMFY BLIND: Cold not mount SPIFFS.");
    return false;
  }

  File blindFile = SPIFFS.open(path, "w");

  if (!blindFile) {
    PRINTLN("SOMFY BLIND: Cannot create file with path: " + path);
    return false;
  }
  const size_t bufferSize = JSON_OBJECT_SIZE(3);

  // DynamicJsonBuffer jsonBuffer(bufferSize);
  //
  // JsonObject root = jsonBuffer.createObject();
  DynamicJsonDocument root(bufferSize);

  root["remoteControllSerial"] = this->remoteControllSerial;
  root["rollingCode"]          = this->rollingCode;

  // convert to String
  String content;
  serializeJson(root, content);

  // root.printTo(content);
  blindFile.print(content);
  blindFile.close();
  SPIFFS.end();
  PRINTLN("SOMFY BLIND: Content " + content + " saved to " + path + ".");
  return true;
}

void SomfyBlind::remoteButton(byte button) {
  SomfyRemote remote(this->rfPin);

  this->load();
  uint16_t nextRollingCode = remote.sendButton(this->remoteControllSerial,
                                               button,
                                               this->rollingCode);
  this->rollingCode = nextRollingCode;
  this->save();
}
