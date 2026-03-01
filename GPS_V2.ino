#include <Wire.h>                  //for screen
#include <Adafruit_GFX.h>          //for screen
#include "Adafruit_LEDBackpack.h"  //for screen
#include <string>
#include <SparkFun_u-blox_GNSS_v3.h>
#include <atomic>
#include <SolarCalculator.h>  //for determining screen brightness
#include <TimeLib.h>

#define outArrSize 100
#define DISPLAY_ADDRESS 0x70  //I2C address of screen
#define HIGH_SCREEN_BRIGHTNESS 15
#define MEDIUM_SCREEN_BRIGHTNESS 5
#define LOW_SCREEN_BRIGHTNESS 0
#define SUN_ANGLE_ABOVE_HORIZON 2

//vars for screen
Adafruit_AlphaNum4 disp = Adafruit_AlphaNum4();
unsigned long timeOfLastFix = 0UL; 
unsigned long sunCalcTimer = 60001; //update the screen brightness on first fix
int prevBrightness = -1;
//GPS
SFE_UBLOX_GNSS myGNSS;

std::atomic<int> currentSpeedDisplay(0);
std::atomic<int> gpsFixType(0);
std::atomic<int> targetScreenBrightness(LOW_SCREEN_BRIGHTNESS);
int prevSpeed =0;
int prevWaitStage = -1;

void displayNumber(int temp){
    disp.clear();
    // int temp = speed * 10;
    // int temp = rand();
    if (temp < 10) { temp = 0; }  // show 0 when stopped and not noise.
    int digit0 = temp / 1000;
    int digit1 = (temp / 100) - (digit0 * 10);
    int digit2 = (temp / 10) - (digit0 * 100) - (digit1 * 10);
    int digit3 = temp - (digit0 * 1000) - (digit1 * 100) - (digit2 * 10);
    if (temp >= 1000) {  //dont show leading 0 if going under 100km/h
      disp.writeDigitAscii(0, 48 + digit0);
    }
    if (temp >= 100) {  //dont show 10s digit if less then 10km/h
      disp.writeDigitAscii(1, 48 + digit1);
    }
    disp.writeDigitAscii(2, 48 + digit2, true);
    disp.writeDigitAscii(3, 48 + digit3);
    disp.writeDisplay();
}

void displayWaiting(){
    unsigned long currentTime = millis();
    unsigned long timeSinceLastFix = currentTime-timeOfLastFix;
    // Divide by 1000 to get animation stages (0, 1, 2)
    int flashStage = (timeSinceLastFix / 1000) % 3;
    
    // ONLY write to I2C if the animation frame has actually changed
    if (flashStage != prevWaitStage) {
      disp.clear();
      for (int x = 0; x < 4; x++) {
        if (flashStage == 0) {
          disp.writeDigitAscii(x, 3U);  // bottom
        } else if (flashStage == 1) {
          disp.writeDigitRaw(x, 192u);  // middle
        } else if (flashStage == 2) {
          disp.writeDigitRaw(x, 1);     // top
        }
      }
      disp.writeDisplay(); 
      prevWaitStage = flashStage;
  }
}

void findSunHeight(){
  double az = 0;
  double el = 0;
 
  setTime(myGNSS.getHour(), myGNSS.getMinute(), myGNSS.getSecond(), 
          myGNSS.getDay(), myGNSS.getMonth(), myGNSS.getYear());
  time_t timeUtc = now();
  double lat = myGNSS.getLatitude();
  lat = lat / 10000000.0;
  double lng = myGNSS.getLongitude();
  lng = lng / 10000000.0;
  calcHorizontalCoordinates(timeUtc, lat, lng, az, el);

  int calculatedBrightness = LOW_SCREEN_BRIGHTNESS;

  if (el > SUN_ANGLE_ABOVE_HORIZON) {
    calculatedBrightness = HIGH_SCREEN_BRIGHTNESS;
  } else if ((el <= SUN_ANGLE_ABOVE_HORIZON) && (el > CIVIL_DAWNDUSK_STD_ALTITUDE)) {
    calculatedBrightness = MEDIUM_SCREEN_BRIGHTNESS;
  } 
  
  targetScreenBrightness.store(calculatedBrightness, std::memory_order_relaxed);

}


