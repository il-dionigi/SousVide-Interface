#include <max6675.h>
#include <PID_v1.h>
#include <PID_AutoTune_v0.h>
#include <MenuBackend.h>
#include <LiquidCrystal.h>
#include <DFR_Key.h>
#include "sousVideItem.h"


//---------------------------------------------

// Prototypes
void menuChanged(MenuChangeEvent changed);
void menuUsed(MenuUseEvent used);
void readButtons();
void navigateMenus();
void sousVideStart(float ckTemp, float ckTime);
void cookPrep(float cookTemp);
void tempAdjust();
void cookTimer(float cookTime);
void cookComplete();
void showCustomTemp();
void showCustomTime();
void updateTemp(); // in Celsius

        /* PID Control */

bool pidEnabled = false;

// Output Relay
#define RelayPin 33 //note: change to whatever pin relay is hooked to

//Generic PID Variables
double Setpoint, Input, Output;

volatile long onTime = 0;

//PID Tuning Parameters
double Kp = 2, Ki = 5, Kd = 1;

PID myPID(&Input, &Output, &Setpoint, Kp, Ki, Kd, DIRECT);

int WindowSize = 5000;
unsigned long windowStartTime;

////AutoTune Variables / Constants
//byte ATuneModeRemember = 2;
//
//double aTuneStep = 500;
//double aTuneNoise = 1;
//unsigned int aTuneLookBack = 20;
//
//bool tuning = false;
//
//PID_ATune aTune(&Input, &Output);

// Temperature Sensor
const int THERMO_GND_PIN = 45;
const int THERMO_VCC_PIN = 47;
const int THERMO_SCK_PIN = 49;
const int THERMO_CS_PIN = 51;
const int THERMO_SO_PIN = 53;

MAX6675 thermocouple(THERMO_SCK_PIN, THERMO_CS_PIN, THERMO_SO_PIN);

uint8_t degree[8] = { 140,146,146,140,128,128,128,128 };

long tempLastReading = millis(); // time of last temp reading
const int tempReadRate = 1000; // time between temp readings (in ms)

// Keypad
DFR_Key keypad;

// Button Pins
const int BUTTON_NONE = 0;
const int BUTTON_SELECT = 1;
const int BUTTON_LEFT = 2;
const int BUTTON_UP = 3;
const int BUTTON_DOWN = 4;
const int BUTTON_RIGHT = 5;

int lastButtonPushed = BUTTON_NONE;
int localKey = SAMPLE_WAIT;

long lastSelectDebounceTime = 0;
long lastLeftDebounceTime = 0;
long lastUpDebounceTime = 0;
long lastDownDebounceTime = 0;
long lastRightDebounceTime = 0;
long debounceDelay = 500;

//Pin assignments for DFRobot LCD Keypad Shield
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

// Menu variables
MenuBackend menu = MenuBackend(menuUsed, menuChanged);

// initialize menuitems
MenuItem menuItem1 = MenuItem("Egg");
MenuItem menuItem1SubItem1 = MenuItem("Hard-boiled", 0);
MenuItem menuItem1SubItem2 = MenuItem("Soft-boiled", 1);
MenuItem menuItem2 = MenuItem("Meat");
MenuItem menuItem2SubItem1 = MenuItem("Item2SubItem1");
MenuItem menuItem2SubItem2 = MenuItem("Item2SubItem2");
MenuItem menuItem2SubItem3 = MenuItem("Item2SubItem3");
MenuItem menuItem3 = MenuItem("Misc.", 2);

SousVideItem* curSVI;
SousVideItem* aSVI[100];
int numSVI = 0;
// Test items
SousVideItem Egg_Hard("Egg", "Hard-boiled", 40, 73.9);
SousVideItem Egg_Soft("Egg", "Soft-boiled", 40, 61.7);
SousVideItem Misc("Meat", "Porkchop", 60, 60);

// Cook variables
bool tempReached = false, beganCook = false, foodCooking = false, cookFinished = false, tempChosen = true;
float temp = 0, prevTemp = 0, cookTemp = 0;

// Time variables
long timeElapsed;
int timeLeft = 0, prevTime, cookTime;
bool timerStarted = false, timeChosen = true;

// Screen variables
bool screenChanged = false;
int screen = 0;
const int PICK_TIME = -1, PICK_TEMP = -2, COOK_PREP = 1, COOK_DURATION = 2, COOK_FINISHED = 3;

