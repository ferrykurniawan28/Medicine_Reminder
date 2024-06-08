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

// String morning = "7:30";
// String afternoon = "12:00";
// String evening = "19:00";
int days = 0;

bool morningSet = false;
bool afternoonSet = false;
bool eveningSet = false;

int morningHour, morningMinute, afternoonHour, afternoonMinute, eveningHour, eveningMinute;

int morningContainer[] = {0, 3, 6};
int afternoonContainer[] = {1, 4, 7};
int eveningContainer[] = {2, 5, 8};

bool ContainerLoaded[] = {false, false, false, false, false, false, true, false, false}; // Array to keep track of which containers are loaded

int currentState = 0;
int containerState = 0;

DateTime futureDate;

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
int currentStateContainer = 10;

int Container0Address = 20;
int Container1Address = 21;
int Container2Address = 22;
int Container3Address = 23;
int Container4Address = 24;
int Container5Address = 25;
int Container6Address = 26;
int Container7Address = 27;
int Container8Address = 28;

int limit = 650;              // Threshold for 40 degrees

void date_time();
void manydays(int sensorValue);
void ask_daily(int sensorValue, String time, int yesState, int noState, bool &set);
void set_day(int sensorValue, String time, int &hour, int &minute, int nextState);
time_t convertToDateTime(int hour, int minute);
void checkAlarms();
void triggerAlarm(String message);
void loadContainer(int containerIndex, int direction);
void saveAlarmSettings();
void loadAlarm();
void saveContainerState(int containerIndex);
int loadCurrentContainer();
String stringCurrentContainer(int &index);
void load_container(int sensorValue);
void rotateStepper(int direction);
void addDaysToDate(int numDays);
void updateLoadedContainer(int containerIndex, bool loaded);
bool isContainerLoaded(int containerIndex);
int getNextContainerIndex(int container[], int size);
bool checkAlarmDate(DateTime currentDate, DateTime future);


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

byte cross[] = {
  B10001,
  B01010,
  B00100,
  B01010,
  B10001,
  B00000,
  B00000,
  B00000
};

byte check[] = {
  B00000,
  B00001,
  B00010,
  B10100,
  B01000,
  B00000,
  B00000,
  B00000
};

unsigned long backlightOffTime = 0; // Variable to store the time when the backlight should be turned off

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(A0, INPUT);
  // pinMode(limitSwitch, INPUT_PULLUP);
  pinMode(A1, INPUT);
  pinMode(13, OUTPUT);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.createChar(0, arrowUp);
  lcd.createChar(1, arrowDown);
  lcd.createChar(2, arrowRight);
  lcd.createChar(3, arrowLeft);
  lcd.createChar(4, cross);
  lcd.createChar(5, check);

  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }
  if (! rtc.isrunning()) {
    Serial.println("RTC is NOT running, let's set the time!");
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
    // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // adjust rtc time to current time on computer
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }

  loadAlarm();
  loadCurrentContainer();

  // int savedContainer = loadCurrentContainer();
  //   if (savedContainer >= 0 && savedContainer < 9) {
  //       int steps = savedContainer * (stepsPerRevolution / 9); // Calculate steps for saved container
  //       myStepper.setSpeed(5);
  //       myStepper.step(steps);
  //   }
}

