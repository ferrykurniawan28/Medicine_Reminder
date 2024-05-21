#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <RTClib.h>
#include <SPI.h>
#include <TimeLib.h>
#include <Stepper.h>
#include <EEPROM.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

RTC_DS1307 rtc;

char daysOfTheWeek[7][12] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

const int stepsPerRevolution = 2038;
Stepper myStepper = Stepper(stepsPerRevolution, 8, 10, 9, 11);

int limitSwitch = 2;

String morning = "7:30";
String afternoon = "12:00";
String evening = "19:00";
int days = 1;

bool morningSet = false;
bool afternoonSet = false;
bool eveningSet = false;

int morningHour = morning.substring(0, morning.indexOf(':')).toInt();
int morningMinute = morning.substring(morning.indexOf(':') + 1).toInt();

int afternoonHour = afternoon.substring(0, afternoon.indexOf(':')).toInt();
int afternoonMinute = afternoon.substring(afternoon.indexOf(':') + 1).toInt();

int eveningHour = evening.substring(0, evening.indexOf(':')).toInt();
int eveningMinute = evening.substring(evening.indexOf(':') + 1).toInt();

int currentState = 0;
int containerState = 0;

// EEPROM addresses
int morningHourAddress = 0;
int morningMinuteAddress = 1;
int morningSetAddress = 2;
int afternoonHourAddress = 3;
int afternoonMinuteAddress = 4;
int afternoonSetAddress = 5;
int eveningHourAddress = 6;
int eveningMinuteAddress = 7;
int eveningSetAddress = 8;
int alarmFlagAddress = 9;
int currentStateContainer = 10;

void date_time();
void manydays(int sensorValue);
void ask_daily(int sensorValue, String time, int yesState, int noState, bool &set);
void set_day(int sensorValue, String time, int &hour, int &minute, int nextState);
time_t convertToDateTime(int hour, int minute);
void checkAlarms();
void triggerAlarm(String message, int containerIndex);
void loadContainer(int steps);
void saveAlarmSettings();
void loadAlarm();
void saveContainerState(int containerIndex);
int loadCurrentContainer();
String stringCurrentContainer(int &index);
void load_container(int sensorValue);

byte arrowUp[] = {
  B00100,
  B01110,
  B10101,
  B00100,
  B00100,
  B00100,
  B00100,
  B00000
};

byte arrowDown[] = {
  B00000,
  B00100,
  B00100,
  B00100,
  B00100,
  B10101,
  B01110,
  B00100
};

byte arrowRight[] = {
  B00000,
  B00100,
  B00010,
  B11111,
  B00010,
  B00100,
  B00000,
  B00000
};

byte arrowLeft[] = {
  B00000,
  B00100,
  B01000,
  B11111,
  B01000,
  B00100,
  B00000,
  B00000
};

unsigned long backlightOffTime = 0; // Variable to store the time when the backlight should be turned off

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(A0, INPUT);
  pinMode(limitSwitch, INPUT_PULLUP);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.createChar(0, arrowUp);
  lcd.createChar(1, arrowDown);
  lcd.createChar(2, arrowRight);
  lcd.createChar(3, arrowLeft);

  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }
  if (! rtc.isrunning()) {
    Serial.println("RTC is NOT running, let's set the time!");
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }

  loadAlarm();

  int savedContainer = loadCurrentContainer();
    if (savedContainer >= 0 && savedContainer < 9) {
        int steps = savedContainer * (stepsPerRevolution / 9); // Calculate steps for saved container
        myStepper.setSpeed(5);
        myStepper.step(steps);
    }
}

