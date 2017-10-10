/*
 * Copyright (c) 2017, Daniel Swann <hs@dswann.co.uk>, Robert Hunt <rob.hunt@nottinghack.org.uk>
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


/* Arduino coin acceptor for Nottingham Hackspace
 *
 * Target board = Arduino Uno, Arduino version = 1.8.5
 *
 * Expects to be connected to:
 *   - Sintron CH-92x coin acceptor, configured for fast pulse mode (30ms) and NC (normally closed) coin line
 *   - I2C 20x4 LCD (any LCD with PCF8574 based I2C interface)
 *   - SPI RFID reader (RFID-RC522)
 *   - Ethernet shield (WIZnet based)
 *   - Light up cancel push button
 * Pin assignments are in Config.h
 *
 * Additional required libraries:
 *   - PubSubClient       - https://github.com/knolleary/pubsubclient
 *   - LiquidCrystal_I2C  - https://github.com/marcoschwartz/LiquidCrystal_I2C
 *   - rfid               - https://github.com/miguelbalboa/rfid
 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <MFRC522.h>
#include "Config.h"

#define LCD_WIDTH 20

typedef MFRC522::Uid rfid_uid;

void callbackMQTT(char* topic, byte* payload, unsigned int length);
uint8_t set_state(state_t new_state);
void message_handler(uint8_t cmd, uint8_t val);
void mqtt_rx_grant(char *payload, unsigned int len);
void mqtt_rx_deny(char *payload);
void mqtt_rx_vend_ok(char *payload, unsigned int len);
void mqtt_rx_vend_deny(char *payload);
void mqtt_rx_display(char *payload);
void onCoinPulse();
void coinAcceptorEnable();
void coinAcceptorDisable();
void timeout_check();
void poll_rfid();
void dbg_println(const __FlashStringHelper *n);
void dbg_println(const char *msg);
void cpy_rfid_uid(rfid_uid *dst, rfid_uid *src);
bool eq_rfid_uid(rfid_uid u1, rfid_uid r2);
void uid_to_hex(char *uidstr, 
rfid_uid uid);

void resetAnimation();
void coin_check();
void displayCountingAnimation();
void formatCurrency(char buf[], int pence);
void doAuth();
void lcd_print(const __FlashStringHelper *line1);
void lcd_print(const __FlashStringHelper *line1, const __FlashStringHelper *line2);
void __debug();
void __prepareDebug();

EthernetClient ethClient;
PubSubClient client(server, MQTT_PORT, callbackMQTT, ethClient);
LiquidCrystal_I2C lcd(0x27, 20, 4);  // 20 x 4 character display at address 0x27
MFRC522 rfid_reader(PIN_RFID_SS, PIN_RFID_RST);

char pmsg[DMSG];
rfid_uid card_number;
char rfid_serial[21];
char tran_id[10]; // transaction id
char mqtt_rx_buf[45];
unsigned long state_entered;
char status_msg[31];

volatile int creditInPence = 0;
volatile long lastCreditChangeTime = 0;
long previousCreditInPence = 0;

state_t state;

byte animation[LCD_WIDTH];

const char lcd_chars[][8] PROGMEM = {
  {0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00}, // counting animation frame 1
  {0x00, 0x00, 0x1F, 0x1F, 0x1F, 0x00, 0x00}, // counting animation frame 2
  {0x00, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x00}, // counting animation frame 3
  {0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F}, // counting animation frame 4
  {0x1F, 0x1F, 0x1F, 0x00, 0x1F, 0x1F, 0x1F}, // counting animation frame 5
  {0x06, 0x09, 0x08, 0x1C, 0x08, 0x09, 0x17} // pound symbol
};

#define ANIM_FRAME_1 0
#define ANIM_FRAME_2 1
#define ANIM_FRAME_3 2
#define ANIM_FRAME_4 3
#define ANIM_FRAME_5 4
#define POUND_SYMBOL 5

/**************************************************** 
 * callbackMQTT
 * called when we get a new MQTT
 * work out which topic was published to and handle as needed
 ****************************************************/
