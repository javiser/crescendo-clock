#ifndef _INCLUDE_MQTT_CONFIG_HPP_
#define _INCLUDE_MQTT_CONFIG_HPP_

// Comment or delete this line if you don't want the MQTT functionality be compiled and used
//#define MQTT_ACTIVE

#ifdef MQTT_ACTIVE
#define MQTT_BROKER_ADDRESS "mqtt://192.168.178.29:1883"
#define MQTT_USERNAME "homeassistant"
#define MQTT_PASSWORD "yourverynicemqttpassword"
#endif

#endif // _INCLUDE_MQTT_CONFIG_HPP_