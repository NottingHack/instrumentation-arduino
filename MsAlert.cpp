#include "MsAlert.h"

//static set_xy_fuct g_set_xy;
//static bool g_invert;

MsAlert::MsAlert(set_xy_fuct set_xy, uint16_t size_x, uint16_t size_y)
{
  _set_xy = set_xy;
  _size_x = size_x;
  _size_y = size_y;
  

  _mt_top = new MatrixText(_set_xy);
  
  strcpy(_top_msg, "TEST ALERT");
  show_text();
}

MsAlert::~MsAlert()
{
  
}

void MsAlert::init()
{
  //_mt_top->set_scroll_speed( 20);


  draw_border(0);
  show_text();
}


void MsAlert::show_text()
{


  _mt_top->show_text(_top_msg, SYSTEM5x8_WIDTH, 5, _size_x-25, 5+8, false);

}

void MsAlert::draw_border(uint8_t a)
{
  // draw border
  // top line
  for (int x=0; x < _size_x; x++)
  {
    if ((x%2) == a)
    {
      _set_xy(x, 0, 1);
      _set_xy(x, 1, 1);
    }
    else
    {
      _set_xy(x, 0, 0);
      _set_xy(x, 1, 0);
    }
  }
  
  // bottom line
  for (int x=0; x < _size_x; x++)
  {
    if ((x%2) == a)
    {
      _set_xy(x, 14, 1);
      _set_xy(x, 15, 1);
    }
    else
    {
      _set_xy(x, 14, 0);
      _set_xy(x, 15, 0);
    }
  }
  
  // left line
  for (int y=0; y < _size_y; y++)
  {
    if ((y%2) == a)
    {
      _set_xy(0, y, 1);
      _set_xy(1, y, 1);
    } 
    else
    {
      _set_xy(0, y, 0);
      _set_xy(1, y, 0);
    } 
  }

  // right line
  for (int y=0; y < _size_y; y++)
  {
    if ((y%2) == a)
    {
      _set_xy(_size_x  -1, y, 1);
      _set_xy(_size_x - 2, y, 1);
    }
    else
    {
      _set_xy(_size_x  -1, y, 0);
      _set_xy(_size_x - 2, y, 0);
    }
  }
}


bool MsAlert::loop()
{
  static uint8_t a;
  a++;
  if (a>60)
    a = 0;
  
  uint8_t buf_updated = false;

  if (a > 30)
    draw_border(1);
  else
    draw_border(0);
  
  _mt_top->loop(true);
  
  buf_updated = true;

  
 // buf_updated |= _mt_bottom->loop();

  return buf_updated;
}



