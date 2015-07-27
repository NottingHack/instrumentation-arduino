#include "MsNowNext.h"

MsNowNext::MsNowNext(set_xy_fuct set_xy, uint16_t size_x, uint16_t size_y)
{
  _set_xy = set_xy;
  _size_x = size_x;
  _size_y = size_y;
 
  _mt_now_time = new MatrixText(set_xy);
  _mt_now_name = new MatrixText(set_xy);
  
}

MsNowNext::~MsNowNext()
{
  
}

void MsNowNext::init()
{
  _mt_now_time->show_text("123456789012345678901234567890", 0, 8, 192, 16, false);
  _mt_now_name->show_text("TEST", 0, 0, 192, 8);
  
  _mt_now_name->set_scroll_speed(0);
}


bool MsNowNext::loop()
{
    uint8_t buf_updated = false;

  buf_updated = _mt_now_time->loop();

  buf_updated |= _mt_now_name->loop();
  
  return buf_updated;
  
}


