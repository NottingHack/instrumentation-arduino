
#ifndef NETCONFCONFIG_H
#define NETCONFCONFIG_H


#include <Ethernet.h>
#include <PubSubClient.h>
#include "Config.h"
#include "net_messaging.h"

struct net_msg_t
{
  byte msgtype[4];
  byte payload[40];
} ;


void net_transport_init();
void net_transport_loop();
void net_transport_send(struct net_msg_t *msg);

void net_transport_mqtt_callback(char* topic, byte* payload, unsigned int length) ;


#endif