void loop() {
  int sensorValue = analogRead(A0);

  
  
  switch (currentState)
  {
  case 0:
    date_time();
    checkAlarms();
    if (sensorValue > 750 && sensorValue < 800)
    {
      currentState = 1;
      backlightOffTime = 0;
    } else if (sensorValue < 10)
    {
      currentState = 9;
    } else if (backlightOffTime == 0) // If the backlight off time is not set yet
    {
      backlightOffTime = millis() + 5000; // Set the backlight off time to 5 seconds from now
    }
    else if (millis() >= backlightOffTime) // If the current time is past the backlight off time
    {
      lcd.noBacklight(); // Turn off the backlight
    } else if (sensorValue <800)
    {
      backlightOffTime = millis() + 5000; // Reset the backlight off time to 5 seconds from now
    }
    
    
    break;
  case 1:
    manydays(sensorValue);
    break;
  case 2:
    ask_daily(sensorValue, "Morning", 3, 4, morningSet);
    break;
  case 3:
    set_day(sensorValue, "Morning", morningHour, morningMinute, 4);
    break;
  case 4:
    ask_daily(sensorValue, "Afternoon", 5, 6, afternoonSet);
    break;
  case 5:
    set_day(sensorValue, "Afternoon", afternoonHour, afternoonMinute, 6);
    break;
  case 6:
    ask_daily(sensorValue, "Evening", 7, 8, eveningSet);
    break;
  case 7:
    set_day(sensorValue, "Evening", eveningHour, eveningMinute, 8);
    break;
  case 8:
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Schedule Set");
    lcd.setCursor(0, 1);
    lcd.print("Successfully");
    delay(2000);
    currentState = 0;
    break;
  case 9:
    load_container(sensorValue);
    break;
  default:
    break;
  }
  

  // if (sensorValue < 10)
  // {
  //   // Serial.println("Right");
  //   lcd.clear();
  // lcd.setCursor(0, 0);
  //   lcd.print("Right");
  // } else if (sensorValue > 300 && sensorValue < 315)
  // {
  //   // Serial.println("Up");
  //   lcd.clear();
  // lcd.setCursor(0, 0);
  //   lcd.print("Up");
  // } else if (sensorValue > 490 && sensorValue < 510)
  // {
  //   // Serial.println("Left");
  //   lcd.print("Left");
  // } else if (sensorValue > 140 && sensorValue < 160)
  // {
  //   // Serial.println("Down");
  //   lcd.print("Down");
  // } else if (sensorValue > 750 && sensorValue < 800)
  // {
  //   // Serial.println("Select");
  //   lcd.print("Select");
  // }
  
}

void date_time(){
  DateTime now = rtc.now();

  lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(now.year(), DEC);
    lcd.print('/');
    lcd.print(now.month(), DEC);
    lcd.print('/');
    lcd.print(now.day(), DEC);
    lcd.print(" (");
    lcd.print(daysOfTheWeek[now.dayOfTheWeek()]);
    lcd.print(") ");
    lcd.setCursor(0, 1);
    lcd.print(now.hour(), DEC);
    lcd.print(':');
    lcd.print(now.minute(), DEC);
    lcd.print(':');
    lcd.print(now.second(), DEC);
    delay(170);
}

void ask_daily(int sensorValue, String time, int yesState, int noState, bool &set){
  lcd.clear();
  lcd.setBacklight(HIGH);
  lcd.setCursor(0, 0);
  lcd.print("Set on ");
  lcd.print(time);
  lcd.print("?");
  lcd.setCursor(0, 1);
  lcd.write(byte(3));
  lcd.print(" No ");
  lcd.print("      ");
  lcd.write(byte(2));
  lcd.print(" Yes ");
  if (sensorValue < 10) {
    currentState = yesState;
    set = true;
    saveAlarmSettings();
  } else if (sensorValue > 490 && sensorValue < 510) {
    currentState = noState;
    set = false;
    saveAlarmSettings();
  }
  delay(170);
}

void manydays(int sensorValue){
  lcd.clear();
  lcd.setBacklight(HIGH);
    lcd.setCursor(0, 0);
    lcd.print("How Many Days?");
    lcd.setCursor(0, 1);
    lcd.print(days);
      if (sensorValue > 300 && sensorValue < 315)
    {
      days++;
    } else if (sensorValue > 140 && sensorValue < 160)
    {
      if (days == 1)
      {
        
      } else
      {
        days--;
      }
    } else if (sensorValue > 750 && sensorValue < 800)
    {
      currentState = 2;
    }
    delay(170);
}

