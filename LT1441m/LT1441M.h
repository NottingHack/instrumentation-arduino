/****************************************************	
 * file = LT1441M.h
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
 * The LT1441M has a 7pin connector for the daisy chained date lines 
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


// ensure this library description is only included once
#ifndef  LT1441M_h
#define  LT1441M_h

//Uncomment for debug prints
#define DEBUG_PRINT1

// include types & constants of Wiring core API
#include "WProgram.h"
#include <avr/pgmspace.h>

// include default fonts
// #include "Var8.h"
#include "Tekton.h"
#include "SystemFont5x8.h"


#define DMSG	141				// number of char's for msg buffer length including '\0' max 256

#define DEFAULT_FONT	System5x8
#define SCROLL_RIGHT	0
#define SCROLL_LEFT		1




// typedef for msg buffers
// 164 bytes per buffer
typedef struct {
	char buffer[DMSG];			// actual message buffer
	int position;				// current char pointer
	byte offset;				// px offset in current char
	int length;					// current string length but px of font
	unsigned int flags;			// scroll and toggle flags (scroll, scrollDir, show, toggle, 
	unsigned int sDelay;					// scroll delay
	unsigned int tDelay;					// toggle delay
	unsigned long sTimeout;		// scroll delay timeout
	unsigned long tTimeout;		// toggle delay timeout	
	byte	fontColor;
	const uint8_t* font;
} messageBuffer	;

// typedef to hold cursor location for class
typedef struct {
	unsigned char x;
	unsigned char y;
	unsigned char page;
} lcdCoord;

typedef byte (*FontCallback)(const byte*);

byte ReadPgmData(const byte* ptr);	//Standard Read Callback

class LT1441M {

private:
// Private Members /////////////////////////////////////////////////////////////
// Member variables only available to other functions in this library

	FontCallback	    FontRead;
	byte				FontColor;
	const uint8_t*		Font;


	// pin variables
	byte _gsi_pin;						    // GSI       Serial data yellow-green
	byte _gaeo_pin;						// /GAEO     Output enable for yellow-green
	byte _latch_pin;						// LATCH     Latch contents of shift register 
	byte _clock_pin;						// CLOCK     Clock Signal for data (read on L->H)
	byte _raeo_pin;						// /RAEO     Output enable for red
	byte _rsi_pin;							// RSI       Serial data red
	
	// bit mask fort msg[].flags
	enum{
		SHOW_LINE		= 1,					
		SCROLL_LINE		= 2,
		SCROLL_NEG		= 4,
		SCROLL_ON		= 8,					// scroll message onto screen but stop if not also scroll_line
		PAGE_1			= 16,					// show on page 0 or 1
		TOGGLE_LINE		= 32,					// toggle after timeout?
		TOGGLE_MASK		= 448,					// bits 6,7,8 used to hold next line number to switch to 0,1,2,3,4,5,6,7
	};
	
	enum{										// Enum to hold static values
	// Colors
		BLACK				= 0xFF,
		WHITE				= 0x00,
		RED					= 1,
		GREEN				= 2,
	// Font Indices
		FONT_LENGTH			= 0,
		FONT_FIXED_WIDTH	= 2,
		FONT_HEIGHT			= 3,
		FONT_FIRST_CHAR		= 4,
		FONT_CHAR_COUNT		= 5,
		FONT_WIDTH_TABLE	= 6,

	/**  coordinate system
	*	upper left corner is (0,0)
	*	positive x is to the right
	*	positive y is down
	*  variable names
	*	dx = "delta x" (or "difference x") which is width
	*	dy = "delta y" which is height
	*/

	/* geometry of the screen and memory */
		dxScreen	= 128,				// number of columns on screen ***LWK*** 128 for finally config of 1x8 panels
		dyScreen	= 16,				// number of rows on screen
		dxRAM		= 128,				// number of cols stored in frameBuffer (including off-screen area)
		dyRAM		= 16,				// number of rows stored in frameBuffer (including off-screen area)
		dPage		= dyRAM / 8, // = 2				// number of pages in frameBuffer (use row height) max 2(.flags & PAGE_1) HARDMax 256 for (byte
		
	/* 'last' values are largest values that are visible on the screen */
		xLast		= dxScreen-1,
		yLast		= dyScreen-1,
	/* 'most' values are largest valid values (e.g. for drawing) */
		xMost		= dxRAM-1,
		yMost		= dyRAM-1,
		pageLast	= dPage-1,
		msgNum		= 8,				// total number of line buffers MAX 8(TOGGLE_MASK) HARDMax 256 (byte)
		scrollBuffer = dxScreen-1,		// number of blank col's between end of scroll and start again
		
		sDelayDefault = 5,
		tDelayDefault = 60000,
	};	// end enum

	byte frameBuffer[dPage][dxRAM];
	int frameOffset[dPage];
	// int frameX[dPage];			//not sure if i may still need to track X
	byte _invert;					// invert frameBuffer on output 0 = normal 1 = invert
	byte _currentLines;				// tracks which lines are on show as bit mask, in theory max of dPage bits set at any time
	
	messageBuffer msg[msgNum];

