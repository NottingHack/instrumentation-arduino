#include "Config.h"
#include "net_messaging.h"


// Send ping request
void net_tx_ping()
{
  struct net_msg_t msg;
  memset(&msg, 0, sizeof(msg));
  memcpy(msg.msgtype,"PING", 4);
  strncpy((char*)msg.payload, "1", 1);
  net_transport_send(&msg);
}

// Cash sale
void net_tx_cash(int amount_scaled, char *pos)
{
  struct net_msg_t msg;
  memset(&msg, 0, sizeof(msg));
  memcpy(msg.msgtype,"CASH", 4);
  sprintf((char*)msg.payload, "%d:%s", amount_scaled, pos);
  net_transport_send(&msg);
}

// Vend cancel
void net_tx_vcan(char *rfid_serial, char *tran_id)
{
  struct net_msg_t msg;
  memset(&msg, 0, sizeof(msg));
  memcpy(msg.msgtype,"VCAN", 4);
  sprintf((char*)msg.payload, "%s:%s", rfid_serial, tran_id);
  net_transport_send(&msg);  
}

// Vend failure
void net_tx_vfail(char *rfid_serial, char *tran_id)
{
  struct net_msg_t msg;
  memset(&msg, 0, sizeof(msg));
  memcpy(msg.msgtype,"VFAL", 4);
  sprintf((char*)msg.payload, "%s:%s", rfid_serial, tran_id);
  net_transport_send(&msg);  
}

// Vend success
void net_tx_vsuc(char *pos, char *rfid_serial, char *tran_id)
{
  struct net_msg_t msg;
  memset(&msg, 0, sizeof(msg));
  memcpy(msg.msgtype,"VSUC", 4);
  sprintf((char*)msg.payload, "%s:%s:%s", rfid_serial, tran_id, pos);
  net_transport_send(&msg);  
}

// Vend request
void net_tx_vreq(int amount_scaled, char *rfid_serial, char *tran_id)
{
  struct net_msg_t msg;
  memset(&msg, 0, sizeof(msg));
  memcpy(msg.msgtype,"VREQ", 4);
  sprintf((char*)msg.payload, "%s:%s:%d", rfid_serial, tran_id, amount_scaled);
  net_transport_send(&msg);  
}

// Auth card - on card first being presented, prior to telling the vending machine
void net_tx_auth(char *rfid_serial, uint8_t rfid_serial_size)
{
  struct net_msg_t msg;
  memset(&msg, 0, sizeof(msg));
  memcpy(msg.msgtype,"AUTH", 4);
  strncpy((char*)msg.payload, rfid_serial, rfid_serial_size);
  net_transport_send(&msg);
}


// Request nh-vend send us what debug level we should be running at
void net_tx_debug(char *rfid_serial, uint8_t rfid_serial_size)
{
  struct net_msg_t msg;
  memcpy(msg.msgtype,"DBUG", 4);
  strncpy((char*)msg.payload, rfid_serial, rfid_serial_size);
  net_transport_send(&msg);
}

void net_rx_message(struct net_msg_t *net_msg, char *rfid_serial, uint8_t rfid_serial_size, char *tran_id, uint8_t tran_id_size, cState *card_state, byte *allowVend, LiquidCrystal_I2C *lcd)
{
  if (!(strncmp((char*)net_msg->msgtype, "GRNT", 4)))
    net_rx_grant(net_msg->payload, rfid_serial, rfid_serial_size, tran_id, tran_id_size, card_state, net_msg);

  else if (!(strncmp((char*)net_msg->msgtype, "DENY", 4)))
    net_rx_deny(net_msg->payload, rfid_serial, rfid_serial_size, card_state);

  else if (!(strncmp((char*)net_msg->msgtype, "VNOK", 4))) // VeNd OK
    net_rx_vend_ok(net_msg->payload, allowVend, net_msg, rfid_serial, tran_id);

  else if (!(strncmp((char*)net_msg->msgtype, "VDNY", 4))) // Vend DeNY
    net_rx_vend_deny(allowVend);

  else if (!(strncmp((char*)net_msg->msgtype, "DBUG", 4))) // DeBUG
    net_rx_set_debug(net_msg->payload);

  else if (!(strncmp((char*)net_msg->msgtype, "DISP", 4))) // DISPlay
    net_rx_display(net_msg->payload, lcd);

  else
    dbg_println(F("Unknown msg rcv"));
}


