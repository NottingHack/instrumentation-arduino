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


// ensure this library description is only included once
#ifndef  LT1441M_h
#define  LT1441M_h

//Uncomment for debug prints
#define DEBUG_PRINT

// include types & constants of Wiring core API
#include "WProgram.h"
#include <avr/pgmspace.h>


// include description files for other libraries used (if any)
#include "Print.h"

// typedef to hold cursor location for class
typedef struct {
	unsigned char x;
	unsigned char y;
	unsigned char page;
} lcdCoord;

typedef uint8_t (*FontCallback)(const uint8_t*);

uint8_t ReadPgmData(const uint8_t* ptr);	//Standard Read Callback

class LT1441M : public Print {

private:
// Private Members /////////////////////////////////////////////////////////////
// Member variables only available to other functions in this library

	FontCallback	    FontRead;
	uint8_t				FontColor;
	const uint8_t*		Font;


	// pin variables
	unsigned char _gsi_pin;						    // GSI       Serial data yellow-green
	unsigned char _gaeo_pin;						// /GAEO     Output enable for yellow-green
	unsigned char _latch_pin;						// LATCH     Latch contents of shift register 
	unsigned char _clock_pin;						// CLOCK     Clock Signal for data (read on L->H)
	unsigned char _raeo_pin;						// /RAEO     Output enable for red
	unsigned char _rsi_pin;							// RSI       Serial data red
	
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
		dxScreen	= 128,				// number of columns on screen ***LWK*** 128 for finaly config of 1x8 panles
		dyScreen	= 16,				// number of rows on screen
		dxRAM		= 350,				// number of cols stored in frameBuffer (including off-screen area) ***LWK*** 840 for 140 chars at 8x6
		dyRAM		= 16,				// number of rows stored in frameBuffer (including off-screen area)
		dPage		= dyRAM / 8, // = 2				// number of pages in frameBuffer (use row height) ***LWK***

	/* 'last' values are largest values that are visible on the screen */
		xLast		= dxScreen-1,
		yLast		= dyScreen-1,
	/* 'most' values are largest valid values (e.g. for drawing) */
		xMost		= dxRAM-1,
		yMost		= dyRAM-1,
		pageLast	= dPage-1,
	};	// end enum

	unsigned char frameBuffer[dPage][dxRAM];
	//unsigned int frameBuffer[dxRAM];
	int offset[dPage];

// Private Methods /////////////////////////////////////////////////////////////
// Functions only available to other functions in this library

	void write_d(unsigned char ins_c);			// write date to frameBuffer
	void shiftOutL(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder, uint8_t val);		// Custom Shift Out routine
	
public:
	
// Public Methods //////////////////////////////////////////////////////////////
// Functions available in Wiring sketches, this library, and other libraries

	/**
	*  Construct to creat LT1441M class 
	*
	*  Pass Pin Assignments via....
	*  LT144M myMatrix(GIS_pin, GAEO_pin, LATCH_pin, CLOCK_pin, RAEO_pin, RSI_pin)
	*/
	LT1441M(unsigned char gsi_pin, unsigned char gaeo_pin, 
		unsigned char latch_pin, unsigned char clock_pin, unsigned char raeo_pin, unsigned char rsi_pin); 

	/**
	*  Setup Arduino and LT1441M for use
	*
	*/
	void begin(void);

	// General Functions That Curently Work
	void clrScreen(void);									// clears the screen and returns cursor to (0,0)
	void clrPage(unsigned char page);						// Clear a single page and return cursor to (0,page)
	virtual void write(unsigned char);						// write to display
	using Print::write;										// pull in write(str) and write(buf, size) from Print
	void setCursor(unsigned char x, unsigned char y);	    // set frameBuffer postion for write
	void setPage(unsigned char page);						// set frameBuffer page for write
	void setCol(unsigned char x);							// set frameBuffer col for write
	void setRow(unsigned char y);							// set freameBuffer row for write
	void invert(void);										// inverts the display
	void normal(void);										// restore normal operation after invert
	int putChar(unsigned char c);							// Put Char to display handels varible fonts
	void selectFont(const uint8_t* font, uint8_t color=BLACK, FontCallback callback=ReadPgmData); // defualt arguments added, callback now last arg

	void update();											// update the display with current frameBuffer
	void enable();											// turn on display
	void disable();											// turn off display
	
	// Most of these to be done
	void rest(void);										// reset display 
	// shift the display so the specified row is at the top of the screen 
	void rowOffset(unsigned char b);
	/* set the start line for the display; the code in this project uses
	this property to adjust so that the first row displayed is row 0;
	so, you probably want to use SetRowOffset for a scrolling effect */
	void startLine(unsigned char b);
	void FindEdges(void);
	void DrawEdges(void);
	void DrawFullRect(void);
	void ScrollDisplay(void);								// Scroll display 
	void DrawCircles(void);
	void DrawColorBars(void);
	void FillScreenPixels(void);
	void updateScroll(void);
	
	
// Public Const Members //////////////////////////////////////////////////////////////
// Member variables available in Wiring sketches, this library, and other libraries
	lcdCoord coord;								// cursor location
	
};



#endif


