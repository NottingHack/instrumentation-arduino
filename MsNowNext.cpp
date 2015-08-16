#include "MsNowNext.h"

MsNowNext::MsNowNext(set_xy_fuct set_xy, uint16_t size_x, uint16_t size_y)
{
  _set_xy = set_xy;
  _size_x = size_x;
  _size_y = size_y;

  _mt_now_time  = new MatrixText(set_xy);
  _mt_now_name  = new MatrixText(set_xy);

  _mt_next_time = new MatrixText(set_xy);
  _mt_next_name = new MatrixText(set_xy);

  _mt_banner    = new MatrixText(set_xy);

  strcpy(_now_time,  "");
  strcpy(_next_time, "");
  strcpy(_now_name,  "");
  strcpy(_next_name, "");
  
  strcpy(_banner_msg, "Waiting for booking data...");

  //_mode = SCREEN_NOW_NEXT;
  _mode = SCREEN_BANNER;
}

MsNowNext::~MsNowNext()
{
  
}

void MsNowNext::init()
{
  _mt_now_name->set_scroll_speed( 20);
  _mt_next_name->set_scroll_speed(20);
  _mt_banner->set_scroll_speed(15);
  
  show_text();
}

bool MsNowNext::process_message(char *json_string)
{
  StaticJsonBuffer<500> json_buffer;
  JsonObject& root = json_buffer.parseObject(json_string);
  if (!root.success())
  {
    Serial.print("parseObject() failed (");
    Serial.print(json_string);
    Serial.println(")");
    return false;
  }
  
  const char* display_time_now  = root["now" ]["display_time"];
  const char* display_time_next = root["next"]["display_time"];
  const char* display_name_now  = root["now" ]["display_name"];
  const char* display_name_next = root["next"]["display_name"];
  
  
  if ((strcmp(display_name_now , "none") == 0) &&
      (strcmp(display_name_next, "none") == 0))
  {
    // No bookings within ~20hrs or so. So just show "no bookings" once in the 
    // middle of the screen.
    strcpy(_banner_msg, "No bookings....");
    set_screen(SCREEN_BANNER);
  }
  else
  {
    strlcpy(_now_time , display_time_now , sizeof(_now_time ));
    strlcpy(_next_time, display_time_next, sizeof(_next_time));
    strlcpy(_now_name , display_name_now , sizeof(_now_name ));
    strlcpy(_next_name, display_name_next, sizeof(_next_name));
    
    set_screen(SCREEN_NOW_NEXT);
  }
  
  show_text();
  
  return true;
}


void MsNowNext::show_text()
{
  uint8_t time_width = (SYSTEM5x8_WIDTH+1) * 5; //56 characters ("00:00")

  if (_mode == SCREEN_NOW_NEXT)
  {
    _mt_now_time->show_text(_now_time  , 0                         , 0, time_width, 7, false);
    _mt_now_name->show_text(_now_name  , time_width+SYSTEM5x8_WIDTH, 0, _size_x   , 7);

    _mt_next_time->show_text(_next_time, 0                         , 8, time_width, 15, false);
    _mt_next_name->show_text(_next_name, time_width+SYSTEM5x8_WIDTH, 8, _size_x   , 15);
  }
  else if (_mode == SCREEN_BANNER)
  {
    _mt_banner->show_text(_banner_msg, 0, 5, _size_x, _size_y);
  }
}

void MsNowNext::set_screen(screen_mode_t mode)
{
  if (mode != _mode)
  {
    // Wipe screen
    for (uint16_t x=0; x < _size_x; x++)
      for (uint16_t y=0; y < _size_y; y++)
        _set_xy(x, y, 0);

    _mode = mode;
  }
}


bool MsNowNext::loop()
{
  uint8_t buf_updated = false;

  if (_mode == SCREEN_NOW_NEXT)
  { 
    buf_updated  = _mt_now_time->loop();
    buf_updated |= _mt_now_name->loop();
    
    buf_updated |= _mt_next_time->loop();
    buf_updated |= _mt_next_name->loop();
  }
  else if (_mode == SCREEN_BANNER)
  {
    buf_updated = _mt_banner->loop();
  }
  
  return buf_updated;
  
}


