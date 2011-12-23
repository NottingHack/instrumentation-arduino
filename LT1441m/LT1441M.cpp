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
 * Original designed for nottinghack's donated LED matrix boards
 * http://wiki.nottinghack.com/wiki/LED_Matrix
 *
 *
 ****************************************************/
/*
History
	001 - Initial release  
	002 - Complete rewrite with out print.h class, using own internal msg buffers
	
Known issues:
	None

Future changes:
	None
 
ToDo:
	Lots
 
 Authors:
	'RepRap' Matt      dps.lwk at gmail.com
 
 Thanks to:
	Michael Margolis for the arduino ks0108 libary used as a template
	mailto:memargolis@hotmail.com?subject=KS0108_Library
 
*/



// include core Wiring API
#include "WProgram.h"
#include <avr/pgmspace.h>
//#include <string.h>


// include this library's description file
#include "LT1441M.h"

byte ReadPgmData(const byte* ptr) {  // note this is a static function
	return pgm_read_byte(ptr);
}

// Constructors ////////////////////////////////////////////////////////////////
// Function that handles the creation and setup of instances


LT1441M::LT1441M(byte gsi_pin, byte gaeo_pin, byte latch_pin, byte clock_pin, byte raeo_pin, byte rsi_pin){
	// transfer pin setting to internal's
	_gsi_pin = gsi_pin;
	_gaeo_pin = gaeo_pin;
	_latch_pin = latch_pin;
	_clock_pin = clock_pin;
	_raeo_pin = raeo_pin;
	_rsi_pin = rsi_pin;
 
	normal();
	
	// clear frameBuffer
	clrScreen();
	// clear line buffers
	for (byte i = 0; i < msgNum; i++){
		clrLine(i);
	}
	_currentLines = 0;
	
	FontRead = ReadPgmData;
	
} //end LT1441M::LT1441M(byte gsi_pin, byte gaeo_pin, byte latch_pin, byte clock_pin, byte raeo_pinn, byte rsi_pin)


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
	digitalWrite(_gaeo_pin, HIGH);				// green off by default
	digitalWrite(_latch_pin, HIGH);
	digitalWrite(_clock_pin, LOW);
	digitalWrite(_raeo_pin, HIGH);				// red off by default
	digitalWrite(_rsi_pin, LOW);
	
	
} //end void LT1441M::begin(void)

void LT1441M::clrScreen(void){					// clears the screen and returns cursor to (0,0)
	// clear frameBuffer
	for (byte i = 0; i < dPage; i++) {
		memset(frameBuffer[i], 0, sizeof(frameBuffer[i]));
		frameOffset[i] = 0;
	}
#ifdef DEBUG_PRINT
	Serial.println("clrScreen");
#endif	
	
} //end void LT1441M::clrScreen(void)

void LT1441M::clrPage(byte page){				// Clear a single page and return cursor to (0, page)
	memset(frameBuffer[page], 0, sizeof(frameBuffer[page]));
	frameOffset[page] = 0;
#ifdef DEBUG_PRINT
	Serial.print("clrPage page: ");
	Serial.println(page, DEC);
#endif							// return cursor to (0, page)
} //end void LT1441M::clrPage(byte page)

void LT1441M::clrLine(byte line){								// clear msg[line] buffer 
	// ***LWK*** so need to clr framebuffer as well 12/07
	memset(msg[line].buffer, 0, sizeof(msg[line].buffer));
	msg[line].position = 0;
	msg[line].offset = 0;
	msg[line].length = 0;
	msg[line].flags = 0;
	msg[line].sDelay = sDelayDefault;
	msg[line].tDelay = tDelayDefault;
	msg[line].sTimeout = 0;
	msg[line].tTimeout = 0;
	msg[line].fontColor = BLACK;
	msg[line].font = DEFAULT_FONT;
	_currentLines &= ~(1 << line);
	if (line == 1 || line == 3)
		msg[line].flags |= PAGE_1;
#ifdef DEBUG_PRINT
	Serial.print("clrLine:");
	Serial.println(line, DEC);
#endif	
} //end void LT1441M::clrLine(byte line)

