/*
 PubSubClientLD.cpp - A simple client for MQTT.
  Nicholas O'Leary
  http://knolleary.net
*/

#include "PubSubClientLD.h"
#include <string.h>

PubSubClientLD::PubSubClientLD() {
   this->_client = NULL;
}

PubSubClientLD::PubSubClientLD(uint8_t *ip, uint16_t port, void (*callback)(char*,uint8_t*,unsigned int), Client& client) {
   this->_client = &client;
   this->callback = callback;
   this->ip = ip;
   this->port = port;
   this->domain = NULL;
}

PubSubClientLD::PubSubClientLD(char* domain, uint16_t port, void (*callback)(char*,uint8_t*,unsigned int), Client& client) {
   this->_client = &client;
   this->callback = callback;
   this->domain = domain;
   this->port = port;
}

boolean PubSubClientLD::connect(char *id) {
   return connect(id,NULL,NULL,0,0,0,0);
}

boolean PubSubClientLD::connect(char *id, char *user, char *pass) {
   return connect(id,user,pass,0,0,0,0);
}

boolean PubSubClientLD::connect(char *id, char* willTopic, uint8_t willQos, uint8_t willRetain, char* willMessage)
{
   return connect(id,NULL,NULL,willTopic,willQos,willRetain,willMessage);
}

boolean PubSubClientLD::connect(char *id, char *user, char *pass, char* willTopic, uint8_t willQos, uint8_t willRetain, char* willMessage) {
   if (!connected()) {
      int result = 0;
      
      if (domain != NULL) {
        result = _client->connect(this->domain, this->port);
      } else {
        result = _client->connect(this->ip, this->port);
      }
      
      if (result) {
         nextMsgId = 1;
         uint8_t d[9] = {0x00,0x06,'M','Q','I','s','d','p',MQTTPROTOCOLVERSION};
         // Leave room in the buffer for header and variable length field
         uint16_t length = 5;
         unsigned int j;
         for (j = 0;j<9;j++) {
            buffer[length++] = d[j];
         }

         uint8_t v;
         if (willTopic) {
            v = 0x06|(willQos<<3)|(willRetain<<5);
         } else {
            v = 0x02;
         }

         if(user != NULL) {
            v = v|0x80;

            if(pass != NULL) {
               v = v|(0x80>>1);
            }
         }

         buffer[length++] = v;

         buffer[length++] = ((MQTT_KEEPALIVE_SEND) >> 8);
         buffer[length++] = ((MQTT_KEEPALIVE_SEND) & 0xFF);
         length = writeString(id,buffer,length);
         if (willTopic) {
            length = writeString(willTopic,buffer,length);
            length = writeString(willMessage,buffer,length);
         }

         if(user != NULL) {
            length = writeString(user,buffer,length);
            if(pass != NULL) {
               length = writeString(pass,buffer,length);
            }
         }
         
         write(MQTTCONNECT,buffer,length-5);
         
         lastInActivity = lastOutActivity = millis();
         
         while (!_client->available()) {
            unsigned long t = millis();
            if (t-lastInActivity > MQTT_KEEPALIVE_TIMEOUT*1000UL) {
               _client->stop();
               return false;
            }
         }
         uint8_t llen;
         uint16_t len = readPacket(&llen);
         
         if (len == 4 && buffer[3] == 0) {
            lastInActivity = millis();
            pingOutstanding = false;
            return true;
         }
      }
      _client->stop();
   }
   return false;
}

uint8_t PubSubClientLD::readByte() {
   while(!_client->available()) {}
   return _client->read();
}

uint16_t PubSubClientLD::readPacket(uint8_t* lengthLength) {
   uint16_t len = 0;
   buffer[len++] = readByte();
   uint8_t multiplier = 1;
   uint16_t length = 0;
   uint8_t digit = 0;
   do {
      digit = readByte();
      buffer[len++] = digit;
      length += (digit & 127) * multiplier;
      multiplier *= 128;
   } while ((digit & 128) != 0);
   *lengthLength = len-1;
   for (uint16_t i = 0;i<length;i++)
   {
      if (len < MQTT_MAX_PACKET_SIZE) {
         buffer[len++] = readByte();
      } else {
         readByte();
         len = 0; // This will cause the packet to be ignored.
      }
   }

   return len;
}

boolean PubSubClientLD::loop() {
   if (connected()) {
      unsigned long t = millis();
      if ((t - lastInActivity > MQTT_KEEPALIVE_TIMEOUT*1000UL) || (t - lastOutActivity > MQTT_KEEPALIVE_TIMEOUT*1000UL)) {
         if (pingOutstanding) {
            _client->stop();
            return false;
         } else {
            buffer[0] = MQTTPINGREQ;
            buffer[1] = 0;
            _client->write(buffer,2);
            lastOutActivity = t;
            lastInActivity = t;
            pingOutstanding = true;
         }
      }
      if (_client->available()) {
         uint8_t llen;
         uint16_t len = readPacket(&llen);
         if (len > 0) {
            lastInActivity = t;
            uint8_t type = buffer[0]&0xF0;
            if (type == MQTTPUBLISH) {
               if (callback) {
                  uint16_t tl = (buffer[llen+1]<<8)+buffer[llen+2];
                  char topic[tl+1];
                  for (uint16_t i=0;i<tl;i++) {
                     topic[i] = buffer[llen+3+i];
                  }
                  topic[tl] = 0;
                  // ignore msgID - only support QoS 0 subs
                  uint8_t *payload = buffer+llen+3+tl;
                  callback(topic,payload,len-llen-3-tl);
               }
            } else if (type == MQTTPINGREQ) {
               buffer[0] = MQTTPINGRESP;
               buffer[1] = 0;
               _client->write(buffer,2);
            } else if (type == MQTTPINGRESP) {
               pingOutstanding = false;
            }
         }
      }
      return true;
   }
   return false;
}

