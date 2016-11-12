

/*
 *
 * LWK16
 *
 * created with FontCreator
 * written by F. Maximilian Thiele
 *
 * http://www.apetech.de/fontCreator
 * me@apetech.de
 *
 * File Name           : LWK16.h
 * Date                : 02.03.2010
 * Font size in bytes  : 13535
 * Font width          : 0
 * Font height         : 19
 * Font first char     : 32
 * Font last char      : 128
 * Font used chars     : 96
 *
 * The font data are defined as
 *
 * struct _FONT_ {
 *     uint16_t   font_Size_in_Bytes_over_all_included_Size_it_self;
 *     uint8_t    font_Width_in_Pixel_for_fixed_drawing;
 *     uint8_t    font_Height_in_Pixel_for_all_characters;
 *     unit8_t    font_First_Char;
 *     uint8_t    font_Char_Count;
 *
 *     uint8_t    font_Char_Widths[font_Last_Char - font_First_Char +1];
 *                  // for each character the separate width in pixels,
 *                  // characters < 128 have an implicit virtual right empty row
 *
 *     uint8_t    font_data[];
 *                  // bit field of all characters
 */

#include <inttypes.h>
#include <avr/pgmspace.h>

#ifndef LWK16_H
#define LWK16_H

#define LWK16_WIDTH 0
#define LWK16_HEIGHT 19