void set_day(int sensorValue, String time, int &hour, int &minute, int nextState){
  lcd.clear();
  lcd.setBacklight(HIGH);
    lcd.setCursor(0, 0);
    lcd.print(time);
    lcd.print(": ");
    lcd.print(hour);
    lcd.print(":");
    lcd.print(minute);
    lcd.setCursor(0, 1);
    lcd.write(byte(0));
    lcd.print("+H ");
    lcd.write(byte(1));
    lcd.print("-H ");
    lcd.write(byte(2));
    lcd.print("+M ");
    lcd.write(byte(3));
    lcd.print("-M ");
      if (sensorValue > 300 && sensorValue < 315)
      {
        hour++;
        delay(170);
        if (hour > 23)
        {
          hour -= 24;
        }
        
      } else if (sensorValue > 140 && sensorValue < 160)
      {
        hour--;
        delay(170);
        if (hour < 0)
        {
          hour += 24;
        }
        
      } else if (sensorValue > 750 && sensorValue < 800)
      {
        currentState = nextState;
        saveAlarmSettings();
      } else if (sensorValue < 10)
      {
        minute++;
        delay(170);
        if (minute > 59)
        {
          minute -= 60;
        }
        
      } else if (sensorValue > 490 && sensorValue < 510)
      {
        minute--;
        delay(170);
        if (minute < 0)
        {
          minute += 60;
        }
    }
    delay(170);
}

void load_container(int sensorValue){
  int load = currentStateContainer;
  lcd.clear();
  lcd.setBacklight(HIGH);
  lcd.setCursor(0, 0);
  lcd.print("Load Container");
  lcd.setCursor(0, 1);
  lcd.write(byte(3));
  lcd.print(stringCurrentContainer(load));
  lcd.write(byte(2));
  if (sensorValue <10)
  {
    load++;
    loadContainer(load);
  } else if (sensorValue > 750 && sensorValue < 800)
  {
    currentState = 0;
  } else if (sensorValue > 490 && sensorValue < 510)
  {
    load--;
    loadContainer(load);
  }
  
  delay(170);
}

time_t convertToDateTime(int hour, int minute) {
  // Get the current date
  int currentYear = year();
  int currentMonth = month();
  int currentDay = day();

  // Create a tmElements_t structure
  tmElements_t tm;

  // Set the structure with current date and provided time
  tm.Year = currentYear - 1970;  // Years since 1970
  tm.Month = currentMonth;
  tm.Day = currentDay;
  tm.Hour = hour;
  tm.Minute = minute;
  tm.Second = 0;  // Set seconds to 0

  // Convert tmElements_t to time_t
  time_t dateTime = makeTime(tm);

  return dateTime;
}

void checkAlarms() {
  DateTime now = rtc.now();

  // Convert current time to time_t
  time_t currentTime = convertToDateTime(now.hour(), now.minute());

  // Convert alarm times to time_t
  time_t morningAlarm = convertToDateTime(morningHour, morningMinute);
  time_t afternoonAlarm = convertToDateTime(afternoonHour, afternoonMinute);
  time_t eveningAlarm = convertToDateTime(eveningHour, eveningMinute);

  // Check and trigger alarms
  if (currentTime == morningAlarm && morningSet) {
    triggerAlarm("Morning Alarm!", 0);
    morningSet = false; // Reset the alarm once triggered
  } else if (currentTime == afternoonAlarm && afternoonSet) {
    triggerAlarm("Afternoon Alarm!", 3);
    afternoonSet = false; // Reset the alarm once triggered
  } else if (currentTime == eveningAlarm && eveningSet) {
    triggerAlarm("Evening Alarm!", 6);
    eveningSet = false; // Reset the alarm once triggered
  }
}

