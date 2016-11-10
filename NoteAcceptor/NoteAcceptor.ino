/*
 * Copyright (c) 2013, Daniel Swann <hs@dswann.co.uk>
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the owner nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */


/* Arduino note acceptor for Nottingham Hackspace
 *
 * Target board = Arduino Uno, Arduino version = 1.0.1
 *
 * Expects to be connected to:
 *   - Innovative NV4 note acceptor, configured for serial mode, 300bps
 *   - I2C 20x4 LCD (HCARDU0023)
 *   - SPI RFID reader (HCMODU0016)
 *   - Ethernet shield
 *   - Light up cancel push button
 * Pin assignments are in Config.h
 *
 * Additional required libraries:
 *   - PubSubClient                       - https://github.com/knolleary/pubsubclient
 *   - HCARDU0023_LiquidCrystal_I2C_V2_1  - http://forum.hobbycomponents.com/viewtopic.php?f=39&t=1368
 *   - rfid                               - https://github.com/miguelbalboa/rfid
 */

#include <SoftwareSerial.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <MFRC522.h>
#include <EEPROM.h>
#include "Config.h"
#include "nv4.h"

#define LCD_WIDTH 20

void callbackMQTT(char* topic, byte* payload, unsigned int length);
uint8_t set_state(state_t new_state);
void message_handler(uint8_t cmd, uint8_t val);
void mqtt_rx_grant(char *payload, unsigned int len);
void mqtt_rx_deny(char *payload);
void mqtt_rx_vend_ok(char *payload, unsigned int len);
void mqtt_rx_vend_deny(char *payload);
void mqtt_rx_display(char *payload);
void timeout_check();
void poll_rfid();
void dbg_println(const __FlashStringHelper *n);
void dbg_println(const char *msg);

EthernetClient ethClient;
PubSubClient client(server, MQTT_PORT, callbackMQTT, ethClient);
LiquidCrystal_I2C lcd(0x3F,20,4);  // 20x4 display at address 0x3F
MFRC522 rfid_reader(PIN_RFID_SS, PIN_RFID_RST);


char pmsg[DMSG];
unsigned long card_number;
char rfid_serial[20];
char tran_id[10]; // transaction id
char mqtt_rx_buf[45];
unsigned long state_entered;
char status_msg[31];


state_t state;

/**************************************************** 
 * callbackMQTT
 * called when we get a new MQTT
 * work out which topic was published to and handle as needed
 ****************************************************/
void callbackMQTT(char* topic, byte* payload, unsigned int length)
{
  if (length >= sizeof(mqtt_rx_buf))
    return;

  memset(mqtt_rx_buf, 0, sizeof(mqtt_rx_buf));
  memcpy(mqtt_rx_buf, payload, length);

  if (!strcmp(topic, S_NOTE_RX))
  {
    dbg_println("Got message: ");
    dbg_println(mqtt_rx_buf);

    if (!(strncmp(mqtt_rx_buf, "GRNT", 4)))
      mqtt_rx_grant(mqtt_rx_buf, length);

    else if (!(strncmp(mqtt_rx_buf, "DENY", 4)))
      mqtt_rx_deny(mqtt_rx_buf);

    else if (!(strncmp(mqtt_rx_buf, "VNOK", 4))) // VeNd OK. Or OK to accept note in this case...
      mqtt_rx_vend_ok(mqtt_rx_buf, length);

    else if (!(strncmp(mqtt_rx_buf, "DISP", 4))) 
      mqtt_rx_display(mqtt_rx_buf);

    else if (!(strncmp(mqtt_rx_buf, "VDNY", 4))) // Server has told us not to accept the payment for some reason
      mqtt_rx_vend_deny(mqtt_rx_buf);
  }

  // Respond to status requests
  else if (!strcmp(topic, S_STATUS))
  {
    if (!strncmp(STATUS_STRING, (char*)payload, strlen(STATUS_STRING)))
    {
      // dbg_println(F("Status Request"));
      client.publish(P_STATUS, status_msg);
    }
  }

} // end void callback(char* topic, byte* payload,int length)

