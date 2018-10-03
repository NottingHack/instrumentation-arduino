
/**************************************************** 
 * sketch = Vend2
 *
 * Nottingham Hackspace
 * CC-BY-SA
 *
 * Source = https://wiki.nottinghack.org.uk/wiki/Vending_Machine/Cashless_Device_Implementation
 * Target controller = SAM D21 / Arduino Zero
 * Development platform = Arduino IDE 1.8.5
 *
 *
 * This code is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * RFID payment system for Vending Machines at
 * Nottingham Hackspace and is part of the
 * Hackspace Instrumentation Project
 *
 *
 ****************************************************/

/*
 History
  000 - Started 01/08/2011
  001 - Initial release
  002 - Add LCD
  003 - ?
  004 - Many changes:
       - Use SAM D21 (Arduino Zero) instead of atmega328 / Arduino Uno
       - MFRC522 RFID instead of RDM880
       - Wiznet network instead of ENC28J60
       - MQTT instead of UDP
       - Code split up into different modules
 
 Known issues:
  
 Future changes:
  
 ToDo:
   1) Network timeout
  
  
  
 Authors:
 'RepRap' Matt    dps.lwk at gmail.com
 James Hayward    jhayward1980 at gmail.com
 Daniel Swann     hs at dswann.co.uk
 
 For more details, see:
 http://wiki.nottinghack.org.uk/wiki/Vending_Machine/Cashless_Device_Implementation
 */

#define VERSION_NUM 0x04
#define VERSION_STRING "Vend ver: 004"

#define SESSION_TIMEOUT 45000 // 45secs.




#include "Config.h"
#include "net_transport.h"
#include "net_messaging.h"
#include "mdb_serial.h"
#include "rfid.h"
#include "debug.h"
#include "serial_menu.h"

#include <FlashAsEEPROM.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h> 
#include <avr/pgmspace.h>



char rfid_serial[21];
bool update_lcd = false;

cState card_state;
char tran_id[10]; // transaction id

// VMC setup values
byte vmcFeatureLevel;	// 1
byte displayColoumns;	// 0
byte displayRows;		// 0
byte displayInfo;		// 0
int maxPrice;
int minPrice;

byte state;
// flags. most of these are used to change the response to the poll command
byte allowACK; // normally we respond with an ACK to a poll, some commands must not until they are ready!
byte cancelled; // vmc has cancelled the operation
byte commandOOS; // 1 == command was out of sequence
byte justReset;	// 1==just had reset but not been polled
byte allowVend; // 0 if not vending, 1 for allow, 2 for disallow
char vendAmountA; // amount (scaled) that is being requested.
char vendAmountB; // amount (scaled) that is being requested.
byte cancel_vend; // Cancel button pressed


void do_ping();
byte checkVend(struct MDB_Byte* data) ;
void MDB_vend();
void MDB_reader();
void MDB_poll();
void MDB_setup();
void MDB_reset();
void MDB_expan();
void MDB_Write(int data);
byte transmitData(struct MDB_Byte* data, byte length) ;
byte generateChecksum(struct MDB_Byte* data, byte length) ;
byte validateChecksum(struct MDB_Byte* data, byte length) ;
void cancel_pressed();



struct MDB_Byte recvData[40];
struct MDB_Byte sendData[40];
byte sendLength;
int rst;
byte ret_button;
uint8_t _debug_level = 0;

byte rf_count;
LiquidCrystal_I2C lcd(LCD_I2C_ADDRESS,LCD_WIDTH,2);

unsigned long last_net_msg;
unsigned long last_ping;

const char lcd_msg1[] PROGMEM = "Ready...";

byte validateChecksum(struct MDB_Byte* data, byte length) 
{
  byte sum = 0; 
  
  for (int i=0; i < (length-1); i++) 
    sum += data[i].data;

  if (data[length-1].data == sum) 
    return 1;
  else 
    return 0;
} // end byte validateChecksum

byte generateChecksum(struct MDB_Byte* data, byte length) 
{
  byte sum = 0; 
  for (int i=0; i < (length); i++) 
    sum += data[i].data;

  return sum;
} // end byte generateChecksum