void LT1441M::setLine(byte line, char* txt, byte show, byte scroll, byte scrollOn, byte dir, byte page/*, const uint8_t* font, byte color*/){
	// check we have room for the string, terminate short otherwise
	if (strlen(txt) > DMSG-1) {
		txt[DMSG-1] = '\0';					// terminate string early
#ifdef DEBUG_PRINT
		Serial.print("setLine Overflow txt: ");
		Serial.println(txt);
#endif	
	}
	
	// copy string into buffer
	strcpy(msg[line].buffer, txt);

/*
	msg[line].fontColor = color;
	msg[line].font = font;
*/	
	// need to calc .length in px based on font
	calcLen(line);
	
	// reset positions and show line
	msg[line].position = 0;
	msg[line].offset = 0;
	if (page == 1 || (page == 10 && (line == 1 || line == 3))) {
			msg[line].flags |= PAGE_1;
	} else /*if (page == 0 || PAGE == 10) */{
		msg[line].flags &= ~PAGE_1;
	} 

	// if length is over frameBuffer width auto scroll?
	if (scroll || msg[line].length > dxRAM) {
		msg[line].flags |= SCROLL_LINE;
		msg[line].sDelay = sDelayDefault;
	} else {
		msg[line].flags &= ~SCROLL_LINE;
	}

	if (scrollOn) {
		msg[line].flags |= SCROLL_ON;
		msg[line].sDelay = sDelayDefault;
	} else {
		msg[line].flags &= ~SCROLL_ON;
	}
	if (scroll || scrollOn) {
		scrollDir(line, dir);
	}
	
	// copy new line to buffer??
	if (show) {
		msg[line].flags |= SHOW_LINE;
		_currentLines |= (1 << line);
		fillBuffer(line);
	}
	
#ifdef DEBUG_PRINT
	Serial.print("setLine Line:");
	Serial.print(line, DEC);
	Serial.print(" Txt:");
	Serial.print(txt);
	Serial.print(" strlen:");
	Serial.print(strlen(txt), DEC);
	Serial.print(" Buffer:");
	Serial.print(msg[line].buffer);
	Serial.print(" strlen:");
	Serial.print(strlen(msg[line].buffer), DEC);
	Serial.print(" Length:");
	Serial.print(msg[line].length);
	Serial.print(" Flags:");
	Serial.print(msg[line].flags, BIN);
	Serial.print(" Scroll:");
	Serial.print(scroll, DEC);
	Serial.print(" Scroll On:");
	Serial.println(scrollOn);
#endif
} //end void LT1441M::setLine(byte line, char *txt)

void LT1441M::showLine(byte line){
	msg[line].flags |= SHOW_LINE;
	_currentLines |= (1 << line);
#ifdef DEBUG_PRINT
	Serial.print("showLine: ");
	Serial.println(line, DEC);
#endif	
} //end void LT1441M::showLine(byte line)

void LT1441M::hideLine(byte line){
	msg[line].flags &= ~SHOW_LINE;
	_currentLines &= ~(1 << line);
	// ***LWK*** need to clear matching msg[line].flags PAGE??? 11/07
	// ***LWK*** dont think so as there is not clean its either set or not 12/07
#ifdef DEBUG_PRINT
	Serial.print("hideLine: ");
	Serial.println(line, DEC);
#endif	
} //end void LT1441M::hideLine(byte line)

void LT1441M::scrollStart(byte line){ 
	msg[line].flags |= SCROLL_LINE;
#ifdef DEBUG_PRINT
	Serial.print("scrollStart Line: ");
	Serial.println(line, DEC);
#endif	
} //end void LT1441M::scrollStart(byte line)

void LT1441M::scrollStop(byte line){	
	msg[line].flags &= ~SCROLL_LINE;
#ifdef DEBUG_PRINT
	Serial.print("scrollStop Line: ");
	Serial.println(line, DEC);
#endif	
} //end void LT1441M::scrollStop(byte line)		

void LT1441M::scrollDelay(byte line, int delay){ 
	msg[line].sDelay = delay;
#ifdef DEBUG_PRINT
	Serial.print("scrollDelay Line: ");
	Serial.print(line, DEC);
	Serial.print(" Delay: ");
	Serial.println(delay, DEC);
#endif	
} //end void LT1441M::scrollDelay(byte line, int delay=sDelayDefault)

