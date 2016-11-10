#include "MsAlert.h"

//static set_xy_fuct g_set_xy;
//static bool g_invert;

MsAlert::MsAlert(set_xy_fuct set_xy, uint16_t size_x, uint16_t size_y)
{
  _set_xy = set_xy;
  _size_x = size_x;
  _size_y = size_y;


  _mt_top = new MatrixText(_set_xy);

  strcpy(_msg, "");
  show_text();
}

MsAlert::~MsAlert()
{

}

void MsAlert::set_message(char *msg)
{
  strlcpy(_msg, msg, sizeof(_msg));
  show_text();
}

void MsAlert::init()
{
  _mt_top->set_scroll_speed(10);

  draw_border(0);
  show_text();
}


void MsAlert::show_text()
{
  bool scroll_text;
  int left_x, right_x;

  left_x  = SYSTEM5x8_WIDTH;
  right_x = _size_x - SYSTEM5x8_WIDTH;

  // If the text won't fit on the display, then it needs to be scrolled.
  scroll_text = ( (strlen(_msg)*(SYSTEM5x8_WIDTH+1)) > (right_x-left_x) );

  _mt_top->show_text(_msg, left_x, 5, right_x, 5+8, scroll_text);
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

  return buf_updated;
}



