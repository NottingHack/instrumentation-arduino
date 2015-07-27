#ifndef MSNOWNEXT_H
#define MSNOWNEXT_H

#include "MatrixScreen.h"
#include "MatrixText.h"
#include <Arduino.h>


class MsNowNext : public MatrixScreen
{
  public:
     MsNowNext(set_xy_fuct set_xy, uint16_t size_x, uint16_t size_y);
     ~MsNowNext();
     void init();
     bool loop();

  private:
    MatrixText *_mt_now_time, *_mt_now_name; // MatrixText strings

};

#endif
