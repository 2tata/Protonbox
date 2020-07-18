#include <bluefairy.h>
#include <Adafruit_NeoPixel.h>
#include <ShiftRegister74HC595.h>

//Pin connected to BC547C Transistor for FAN PWM
const uint8_t FAN_PIN = D1;

//Right LED-strip data pin
const uint8_t LED_PIN_R = D2;
//Left LED-strip data pin
const uint8_t LED_PIN_L = D3;

const uint8_t POWER_BUTTON = D4;
const uint8_t POWER_LED = D5;

//Pin connected to latch pin (ST_CP) of 74HC595
const uint8_t latchPin = D6;
//Pin connected to clock pin (SH_CP) of 74HC595
const uint8_t clockPin = D7;
////Pin connected to Data in (DS) of 74HC595
const uint8_t dataPin = D8;

// How many NeoPixels are attached to the Arduino?
#define PIXELTYPE uint8_t
const PIXELTYPE NUMPIXELS = 30;

// number of shift registers attached in series
const uint8_t numberOfShiftRegisters = 2;

Adafruit_NeoPixel stripL(NUMPIXELS, LED_PIN_L, NEO_RGB + NEO_KHZ800);
Adafruit_NeoPixel stripR(NUMPIXELS, LED_PIN_R, NEO_RGB + NEO_KHZ800);

ShiftRegister74HC595<numberOfShiftRegisters> sr(dataPin, clockPin, latchPin);

bluefairy::Scheduler scheduler;

void clearStrip(Adafruit_NeoPixel * myStrip) {
  for ( PIXELTYPE i = 0; i < NUMPIXELS; i++) {
    myStrip->setPixelColor(i, 0x000000);
    myStrip->show();
  }
}

void rainbow(unsigned long& firstPixelHue, Adafruit_NeoPixel* myStrip) {
  if (firstPixelHue < 5 * 65536) {
    for (int i = 0; i < myStrip->numPixels(); i++) { // For each pixel in strip...
      int pixelHue = firstPixelHue + (i * 65536L / myStrip->numPixels());
      myStrip->setPixelColor(i, myStrip->gamma32(myStrip->ColorHSV(pixelHue)));
    }
    myStrip->show(); // Update strip with new contents
    firstPixelHue += 256;
  } else {
    firstPixelHue = 0;
  }
  //  return firstPixelHue;
}

uint32_t dimColor(uint32_t color, uint8_t width) {
  return (((color & 0xFF0000) / width) & 0xFF0000) + (((color & 0x00FF00) / width) & 0x00FF00) + (((color & 0x0000FF) / width) & 0x0000FF);
}

uint32_t old_val[NUMPIXELS];

//TODO evtl. count inc/dec in else section
void knightRider(uint8_t& direct, uint8_t& wait_count, PIXELTYPE& count, uint32_t& color, uint32_t* old_valR, uint8_t& width, Adafruit_NeoPixel * myStrip) {
  if (wait_count < 5) {
    wait_count++;
  } else {
    wait_count = 0;
    if (direct == 0) {
      myStrip->setPixelColor(count, color);
      old_val[count] = color;
      for (PIXELTYPE x = count; x > 0; x--) {
        old_val[x - 1] = dimColor(old_val[x - 1], width);
        myStrip->setPixelColor(x - 1, old_val[x - 1]);
      }
      myStrip->show();
      count++;
      if (count >= NUMPIXELS) {
        count = NUMPIXELS - 1;
        direct = 1;
      }
    } else {
      myStrip->setPixelColor(count, color);
      old_val[count] = color;
      for (PIXELTYPE x = count; x <= NUMPIXELS ; x++) {
        old_val[x - 1] = dimColor(old_val[x - 1], width);
        myStrip->setPixelColor(x + 1, old_val[x + 1]);
      }
      myStrip->show();
      count--;
      if (count <= 0) {
        count = 1;
        direct = 0;
      }
    }
  }
}