void callbackMQTT(char* topic, byte* payload, unsigned int length)
{
  if (length >= sizeof(mqtt_rx_buf))
  {
    dbg_println(F("Message exceeds mqtt_rx_buf size!"));
    return;
  }
  
  memset(mqtt_rx_buf, 0, sizeof(mqtt_rx_buf));
  memcpy(mqtt_rx_buf, payload, length);

  if (!strcmp(topic, S_COIN_RX))
  {
    dbg_println("Got message: ");
    dbg_println(mqtt_rx_buf);

    if (!(strncmp(mqtt_rx_buf, "GRNT", 4))) // GRaNT - known RFID card
      mqtt_rx_grant(mqtt_rx_buf, length);

    else if (!(strncmp(mqtt_rx_buf, "DENY", 4))) // DENY - unknown RFID card
      mqtt_rx_deny(mqtt_rx_buf);

    else if (!(strncmp(mqtt_rx_buf, "VNOK", 4))) // VeNd OK - proceed to vend, N/A for payments
      mqtt_rx_vend_ok(mqtt_rx_buf, length);

    else if (!(strncmp(mqtt_rx_buf, "DISP", 4))) // DISPlay - display message to LCD
      mqtt_rx_display(mqtt_rx_buf);

    else if (!(strncmp(mqtt_rx_buf, "VDNY", 4))) // Vend DeNY - reject vend, N/A for payments
      mqtt_rx_vend_deny(mqtt_rx_buf);
  }
  // Respond to status requests
  else if (!strcmp(topic, S_STATUS))
  {
    if (!strncmp(STATUS_STRING, (char*)payload, strlen(STATUS_STRING)))
    {
      client.publish(P_STATUS, status_msg);
    }
  }

}

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

// unknown / invalid card
void mqtt_rx_deny(char *payload)
{
  dbg_println(F("DENY received.."));
  lcd_print(F("Unknown card..."));
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

  set_state(STATE_ACCEPT_CONFIRM_WAIT);
}

void mqtt_rx_vend_deny(char *payload)
{
  if (state == STATE_ACCEPT_NET_WAIT)
  {
    lcd_print(F("Failed to record"), F("payment"));
    delay(1500);
    set_state(STATE_READY);
  }
  else
  {
    // Message out of sequence
    dbg_println(F("Reveived OOS VNDY message"));
  }
}

void mqtt_rx_display(char *payload)
{
  // Display payload on LCD display - \n in string seperates line 1+2.
  char ln1[LCD_WIDTH + 1];
  char ln2[LCD_WIDTH + 1];
  char *payPtr, *lnPtr;

  // Skip over "DISP:"
  payload += sizeof("DISP");

  payPtr = (char*)payload;

  memset(ln1, 0, sizeof(ln1));
  memset(ln2, 0, sizeof(ln2)); 

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
    dbg_println(F("Trying to connect"));
    if (client.connect(CLIENT_ID)) 
    {
      dbg_println(F("Connected to MQTT"));
      set_state(STATE_READY);
      client.publish(P_STATUS, RESTART);
      client.subscribe(S_COIN_RX);
      client.subscribe(S_STATUS);
    }
    else
    {
      set_state(STATE_CONN_WAIT);
    }
  }
}

void setup()
{
  Serial.begin(9600);
  dbg_println(F("Start!"));

  set_state(STATE_CONN_WAIT);

  dbg_println(F("Init LCD"));
  lcd.init();
  lcd.backlight();

  // create the custom LCD characters from byte strings in PROGMEM
  byte bb[8]; // byte buffer for character data
  for (byte i = 0; i < sizeof(lcd_chars) / sizeof(lcd_chars[0]); i++)
  {
    for (byte j = 0; j < sizeof(lcd_chars[0]); j++)
    {
      bb[j] = pgm_read_byte(&lcd_chars[i][j]);
    }
    lcd.createChar(i, bb);
  }

  resetAnimation();
  
  lcd_print(F("Nottinghack coin    "), F("acceptor v1.00....  "));

  dbg_println(F("Init SPI"));
  SPI.begin();

  pinMode(PIN_COIN_PULSE, INPUT_PULLUP); 
  pinMode(PIN_COIN_ENABLE, OUTPUT); 
  pinMode(PIN_CANCEL_BUTTON, INPUT_PULLUP);
  pinMode(PIN_CANCEL_LIGHT, OUTPUT);

  // trigger the interrupt on rising edge
  attachInterrupt(digitalPinToInterrupt(PIN_COIN_PULSE), onCoinPulse, RISING);
    
  strcpy(status_msg, RUNNING);
  
  dbg_println(F("Start Ethernet"));
  Ethernet.begin(mac, ip);

  dbg_println(F("Init RFID"));
  rfid_reader.PCD_Init();

  // Start MQTT and say we are alive
  dbg_println(F("Check MQTT"));
  checkMQTT();

  delay(100);

  dbg_println(F("Setup done..."));
} // end void setup()

