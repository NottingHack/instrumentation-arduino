
/**************************************************** 
 * sketch = Vend
 *
 * Nottingham Hackspace
 * CC-BY-SA
 *
 * Source = http://wiki.nottinghack.org.uk/wiki/...
 * Target controller = Arduino 328 (Nanode v5)
 * Clock speed = 16 MHz
 * Development platform = Arduino IDE 0022
 * C compiler = WinAVR from Arduino IDE 0022
 * 
 * 
 * This code is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *
 * RFID payment system for Vending Machine at 
 * Nottingham Hackspace and is part of the 
 * Hackspace Instrumentation Project
 *
 * 
 ****************************************************/

/*  
 History
  000 - Started 01/08/2011
  001 - Initial release

 
 Known issues:
  
 Future changes:
  
 ToDo:
   1) Network timeout
  
  
  
 Authors:
 'RepRap' Matt    dps.lwk at gmail.com
 James Hayward    jhayward1980 at gmail.com
 Daniel Swann     hs at dswann.co.uk
 
 */

#define VERSION_NUM 001
#define VERSION_STRING "Vend ver: 001"



// Port 52240
#define DEST_PORT_L  0x10
#define DEST_PORT_H  0xCC

// Port 52240
#define PORT_L  0x10
#define PORT_H  0xCC

#define BAUD 9600

// ARP retry interval - 10s
#define ARP_RETRY 10000

// Uncomment for debug prints
#define DEBUG_PRINT
// extra prints likely to upset timing
///#define DEBUG_PRINT_1

// shows human readable msgs on serial line, shouldn't mess with timing
#define HUMAN_PRINT

#include <NewSoftSerial.h>
#include <EtherShield.h>
#include "Config.h"
#include <NanodeUNIO.h>
#include <util/setbaud.h>
#include <EEPROM.h>



// The ethernet shield
EtherShield es=EtherShield();


static uint8_t mymac[6]; // Read from chip on nanode.
static uint8_t myip[4] = {10,0,0,62};
static uint8_t dstip[4] = {10,0,0,2};
 uint8_t dstmac[6]; 

const char iphdr[] PROGMEM ={0x45,0,0,0x82,0,0,0x40,0,0x20}; // 0x82 is the total
char udpPayload [48];
char rfid_serial[20];

uint8_t  srcport = 11023;

// Packet buffer, must be big enough for packet and payload
#define BUFFER_SIZE 150
static uint8_t buf[BUFFER_SIZE+1];

struct MDB_Byte
{
  byte data;
  byte mode;
};

struct net_msg_t
{
  byte msgtype[4];
  byte payload[40];
} net_msg;

struct MDB_Byte recvData[10];
struct MDB_Byte sendData[10];
NewSoftSerial rfid_reader(RFID_RX,RFID_TX);
byte sendLength;
int rst;
int debug;
byte ret_button;
short gDebug;
byte rf_count;

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
  char tmpstr[7];
  
  if (debug) dbg_println("(");
  
  do 
  {
    mdbb.data = generateChecksum(data, length);
    mdbb.mode = 0x1;

    tx = 0;
    memcpy(&tx, &mdbb, 2);

    for (int i=0; i < length; i++) 
      MDB_Write(data[i].data);

    MDB_Write(tx); //chk

    while ( !(UCSR0A & (1<<RXC0)) );
    
    get_MDB_Byte(&mdbb);
    
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
    if (debug) dbg_println(")");
  
  return transmitState;
} // end void transmitData


void serial_init()
{
  // Set baud rate
  UBRR0H = UBRRH_VALUE;
  UBRR0L = UBRRL_VALUE;

  // Disable USART rate doubler (arduino bootloader leaves it enabled...)
  UCSR0A &= ~(1 << U2X0);
    
  // Set async, no parity, 1 stop, 9bit
  UCSR0C = (0<<UMSEL01)|(0<<UMSEL00)|(0<<UPM01)|(0<<UPM00)|(0<<USBS0)|(1<<UCSZ01)|(1<<UCSZ00); 
  UCSR0B |= (1<<UCSZ02); // 9bit

  // Enable rx/tx
  UCSR0B |= (1<<RXEN0)|(1<<TXEN0);
}