void loop() {
  int sensorValue = analogRead(A0);
  // Serial.println(sensorValue);

  
  
  switch (currentState)
  {
  case 0:
    date_time();
    checkAlarms();
    if (sensorValue > 750 && sensorValue < 800)
    {
      currentState = 1;
      backlightOffTime = 0;
    } else if (sensorValue < 50)
    {
      currentState = 9;
    } 
    // else if (backlightOffTime == 0)
    // {
    //   backlightOffTime = millis() + 5000;
    // }
    // // else if (millis() >= backlightOffTime)
    // // {
    // //   lcd.noBacklight(); // Turn off the backlight
    // // } 
    // else
    // {
    //   backlightOffTime = millis() + 5000; // Reset the backlight off time to 5 seconds from now
    // }
    
    
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
      //  function down
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
  if (sensorValue < 50) {
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
      if (sensorValue > 300 && sensorValue < 350)
    {
      days++;
    } else if (sensorValue > 140 && sensorValue < 190)
    {
      if (days == 0)
      {
        
      } else
      {
        days--;
      }
    } else if (sensorValue > 750 && sensorValue < 800)
    {
      addDaysToDate(days);
      delay(2000);
      currentState = 2;
    }
    delay(170);
}

void addDaysToDate(int numDays) {
    DateTime now = rtc.now(); // Get the current date and time

    // Add the specified number of days to the current date
    futureDate = now + TimeSpan(numDays, 0, 0, 0);

    // Display the new date on the LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Reminder Until:");
    lcd.setCursor(0, 1);
    lcd.print(futureDate.day());
    lcd.print("/");
    lcd.print(futureDate.month());
    lcd.print("/");
    lcd.print(futureDate.year());

    delay(5000); // Delay to show the new date
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
      if (sensorValue > 300 && sensorValue < 350)
      {
        hour++;
        delay(170);
        if (hour > 23)
        {
          hour -= 24;
        }
        
      } else if (sensorValue > 140 && sensorValue < 190)
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
      } else if (sensorValue < 50)
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

void load_container(int sensorValue) {
  int load = containerState;
  lcd.clear();
  lcd.setBacklight(HIGH);
  lcd.setCursor(0, 0);
  lcd.print("Load Container");
  lcd.setCursor(0, 1);
  lcd.write(byte(3));
  lcd.print(" ");
  lcd.print(stringCurrentContainer(load));
  lcd.print(" ");
  lcd.write(byte(2));
  lcd.print(" ");
  lcd.print(load + 1);
  if (isContainerLoaded(load)) {
    lcd.write(byte(5));
  } else {
    lcd.write(byte(4));
  }

  // Serial.println(sensorValue);
  if (sensorValue < 50) {
    if (load >= 8) {
      load = 0;
    } else {
      load++;
    }
    loadContainer(load, 1);
  } else if (sensorValue > 750 && sensorValue < 800) {
    currentState = 0;
  } else if (sensorValue > 490 && sensorValue < 510) {
    if (load <= 0) {
      load = 8;
    } else {
      load--;
    }
    loadContainer(load, -1);
  } else if (sensorValue > 300 && sensorValue < 350) {
    updateLoadedContainer(load, true);
  } else if (sensorValue > 140 && sensorValue < 190) {
    updateLoadedContainer(load, false);
  }

  delay(170);
}

// get the next container index that is loaded
int getNextContainerIndex(int container[], int size) {
    for (int i = 0; i < size; i++) {
        if (ContainerLoaded[container[i]]) {
            return container[i];
        }
    }
    return -1; // No available container
}


// int getNextContainerIndex(int container[], int size) {
//     for (int i = 0; i < size; i++) {
//         if (!ContainerLoaded[container[i]]) {
//             return container[i];
//         }
//     }
//     return -1; // No available container
// }


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
    // Serial.println("Morning alarm triggered!");
    int targetIndex = getNextContainerIndex(morningContainer, sizeof(morningContainer) / sizeof(morningContainer[0]));
    Serial.println(targetIndex);
    if (targetIndex != -1 && checkAlarmDate(now,futureDate))
    {
      Serial.println("Loading container...");
      loadContainer(targetIndex, 1);
      Serial.println("Container loaded!");

      Serial.println("Saving container state...");
      saveContainerState(targetIndex);
      Serial.println("Container state saved!");
      triggerAlarm("Morning Alarm!");
    } else
    {
      morningSet = false;
    }
    // triggerAlarm("Morning Alarm!", targetIndex);
    
  } else if (currentTime == afternoonAlarm && afternoonSet) {
    int targetIndex = getNextContainerIndex(morningContainer, sizeof(morningContainer) / sizeof(morningContainer[0]));
    if (targetIndex != -1 && checkAlarmDate(now,futureDate))
    {
      triggerAlarm("Afternoon Alarm!");
    } else
    {
      afternoonSet = false;
    }
    
  } else if (currentTime == eveningAlarm && eveningSet) {
    int targetIndex = getNextContainerIndex(morningContainer, sizeof(morningContainer) / sizeof(morningContainer[0]));
    if (targetIndex != -1 && checkAlarmDate(now,futureDate))
    {
      triggerAlarm("Evening Alarm!");
    } else
    {
      eveningSet = false;
    }
    
  }
}

bool checkAlarmDate(DateTime currentDate, DateTime future) {
  if (currentDate.year() <= future.year() && currentDate.month() <= future.month() && currentDate.day() <= future.day()) {
    return true;
  }
  return false;
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
  
  // byte alarmFlag = 0;
  // alarmFlag |= morningSet << 0;
  // alarmFlag |= afternoonSet << 1;
  // alarmFlag |= eveningSet << 2;
  // EEPROM.write(alarmFlagAddress, alarmFlag);
}

