#include <Arduino.h>
#include <PxMatrix.h>
#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

//wifi
String ssid = "xxx"; 
String password = "yyy"; 
WiFiClient wifiClient;

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "id.pool.ntp.org", 25200);

uint8_t scheduller=5;
uint32_t prevmill;//previous millis
boolean slide = true;

// Pins for LED MATRIX
#ifdef ESP32

#define P_LAT 22
#define P_A 19
#define P_B 23
#define P_C 18
#define P_D 5
#define P_E 15
#define P_OE 16
hw_timer_t * timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

#endif

#ifdef ESP8266

#include <Ticker.h>
Ticker display_ticker;
#define P_LAT 16
#define P_A 5
#define P_B 4
#define P_C 15
#define P_D 12
#define P_E 0
#define P_OE 2

#endif

#define matrix_width 32
#define matrix_height 16
uint8_t display_draw_time=10; //30-70 is usually fine
PxMATRIX display(32,16,P_LAT, P_OE,P_A,P_B,P_C);

// Some standard colors
uint16_t myRED = display.color565(255, 0, 0);
uint16_t myGREEN = display.color565(0, 255, 0);
uint16_t myBLUE = display.color565(0, 0, 255);
uint16_t myWHITE = display.color565(255, 255, 255);
uint16_t myYELLOW = display.color565(255, 255, 0);
uint16_t myCYAN = display.color565(0, 255, 255);
uint16_t myMAGENTA = display.color565(255, 0, 255);
uint16_t myROSE = display.color565(255, 100, 255);
uint16_t myBLACK = display.color565(0, 0, 0);

#ifdef ESP8266
// ISR for display refresh
void display_updater(){
  display.display(display_draw_time);
}
#endif

#ifdef ESP32
void IRAM_ATTR display_updater(){
  // Increment the counter and set the time of ISR
  portENTER_CRITICAL_ISR(&timerMux);
  display.display(display_draw_time);
  portEXIT_CRITICAL_ISR(&timerMux);
}
#endif


void display_update_enable(bool is_enable){

#ifdef ESP8266
  if (is_enable)
    display_ticker.attach(0.004, display_updater);
  else
    display_ticker.detach();
#endif

#ifdef ESP32
  if (is_enable){
    timer = timerBegin(0, 80, true);
    timerAttachInterrupt(timer, &display_updater, true);
    timerAlarmWrite(timer, 4000, true);
    timerAlarmEnable(timer);
  }
  else{
    timerDetachInterrupt(timer);
    timerAlarmDisable(timer);
  }
#endif
}

// ========= wifi =========
void wifi(){
  WiFi.persistent(false);
  delay(1000);
  WiFi.disconnect();
  delay(1000);
  WiFi.mode(WIFI_OFF);
  delay(1*1000); //3*1000
  WiFi.mode(WIFI_STA);  
  delay(1*1000); //3*1000

  //searching wifi
  uint8_t trying=0;
  bool found=false;
  while (!found){
    trying++;
    Serial.print("Scan start ... ");
    int n = WiFi.scanNetworks();
    if (n<=0){
      Serial.println("[ERROR] Network Not Found !!!");
    }else{
      Serial.print(n);
      Serial.println(" network(s) found");
      for (int i = 0; i < n; i++){
        if (WiFi.SSID(i)==ssid){      
          Serial.print(WiFi.SSID(i));
          Serial.print(" found !!!");
          Serial.print("\t");
          Serial.print(WiFi.RSSI(i));
          found=true;
        }
      }  
    }   

    if (trying>=5){
      Serial.println("[ERROR] reboot cause wifi can't found");
      ESP.restart();
    }
    Serial.println();
  }
  //WiFi.scanDelete();  
  delay(1000);
 
  WiFi.begin(ssid.c_str(), password.c_str());

  // Serial.println();
  // Serial.println("WIFI SETTING");
  // WiFi.printDiag(Serial);
  // Serial.println();
  
  //try connect to wifi
  Serial.println("Connecting to Wifi ");
  trying=0;
  while (WiFi.status() != WL_CONNECTED) { 
    trying++; 
    Serial.print(trying);
    Serial.print(" ");
    delay(1000);

    String stateW;
    switch(WiFi.status()){
      case 0:
        stateW="WL_IDLE_STATUS";
        break;
      case 1:
        stateW="WL_NO_SSID_AVAIL";
        break;
      case 2:
        stateW="WL_SCAN_COMPLETED";
        break;
      case 3:
        stateW="WL_CONNECTED";
        break;
      case 4:
        stateW="WL_CONNECT_FAILED";
        break;
      case 5:
        stateW="WL_CONNECTION_LOST";
        break;
      case 6:
        stateW="WL_CONNECT_WRONG_PASSWORD";
        break;
      case 7:
        stateW="WL_DISCONNECTED";
        break;
    } 
    Serial.println(stateW);

    if (trying%10==0){
       Serial.println();
    }
    if (trying>=60){
      Serial.println("[ERROR] reboot cause wifi can't connect");
      ESP.restart();
    }
  }
  
  Serial.println("[INFO] Wifi Connected");
}

