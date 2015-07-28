#include "MsNowNext.h"

MsNowNext::MsNowNext(set_xy_fuct set_xy, uint16_t size_x, uint16_t size_y)
{
  _set_xy = set_xy;
  _size_x = size_x;
  _size_y = size_y;
 
  _mt_now_time = new MatrixText(set_xy);
  _mt_now_name = new MatrixText(set_xy);
  
  _mt_next_time = new MatrixText(set_xy);
  _mt_next_name = new MatrixText(set_xy);
  
  strcpy(_now_time,  "");
  strcpy(_next_time, "");
  strcpy(_now_name,  "");
  strcpy(_next_name, "");
}

MsNowNext::~MsNowNext()
{
  
}

void MsNowNext::init()
{
  _mt_now_name->set_scroll_speed( 20);
  _mt_next_name->set_scroll_speed(20);
  show_text();
  
}

bool MsNowNext::process_message(char *json_string)
{
  StaticJsonBuffer<500> json_buffer;
  JsonObject& root = json_buffer.parseObject(json_string);
  if (!root.success())
  {
    Serial.println("parseObject() failed");
    Serial.println(json_string);
    return false;
  }
  
  const char* display_time_now  = root["now" ]["display_time"];
  const char* display_time_next = root["next"]["display_time"];
  const char* display_name_now  = root["now" ]["display_name"];
  const char* display_name_next = root["next"]["display_name"];
  
  strlcpy(_now_time , display_time_now , sizeof(_now_time ));
  strlcpy(_next_time, display_time_next, sizeof(_next_time));
  strlcpy(_now_name , display_name_now , sizeof(_now_name ));
  strlcpy(_next_name, display_name_next, sizeof(_next_name));
  
  show_text();
  
  
  Serial.println(_now_time);
  Serial.println(_next_time);
  Serial.println(_now_name);
  Serial.println(_next_name); 
  
  return true;
}


void MsNowNext::show_text()
{
  uint8_t time_width = (SYSTEM5x8_WIDTH+1) * 5; //56 characters ("00:00")

  _mt_now_time->show_text(_now_time  , 0                        , 0, time_width, 7, false);
  _mt_now_name->show_text(_now_name , time_width+SYSTEM5x8_WIDTH, 0, _size_x   , 7);

  _mt_next_time->show_text(_next_time, 0                         , 8, time_width, 15, false);
  _mt_next_name->show_text(_next_name, time_width+SYSTEM5x8_WIDTH, 8, _size_x   , 15);  
}



bool MsNowNext::loop()
{
    uint8_t buf_updated = false;

  buf_updated = _mt_now_time->loop();

  buf_updated |= _mt_now_name->loop();
  
  buf_updated |= _mt_next_time->loop();
  buf_updated |= _mt_next_name->loop();
  
  return buf_updated;
  
}


