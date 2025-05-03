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

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// SD
#define MISO 12
#define SD_CS 6

// melody
int melody[] = {
  262, 262, 0, 262,
  0, 208, 262, 0,
  330, 0, 392, 0,
  262, 0, 349, 0,
  330, 0, 294, 0
};

int game_over_melody[] = {
  392, 0, 370, 0,
  349, 0, 330, 0,
  294, 0, 262, 0,
  196, 0, 0, 0
};

int *current_melody = melody;
unsigned int current_melody_length = sizeof(melody) / sizeof(int);

unsigned int noteDuration = 150;
unsigned long previousMillisMusic = 0;
unsigned int noteIndex = 0;
unsigned int pitch = 1;
unsigned int speed = 1;

// color picker
unsigned long previousMillisColor = 0;
unsigned int choice_idx = 0;
unsigned int num_choices = 0;
int all_choices[100];

int circle_radius = 30;

int player_choice;
int score;


// states
#define NOT_STARTED 0
#define PICKING 1
#define GUESSING 2
#define GAME_OVER 3
#define RESTARTING 4
unsigned int current_state = NOT_STARTED;

int blink = 0;

// intrerupts
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
    tft.fillScreen(ST77XX_BLACK);
  }

  // restart game
  if (current_state == GAME_OVER && (digitalRead(buttonRedPin) || digitalRead(buttonBluePin) 
  || digitalRead(buttonGreenPin) || digitalRead(buttonYellowPin) == HIGH)) {
    current_state = RESTARTING;
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
  randomSeed(analogRead(A0));

  // init buttons
  pinMode(buttonRedPin, INPUT_PULLUP);
  pinMode(buttonBluePin, INPUT_PULLUP);
  pinMode(buttonGreenPin, INPUT_PULLUP);
  pinMode(buttonYellowPin, INPUT_PULLUP);

  PCICR |= (1 << PCIE1);
  PCMSK1 |= (1 << PCINT8) | (1 << PCINT9) | (1 << PCINT10) | (1 << PCINT11);

  // init buzzer
  pinMode(buzzerPin, OUTPUT);

  SPI.begin();

  // set the cs pins
  pinMode(TFT_CS, OUTPUT);
  pinMode(SD_CS, OUTPUT);

  // turn off the communication
  digitalWrite(TFT_CS, HIGH);
  digitalWrite(SD_CS, HIGH);
  
  // init lcd
  digitalWrite(TFT_CS, LOW);
  tft.initR(INITR_18BLACKTAB);
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);
  digitalWrite(TFT_CS, HIGH);

  // init SD card
  digitalWrite(SD_CS, LOW);
  if (!SD.begin(-1)) {
    Serial.println("Error!");
    return;
  }
  Serial.println("SD initialized");
  digitalWrite(SD_CS, HIGH);

}