void saveAlarmSettings() {
  // Save alarm settings to EEPROM
  EEPROM.write(morningHourAddress, morningHour);
  EEPROM.write(morningMinuteAddress, morningMinute);
  EEPROM.write(morningSetAddress, morningSet);
  EEPROM.write(afternoonHourAddress, afternoonHour);
  EEPROM.write(afternoonMinuteAddress, afternoonMinute);
  EEPROM.write(afternoonSetAddress, afternoonSet);
  EEPROM.write(eveningHourAddress, eveningHour);
  EEPROM.write(eveningMinuteAddress, eveningMinute);
  EEPROM.write(eveningSetAddress, eveningSet);
  
  byte alarmFlag = 0;
  alarmFlag |= morningSet << 0;
  alarmFlag |= afternoonSet << 1;
  alarmFlag |= eveningSet << 2;
  EEPROM.write(alarmFlagAddress, alarmFlag);
}

void loadAlarm(){
  // Load alarm settings from EEPROM
  morningHour = EEPROM.read(morningHourAddress);
  morningMinute = EEPROM.read(morningMinuteAddress);
  morningSet = EEPROM.read(morningSetAddress);
  afternoonHour = EEPROM.read(afternoonHourAddress);
  afternoonMinute = EEPROM.read(afternoonMinuteAddress);
  afternoonSet = EEPROM.read(afternoonSetAddress);
  eveningHour = EEPROM.read(eveningHourAddress);
  eveningMinute = EEPROM.read(eveningMinuteAddress);
  eveningSet = EEPROM.read(eveningSetAddress);

  byte alarmFlag = EEPROM.read(alarmFlagAddress);
  morningSet = alarmFlag & (1 << 0);
  afternoonSet = alarmFlag & (1 << 1);
  eveningSet = alarmFlag & (1 << 2);

}

void triggerAlarm(String message, int containerIndex) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(message);
  saveContainerState(containerIndex);
  // rotate stepper motor 40 degrees
  myStepper.setSpeed(5);
  myStepper.step(40);

  // stop rotating if limit switch is pressed
  while (digitalRead(limitSwitch) == HIGH) {
    myStepper.setSpeed(5);
    myStepper.step(40);
  }

  lcd.setBacklight(HIGH); // Turn on the backlight
  digitalWrite(13, HIGH); // Turn on the LED
  delay(500); // Keep the message on screen for 5 seconds
  lcd.setBacklight(LOW); // Turn off the backlight
  digitalWrite(13, LOW); // Turn off the LED
  delay(500); // Keep the message on screen for 5 seconds
  lcd.setBacklight(HIGH); // Turn on the backlight
  digitalWrite(13, HIGH); // Turn on the LED
  delay(500); // Keep the message on screen for 5 seconds
  lcd.setBacklight(LOW); // Turn off the backlight
  digitalWrite(13, LOW); // Turn off the LED
  delay(500); // Keep the message on screen for 5 seconds
  lcd.setBacklight(HIGH); // Turn on the backlight
  digitalWrite(13, HIGH); // Turn on the LED
  delay(500);
}

// need to adjust the steps to load the container
void loadContainer(int containerIndex) {
    int steps = containerIndex * (stepsPerRevolution / 9); // Calculate steps for target container
    myStepper.setSpeed(5);
    myStepper.step(steps);

    // Stop rotating if limit switch is pressed
    // while (digitalRead(limitSwitch) == HIGH) {
    //     myStepper.setSpeed(5);
    //     myStepper.step(steps);
    // }

    saveContainerState(containerIndex); // Save the current container index
}


void saveContainerState(int containerIndex) {
  // Save container state to EEPROM
  EEPROM.write(currentStateContainer, containerIndex);
}

int loadCurrentContainer(){
  containerState = EEPROM.read(currentStateContainer);

  return containerState;
}

String stringCurrentContainer(int &index){
  String container;
  switch (index)
  {
  case 0:
    container = "Morning";
    break;
  case 1:
    container = "Afternoon";
    break;
  case 2:
    container = "Evening";
    break;
  case 3:
    container = "Morning";
    break;
  case 4:
    container = "Afternoon";
    break;
  case 5:
    container = "Evening";
    break;
  case 6:
    container = "Morning";
    break;
  case 7:
    container = "Afternoon";
    break;
  case 8:
    container = "Evening";
    break;
  default:
    break;
  }
  return container;
}