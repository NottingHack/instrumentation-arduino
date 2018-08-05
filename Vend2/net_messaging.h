
#ifndef NETMESSAGING_H
#define NETMESSAGING_H


#include <FlashAsEEPROM.h>
#include <LiquidCrystal_I2C.h>
#include "net_transport.h"
#include "debug.h"

extern short gDebug;

void net_tx_ping();
void net_tx_cash(int amount_scaled, char *pos);
void net_tx_vcan(char *rfid_serial, char *tran_id);
void net_tx_vfail(char *rfid_serial, char *tran_id);
void net_tx_vsuc(char *pos, char *rfid_serial, char *tran_id);
void net_tx_vreq(int amount_scaled, char *rfid_serial, char *tran_id);
void net_tx_auth(char *rfid_serial, uint8_t rfid_serial_size);
void net_tx_debug(char *rfid_serial, uint8_t rfid_serial_size);

void net_rx_message(struct net_msg_t *net_msg, char *rfid_serial, uint8_t rfid_serial_size, char *tran_id, cState *card_state, byte *allowVend, LiquidCrystal_I2C *lcd);
void net_rx_display(byte *payload, LiquidCrystal_I2C *lcd);
void net_rx_set_debug(byte *payload);
void net_rx_vend_ok(byte *payload, byte *allowVend, struct net_msg_t *net_msg, char *rfid_serial, char *tran_id);
void net_rx_vend_deny(byte *allowVend);
void net_rx_deny(byte *payload, char *rfid_serial, uint8_t rfid_serial_size, cState *card_state);
void net_rx_grant(byte *payload, char *rfid_serial, uint8_t rfid_serial_size, char *tran_id, cState *card_state, struct net_msg_t *net_msg);

#endif