void LT1441M::scrollDir(byte line, byte dir){
	if (dir == SCROLL_LEFT) {
		msg[line].flags |= SCROLL_NEG;
	} else {
		msg[line].flags &= ~SCROLL_NEG;
	}
#ifdef DEBUG_PRINT
	Serial.print("scrollDir Line: ");
	Serial.print(line, DEC);
	Serial.print(" Dir: ");
	Serial.println(dir, DEC);
#endif	
} //end void LT1441M::scrollDir(byte line, byte dir)

void LT1441M::toggleStart(byte line1, byte line2, int delay){
	// set to show the first line
	_currentLines |= (1 << line1);
	_currentLines &= ~(1 << line2);
	// enable showing of both and seyt toggle 
	msg[line1].flags |= SHOW_LINE | TOGGLE_LINE;
	msg[line2].flags |= SHOW_LINE | TOGGLE_LINE;
	//set which line to switch back to after time out
	msg[line1].flags |= (line2 << 5);
	msg[line2].flags |= (line1 << 5);
	// set switch dealys
	msg[line1].tDelay = delay;
	msg[line2].tDelay = delay;

	// update buffer with line1??
	// or assume its all ready there
	// TODO

	
	// start time out for first line
	msg[line1].tTimeout = millis();
#ifdef DEBUG_PRINT
	Serial.print("toggleStart Line1: ");
	Serial.print(line1, DEC);
	Serial.print(" Line2: ");
	Serial.print(line2, DEC);
	Serial.print(" Delay: ");
	Serial.println(delay, DEC);
#endif
} //end void LT1441M::toggleStart(byte line1, byte line2, int delay=tDelayDefault)

void LT1441M::toggleStop(byte line1, byte line2){
	msg[line1].flags &= ~(TOGGLE_MASK|TOGGLE_LINE);
	msg[line2].flags &= ~(TOGGLE_MASK|TOGGLE_LINE|SHOW_LINE);
	
#ifdef DEBUG_PRINT
	Serial.print("toggleStart Line1: ");
	Serial.print(line1, DEC);
	Serial.print(" Line2: ");
	Serial.println(line2, DEC);
#endif
} //end LT1441M::toggleStop(byte line1, byte line2)
/*
void LT1441M::write(byte c){					// write single ASCII char to display
	if (c == 0x0D){								// check for CR '\r'
		setCol(0);
	}else if(c == 0x0A){						// check for LF		'\n' increase based on current font height
		setPage(coord.page + (FontRead(Font+FONT_HEIGHT)+7)/8);
	}else{
		putChar(c);								// output char
	}
} //end void LT1441M::write(byte c)

void LT1441M::setCursor(byte x, byte y){		// set frameBuffer postion for write
	if (x > xMost){								// check to make sure not off right edge of screen and
		x=0;									// reset start of screen
	}
	coord.x = x;								// set coord to match
	byte page;
	if (y > yMost){								// check to make sure not off bottom edge of screen and
		page = 0;								// reset start of screen
	}else{
		page = y / 8;							// ***LWK***
	}
	coord.page = page;							// set coord to match
	coord.y = y;
} //end void LT1441M::setCursor(byte x, byte y)

void LT1441M::setPage(byte page){				// set frameBuffer page for write
	if (page > pageLast){						// check to make sure not off bottom edge of screen and
		page=0;									// reset start of screen
	}

	coord.page = page;							// set coord to match
	coord.y = page * 8;							// set coord.y to first row in page
} //end void LT1441M::setPage(byte page)

void LT1441M::setCol(byte x){					// set frameBuffer col for write
	if (x > xMost){							// check to make sure not off right edge of screen and
		x=0;									// reset start of screen
	}
	coord.x = x;								// set coord to match
} //end void LT1441M::setCol(byte x)

void LT1441M::setRow(byte y){					// set frameBuffer row for write
	byte page;
	if (y > yMost){								// check to make sure not off bottom edge of screen and
		page = 0;								// reset start of screen
		y = 0;
	}else{
		page = y / 8;							// 16 pixels, y is  0-7 = 0, 8-15 = 2, 16> =0
	}
	coord.page = page;							// set coord to match
	coord.y = y;
} //end void LT1441M::setRow(byte y)
*/
void LT1441M::invert(void){						// inverts the display
	_invert = 1;
#ifdef DEBUG_PRINT
	Serial.println("invert");
#endif
} //end void LT1441M::invert(void)