byte transmitData(struct MDB_Byte* data, byte length) 
{
  int transmitState;
  struct MDB_Byte mdbb;
  int tx;
  
  if (_debug_level) 
    dbg_println("(");
  
  do 
  {
    mdbb.data = generateChecksum(data, length);
    mdbb.mode = 0x1;

    tx = 0;
    memcpy(&tx, &mdbb, 2);

    for (int i=0; i < length; i++) 
      MDB_Write(data[i].data);

    MDB_Write(tx); //chk

    while ( !mdb_serial_data_available() );
    
    mdb_serial_get_byte(&mdbb);
    
    if (mdbb.data == rNAK || mdbb.data == RET) 
    {
      transmitState = rRETRANSMIT;
    } else if (mdbb.data == rACK) 
    {
      transmitState = rCONTINUE;
    } else 
    {    
      transmitState = rUNKNOWN;
    }
  } while (transmitState == rRETRANSMIT);
  if (_debug_level) dbg_println(")");

  return transmitState;
} // end void transmitData




void MDB_Write(int data)
{
  char tmpstr[7];
  struct MDB_Byte b;

  memcpy(&b, &data, 2);

  mdb_serial_write(b);
}


void setup()
{
//  wdt_reset ();
  dbg_init();
  delay(4000);
  last_net_msg = millis(); // prevent instant timeout
  last_ping = millis();

  serial_menu_init(); // Also reads in network config from EEPROM
  net_transport_init();

  lcd.init();
  lcd.backlight();
  lcd.print(F("Initialising..."));

  // Clear tx/rx buffers
  memset(recvData, 0, sizeof(recvData));
  memset(sendData, 0, sizeof(sendData));

  // Clear card number buffer
  memset(rfid_serial, 0, sizeof(rfid_serial));

  // Clear transaction id
  memset(tran_id, 0, sizeof(tran_id));

  card_state = NO_CARD;
  cancel_vend = 0;

  // Init. hardware serial for 9600/9bit
  mdb_serial_init();

  // Setup Pins
  pinMode(RETURN_BUTTON, INPUT);
  pinMode(LED_ACTIVE, OUTPUT);
  pinMode(LED_CARD, OUTPUT);
 // pinMode(LED_WAIT, OUTPUT);

  // Set default output's
  digitalWrite(LED_ACTIVE, LOW);
  digitalWrite(LED_CARD, LOW);
  //digitalWrite(LED_WAIT, LOW);

  state = sINACTIVE;
  // flags defaults
  allowACK = 1;
  justReset = true;
  commandOOS = false;
  allowVend = 0;
  vendAmountA = 0;
  vendAmountB = 0;
  ret_button = false;

  rfid_init();

  // let everything else settle
  delay(100);

  net_tx_debug(rfid_serial, sizeof(rfid_serial)); // reqeust debug level

  dbg_println(F("Init done"));
  lcd.clear();
  lcd.print(F("Wait..."));
  

  serial_show_main_menu();
} // end void setup()


void MDB_reset()
{
  if (rst)
  {
    rst = 0;
   // digitalWrite(LED_WAIT, LOW);
  } else
  {
    rst = 1;
  //  digitalWrite(LED_WAIT, HIGH);
  }

  // Reset has no associated data, so just read checksum byte
  mdb_serial_get_byte(&recvData[1]);
  if (validateChecksum(recvData, 2) == 1) 
  {
    MDB_Write(sACK);
    // ***LWK*** now need to change to just reset state (substate of sINACTIVE) need a flag for use in POLL
  // flags defaults
    allowACK = 1;
    justReset = true;
    commandOOS = false;
    allowVend = 0;
    vendAmountA = 0;
    vendAmountB = 0;
    cancel_vend = 0;

    // Clear any rfid card / transaction details
    memset(rfid_serial, 0, sizeof(rfid_serial));
    memset(tran_id, 0, sizeof(tran_id));
    card_state = NO_CARD;

    digitalWrite(LED_ACTIVE, LOW);
    if (_debug_level) dbg_println(F("RST: ACK sent"));

  } else
  {
    MDB_Write(sNAK);
    if (_debug_level) dbg_println(F("RST: NAK sent"));
  }
}