void fading(uint8_t& brightness, int8_t& fadeAmount, uint8_t& wait_count) {
  // wait for 30 milliseconds to see the dimming effect
  if (wait_count < 3) {
    wait_count++;
  } else {
    wait_count = 0;
    analogWrite(POWER_LED, brightness);

    // change the brightness for next time through the loop:
    brightness = brightness + fadeAmount;

    // reverse the direction of the fading at the ends of the fade:
    if (brightness <= 0 || brightness >= 255) {
      fadeAmount = - fadeAmount;
    }
  }
}

// 0: init state everyting is off.
// 1: start state call start sequents
// 2: run state aktive state while all boards running.
// 3: buttong is more than 10 sec pushed force off all boards.
int state = 0;

uint8_t clearBeforR = 0b1000;
uint8_t directR = 0;
uint8_t waitRcount = 0;
PIXELTYPE countR = 1;
uint32_t colorR = 0x00FF00;
uint32_t old_valR[NUMPIXELS]; // up to 256 lights!
uint8_t widthR = 5;
unsigned long firstPixelHueR = 0;

//TODO: clearBeforR bit maste prüfen.
//TODO: shutdown?
void StripeR() {
  switch (state) {
    case 0:
      // 0: init state everyting is off.
      if (clearBeforR && 0b1000 == 1) {
        clearStrip(&stripR);       // Turn OFF all pixels ASAP
        clearBeforR = 0b0100;
      }
      stripR.setBrightness(255);
      // Larson time baby!
      knightRider(directR, waitRcount, countR, colorR, &old_valR[0], widthR, &stripR);
      break;

    case 1:
      // 1: start state call start sequents
      if (clearBeforR == 0) {
        clearStrip(&stripR);       // Turn OFF all pixels ASAP
        clearBeforR = 1;
      }
      //TODO: start sequence for each board green LEDs behind board aktivieren.
      break;

    case 2:
      // 2: run state aktive state while all boards running.
      if (clearBeforR == 0) {
        clearStrip(&stripR);       // Turn OFF all pixels ASAP
        clearBeforR = 1;
      }
      stripR.setBrightness(50);
      rainbow(firstPixelHueR, &stripR);
      break;

    case 3:
      // 3: buttong is more than 10 sec pushed force off all boards.
      //TODO: countdown? 30 LEDs == 30sec?
      state = 0;
      break;
  }
}

uint8_t clearBeforL = 0;
uint8_t directL = 0;
uint8_t waitLcount = 0;
PIXELTYPE countL = 1;
uint32_t colorL = 0x00FF00;
uint32_t old_valL[NUMPIXELS]; // up to 256 lights!
uint8_t widthL = 5;
unsigned long firstPixelHueL = 0;

void StripeL() {
  switch (state) {
    case 0:
      // 0: init state everyting is off.
      if (clearBeforL == 1) {
        clearStrip(&stripL);       // Turn OFF all pixels ASAP
        clearBeforL = 0;
      }
      stripL.setBrightness(255);
      // Larson time baby!
      knightRider(directL, waitLcount, countL, colorL, &old_valL[0], widthL, &stripL);
      break;

    case 1:
      // 1: start state call start sequents
      if (clearBeforL == 0) {
        clearStrip(&stripL);       // Turn OFF all pixels ASAP
        clearBeforL = 1;
      }
      //TODO: start sequence for each board
      break;

    case 2:
      // 2: run state aktive state while all boards running.
      if (clearBeforL == 0) {
        clearStrip(&stripL);       // Turn OFF all pixels ASAP
        clearBeforL = 1;
      }
      stripL.setBrightness(50);
      rainbow(firstPixelHueL, &stripL);
      break;

    case 3:
      // 3: buttong is more than 10 sec pushed force off all boards.
      //TODO: countdown? 30 LEDs == 30sec?
      break;
  }
}

