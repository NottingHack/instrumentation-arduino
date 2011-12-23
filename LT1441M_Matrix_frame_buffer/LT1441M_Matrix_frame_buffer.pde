
/****************************************************	
 * sketch = LT1441M_Matrix_frame_buffer
 * LWK's LT1441M Matrix Frame Buffer TEST
 * CC-BY-SA
 * Source = lwk.mjhosting.co.uk
 * Target controller = Arduino 328
 * Clock speed = 16 MHz
 * Development platform = Arduino IDE 002.
 * C compiler = WinAVR from Arduino IDE 0022
 * Programer = Arduino 328
 * Target sharp LYT1441M
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *
 * Origanl designed for nottinghack's donated LED matrix boards
 * http://wiki.nottinghack.com/wiki/LED_Matrix
 *
 *
 ****************************************************/
 
/*  
History
 	001 - Initial release 
            Commands
             - PGxxx  print text in green
             - PRxxx  print text in red
             - PN     clear the board
             - FSxxx  font select xxx = name on linked font file
 Known issues:
        None
 Future changes:
 	None

 ToDo:
 	Lots
 */

#define VERSION_NUM 001

//Uncommetn for debug prints
//#define DEBUG_PRINT

//basic 8X8 font
#include "font8x8.h"


/*
 * The LT1441M has a 7pin connector for the dasiy chained date lines 
 * 
 * GSI       Serial data yellow-green
 * /GAEO     Output enable for yellow-green
 * LATCH     Latch contents of shift register
 * GND1      Ground of IC
 * CLOCK     Clock Signal for data (read on L->H)
 * /RAEO     Output enable for red
 * RSI       Serial data red
 *
 * Power is via CN1
 * V1        Power for LED(red)
 * GND1      Ground for IC
 * VDD       Power for IC
 * GND2      Ground for LED
 * V2        Power for LED(yellow-green
 */

/* pin numbers for connection LT1441M */
#define GSI 8
#define GAEO 9
#define LATCH 10
#define CLOCK 11
#define RAEO 12
#define RSI 13

#define RED 1
#define GREEN 2

/*
assume config of 1X8 display 
each panel is 16x16 pixels
so 16X128 display
font is 8x8
2x16 char's
use display upside down!! so pixels are up->down left->right oppsit to pdf!! :D
font file are in up->down left-right format all ready :)

123456789012
Hello World! 
-is 12 char's 
123456789012345
Nottinghack!

*/

#define DISPLAY_WIDTH 128
#define BUFFER_LENGTH 150
//frame bufffers  96 cols 8pix x 12 chars
unsigned char line1[BUFFER_LENGTH+1] = {0};
unsigned char line2[BUFFER_LENGTH+1] = {0};



void setup() {

 /* Basic Pin Setup */
 pinMode(GSI, OUTPUT);
 pinMode(GAEO, OUTPUT);
 pinMode(LATCH, OUTPUT);
 pinMode(CLOCK, OUTPUT);
 pinMode(RAEO, OUTPUT);
 pinMode(RSI, OUTPUT);
 digitalWrite(GSI, LOW);
 digitalWrite(GAEO, HIGH);
 digitalWrite(LATCH, HIGH);
 digitalWrite(CLOCK, LOW);
 digitalWrite(RAEO, HIGH);
 digitalWrite(RSI, LOW);
 
 /* setup all done start the serial port */
 Serial.begin(115200);
 Serial.print("LT1441M_Matric_buffer_test v:");
 Serial.println(VERSION_NUM);
 Serial.println("Hello..."); 
 
 
 buffer();  // setup text in buffers
   //matrixOut(0);
} //end void setup()

void loop() {

  for (int l = 0; l < BUFFER_LENGTH; l++) {
    matrixOut(l);
    delay(200);
  }

  
} //end void loop()

void buffer() {
 
  char text1[] = "Welcome   to";
  char text2[] = "NottingHack!";
    for (int i = 0; i < 14; i++) { 
      unsigned int c = text1[i] *8;
      unsigned int d = text2[i] *8; 
    for (int j = 0; j < 8; j++) {
      line1[ (i*8) + j ] =  Standard8x8[c++];   
      line2[ (i*8) + j ] =  Standard8x8[d++];
      //Serial.print(line1[(i*8)+j],HEX);
    }
  }
  
  
}


void matrixOut(int offset) {
  // Standard8x8[] font

//  digitalWrite(LATCH, LOW);
//  digitalWrite(LATCH, HIGH);
  
  //hello world! top line red
  //shiftOut(dataPin, clockPin, bitOrder, value) 

  for (int i = 0; i < DISPLAY_WIDTH; i++) { 
        int o = i + offset;
    if (o > BUFFER_LENGTH)        // loop frame buffer ???
        o -= BUFFER_LENGTH;
        
      shiftOut(RSI, CLOCK, LSBFIRST, line1[o]); 
      shiftOut(GSI, CLOCK, LSBFIRST, line2[o]);
  }
  
  digitalWrite(LATCH, LOW);
  digitalWrite(LATCH, HIGH); 

  digitalWrite(GAEO, LOW);
  digitalWrite(RAEO, LOW);

} //end void matrixOut()

/*
void shiftOutDual(int dataPin, int dataPin1, int clockPin, int bitOrder, int val, int val1){

	int i;

	for (i = 0; i < 8; i++)  {
		if (bitOrder == LSBFIRST) {
			digitalWrite(dataPin, !!(val & (1 << i)));
			digitalWrite(dataPin1, !!(val1 & (1 << i)));
		}else{
			digitalWrite(dataPin, !!(val & (1 << (7 - i))));
			digitalWrite(dataPin1, !!(val1 & (1 << (7 - i))));
                }

		digitalWrite(clockPin, HIGH);
		digitalWrite(clockPin, LOW);

	}

} 

*/


