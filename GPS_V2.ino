#include <Wire.h>                  //for screen
#include <Adafruit_GFX.h>          //for screen
#include "Adafruit_LEDBackpack.h"  //for screen
#include <string>
#include <SparkFun_u-blox_GNSS_v3.h>
#include <atomic>

#define outArrSize 100
#define DISPLAY_ADDRESS 0x70  //I2C address of screen
#define SCREEN_BRIGHTNESS 15

//vars for serial communication
String incomingData;
char outData[outArrSize];

//vars for screen
Adafruit_AlphaNum4 disp = Adafruit_AlphaNum4();

//GPS
SFE_UBLOX_GNSS myGNSS;

std::atomic<int> currentSpeedDisplay(0);
int prevSpeed =0;

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


void setup() { //DISPLAY
  //serial setup
  Serial.begin(115200);
  //setup screen
  // Wire.setSCL(17);
  // Wire.setSDA(16);
  Wire.setSCL(5);
  Wire.setSDA(4);
  disp.begin(DISPLAY_ADDRESS);
  disp.setBrightness(SCREEN_BRIGHTNESS);

}

void loop() { //DISPLAY

  int speedDisplay = currentSpeedDisplay.load(std::memory_order_relaxed);
  if (speedDisplay != prevSpeed){
    displayNumber(speedDisplay);
    prevSpeed = speedDisplay;
  }
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
    
    // Update the shared variable for Core 0 to read
    float currentSpeedKmhTemp = speed_mms * 0.036; // prepare number for display in kmh speed * 0.0036 * 10
    int speedInt = currentSpeedKmhTemp;
    currentSpeedDisplay.store(speedInt, std::memory_order_relaxed);
  }
}

