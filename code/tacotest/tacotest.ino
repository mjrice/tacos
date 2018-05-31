#include <Adafruit_STMPE610.h>

/***************************************************
  Taco Shield test app
 ****************************************************/

#include "SPI.h"
#include "ILI9341_t3.h"

#define COLOR_BG               ILI9341_WHITE
#define COLOR_TEXT             ILI9341_BLACK
#define COLOR_TEXT_BG          COLOR_BG
#define COLOR_TEXT_HL          ILI9341_GREEN

#define ROW_SIZE_PIXELS        18
#define TXT_ITEM_WIDTH_PIXELS  150
#define ROW_PADDING            4
#define SCREENPOS_X1           10 
#define SCREENPOS_Y1           44
#define SCREENPOS_X11          (SCREENPOS_X1+ROW_PADDING+ROW_SIZE_PIXELS)
#define TEXT_ROWS              9 //(ILI9341_TFTWIDTH / (ROW_SIZE_PIXELS+ROW_PADDING))


// TACo Shield pin assignments:
// ------------------------------------------------------------------------------
#define TFT_DC         9
#define TFT_CS         10
#define T0_ANALOG_PIN  A1
#define T1_ANALOG_PIN  A4
#define T2_ANALOG_PIN  A5
#define T3_ANALOG_PIN  A15
#define FET1_EN        3
#define FET2_EN        4
#define FET3_EN        32
#define FET4_EN        24
#define FET5_EN        25
#define X_ENABLE_PIN    0
#define X_STEP_PIN      6
#define X_DIR_PIN       5
#define Y_ENABLE_PIN    1
#define Y_STEP_PIN     20 
#define Y_DIR_PIN      21
#define Z_ENABLE_PIN   29
#define Z_STEP_PIN     14
#define Z_DIR_PIN       7
#define E0_ENABLE_PIN  30
#define E0_STEP_PIN    22
#define E0_DIR_PIN     23
#define E1_ENABLE_PIN  31
#define E1_STEP_PIN    16
#define E1_DIR_PIN     17

#define ENDSTOPPULLUPS
#define X_MIN_PIN      26
#define X_MAX_PIN      33
#define Y_MIN_PIN      37
#define Y_MAX_PIN      39
#define Z_MIN_PIN      27
#define Z_MAX_PIN      28

// ------------------------------------------------------------------------------

const int thermistor_pins [] = { T0_ANALOG_PIN,T1_ANALOG_PIN,T2_ANALOG_PIN,T3_ANALOG_PIN};
const int mosfet_pins[] =      { FET1_EN,      FET2_EN,      FET3_EN,      FET4_EN,       FET5_EN               };
const char *motor_label[] =    { "X",          "Y",          "Z",          "E0",          "E1"                  };
const int motor_step_pins[] =  { X_STEP_PIN,   Y_STEP_PIN,   Z_STEP_PIN,   E0_STEP_PIN,   E1_STEP_PIN           };
const int motor_dir_pins[] =   { X_DIR_PIN,    Y_DIR_PIN,    Z_DIR_PIN,    E0_DIR_PIN,    E1_DIR_PIN            };
const int motor_en_pins[] =    { X_ENABLE_PIN, Y_ENABLE_PIN, Z_ENABLE_PIN, E0_ENABLE_PIN, E1_ENABLE_PIN         };
const int endstop_pins[] =     { X_MIN_PIN,    X_MAX_PIN,    Y_MIN_PIN,    Y_MAX_PIN,     Z_MIN_PIN,  Z_MAX_PIN };
const char *endstop_label[] =  { "XMIN",       "XMAX",       "YMIN",       "YMAX",        "ZMIN",     "ZMAX"    };
int loopcounter = 0;

// This is calibration data for the raw touch data to the screen coordinates
#define TS_MINX 150
#define TS_MINY 130
#define TS_MAXX 3800
#define TS_MAXY 4000