void setup()
{
  Serial.begin(9600);

  // PID Output
  pinMode(RelayPin, OUTPUT);

  // Temperature Sensor
  pinMode(THERMO_VCC_PIN, OUTPUT);
  pinMode(THERMO_GND_PIN, OUTPUT);
  digitalWrite(THERMO_VCC_PIN, HIGH);
  digitalWrite(THERMO_GND_PIN, LOW);

  lcd.createChar(0, degree);  // degree symbol

  // SousVide Item Setup
  aSVI[0] = &Egg_Hard;
  numSVI++;
  aSVI[1] = &Egg_Soft;
  numSVI++;
  aSVI[2] = &Misc;
  numSVI++;

  // Menu Setup
  menu.getRoot().add(menuItem1);
  menuItem1.addRight(menuItem2).addRight(menuItem3);
  menuItem1.add(menuItem1SubItem1).addRight(menuItem1SubItem2);
  menuItem2.add(menuItem2SubItem1).addRight(menuItem2SubItem2).addRight(menuItem2SubItem3);
  menu.toRoot();

  // LCD Setup
  lcd.begin(16, 2);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("SousVide v 0.1");
  delay(2500);

  /*
  OPTIONAL
  keypad.setRate(x);
  Sets the sample rate at once every x milliseconds.
  Default: 10ms
  */
  keypad.setRate(10);
}

void loop()
{
  /*
  keypad.getKey();
  Grabs the current key.
  Returns a non-zero integer corresponding to the pressed key,
  OR
  Returns 0 for no keys pressed,
  OR
  Returns -1 (sample wait) when no key is available to be sampled.
  */
  readButtons();
  navigateMenus();
}

void menuChanged(MenuChangeEvent changed)
{
  MenuItem newMenuItem = changed.to; //get the destination menu

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Select your food");
  lcd.setCursor(0, 1); //set the start position for lcd printing to the second row

  if (newMenuItem.getName() == menu.getRoot())
  {
    lcd.print("Enter to choose:");
  }
  else if (newMenuItem.getName() == menuItem1)
  {
    lcd.print(menuItem1.getName());
  }
  else if (newMenuItem.getName() == menuItem1SubItem1)
  {
    lcd.print(menuItem1SubItem1.getName());
  }
  else if (newMenuItem.getName() == menuItem1SubItem2)
  {
    lcd.print(menuItem1SubItem2.getName());
  }
  else if (newMenuItem.getName() == menuItem2)
  {
    lcd.print(menuItem2.getName());
  }
  else if (newMenuItem.getName() == menuItem2SubItem1)
  {
    lcd.print(menuItem2SubItem1.getName());
  }
  else if (newMenuItem.getName() == menuItem2SubItem2)
  {
    lcd.print(menuItem2SubItem2.getName());
  }
  else if (newMenuItem.getName() == menuItem2SubItem3)
  {
    lcd.print(menuItem2SubItem3.getName());
  }
  else if (newMenuItem.getName() == menuItem3)
  {
    lcd.print(menuItem3.getName());
  }
}

void menuUsed(MenuUseEvent used) {
  if (used.item.hasShortkey())
  {
    curSVI = aSVI[used.item.getShortkey()];
  }

  sousVideStart(curSVI->getCookTemp(), curSVI->getCookTime());

  menu.toRoot();  //back to Main
}


void  readButtons()
{  //read buttons status
  localKey = keypad.getKey();
  switch (localKey)
  {
  case BUTTON_SELECT:
    if ((millis() - lastSelectDebounceTime) > debounceDelay)
    {
      lastButtonPushed = BUTTON_SELECT;
      lastSelectDebounceTime = millis();
    }
    break;
  case BUTTON_LEFT:
    if ((millis() - lastLeftDebounceTime) > debounceDelay)
    {
      lastButtonPushed = BUTTON_LEFT;
      lastLeftDebounceTime = millis();
    }
    break;
  case BUTTON_UP:
    if ((millis() - lastUpDebounceTime) > debounceDelay)
    {
      lastButtonPushed = BUTTON_UP;
      lastUpDebounceTime = millis();
    }
    break;
  case BUTTON_DOWN:
    if ((millis() - lastDownDebounceTime) > debounceDelay)
    {
      lastButtonPushed = BUTTON_DOWN;
      lastDownDebounceTime = millis();
    }
    break;
  case BUTTON_RIGHT:
    if ((millis() - lastRightDebounceTime) > debounceDelay)
    {
      lastButtonPushed = BUTTON_RIGHT;
      lastRightDebounceTime = millis();
    }
    break;
  default:
    lastButtonPushed = BUTTON_NONE;
    break;
  }
}