void MDB_setup()
{
  mdb_serial_get_byte(&recvData[1]);
  if (recvData[1].data == CONFIG_DATA) 
  {
    dbg_println(F("MDB_setup: config data"));

    for (int i = 2; i < 7; i++) 
      mdb_serial_get_byte(&recvData[i]);

    if (1) //validateChecksum(recvData, 7)) 
    {
      // Store the VMC data
      // need to do stuff here! ***LWK*** done
      vmcFeatureLevel = recvData[2].data;  // 1
      displayColoumns = recvData[3].data;  // 0
      displayRows = recvData[4].data;    // 0
      displayInfo = recvData[5].data;    // 0
              
      // build the data
      sendLength = 8;
              
      sendData[0].data = READER_CONFIG_DATA;
      sendData[1].data = FEATURE_LEVEL;
      sendData[2].data = COUNTRY_CODE_HIGH;
      sendData[3].data = COUNTRY_CODE_LOW;
      sendData[4].data = SCALE_FACTOR;
      sendData[5].data = DECIMAL_PLACES;
      sendData[6].data = MAX_RESPONSE_TIME;
      sendData[7].data = MISC_OPTIONS;
              
      if (transmitData(sendData, sendLength) == rUNKNOWN) 
      {
              dbg_println(F("MDB_setup: tx fail"));
                // god knows what we do here!
      } else 
      {
        // we are rCONTINUE
        // we still need min/max prices to change state
        if (_debug_level) dbg_println(F("Setup recv, config sent"));
      }
    } else
    {
      // bad checksum
      dbg_println(F("MDB_setup: bad chk"));
      MDB_Write(sNAK);
    }
  } else if (recvData[1].data == MINMAX_PRICES)
  {
    if (_debug_level)
      dbg_println(F("MDB_setup: MINMAX_PRICES"));

    // this has four additional data bytes (plus CHK of course)
    for (int i = 2; i < 7; i++) 
      mdb_serial_get_byte(&recvData[i]);

    if (validateChecksum(recvData, 7)) 
    {
      // Store the VMC data
      // max and min prices stored in two bytes each, max first
      maxPrice = recvData[2].data << 8;
      maxPrice += recvData[3].data;
      minPrice = recvData[4].data << 8;
      minPrice += recvData[5].data;

      if (_debug_level) dbg_println(F("MDB_setup: got minmax"));

      // respond with just an ACK
      MDB_Write(sACK);
      state = sDISABLED;
      digitalWrite(LED_ACTIVE, LOW);

      if (_debug_level) dbg_println(F("State Change: Disabled!"));

    }
    else
    {
      // invalid checksum
      MDB_Write(sNAK);
    }
  }
  else
  {
    if (_debug_level) dbg_println(F("MDB_setup: unexpected setup"));
  }
}


void MDB_expan()
{
  mdb_serial_get_byte(&recvData[1]);
  if (recvData[1].data == RESQUEST_ID) 
  {
    if (_debug_level) dbg_println(F("MDB_expan: id"));

    for (int i = 2; i < 32; i++) 
      mdb_serial_get_byte(&recvData[i]);

    if (validateChecksum(recvData, 32)) 
    {
      // VMC is telling us about it's serial number, f/w version, etc. Don't do anything with received data as we don't care
              
      // build the data
      sendLength = 30;

      sendData[0].data = PERIPHERAL_ID;
      memcpy(&sendData[1], MANUFACTURER_CODE, 3);
      memcpy(&sendData[4], SERIAL_NUMBER, 12);
      memcpy(&sendData[16], MODEL_NUMBER, 12);
      sendData[28].data = 0x00;
      sendData[29].data = VERSION_NUM;
      
      if (transmitData(sendData, sendLength) == rUNKNOWN) 
      {
        if (_debug_level) dbg_println(F("MDB_expan: tx fail"));
      } else 
      {
        // we are rCONTINUE
        if (_debug_level) dbg_println(F("expan, id sent"));
      }
    } else 
    {
      // bad checksum
      if (_debug_level) dbg_println(F("MDB_expan: bad chk"));
      MDB_Write(sNAK);
    }
  } 
  else
  {
    if (_debug_level) dbg_println(F("MDB_expan: unexpected subcmd"));
  }
}