#define STMPE_CS 8
ILI9341_t3 tft = ILI9341_t3(TFT_CS, TFT_DC);
Adafruit_STMPE610 ts = Adafruit_STMPE610(STMPE_CS);

int active_screen;

void setup() 
{
  active_screen=1;
  SPI.setMOSI(11);
  SPI.setSCK(13);
  delay(100);
  tft.begin();
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_YELLOW);
  tft.setTextSize(2);
  tft.setRotation(1);
  tft.print("Waiting for Arduino Serial Monitor...");

  Serial.begin(115200);
  int j = 0;
  int k = 0;
  while (!Serial && k<200) // wait for Arduino Serial Monitor
  {
      j++;
      if(j>200000) 
      { 
         tft.print("."); 
         k++;
         j=0;
      }
  }
  
  Serial.println("ILI9341 Diagnostics:"); 
  // read diagnostics (optional but can help debug problems)
  uint8_t x = tft.readcommand8(ILI9341_RDMODE);
  Serial.print("Display Power Mode: 0x"); 
  Serial.println(x, HEX);
  x = tft.readcommand8(ILI9341_RDMADCTL);
  Serial.print("MADCTL Mode: 0x"); 
  Serial.println(x, HEX);
  x = tft.readcommand8(ILI9341_RDPIXFMT);
  Serial.print("Pixel Format: 0x"); 
  Serial.println(x, HEX);
  x = tft.readcommand8(ILI9341_RDIMGFMT);
  Serial.print("Image Format: 0x"); 
  Serial.println(x, HEX);
  x = tft.readcommand8(ILI9341_RDSELFDIAG);
  Serial.print("Self Diagnostic: 0x"); 
  Serial.println(x, HEX); 
  Serial.println(F("Setup Done!"));

  // setup mosfet output pins----------------------
  for(j=0;j<5;j++) 
  {
      pinMode(mosfet_pins[j],OUTPUT);
  }

  // setup motor driver pins-----------------------
  for(j=0;j<5;j++)
  {
      pinMode(motor_en_pins[j],OUTPUT);
      pinMode(motor_dir_pins[j],OUTPUT);
      pinMode(motor_step_pins[j],OUTPUT);
  }
  
   // setup endstop pins---------------------------
   #ifdef ENDSTOPPULLUPS
   for(j=0;j<6;j++)
   {
      pinMode(endstop_pins[j],INPUT_PULLUP);
   }
   #endif

  if(1) 
  {
    tft.fillScreen(COLOR_BG);
    tft.setCursor(0,0);
    tft.setTextSize(4);
    tft.setTextColor(COLOR_TEXT_HL);  
    tft.println("TACo Shield");
    tft.setTextColor(COLOR_TEXT);  
    tft.setTextSize(1);
    tft.println("Teensy Automation & Control Shield Test");
    tft.setTextSize(2);
  }

  greenBtn();
}

#define GREENBUTTON_X 10
#define GREENBUTTON_Y 100 
#define GREENBUTTON_W 20
#define GREENBUTTON_H 50
#define REDBUTTON_X 200
#define REDBUTTON_Y 130
#define REDBUTTON_W 60
#define REDBUTTON_H 30

#define FRAME_X 10
#define FRAME_Y 20 
#define FRAME_W 300
#define FRAME_H 100

void drawFrame()
{
  tft.drawRect(FRAME_X, FRAME_Y, FRAME_W, FRAME_H, ILI9341_BLACK);
}

void greenBtn()
{
  //tft.fillRect(GREENBUTTON_X, GREENBUTTON_Y, GREENBUTTON_W, GREENBUTTON_H, ILI9341_GREEN);
  tft.fillRect(REDBUTTON_X, REDBUTTON_Y, REDBUTTON_W, REDBUTTON_H, ILI9341_BLUE);
  //drawFrame();
  tft.setCursor(REDBUTTON_X + 6 , REDBUTTON_Y + (REDBUTTON_H/2));
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.println("OFF");
  //RecordOn = true;
}

