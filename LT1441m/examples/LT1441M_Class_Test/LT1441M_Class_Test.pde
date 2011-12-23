/****************************************************	
 * sketch = LT1441M.pde
 * LWK's LT1441M Matrix Class example
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
 Known issues:
	None
 Future changes:
 	None

 ToDo:
 	Lots
 */

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


 
#include <LT1441M.h>        // Class for LT1441M
#include "Arial14.h"         // proportional font
#include "SystemFont5x8.h"   // system font
#include "LWK16v3.h"
#include "LWK16.h"

int buttonState = 0;
int buttonState2 = 0;
int buttonState3 = 0;
int inByte = 0;
int lastfont = 0;

/* pin numbers for connection LT1441M */
#define GSI 8
#define GAEO 9
#define LATCH 10
#define CLOCK 11
#define RAEO 12
#define RSI 13

/* 
 * setup Matrix instance
 * LT1441M(unsigned char gsi_pin, unsigned char gaeo_pin, unsigned char latch_pin, 
 *			unsigned char clock_pin, unsigned char raeo_pinn, unsigned char rsi_pin); 
 */
 
LT1441M myMatrix(GSI, GAEO, LATCH, CLOCK, RAEO, RSI);

void setup()
{


  Serial.begin(115200);      // only enable if atmega1280 as pin confilct on 328
  
  
  pinMode(8, INPUT);
  pinMode(9, INPUT);
  pinMode(13, OUTPUT);

  myMatrix.begin();
  myMatrix.selectFont(System5x8);  
  myMatrix.println("Northackton");
//  myMatrix.selectFont(LWK16v3); 
  myMatrix.println("Maker Faire 2011");
//  myMatrix.selectFont(System5x8);
  myMatrix.update();
  myMatrix.enable();
   
}

void loop()
{

/*  
 buttonState = digitalRead(8);
 buttonState2 = digitalRead(9);
 buttonState3 = digitalRead(10);

 // check if the pushbutton is pressed.
  // if it is, the buttonState is HIGH:
  if (buttonState == HIGH) {  
      myMatrix.selectFont(LWK16v3);
      myMatrix.clrScreen();
      myMatrix.println("THE QUICK BROWN");
      myMatrix.println("FOX JUMPS OVER");
      myMatrix.println("THE LAZY DOG");

//      myMatrix.println("jumps over the lazy dog");
//      myMatrix.println("This is 5");
   
  } 
  if (buttonState2 == HIGH) {     
      myMatrix.selectFont(LWK16v3);
      myMatrix.clrScreen();

      myMatrix.println("the quick brown");
      myMatrix.println("fox jumps over");
      myMatrix.println("the lazy dog");
    
  } 
  if (buttonState3 == HIGH) {  
      myMatrix.selectFont(LWK16v3);
      myMatrix.clrScreen();
      
      
          for(int a = 33; a <= 48; a++){
      myMatrix.write(a);
    }
        myMatrix.println();
    for(int a = 58; a <= 64; a++){
      myMatrix.write(a);
    }
    myMatrix.println();
        for(int a = 91; a <= 96; a++){
      myMatrix.write(a);
    }
        myMatrix.println();
        for(int a = 123; a <= 126; a++){
      myMatrix.write(a);
    }
    } 
  delay(150);
  */
  
  /*
    if (Serial.available() > 0) {
    // get incoming byte:
    inByte = Serial.read();
	if (inByte == 0x0D){								// check for CR
		myMatrix.setCol(0);
		myMatrix.setPage(myMatrix.coord.page + 1);
                Serial.println();
	}else if(inByte == 0x0A){						// check for LF
		myMatrix.setPage(myMatrix.coord.page + 1);
                Serial.print(inByte, HEX);
	}else if(inByte == 0x19){						// check for ^Y 		End of Medium
		myMatrix.clrScreen();
                Serial.write(0x1B);
                Serial.write('[1J');
       	}else{										// output char
                myMatrix.print(char(inByte));
                Serial.print(inByte);
        }
    //Serial.print(inByte, HEX);
    }
    */
    
  myMatrix.update();
  delay(100);
        

}