void MDB_poll()
{
  // lots of POLL sub commands might hand off to a sub routine to keep code cleaner
  // has no associated data, so just read checksum byte
  mdb_serial_get_byte(&recvData[1]);

  if (validateChecksum(recvData, 2) == 0) 
  {
    // bad checksum
    MDB_Write(sNAK);
    if (_debug_level) dbg_println(F(">>NAK - bad csum"));
    return;
  }

  if (cancel_vend)
  {
   sendData[0].data = SESSION_CANCEL_REQUEST;
   transmitData(sendData, 1); 
   cancel_vend = 0;
   return;
  }

  if (commandOOS) 
  {
    commandOOS = false;
    MDB_Write(CMD_OUT_OF_SEQUENCE);
    MDB_Write(CMD_OUT_OF_SEQUENCE_CHK);

    if (_debug_level) dbg_println(F("Sent cms OOS"));

    return;
  }

  if (justReset)
  {
    sendData[0].data = JUST_RESET;
    transmitData(sendData, 1);
    justReset = false;  // told vmc

    if (_debug_level) dbg_println(F("Sent JUST_RESET"));
    return;
   }

   if (allowVend == 1) 
   {
     // vend allowed!
     sendLength = 3;

     sendData[0].data = VEND_APPROVED;
     sendData[1].data = vendAmountA; // bottom bits
     sendData[2].data = vendAmountB; // top bits;

     if (transmitData(sendData, sendLength) == rUNKNOWN)
     {
       if (_debug_level) dbg_println(F("huh?"));
     } else 
     {
       // vend should work!
       //allowVend = 0;
     }
     
     if (_debug_level) dbg_println(F("allowed vend"));
     return;
   } else if (allowVend == 2) 
   {
     // vend disallowed
     MDB_Write(VEND_DENIED);
     MDB_Write(VEND_DENIED_CHK);
     allowVend = 0;
     return;
   }

   if (card_state == GOOD) 
   {
     // build the response
     sendLength = 3;
     memset(sendData, 0, sizeof(sendData));
     sendData[0].data = BEGIN_SESSION;
     sendData[1].data = 0xFF;
     sendData[2].data = 0xFF;

     if (transmitData(sendData, sendLength) == rUNKNOWN)
     {

     } else 
     {
       state = sSESSION_IDLE;

       dbg_println(F("State change: Session Idle!"));
     }

     card_state = IN_USE;
     return;
   }

   // more POLL subcommands

   // Default bahaviour
   if (allowACK == 1) 
   {
     MDB_Write(sACK);
   }

   // otherwise we are silent.
 }

void MDB_reader()
{ 
  mdb_serial_get_byte(&recvData[1]);
  
  if (recvData[1].data == READER_ENABLE)
  {
    mdb_serial_get_byte(&recvData[2]);
    if (validateChecksum(recvData, 3))
    {
      if (state == sDISABLED)
      {
        state = sENABLED;
        MDB_Write(sACK);
        digitalWrite(LED_ACTIVE, HIGH);
        memset(rfid_serial, 0, sizeof(rfid_serial));

        dbg_println(F("State Change: Enabled!"));

        lcd.clear();
        lcd.print(F("Ready..."));
      } else 
      {
        // we are not in the correct state
        // ACK this and then send cmd out of sequence
        commandOOS = true;
        MDB_Write(sACK);
      }
    } else 
    {
      // invalid checksum
      MDB_Write(sNAK);
    }
  }

  if (recvData[1].data == READER_DISABLE)
  {
    mdb_serial_get_byte(&recvData[2]);
    if (validateChecksum(recvData, 3))
    {
      if (state == sENABLED)
      {
        state = sDISABLED;
        MDB_Write(sACK);
        digitalWrite(LED_ACTIVE, LOW);
        memset(rfid_serial, 0, sizeof(rfid_serial));

        dbg_println(F("State Change: Disabled! (VMC cmd)"));
        lcd.clear();

      } else 
      {
        // we are not in the correct state
        // ACK this and then send cmd out of sequence
        commandOOS = true;
        MDB_Write(sACK);
      }
    } else 
    {
      // invalid checksum
      MDB_Write(sNAK);
    }
  }
}

