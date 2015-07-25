/*
  MatrixScreen.h 
*/
#ifndef MatrixScreen_h
#define MatrixScreen_h



class MatrixScreen
{
  public:
    MatrixScreen();
    
    virtual void init()  = 0;
    virtual void loop()  = 0;

  private:

};

#endif