void loop()
{
  unsigned long currentMillis = millis();

  PlayMusic(currentMillis);

  switch (current_state) {
  case NOT_STARTED:
    if (currentMillis - previousMillisColor >= 500) {
      previousMillisColor = currentMillis;

      if (blink == 0) {
        tft.setTextColor(ST77XX_BLACK);
        tft.setCursor(15, 30);
        tft.print("Press any button\n\n      to start!");
        tft.setTextColor(ST77XX_WHITE);
        blink = 1;
      } else {
        tft.setCursor(15, 30);
        tft.print("Press any button\n\n      to start!");
        blink = 0;
      }
    }
    break;
  case PICKING:
    if (currentMillis - previousMillisColor >= 500) {
      tft.drawCircle(LCD_WIDTH / 2, LCD_LENGTH / 2, circle_radius, ST77XX_BLACK);
    }
    if (choice_idx < num_choices) {
      ShowColors(currentMillis);
    } else {
      PickNewColor(currentMillis);
    }
    break;
  case GUESSING:
    if (currentMillis - previousMillisColor >= 500) {
      tft.drawCircle(LCD_WIDTH / 2, LCD_LENGTH / 2, circle_radius, ST77XX_BLACK);
    }

    if (choice_idx < num_choices) {
      MakeChoice(currentMillis);
    } else {
      current_state = PICKING;
      choice_idx = 0;
    }
    break;
  case GAME_OVER:
    if (currentMillis - previousMillisColor >= 500) {
      previousMillisColor = currentMillis;

      if (blink == 0) {
        tft.setTextColor(ST77XX_BLACK);
        tft.setCursor(15, 100);
        tft.print("Press any button\n\n      to start!");
        tft.setTextColor(ST77XX_WHITE);
        blink = 1;
      } else {
        tft.setCursor(15, 100);
        tft.print("Press any button\n\n      to start!");
        blink = 0;
      }
    }
    break;
  case RESTARTING:
    choice_idx = 0;
    num_choices = 0;
    current_melody = melody;
    current_melody_length = sizeof(melody) / sizeof(int);
    tft.fillScreen(ST77XX_BLACK);
    
    current_state = PICKING;
  default:
    break;
  }

}

void MakeChoice(unsigned long currentMillis) {
  if (player_choice == -1) return;
  previousMillisColor = currentMillis;

  Serial.println(choice_idx);

  DrawColor(player_choice);
  
  if (player_choice != all_choices[choice_idx++]) {
    current_state = GAME_OVER;
    Serial.println(player_choice);
    Serial.println(all_choices[choice_idx - 1]);

    tone(buzzerPin, 0);
    delay(500);
    
    current_melody = game_over_melody;
    current_melody_length = sizeof(game_over_melody) / sizeof(int);
    noteIndex = 0;

    tft.fillScreen(ST77XX_BLACK);

    tft.setTextSize(2);
    tft.setCursor(10, 30);
    tft.print("GAME OVER");

    char buffer[100];
    snprintf(buffer, sizeof(buffer), "Final score: %d", num_choices - 1);

    tft.setTextSize(1);
    tft.setCursor(20, 70);
    tft.print(buffer);
    
    tft.setCursor(15, 100);
    tft.print("Press any button\n\n      to start!");

    Serial.println("wrong!");
  }

  player_choice = -1;
}

void ShowColors(unsigned long currentMillis) {
  
  if (currentMillis - previousMillisColor >= 1000) {
    previousMillisColor = currentMillis;

    unsigned int color = all_choices[choice_idx++];
    DrawColor(color);
  }
}

void PickNewColor(unsigned long currentMillis) {
  if (currentMillis - previousMillisColor >= 500) {
    tft.drawCircle(LCD_WIDTH / 2, LCD_LENGTH / 2, circle_radius, ST77XX_BLACK);
  }

  if (currentMillis - previousMillisColor >= 1000) {
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
  switch (choice) {
  case 0:
    tft.drawCircle(LCD_WIDTH / 2, LCD_LENGTH / 2, circle_radius, ST77XX_RED);
    break;
  case 1:
    tft.drawCircle(LCD_WIDTH / 2, LCD_LENGTH / 2, circle_radius, ST77XX_GREEN);
    break;
  case 2:
    tft.drawCircle(LCD_WIDTH / 2, LCD_LENGTH / 2, circle_radius, ST77XX_BLUE);
    break;
  case 3:
    tft.drawCircle(LCD_WIDTH / 2, LCD_LENGTH / 2, circle_radius, ST77XX_YELLOW);
    break;
  default:
    break;
  }
}

void PlayMusic(unsigned long currentMillis)
{
  if (currentMillis - previousMillisMusic >= noteDuration / speed) {
    previousMillisMusic = currentMillis;

    tone(buzzerPin, pitch * current_melody[noteIndex]);

    noteIndex++;
    if (noteIndex >= current_melody_length) {
      noteIndex = 0;
    }
  }
}