uint8_t set_state(state_t new_state)
{
  state_entered = millis();

  switch(new_state)
  {
    case STATE_CONN_WAIT:
      dbg_println(F("Setting state: CONN_WAIT"));
      if (state >= STATE_READY)
      {
        // State has gone backwards to CONN_WAIT... we had a connection, but lost it.
        lcd_print(F("Connection lost!"));

        // Abort anything in progress
        if (state == STATE_ACCEPT_NET_WAIT)
        {
          dbg_println(F("Rejecting note."));
          //nv4_reject_escrow();
        }
        coinAcceptorDisable();
        memset(&card_number, 0, sizeof(card_number));
        memset(rfid_serial, 0, sizeof(rfid_serial));
        memset(tran_id, 0, sizeof(tran_id));
      }
      state = STATE_CONN_WAIT;

      break;

    case STATE_READY:
      dbg_println(F("Setting state: READY"));

      // To go from STATE_ACCEPT_NET_WAIT->STATE_READY means something's gone wrong. Reject the note.
      if (state == STATE_ACCEPT_NET_WAIT)
      {
        dbg_println(F("Rejecting note."));
        //nv4_reject_escrow();
      }

      coinAcceptorDisable();
      memset(&card_number, 0, sizeof(card_number));
      memset(rfid_serial, 0, sizeof(rfid_serial));
      memset(tran_id, 0, sizeof(tran_id));

      lcd_print(F("Please scan RFID    "), F("card.... "));

      state = STATE_READY;
      break;

    case STATE_AUTH_WAIT:
      dbg_println(F("Setting state: AUTH_WAIT"));
      lcd_print(F("Card lookup..."));
      state = STATE_AUTH_WAIT;
      break;

    case STATE_ACCEPT:
      coinAcceptorEnable();
      dbg_println(F("Setting state: STATE_ACCEPT"));
      lcd_print(F("Please insert coins"));
      state = STATE_ACCEPT;
      break;

    case STATE_ACCEPT_COIN_COUNT:
      dbg_println(F("Setting state: STATE_ACCEPT_COIN_COUNT"));
      state = STATE_ACCEPT_COIN_COUNT;
      break;

    case STATE_ACCEPT_NET_WAIT:
      dbg_println(F("Setting state: STATE_ACCEPT_NET_WAIT"));
      state = STATE_ACCEPT_NET_WAIT;
      coinAcceptorDisable();
      resetAnimation();
      break;

    case STATE_ACCEPT_CONFIRM_WAIT:
      dbg_println(F("Setting state: STATE_ACCEPT_CONFIRM_WAIT"));
      state = STATE_ACCEPT_CONFIRM_WAIT;
      break;

    default:
      break;
  }

  // Illuminate the cancel button light only if in accept state
  digitalWrite(PIN_CANCEL_LIGHT, (state == STATE_ACCEPT ? HIGH : LOW));

  return 0;
}

void loop()
{
  // Poll MQTT
  // should cause callback if there's a new message
  client.loop();

  // are we still connected to MQTT
  checkMQTT();

  // Check for RFID card
  poll_rfid();

  // Check for timeouts
  timeout_check();

  coin_check();

  // Check for cancel button being pushed
  if ((state == STATE_ACCEPT) && (digitalRead(PIN_CANCEL_BUTTON) == LOW))
  {
    dbg_println(F("User cancel!"));
    sprintf(pmsg, "VCAN:%s:%s", rfid_serial, tran_id);
    client.publish(P_COIN_TX, pmsg);
    set_state(STATE_READY);
  }

} // end void loop()

