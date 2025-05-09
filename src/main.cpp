#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
// #include <TFT_ST7735.h> 
#include <SPI.h>
// #include <SD.h>

// buttons
#define buttonRedPin A0
#define buttonGreenPin A1
#define buttonBluePin A2
#define buttonYellowPin A3

// buzzer
#define buzzerPin 2

// joystick
#define xPin A5
#define yPin A4
#define jsButtonPin 3

int xVal;
int yVal;

// LCD
#define TFT_CS  10
#define TFT_DC  9
#define TFT_RST -1

#define LCD_WIDTH 128
#define LCD_LENGTH 160

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// SD
#include "SdFat.h"
#if SPI_DRIVER_SELECT == 2  // Must be set in SdFat/SdFatConfig.h

// SD_FAT_TYPE = 0 for SdFat/File as defined in SdFatConfig.h,
// 1 for FAT16/FAT32, 2 for exFAT, 3 for FAT16/FAT32 and exFAT.
#define SD_FAT_TYPE 1
//
// Chip select may be constant or RAM variable.
const uint8_t SD_CS_PIN = 4;
//
// Pin numbers in templates must be constants.
const uint8_t SOFT_MISO_PIN = 7;
const uint8_t SOFT_MOSI_PIN = 6;
const uint8_t SOFT_SCK_PIN = 5;

// SdFat software SPI template
SoftSpiDriver<SOFT_MISO_PIN, SOFT_MOSI_PIN, SOFT_SCK_PIN> softSpi;
// Speed argument is ignored for software SPI.
#if ENABLE_DEDICATED_SPI
#define SD_CONFIG SdSpiConfig(SD_CS_PIN, DEDICATED_SPI, SD_SCK_MHZ(0), &softSpi)
#else  // ENABLE_DEDICATED_SPI
#define SD_CONFIG SdSpiConfig(SD_CS_PIN, SHARED_SPI, SD_SCK_MHZ(0), &softSpi)
#endif  // ENABLE_DEDICATED_SPI

#if SD_FAT_TYPE == 0
SdFat sd;
File file;
#elif SD_FAT_TYPE == 1
SdFat32 sd;
File32 file;
#elif SD_FAT_TYPE == 2
SdExFat sd;
ExFile file;
#elif SD_FAT_TYPE == 3
SdFs sd;
FsFile file;
#else  // SD_FAT_TYPE
#error Invalid SD_FAT_TYPE
#endif  // SD_FAT_TYPE


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
int all_choices[101];

int circle_radius = 30;

int player_choice;
int score;

// keyboard & leaderboard

const char *alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
const int alphabetLen = strlen(alphabet);
const int charsPerLine = 6;
const int numLines = strlen(alphabet) / charsPerLine;

int selectIdxX = 0;
int selectIdxY = 0;
int oldIdxX;
int oldIdxY;
int maxIdx = strlen(alphabet) + 2;

int selectChange = 0;
unsigned long lastJoystickMoveMillis = 0;
unsigned long lastJoystickSelectMillis = 0;
const int joystickDelay = 250;
int readySelect = 0;

char name[11] = "";
node_t *list_head = NULL;


// states
#define NOT_STARTED   0
#define PICKING       1
#define GUESSING      2
#define GAME_OVER     3
#define RESTARTING    4
#define KEYBOARD      5
#define LEADERBOARD   6
unsigned int current_state = NOT_STARTED;

int blink = 0;

// intrerupts
volatile unsigned long lastInterruptTime = 0;
const unsigned long debounceDelay = 200;

