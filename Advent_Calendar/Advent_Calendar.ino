/**
 * @copyright Copyright (c) 2024  Shenzhen Xin Yuan Electronic Technology Co., Ltd
 * @date      2024-04-05
 * @note      Arduino Setting
 *            Tools ->
 *                  Board:"ESP32S3 Dev Module"
 *                  USB CDC On Boot:"Enable"
 *                  USB DFU On Boot:"Disable"
 *                  Flash Size : "16MB(128Mb)"
 *                  Flash Mode"QIO 80MHz
 *                  Partition Scheme:"16M Flash(3M APP/9.9MB FATFS)"
 *                  PSRAM:"OPI PSRAM"
 *                  Upload Mode:"UART0/Hardware CDC"
 *                  USB Mode:"Hardware CDC and JTAG"
 *  
 */

#ifndef BOARD_HAS_PSRAM
#error "Please enable PSRAM, Arduino IDE -> tools -> PSRAM -> OPI !!!"
#endif

#include <Arduino.h>
#include "epd_driver.h"
#include "firasans.h"
#include <Wire.h>
#include <TouchDrvGT911.hpp>
#include "utilities.h"
#include <WiFi.h>
#include "time.h"
#include "advents.h"
#include <Arduino_MultiWiFi.h>

#include "cat1.h"
#include "cat2.h"
#include "cat3.h"
//#include "cat4.h"
//#include "cat5.h"

// I made all the images this same size, feel free to change and have a different area for each picture
Rect_t imageArea = {
  .x = 80,
  .y = 150,
  .width = 600,
  .height =  400
};

MultiWiFi multiWiFi;


const char* ntpServer = "pool.ntp.org";
const long  ct_Offset_sec = 3600 - (7 *3600);
const int   daylightOffset_sec = 0;

struct tm timeinfo;

#define BUTTON_ROWS 5
#define BUTTON_COLS 5
//#define EDGE_SPACING 5 //pixels between button and from edge
#define VERTICAL_BUTTON_SPACING 10 // vertical pixels between buttons and from edge
#define HORIZONTAL_BUTTON_SPACING 10 // horizontal pixels between buttons
#define STATS_LINE 200
#define BUTTON_WIDTH EPD_WIDTH - STATS_LINE
#define BUTTON_HEIGHT EPD_HEIGHT
#define buttonWidth  (BUTTON_WIDTH - (HORIZONTAL_BUTTON_SPACING * (BUTTON_ROWS + 1))) / (BUTTON_ROWS) //subtract negative space from total space, divide by number of buttons
#define buttonHeight  (BUTTON_HEIGHT - (HORIZONTAL_BUTTON_SPACING * (BUTTON_COLS + 1))) / (BUTTON_COLS)
#define NUM_BUTTONS BUTTON_ROWS * BUTTON_COLS
typedef struct {
  uint16_t x;
  uint16_t y;
} Button;


Button buttonCoords1D [NUM_BUTTONS]; //

uint8_t debounce = 0;
uint8_t state = 1;
int cursor_x;
int cursor_y;
int16_t  x, y; //store touch coordinates

TouchDrvGT911 touch;
uint8_t *framebuffer = NULL;

const char* loveArray[] = {love1, love2, love3, love4, love5};
const char* dateArray[] = {date1, date2, date3, date4, date5};
const char* couponArray[] = {coupon1, coupon2, coupon3, coupon4, coupon5};


//Initialize the top left x,y coordinates for each button. Draw each button as well
void initializeButtonCoords(){
  uint8_t buttonNum = 1;
  char str[20];
  for(int col = 0; col < BUTTON_COLS; col++){
    for(int row = 0; row < BUTTON_ROWS; row++){


      buttonCoords1D[buttonNum].x = (HORIZONTAL_BUTTON_SPACING * (row + 1)) + (row * buttonWidth);
      buttonCoords1D[buttonNum].y = (VERTICAL_BUTTON_SPACING * (col + 1)) + (col * buttonHeight);


      cursor_x = buttonCoords1D[buttonNum].x + (buttonWidth/2) - 10;
      cursor_y = buttonCoords1D[buttonNum].y + (buttonHeight/2) + 12;

      sprintf(str, "%d", buttonNum);

      writeln((GFXfont *)&FiraSans, str, &cursor_x, &cursor_y, framebuffer);
      epd_draw_rect(buttonCoords1D[buttonNum].x, buttonCoords1D[buttonNum].y, buttonWidth, buttonHeight, 0, framebuffer);
      buttonNum++;
    }
  }

}
//draw month and day in the top right
void drawTimeInfo(){

  char day_str[3];
  char month_abbr[4];
  strftime(day_str, sizeof(day_str), "%d", &timeinfo);
  strftime(month_abbr, sizeof(month_abbr), "%b", &timeinfo);
  cursor_x = 835;
  cursor_y = 350;
  write_string((GFXfont *)&FiraSans, (char *)day_str, &cursor_x, &cursor_y, framebuffer);
  cursor_x = 820;
  cursor_y = 300;

  write_string((GFXfont *)&FiraSans, (char *)month_abbr, &cursor_x, &cursor_y, framebuffer); 
}


