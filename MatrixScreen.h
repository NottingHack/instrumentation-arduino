/*
  MatrixScreen.h 
*/
#ifndef MatrixScreen_h
#define MatrixScreen_h

#define __STDC_LIMIT_MACROS
#include <stdint.h>

typedef unsigned char byte;
typedef void (*set_xy_fuct)(uint16_t,byte,byte);




class MatrixScreen
{
  public:
    //MatrixScreen() ;
    
    virtual void init()  = 0;
    virtual bool loop()  = 0;

  protected:
    set_xy_fuct _set_xy;
    uint16_t _size_x;
    uint16_t _size_y;

};

#endif