boolean PubSubClientLD::publish(char* topic, char* payload) {
   return publish(topic,(uint8_t*)payload,strlen(payload),false);
}

boolean PubSubClientLD::publish(char* topic, uint8_t* payload, unsigned int plength) {
   return publish(topic, payload, plength, false);
}

boolean PubSubClientLD::publish(char* topic, uint8_t* payload, unsigned int plength, boolean retained) {
   if (connected()) {
      // Leave room in the buffer for header and variable length field
      uint16_t length = 5;
      length = writeString(topic,buffer,length);
      uint16_t i;
      for (i=0;i<plength;i++) {
         buffer[length++] = payload[i];
      }
      uint8_t header = MQTTPUBLISH;
      if (retained) {
         header |= 1;
      }
      return write(header,buffer,length-5);
   }
   return false;
}

boolean PubSubClientLD::publish_P(char* topic, uint8_t* PROGMEM payload, unsigned int plength, boolean retained) {
   uint8_t llen = 0;
   uint8_t digit;
   int rc;
   uint16_t tlen;
   int pos = 0;
   int i;
   uint8_t header;
   unsigned int len;
   
   if (!connected()) {
      return false;
   }
   
   tlen = strlen(topic);
   
   header = MQTTPUBLISH;
   if (retained) {
      header |= 1;
   }
   buffer[pos++] = header;
   len = plength + 2 + tlen;
   do {
      digit = len % 128;
      len = len / 128;
      if (len > 0) {
         digit |= 0x80;
      }
      buffer[pos++] = digit;
      llen++;
   } while(len>0);
   
   pos = writeString(topic,buffer,pos);
   
   rc += _client->write(buffer,pos);
   
   for (i=0;i<plength;i++) {
      rc += _client->write((char)pgm_read_byte_near(payload + i));
   }
   
   lastOutActivity = millis();
   return rc == len + 1 + plength;
}

boolean PubSubClientLD::write(uint8_t header, uint8_t* buf, uint16_t length) {
   uint8_t lenBuf[4];
   uint8_t llen = 0;
   uint8_t digit;
   uint8_t pos = 0;
   uint8_t rc;
   uint8_t len = length;
   do {
      digit = len % 128;
      len = len / 128;
      if (len > 0) {
         digit |= 0x80;
      }
      lenBuf[pos++] = digit;
      llen++;
   } while(len>0);

   buf[4-llen] = header;
   for (int i=0;i<llen;i++) {
      buf[5-llen+i] = lenBuf[i];
   }
   rc = _client->write(buf+(4-llen),length+1+llen);
   
   lastOutActivity = millis();
   return (rc == 1+llen+length);
}

boolean PubSubClientLD::subscribe(char* topic) {
   if (connected()) {
      // Leave room in the buffer for header and variable length field
      uint16_t length = 5;
      nextMsgId++;
      if (nextMsgId == 0) {
         nextMsgId = 1;
      }
      buffer[length++] = (nextMsgId >> 8);
      buffer[length++] = (nextMsgId & 0xFF);
      length = writeString(topic, buffer,length);
      buffer[length++] = 0; // Only do QoS 0 subs
      return write(MQTTSUBSCRIBE|MQTTQOS1,buffer,length-5);
   }
   return false;
}

boolean PubSubClientLD::unsubscribe(char* topic) {
   if (connected()) {
      uint16_t length = 5;
      nextMsgId++;
      if (nextMsgId == 0) {
         nextMsgId = 1;
      }
      buffer[length++] = (nextMsgId >> 8);
      buffer[length++] = (nextMsgId & 0xFF);
      length = writeString(topic, buffer,length);
      return write(MQTTUNSUBSCRIBE|MQTTQOS1,buffer,length-5);
   }
   return false;
}

void PubSubClientLD::disconnect() {
   buffer[0] = MQTTDISCONNECT;
   buffer[1] = 0;
   _client->write(buffer,2);
   _client->stop();
   lastInActivity = lastOutActivity = millis();
}

uint16_t PubSubClientLD::writeString(char* string, uint8_t* buf, uint16_t pos) {
   char* idp = string;
   uint16_t i = 0;
   pos += 2;
   while (*idp) {
      buf[pos++] = *idp++;
      i++;
   }
   buf[pos-i-2] = (i >> 8);
   buf[pos-i-1] = (i & 0xFF);
   return pos;
}


boolean PubSubClientLD::connected() {
   boolean rc;
   if (_client == NULL ) {
      rc = false;
   } else {
      rc = (int)_client->connected();
      if (!rc) _client->stop();
   }
   return rc;
}

