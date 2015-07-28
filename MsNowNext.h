#ifndef MSNOWNEXT_H
#define MSNOWNEXT_H

#include "MatrixScreen.h"
#include "MatrixText.h"
#include <Arduino.h>
#include <ArduinoJson.h>

class MsNowNext : public MatrixScreen
{
  public:
     MsNowNext(set_xy_fuct set_xy, uint16_t size_x, uint16_t size_y);
     ~MsNowNext();
     void init();
     bool loop();
     bool process_message(char *json_string);

  private:
    void show_text();
    MatrixText *_mt_now_time , *_mt_now_name ; // MatrixText strings
    MatrixText *_mt_next_time, *_mt_next_name; 
    char _now_time [  6];
    char _next_time[  6];
    char _now_name [128];
    char _next_name[128];
};

#endif