void wifiInit(){
  Serial.println();
  Serial.println();
  Serial.println("Connecting to Wifi");
  multiWiFi.add(WIFI_SSID1, WIFI_PASSWORD1);
  multiWiFi.add(WIFI_SSID2, WIFI_PASSWORD2);
  multiWiFi.add(WIFI_SSID3, WIFI_PASSWORD3);


  //WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  // while (WiFi.status() != WL_CONNECTED) {
  //     delay(500);
  //     Serial.print(".");
  // }
  if (multiWiFi.run()) {
    Serial.println("Connected to WiFi!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.println(WiFi.SSID());
  } else {
    Serial.println("Failed to connect to any WiFi network.");
    // Handle failed connection
  }

  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  //init and get the time
  configTime(ct_Offset_sec, daylightOffset_sec, ntpServer);
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");

  //disconnect WiFi as it's no longer needed
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
}

// Return which button was pressed (1-25) from x,y coordinates
uint8_t getButtonPressed(uint16_t x, uint16_t y){
  uint8_t buttonNum = 1;
  for(int col = 0; col < BUTTON_COLS; col++){
      for(int row = 0; row < BUTTON_ROWS; row++){
        if((x > (buttonCoords1D[buttonNum].x) && x < (buttonCoords1D[buttonNum].x + buttonWidth)) && (y > (buttonCoords1D[buttonNum].y) && y < (buttonCoords1D[buttonNum].y + buttonHeight))){
          return buttonNum;
        }
        buttonNum++;
      }
   }
}

//Go into sleep mode if date is not december or button has been pressed
void shutoff(){
  // The touch interrupt uses non-RTC-IO, so the touch wake-up function cannot be used to set the touch to sleep
  //touch.sleep();

  delay(5);

  //Wire.end();

  //pinMode(BOARD_SDA, OPEN_DRAIN);
  //pinMode(BOARD_SCL, OPEN_DRAIN);
  //pinMode(TOUCH_INT, OPEN_DRAIN);

  epd_poweroff();

#if defined(CONFIG_IDF_TARGET_ESP32)
  // Set to wake up by GPIO39
  //esp_sleep_enable_ext1_wakeup(GPIO_SEL_39, ESP_EXT1_WAKEUP_ANY_LOW);
#elif defined(CONFIG_IDF_TARGET_ESP32S3)
  //esp_sleep_enable_ext1_wakeup(GPIO_SEL_21, ESP_EXT1_WAKEUP_ANY_LOW);
#endif
  esp_deep_sleep_start();
  
}


void setup()
{
  Serial.begin(115200);
  
  wifiInit();

  //initialize frame buffer
  framebuffer = (uint8_t *)ps_calloc(sizeof(uint8_t), EPD_WIDTH * EPD_HEIGHT / 2);
  if (!framebuffer) {
    Serial.println("alloc memory failed !!!");
    while (1);
  }
  memset(framebuffer, 0xFF, EPD_WIDTH * EPD_HEIGHT / 2);

  //Initialize the ePaper display
  epd_init();

  //* Sleep wakeup must wait one second, otherwise the touch device cannot be addressed TODO dont need this
  if (esp_sleep_get_wakeup_cause() != ESP_SLEEP_WAKEUP_UNDEFINED) {
    delay(1000);
  }

  Wire.begin(BOARD_SDA, BOARD_SCL);

  // Assuming that the previous touch was in sleep state, wake it up
  pinMode(TOUCH_INT, OUTPUT);
  digitalWrite(TOUCH_INT, HIGH);

  /*
  * The touch reset pin uses hardware pull-up,
  * and the function of setting the I2C device address cannot be used.
  * Use scanning to obtain the touch device address.*/
  uint8_t touchAddress = 0;
  Wire.beginTransmission(0x14);
  if (Wire.endTransmission() == 0) {
    touchAddress = 0x14;
  }
  Wire.beginTransmission(0x5D);
  if (Wire.endTransmission() == 0) {
    touchAddress = 0x5D;
  }
  if (touchAddress == 0) {
    while (1) {
      Serial.println("Failed to find GT911 - check your wiring!");
      delay(1000);
    }
}
  touch.setPins(-1, TOUCH_INT);
  if (!touch.begin(Wire, touchAddress, BOARD_SDA, BOARD_SCL )) {
    while (1) {
      Serial.println("Failed to find GT911 - check your wiring!");
      delay(1000);
    }
  }
  touch.setMaxCoordinates(EPD_WIDTH, EPD_HEIGHT);

  touch.setSwapXY(true);
  touch.setMirrorXY(false, true);

  Serial.println("Started Touchscreen poll...");

  //power on screen
  epd_poweron();
  //clear the screen by flashing
  epd_clear();

  initializeButtonCoords();
  
  //Add Title Block
  char *adventCalendar = "   2025\n  Advent\nCalendar\n   Last\nChecked:";
  cursor_x = 770;
  cursor_y = 50;
  write_string((GFXfont *)&FiraSans, (char *)adventCalendar, &cursor_x, &cursor_y, framebuffer);
  drawTimeInfo();

  //draw rectabgle no fill
  //epd_draw_rect(10, 20, EPD_WIDTH - 20, EPD_HEIGHT / 2 + 80, 0, framebuffer);
  //draw a picture, area is not cleared (area assumed to be white)
  epd_draw_grayscale_image(epd_full_screen(), framebuffer);

  if (timeinfo.tm_mon + 1 !=12 || timeinfo.tm_mon + 1 !=1){ //Allow january
    char *notYet = "Not Dec\n   Yet!";

    cursor_x = 780;
    cursor_y = 450;
    write_string((GFXfont *)&FiraSans, (char *)notYet, &cursor_x, &cursor_y, framebuffer);
    epd_poweron();
    epd_draw_grayscale_image(epd_full_screen(), framebuffer);
    shutoff();
  }
  epd_poweroff();

}




void loop()
{
  uint8_t touched = touch.getPoint(&x, &y);
  if (touched) {
    if (debounce == 0){
      debounce++;
      return;
    }
    
    debounce = 0;
    // Serial.printf("X:%d Y:%d\n", x, y);
    uint8_t buttonNum = getButtonPressed(x, y);

    if(buttonNum > timeinfo.tm_mday){
      char *no = "NO";
      
      cursor_x = buttonCoords1D[buttonNum].x;
      cursor_y = buttonCoords1D[buttonNum].y + 50;
      write_string((GFXfont *)&FiraSans, (char *)no, &cursor_x, &cursor_y, framebuffer);
      epd_poweron();
      epd_draw_grayscale_image(epd_full_screen(), framebuffer);
      epd_poweroff(); 
      return;
    }

    memset(framebuffer, 0xFF, EPD_WIDTH * EPD_HEIGHT / 2); //clear the frame buffer
    Serial.print(millis());
    Serial.print(":");
    Serial.println(buttonNum);
    epd_poweron();

    cursor_x = 10;
    cursor_y = 200;
    switch (buttonNum) {
      case 1:
        epd_clear_area(epd_full_screen());
        cursor_x = 10;
        cursor_y = 200;
        write_string((GFXfont *)&FiraSans, (char *)loveArray[0], &cursor_x, &cursor_y, framebuffer);
        break;
      case 2:
        epd_clear_area(epd_full_screen());
        cursor_x = 10;
        cursor_y = 200;
        write_string((GFXfont *)&FiraSans, (char *)dateArray[0], &cursor_x, &cursor_y, framebuffer);
        break;
      case 3:
        epd_clear_area(epd_full_screen());
        write_string((GFXfont *)&FiraSans, (char *)couponArray[0], &cursor_x, &cursor_y, framebuffer);
        break;
      case 4:
        epd_clear_area(epd_full_screen());
        // ask for voice note 1
        cursor_x = 10;
        cursor_y = 200;
        write_string((GFXfont *)&FiraSans, (char *)voiceMessage, &cursor_x, &cursor_y, framebuffer);
        break;
      case 5:
        // Christmas pic 1
        epd_clear_area(epd_full_screen());
        epd_copy_to_framebuffer(imageArea, (uint8_t *) ImageResource_cat1_600x400, framebuffer);
        break;
      case 6:
        epd_clear_area(epd_full_screen());
        cursor_x = 10;
        cursor_y = 200;
        write_string((GFXfont *)&FiraSans, (char *)loveArray[1], &cursor_x, &cursor_y, framebuffer);
        break;
      case 7:
        epd_clear_area(epd_full_screen());
        cursor_x = 10;
        cursor_y = 200;
        write_string((GFXfont *)&FiraSans, (char *)dateArray[1], &cursor_x, &cursor_y, framebuffer);
        break;
      case 8:
        epd_clear_area(epd_full_screen());
        write_string((GFXfont *)&FiraSans, (char *)couponArray[1], &cursor_x, &cursor_y, framebuffer);
        break;
      case 9:
        epd_clear_area(epd_full_screen());
        // ask for voice note 2
        cursor_x = 10;
        cursor_y = 200;
        write_string((GFXfont *)&FiraSans, (char *)voiceMessage, &cursor_x, &cursor_y, framebuffer);
        break;
      case 10:
        // Christmas pic 2
        epd_clear_area(epd_full_screen());
        epd_copy_to_framebuffer(imageArea, (uint8_t *) ImageResource_cat2_600x400, framebuffer);
        break;
      case 11:
        epd_clear_area(epd_full_screen());
        cursor_x = 10;
        cursor_y = 200;
        write_string((GFXfont *)&FiraSans, (char *)loveArray[2], &cursor_x, &cursor_y, framebuffer);
        break;
      case 12:
        epd_clear_area(epd_full_screen());
        cursor_x = 10;
        cursor_y = 200;
        write_string((GFXfont *)&FiraSans, (char *)dateArray[2], &cursor_x, &cursor_y, framebuffer);
        break;
      case 13:
        epd_clear_area(epd_full_screen());
        write_string((GFXfont *)&FiraSans, (char *)couponArray[2], &cursor_x, &cursor_y, framebuffer);
        break;
      case 14:
        epd_clear_area(epd_full_screen());
        // ask for voice note 3
        cursor_x = 10;
        cursor_y = 200;
        write_string((GFXfont *)&FiraSans, (char *)voiceMessage, &cursor_x, &cursor_y, framebuffer);
        break;
      case 15:
        epd_clear_area(epd_full_screen());
        epd_copy_to_framebuffer(imageArea, (uint8_t *) ImageResource_cat3_600x400, framebuffer);
        // Christmas pic 3
        break;
      case 16:
        epd_clear_area(epd_full_screen());
        cursor_x = 10;
        cursor_y = 200;
        write_string((GFXfont *)&FiraSans, (char *)loveArray[3], &cursor_x, &cursor_y, framebuffer);
        break;
      case 17:
        epd_clear_area(epd_full_screen());
        cursor_x = 10;
        cursor_y = 200;
        write_string((GFXfont *)&FiraSans, (char *)dateArray[3], &cursor_x, &cursor_y, framebuffer);
        break;
      case 18:
        epd_clear_area(epd_full_screen());
        write_string((GFXfont *)&FiraSans, (char *)couponArray[3], &cursor_x, &cursor_y, framebuffer);
        break;
      case 19:
        epd_clear_area(epd_full_screen());
        // ask for voice note 4
        cursor_x = 10;
        cursor_y = 200;
        write_string((GFXfont *)&FiraSans, (char *)voiceMessage, &cursor_x, &cursor_y, framebuffer);
        break;
      case 20:
        // Christmas pic 5
        epd_clear_area(epd_full_screen());
        //epd_copy_to_framebuffer(imageArea, (uint8_t *) ImageResource_cat4_600x400, framebuffer);
        write_string((GFXfont *)&FiraSans, "Image didnt fit, message me for it :)", &cursor_x, &cursor_y, framebuffer);
        break;
      case 21:
        epd_clear_area(epd_full_screen());
        cursor_x = 10;
        cursor_y = 200;
        write_string((GFXfont *)&FiraSans, (char *)loveArray[4], &cursor_x, &cursor_y, framebuffer);
        break;
      case 22:
        epd_clear_area(epd_full_screen());
        cursor_x = 10;
        cursor_y = 200;
        write_string((GFXfont *)&FiraSans, (char *)dateArray[4], &cursor_x, &cursor_y, framebuffer);
        break;
      case 23:
        epd_clear_area(epd_full_screen());
        write_string((GFXfont *)&FiraSans, (char *)couponArray[4], &cursor_x, &cursor_y, framebuffer);
        break;
      case 24:
        epd_clear_area(epd_full_screen());
        // ask for voice note 5
        cursor_x = 10;
        cursor_y = 200;
        write_string((GFXfont *)&FiraSans, (char *)voiceMessage, &cursor_x, &cursor_y, framebuffer);
        break;
      case 25:

        // Christmas pic 5
        epd_clear_area(epd_full_screen());
        write_string((GFXfont *)&FiraSans, "Image didnt fit, message me for it :)", &cursor_x, &cursor_y, framebuffer);
        //epd_copy_to_framebuffer(imageArea, (uint8_t *) ImageResource_cat5_600x400, framebuffer);
        break;
      default:
        // TODO
        break;
          
    } 
    epd_draw_grayscale_image(epd_full_screen(), framebuffer);
    shutoff(); 
  }
  delay(10);
}