void navigateMenus()
{
  MenuItem currentMenu = menu.getCurrent();
  MenuItem tempMenu = menu.getCurrent();

  switch (lastButtonPushed)
  {
  case BUTTON_SELECT:
    if (!(currentMenu.moveDown())) {  //if the current menu has a child and has been pressed enter then menu navigate to item below
      menu.use();
    }
    else {  //otherwise, if menu has no child and has been pressed enter the current menu is used
      menu.moveDown();
    }
    break;
  case BUTTON_UP:
    menu.toRoot();  //back to main
    break;
  case BUTTON_DOWN: //back to parent
    if (menu.getCurrent() == menu.getRoot())
      break;
    while ((menu.getCurrent().getBefore())->getName() == NULL)
    {
      lcd.clear();  //removing breaks code
      menu.moveLeft();
    }
    lcd.clear();  //removing breaks code
    menu.moveUp();
    break;
  case BUTTON_RIGHT:
    menu.moveRight();
    break;
  case BUTTON_LEFT:
    menu.moveLeft();
    break;
  case BUTTON_NONE:
  default:
    break;
  }

  lastButtonPushed = BUTTON_NONE; //reset the lastButtonPushed variable
}

void sousVideStart(float ckTemp, float ckTime)
{
  // sousVide start-up
  windowStartTime = millis();

  // initialize the variables we're linked to
  Setpoint = ckTemp;

  // tell the PID to range between 0 and full window size
  myPID.SetOutputLimits(0, WindowSize);

  // turn PID on
  myPID.SetMode(AUTOMATIC);
  pidEnabled = true;

  foodCooking = true;
  //tempReached = false;
  tempReached = true;
  beganCook = false;
  screenChanged = true;
  screen = PICK_TEMP;

  cookTemp = ckTemp;
  cookTime = ckTime * 60; // converting minutes to seconds

  if (cookTemp == -1 || cookTime == -1)
  {
    tempChosen = false;
    timeChosen = false;
    //cookTemp = 60;  // default time
    //cookTemp = 50;  // default time
  }
  else
    screen = COOK_PREP;

  while (tempChosen == false || timeChosen == false)
  {
    readButtons();

    if (lastButtonPushed != BUTTON_NONE && lastButtonPushed != SAMPLE_WAIT)
    {
      switch (lastButtonPushed)
      {
      case BUTTON_LEFT:
        if (tempChosen == false)
          cookTemp -= 0.1;
        else
          cookTime--;
        break;
      case BUTTON_RIGHT:
        if (tempChosen == false)
          cookTemp += 0.1;
        else
          cookTime++;
        break;
      case BUTTON_DOWN:
        if (tempChosen == false)
          cookTemp -= 10;
        else
          cookTime -= 10;
        break;
      case BUTTON_UP:
        if (tempChosen == false)
          cookTemp += 10;
        else
          cookTime += 10;
        break;
      case BUTTON_SELECT:
        if (tempChosen == false)    // selecting temp
        {
          tempChosen = true;
          screen = PICK_TIME;
        }
        else    // selecting temp
        {
          timeChosen = true;
          screen = COOK_PREP;
        }
        break;
      }
      screenChanged = true;
    }

    if (screenChanged)
    {
      switch (screen)
      {
      case PICK_TEMP:
        showCustomTemp();
        break;
      case PICK_TIME:
        showCustomTime();
        break;
      }
    }

    prevTemp = cookTemp;
    prevTime = cookTime;
    screenChanged = false;
  }

  while (foodCooking)
  {
    updateTemp();
    readButtons();
    tempAdjust();

    if (!tempReached) // Update screen while reaching temp
    {
      if (temp != prevTemp)
      {
        screenChanged = true;
        prevTemp = temp;
      }
    }
    else if (tempReached && !timerStarted)
    {
      if (lastButtonPushed == BUTTON_SELECT)
      {
        timerStarted = true;
        timeElapsed = millis();
        timeLeft = cookTime;
      }
    }
    else if (timerStarted && !cookFinished)
    {
      if (millis() - timeElapsed > 1000) // one minute passed
      {
        timeLeft--;
        timeElapsed += 1000;
        screenChanged = true;
      }
    }
    else if (cookFinished)
    {
      if (lastButtonPushed == BUTTON_SELECT)
      {
        pidEnabled = false;
        foodCooking = false;
      }
    }

    if (screenChanged)
    {
      switch (screen)
      {
      case COOK_PREP:
        cookPrep(cookTemp);
        break;
      case COOK_DURATION:
        cookTimer(cookTime); // drop through to COOK_FINISHED
      case COOK_FINISHED:
        cookComplete();
        break;
      }
    }
    screenChanged = false;
  }
}

