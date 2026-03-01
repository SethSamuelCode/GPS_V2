# High-Speed Dual-Core GPS Speedometer (25Hz)

A highly optimized, zero-latency GPS speedometer built on the **Raspberry Pi Pico (RP2040)**. 

Unlike standard GPS projects that update at a sluggish 1Hz, this project pushes the **u-blox M10S** chip to its absolute hardware limit of **25Hz**. To prevent screen-tearing and I2C bus collisions, the code utilizes a true **Dual-Core Architecture**, leveraging thread-safe atomic variables to pass data instantly between the GPS polling core and the LED display core.

Additionally, it features an intelligent **Auto-Brightness System** that calculates the physical position of the sun in the sky based on your GPS coordinates and time, dimming the screen seamlessly at sunset!

##  Key Features
* **Ultra-Fast 25Hz Tracking:** Disables secondary constellations (GLONASS, Galileo, etc.) to force the u-blox M10S into its maximum 25Hz (40ms) update rate.
* **Dual-Core Architecture:** 
  * **Core 1** handles the blocking 25Hz I2C GPS polling and heavy floating-point solar math.
  * **Core 0** runs an unthrottled, zero-delay event loop to update the screen the exact microsecond new data arrives.
* **Thread-Safe Memory:** Uses lock-free `std::atomic<int>` variables for 1-clock-cycle data transfers between cores.
* **Astrological Auto-Brightness:** Uses the `SolarCalculator` to track the sun's elevation angle, adjusting the Adafruit LED matrix through 3 stages (High, Medium, Low) based on dawn/dusk.
* **Smart Animation:** Throttled loading animation displays when waiting for a 3D GPS fix, preventing I2C bus saturation.
* **Hardware Watchdog:** Includes a 2-second hardware watchdog timer (`rp2040.wdt`) to automatically reboot the system if a core hangs.

##  Hardware Requirements
* **Microcontroller:** Raspberry Pi Pico (RP2040)
* **GPS Module:** u-blox M10S GNSS Receiver
* **Display:** Adafruit 4-Digit 14-Segment Alphanumeric Display (with HT16K33 I2C Backpack)
* **Power Supply:** 3.3V regulated power source (can be powered from the Pico's 3V3 pin)

##  Wiring & Pinout
Because of the dual-core architecture, the GPS and Display are kept on entirely separate hardware I2C buses. This prevents collisions when both cores attempt to use I2C simultaneously.

| Component | RP2040 Pin | Function | Pico I2C Bus |
| :--- | :--- | :--- | :--- |
| **Adafruit Display** | **Pin 4** (GP4) | SDA | `Wire` (I2C0) |
| **Adafruit Display** | **Pin 5** (GP5) | SCL | `Wire` (I2C0) |
| **u-blox M10S GPS** | **Pin 2** (GP2) | SDA | `Wire1` (I2C1) |
| **u-blox M10S GPS** | **Pin 3** (GP3) | SCL | `Wire1` (I2C1) |
| **Both** | **Pin 36** (3V3) | Power | N/A |
| **Both** | **appropriate ground pin** (GND) | Ground | N/A |

*(Note: Ensure both the GPS and Display are connected to `3V3` and `GND`).*

##  Required Libraries
You will need to install the following libraries via the Arduino Library Manager:

1. **Adafruit GFX Library** by Adafruit
2. **Adafruit LED Backpack Library** by Adafruit
3. **SparkFun u-blox GNSS v3** by SparkFun *(Must be v3 for M10 hardware support)*
4. **SolarCalculator** by JP Meijers
5. **Time** by Michael Margolis (`TimeLib.h`)

##  Installation & Setup

1. **Install the RP2040 Board Core:** 
   This project relies on the [earlephilhower/arduino-pico](https://github.com/earlephilhower/arduino-pico) core to support `setup1()` and `loop1()`. Ensure you have this installed in your Arduino IDE Board Manager, **not** the official Arduino mbed core.
2. Clone this repository or copy the `.ino` file into a new Arduino sketch.
3. Wire the components according to the table above.
4. Select **Raspberry Pi Pico** in the Arduino Tools menu.
5. Compile and upload!

##  Code Architecture Notes
### Why no `delay()`?
You will notice there is not a single `delay()` in either `loop()`. The code is entirely **Event-Driven**. 
* Core 0 only pushes an I2C command to the screen if the calculated integer has actually changed.
* Core 1 only processes solar math once every 60 seconds, and only polls the GPS when `getPVT()` returns true. 
This means the Pico spends most of its time running the loop at millions of iterations per second, ensuring absolute minimum latency when data *does* change.

### The Speed Math
The u-blox chip outputs speed in millimeters per second (`mm/s`).
To convert to km/h, we multiply by `0.0036`. However, to display one decimal place on the 4-digit screen, we need the final value multiplied by 10. 
Therefore, the code uses `speed_mms * 0.036` to combine the unit conversion and the decimal shift into one efficient calculation.

##  License
## 📄 License
This project is licensed under the **GNU General Public License v3.0 (GPL-3.0)**. 
See the `LICENSE` file for details. You are free to use, modify, and distribute this software, provided that any derivative works are also open-source under the same license.