#ifndef SETTING_H
#define SETTING_H

#include "SomfyBlind.h"

#define DEVICE_NAME "AlarmSunblindsController"

#define DEVICE_RESTART_ESP_TIME 86400000 // 24 hours
#define PIN_RF_TRANSMITER_SOMFY_DATA D2
#define PIN_RF_RECEIVER_DATA D6
#define PIN_RF_RECEIVER_CS D7            // HIGH: Normal working; LOW: Sleep mode
#define BLINDS_COUNT 6
#define TOPICS_COUNT 7                   // It is allways BLINDS_COUNT plus one topic for Alarm

#define MQTT_TOPIC_ALARM_GET "get/home/lock"
#define MQTT_TOPIC_ALARM_SET "set/home/lock"

#define MQTT_ALARM_PUBLISH_STATUS_INTERVAL 600000
#define MQTT_SUNBLIND_PUBLISH_STATUS_INTERVAL 300000

SomfyBlind *smallBlind1 = new SomfyBlind("smallBedroomSunblind1",
                                         "/apartment/smallBedroom/sunblind/1",
                                         PIN_RF_TRANSMITER_SOMFY_DATA);

SomfyBlind *largeBlind1 = new SomfyBlind("largeBedroomSunblind1",
                                         "/apartment/largeBedroom/sunblind/1",
                                         PIN_RF_TRANSMITER_SOMFY_DATA);

SomfyBlind *largeBlind2 = new SomfyBlind("largeBedroomSunblind2",
                                         "/apartment/largeBedroom/sunblind/2",
                                         PIN_RF_TRANSMITER_SOMFY_DATA);

SomfyBlind *largeBlind3 = new SomfyBlind("largeBedroomSunblind3",
                                         "/apartment/largeBedroom/sunblind/3",
                                         PIN_RF_TRANSMITER_SOMFY_DATA);

SomfyBlind *largeBlind4 = new   SomfyBlind("largeBedroomSunblind4",
                                           "/apartment/largeBedroom/sunblind/4",
                                           PIN_RF_TRANSMITER_SOMFY_DATA);
SomfyBlind *largeBlindAll = new SomfyBlind("largeBedroomSunblindAll",
                                           "/apartment/largeBedroom/sunblind/all",
                                           PIN_RF_TRANSMITER_SOMFY_DATA);
SomfyBlind blinds[BLINDS_COUNT] =
{ *smallBlind1, *largeBlind1, *largeBlind2, *largeBlind3, *largeBlind4, *largeBlindAll };

// RF codes of the remote control
unsigned long rcButtonA1[4] = { 2184988, 2683484, 3096748, 2244204 };
unsigned long rcButtonA2[4] = { 2893180, 2387756, 2328972, 3017404 };

unsigned long rcButtonB1[4] = { 2429925, 2147333, 2584885, 2746101 };
unsigned long rcButtonB2[4] = { 2866629, 2529429, 2993109, 2808901 };

unsigned long rcButtonC1[4] = { 3017406, 2893182, 2387758, 2328974 };
unsigned long rcButtonC2[4] = { 2244206, 2184990, 2683486, 3096750 };

unsigned long rcButtonD1[4] = { 2866631, 2529431, 2993111, 2808903 };
unsigned long rcButtonD2[4] = { 2429927, 2147335, 2584887, 2746103 };

unsigned long rcButtonMaster1[4] = { 2893170, 3017394, 2328962, 2387746 };
unsigned long rcButtonMaster2[4] = { 2244194, 3096738, 2683474, 2184978 };

#define MQTT_TOPIC_LAMP_1_SET "set/apartment/livingRoom/lamp/1"
#define MQTT_TOPIC_LAMP_2_SET "set/apartment/livingRoom/lamp/2"
#define MQTT_TOPIC_LAMP_3_SET "set/apartment/livingRoom/lamp/3"
#define MQTT_TOPIC_LAMP_4_SET "set/apartment/livingRoom/lamp/4"

#endif // ifndef SETTING_H
