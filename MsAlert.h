#ifndef MSALERT_H
#define MSALERT_H

#include "MatrixScreen.h"
#include "MatrixText.h"
#include <Arduino.h>
#include <ArduinoJson.h>


class MsAlert : public MatrixScreen
{
  public:
     MsAlert(set_xy_fuct set_xy, uint16_t size_x, uint16_t size_y);
     ~MsAlert();
     void init();
     bool loop();
     

  private:
    // void show_text();
    MatrixText *_mt_top , *_mt_bottom;
    void show_text();
    void draw_border(uint8_t a);

    char _top_msg[100];
    char _bottom_msg[100];



    
};

#endif
