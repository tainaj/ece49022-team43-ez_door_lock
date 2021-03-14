// include the library code:
#include <LiquidCrystal.h>
#include "Keypad.h"
#include "Wire.h"

const byte ROWS = 4; //four rows
const byte COLS = 4; //four columns

char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

byte rowPins[ROWS] = {25, 33, 32, 22};
byte colPins[COLS] = {13, 14, 27, 26};

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);
 
// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(19, 23, 18, 17, 16, 15);

const int len_key = 4;
char master_key[len_key] = {'1','2','3','4'};
char attempt_key[len_key];
int z=0;

void setup() {
  
  pinMode(4, OUTPUT);
  digitalWrite(4, HIGH); 
  delay(3000); 
  digitalWrite(4, LOW);

  lcd.begin(16, 2);
  lcd.print("Welcome!");
  lcd.setCursor(0,1);
  lcd.print("Enter PIN or FP.");
}

void loop() {
  
  char key = keypad.getKey();
  if (key == '*') {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.clear();
    lcd.print("Enter PIN");
    PINMODE();
  }
  else if (key == '#') {
    AdminControl();
  }
}

void AdminControl() {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("1. Add ID");
  lcd.setCursor(0,1);
  lcd.print("2. Delete ID");
}


void PINMODE() {
  while(true) {
  char key = keypad.getKey();
  lcd.setCursor(z-1,1);
  lcd.print("*");
  if (key){
    switch(key){
      case '*':
        z=0;
        break;
      case '#':
        delay(100); // added debounce
        checkKEY();
        break;
      default:
         attempt_key[z]=key;
         z++;
      }
  }
  }
}

void checkKEY()
{
   int correct=0;
   int i;
   for (i=0; i<len_key; i++) {
    if (attempt_key[i]==master_key[i]) {
      correct++;
      }
    }
   if (correct==len_key && z==len_key){
    lcd.setCursor(0,1);
    lcd.print("Correct PIN");
    delay(1000);
    z=0;
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Open!");
    delay(3000);

    ESP.restart();
   }
   else
   {
    lcd.setCursor(0,1);
    lcd.print("Incorrect PIN");
    delay(3000);
    z=0;
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Enter PIN");
   }
   for (int zz=0; zz<len_key; zz++) {
    attempt_key[zz]=0;
   }
}