//
// thermistor table, copied out of Marlin thermistortables.h 
//
#define OVERSAMPLENR 1
const short temptable_5[][2] PROGMEM = {
// ATC Semitec 104GT-2 (Used in ParCan)
// Verified by linagee. Source: http://shop.arcol.hu/static/datasheets/thermistors.pdf
// Calculated using 4.7kohm pullup, voltage divider math, and manufacturer provided temp/resistance
   {1*OVERSAMPLENR, 713},
   {17*OVERSAMPLENR, 300}, //top rating 300C
   {20*OVERSAMPLENR, 290},
   {23*OVERSAMPLENR, 280},
   {27*OVERSAMPLENR, 270},
   {31*OVERSAMPLENR, 260},
   {37*OVERSAMPLENR, 250},
   {43*OVERSAMPLENR, 240},
   {51*OVERSAMPLENR, 230},
   {61*OVERSAMPLENR, 220},
   {73*OVERSAMPLENR, 210},
   {87*OVERSAMPLENR, 200},
   {106*OVERSAMPLENR, 190},
   {128*OVERSAMPLENR, 180},
   {155*OVERSAMPLENR, 170},
   {189*OVERSAMPLENR, 160},
   {230*OVERSAMPLENR, 150},
   {278*OVERSAMPLENR, 140},
   {336*OVERSAMPLENR, 130},
   {402*OVERSAMPLENR, 120},
   {476*OVERSAMPLENR, 110},
   {554*OVERSAMPLENR, 100},
   {635*OVERSAMPLENR, 90},
   {713*OVERSAMPLENR, 80},
   {784*OVERSAMPLENR, 70},
   {846*OVERSAMPLENR, 60},
   {897*OVERSAMPLENR, 50},
   {937*OVERSAMPLENR, 40},
   {966*OVERSAMPLENR, 30},
   {986*OVERSAMPLENR, 20},
   {1000*OVERSAMPLENR, 10},
   {1010*OVERSAMPLENR, 0}
};

float lookupT(short int adcval)
{
  float t1=0.0f;
  float t2 = 0.0f;
  float tval;
  int i = 0;
  
  //Serial.print("temperature lookup on val ");
  //Serial.println(adcval);

  while(adcval>temptable_5[i][0] && temptable_5[i][1] > 0)
  {
     i++;
  }

  if(i>0) t1 = temptable_5[i-1][1];
  t2 = temptable_5[i][1];

  // linear interpolate between the nearest two values in the thermistor table
  tval = t1 + (t2-t1) * (adcval-temptable_5[i-1][0])/(temptable_5[i][0]-temptable_5[i-1][0]);

 // round to nearest degree C
  tval = (int)tval;
  
  //Serial.print("interpolated value ");
  //Serial.println(tval);
  
  return tval;
}

int motor_dir = 0;
  char str[40];
  
void loop(void) 
{
  if(active_screen==1)
  {    
    if (!ts.bufferEmpty())
    {   
      // Retrieve a point  
      TS_Point p = ts.getPoint(); 
      // Scale using the calibration #'s
      // and rotate coordinate system
      p.x = map(p.x, TS_MINY, TS_MAXY, 0, tft.height());
      p.y = map(p.y, TS_MINX, TS_MAXX, 0, tft.width());
      int y = tft.height() - p.x;
      int x = p.y;
      sprintf(str,"x=%d y=%d",x,y);
      showItem(10,str);    
    }

    int state;

    for(int i=0;i<6;i++)
    {
      // endstops should be configured for normally-closed condition, so a missing switch or a connected but 
      // activated switch will either return a logical "1" and a connected but open switch will return a logical "0"
      state = digitalRead(endstop_pins[i]);
      sprintf(str,"%s: %s",endstop_label[i],state?"CLOSED":"OPEN");
      showItem(i+1,str);
    }
    
  }
  else if(active_screen==2)
  {
       updateScreen2();
  }
}