static uint8_t LWK16[] PROGMEM = {
    0x34, 0xDF, // size
    0x00, // width
    0x13, // height
    0x20, // first char
    0x60, // char count
    
    // char widths
    0x00, 0x02, 0x06, 0x0D, 0x0A, 0x0C, 0x0A, 0x02, 0x05, 0x05, 
    0x07, 0x08, 0x02, 0x05, 0x02, 0x07, 0x09, 0x06, 0x08, 0x07, 
    0x0A, 0x08, 0x08, 0x09, 0x08, 0x08, 0x02, 0x03, 0x05, 0x06, 
    0x05, 0x07, 0x0D, 0x0A, 0x08, 0x08, 0x0A, 0x07, 0x09, 0x0A, 
    0x0A, 0x08, 0x0A, 0x08, 0x07, 0x0D, 0x0B, 0x0B, 0x07, 0x0D, 
    0x09, 0x09, 0x0A, 0x0A, 0x09, 0x0F, 0x0A, 0x09, 0x0A, 0x04, 
    0x06, 0x05, 0x06, 0x0A, 0x03, 0x08, 0x08, 0x07, 0x08, 0x07, 
    0x07, 0x07, 0x07, 0x02, 0x04, 0x07, 0x02, 0x0A, 0x07, 0x07, 
    0x07, 0x07, 0x06, 0x06, 0x06, 0x07, 0x07, 0x0A, 0x08, 0x08, 
    0x07, 0x05, 0x02, 0x06, 0x08, 0x06, 
    
    // font data
    0xFE, 0xFE, 0x6F, 0x6F, 0x00, 0x00, // 33
    0x7C, 0x7C, 0x00, 0x00, 0x7C, 0x7C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 34
    0x00, 0x30, 0x30, 0xF0, 0xFC, 0x3C, 0x30, 0xB0, 0xF0, 0xFC, 0x3C, 0x30, 0x30, 0x06, 0x36, 0x3F, 0x0F, 0x06, 0x06, 0x3E, 0x3F, 0x0F, 0x06, 0x06, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 35
    0xF0, 0xF8, 0x98, 0x8C, 0xFF, 0xFF, 0x8C, 0x8C, 0x08, 0x00, 0x18, 0x38, 0x31, 0x31, 0xFF, 0xFF, 0x31, 0x3B, 0x1F, 0x0E, 0x00, 0x00, 0x00, 0x00, 0x20, 0x20, 0x00, 0x00, 0x00, 0x00, // 36
    0x38, 0x7C, 0x6C, 0x7C, 0x38, 0xC0, 0x70, 0x1C, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x1C, 0x07, 0x01, 0x00, 0x1C, 0x3E, 0x36, 0x3E, 0x1C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 37
    0x00, 0x00, 0x80, 0xF8, 0xFC, 0xEC, 0x3C, 0x18, 0x00, 0x80, 0x0E, 0x1F, 0x39, 0x30, 0x30, 0x31, 0x33, 0x1E, 0x1E, 0x17, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 38
    0x7C, 0x7C, 0x00, 0x00, 0x00, 0x00, // 39
    0xC0, 0xF0, 0x3C, 0x0E, 0x06, 0x1F, 0x7F, 0xE0, 0x80, 0x00, 0x00, 0x00, 0x00, 0x20, 0x20, // 40
    0x06, 0x0E, 0x3C, 0xF0, 0xC0, 0x00, 0x80, 0xE0, 0x7F, 0x1F, 0x20, 0x20, 0x00, 0x00, 0x00, // 41
    0x08, 0xD8, 0x78, 0x3C, 0xF8, 0xD8, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 42
    0x00, 0x00, 0x00, 0xE0, 0xE0, 0x00, 0x00, 0x00, 0x03, 0x03, 0x03, 0x1F, 0x1F, 0x03, 0x03, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 43
    0x00, 0x00, 0xC0, 0x60, 0x00, 0x00, // 44
    0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x03, 0x03, 0x03, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, // 45
    0x00, 0x00, 0x30, 0x30, 0x00, 0x00, // 46
    0x00, 0x00, 0x00, 0x80, 0xE0, 0x7C, 0x1C, 0x60, 0x78, 0x1E, 0x07, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 47
    0xE0, 0xF8, 0x38, 0x0C, 0x0C, 0x0C, 0x1C, 0xF8, 0xE0, 0x07, 0x1F, 0x38, 0x30, 0x30, 0x30, 0x38, 0x1F, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 48
    0x30, 0x18, 0xFC, 0xFC, 0x00, 0x00, 0x30, 0x30, 0x3F, 0x3F, 0x30, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 49
    0x30, 0x38, 0x1C, 0x0C, 0x8C, 0x8C, 0xF8, 0x78, 0x38, 0x3E, 0x36, 0x33, 0x31, 0x31, 0x30, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 50
    0x18, 0x9C, 0x8C, 0x8C, 0xCC, 0x78, 0x78, 0x18, 0x30, 0x31, 0x31, 0x31, 0x3F, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 51
    0x00, 0x00, 0x80, 0xC0, 0x30, 0x18, 0xFC, 0xFC, 0x00, 0x00, 0x06, 0x07, 0x07, 0x06, 0x06, 0x06, 0x3F, 0x3F, 0x06, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 52
    0xFC, 0xFC, 0x6C, 0x6C, 0x6C, 0xEC, 0xCC, 0x8C, 0x19, 0x39, 0x30, 0x30, 0x30, 0x38, 0x1F, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 53
    0x80, 0xE0, 0xF0, 0xB8, 0x9C, 0x8C, 0x80, 0x00, 0x0F, 0x1F, 0x39, 0x31, 0x31, 0x39, 0x1F, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 54
    0x0C, 0x0C, 0x0C, 0x0C, 0x8C, 0xEC, 0x3C, 0x1C, 0x0C, 0x00, 0x00, 0x38, 0x1E, 0x07, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 55
    0x70, 0xF8, 0xCC, 0xCC, 0xCC, 0xCC, 0xF8, 0x30, 0x0F, 0x1F, 0x31, 0x30, 0x30, 0x31, 0x1F, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 56
    0xF0, 0xF8, 0x9C, 0x0C, 0x0C, 0x9C, 0xF8, 0xF0, 0x00, 0x61, 0x63, 0x33, 0x3B, 0x1F, 0x0F, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 57
    0xC0, 0xC0, 0x18, 0x18, 0x00, 0x00, // 58
    0x00, 0xC0, 0xC0, 0x60, 0x70, 0x10, 0x00, 0x00, 0x00, // 59
    0x00, 0x80, 0xC0, 0xE0, 0x60, 0x01, 0x03, 0x06, 0x0C, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, // 60
    0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 61
    0x20, 0x60, 0xC0, 0x80, 0x00, 0x0C, 0x06, 0x06, 0x03, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, // 62
    0x30, 0x38, 0x18, 0x18, 0x38, 0xF0, 0xE0, 0x00, 0x60, 0x6C, 0x04, 0x07, 0x03, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 63
    0xC0, 0xF0, 0x3C, 0x8C, 0xC6, 0x66, 0x26, 0x86, 0x06, 0x0C, 0x1C, 0xF8, 0xE0, 0x0F, 0x1F, 0x78, 0x63, 0xC7, 0xC6, 0xC3, 0xC3, 0xE6, 0x66, 0x06, 0x03, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 64
    0x00, 0x00, 0x00, 0x80, 0xE0, 0x70, 0x18, 0xFC, 0xE0, 0x00, 0x30, 0x3C, 0x0E, 0x03, 0x03, 0x03, 0x03, 0x07, 0x3F, 0x3C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 65
    0xFC, 0xFC, 0x8C, 0x8C, 0xDC, 0xF8, 0x70, 0x00, 0x3F, 0x3F, 0x31, 0x31, 0x31, 0x1B, 0x1F, 0x0E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 66
    0x80, 0xE0, 0x70, 0x18, 0x0C, 0x0C, 0x1C, 0x1C, 0x0F, 0x1F, 0x38, 0x30, 0x30, 0x38, 0x18, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 67
    0xFC, 0xFC, 0x18, 0x18, 0x18, 0x30, 0x30, 0x60, 0xC0, 0x80, 0x1F, 0x3F, 0x30, 0x30, 0x30, 0x30, 0x38, 0x1C, 0x0F, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 68
    0xFC, 0xFC, 0x8C, 0x8C, 0x8C, 0x8C, 0x8C, 0x1F, 0x3F, 0x31, 0x31, 0x31, 0x31, 0x31, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 69
    0xFC, 0xFC, 0x8C, 0x8C, 0x8C, 0x8C, 0x8C, 0x8C, 0x08, 0x3F, 0x3F, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 70
    0x80, 0xE0, 0x70, 0xB8, 0x9C, 0x8C, 0x8C, 0x98, 0x98, 0x80, 0x0F, 0x1F, 0x30, 0x31, 0x31, 0x31, 0x19, 0x1D, 0x0F, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 71
    0xFC, 0xFC, 0x00, 0x00, 0x80, 0x80, 0x80, 0x80, 0xFC, 0xFC, 0x3F, 0x3F, 0x03, 0x03, 0x01, 0x01, 0x01, 0x01, 0x3F, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 72
    0x0C, 0x0C, 0x0C, 0xFC, 0xFC, 0x0C, 0x0C, 0x0C, 0x30, 0x30, 0x30, 0x3F, 0x3F, 0x30, 0x30, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 73
    0x00, 0x00, 0x0C, 0x0C, 0x0C, 0xFC, 0xFC, 0x0C, 0x0C, 0x0C, 0x0E, 0x1E, 0x18, 0x30, 0x30, 0x3F, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 74
    0xFC, 0xFC, 0x80, 0x40, 0x60, 0x30, 0x18, 0x0C, 0x3F, 0x3F, 0x03, 0x06, 0x0C, 0x18, 0x30, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 75
    0xFC, 0xFC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x30, 0x30, 0x30, 0x30, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 76
    0x00, 0x80, 0xF8, 0x3C, 0xFC, 0xC0, 0x00, 0x80, 0xF8, 0x3C, 0xF8, 0x80, 0x00, 0x38, 0x3F, 0x03, 0x00, 0x0F, 0x3F, 0x38, 0x3F, 0x07, 0x00, 0x0F, 0x3F, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 77
    0xFC, 0xFC, 0x30, 0x60, 0xC0, 0x80, 0x00, 0x00, 0x00, 0xFC, 0xF8, 0x3F, 0x3F, 0x00, 0x00, 0x00, 0x03, 0x06, 0x0C, 0x18, 0x3F, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 78
    0xC0, 0xE0, 0x78, 0x18, 0x0C, 0x0C, 0x0C, 0x0C, 0x1C, 0xF8, 0xF0, 0x07, 0x1F, 0x18, 0x30, 0x30, 0x30, 0x30, 0x18, 0x1E, 0x0F, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 79
    0xFC, 0xFC, 0x8C, 0x8C, 0xDC, 0xF8, 0x70, 0x3F, 0x3F, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 80
    0xC0, 0xF0, 0x38, 0x18, 0x1C, 0x0C, 0x0C, 0x0C, 0x0C, 0x1C, 0x38, 0xF0, 0xC0, 0x03, 0x0F, 0x1C, 0x18, 0x30, 0x30, 0x36, 0x3C, 0x18, 0x30, 0x78, 0xDF, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, // 81
    0xFC, 0xFC, 0x0C, 0x0C, 0x18, 0x38, 0xF0, 0xE0, 0x00, 0x3F, 0x3F, 0x06, 0x06, 0x0E, 0x0F, 0x19, 0x30, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 82
    0x00, 0x70, 0xF8, 0xDC, 0xCC, 0xCC, 0xCC, 0x88, 0x00, 0x0C, 0x1C, 0x30, 0x30, 0x30, 0x30, 0x19, 0x1F, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 83
    0x0C, 0x0C, 0x0C, 0x0C, 0xFC, 0xFC, 0x0C, 0x0C, 0x0C, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 84
    0xFC, 0xFC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFC, 0xFC, 0x07, 0x1F, 0x1C, 0x30, 0x30, 0x30, 0x38, 0x1C, 0x0F, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 85
    0x1C, 0xFC, 0xE0, 0x00, 0x00, 0x00, 0xF0, 0xFC, 0x0C, 0x00, 0x01, 0x0F, 0x3E, 0x30, 0x3F, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 86
    0x3C, 0xFC, 0xC0, 0x00, 0x00, 0xC0, 0xF8, 0x1C, 0xFC, 0xC0, 0x00, 0x00, 0xC0, 0xFC, 0x3C, 0x00, 0x03, 0x3F, 0x38, 0x3E, 0x07, 0x01, 0x00, 0x1F, 0x3F, 0x30, 0x3E, 0x0F, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 87
    0x0C, 0x1C, 0x30, 0x60, 0xC0, 0xC0, 0x60, 0x30, 0x1C, 0x0C, 0x30, 0x38, 0x0C, 0x06, 0x03, 0x03, 0x07, 0x0E, 0x38, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 88
    0x0C, 0x3C, 0xF0, 0xC0, 0x00, 0xC0, 0xF0, 0x7C, 0x1C, 0x00, 0x00, 0x20, 0x3B, 0x1F, 0x0F, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 89
    0x0C, 0x0C, 0x0C, 0x8C, 0xCC, 0xEC, 0x7C, 0x3C, 0x1C, 0x0C, 0x38, 0x3C, 0x36, 0x33, 0x31, 0x30, 0x30, 0x30, 0x30, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 90
    0xFE, 0xFE, 0x06, 0x06, 0xFF, 0xFF, 0x80, 0x80, 0x20, 0x20, 0x20, 0x20, // 91
    0x0C, 0x3C, 0xF0, 0x80, 0x00, 0x00, 0x00, 0x00, 0x01, 0x1F, 0x7C, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 92
    0x06, 0x06, 0x06, 0xFE, 0xFE, 0x80, 0x80, 0x80, 0xFF, 0xFF, 0x20, 0x20, 0x20, 0x20, 0x20, // 93
    0x10, 0x1C, 0x0E, 0x06, 0x0C, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 94
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, // 95
    0x06, 0x0C, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 96
    0x00, 0x80, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0x00, 0x1F, 0x3F, 0x31, 0x30, 0x30, 0x1F, 0x3F, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 97
    0xFE, 0xFE, 0x80, 0xC0, 0xC0, 0xC0, 0x80, 0x00, 0x3F, 0x3F, 0x31, 0x30, 0x30, 0x39, 0x1F, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 98
    0x00, 0x80, 0xC0, 0xC0, 0xC0, 0xC0, 0x80, 0x0E, 0x1F, 0x31, 0x30, 0x30, 0x30, 0x19, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 99
    0x00, 0x80, 0xC0, 0xC0, 0xC0, 0xC0, 0xFE, 0xFE, 0x0F, 0x1F, 0x39, 0x30, 0x30, 0x30, 0x3F, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 100
    0x00, 0x80, 0xC0, 0xC0, 0xC0, 0xC0, 0x80, 0x0F, 0x1F, 0x39, 0x34, 0x36, 0x33, 0x19, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 101
    0xC0, 0xC0, 0xF8, 0xFC, 0xCE, 0xC6, 0x06, 0x00, 0x00, 0x3F, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 102
    0x00, 0x80, 0xC0, 0xC0, 0xC0, 0xC0, 0x80, 0x1F, 0x3F, 0x31, 0x30, 0x98, 0xFF, 0x7F, 0x60, 0x60, 0x60, 0x60, 0x60, 0x20, 0x00, // 103
    0xFE, 0xFE, 0x80, 0xC0, 0xC0, 0xC0, 0x80, 0x3F, 0x3F, 0x01, 0x00, 0x00, 0x3F, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 104
    0xCC, 0xCC, 0x3F, 0x3F, 0x00, 0x00, // 105
    0x00, 0x00, 0xCC, 0xCC, 0x00, 0x00, 0xFF, 0xFF, 0xC0, 0xC0, 0xE0, 0x60, // 106
    0xFE, 0xFE, 0x00, 0x00, 0x80, 0xC0, 0x00, 0x3F, 0x3F, 0x06, 0x07, 0x0F, 0x38, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 107
    0xFE, 0xFE, 0x3F, 0x3F, 0x00, 0x00, // 108
    0xC0, 0xC0, 0x80, 0xC0, 0xC0, 0xC0, 0x80, 0xC0, 0xC0, 0x80, 0x3F, 0x3F, 0x01, 0x00, 0x3F, 0x3F, 0x01, 0x00, 0x3F, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 109
    0xC0, 0xC0, 0x80, 0xC0, 0xC0, 0xC0, 0x80, 0x3F, 0x3F, 0x01, 0x00, 0x00, 0x3F, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 110
    0x00, 0x80, 0xC0, 0xC0, 0xC0, 0x80, 0x00, 0x0F, 0x1F, 0x30, 0x30, 0x30, 0x1F, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 111
    0xE0, 0xE0, 0x80, 0xC0, 0xC0, 0xC0, 0x80, 0xFF, 0xFF, 0x30, 0x30, 0x30, 0x1F, 0x0F, 0xE0, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x00, // 112
    0x00, 0x80, 0x80, 0xC0, 0xC0, 0xC0, 0xC0, 0x0F, 0x1F, 0x31, 0x30, 0x30, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x60, // 113
    0xC0, 0xC0, 0x80, 0x40, 0xC0, 0xC0, 0x3F, 0x3F, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 114
    0x00, 0x80, 0x80, 0xC0, 0xC0, 0xC0, 0x13, 0x37, 0x37, 0x36, 0x3E, 0x1C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 115
    0xC0, 0xC0, 0xF8, 0xF8, 0xC0, 0xC0, 0x00, 0x00, 0x3F, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 116
    0xC0, 0xC0, 0x00, 0x00, 0x00, 0xC0, 0xC0, 0x1F, 0x3F, 0x30, 0x30, 0x30, 0x3F, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 117
    0xC0, 0xC0, 0x00, 0x00, 0x00, 0xC0, 0xC0, 0x00, 0x07, 0x1F, 0x38, 0x1F, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 118
    0xC0, 0xC0, 0x00, 0x00, 0xC0, 0xC0, 0x00, 0x00, 0xC0, 0xC0, 0x0F, 0x3F, 0x30, 0x3E, 0x0F, 0x03, 0x3E, 0x30, 0x3F, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 119
    0xC0, 0xC0, 0x80, 0x00, 0x00, 0x80, 0xC0, 0xC0, 0x30, 0x39, 0x1F, 0x0F, 0x0F, 0x1F, 0x39, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 120
    0xC0, 0xC0, 0x80, 0x00, 0x00, 0x00, 0xC0, 0xC0, 0x00, 0x03, 0x8F, 0xFE, 0x78, 0x1E, 0x07, 0x01, 0x00, 0xC0, 0xE0, 0x20, 0x00, 0x00, 0x00, 0x00, // 121
    0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0x30, 0x38, 0x3C, 0x36, 0x33, 0x31, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 122
    0x00, 0xFC, 0xFE, 0x06, 0x06, 0x03, 0x7F, 0xFD, 0x80, 0x80, 0x00, 0x00, 0x20, 0x20, 0x20, // 123
    0xFE, 0xFE, 0xFF, 0xFF, 0x20, 0x20, // 124
    0x06, 0x06, 0xFE, 0xFC, 0x00, 0x00, 0x80, 0x80, 0xFD, 0x7F, 0x07, 0x02, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, // 125
    0x80, 0xC0, 0x60, 0xE0, 0xC0, 0x00, 0x00, 0xC0, 0x03, 0x01, 0x00, 0x00, 0x03, 0x03, 0x03, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 126
    0xFE, 0x06, 0x06, 0x06, 0x06, 0xFE, 0x3F, 0x30, 0x30, 0x30, 0x30, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 // 127
    
};

#endif