void get_MDB_Byte(struct MDB_Byte* mdbb)
{
  int b;
  char tmpstr[7];

  b = 0;
  b = USART_Receive();
  memcpy (mdbb, &b, 2); 
  
  if (mdbb->mode)
    sprintf(tmpstr, "<%.2x*", mdbb->data);
  else
    sprintf(tmpstr, "<%.2x", mdbb->data);
     
  if (gDebug > 1) dbg_print(tmpstr);
}

int USART_Receive()
{
  unsigned char status, resh, resl;
  char tmpstr[64];  

  // Wait for data to be received 
  while (!(UCSR0A & (1<<RXC0)));
  /* Get status and 9th bit, then data */
  /* from buffer */
  status = UCSR0A;
  resh = UCSR0B;
  resl = UDR0;

  /* If error, return -1 */
  if ( (status & (1<<FE0))  | (status & (1<<DOR0)) | (status & (1<<UPE0)))
  {
    sprintf(tmpstr, "<RX ERR: %.2x [%d,%d,%d], [%.2x, %.2x]", status, (status & (1<<FE0)), (status & (1<<DOR0)),(status & (1<<UPE0)), resh, resl);
    dbg_print(tmpstr);    
    return -1;
  }

  /* Filter the 9th bit, then return */
  resh = (resh >> 1) & 0x01;
  return ((resh << 8) | resl);
}

void MDB_Write(int data)
{
  char tmpstr[7];
  struct MDB_Byte b;
  
  memcpy(&b, &data, 2);
  
  serial_write(b);
  
  if (debug) 
  {  
    if (b.mode)
      sprintf(tmpstr, ">%.2x*", b.data);
    else
      sprintf(tmpstr, ">%.2x", b.data);
    if (gDebug > 1) dbg_print(tmpstr);
  }  
}

void serial_write(struct MDB_Byte mdbb)
{    
  while ( !( UCSR0A & (1<<UDRE0)));
  
  UCSR0B &= ~(1<<TXB80);  
  
  if (mdbb.mode)
    UCSR0B |= (1<<TXB80);
  
  UDR0 = mdbb.data;
}

void setup() 
{  
  // Clear tx/rx buffers
  memset(recvData, 0, sizeof(recvData));
  memset(sendData, 0, sizeof(sendData));
  
  // Clear card number buffer
  memset(rfid_serial, 0, sizeof(rfid_serial));
  
  // Clear transaction id
  memset(tran_id, 0, sizeof(tran_id));
  
  // Get debug level
  gDebug = EEPROM.read(EEPROM_DEBUG);
  
  // Get MAC address
  NanodeUNIO unio(NANODE_MAC_DEVICE);
  unio.read(mymac,NANODE_MAC_ADDRESS,6);
  
  // Init network
  memset (udpPayload, 0, sizeof(udpPayload));

  // Initialise SPI interface & ENC28J60
  es.ES_enc28j60SpiInit();
  es.ES_enc28j60Init(mymac, 8);

  //init the ethernet/ip layer:
  es.ES_init_ip_arp_udp_tcp(mymac,myip, 80);  
  
  debug = 1;
  
  // Start RFID Serial
 // Serial.begin(115200);
  
  card_state = NO_CARD;
  cancel_vend = 0;
  
  // Init. hardware serial for 9600/9bit
  serial_init(); 
  
  // Setup Pins
  pinMode(RETURN_BUTTON, INPUT);
  pinMode(LED_ACTIVE, OUTPUT);
  pinMode(LED_CARD, OUTPUT);
  pinMode(LED_WAIT, OUTPUT);  
  
  // Set default output's
  digitalWrite(SPEAKER, HIGH);
  digitalWrite(LED_ACTIVE, LOW);
  digitalWrite(LED_CARD, LOW);
  digitalWrite(LED_WAIT, LOW);
  
  state = sINACTIVE;
  // flags defaults
  allowACK = 1;
  justReset = true;
  commandOOS = false;
  allowVend = 0;
  vendAmountA = 0;
  vendAmountB = 0;
  ret_button = false;
  
  rfid_reader.begin(9600);
  
  // let everything else settle
  delay(100);
  
  //Get MAC address of server
  get_mac();
  
  net_tx_debug(); // reqeust debug level
  
  dbg_print("Init complete");  
} // end void setup()