ISR(PCINT1_vect) {
  unsigned long currentTime = millis();
  if (currentTime - lastInterruptTime < debounceDelay) {
    return; // ignore bounce
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
  if ((current_state == GAME_OVER || current_state == LEADERBOARD) && (digitalRead(buttonRedPin) || digitalRead(buttonBluePin) 
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
  
  while (!Serial);


  xVal = analogRead(xPin);
  randomSeed(xVal);

  // init joystick
  pinMode(xPin, INPUT);
  pinMode(yPin, INPUT);
  pinMode(jsButtonPin, INPUT_PULLUP);

  // init buttons
  pinMode(buttonRedPin, INPUT_PULLUP);
  pinMode(buttonBluePin, INPUT_PULLUP);
  pinMode(buttonGreenPin, INPUT_PULLUP);
  pinMode(buttonYellowPin, INPUT_PULLUP);

  PCICR |= (1 << PCIE1);
  PCMSK1 |= (1 << PCINT8) | (1 << PCINT9) | (1 << PCINT10) | (1 << PCINT11);

  // init buzzer
  pinMode(buzzerPin, OUTPUT);

  // init lcd
  tft.initR(INITR_18BLACKTAB);
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);

  for (int i = 0; i < 10; i++) {
    addNode(&list_head, "000  -  AAAAAAAAAA");
  }
  addNode(&list_head, "001  -  AAAAAAAAAA");
  addNode(&list_head, "010  -  AAAAAAAAAA");
  addNode(&list_head, "005  -  AAAAAAAAAA");

  // init SD card
  // if (!sd.begin(SD_CONFIG)) {
  //   sd.initErrorHalt();
  // }

  // if (!file.open("test_leaderboard.txt", O_RDWR | O_CREAT | O_TRUNC)) {
  //   sd.errorHalt(F("open failed"));
  // }

  // for (int i = 0; i < 10; i++) {
  //   file.println(F("000  -  AAAAAAAAAA"));
  // }
  // file.println(F("001  -  AAAAAAAAAA"));
  // file.println(F("010  -  AAAAAAAAAA"));
  // file.println(F("005  -  AAAAAAAAAA"));
  // file.close();

  // if (!file.open("test_leaderboard.txt", O_RDWR | O_CREAT)){
  //   sd.errorHalt(F("open failed"));
  // }

  // char line[20];
  // memset(line, 0, sizeof(line));

  // while (file.available()) {
  //   char c = file.read();
  //   if (c == '\n') {
  //     addNode(&list_head, line);

  //     memset(line, 0, sizeof(line));
  //   }
  //   else if (c != '\r') {
  //     line[strlen(line)] = c;
  //   }
  // }

  // file.close();

  // printScore();
}

void loadData()
{
  
}

void loop()
{
  unsigned long currentMillis = millis();

  PlayMusic(currentMillis);

  xVal = analogRead(xPin);
  yVal = analogRead(yPin);

  // Serial.println(xVal);
  // Serial.println(yVal);

  if (current_state == NOT_STARTED && digitalRead(jsButtonPin) == LOW){
    // if (!file.open("test_leaderboard.txt", O_RDWR | O_CREAT)) {
    //   sd.errorHalt(F("open failed"));
    // }
    
    // tft.fillScreen(ST7735_BLACK);
    tft.fillScreen(ST7735_BLACK);
    printScore();

    current_state = LEADERBOARD;
  }

  if (current_state == GAME_OVER && digitalRead(jsButtonPin) == LOW) {
    current_melody = melody;
    current_melody_length = sizeof(melody) / sizeof(int);

    drawKeyboard();

    current_state = KEYBOARD;
  }
  
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
    break;
  case KEYBOARD:
    if (digitalRead(jsButtonPin) == HIGH && !readySelect) {
      readySelect = 1;
    }

    if (currentMillis - lastJoystickSelectMillis > joystickDelay) {
      if (digitalRead(jsButtonPin) == LOW && readySelect) {
        lastJoystickSelectMillis = currentMillis;
        readySelect = 0;

        if (selectIdxY * charsPerLine + selectIdxX >= 0 && selectIdxY * charsPerLine + selectIdxX < alphabetLen && (int)strlen(name) < 10) {
          strncat(name, &alphabet[selectIdxY * charsPerLine + selectIdxX], 1);
          
          tft.fillRect(LCD_WIDTH / 4, 20, 100, 30, ST7735_BLACK);
          tft.setCursor(LCD_WIDTH / 4, 20);
          tft.print(name);
        } else if (selectIdxY * charsPerLine + selectIdxX == alphabetLen && (int)strlen(name) > 0) {
          name[strlen(name) - 1] = '\0';

          tft.fillRect(LCD_WIDTH / 4, 20, 100, 30, ST7735_BLACK);
          tft.setCursor(LCD_WIDTH / 4, 20);
          tft.print(name);
        } else if (selectIdxY * charsPerLine + selectIdxX == alphabetLen + 1) {
          current_state = LEADERBOARD;
          name[strlen(name)] = '\0';

          char buffer[20];
          
          snprintf(buffer, sizeof(buffer), "%03d  -  %s", num_choices - 1, name);
          addNode(&list_head, buffer);

          // if (!file.open("test_leaderboard.txt", O_RDWR | O_CREAT | O_APPEND)) {
          //   sd.errorHalt(F("open failed here"));
          // }
          // file.println(buffer);
          // file.close();
          
          // loadData();

          // Serial.println(list_head->player_data);
          // saveScore();

          memset(name, 0, sizeof(name));
          selectIdxX = 0;
          selectIdxY = 0;
          tft.fillScreen(ST7735_BLACK);
          printScore();
        }
      }
    }
    
    if (currentMillis - lastJoystickMoveMillis > joystickDelay) {
      oldIdxX = selectIdxX;
      oldIdxY = selectIdxY;
      
      if (xVal > 800) {
        lastJoystickMoveMillis = currentMillis;
        
        selectIdxX++;
        selectChange = 1;
      } else if (xVal < 150) {
        lastJoystickMoveMillis = currentMillis;
        
        selectIdxX--;
        selectChange = 1;
      } else if (yVal > 800) {
        lastJoystickMoveMillis = currentMillis;
        
        selectIdxY++;
        selectChange = 1;
      } else if (yVal < 150) {
        lastJoystickMoveMillis = currentMillis;
        
        selectIdxY--;
        selectChange = 1;
      }

      if (selectChange) {
        if (selectIdxX < 0) {
          selectIdxX = charsPerLine - 1;
        }else if (selectIdxX >= charsPerLine) {
          selectIdxX = 0;
        }
        
        if (selectIdxY < 0) {
          selectIdxY = numLines;
        } else if (selectIdxY > numLines) {
          selectIdxY = 0;
        }

        // remove old square select
        if (oldIdxY * charsPerLine + oldIdxX < alphabetLen) {
          tft.drawRect(11 + oldIdxX * 18, LCD_LENGTH / 2 + (oldIdxY - 1) * 22 - 3, 14, 14, ST7735_BLACK);
        } else if (oldIdxY * charsPerLine + oldIdxX == alphabetLen) {
          tft.drawRect(14 + oldIdxX * 18, LCD_LENGTH / 2 + (oldIdxY - 1) * 22 - 3, 30, 14, ST7735_BLACK);
        } else if (oldIdxY * charsPerLine + oldIdxX == alphabetLen + 1) {
          tft.drawRect(29 + oldIdxX * 18, LCD_LENGTH / 2 + (oldIdxY - 1) * 22 - 3, 30, 14, ST7735_BLACK);
        }

        while (selectIdxY * charsPerLine + selectIdxX >= maxIdx) {
          selectIdxX--;
        }
        
        // draw new square select
        if (selectIdxY * charsPerLine + selectIdxX < alphabetLen) {
          tft.drawRect(11 + selectIdxX * 18, LCD_LENGTH / 2 + (selectIdxY - 1) * 22 - 3, 14, 14, ST7735_WHITE);
        } else if (selectIdxY * charsPerLine + selectIdxX == alphabetLen) {
          tft.drawRect(14 + selectIdxX * 18, LCD_LENGTH / 2 + (selectIdxY - 1) * 22 - 3, 30, 14, ST7735_WHITE);
        } else if (selectIdxY * charsPerLine + selectIdxX == alphabetLen + 1) {
          tft.drawRect(29 + selectIdxX * 18, LCD_LENGTH / 2 + (selectIdxY - 1) * 22 - 3, 30, 14, ST7735_WHITE);
        }
        selectChange = 0;
      }
    }

    break;
  default:
    break;
  }

}

void printScore()
{
  int i = 0;
  tft.setCursor(0, 5);
  node_t *p = list_head;
  while (p != NULL && i < 10) {
    tft.print(" ");
    tft.println(p->player_data);
    tft.println();
    p = p->next;
    i++;
  }
}

#define MAX_NODES 10

void addNode(node_t** list_head, char *player_data) {
  node_t *new_node = createNode(player_data);
  if (new_node == NULL) return;

  Serial.println("NEW NODE");
  Serial.println(new_node->player_data);

  node_t *p = *list_head;
  node_t *prev = NULL;
  int count = 0;

  while (p != NULL && strcmp(new_node->player_data, p->player_data) <= 0) {
    prev = p;
    p = p->next;
    count++;
  }

  if (count >= MAX_NODES) {
    free(new_node);
    return;
  }

  if (prev == NULL) {
    new_node->next = *list_head;
    *list_head = new_node;
  } else {
    prev->next = new_node;
    new_node->next = p;
  }

  // după inserare, verificăm dacă avem > 10 noduri → ștergem ultimul
  p = *list_head;
  prev = NULL;
  count = 0;
  while (p != NULL) {
    count++;
    if (count > MAX_NODES) {
      // tăiem lista la 10
      prev->next = NULL;
      free(p);
      Serial.println("NODE DELETED");
      break;
    }
    prev = p;
    p = p->next;
  }
}

node_t* createNode(char *player_data) {
  node_t *new_node = (node_t *)malloc(sizeof(node_t));
  if (new_node == NULL) {
    return NULL;
  }

  new_node->next = NULL;
  strncpy(new_node->player_data, player_data, sizeof(new_node->player_data) - 1);
  new_node->player_data[sizeof(new_node->player_data) - 1] = '\0';

  return new_node;
}

void freeList(node_t **list_head) {
  node_t *p;
  
  while(*list_head != NULL) {
    p = *list_head;
    *list_head = (*list_head)->next;
    free(p);
  }
}

void drawKeyboard() {
  selectIdxX = 0;

  tft.fillScreen(ST7735_BLACK);

  for (int i = 0; i < numLines + 1; i++) {
    tft.setCursor(15, LCD_LENGTH / 2 + (i - 1) * 22);

    for (int j = 0; (j < charsPerLine) && (charsPerLine * i + j < (int)strlen(alphabet)); j++) {
      tft.print(alphabet[charsPerLine * i + j]);
      tft.print("  ");
    }
  }

  tft.print(" DEL  OKAY");

  tft.drawRect(11, LCD_LENGTH / 2 + (selectIdxX - 1) * 22 - 3, 14, 14, ST7735_WHITE);

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

    char buffer[20];
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

#else  // SPI_DRIVER_SELECT
#error SPI_DRIVER_SELECT must be two in SdFat/SdFatConfig.h
#endif  // SPI_DRIVER_SELECT