void mqtt_rx_grant(char *payload, unsigned int len)
{
  unsigned int i;
  char buf[25];

  // expected payload format:
  // GRNT:<rfid serial>:<transaction id>

  if (len < 9)
  {
    dbg_println(F("Invalid GRNT message!"));
    set_state(STATE_READY);
    return;
  }

  payload += sizeof("GRNT"); // Skip over "GRNT:"

  for (i=0; (i < len) && (payload[i] != ':'); i++); // Find ':'
  if (payload[i++] != ':')
  {
    dbg_println(F("tID not found"));
    set_state(STATE_READY);
    return;
  }

  // check card number matches
  if (strncmp(payload, rfid_serial, strlen(rfid_serial)))
  {
    // hmmm, the card read doesn't match the one that's just been accepted.
    dbg_println(F("Card serial mismatch"));
    dbg_println(payload);
    dbg_println(rfid_serial);
    set_state(STATE_READY);
    return;
  }

  memcpy(tran_id, payload+i, sizeof(tran_id)-1);
  sprintf(buf, "trnid=%s", tran_id);
  dbg_println(buf);
  set_state(STATE_ACCEPT);
}

void mqtt_rx_deny(char *payload)
// Unknown / invalid card 
{
  dbg_println(F("DENY received.."));
  lcd.clear();
  lcd.home();
  lcd.print("Unknown card...");
  delay(400);
  set_state(STATE_READY);
}

void mqtt_rx_vend_ok(char *payload, unsigned int len)
{
  unsigned int i;
  // Expeceted format:
  // VNOK:<card>:<transaction id>

  if (len < 9)
  {
    dbg_println("Invalid VNOK message!");
    set_state(STATE_READY);
    return;
  }

  // Skip over "VNOK:"
  payload += sizeof("VNOK");

  for (i=0; (i < len) && (payload[i] != ':'); i++); // Find ':'
  if (payload[i++] != ':')
  {
    // invalid message
    dbg_println(F("Vend denied (1)"));
    set_state(STATE_READY);
    return;
  }

  // Check we still have the card details
  if (strlen(rfid_serial) <= 1)
  {
    dbg_println(F("Card details lost!"));
    set_state(STATE_READY);
    return;
  }

  // card number in message must match stored card number
  if (strncmp(rfid_serial, payload, strlen(rfid_serial)))
  {
    dbg_println(F("Card mismatch"));
    set_state(STATE_READY);
    return;
  }

  // stored transaction number must match that in the message
  if (strncmp(tran_id, payload+i, strlen(tran_id)))
  {
    dbg_println(F("tran_id mismatch"));
    set_state(STATE_READY);
    return;
  }

  // To get here, all must be good, so accept the escrowed note
  dbg_println(F("VNOK: All good"));
  nv4_accept_escrow();

  set_state(STATE_ACCEPT_NOTE_WAIT);
}

void mqtt_rx_vend_deny(char *payload)
{
  if (state == STATE_ACCEPT_NET_WAIT)
  {
    lcd.clear();
    lcd.home();
    lcd.print("Failed to record");
    lcd.setCursor(0,1);
    lcd.print("payment");
    delay(1500);
    set_state(STATE_READY);
  } else
  { // Message out of sequence
    dbg_println(F("Reveived OOS VNDY message"));
  }
}

void mqtt_rx_display(char *payload)
{
  // Display payload on LCD display - \n in string seperates line 1+2.
  char ln1[LCD_WIDTH+1];
  char ln2[LCD_WIDTH+1];
  char *payPtr, *lnPtr;

  // Skip over "DISP:"
  payload += sizeof("DISP");

  payPtr = (char*)payload;

  memset(ln1, 0, sizeof(ln1));
  memset(ln2, 0, sizeof(ln2)); 

  /* Clear display */
  //lcd.clear();

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
  lcd.setCursor(0, 2);
  lcd.print(ln1); 

  /* Output 2nd line */
  lcd.setCursor(0, 3);
  lcd.print(ln2); 
}

/**************************************************** 
 * check we are still connected to MQTT
 * reconnect if needed
 *
 ****************************************************/