void get_mac()
{
  byte gotmac = 0;  
  int plen, i;
  
  unsigned long time = millis();
  
  // Send arp request
  es.ES_client_arp_whohas(buf, dstip);
  
  while(!gotmac)
  {
    plen = es.ES_enc28j60PacketReceive(BUFFER_SIZE, buf); 
    if (plen>0)
    {
      if (es.ES_eth_type_is_arp_and_my_ip(buf, plen))
      {          
        gotmac = 1;
        
        // Check the reply is actually for dstip
        for (i=0; i < 4; i++)
          if(buf[ETH_ARP_SRC_IP_P+i]!=dstip[i])
            gotmac  = 0;
         
        if (gotmac)
        {
          // Save mac & return
          for (i=0; i < 6; i++)
            dstmac[i] = buf[ETH_ARP_SRC_MAC_P + i];     
          
          return;      
        }
      }
    }    
    
    if ((millis() - time) > ARP_RETRY)
    {
      time = millis();
      // Re-send arp request
      es.ES_client_arp_whohas(buf, dstip);      
    } 
    
  } // while(!gotmac)
  
}

void MDB_reset()
{
  if (rst)
  {
    rst = 0;
    digitalWrite(LED_WAIT, LOW);
  } else
  {
    rst = 1;
    digitalWrite(LED_WAIT, HIGH);
  }
  
  // Reset has no associated data, so just read checksum byte
  get_MDB_Byte(&recvData[1]);
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
    if (debug) dbg_println("Reset recv and checked, ACK sent");

  } else 
  {
    MDB_Write(sNAK);
    if (debug) dbg_println("Reset recv, checksum invalid, NAK sent");            
  }
}

void MDB_setup()
{
  get_MDB_Byte(&recvData[1]);
  if (recvData[1].data == CONFIG_DATA) 
  {
    for (int i = 2; i < 7; i++) 
      get_MDB_Byte(&recvData[i]);

    if (validateChecksum(recvData, 7)) 
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
                // god knows what we do here!          
      } else 
      {
        // we are rCONTINUE
        // we still need min/max prices to change state
        if (debug) dbg_println("Setup recv, our config sent");
      }
    } else 
    {
      // bad checksum
      MDB_Write(sNAK);
    }
  } else if (recvData[1].data == MINMAX_PRICES) 
  {
    // this has four additional data bytes (plus CHK of course)
    for (int i = 2; i < 7; i++) 
      get_MDB_Byte(&recvData[i]);
       
    if (validateChecksum(recvData, 7)) 
    {
      // Store the VMC data
      // max and min prices stored in two bytes each, max first
      maxPrice = recvData[2].data << 8;
      maxPrice += recvData[3].data;
      minPrice = recvData[4].data << 8;
      minPrice += recvData[5].data;
#ifdef DEBUG_PRINT_1
      if (debug) dbg_print("min: ");
      if (debug) dbg_print(recvData[4].data, HEX);
      if (debug) dbg_print(", ");
      if (debug) dbg_print(recvData[5].data, HEX);
      if (debug) dbg_print(": ");
      if (debug) dbg_println(minPrice);
#endif
              
      // respond with just an ACK
      MDB_Write(sACK);
      state = sDISABLED;
      digitalWrite(LED_ACTIVE, LOW);

      if (debug) dbg_println("State Change: Disabled!");

    } else 
    {
      // invalid checksum
      MDB_Write(sNAK);
    }
  }
}