void updateScreen2()
{
  int val;
  char str[40];
  float temperature;
  int i;
  int final_val;
  int index;
  
  /*alive_led++;
  if(alive_led & 0x40) new_led_color=ILI9341_RED;
  else                 new_led_color= COLOR_BG;
  if(new_led_color!=led_color)
  {
       led_color=new_led_color;
       tft.fillCircle(302,14,10, led_color);
  }*/
  
  switch(loopcounter)
  {
    case 0: // -
    case 1: // -
    case 2: // -
    case 3: // - read and display temperature for the four thermistor inputs
            ReadThermistor(loopcounter);            
            break;

    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
           digitalWrite(mosfet_pins[loopcounter-4],1);
           sprintf(str,"FET%d: ON",loopcounter-4);
           showItem(loopcounter+1,str);
           break; 
           
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
           index=loopcounter-9;
           TurnMotor(index,motor_dir);
           break;
                      
     case 14:
     case 15:
     case 16:
     case 17:
     case 18:
           index=loopcounter-14;
           digitalWrite(mosfet_pins[index],0);
           sprintf(str,"FET%d: OFF",index);
           showItem(index+5,str);
           break;
            
    default: // reset loop
            loopcounter=-1;    
            motor_dir^=1;
  }
   
  loopcounter++;
  delay(50);
}

void ReadThermistor(int index)
{  
     int val = analogRead(thermistor_pins[index]);
     float temperature = lookupT(val);
     char str[20];
     if(temperature>0)
              sprintf(str,"T%d:   %g C",index,temperature);
     else
              sprintf(str,"T%d:   ---",index);
     showItem(index+1,str);
}

void TurnMotor(int index,int motor_dir)
{
     digitalWrite(motor_en_pins[index],0);
     digitalWrite(motor_dir_pins[index],motor_dir);
     int i = motor_dir?999:0;
     int final_val = motor_dir?0:999;
     char str[20];
           
     while(i != final_val) 
     {
               sprintf(str,"%s POS %d",motor_label[index],i);
               showItem(index+10,str);
               digitalWrite(motor_step_pins[index],1);
               delayMicroseconds(10);
               digitalWrite(motor_step_pins[index],0);
               //delayMicroseconds(10);
               i+= (motor_dir?-1:1);
     }
     digitalWrite(motor_en_pins[index],1);
}
           
void showItem(int screenpos,char *str)
{
    int xpos,ypos;

  xpos=SCREENPOS_X1;

    while(screenpos>TEXT_ROWS*2) screenpos-=(TEXT_ROWS*2);
    
    if(screenpos>TEXT_ROWS) 
    {
       xpos = SCREENPOS_X1 + TXT_ITEM_WIDTH_PIXELS+ROW_PADDING;
       ypos = SCREENPOS_Y1 + (screenpos-(TEXT_ROWS+1))*(ROW_SIZE_PIXELS+ROW_PADDING); 
    }
    else
    {
       ypos = SCREENPOS_Y1 + (screenpos-1)*(ROW_SIZE_PIXELS+ROW_PADDING); 
    }
    tft.fillRect(xpos-1,ypos-2,TXT_ITEM_WIDTH_PIXELS,ROW_SIZE_PIXELS+2,COLOR_TEXT_BG );
    tft.setCursor(xpos,ypos);            
    tft.println(str);

}

unsigned long testFillScreen() 
{
  unsigned long start = micros();
  tft.fillScreen(ILI9341_BLACK);
  tft.fillScreen(ILI9341_RED);
  tft.fillScreen(ILI9341_GREEN);
  tft.fillScreen(ILI9341_BLUE);
  tft.fillScreen(ILI9341_BLACK);
  return micros() - start;
}