void MDB_vend()
{
  mdb_serial_get_byte(&recvData[1]);
  char buf[32];
  int amount;
  int amount_scaled;
  
  
  switch (recvData[1].data)
  {
    case VEND_REQUEST:
      // four more bytes of data, plus checksum
      for (int i = 2; i < 6; i++) 
        mdb_serial_get_byte(&recvData[i]);
      

      amount = recvData[2].data << 8;
      amount += recvData[3].data;
      amount_scaled = (amount * SCALE_FACTOR);

      sprintf(buf, "Vendreq %d (%dp)", amount, amount_scaled); 
     
      if (_debug_level) dbg_print(buf);
      
      if (/*validateChecksum(recvData, 6)*/ 1) 
      {
      // find price and item number and hand off to network
      // for now we just ACK and let the POLL respond
        MDB_Write(sACK);
        allowVend = 0;
        net_tx_vreq(amount_scaled, rfid_serial, tran_id);   // Send VREQ message for the amount
      } else 
      {
        MDB_Write(sNAK);
      }
      break;
      
    case VEND_SUCCESS:
      // two more bytes of data
      for (int i = 2; i < 4; i++)
        mdb_serial_get_byte(&recvData[i]);
      
      sprintf(buf, "%.2x-%.2x", recvData[2].data, recvData[3].data); 
      MDB_Write(sACK);
      net_tx_vsuc(buf, rfid_serial, tran_id);
      lcd.clear();
      allowVend = 0;
      break;
      
   case VEND_FAILURE:
      MDB_Write(sACK);
      net_tx_vfail(rfid_serial, tran_id);
      allowVend = 0;
      dbg_println(F("Vend failure"));
      break;
      
    case SESSION_COMPLETE:
      dbg_println(F("Session complete"));
      // build the response
      sendLength = 1;
      memset(sendData, 0, sizeof(sendData));
      sendData[0].data = END_SESSION;
      transmitData(sendData, sendLength);
      memset(rfid_serial, 0, sizeof(rfid_serial));
      memset(tran_id, 0, sizeof(tran_id));
      card_state = NO_CARD;
      state = sENABLED;
      lcd.clear();
      lcd.print(F("Ready..."));
      digitalWrite(LED_ACTIVE, HIGH);
      break;
      
    case CASH_SALE:
      if (_debug_level) dbg_println(F("Cash sale"));
       // four more bytes of data, plus checksum
      for (int i = 2; i < 6; i++) 
        mdb_serial_get_byte(&recvData[i]);

      amount = recvData[2].data << 8;
      amount += recvData[3].data;
      amount_scaled = (amount * SCALE_FACTOR);   
      
      // Get location
      sprintf(buf, "%.2x-%.2x", recvData[4].data, recvData[5].data); 

      MDB_Write(sACK);
      net_tx_cash(amount_scaled, buf);   // Send CASH message
      break;
    
      
    default:
      if (_debug_level) dbg_println(F("MDB_vend> err"));
  } 
}

