#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>

#define buttonRedPin A0
#define buttonBluePin A1
#define buttonGreenPin A2
#define buttonYellowPin A3

#define redLedPin   2
#define greenLedPin 3
#define blueLedPin  4

#define buzzerPin 5

#define TFT_SCL 13
#define TFT_SDA 11
#define TFT_RST 12
#define TFT_DC  10
#define TFT_CS  9
#define TFT_BLK 8

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// melody
int melody[] = {
  262, 262, 0, 262, // C C pauza C
  0, 208, 262, 0,   // pauza G C pauza
  330, 0, 392, 0,   // E pauza G pauza
  262, 0, 349, 0,   // C pauza F pauza
  330, 0, 294, 0    // E pauza D pauza
};

unsigned int noteDuration = 150; // durata per nota (mai rapidă decât înainte)
unsigned long previousMillisMusic = 0;
unsigned int noteIndex = 0;
unsigned int pitch = 1;
unsigned int speed = 1;

// color picker
unsigned long previousMillisColor = 0;
unsigned int pinChoices[] = {redLedPin, greenLedPin, blueLedPin};
unsigned int choice = 0;

// states
#define NOT_STARTED 0
#define PICKING 1
#define GUESSING 2
unsigned int current_state = NOT_STARTED;

void setup()
{
  // init buttons
  pinMode(buttonRedPin, INPUT_PULLUP);
  pinMode(buttonBluePin, INPUT_PULLUP);
  pinMode(buttonGreenPin, INPUT_PULLUP);
  pinMode(buttonYellowPin, INPUT_PULLUP);

  // init leds
  pinMode(redLedPin, OUTPUT);
  pinMode(greenLedPin, OUTPUT);
  pinMode(blueLedPin, OUTPUT);

  // init buzzer
  pinMode(buzzerPin, OUTPUT);

  tft.initR(INITR_18BLACKTAB);  // Folosește INITR_BLACKTAB pentru ecran 128x160
  tft.fillScreen(ST77XX_BLACK);  // Umple ecranul cu culoarea neagră
  tft.setTextColor(ST77XX_WHITE);  // Culoare text alb
  tft.setTextSize(1);  // Dimensiunea textului
  tft.setCursor(10, 10);  // Poziționează cursorul
  tft.print("Hello, World!\n Si cu tini uai?");  // Afișează textul

  Serial.begin(9600);
}

void loop()
{
  
  if (digitalRead(buttonRedPin) == HIGH) { 
    tone(buzzerPin, 262); // C4
    pitch = 1;
    speed = 1;
  } 
  else if (digitalRead(buttonBluePin) == HIGH) { 
    tone(buzzerPin, 330); // E4
    pitch = 2;
  }
  else if (digitalRead(buttonGreenPin) == HIGH) { 
    tone(buzzerPin, 392); // G4
    pitch = 3;
  }
  else if (digitalRead(buttonYellowPin) == HIGH) { 
    tone(buzzerPin, 523); // C5
    pitch = 4;
    speed = 2;
  }

  unsigned long currentMillis = millis();

  PlayMusic(currentMillis);

  switch (current_state)
  {
  case NOT_STARTED:
    /* code */
    break;
  
  default:
    break;
  }

  if (currentMillis - previousMillisColor >= 1000) { 
    // Actualizează timpul pentru următoarea selecție
    previousMillisColor = currentMillis;

    // Stinge toate ledurile înainte de a alege unul nou
    digitalWrite(redLedPin, LOW);
    digitalWrite(greenLedPin, LOW);
    digitalWrite(blueLedPin, LOW);

    // Alege un pin aleatoriu
    choice = random(0, 3); // Alegere între 0 și 2

    // Aprinde ledul corespunzător
    digitalWrite(pinChoices[choice], HIGH);
  }
  
  
}

void PlayMusic(unsigned long currentMillis)
{
  if (currentMillis - previousMillisMusic >= noteDuration / speed)
  {
    previousMillisMusic = currentMillis;

    // cântăm următoarea notă
    tone(buzzerPin, pitch * melody[noteIndex]);

    noteIndex++;
    if (noteIndex >= (sizeof(melody) / sizeof(melody[0])))
    {
      noteIndex = 0;
    }
  }
}