void coin_check()
{
  char buf[LCD_WIDTH + 1];

  // if coins have been inserted since the last check
  if (previousCreditInPence != creditInPence)
  {
    if (state == STATE_ACCEPT)
    {
      set_state(STATE_ACCEPT_COIN_COUNT);
      
      // show that we're counting coins
      lcd_print(F("Counting Coins..."));
    }

    if (millis() - lastCreditChangeTime > COIN_PULSE_TIMEOUT_MS)
    {
      // we have finished cointing the current coin(s)
      if (state == STATE_ACCEPT_COIN_COUNT)
      {
        sprintf(pmsg, "VREQ:%s:%s:%d", rfid_serial, tran_id, creditInPence);
        client.publish(P_COIN_TX, pmsg);
        set_state(STATE_ACCEPT_NET_WAIT);

        lcd_print(F("Recording payment:"));

        formatCurrency(buf, creditInPence); 
        lcd.setCursor(0, 1);
        lcd.print(buf);

        // display the recorded payment to the user
        delay(1500);
      }
      else if (state == STATE_ACCEPT_CONFIRM_WAIT)
      {
        // Note accepted
        sprintf(pmsg, "VSUC:%s:%s:CHANNEL%d", rfid_serial, tran_id, 0);
        client.publish(P_COIN_TX, pmsg);
        
        // reset counts to zero now transaction is complete
        previousCreditInPence = 0;
        creditInPence = 0;
        
        set_state(STATE_READY);
        doAuth();
      }
      else
      {
        // Unexpected coins - acceptor shouldn't have been active!
        dbg_println(F("Unexpected coins!"));
        coinAcceptorDisable();
      }
    }
    else
    {
      // still in the process of counting coins as we haven't hit the pulse timeout
      displayCountingAnimation();
    }
  }
}

void formatCurrency(char buf[], int pence)
{
    int pounds = pence / 100;
    int remaining_pence = pence % 100;
    if (remaining_pence < 10)
    {
      snprintf(buf, 8, "%c%d.0%d", POUND_SYMBOL, pounds, remaining_pence);
    }
    else
    {
      snprintf(buf, 8, "%c%d.%d", POUND_SYMBOL, pounds, remaining_pence);
    }
}

void displayCountingAnimation()
{
  // print an animation on LCD
  lcd.setCursor(0, 1);
  byte firstChar = animation[0];
  for (int i = 0; i < LCD_WIDTH - 1; i++) {
    animation[i] = animation[i+1];
    lcd.write(animation[i]);
  }
  animation[LCD_WIDTH - 1] = firstChar;
  lcd.write(animation[LCD_WIDTH - 1]);
  delay(200);
}

void resetAnimation()
{
  animation[0] = ANIM_FRAME_1;
  animation[1] = ANIM_FRAME_1;
  animation[2] = ANIM_FRAME_1;
  animation[3] = ANIM_FRAME_1;
  animation[4] = ANIM_FRAME_1;
  animation[5] = ANIM_FRAME_1;
  animation[6] = ANIM_FRAME_1;
  animation[7] = ANIM_FRAME_1;
  animation[8] = ANIM_FRAME_1;
  animation[9] = ANIM_FRAME_1;
  animation[10] = ANIM_FRAME_1;
  animation[11] = ANIM_FRAME_1;
  animation[12] = ANIM_FRAME_2;
  animation[13] = ANIM_FRAME_3;
  animation[14] = ANIM_FRAME_4;
  animation[15] = ANIM_FRAME_5;
  animation[16] = ANIM_FRAME_5;
  animation[17] = ANIM_FRAME_4;
  animation[18] = ANIM_FRAME_3;
  animation[19] = ANIM_FRAME_2;
}

void poll_rfid()
{
  if (state != STATE_READY)
  {
    return;
  }

  if (!rfid_reader.PICC_IsNewCardPresent())
    return;

  if (!rfid_reader.PICC_ReadCardSerial())
    return;

  doAuth();
}

void doAuth()
{
  cpy_rfid_uid(&card_number, &rfid_reader.uid);
  uid_to_hex(rfid_serial, card_number);

  sprintf(pmsg, "AUTH:%s", rfid_serial);
  client.publish(P_COIN_TX, pmsg);
  set_state(STATE_AUTH_WAIT);
}