void checkMQTT() 
{
  if (!client.connected()) 
  {
    if (client.connect(CLIENT_ID)) 
    {
      dbg_println(F("Connected to MQTT"));
      set_state(STATE_READY);
      client.publish(P_STATUS, RESTART);
      client.subscribe(S_NOTE_RX);
      client.subscribe(S_STATUS);
    } else
    {
      set_state(STATE_CONN_WAIT);
    }
  }
}

void setup()
{
  Serial.begin(9600);
  dbg_println(F("Start!"));

  state = STATE_CONN_WAIT;

  dbg_println(F("Init LCD"));
  lcd.init();
  lcd.backlight();
  lcd.home();
  lcd.print(F("Nottinghack note    "));
  lcd.setCursor(0, 1);
  lcd.print(F("acceptor v0.01....  "));

  dbg_println(F("Init SPI"));
  SPI.begin();

  // Start serial for note acceptor
  dbg_println(F("Init NV4"));
  nv4_init(message_handler, PIN_NOTE_RX, PIN_NOTE_TX);

  // Accept notes for channels with a configured value of > 0
  for (uint8_t i=NV4_CHANNEL1; i <= NV4_CHANNEL16; i++)
  {
    if (channel_value[i])
      nv4_uninhibit(i);
    else
      nv4_inhibit(i);
  }

  // Enable escrow mode - give us a chance to reject the note if we fail to record it
  nv4_enable_escrow();

  while(nv4_loop());

  pinMode(PIN_CANCEL_BUTTON, INPUT);
  digitalWrite(PIN_CANCEL_BUTTON, HIGH);
  
  strcpy(status_msg, RUNNING);
  
  dbg_println(F("Start Ethernet"));
  Ethernet.begin(mac, ip);

  dbg_println(F("Init RFID"));
  rfid_reader.PCD_Init();

  // Start MQTT and say we are alive
  dbg_println(F("Check MQTT"));
  checkMQTT();

  /* If the acceptor was jammed prior to the last reset, assume it still is, unless the cancel button has been held down on power on */
  if (EEPROM.read(EEPROM_JAMMED)==1)
  {
    if (digitalRead(PIN_CANCEL_BUTTON) == LOW)
    {
      // clear the jammed EEPROM flag 
      EEPROM.write(EEPROM_JAMMED, 0);
      dbg_println(F("Clearing JAMMED flag"));
    } else
    {
      dbg_println(F("Prior JAM..."));
      set_state(STATE_JAMMED);
    }
  }


  delay(100);

  dbg_println(F("Setup done..."));
} // end void setup()