void showCustomTemp()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Select temp:");
  lcd.setCursor(0, 1);
  lcd.print(cookTemp);
  #if ARDUINO >= 100
    lcd.write((byte)0);
  #else
    lcd.print(0, BYTE);
  #endif
    lcd.print("C");
}

void showCustomTime()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Select time:");
  lcd.setCursor(0, 1);
  lcd.print(cookTime);
  lcd.print("min.");
}

void cookPrep(float cookTemp)
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("GoalTemp:");
  lcd.print(cookTemp);
  #if ARDUINO >= 100
    lcd.write((byte)0);
  #else
    lcd.print(0, BYTE);
  #endif
    lcd.print("C");
    lcd.setCursor(0, 1);
    lcd.print("PresTemp:");
    lcd.print(temp);
  #if ARDUINO >= 100
    lcd.write((byte)0);
  #else
    lcd.print(0, BYTE);
  #endif
    lcd.print("C");

  if (temp >= cookTemp)
  {
    tempReached = true;
    screen = COOK_DURATION;
    cookTimer(cookTime);
  }
}

void tempAdjust()
{
  //get temp reading
  Input = temp;
  myPID.Compute();

  //turn output pin on/off based on pid output
  if ((millis() - windowStartTime) > WindowSize)
  {
    windowStartTime += WindowSize;
  }

  if (Output > millis() - windowStartTime)
  {
    digitalWrite(RelayPin, HIGH);
  }
  else
  {
    digitalWrite(RelayPin, LOW);
  }
}

void cookTimer(float cookTime)
{
  if (!timerStarted)
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("- Temp reached -");
    lcd.setCursor(0, 1);
    lcd.print("Ready to start?");
    if (lastButtonPushed == BUTTON_SELECT)
    {
      timerStarted = true;
      timeElapsed = millis();
      timeLeft = cookTime;
    }
  }
  else // timer started
  {
    int curH = (timeLeft / 3600);
    int setH = (curSVI->getCookTime() / 60);
    int curM = ((timeLeft % 3600) / 60);
    int setM = ((int)curSVI->getCookTime() % 60);
    lcd.clear();
    lcd.setCursor(0, 0);
    
    // print setTemp and degree
    lcd.print(curSVI->getCookTemp());
    lcd.setCursor(4,0);
  #if ARDUINO >= 100
    lcd.write((byte)0);
  #else
    lcd.print(0, BYTE);
  #endif
    lcd.print("C    ");
    
    // print curTemp
    lcd.print(temp);
    lcd.setCursor(14,0);
  #if ARDUINO >= 100
    lcd.write((byte)0);
  #else
    lcd.print(0, BYTE);
  #endif
    lcd.print("C");

    lcd.setCursor(0, 1);
    lcd.print("TimeLeft: ");
    // print curTime
    if (curH < 10)
    {
      lcd.print("0");
    }
    lcd.print(curH);
    lcd.print("h");
    if (curM < 10)
    {
      lcd.print("0");
    }
    lcd.print(curM);
    lcd.print("m");
  }

  if (timeLeft < 0) // fix this later
  {
    screen = COOK_FINISHED;
    cookFinished = true;
  }
}

void cookComplete()
{
  if (cookFinished)
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Cook done: stop");
    lcd.setCursor(0, 1);
    lcd.print("temp control?");
  }
}

void updateTemp()
{
  if ((millis() - tempLastReading) > tempReadRate)
  {
    temp = thermocouple.readCelsius();
    temp--; 
    delay(200);
    tempLastReading = millis();
  }
}