void loop() 
{

  byte readcard = 0;
  bool flush_buffer = false;
  unsigned long time_card_read=0;
  bool net_timeout = false;
  bool just_polled = false;
  
 
  while (1)
  {
    net_transport_loop();
    if ((millis() - last_net_msg) < NET_RESET_TIMEOUT)
    {
      net_timeout = false;
     // wdt_reset();
    }
    else
    {
      if (!net_timeout)
      {
        net_timeout = true;
        if (_debug_level) dbg_println(F("Net timeout"));
      }
    }
    
    if (mdb_serial_data_available())
    {
      mdb_serial_get_byte(&recvData[0]);

      if ((recvData[0].mode) && ((recvData[0].data & ADDRESS_MASK) == CASHLESS_ADDRESS))
      {
        switch (recvData[0].data) 
        {
          case RESET:
            if (_debug_level) dbg_println(F("Reset"));
            MDB_reset();
            break;
  
          case SETUP:
            if (_debug_level) dbg_println(F("Setup"));
            MDB_setup();
            break;
            
          case POLL:
            if (_debug_level) dbg_println(F("Poll"));
            MDB_poll();
            just_polled = true;
            break;
            
          case READER:
            if (_debug_level) dbg_println(F("Reader"));
            MDB_reader();
            break;
           
          case VEND:
            if (_debug_level) dbg_println(F("Vend"));
            MDB_vend();          
            break;
                      
          case EXPANSION:
            if (_debug_level) dbg_println(F("Expan"));
            MDB_expan();
            break;

          default:
            // should never get here once done programing all features
            if (_debug_level) dbg_println (F("Unknown Cmd"));
            break;
        } 
      }// else // end if 
      
    
    } // end if (MDB.available())


  if (just_polled)
  {
    if (_debug_level) dbg_println(F("just polled"));
    
    just_polled = false;
    // Only want to try and read a card after the VMC has polled device 0x50  (it's the last in it's polling cycle)
//    if ((recvData[0].mode) && (recvData[0].data == 0x50)) // TODO: is this right?
    {
      readcard = 1;
    }

    // Poll RFID
    if ((state == sENABLED) && (card_state == NO_CARD) && (readcard))// && (!(recvData[0].mode)))
    {
      readcard=0;
      time_card_read=0;
      
      // If this is the first time polling the reader since we successfully read a card, flush its buffer
      if (flush_buffer)
      {
        memset(rfid_serial, 0, sizeof(rfid_serial));
        flush_buffer = false;
        time_card_read = 0;
      }
      
      // we can now check for a card
      if (rfid_poll(rfid_serial))
      {
        // card has been read
        card_state = NET_WAIT;
        net_tx_auth(rfid_serial, sizeof(rfid_serial)); // try to authenticate card
        flush_buffer = true;
        time_card_read = millis();
      }
    }
  }    
    
    if (time_card_read && ((millis() - time_card_read) > SESSION_TIMEOUT))
    {
      dbg_println(F("Session timeout"));
      cancel_pressed();
      time_card_read = 0; 
    }
    net_transport_loop();

    // Do we have an RFID serial in memory?
    if (rfid_serial[0] == 0)
      digitalWrite(LED_CARD, HIGH); // inverse logic for inbuilt nanode led
    else
      digitalWrite(LED_CARD, LOW);

   // Return / Cancel button pushed?
   if (digitalRead(RETURN_BUTTON) == HIGH)
   {
     if (!ret_button)
     {  
       ret_button = true;
       cancel_pressed();
     }

   } else
   {
     ret_button = false;
   }

   do_ping();
   serial_menu();
  } // while(1)
} // end void loop()

byte checkVend(struct MDB_Byte* data) 
{  
  vendAmountA = data[2].data;
  vendAmountB = data[3].data;  
  return 1; 
}

// cancel button pushed, cancel vend.
void cancel_pressed()
{
  // If vend has already be approved, then it's too late.
  if (allowVend == 1)
  {
    if (_debug_level) dbg_println(F("OOS cancel"));
    return;    
  }
  
  if ((rfid_serial[0] != 0) || (tran_id[0] != 0))
  {
    lcd.clear();
    net_tx_vcan(rfid_serial, tran_id);
    cancel_vend = 1;
    memset(rfid_serial, 0, sizeof(rfid_serial)); 
    memset(tran_id, 0, sizeof(tran_id));       
    card_state = NO_CARD;
  } else
  {
    if (_debug_level) dbg_println(F("Nothing to cancel"));
  }
}

void do_ping()
{
  if (((millis() - last_net_msg) > PING_INTERVAL) &&
      ((millis() - last_ping)    > PING_INTERVAL))
  {
    last_ping = millis();
    net_tx_ping();
  }
}