void net_rx_display(byte *payload, LiquidCrystal_I2C *lcd)
{
  // Display payload on LCD display - \n in string seperates line 1+2.
  char ln1[LCD_WIDTH+1];
  char ln2[LCD_WIDTH+1];
  char *payPtr, *lnPtr;

  payPtr = (char*)payload;

  memset(ln1, 0, sizeof(ln1));
  memset(ln2, 0, sizeof(ln2)); 
  
  /* Clear display */
  lcd->clear();
  
  lnPtr = ln1;
  for (int i=0; i < LCD_WIDTH; i++)
  {    
    if ((*payPtr=='\n') || (*payPtr=='\0') || (*payPtr=='\r'))
    {
      payPtr++;
      break;
    }
    
   *lnPtr++ = *payPtr++;    
  }

  lnPtr = ln2;
  for (int i=0; i < LCD_WIDTH; i++)
  { 
    if (*payPtr=='\0')
      break;
      
    if (*payPtr=='\n')
    {
      payPtr++;
      *lnPtr=' ';
    }
    
    *lnPtr++ = *payPtr++;
  }
  
  /* Output 1st line */
  lcd->print(ln1); 
  
  /* Output 2nd line */
  lcd->setCursor(0, 1);
  lcd->print(ln2);
}



void net_rx_set_debug(byte *payload)
{
  // Set debug level (update EEPROM if applicable)
  
  short debug_level;
  short new_dlvl;
  
  
  debug_level = EEPROM.read(EEPROM_DEBUG);

  switch (*payload)
  {
    case '0':
      new_dlvl = 0;
      break;
      
    case '1':
      new_dlvl = 1;
      break;

    case '2':
      new_dlvl = 2;
      break;
      
    default:
      return;
  }
 
    
  if (debug_level != new_dlvl)
  {
    // update eeprom
    EEPROM.write(EEPROM_DEBUG, new_dlvl);
    EEPROM.commit();
    dbg_println(F("EEPROM Updated"));
  }
    
  gDebug = new_dlvl;  
  dbg_println(F("set debug"));
  gDebug = 2;
} 



void net_rx_vend_deny(byte *allowVend)
{
  // Not too bothered about the payload here - either the card number in the message matches, in which case
  // we want to decline the card, or it doesn't and something's gone wrong, so we want to decline the card...
  
  dbg_println(F("Card dcl (2)")); 
  *allowVend = 2;
} 



void net_rx_vend_ok(byte *payload, byte *allowVend, struct net_msg_t *net_msg, char *rfid_serial, char *tran_id)
{
  // Expeceted format:
  // <card>:<transaction id>

  unsigned int i;
  
  for (i=0; (i < sizeof(net_msg->payload)) && (payload[i] != ':'); i++); // Find ':'
  if (payload[i++] != ':')
  {
    // invalid message
    *allowVend = 2;
    dbg_println(F("Vend dcl (1)"));
    return;
  }
  
  // Check we still have the card details
  if (strlen(rfid_serial) <= 1)
  {
    *allowVend = 2;
    dbg_println(F("Card details lost!"));
    return;
  }
  
  // card number in message must match stored card number
  if (strncmp(rfid_serial, (char*)payload, strlen(rfid_serial)))
  {
    *allowVend = 2;
    dbg_println(F("Card mismatch"));
    return;
  }
    
  // stored transaction number must match that in the message
  if (strncmp(tran_id, (char*)payload+i, strlen(tran_id)))
  {
    *allowVend = 2;
    dbg_println(F("tran_id mismatch"));
    return;
  }    
    
  // To get here, all must be good - so allow vend
  *allowVend = 1;
  dbg_println(F("Allowing vend"));
} 

void net_rx_deny(byte *payload, char *rfid_serial, uint8_t rfid_serial_size, cState *card_state)
{
  // Not too bothered about the payload here - either the card number in the message matches, in which case
  // we want to decline the card, or it doesn't and something's gone wrong, so we want to decline the card...
  
  dbg_println(F("Card dcl"));
  memset(rfid_serial, 0, rfid_serial_size);
  *card_state = NO_CARD; 
} 


void net_rx_grant(byte *payload, char *rfid_serial, uint8_t rfid_serial_size, char *tran_id, uint8_t tran_id_size, cState *card_state, struct net_msg_t *net_msg)
{
  // expected payload format:
  // <rfid serial>:<transaction id>
  unsigned int i;
  char buf[25];
  
  for (i=0; (i < sizeof(net_msg->payload)) && (payload[i] != ':'); i++); // Find ':'
  if (payload[i++] != ':')
  {
    *card_state = NO_CARD;
    memset(rfid_serial, 0, rfid_serial_size);
    dbg_println(F("tID not found"));
    return;
  }
  
  // check card number matches
  if (!strncmp((char*)payload, rfid_serial, i))
  {
    // hmmm, the card read doesn't match the one that's just been accepted.
    *card_state = NO_CARD;
    memset(rfid_serial, 0, rfid_serial_size);
    dbg_println(F("serial mismatch"));
    return;
  }
    
  memcpy(tran_id, payload+i, tran_id_size-1);
  sprintf(buf, "trnid=%s", tran_id);
  dbg_println(buf);
  *card_state = GOOD;

}