void LT1441M::normal(void){						// restore normal operation after invert
	_invert = 0;
#ifdef DEBUG_PRINT
	Serial.println("non-invert");
#endif
} //end void LT1441M::normal(void)

void LT1441M::selectFont(byte line, const uint8_t* font,byte color) {
	msg[line].fontColor = color;
	msg[line].font = font;
} //end void LT1441M::selectFont(const byte* font,byte color, FontCallback callback)

void LT1441M::loop(){											// update the display 

	updateToggle();
	updateScroll();
	output();

} //end void LT1441M::update() 	

void LT1441M::enable(){													// turn on display
	digitalWrite(_gaeo_pin, LOW);			//enable green
	digitalWrite(_raeo_pin, LOW);			//enable red
} //end void LT1441M::enable()
	
void LT1441M::disable(){												// turn off display
	digitalWrite(_gaeo_pin, HIGH);			//disable green
	digitalWrite(_raeo_pin, HIGH);			//disable red
} //end void LT1441M::disable()



// Private Methods /////////////////////////////////////////////////////////////
// Functions only available to other functions in this library

void LT1441M::write_d(byte ins_c, byte page){		// write date to framebuffer
	// ***LWK*** add scrollDIR check, cant check here not working with msg[line] may need write_n
	frameBuffer[page][frameOffset[page]] = ins_c;
	
	if (frameOffset[page] == xMost){
		frameOffset[page] = 0;
	}else {
		frameOffset[page]++;
	}
	
#ifdef DEBUG_PRINT1
	Serial.print("(");
	Serial.print(frameOffset[page]-1, DEC);
	Serial.print(",");
	Serial.print(page, DEC);
	Serial.print(",");
	Serial.print(ins_c, HEX);
	Serial.println(")");
#endif	
} //end void LT1441M::write_d(byte ins_c)

void LT1441M::shiftOutDual(byte val){
	byte i;
	
	if (_invert) {
		val = ~val;
	}
	
	for (i = 0; i < 8; i++)  {
		digitalWrite(_rsi_pin, !!(val & (1 << i)));
		digitalWrite(_gsi_pin, !!(val & (1 << i)));  
		
		digitalWrite(_clock_pin, HIGH);
		digitalWrite(_clock_pin, LOW);		
	}
} //end void LT1441M::shiftOut(byte val)

void LT1441M::output() { 
	for (int i = 0; i < dxScreen; i++) {
		for (byte j = 0; j <dPage; j++) {
			int o = frameOffset[j] + i;
			if (o > xMost) {
				o = o - dxRAM;
			}
			
			shiftOutDual(frameBuffer[j][o]); 
		}
	}
	
	digitalWrite(_latch_pin, LOW);
	digitalWrite(_latch_pin, HIGH); 
	
} //end void LT1441M::output()

void LT1441M::updateToggle(){
	// TODO
	// for each _currentlines 
	// if toggle set && .tTimeout
	// change _currentlines to toggle next mask
	// if old font 8 new font 16 clear other _currentline
	// else if old font 16 and new font 8 set _currentline for other lines if show
	// fillbuffer() // wont preserv scroll yet
	// start .tTimeout
	for (byte line=0; line < dPage+1; line++) {
		if (_currentLines & 1 << line) {
			if ((msg[line].flags & TOGGLE_LINE) == TOGGLE_LINE && (millis() - msg[line].tTimeout) > msg[line].tDelay) {
				byte line2 = (msg[line].flags & TOGGLE_MASK) >> 6;
				_currentLines &= ~(1 << line);
				if (FontRead(msg[line2].font+FONT_HEIGHT) > FontRead(msg[line].font+FONT_HEIGHT) ) {
					_currentLines = (1 << line2);
				} else if (FontRead(msg[line2].font+FONT_HEIGHT) < FontRead(msg[line].font+FONT_HEIGHT)) {
					_currentLines |= (1 << line2);
					for (byte i=0; i < dPage+1; i++) {
						if (i != line && i != line2 && (msg[i].flags & SHOW_LINE) == SHOW_LINE) {
							_currentLines |= (1 << i);
						}
					}
				}
				fillBuffer(line2);
				msg[line2].tTimeout = millis();
			}
		}
	}
} //end void LT1441M::updateToggle()