unsigned long testLines(uint16_t color) 
{
  unsigned long start, t;
  int           x1, y1, x2, y2,
                w = tft.width(),
                h = tft.height();

  tft.fillScreen(ILI9341_BLACK);

  x1 = y1 = 0;
  y2    = h - 1;
  start = micros();
  for(x2=0; x2<w; x2+=6) tft.drawLine(x1, y1, x2, y2, color);
  x2    = w - 1;
  for(y2=0; y2<h; y2+=6) tft.drawLine(x1, y1, x2, y2, color);
  t     = micros() - start; // fillScreen doesn't count against timing

  tft.fillScreen(ILI9341_BLACK);

  x1    = w - 1;
  y1    = 0;
  y2    = h - 1;
  start = micros();
  for(x2=0; x2<w; x2+=6) tft.drawLine(x1, y1, x2, y2, color);
  x2    = 0;
  for(y2=0; y2<h; y2+=6) tft.drawLine(x1, y1, x2, y2, color);
  t    += micros() - start;

  tft.fillScreen(ILI9341_BLACK);

  x1    = 0;
  y1    = h - 1;
  y2    = 0;
  start = micros();
  for(x2=0; x2<w; x2+=6) tft.drawLine(x1, y1, x2, y2, color);
  x2    = w - 1;
  for(y2=0; y2<h; y2+=6) tft.drawLine(x1, y1, x2, y2, color);
  t    += micros() - start;

  tft.fillScreen(ILI9341_BLACK);

  x1    = w - 1;
  y1    = h - 1;
  y2    = 0;
  start = micros();
  for(x2=0; x2<w; x2+=6) tft.drawLine(x1, y1, x2, y2, color);
  x2    = 0;
  for(y2=0; y2<h; y2+=6) tft.drawLine(x1, y1, x2, y2, color);

  return micros() - start;
}

unsigned long testFastLines(uint16_t color1, uint16_t color2) {
  unsigned long start;
  int           x, y, w = tft.width(), h = tft.height();

  tft.fillScreen(ILI9341_BLACK);
  start = micros();
  for(y=0; y<h; y+=5) tft.drawFastHLine(0, y, w, color1);
  for(x=0; x<w; x+=5) tft.drawFastVLine(x, 0, h, color2);

  return micros() - start;
}

unsigned long testRects(uint16_t color) {
  unsigned long start;
  int           n, i, i2,
                cx = tft.width()  / 2,
                cy = tft.height() / 2;

  tft.fillScreen(ILI9341_BLACK);
  n     = min(tft.width(), tft.height());
  start = micros();
  for(i=2; i<n; i+=6) {
    i2 = i / 2;
    tft.drawRect(cx-i2, cy-i2, i, i, color);
  }

  return micros() - start;
}

unsigned long testFilledRects(uint16_t color1, uint16_t color2) {
  unsigned long start, t = 0;
  int           n, i, i2,
                cx = tft.width()  / 2 - 1,
                cy = tft.height() / 2 - 1;

  tft.fillScreen(ILI9341_BLACK);
  n = min(tft.width(), tft.height()) - 1;
  for(i=n; i>0; i-=6) {
    i2    = i / 2;
    start = micros();
    tft.fillRect(cx-i2, cy-i2, i, i, color1);
    t    += micros() - start;
    // Outlines are not included in timing results
    tft.drawRect(cx-i2, cy-i2, i, i, color2);
  }

  return t;
}

unsigned long testFilledCircles(uint8_t radius, uint16_t color) {
  unsigned long start;
  int x, y, w = tft.width(), h = tft.height(), r2 = radius * 2;

  tft.fillScreen(ILI9341_BLACK);
  start = micros();
  for(x=radius; x<w; x+=r2) {
    for(y=radius; y<h; y+=r2) {
      tft.fillCircle(x, y, radius, color);
    }
  }

  return micros() - start;
}

unsigned long testCircles(uint8_t radius, uint16_t color) {
  unsigned long start;
  int           x, y, r2 = radius * 2,
                w = tft.width()  + radius,
                h = tft.height() + radius;

  // Screen is not cleared for this one -- this is
  // intentional and does not affect the reported time.
  start = micros();
  for(x=0; x<w; x+=r2) {
    for(y=0; y<h; y+=r2) {
      tft.drawCircle(x, y, radius, color);
    }
  }

  return micros() - start;
}






