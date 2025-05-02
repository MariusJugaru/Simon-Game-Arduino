#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include <SD.h>

#define buttonRedPin A0
#define buttonGreenPin A1
#define buttonBluePin A2
#define buttonYellowPin A3

// #define redLedPin   2
// #define greenLedPin 3
// #define blueLedPin  4

#define buzzerPin 2

// LCD
#define TFT_SCL 13
#define TFT_SDA_MOSI 11

#define TFT_RST 10
#define TFT_DC  9
#define TFT_CS  8
#define TFT_BLK 7

#define LCD_WIDTH 128
#define LCD_LENGTH 160

// SD
#define MISO 12
#define SD_CS 6

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
// unsigned int pinChoices[] = {redLedPin, greenLedPin, blueLedPin};
// unsigned int colorChoice[] = {0, 1, 2, 3}; // 0 - red, 1 - green, 2 - blue, 3 - yellow
unsigned int choice_idx = 0;
unsigned int num_choices = 0;
int all_choices[100];

int player_choice;


// states
#define NOT_STARTED 0
#define PICKING 1
#define GUESSING 2
#define GAME_OVER 3
unsigned int current_state = NOT_STARTED;

volatile unsigned long lastInterruptTime = 0;
const unsigned long debounceDelay = 200;

ISR(PCINT1_vect) {
  unsigned long currentTime = millis();
  if (currentTime - lastInterruptTime < debounceDelay) {
    return; // ignorăm bouncing-ul
  }
  lastInterruptTime = currentTime;
  
  // start game
  if (current_state == NOT_STARTED && (digitalRead(buttonRedPin) || digitalRead(buttonBluePin) 
  || digitalRead(buttonGreenPin) || digitalRead(buttonYellowPin) == HIGH)) {
    current_state = PICKING;
    choice_idx = 0;
  }

  // make a choice
  if (player_choice == -1 && current_state == GUESSING) {
    if (digitalRead(buttonRedPin) == HIGH) player_choice = 0;
    if (digitalRead(buttonGreenPin) == HIGH) player_choice = 1;
    if (digitalRead(buttonBluePin) == HIGH) player_choice = 2;
    if (digitalRead(buttonYellowPin) == HIGH) player_choice = 3;

    // Serial.println(player_choice);
  }

}

void setup()
{
  Serial.begin(9600);

  // init buttons
  pinMode(buttonRedPin, INPUT_PULLUP);
  pinMode(buttonBluePin, INPUT_PULLUP);
  pinMode(buttonGreenPin, INPUT_PULLUP);
  pinMode(buttonYellowPin, INPUT_PULLUP);

  PCICR |= (1 << PCIE1);
  PCMSK1 |= (1 << PCINT8) | (1 << PCINT9) | (1 << PCINT10) | (1 << PCINT11);

  // init buzzer
  pinMode(buzzerPin, OUTPUT);

  // set the cs pins
  pinMode(TFT_CS, OUTPUT);
  pinMode(SD_CS, OUTPUT);

  // turn off the communication
  digitalWrite(TFT_CS, HIGH);
  digitalWrite(SD_CS, HIGH);

  SPI.begin();

  // init lcd
  digitalWrite(TFT_CS, LOW);  // activăm LCD
  tft.initR(INITR_18BLACKTAB);  // Folosește INITR_BLACKTAB pentru ecran 128x160
  tft.fillScreen(ST77XX_BLACK);  // Umple ecranul cu culoarea neagră
  tft.setTextColor(ST77XX_WHITE);  // Culoare text alb
  tft.setTextSize(1);  // Dimensiunea textului
  tft.setCursor(10, 10);  // Poziționează cursorul
  tft.print("Hello, World!\n Si cu tini uai?");  // Afișează textul
  digitalWrite(TFT_CS, HIGH);

  // init SD card
  // digitalWrite(SD_CS, LOW);
  // if (!SD.begin(SD_CS)) {
  //   Serial.println("Error!");
  //   return;
  // }
  // Serial.println("SD initialized");
  // digitalWrite(SD_CS, HIGH);

}

void loop()
{
  unsigned long currentMillis = millis();

  PlayMusic(currentMillis);

  switch (current_state)
  {
  case NOT_STARTED:
    /* code */
    break;
  case PICKING:
    if (currentMillis - previousMillisColor >= 500)
    {
      tft.drawCircle(LCD_WIDTH / 2, LCD_LENGTH / 2, 20, ST77XX_BLACK);
    }
    if (choice_idx < num_choices) {
      ShowColors(currentMillis);
    } else {
      PickNewColor(currentMillis);
    }
    break;
  case GUESSING:
    if (currentMillis - previousMillisColor >= 500)
    {
      tft.drawCircle(LCD_WIDTH / 2, LCD_LENGTH / 2, 20, ST77XX_BLACK);
    }
    if (choice_idx < num_choices) {
      MakeChoice(currentMillis);
    } else {
      current_state = PICKING;
      choice_idx = 0;
    }
    
    break;
  default:
    break;
  }
}

void MakeChoice(unsigned long currentMillis) {
  if (player_choice == -1) return;
  previousMillisColor = currentMillis;

  Serial.println(choice_idx);

  if (player_choice != all_choices[choice_idx++]) {
    current_state = GAME_OVER;
    Serial.println(player_choice);
    Serial.println(all_choices[choice_idx - 1]);
    Serial.println("wrong!");
  }

  DrawColor(player_choice);
  player_choice = -1;
}

void ShowColors(unsigned long currentMillis) {
  
  if (currentMillis - previousMillisColor >= 1000)
  {
    previousMillisColor = currentMillis;

    unsigned int color = all_choices[choice_idx++];
    DrawColor(color);
  }
}

void PickNewColor(unsigned long currentMillis) {
  if (currentMillis - previousMillisColor >= 500)
  {
    tft.drawCircle(LCD_WIDTH / 2, LCD_LENGTH / 2, 20, ST77XX_BLACK);
  }
  if (currentMillis - previousMillisColor >= 1000)
  {
    previousMillisColor = currentMillis;

    // select a random color, 0 - red, 1 - green, 2 - blue, 3 - yellow
    unsigned int choice = random(0, 4);

    DrawColor(choice);

    all_choices[num_choices++] = choice;
    current_state = GUESSING;

    choice_idx = 0;
    player_choice = -1;
  }
}

void DrawColor(unsigned int choice)
{
  switch (choice)
  {
  case 0:
    tft.drawCircle(LCD_WIDTH / 2, LCD_LENGTH / 2, 20, ST77XX_RED);
    break;
  case 1:
    tft.drawCircle(LCD_WIDTH / 2, LCD_LENGTH / 2, 20, ST77XX_GREEN);
    break;
  case 2:
    tft.drawCircle(LCD_WIDTH / 2, LCD_LENGTH / 2, 20, ST77XX_BLUE);
    break;
  case 3:
    tft.drawCircle(LCD_WIDTH / 2, LCD_LENGTH / 2, 20, ST77XX_YELLOW);
    break;
  default:
    break;
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
