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
    enum screen_mode_t {SCREEN_NOW_NEXT, SCREEN_BANNER};
    void show_text();
    void set_screen(screen_mode_t mode);
    MatrixText *_mt_now_time , *_mt_now_name ; // MatrixText strings
    MatrixText *_mt_next_time, *_mt_next_name;
    MatrixText *_mt_banner;
    char _banner_msg[100];
    char _now_time  [  6];
    char _next_time [  6];
    char _now_name  [128];
    char _next_name [128];
    screen_mode_t _mode;
    
};

#endif
