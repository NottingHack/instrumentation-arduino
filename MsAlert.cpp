#include "MsAlert.h"

static set_xy_fuct g_set_xy;
static bool g_invert;

MsAlert::MsAlert(set_xy_fuct set_xy, uint16_t size_x, uint16_t size_y)
{
  _set_xy = set_xy;
  _size_x = size_x;
  _size_y = size_y;
  
  g_set_xy = set_xy;
  g_invert = false;
  _mt_top = new MatrixText(alert_set_xy);
  
  strcpy(_top_msg, "TEST ALERT");
  show_text();
}

MsAlert::~MsAlert()
{
  
}

void MsAlert::init()
{
  //_mt_top->set_scroll_speed( 20);


  show_text();
}


void MsAlert::show_text()
{


  _mt_top->show_text(_top_msg, 2, 5, _size_x-25, 5+8, false);

}


bool MsAlert::loop()
{
  static uint8_t a;
  a++;
  
  uint8_t buf_updated = false;


  
  if (a == 50)
  {
    a = 0;
    g_invert = !g_invert;
  
// for (int x = 21; x < 151 ; x++)
    for (int x = 0; x < 100 ; x++)
      for (int y = 0; y < 2; y++)
      {

        alert_set_xy(x, y, 1);
      }
  }
    
    _mt_top->loop(true);
  
  buf_updated = true;

  
 // buf_updated |= _mt_bottom->loop();

  return buf_updated;
}

void alert_set_xy (uint16_t x, byte y, byte val)
{
  
//  g_set_xy(x, y, (g_invert ? !val : val));
g_set_xy(x, y, val);
  
}