void MDB_poll()
{
  // lots of POLL sub commands might hand off to a sub routine to keep code cleaner
  // has no associated data, so just read checksum byte
  get_MDB_Byte(&recvData[1]);
  
  if (validateChecksum(recvData, 2) == 0) 
  {
    // bad checksum
    MDB_Write(sNAK);
    if (debug) dbg_println(">>NAK - bad checksum");
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
#ifdef DEBUG_PRINT_1
    if (debug) dbg_println("Sent command out of sequence");
#endif
    return;
  }
  
  if (justReset) 
  {
    sendData[0].data = JUST_RESET;
    transmitData(sendData, 1);
    justReset = false;  // told vmc 
#ifdef DEBUG_PRINT_1
    if (debug) dbg_println("Sent JUST_RESET");
#endif
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
       if (debug) dbg_println("huh?");
     } else 
     {
       // vend should work!
       allowVend = 0;
     }
     
     if (debug) dbg_println("allowed vend");
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
//#ifdef HUMAN_PRINT
       if (debug) dbg_println("State change: Session Idle!");
//#endif
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
  get_MDB_Byte(&recvData[1]);
  
  if (recvData[1].data == READER_ENABLE) 
  {
    get_MDB_Byte(&recvData[2]);
    if (validateChecksum(recvData, 3)) 
    {
      if (state == sDISABLED) 
      {
        state = sENABLED;
        MDB_Write(sACK);
        digitalWrite(LED_ACTIVE, HIGH);
        memset(rfid_serial, 0, sizeof(rfid_serial));
#ifdef HUMAN_PRINT
        if (debug) dbg_println("State Change: Enabled!");
#endif
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
    get_MDB_Byte(&recvData[2]);
    if (validateChecksum(recvData, 3)) 
    {
      if (state == sENABLED) 
      {
        state = sDISABLED;
        MDB_Write(sACK);
        digitalWrite(LED_ACTIVE, LOW);
        memset(rfid_serial, 0, sizeof(rfid_serial));

        if (debug) dbg_println("State Change: Disabled! (VMC commanded)");

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
  get_MDB_Byte(&recvData[1]);
  char buf[32];
  int amount;
  int amount_scaled;
  
  
  switch (recvData[1].data)
  {
    case VEND_REQUEST:
      // four more bytes of data, plus checksum
      for (int i = 2; i < 6; i++) 
        get_MDB_Byte(&recvData[i]);
      

      amount = recvData[2].data << 8;
      amount += recvData[3].data;
      amount_scaled = (amount * SCALE_FACTOR);
      
     // sprintf(buf, "Vendreq %.2x, %.2x", recvData[2], recvData[3]); 
     // if (debug) dbg_print(buf);
      sprintf(buf, "Vendreq %d (%dp)", amount, amount_scaled); 
     
      if (debug) dbg_print(buf);      
      
      if (/*validateChecksum(recvData, 6)*/ 1) 
      {
      // find price and item number and hand off to network        
      // for now we just ACK and let the POLL respond
        MDB_Write(sACK);
        allowVend = 0;
        net_tx_vreq(amount_scaled);   // Send VREQ message for the amount

//        allowVend = checkVend(recvData);
        // let's see what the vmc has sent DEBUG
        //if (debug) dbg_println("vend started: ");
        //for (i = 2; i < 6; i++) {
        //  if (debug) dbg_print("b: ");
        //  if (debug) dbg_println(recvData[i], HEX);
        //}
      } else 
      {
        MDB_Write(sNAK);
      }
      break;
      
    case VEND_SUCCESS:
      // two more bytes of data
      for (int i = 2; i < 4; i++) 
        get_MDB_Byte(&recvData[i]);      
      
      sprintf(buf, "%.2x-%.2x", recvData[2].data, recvData[3].data); 
      MDB_Write(sACK);
      net_tx_vsuc(buf);
      break;
      
   case VEND_FAILURE:     
      MDB_Write(sACK);
      net_tx_vfail();
      if (debug) dbg_print("Vend failure!");
      break;
      
    case SESSION_COMPLETE:
      if (debug) dbg_print("Session complete");
      // build the response
      sendLength = 1;
      memset(sendData, 0, sizeof(sendData));
      sendData[0].data = END_SESSION;        
      transmitData(sendData, sendLength);  
      memset(rfid_serial, 0, sizeof(rfid_serial)); 
      memset(tran_id, 0, sizeof(tran_id));       
      card_state = NO_CARD;
      state = sENABLED;
      digitalWrite(LED_ACTIVE, HIGH);  
      break;
      
    case CASH_SALE:
      if (debug) dbg_print("Cash sale");
       // four more bytes of data, plus checksum
      for (int i = 2; i < 6; i++) 
        get_MDB_Byte(&recvData[i]);

      amount = recvData[2].data << 8;
      amount += recvData[3].data;
      amount_scaled = (amount * SCALE_FACTOR);   
      
      // Get location
      sprintf(buf, "%.2x-%.2x", recvData[4].data, recvData[5].data); 

      MDB_Write(sACK);
      net_tx_cash(amount_scaled, buf);   // Send CASH message
      break;   
    
      
    default:
      if (debug) dbg_print("MDB_vend> Error");
  }
}

void loop() 
{
  
/*  while (1)     
  {
      while ( !( UCSR0A & (1<<UDRE0))); 
    // if (mdbb.mode)
     //   UCSR0B |= (1<<TXB80);
  
      UDR0 = '0';
  }
  */
  int i;
  uint16_t plen, dat_p;
  byte readcard = 0;
  char tmpstr[16];
  
  while (1)
  {
    
    if ((UCSR0A & (1<<RXC0))) 
    {
      get_MDB_Byte(&recvData[0]);
      
      if ((recvData[0].mode) && ((recvData[0].data & ADDRESS_MASK) == CASHLESS_ADDRESS))
      {
        switch (recvData[0].data) 
        {
          case RESET:
            dbg_println("Reset");
            MDB_reset();
            break;
  
          case SETUP:
            dbg_println("Setup");
            MDB_setup();        
            break;
            
          case POLL:
            if (gDebug > 1) dbg_println("Poll");
            MDB_poll();
            break;
            
          case READER:
            dbg_println("Reader");
            MDB_reader();
            break;          
           
          case VEND:
            dbg_println("Vend");
            MDB_vend();          
            break;
                      
          case EXPANSION:
            dbg_println("Expan");
            break;
            
          default:
            // should never get here once done programing all features
            dbg_println("Unknown Command recv");
            break;
        } 
      } else // end if 
      {
        // Only want to try and read a card after the VMC has polled device 0x50  (it's the last in it's polling cycle)
        if ((recvData[0].mode) && (recvData[0].data == 0x50))
        {
          readcard = 1;
        }      
    
      // Poll RFID
      if ((state == sENABLED) && (card_state == NO_CARD) && (readcard) && (!(recvData[0].mode)))
      {
        readcard=0;
        // we can now check for a card
        if (pollRFID())
        {
          // card has been read
          card_state = NET_WAIT;     
          net_tx_auth(); // try to authenticate card
        }
      }
      
     }
   
    } // end if (MDB.available())
    net_loop();
    
    
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
    dbg_println("To late to cancel vend");
    return;    
  }
  
  if ((rfid_serial[0] != 0) || (tran_id[0] != 0))
  {
    net_tx_vcan();
    cancel_vend = 1;
    memset(rfid_serial, 0, sizeof(rfid_serial)); 
    memset(tran_id, 0, sizeof(tran_id));       
    card_state = NO_CARD;  
  } else
  {
    dbg_println("Nothing to cancel");
  }
}

void net_loop()
{
  int i;
  uint16_t plen, dat_p;

  plen = es.ES_enc28j60PacketReceive(BUFFER_SIZE, buf);
  
  if (plen) // 0 if no packet received
  {
    dat_p=es.ES_packetloop_icmp_tcp(buf,plen);

    if (es.ES_eth_type_is_ip_and_my_ip(buf, plen)) // Is packet for us?
    {
      // Is it UDP & on the expected port?
      if ((buf[IP_PROTO_P] == IP_PROTO_UDP_V) && (buf[UDP_DST_PORT_H_P]==PORT_H) && (buf[UDP_DST_PORT_L_P] == PORT_L )) 
      {
        memcpy(net_msg.msgtype, buf+UDP_DATA_P, 4);
        memcpy(net_msg.payload, buf+UDP_DATA_P+5, sizeof(net_msg.payload));  
        
        if (!(strncmp((char*)net_msg.msgtype, "GRNT", 4)))
          net_rx_grant(net_msg.payload);
        
        else if (!(strncmp((char*)net_msg.msgtype, "DENY", 4)))
          net_rx_deny(net_msg.payload);        
        
        else if (!(strncmp((char*)net_msg.msgtype, "VNOK", 4))) // VeNd OK
          net_rx_vend_ok(net_msg.payload);
        
        else if (!(strncmp((char*)net_msg.msgtype, "VDNY", 4))) // Vend DeNY
          net_rx_vend_deny(net_msg.payload);
          
        else if (!(strncmp((char*)net_msg.msgtype, "DBUG", 4))) // DeBUG
          net_rx_set_debug(net_msg.payload);          
      }   
    }
  }
}

void net_rx_grant(byte *payload)
{
  // expected payload format:
  // <rfid serial>:<transaction id>
  int i;
  char buf[25];
  
  for (i=0; (i < sizeof(net_msg.payload)) && (payload[i] != ':'); i++); // Find ':'
  if (payload[i++] != ':')
  {
    card_state = NO_CARD;
    memset(rfid_serial, 0, sizeof(rfid_serial));
    dbg_println("tID not found");
    return;
  }
  
  // check card number matches
  if (!strncmp((char*)payload, rfid_serial, i))
  {
    // hmmm, the card read doesn't match the one that's just been accepted.
    card_state = NO_CARD;
    memset(rfid_serial, 0, sizeof(rfid_serial));
    dbg_println("Card serial mismatch");
    return;
  }
    
  memcpy(tran_id, payload+i, sizeof(tran_id)-1);
  sprintf(buf, "trnid=%s", tran_id);
  dbg_println(buf);
  card_state = GOOD;

} 

void net_rx_deny(byte *payload)
{
  // Not too bothered about the payload here - either the card number in the message matches, in which case
  // we want to decline the card, or it doesn't and something's gone wrong, so we want to decline the card...
  
  dbg_println("Card declined");
  memset(rfid_serial, 0, sizeof(rfid_serial));
  card_state = NO_CARD; 
} 


void net_rx_vend_ok(byte *payload)
{
  // Expeceted format:
  // <card>:<transaction id>
  
  int i;
  
  for (i=0; (i < sizeof(net_msg.payload)) && (payload[i] != ':'); i++); // Find ':'
  if (payload[i++] != ':')
  {
    // invalid message
    allowVend = 2;
    dbg_println("Vend denied (1)");
    return;
  }
  
  // Check we still have the card details
  if (strlen(rfid_serial) <= 1)
  {
    allowVend = 2;
    dbg_println("Card details lost!");
    return;
  }
  
  // card number in message must match stored card number
  if (strncmp(rfid_serial, (char*)payload, strlen(rfid_serial)))
  {
    allowVend = 2;
    dbg_println("Card mismatch");
    return;
  }
    
  // stored transaction number must match that in the message
  if (strncmp(tran_id, (char*)payload+i, strlen(tran_id)))
  {
    allowVend = 2;
    dbg_println("tran_id mismatch");
    return;
  }    
    
  // To get here, all must be good - so allow vend
  allowVend = 1;
  dbg_println("Allowing vend.");
} 

void net_rx_vend_deny(byte *payload)
{
  // Not too bothered about the payload here - either the card number in the message matches, in which case
  // we want to decline the card, or it doesn't and something's gone wrong, so we want to decline the card...
  
  dbg_println("Card declined (2)");
  allowVend = 2;
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
    dbg_println("EEPROM Updated");
  }
    
  gDebug = new_dlvl;  
  dbg_println("set debug");
  gDebug = 2;
} 



// Auth card - on card first being presented, prior to telling the vending machine
void net_tx_auth()
{
  struct net_msg_t msg;
  memcpy(msg.msgtype,"AUTH", 4);
  strncpy((char*)msg.payload, rfid_serial, sizeof(rfid_serial));
  net_send(&msg);  
}

// Vend request
void net_tx_vreq(int amount_scaled)
{
  struct net_msg_t msg;
  memcpy(msg.msgtype,"VREQ", 4);
  sprintf((char*)msg.payload, "%s:%s:%d", rfid_serial, tran_id, amount_scaled);
  net_send(&msg);  
}

// Vend success
void net_tx_vsuc(char *pos)
{
  struct net_msg_t msg;
  memcpy(msg.msgtype,"VSUC", 4);
  sprintf((char*)msg.payload, "%s:%s:%s", rfid_serial, tran_id, pos);
  net_send(&msg);  
}

// Vend failure
void net_tx_vfail()
{
  struct net_msg_t msg;
  memcpy(msg.msgtype,"VFAL", 4);
  sprintf((char*)msg.payload, "%s:%s", rfid_serial, tran_id);
  net_send(&msg);  
}

// Vend cancel
void net_tx_vcan()
{
  struct net_msg_t msg;
  memcpy(msg.msgtype,"VCAN", 4);
  sprintf((char*)msg.payload, "%s:%s", rfid_serial, tran_id);
  net_send(&msg);  
}

// Cash sale
void net_tx_cash(int amount_scaled, char *pos)
{
  struct net_msg_t msg;
  memcpy(msg.msgtype,"CASH", 4);
  sprintf((char*)msg.payload, "%d:%s", amount_scaled, pos);
  net_send(&msg);  
}

void dbg_println(char* str)
{
  //todo.
  dbg_print(str);
}
  
void dbg_print(char* str)
{
  if (gDebug > 0)
  {
    struct net_msg_t msg;
    memcpy(msg.msgtype,"INFO", 4);
    strncpy((char*)msg.payload, str, sizeof(msg.payload));
    net_send(&msg);
  }
}

// Request nh-vend send us what debug level we should be running at
void net_tx_debug()
{
  struct net_msg_t msg;
  memcpy(msg.msgtype,"DBUG", 4);
  strncpy((char*)msg.payload, rfid_serial, sizeof(rfid_serial));
  net_send(&msg);  
}

void net_send(struct net_msg_t *msg)
{
  uint8_t i=0;
  uint16_t ck;
  
  memset(udpPayload, 0, sizeof(udpPayload));
  memcpy(udpPayload, msg->msgtype, 4);
  udpPayload[4] = ':';
  memcpy(udpPayload+5, msg->payload, sizeof(msg->payload));
  
  // Setup the MAC addresses for ethernet header
  while(i<6)
  {
    buf[ETH_DST_MAC +i]= dstmac[i]; //0xff; // Broadcast address
    buf[ETH_SRC_MAC +i]= mymac[i];
    i++;
  }
  buf[ETH_TYPE_H_P] = ETHTYPE_IP_H_V;
  buf[ETH_TYPE_L_P] = ETHTYPE_IP_L_V;
  es.ES_fill_buf_p(&buf[IP_P],9,iphdr);

  // IP Header
  buf[IP_TOTLEN_L_P]=28+sizeof(udpPayload);
  buf[IP_PROTO_P]=IP_PROTO_UDP_V;
  i=0;
  while(i<4)
  {
    buf[IP_DST_P+i] = dstip[i];
    buf[IP_SRC_P+i] = myip[i];
    i++;
  }
  es.ES_fill_ip_hdr_checksum(buf);
  buf[UDP_DST_PORT_H_P]=DEST_PORT_H;
  buf[UDP_DST_PORT_L_P]=DEST_PORT_L;
  buf[UDP_SRC_PORT_H_P]=PORT_H;
  buf[UDP_SRC_PORT_L_P]=PORT_L; 
  buf[UDP_LEN_H_P]=0;
  buf[UDP_LEN_L_P]=8+sizeof(udpPayload); // fixed len
  // zero the checksum
  buf[UDP_CHECKSUM_H_P]=0;
  buf[UDP_CHECKSUM_L_P]=0;
  // copy the data:
  i=0;
  // most fields are zero, here we zero everything and fill later
  uint8_t* b = (uint8_t*)&udpPayload;
  while(i< sizeof( udpPayload ) )
  { 
    buf[UDP_DATA_P+i]=*b++;
    i++;
  }
  // Create correct checksum
  ck=es.ES_checksum(&buf[IP_SRC_P], 16 + sizeof( udpPayload ),1);
  buf[UDP_CHECKSUM_H_P]=ck>>8;
  buf[UDP_CHECKSUM_L_P]=ck& 0xff;
  es.ES_enc28j60PacketSend(42 + sizeof( udpPayload ), buf);
}



  
/**************************************************** 
 * Poll RFID
 *  
 ****************************************************/
bool pollRFID()
{
   if (gDebug > 1) dbg_println("Poll R");
  // quickly send query to read cards serial
  for (int i=0; i < 8; i++)
    rfid_reader.print(query[i]);
  
  // get the serial number
  read_response(rfidReturn);
  
  // check status flag is clear and that we have a serial 
  if (rfidReturn[3] == 0 && rfidReturn[2] >= 6) 
  {
    // grab first digit of serial
    unsigned long cardNumber = rfidReturn[5];
    
    // loop for the rest
    for (int j=1; j < (rfidReturn[2] - 2); j++)
      cardNumber = (cardNumber << 8) | rfidReturn[j+5];
    
    // check we are not reading the same card again or if we are its been a sensible time since last read it
    if (cardNumber != lastCardNumber || (millis() - cardTimeOut) > CARD_TIMEOUT) 
    {
      lastCardNumber = cardNumber;
      cardTimeOut = millis();
      // convert cardNumber to a string array
      ultoa(cardNumber, rfid_serial, 10);
      return true;
   // delay(50);

    } // end if
    
    // there's a card but we can't read it
  } else if (rfidReturn[3] == 1 && rfidReturn[4] != 131 && (millis() - cardTimeOut) > CARD_TIMEOUT) 
  {
    cardTimeOut = millis();
  //  Serial.println("Unknown Card Type");
  } 
  return false;
} // end bool pollRFID()


void read_response(unsigned char* dataResp)
{
  byte incomingByte;
  byte responsePtr =0;
  unsigned long time = millis();
  while (millis() - time < 4) 
  {
    // read the incoming byte:
    if (rfid_reader.available() > 0) 
    {
      incomingByte = (char)rfid_reader.read();
      dataResp[responsePtr++]=incomingByte;
      time = millis();
    }
  } // end while
  dataResp[responsePtr]=0;
} // end void read_response(char* DATARESP)

