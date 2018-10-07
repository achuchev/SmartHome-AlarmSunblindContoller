#ifndef SETTING_H
#define SETTING_H

#include "SomfyBlind.h"

#define DEVICE_NAME "AlarmSunblindsController"

#define DEVICE_RESTART_ESP_TIME 86400000 // 24 hours
#define RF_PIN D2
#define BLINDS_COUNT 6
#define TOPICS_COUNT 7                   // It is allways BLINDS_COUNT plus one topic for Alarm

#define MQTT_TOPIC_ALARM_GET "get/home/lock"
#define MQTT_TOPIC_ALARM_SET "set/home/lock"

#define MQTT_ALARM_PUBLISH_STATUS_INTERVAL 600000
#define MQTT_SUNBLIND_PUBLISH_STATUS_INTERVAL 300000

SomfyBlind *smallBlind1 = new SomfyBlind("smallBedroomSunblind1",
                                         "/apartment/smallBedroom/sunblind/1",
                                         RF_PIN);

SomfyBlind *largeBlind1 = new SomfyBlind("largeBedroomSunblind1",
                                         "/apartment/largeBedroom/sunblind/1",
                                         RF_PIN);

SomfyBlind *largeBlind2 = new SomfyBlind("largeBedroomSunblind2",
                                         "/apartment/largeBedroom/sunblind/2",
                                         RF_PIN);

SomfyBlind *largeBlind3 = new SomfyBlind("largeBedroomSunblind3",
                                         "/apartment/largeBedroom/sunblind/3",
                                         RF_PIN);

SomfyBlind *largeBlind4 = new SomfyBlind("largeBedroomSunblind4",
                                         "/apartment/largeBedroom/sunblind/4",
                                         RF_PIN);
SomfyBlind *largeBlindAll = new SomfyBlind("largeBedroomSunblindAll",
                                           "/apartment/largeBedroom/sunblind/all",
                                           RF_PIN);
SomfyBlind blinds[BLINDS_COUNT] =
{ *smallBlind1, *largeBlind1, *largeBlind2, *largeBlind3, *largeBlind4, *largeBlindAll };

#endif // ifndef SETTING_H