void LT1441M::updateScroll(){
	// TODO
	// for each _currentline
	// if scroll && .sTimeout
	// putOne
	// else if scrollOn && .sTimeout && .position == strlen(msg[line].buffer) && .offest != dxRAM - .length
	// putOne
	for (byte line=0; line < dPage+1; line++) {
		if (_currentLines & 1 << line) {
			if ((msg[line].flags & SCROLL_LINE) == SCROLL_LINE && (millis() - msg[line].sTimeout) > msg[line].sDelay) {
				putOne(line);
			} else if ((msg[line].flags & SCROLL_ON) == SCROLL_ON && (millis() - msg[line].sTimeout) > msg[line].sDelay) {
				if (msg[line].position != strlen(msg[line].buffer) ) {
					putOne(line);
				} else if (msg[line].offset != dxRAM - msg[line].length) {
					putOne(line);
				}
			}
		}
	}
	
} //end void LT1441M::Scroll()


void LT1441M::calcLen(byte line){
	msg[line].length = 0;

	byte firstChar = FontRead(msg[line].font+FONT_FIRST_CHAR);
	byte charCount = FontRead(msg[line].font+FONT_CHAR_COUNT);
	
	for (int s=0; s < strlen(msg[line].buffer); s++) {
		byte c = msg[line].buffer[s];
		if(c < firstChar || c >= (firstChar+charCount)) {
			continue;
		}
		c-= firstChar;
		
		if( FontRead(msg[line].font+FONT_LENGTH) == 0 && FontRead(msg[line].font+FONT_LENGTH+1) == 0) {
			// zero length is flag indicating fixed width font (array does not contain width data entries)
			msg[line].length += FontRead(msg[line].font+FONT_FIXED_WIDTH); 
		}
		else{
			// variable width font, read width data
			msg[line].length += FontRead(msg[line].font+FONT_WIDTH_TABLE+c);
		}
		
		// last but not least, add 1px space between chars
		msg[line].length++;
		
	} // end for string
} //end void LT1441M::calcLen(byte line)

void LT1441M::fillBuffer(byte line){
	byte page = (msg[line].flags & PAGE_1) >> 4;
	clrPage(page);
	if ((msg[line].flags & SCROLL_ON) == SCROLL_ON) {
		// wipe buffer and add 1px col at end to start scroll
		// ***LWK*** add scrollDIR check
		putOne(line);
	} else {
		// fill all dxRAM
		if (msg[line].length > dxRAM) {
			Serial.println("LWK2");
			putFull(line);
		} else {
			Serial.println("LWK1");
			for (int i=0; i < strlen(msg[line].buffer); i++) {
				putChar(line);
			}
			// start scrollBuffer for remaining screen space
			// above for loop results in .position == '\0' and .offset == 0
			// so setup .offset with rest of screen space 
			msg[line].offset = dxRAM - msg[line].length;
			frameOffset[page] = 0; 
		}

	}
	output();
} //end void LT1441M::fillBuffer(byte page, byte line)