uint8_t set_state(state_t new_state)
{
  if (state == STATE_JAMMED) // Don't auto-exit from a jammed state
    return 0;

  state_entered = millis();

  switch(new_state)
  {
    case STATE_CONN_WAIT:
      dbg_println(F("Setting state: CONN_WAIT"));
      if (state >= STATE_READY)
      {
        // State has gone backwards to CONN_WAIT... we had a connection, but lost it.
        lcd.clear();
        lcd.home();
        lcd.print("Connection lost!");

        // Abort anything in progress
        if (state == STATE_ACCEPT_NET_WAIT)
        {
          dbg_println(F("Rejecting note."));
          nv4_reject_escrow();
        }
        nv4_disable_all();
        card_number = 0;
        memset(rfid_serial, 0, sizeof(rfid_serial));
        memset(tran_id, 0, sizeof(tran_id));
      }
      state = STATE_CONN_WAIT;

      break;

    case STATE_READY:
      dbg_println(F("Setting state: READY"));

      if (state == STATE_ACCEPT_NET_WAIT) // To go from STATE_ACCEPT_NET_WAIT->STATE_READY means something's gone wrong. Reject the note.
      {
        dbg_println(F("Rejecting note."));
        nv4_reject_escrow();
      }

      nv4_disable_all();
      card_number = 0;
      memset(rfid_serial, 0, sizeof(rfid_serial));
      memset(tran_id, 0, sizeof(tran_id));
      lcd.clear();
      lcd.home();
      lcd.print(F("Please scan RFID    "));
      lcd.setCursor(0, 1);
      lcd.print(F("card.... "));

      state = STATE_READY;
      break;

    case STATE_AUTH_WAIT:
      dbg_println(F("Setting state: AUTH_WAIT"));
      lcd.clear();
      lcd.home();
      lcd.print(F("Card lookup..."));
      state = STATE_AUTH_WAIT;
      break;

    case STATE_ACCEPT:
      nv4_enable_all();
      dbg_println(F("Setting state: STATE_ACCEPT"));
      lcd.clear();
      lcd.home();
      lcd.print(F("Please insert note"));
      state = STATE_ACCEPT;
      break;

    case STATE_ACCEPT_NET_WAIT:
      dbg_println(F("Setting state: STATE_ACCEPT_NET_WAIT"));
      state = STATE_ACCEPT_NET_WAIT;
      break;

    case STATE_ACCEPT_NOTE_WAIT:
      dbg_println(F("Setting state: STATE_ACCEPT_NOTE_WAIT"));
      state = STATE_ACCEPT_NOTE_WAIT;
      break;

    case STATE_JAMMED:
      lcd.clear();
      lcd.home();
      lcd.print(F("Out of Order:"));
      lcd.setCursor(0,1);
      lcd.print(F("Note jam detected"));
      nv4_disable_all();
      card_number = 0;
      memset(rfid_serial, 0, sizeof(rfid_serial));
      memset(tran_id, 0, sizeof(tran_id));
      state = STATE_JAMMED;
      strcpy(status_msg, JAMMED);

      // Remember this, so a power cycle doesn't clear this state (as it'll likley still be physically jammed)
      EEPROM.write(EEPROM_JAMMED, 1);
      sprintf(pmsg, "JAMMED", rfid_serial, tran_id);
      client.publish(P_NOTE_TX, pmsg);
      break;

    default:
      break;
  }

  // Illuminate the cancel button light only if in accept state
  digitalWrite(PIN_CANCEL_LIGHT, (state==STATE_ACCEPT ? HIGH : LOW));

  return 0;
}

void loop()
{
  // Poll MQTT
  // should cause callback if there's a new message
  client.loop();

  // are we still connected to MQTT
  checkMQTT();

  // Check for data from note acceptor 
  nv4_loop();

  // Check for RFID card
  poll_rfid();

  // Check for timeouts
  timeout_check();

  // Check for cancel button being pushed
  if ((state == STATE_ACCEPT) && (digitalRead(PIN_CANCEL_BUTTON) == LOW))
  {
    dbg_println(F("User cancel!"));
    sprintf(pmsg, "VCAN:%s:%s", rfid_serial, tran_id);
    client.publish(P_NOTE_TX, pmsg);
    set_state(STATE_READY);
  }

  // Ensure that if/when a jam gets cleared, that the acceptor is disabled. For some reason, when cleared, the acceptor seems to
  // swallow the next note inserted without giving us the option to reject it! So repeatley call the disable function to stop that.
  if (state == STATE_JAMMED)
  {
    nv4_disable_all();
  }

} // end void loop()