// TODO: task intervall could be way higher instead of 10?
void FanControl() {
  switch (state) {
    case 0:
      // 0: init state everyting is off.
      digitalWrite(FAN_PIN, HIGH);
      break;

    case 1:
      // 1: start state call start sequents
      //TODO: start sequence for each board
      //TODO: Whats the minimum PWM that the FANs still rotate?
      //TODO: map 11 steps from the minimum to the maximum.
      digitalWrite(FAN_PIN, LOW);
      break;

    case 2:
      // 2: run state aktive state while all boards running.
      digitalWrite(FAN_PIN, LOW);
      break;

    case 3:
      // 3: button is more than 10 sec pushed force off all boards.
      //TODO: countdown? 30 LEDs == 30sec?
      digitalWrite(FAN_PIN, HIGH);
      break;
  }
}

uint8_t brightness = 0;    // how bright the LED is
int8_t fadeAmount = 5;    // how many points to fade the LED by
uint8_t LED_wait = 0;
uint32_t state1_wait = 0;
uint8_t flip = LOW;

// TODO: task intervall could be 30 instead of 10?
void PoweLEDcontrol() {
  switch (state) {
    case 0:
      // 0: init state everyting is off.
      fading(brightness, fadeAmount, LED_wait);
      break;
    case 1:
      // 1: start state call start sequents
      //TODO: start sequence for each board
      //TODO: LED blinking until start sequenz ends?
      if (state1_wait < 500) {
        state1_wait++;
      } else {
        digitalWrite(POWER_LED, flip);
        flip=~flip; //TODO prüfen ob das geht 
      }
      break;

    case 2:
      // 2: run state aktive state while all boards running.
      digitalWrite(POWER_LED, HIGH);  // LED on
      break;

    case 3:
      // 3: button is more than 10 sec pushed force off all boards.
      //TODO: countdown? 30 LEDs == 30sec?
      digitalWrite(POWER_LED, LOW); // LED off
      break;
  }
}

// TODO: task intervall could be way higher instead of 10?
void BoardControl() {
  switch (state) {
    case 0:
      // 0: init state everyting is off.
      sr.setAllLow(); // set all pins LOW
      break;
    case 1:
      // 1: start state call start sequents
      //TODO: start sequence for each board
      //TOTO: every 3 seconds one board == 33sec start sequence?
      state = 2;
      break;

    case 2:
      // 2: run state aktive state while all boards running.
      sr.setAllLow(); // set all pins LOW
      break;

    case 3:
      // 3: button is more than 10 sec pushed force off all boards.
      //TODO: countdown? 30 LEDs == 30sec?
      sr.setAllHigh(); // set all pins LOW
      state = 0;
      break;
  }
}

int buttonPush = 0;
int buttonState = 0;         // current state of the button
int lastButtonState = 0;     // previous state of the button

void ButtonControl() {
  // read the pushbutton input pin:
  buttonState = digitalRead(POWER_BUTTON);

  // compare the buttonState to its previous state
  if (buttonState != lastButtonState) {
    // if the state has changed, increment the counter
    if (buttonState == LOW) {
      // if the current state is LOW then the button went from off to on:
      buttonPush = !buttonPush;
    } else {
      // if the current state is HIGH then the button went from on to off:
    }
    // Delay a little bit to avoid bouncing
    delay(50);
  }
  // save the current state as the last state, for next time through the loop
  lastButtonState = buttonState;
}

void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(FAN_PIN, OUTPUT);
  pinMode(POWER_BUTTON, INPUT);
  pinMode(POWER_LED, OUTPUT);

  // set initial state, LED off
  digitalWrite(POWER_LED, buttonState);
  sr.setAllLow(); // set all pins LOW

  digitalWrite(FAN_PIN, LOW);


  stripR.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  clearStrip(&stripR);       // Turn OFF all pixels ASAP
  stripR.setBrightness(0);  // Set BRIGHTNESS to 0 (max = 255)
  stripL.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  clearStrip(&stripL);       // Turn OFF all pixels ASAP
  stripL.setBrightness(0);  // Set BRIGHTNESS to 0 (max = 255)

  scheduler.every(10, StripeR);
  scheduler.every(10, StripeL);
  scheduler.every(10, FanControl);
  scheduler.every(10, PoweLEDcontrol);
  scheduler.every(10, BoardControl);
  scheduler.every(10, ButtonControl);
}

void loop() {
  scheduler.loop();
}
