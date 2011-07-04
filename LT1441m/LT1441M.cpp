/****************************************************	
 * file = LT1441M.cpp
 * LWK's LT1441M Matrix Class
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



// include core Wiring API
#include "WProgram.h"
#include <avr/pgmspace.h>


// include this library's description file
#include "LT1441M.h"

uint8_t ReadPgmData(const uint8_t* ptr) {  // note this is a static function
	return pgm_read_byte(ptr);
}

// Constructors ////////////////////////////////////////////////////////////////
// Function that handles the creation and setup of instances


LT1441M::LT1441M(unsigned char gsi_pin, unsigned char gaeo_pin, unsigned char latch_pin, unsigned char clock_pin, unsigned char raeo_pin, unsigned char rsi_pin){
	// transfer pin setting to internal's
	_gsi_pin = gsi_pin;
	_gaeo_pin = gaeo_pin;
	_latch_pin = latch_pin;
	_clock_pin = clock_pin;
	_raeo_pin = raeo_pin;
	_rsi_pin = rsi_pin;
	
	// setup coord structed for use
	coord.x = 0;
	coord.y = 0;
	coord.page = 0;
	
	// clear frameBuffer
	for (int i = 0; i < dPage; i++) {
		for (int j = 0; j < dxRAM; j++) {
			frameBuffer[i][j] = 0x00;
		}
	}

} //end LT1441M::LT1441M(unsigned char gsi_pin, unsigned char gaeo_pin, unsigned char latch_pin, unsigned char clock_pin, unsigned char raeo_pinn, unsigned char rsi_pin)


// Public Methods //////////////////////////////////////////////////////////////
// Functions available in Wiring sketches, this library, and other libraries

void LT1441M::begin(void){

	// set Control pins to Output
	pinMode(_gsi_pin, OUTPUT);
	pinMode(_gaeo_pin, OUTPUT);
	pinMode(_latch_pin, OUTPUT);
	pinMode(_clock_pin, OUTPUT);
	pinMode(_raeo_pin, OUTPUT);
	pinMode(_rsi_pin, OUTPUT); 
	
	// set initial pin states and reset chip
	digitalWrite(_gsi_pin, LOW);
	digitalWrite(_gaeo_pin, HIGH);			// green off by default
	digitalWrite(_latch_pin, HIGH);
	digitalWrite(_clock_pin, LOW);
	digitalWrite(_raeo_pin, HIGH);			// red off by default
	digitalWrite(_rsi_pin, LOW);
	
	// Clear the frameBuffer down to start with and rests cusor to (0,0)
	clrScreen();
	
} //end void LT1441M::begin(void)


void LT1441M::write(unsigned char c){				// wirte single ASCII char to display
	if (c == 0x0D){								// check for CR '\r'
		setCol(0);
	}else if(c == 0x0A){						// check for LF		'\n' increas based on current font height
		setPage(coord.page + (FontRead(Font+FONT_HEIGHT)+7)/8);
	}else{
		putChar(c);								// output char
	}
} //end void LT1441M::write(unsigned char c)

void LT1441M::clrScreen(void){						// clears the screen and returns cursor to (0,0)
	 for (int i=0; i <= pageLast; i++){		// loop for pages 
		setCol(0);
		setPage(i);
		for (int j=0; j <= xMost; j++){		// for 128 colums?? **LWK**updaet to use const defines in header
			write_d(0x00);
		}
	}
	setCursor(0,0);
#ifdef DEBUG_PRINT
	Serial.println("clrScreen");
#endif	
	
} //end void LT1441M::clrScreen(void)

void LT1441M::clrPage(unsigned char page){			// Clear a single page and return cursor to (0, page)
	setCol(0);
	setPage(page);
	for (int j=0; j <= xMost; j++){			// for 128 colums?? **LWK**updaet to use const defines in header
		write_d(0x00);
	}
	setCol(0);
	setPage(page);								// return cursor to (0, page)
} //end void LT1441M::clrPage(unsigned char page)

void LT1441M::setCursor(unsigned char x, unsigned char y){		// set frameBuffer postion for write
	if (x > xMost){								// check to make sure not off right edge of screen and
		x=0;									// reset start of screen
	}
	coord.x = x;								// set coord to match
	unsigned char page;
	if (y > yMost){								// check to make sure not off bottom edge of screen and
		page = 0;								// reset start of screen
	}else{
		page = y / 8;						// ***LWK***
	}
	coord.page = page;							// set coord to match
	coord.y = y;
} //end void LT1441M::setCursor(unsigned char x, unsigned char y)

void LT1441M::setPage(unsigned char page){			// set frameBuffer page for write
	if (page > pageLast){						// check to make sure not off bottom edge of screen and
		page=0;									// reset start of screen
	}

	coord.page = page;							// set coord to match
	coord.y = page * 8;							// set coord.y to first row in page
} //end void LT1441M::setPage(unsigned char page)

void LT1441M::setCol(unsigned char x){				// set frameBuffer col for write
	if (x > xMost){							// check to make sure not off right edge of screen and
		x=0;									// reset start of screen
	}
	coord.x = x;								// set coord to match
} //end void LT1441M::setCol(unsigned char x)

void LT1441M::setRow(unsigned char y){				// set frameBuffer row for write
	unsigned char page;
	if (y > yMost){								// check to make sure not off bottom edge of screen and
		page = 0;								// reset start of screen
		y = 0;
	}else{
		page = y / 8;						// 16 pixels, y is  0-7 = 0, 8-15 = 2, 16> =0
	}
	coord.page = page;							// set coord to match
	coord.y = y;
} //end void LT1441M::setRow(unsigned char y)

void LT1441M::invert(void){						// inverts the display
	// needs work
} //end void LT1441M::invert(void)

void LT1441M::normal(void){						// restore normal operation after invert
	// needs work
} //end void LT1441M::normal(void)


void LT1441M::selectFont(const uint8_t* font,uint8_t color, FontCallback callback) {
	Font = font;
	FontRead = callback;
	FontColor = color;
} //end void LT1441M::selectFont(const uint8_t* font,uint8_t color, FontCallback callback)

int LT1441M::putChar(unsigned char c) {			// Put Char to display handels varible fonts
	uint8_t width = 0;
	uint8_t height = FontRead(Font+FONT_HEIGHT);
	uint8_t bytes = (height+7)/8;
	
	uint8_t firstChar = FontRead(Font+FONT_FIRST_CHAR);
	uint8_t charCount = FontRead(Font+FONT_CHAR_COUNT);
	
	uint16_t index = 0;
	uint8_t x = coord.x, y = coord.y;
	
	if(c < firstChar || c >= (firstChar+charCount)) {
		return 1;
	}
	c-= firstChar;

	if( FontRead(Font+FONT_LENGTH) == 0 && FontRead(Font+FONT_LENGTH+1) == 0) {
    // zero length is flag indicating fixed width font (array does not contain width data entries)
	   width = FontRead(Font+FONT_FIXED_WIDTH); 
	   index = c*bytes*width+FONT_WIDTH_TABLE;
	}
	else{
	// variable width font, read width data, to get the index
	   for(uint8_t i=0; i<c; i++) {  
		 index += FontRead(Font+FONT_WIDTH_TABLE+i);
	   }
	   index = index*bytes+charCount+FONT_WIDTH_TABLE;
	   width = FontRead(Font+FONT_WIDTH_TABLE+c);
    }

	// last but not least, draw the character
	for(uint8_t i=0; i<bytes; i++) {
		uint8_t page = i*width;
		for(uint8_t j=0; j<width; j++) {
			uint8_t data = FontRead(Font+index+page+j);
		
			if(height > 8 && height < (i+1)*8) {
				data >>= (i+1)*8-height;
			}
			
			if(FontColor == BLACK) {
				write_d(data);
			} else {
				write_d(~data);
			}
		}
		// 1px gap between chars
		if(FontColor == BLACK) {
			write_d(0x00);
		} else {
			write_d(0xFF);
		}
		setCursor(x, coord.y+8);
	}
	setCursor(x+width+1, y);

	return 0;
} //end int LT1441M::putChar(unsigned char c)

void LT1441M::update(){											// update the display with current frameBuffer

	for (int i = 0; i < dxScreen; i++) {

		int j = i + offset[0];
		if (j > xMost) {
			j = j - dxRAM;
		}
		
		shiftOutL(_rsi_pin, _clock_pin, LSBFIRST, frameBuffer[0][j]); 
		shiftOutL(_rsi_pin, _clock_pin, LSBFIRST, frameBuffer[1][j]);
	}
	
	digitalWrite(_latch_pin, LOW);
	digitalWrite(_latch_pin, HIGH); 
	
	updateScroll();

} //end void LT1441M::update() 	

void LT1441M::updateScroll(){
	
	for (int i = 0; i < dPage; i++) {
		if(offset[i] > xMost){
			offset[i] -= xMost;
		} else {
			offset[i]++;
		}
	}
	
} //end voidScroll()

void LT1441M::shiftOutL(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder, uint8_t val){
	uint8_t i;
	
	for (i = 0; i < 8; i++)  {
		if (bitOrder == LSBFIRST) {
			digitalWrite(dataPin, !!(val & (1 << i)));
			digitalWrite(_gsi_pin, !!(val & (1 << i)));  /// ***LWK***
		} else {
			digitalWrite(dataPin, !!(val & (1 << (7 - i))));
		}
		
		//delayMicroseconds(200);
		digitalWrite(clockPin, HIGH);
		//delayMicroseconds(200);
		digitalWrite(clockPin, LOW);		
	}
} //end void shiftOut(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder, uint8_t val)



void LT1441M::enable(){													// turn on diaply
	digitalWrite(_gaeo_pin, LOW);			//enable green
	digitalWrite(_raeo_pin, LOW);			//enable red
} //end void LT1441M::enable()
	



void LT1441M::ScrollDisplay(){
	
} //end void ScrollDisplay(void)


// Private Methods /////////////////////////////////////////////////////////////
// Functions only available to other functions in this library

void LT1441M::write_d(unsigned char ins_c){		// write date to framebuffer
	frameBuffer[coord.page][coord.x] = ins_c;
	setCol(coord.x+1);

#ifdef DEBUG_PRINT
	Serial.print("(");
	Serial.print(coord.x, DEC);
	Serial.print(",");
	Serial.print(coord.y, DEC);
	Serial.print(",");
	Serial.print(coord.page, DEC);
	Serial.print(",");
	Serial.print(ins_c, HEX);
	Serial.println(")");
#endif	
	
} //end void LT1441M::write_d(unsigned char ins_c)