void LT1441M::putOne(byte line){
	byte page = (msg[line].flags & PAGE_1) >> 4;
//	byte x = frameOffset[page];
	
	byte width = 0;
	byte height = FontRead(msg[line].font+FONT_HEIGHT);
	byte bytes = (height+7)/8;
	
	byte firstChar = FontRead(msg[line].font+FONT_FIRST_CHAR);
	byte charCount = FontRead(msg[line].font+FONT_CHAR_COUNT);
	
	unsigned int index = 0;
	
	// check position if end of string scroll for scrollBuffer before starting again
	// still need to know bytes to add blank px for full height,  and update offsets
	// if at string terminator
	if (msg[line].position == strlen(msg[line].buffer)){
		// write blank px to screen of offset
		for(byte i=0; i<bytes; i++) {
			// 1px gap 
			if(msg[line].fontColor == BLACK) {
				write_d(0x00, page);
			} else {
				write_d(0xFF, page);
			}
			
			if (page == pageLast) {
				page = 0;
			} else {
				page++;
			}
		}
		if (msg[line].offset < scrollBuffer) {
			msg[line].offset++;
		} else {
		    // scrolled out blank to width of scrollBuffer, now rest position and offset for next call
			msg[line].position = 0;
			msg[line].offset = 0;
		}

	} else {	
		byte c = msg[line].buffer[msg[line].position];
		
		if(c < firstChar || c >= (firstChar+charCount)) {
			// skip unknown char
			msg[line].position++;
#ifdef DEBUG_PRINT
			Serial.print("putOne Unknown char in Line: ");
			Serial.print(line, DEC);
			Serial.print(" Position: ");
			Serial.println(msg[line].position-1);
#endif
			return;
		}
		c-= firstChar;
		
		if( FontRead(msg[line].font+FONT_LENGTH) == 0 && FontRead(msg[line].font+FONT_LENGTH+1) == 0) {
			// zero length is flag indicating fixed width font (array does not contain width data entries)
			width = FontRead(msg[line].font+FONT_FIXED_WIDTH); 
			index = c*bytes*width+FONT_WIDTH_TABLE;
		}
		else{
			// variable width font, read width data, to get the index
			for(byte i=0; i<c; i++) {  
				index += FontRead(msg[line].font+FONT_WIDTH_TABLE+i);
			}
			index = index*bytes+charCount+FONT_WIDTH_TABLE;
			width = FontRead(msg[line].font+FONT_WIDTH_TABLE+c);
		}
		
		byte o = msg[line].offset;
		// last but not least, draw the character
		for(byte i=0; i<bytes; i++) {
			
			byte p = i*width;
			if (o < width ) {
				byte data = FontRead(msg[line].font+index+p+o);
				
				if(height > 8 && height < (i+1)*8) {
					data >>= (i+1)*8-height;
				}
				
				if(msg[line].fontColor == BLACK) {
					write_d(data, page);
				} else {
					write_d(~data, page);
				}
				// update msg[] pointer just on first loop
				if (i == 0) 
					msg[line].offset++;
			} else {
				// 1px gap between chars
				if(msg[line].fontColor == BLACK) {
					write_d(0x00, page);
				} else {
					write_d(0xFF, page);
				}
				// update msg[] pointer just on first loop
				if (i == 0){
					msg[line].position++;
					msg[line].offset = 0;
				}
			}
			if (page == pageLast) {
				page = 0;
			} else {
				page++;
			}
		}
		// update offset here after using in the loop
	}
	msg[line].sTimeout = millis();
} //end void LT1441M::putOne(byte line)