void setup() { //DISPLAY
  //serial setup
  Serial.begin(115200);
  //setup screen
  Wire.setSCL(5);
  Wire.setSDA(4);
  Wire.begin();
  disp.begin(DISPLAY_ADDRESS);
  disp.setBrightness(targetScreenBrightness.load(std::memory_order_relaxed));

  rp2040.wdt_begin(2000); //start watchdog timer. 2 second timer. 

}

void loop() { //DISPLAY
  //set brightness if it changed
  int reqBrightness = targetScreenBrightness.load(std::memory_order_relaxed);
  if (reqBrightness != prevBrightness){
    disp.setBrightness(reqBrightness);
    prevBrightness = reqBrightness;
  }

  if (gpsFixType.load()>0){
    timeOfLastFix = millis(); //reload wait time for wait screen
    int speedDisplay = currentSpeedDisplay.load(std::memory_order_relaxed);
    if (speedDisplay != prevSpeed){
      displayNumber(speedDisplay);
      prevSpeed = speedDisplay;
    }
  } else {
    displayWaiting();
    prevSpeed= -1;
  }

  rp2040.wdt_reset();
}

void setup1() {
  Wire1.setSDA(2);
  Wire1.setSCL(3);
  Wire1.begin();
  Wire1.setClock(400000); // 400kHz required for 25Hz data

  // Wait a moment for Core 0's Serial to initialize
  delay(1000); 

  if (myGNSS.begin(Wire1) == false) { // Explicitly tell it to use Wire
    Serial.println("u-blox GNSS not detected!");
    while (1); // Freeze Core 1 (Core 0 will keep running!)
  }

  myGNSS.setI2COutput(COM_TYPE_UBX); 
  myGNSS.saveConfigSelective(VAL_CFG_SUBSEC_IOPORT);

  // Disable extra constellations to unlock 25Hz
  myGNSS.setVal8(UBLOX_CFG_SIGNAL_GPS_ENA, 1);
  myGNSS.setVal8(UBLOX_CFG_SIGNAL_GLO_ENA, 0);
  myGNSS.setVal8(UBLOX_CFG_SIGNAL_GAL_ENA, 0);
  myGNSS.setVal8(UBLOX_CFG_SIGNAL_BDS_ENA, 0);
  myGNSS.setVal8(UBLOX_CFG_SIGNAL_QZSS_ENA, 0);
  myGNSS.setVal8(UBLOX_CFG_SIGNAL_SBAS_ENA, 0);

  // Set to 25Hz
  if (myGNSS.setNavigationFrequency(25)) {
    Serial.println("Core 1: GPS locked at 25Hz!");
  } else {
    Serial.println("Core 1: GPS rejected 25Hz command.");
  }
}

void loop1() {
  if (myGNSS.getPVT()) {
    long speed_mms = myGNSS.getGroundSpeed();
    int tempGpsFixType = myGNSS.getFixType();
    
    // Update the shared variable for Core 0 to read
    float currentSpeedKmhTemp = speed_mms * 0.036; // prepare number for display in kmh speed * 0.0036 * 10
    int speedInt = currentSpeedKmhTemp;

    currentSpeedDisplay.store(speedInt, std::memory_order_relaxed);
    gpsFixType.store(tempGpsFixType,std::memory_order_relaxed);
  
    if ((tempGpsFixType > 0)&&(millis() - sunCalcTimer > 60000)){//fire every min but only if gps signal
      sunCalcTimer = millis();
      findSunHeight();
    }
  }
}