void onCoinPulse()
{
  // debounce the pulse for 1ms - the relay used to turn on the coin acceptor will cause the COIN output to bounce a little at startup
  delay(1);
  
  if (digitalRead(PIN_COIN_PULSE) == HIGH)
  {
    if (state == STATE_ACCEPT || state == STATE_ACCEPT_COIN_COUNT)
    {
      creditInPence += COIN_PULSE_VALUE;
      lastCreditChangeTime = millis();
    }
    else
    {
      // this should never happen as the coin acceptor should be powered off unless we're in STATE_ACCEPT or STATE_ACCEPT_COIN_WAIT
      dbg_println(F("Error - unexpected coin pulse"));
    }
  }
}

void coinAcceptorEnable()
{
  dbg_println(F("Enabling Coin Acceptor"));
  digitalWrite(PIN_COIN_ENABLE, HIGH);
}

void coinAcceptorDisable()
{
  dbg_println(F("Disabling Coin Acceptor"));
  digitalWrite(PIN_COIN_ENABLE, LOW);
}

void timeout_check()
{
  if
  (
    ((state == STATE_AUTH_WAIT      ) && abs((millis() - state_entered) > TIMEOUT_COMS)) ||
    ((state == STATE_ACCEPT_NET_WAIT) && abs((millis() - state_entered) > TIMEOUT_COMS))
  )
  {
    lcd_print(F("Error - timeout..."));
    dbg_println(F("Error - timeout..."));
    delay(1500);
    set_state(STATE_READY);
    return;
  }

  if ((state == STATE_ACCEPT) && abs((millis() - state_entered) > TIMEOUT_SES ))
  {
    // disable the coin acceptor immediatly otherwise coins migh be inserted whilst we dwell on the LCD message for the user
    coinAcceptorDisable();
      
    // Session timeout
    lcd_print(F("Session Timeout!"));
    dbg_println("Session timeout");

    sprintf(pmsg, "VCAN:%s:%s", rfid_serial, tran_id);
    client.publish(P_COIN_TX, pmsg);
    delay(1500);
    set_state(STATE_READY);
  }
}

void lcd_print(const __FlashStringHelper *line1)
{
  lcd_print(line1, F(""));
}

void lcd_print(const __FlashStringHelper *line1, const __FlashStringHelper *line2)
{
  lcd.clear();
  lcd.home();
  lcd.print(line1);
  lcd.setCursor(0, 1);
  lcd.print(line2);
}

void dbg_println(const __FlashStringHelper *n)
{
  __prepareDebug();
  strcat_P(pmsg, (char*)n);
  __debug();
}

void dbg_println(const char *msg)
{
  __prepareDebug();
  strcat(pmsg, msg);
  __debug();
}

void __prepareDebug()
{
  memset(pmsg, 0, sizeof(pmsg));
  strcpy(pmsg, "INFO:");
}

void __debug()
{
  #ifdef DEBUG_MQTT
  if (state > STATE_CONN_WAIT)
  {
    client.publish(P_COIN_TX, pmsg);
  }
  #endif

  #ifdef DEBUG_SERIAL
  Serial.println(pmsg + sizeof("INFO"));
  #endif 
}

void cpy_rfid_uid(rfid_uid *dst, rfid_uid *src)
{
  memcpy(dst, src, sizeof(rfid_uid));
}

bool eq_rfid_uid(rfid_uid u1, rfid_uid u2)
{
  if (memcmp(&u1, &u2, sizeof(rfid_uid)))
    return false;
  else
    return true;
}

void uid_to_hex(char *uidstr, rfid_uid uid)
{
  switch (uid.size)
  {
    case 4:
      sprintf(uidstr, "%02x%02x%02x%02x", uid.uidByte[0], uid.uidByte[1], uid.uidByte[2], uid.uidByte[3]);
      break;

    case 7:
      sprintf(uidstr, "%02x%02x%02x%02x%02x%02x%02x", uid.uidByte[0], uid.uidByte[1], uid.uidByte[2], uid.uidByte[3], uid.uidByte[4], uid.uidByte[5], uid.uidByte[6]);
      break;

    case 10:
      sprintf(uidstr, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x", uid.uidByte[0], uid.uidByte[1], uid.uidByte[2], uid.uidByte[3], uid.uidByte[4], uid.uidByte[5], uid.uidByte[6], uid.uidByte[7], uid.uidByte[8], uid.uidByte[9]);
      break;

    default:
      sprintf(uidstr, "ERR:%d", uid.size);
      break;
  }
}