void LT1441M::putFull(byte line){
	byte page = (msg[line].flags & PAGE_1) >> 4;
//	byte x = frameOffset[page];
	byte space = dxRAM;
	
	byte width = 0;
	byte height = FontRead(msg[line].font+FONT_HEIGHT);
	byte bytes = (height+7)/8;
	
	byte firstChar = FontRead(msg[line].font+FONT_FIRST_CHAR);
	byte charCount = FontRead(msg[line].font+FONT_CHAR_COUNT);
	
	do {
		unsigned int index = 0;
		
		byte c = msg[line].buffer[msg[line].position];
		
		if(c < firstChar || c >= (firstChar+charCount)) {
			// skip unknown char
			msg[line].position++;
	#ifdef DEBUG_PRINT
			Serial.print("putFull Unknown char in Line: ");
			Serial.println(line, DEC);
	#endif
			continue;
		}
		c-= firstChar;
		
		if( FontRead(msg[line].font+FONT_LENGTH) == 0 && FontRead(msg[line].font+FONT_LENGTH+1) == 0) {
			// zero length is flag indicating fixed width font (array does not contain width data entries)
			width = FontRead(msg[line].font+FONT_FIXED_WIDTH); 
			index = c*bytes*width+FONT_WIDTH_TABLE;
		}
		else{
			// variable width font, read width data, to get the index
			for(byte i=0; i<c; i++) {  
				index += FontRead(msg[line].font+FONT_WIDTH_TABLE+i);
			}
			index = index*bytes+charCount+FONT_WIDTH_TABLE;
			width = FontRead(msg[line].font+FONT_WIDTH_TABLE+c);
		}
		if (width+1 > space) {
			// do something else
			// to fill just the remanind space
			for (int a=0; a < space; a++) {
				putOne(line);
				space--;
			}
			break;
		} else {
			space -= (width+1);
		}
		
		// last but not least, draw the character
		for(byte i=0; i<bytes; i++) {
			byte p = i*width;
			for(byte j=0; j<width; j++) {
				byte data = FontRead(msg[line].font+index+p+j);
				
				if(height > 8 && height < (i+1)*8) {
					data >>= (i+1)*8-height;
				}
				
				if(msg[line].fontColor == BLACK) {
					write_d(data, page);
				} else {
					write_d(~data, page);
				}
			}
			// 1px gap between chars
			if(msg[line].fontColor == BLACK) {
				write_d(0x00, page);
			} else {
				write_d(0xFF, page);
			}
			// update msg[] pointer just on first loop
			if (i == 0){
				msg[line].position++;
				msg[line].offset = 0;
			}
			
			if (bytes > 1 && page == pageLast) {
				page = 0;
			} else if (bytes > 1) {
				page++;
			}
		}
	}while (space != 0);
		
	msg[line].sTimeout = millis();
} //end void LT1441M::putFull(byte line)

void LT1441M::putChar(byte line) {			// Put Char to display handels variable fonts
	byte page = (msg[line].flags & PAGE_1) >> 4;
//	byte x = frameOffset[page];
	
	byte width = 0;
	byte height = FontRead(msg[line].font+FONT_HEIGHT);
	byte bytes = (height+7)/8;
	
	byte firstChar = FontRead(msg[line].font+FONT_FIRST_CHAR);
	byte charCount = FontRead(msg[line].font+FONT_CHAR_COUNT);
	
	unsigned int index = 0;
	
	byte c = msg[line].buffer[msg[line].position];
	
	if(c < firstChar || c >= (firstChar+charCount)) {
		// skip unknown char
		msg[line].position++;
#ifdef DEBUG_PRINT
		Serial.print("putChar Unknown char in Line: ");
		Serial.println(line, DEC);
#endif
		return;
	}
	c-= firstChar;
	
	if( FontRead(msg[line].font+FONT_LENGTH) == 0 && FontRead(msg[line].font+FONT_LENGTH+1) == 0) {
		// zero length is flag indicating fixed width font (array does not contain width data entries)
		width = FontRead(msg[line].font+FONT_FIXED_WIDTH); 
		index = c*bytes*width+FONT_WIDTH_TABLE;
	}
	else{
		// variable width font, read width data, to get the index
		for(byte i=0; i<c; i++) {  
			index += FontRead(msg[line].font+FONT_WIDTH_TABLE+i);
		}
		index = index*bytes+charCount+FONT_WIDTH_TABLE;
		width = FontRead(msg[line].font+FONT_WIDTH_TABLE+c);
	}
	
	
	// last but not least, draw the character
	for(byte i=0; i<bytes; i++) {
		byte p = i*width;
		for(byte j=0; j<width; j++) {
			byte data = FontRead(msg[line].font+index+p+j);
			
			if(height > 8 && height < (i+1)*8) {
				data >>= (i+1)*8-height;
			}
			
			if(msg[line].fontColor == BLACK) {
				write_d(data, page);
			} else {
				write_d(~data, page);
			}
		}
		// 1px gap between chars
		if(msg[line].fontColor == BLACK) {
			write_d(0x00, page);
		} else {
			write_d(0xFF, page);
		}
		// update msg[] pointer just on first loop
		if (i == 0){
			msg[line].position++;
			msg[line].offset = 0;
		}
		
		if (bytes > 1 && page == pageLast) {
			page = 0;
		} else if (bytes > 1) {
			page++;
		}
	}
	
	msg[line].sTimeout = millis();
} //end void LT1441M::putChar(byte c)