String getTime(){
  timeClient.update();
  
  if(!timeClient.update()) {
    timeClient.forceUpdate();
  }
  
   time_t rawtime = timeClient.getEpochTime();
   struct tm * ti;
   ti = localtime (&rawtime);

   uint16_t year = ti->tm_year + 1900;
   String yearStr = String(year);
   yearStr = yearStr.substring(2,4);

   uint8_t month = ti->tm_mon + 1;
   String monthStr = month < 10 ? "0" + String(month) : String(month);

   uint8_t day = ti->tm_mday;
   String dayStr = day < 10 ? "0" + String(day) : String(day);

   uint8_t hours = ti->tm_hour;
   String hoursStr = hours < 10 ? "0" + String(hours) : String(hours);

   uint8_t minutes = ti->tm_min;
   String minuteStr = minutes < 10 ? "0" + String(minutes) : String(minutes);

   uint8_t seconds = ti->tm_sec;
   String secondStr = seconds < 10 ? "0" + String(seconds) : String(seconds);

   return dayStr + "/" + monthStr + "/" + yearStr + "T" +
          hoursStr + ":" + minuteStr + ":" + secondStr;
}

void clock(String hour, String minute, String date, String month){
  display.clearDisplay();
    
  display.setTextColor(myGREEN);
  display.setCursor(2,0);
  display.print(hour);
  display.setTextColor(myRED);
  display.setCursor(13,0);
  display.print(":");
  display.setTextColor(myCYAN);
  display.setCursor(17,0);
  display.print(minute);
     
  display.setTextColor(myYELLOW);
  display.setCursor(2,8);
  display.print(date);
  display.setTextColor(myRED);
  display.setCursor(13,8);
  display.print("/");
  display.setTextColor(myROSE);
  display.setCursor(17,8);
  display.print(month);  
}

void scroll_text(unsigned long scroll_delay, String line1, String line2, uint8_t colorR, uint8_t colorG, uint8_t colorB){
  uint16_t text_length = line1.length();
  display.setTextWrap(false);  // we don't wrap text so it scrolls nicely
  display.setTextSize(1);
  display.setRotation(0);
  display.setTextColor(display.color565(colorR,colorG,colorB));

  // Asuming 5 pixel average character width
  for (int xpos=matrix_width; xpos>-(matrix_width+text_length*5); xpos--){
    display.setTextColor(display.color565(colorR,colorG,colorB));
    
    display.clearDisplay();
    display.setCursor(xpos,0);
    display.println(line1);
    display.setCursor(xpos,8);
    display.println(line2);
    delay(scroll_delay);
    yield();
  
    delay(scroll_delay/5);
    yield();
  }  

}

void setup() {
  Serial.begin(9600);
  delay(1000);
  Serial.println();
  Serial.println("===== START =====");

  //init_display
  display.begin(8);
  display_update_enable(true);
  
  display.clearDisplay();
  scroll_text(50,"Connecting to","         WIFI",96,96,250);
  delay(2000);

  //init_wifi
  wifi();
  
  //init_ntp
  timeClient.begin();
  
  Serial.print("FreeHeap:");
  Serial.println(ESP.getFreeHeap());
  Serial.println("===== START =====");
}

void loop() {
  uint32_t curmill=uint32_t((millis()/1000));
  if ((curmill-prevmill)>=scheduller){ 
    prevmill=curmill;

    String timestamp = getTime();//02/06/2023T00:00:00
    Serial.print(timestamp);
    Serial.print(" [");
    Serial.print(ESP.getFreeHeap());
    Serial.println("] ");
    
    String ymd    = timestamp.substring(0, timestamp.indexOf("T"));
    String date   = ymd.substring(0, ymd.indexOf("/"));
    String month  = ymd.substring(ymd.indexOf("/")+1, ymd.lastIndexOf("/"));
 
    String hms    = timestamp.substring(timestamp.indexOf("T")+1, timestamp.length());
    String hour   = hms.substring(0,hms.indexOf(":"));
    String minute = hms.substring(hms.indexOf(":")+1,hms.lastIndexOf(":"));

    if (slide){
      clock(hour,minute,date,month);
      slide=false;
    }else{
      display.clearDisplay();
      scroll_text(50,hms,ymd,96,96,250);
      slide=true;
    }
  }
}