void message_handler(uint8_t cmd, uint8_t val)
/* Handle messages from note acceptor */
{
  char buf[21];

  sprintf(buf, "mh> cmd=%d, val=%d", cmd, val);
  
  // If jammed, we constantly send out a "DISABLE_ALL" command (so it's echoed back by the note acceptor). Don't flood the debug log with this.
  if (!(state == STATE_JAMMED && cmd == NV4_DISABLE_ALL))
    dbg_println(buf);

  if (cmd == NV4_ACCEPT)
  {
    if (state == STATE_ACCEPT)
    {
      // Note first inserted - to be held in escrow until we get a VNOK message
      sprintf(pmsg, "VREQ:%s:%s:%d", rfid_serial, tran_id, channel_value[val]);
      client.publish(P_NOTE_TX, pmsg);
      set_state(STATE_ACCEPT_NET_WAIT);

      lcd.clear();
      lcd.home();
      lcd.print(F("Recording payment:"));
      sprintf(buf, "%dp", channel_value[val]);
      lcd.setCursor(0, 1);
      lcd.print(buf);
    } else if (state == STATE_ACCEPT_NOTE_WAIT)
    {
      // Note accepted
      sprintf(pmsg, "VSUC:%s:%s:CHANNEL%d", rfid_serial, tran_id, val);
      client.publish(P_NOTE_TX, pmsg);
      set_state(STATE_READY);
    } else
    {
      // Unexpected note - acceptor shouldn't have been active!
      dbg_println(F("Unexpected note!"));
      nv4_reject_escrow();
      nv4_disable_all();
    }
  } else if (cmd == NV4_ESCROW_ABORT)
  {
    lcd.clear();
    lcd.home();
    lcd.print(F("Aborted!"));
    delay(1000);
    set_state(STATE_READY);
  } else if (cmd == NV4_NOT_RECOGNISED)
  {
    lcd.setCursor(0, 1);
    lcd.print(F("Note not recognised"));

    // reset timeout
    state_entered = millis();
  } else if ((cmd == NV4_STACKER_FULL) || /* jammed */
             (cmd == NV4_NOTE_TAKEN))     /* Note swallowed that shouldn't have been... */
  {
    set_state(STATE_JAMMED);
    dbg_println(F("ERROR: Jam reported!"));
  }

}

void poll_rfid()
{
  byte *pCard_number = (byte*)&card_number;

  if (state != STATE_READY)
    return;

  if (!rfid_reader.PICC_IsNewCardPresent())
    return;

  if (!rfid_reader.PICC_ReadCardSerial())
    return;

  // Convert 4x bytes received to long (4 bytes)
  for (int i = rfid_reader.uid.size-1; i >= 0; i--) 
    *(pCard_number++) = rfid_reader.uid.uidByte[i];

  ultoa(card_number, rfid_serial, 10);

  sprintf(pmsg, "AUTH:%s", rfid_serial);
  client.publish(P_NOTE_TX, pmsg);
  set_state(STATE_AUTH_WAIT);
}

void timeout_check()
{
  if
  (
    ((state == STATE_AUTH_WAIT      ) && abs((millis() - state_entered) > TIMEOUT_COMS)) ||
    ((state == STATE_ACCEPT_NET_WAIT) && abs((millis() - state_entered) > TIMEOUT_COMS))
  )
  {
    lcd.clear();
    lcd.home();
    lcd.print(F("Error - timeout..."));
    dbg_println("Error: timeout");
    delay(1500);
    set_state(STATE_READY);
    return;
  }

  if ((state == STATE_ACCEPT) && abs((millis() - state_entered) > TIMEOUT_SES ))
  {
    // Session timeout
    lcd.clear();
    lcd.home();
    lcd.print("Timeout!");
    dbg_println("Session timeout");
    sprintf(pmsg, "VCAN:%s:%s", rfid_serial, tran_id);
    client.publish(P_NOTE_TX, pmsg);
    delay(1500);
    set_state(STATE_READY);
  }
}

void dbg_println(const __FlashStringHelper *n)
{
  uint8_t c;
  byte *payload;
  char *pn = (char*)n;

  memset(pmsg, 0, sizeof(pmsg));
  strcpy(pmsg, "INFO:");
  payload = (byte*)pmsg + sizeof("INFO");

  while ((c = pgm_read_byte_near(pn++)) != 0)
    *(payload++) = c;

#ifdef DEBUG_MQTT
  client.publish(P_NOTE_TX, pmsg);
#endif

#ifdef DEBUG_SERIAL
  Serial.println(pmsg+sizeof("INFO"));
#endif
}

void dbg_println(const char *msg)
{
  byte *payload;

  memset(pmsg, 0, sizeof(pmsg));
  strcpy(pmsg, "INFO:");
  strcat(pmsg, msg);
  payload = (byte*)pmsg + sizeof("INFO");

#ifdef DEBUG_MQTT
  if (state > STATE_CONN_WAIT)
    client.publish(P_NOTE_TX, pmsg);
#endif

#ifdef DEBUG_SERIAL
  Serial.println(pmsg+sizeof("INFO"));
#endif
}