// Private Methods /////////////////////////////////////////////////////////////
// Functions only available to other functions in this library

	void write_d(byte ins_c, byte page);			// write date to frameBuffer
	void shiftOutDual(byte val);		// Custom Shift Out routine
	void output();						// output the full frameBuffer using frameOffset's
	void updateToggle();				// update frameBuffer if a line toggle is needed
	void updateScroll();				// update frameBuffer based on current scroll flags per msg[line]
	void calcLen(byte line);			// calculate the length in px of msg[line].buffer, stores in msg[line].length so no return
	void fillBuffer(byte line);			// used to fill new buffer after setLine()
	void putOne(byte line);				// used to put one more col of char onto frame buffer
	void putFull(byte line);			// used to put full char onto frame buffer
	void putChar(byte line);				// Put Char to display handels variable fonts
	
public:
	
// Public Methods //////////////////////////////////////////////////////////////
// Functions available in Wiring sketches, this library, and other libraries

	/**
	*  Construct to createCurrently LT1441M class 
	*
	*  Pass Pin Assignments via....
	*  LT144M myMatrix(GIS_pin, GAEO_pin, LATCH_pin, CLOCK_pin, RAEO_pin, RSI_pin)
	*/
	LT1441M(byte gsi_pin, byte gaeo_pin, byte latch_pin, byte clock_pin, byte raeo_pin, byte rsi_pin); 

	/**
	*  Setup Arduino and LT1441M for use
	*
	*/
	void begin(void);

	// General Functions That Currently Work
	void clrScreen(void);									// clears the screen and returns cursor to (0,0)
	void clrPage(byte page);								// Clear a single page and return cursor to (0,page)
	void clrLine(byte line);								// clear msg[line] buffer 
	
	void setLine(byte line, char* txt, byte showNow=0, byte scroll=0, byte scrollOn=0, byte dir=SCROLL_RIGHT, byte page=10/*, const uint8_t* font=DEFAULT_FONT, byte color=BLACK*/);	// set a line of text
	void showLine(byte line);								// 
	void hideLine(byte line);								// 
	void scrollStart(byte line);							// 
	void scrollStop(byte line);								// 
	void scrollDelay(byte line, int delay=sDelayDefault);	// 
	void scrollDir(byte line, byte dir);					// use SCROLL_LEFT and SCROLL_RIGHT for dir
	void toggleStart(byte line1, byte line2, int delay=tDelayDefault);	// setup toggle between two line starting with line1, setLine() for both first 
	void toggleStop(byte line1, byte line2);										// stops toggle and leaves line1 showing 
	void cascadeSetup(byte line1, byte line2, int delay=tDelayDefault);	// setup toggle between two line starting with line1, setLine() for both first 
	void cascadeStart(byte line);
	// ***LWK*** 3/2/12 posible TODO
	/*
	void cascadeStop();										// stop all or just one based in a line in that series
	void cascadeRemove(bŷte line);							// remove a cascade to the next line
	*/
/*	
	void write(byte);										// write to display
	
	// no longer needed??

	void setCursor(byte x, byte y);							// set frameBuffer postion for write
	void setPage(byte page);								// set frameBuffer page for write
	void setCol(byte x);									// set frameBuffer col for write
	void setRow(byte y);									// set frameBuffer row for write
*/	
	void invert(void);										// inverts the display
	void normal(void);										// restore normal operation after invert
	

	void selectFont(byte line, const uint8_t* font=DEFAULT_FONT, byte color=BLACK); // set font per msg[line]

	void loop(void);											// update the display
	void enable(void);											// turn on display
	void disable(void);											// turn off display
	
	// Most of these to be done
	void rest(void);										// reset display 
	// shift the display so the specified row is at the top of the screen 
	void rowOffset(byte b);
	/* set the start line for the display; the code in this project uses
	this property to adjust so that the first row displayed is row 0;
	so, you probably want to use SetRowOffset for a scrolling effect */


	
// Public Const Members //////////////////////////////////////////////////////////////
// Member variables available in Wiring sketches, this library, and other libraries
	lcdCoord coord;								// cursor location
	
};



#endif