void loadAlarm(){
  // Load alarm settings from EEPROM
  if (EEPROM.read(morningHourAddress) != 255)
  {
    morningHour = EEPROM.read(morningHourAddress);
  } else
  {
    morningHour = 8;
  }
  
  if (EEPROM.read(morningMinuteAddress) != 255)
  {
    morningMinute = EEPROM.read(morningMinuteAddress);
  } else
  {
    morningMinute = 20;
  }
  
  if (EEPROM.read(morningSetAddress) != 255)
  {
    morningSet = EEPROM.read(morningSetAddress);
  } else
  {
    morningSet = morningSet;
  }
  
  if (EEPROM.read(afternoonHourAddress) != 255)
  {
    afternoonHour = EEPROM.read(afternoonHourAddress);
  } else
  {
    afternoonHour = 12;
  }
  
  if (EEPROM.read(afternoonMinuteAddress) != 255)
  {
    afternoonMinute = EEPROM.read(afternoonMinuteAddress);
  } else
  {
    afternoonMinute = 20;
  }

  if (EEPROM.read(afternoonSetAddress) != 255)
  {
    afternoonSet = EEPROM.read(afternoonSetAddress);
  } else
  {
    afternoonSet = afternoonSet;
  }

  if (EEPROM.read(eveningHourAddress) != 255)
  {
    eveningHour = EEPROM.read(eveningHourAddress);
  } else
  {
    eveningHour = 20;
  }
  
  if (EEPROM.read(eveningMinuteAddress) != 255)
  {
    eveningMinute = EEPROM.read(eveningMinuteAddress);
  } else
  {
    eveningMinute = 20;
  }

  if (EEPROM.read(eveningSetAddress) != 255)
  {
    eveningSet = EEPROM.read(eveningSetAddress);
  } else
  {
    eveningSet = eveningSet;
  }

}

void triggerAlarm(String message) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(message);
  lcd.setCursor(0, 1);
  lcd.print("Container ");
  lcd.print(containerState + 1);

  digitalWrite(13, HIGH); // Turn on the LED
  lcd.setBacklight(HIGH); // Turn on the backlight
  delay(500); // Keep the message on screen for 5 seconds
  digitalWrite(13, LOW); // Turn on the LED
  lcd.setBacklight(LOW); // Turn on the backlight
  delay(500); // Keep the message on screen for 5 seconds
  digitalWrite(13, HIGH); // Turn on the LED
  lcd.setBacklight(HIGH); // Turn on the backlight
  delay(500); // Keep the message on screen for 5 seconds
  digitalWrite(13, LOW); // Turn on the LED
  lcd.setBacklight(LOW); // Turn on the backlight
  delay(500); // Keep the message on screen for 5 seconds
  digitalWrite(13, HIGH); // Turn on the LED
  lcd.setBacklight(HIGH); // Turn on the backlight
  delay(500); // Keep the message on screen for 5 seconds
  digitalWrite(13, LOW); // Turn on the LED
  lcd.setBacklight(HIGH); // Turn on the backlight
  delay(500); // Keep the message on screen for 5 seconds
  
  currentState = 0;
}

// need to adjust the steps to load the container
void loadContainer(int containerIndex, int direction) {
  while (containerIndex != containerState) {
    rotateStepper(direction);
    if (direction > 0) {
      containerState++;
      if (containerState > 8) {
        containerState = 0;
      }
    } else {
      containerState--;
      if (containerState < 0) {
        containerState = 8;
      }
    }
  }
}


void saveContainerState(int containerIndex) {
  // Save container state to EEPROM
  EEPROM.write(currentStateContainer, containerIndex);
}

int loadCurrentContainer(){
  if (EEPROM.read(currentStateContainer) != 255)
  {
    containerState = EEPROM.read(currentStateContainer);
  } else
  {
    containerState = 0;
  }
  

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

void rotateStepper(int direction) {
  int currentVal = analogRead(A1);
  Serial.println(currentVal);

  do {
    while (currentVal < limit)
    {
      myStepper.step(stepsPerRevolution / 360 * direction); // Move one degree at a time
      delay(10);
      currentVal = analogRead(A1); // Update currentVal inside the loop
      Serial.println(currentVal);
    }
    myStepper.step(stepsPerRevolution / 360 * direction); // Move one degree at a time
    delay(10);
    currentVal = analogRead(A1); // Update currentVal inside the loop
    Serial.println(currentVal);
  } while (currentVal > limit);

  // stop the stepper
  myStepper.step(0);
}

void updateLoadedContainer(int containerIndex, bool loaded) {
  for (int i = 0; i < 9; i++) {
    if (i == containerIndex) {
      ContainerLoaded[i] = loaded;
    }
  }
}

bool isContainerLoaded(int containerIndex) {
  return ContainerLoaded[containerIndex];
}