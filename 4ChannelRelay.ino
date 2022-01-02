/*
  Four Channel IR Base Remote Control
  Developed By - Soe Htet Aung
  Special thanks to - Yamin Aung, Aye Min Oo, Aung Myint Oo, Saw Htun Aung

  There are three main functions in this project.
    1) Record Five IR code for each channel and save IR Code in Arduino EEPROM.
    2) Playing (Turn on or Turn off each channel by IR remote control)
    3) Turn Off state of each channel when the board is turned off

  References :
    1) EEPROM
        - https://roboticsbackend.com/arduino-write-string-in-eeprom/
        - https://www.arduino.cc/en/Tutorial/EEPROMRead
    2) IR Sensor
        - http://www.electronoobs.com/eng_arduino_tut34
    3) Button
        - https://www.programmingelectronics.com/tutorial-19-debouncing-a-button-with-arduino-old-version/

  Library Files :
    1) https://github.com/z3t0/Arduino-IRremote
*/

#include <IRremote.h>
#include <EEPROM.h>

// IR Reciever
const int RECV_PIN = 8;
IRrecv irrecv(RECV_PIN);
decode_results irResults;

// Channel
const int CHANNEL_LENGTH = 5;
const int CHANNEL_COUNT = 4;

const int CHANNEL_PIN[CHANNEL_COUNT] = {12, 11, 10, 9};    // Channel 1 to 4
bool CHANNEL_STATE[CHANNEL_COUNT];                        // Toggle Channel ON/OFF
int cIndex = 0;                                           // Channel Index to Save Five button IR code for each channel
unsigned long channelArr[CHANNEL_COUNT][CHANNEL_LENGTH];  // Tempory Array for Channel

// Recording
const int RecordButton = 7;
const int RecordLED = 13;
const int pushButtonInterval = 700;                      // The time we need to wait in milli seconds.

// Others
unsigned long previousMillis = 0;
unsigned long currentMillis = 0;
bool isClearTempData = false;
/*
   ---- Record Status ----
   0 - Play Mode
   1 - Record for Channel One
   2 - Record for Channel Two
   3 - Record for Channel Three
   4 - Record for Channel Four
   5 - Save Record to EEPROM
*/
int recordStatus = 0;

void setup() {
  // IR Reciever
  irrecv.enableIRIn();
  irrecv.blink13(true);

  // Preparing for channel
  for (int i = 0; i < CHANNEL_COUNT; i++)
  {
    pinMode(CHANNEL_PIN[i], OUTPUT);
    CHANNEL_STATE[i] = false;
  }

  pinMode(RecordLED, OUTPUT);

  // Push Button
  pinMode(RecordButton, INPUT);

  delay(1000);

  // Load IR Data
  readEEPROM(0);

  Serial.begin(9600);

  // For Error Tracing
  // PrintTempChannelData();
}

void loop() {
  currentMillis = millis();
  recordStatusUpdate();

  // Clear Tempory Channel Value when record channel is first channel and tempory data loaded from EEPROM is not cleared.
  if (recordStatus == 1 && !isClearTempData) {
    ClearTempData();
    isClearTempData = true;
    
    // For Error Tracing
    // PrintTempChannelData();
  }

  if (irrecv.decode(&irResults))
  {
    if (validIRCode(irResults) && irResults.value > 0)
    {
      // Running State
      if (recordStatus == 0)
      {
        for (int i = 0; i < CHANNEL_COUNT; i++)
        {
          for (int j = 0; j < CHANNEL_LENGTH; j++)
          {
            if ((unsigned long)channelArr[i][j] == (unsigned long)irResults.value)
            {
               /* 
                *  CHANNEL_STATE[i] = !CHANNEL_STATE[i];
                *  Show error when pressing button again and again above 7 times.
                */
              
              // Serial.println(CHANNEL_STATE[i]);
              if (CHANNEL_STATE[i])
                CHANNEL_STATE[i] = false;
              else
                CHANNEL_STATE[i] = true;

              digitalWrite(CHANNEL_PIN[i], CHANNEL_STATE[i]);
              // Serial.println(CHANNEL_STATE[i]);
              break;
            }
          }
        }
        delay(1000);  // Wait One Second for next IR remote press.
      }

      // Recording State (1 to 4)
      else if (recordStatus <= CHANNEL_COUNT)
      {
        // IR value not exists in Channel Array
        if (!isInArray(irResults.value, channelArr[recordStatus - 1]))
        {
          // Add to tempory channel array
          channelArr[recordStatus - 1][cIndex] = irResults.value;

          // Increase channel Index for next button
          // Becareful Channel Index start from 0 and end to 4, so Max value is CHANNEL_LENGTH-1
          cIndex = maxReset(cIndex, CHANNEL_LENGTH - 1);
        }
      }
    }
    irrecv.resume();
  }
}

/*
   When record button press update the recording status
   and display the recording status with recording LED.
   Turn recording LED On when recording state.
   Turn recording LED Off when recording finish or not recording state.

   Turn Channel LED On to show the current channel.
*/
void recordStatusUpdate() {
  if ((unsigned long)(currentMillis - previousMillis) >= pushButtonInterval)
  {
    // Change Recording Status
    if (digitalRead(RecordButton) == HIGH)
    {
      //Serial.println("High");
      // Increase recordStatus
      recordStatus = maxReset(recordStatus, 5);

      // Reset channel index to start of array when record channel change.
      cIndex = 0;
      //Serial.println(recordStatus);
    }

    // Save Record and Update recordStatus to Zero.
    if (recordStatus == 5)
    {
      // Save Record to EEPROM.
      saveEEPROM(0);

      // Set isClearData to false to clean data when record channel is One.
      isClearTempData = false;

      // Update LED status
      UpdateLED();

      // Update the record with max limit
      recordStatus = 0;
    }

    // Update LED status
    UpdateLED();

    previousMillis = millis();
  }
}

void UpdateLED() {
  //  Recording State
  if (recordStatus > 0 && recordStatus <= CHANNEL_COUNT)
  {
    for (int i = 0; i < CHANNEL_COUNT; i++) {
      digitalWrite(CHANNEL_PIN[i], LOW);
    }
    digitalWrite(RecordLED, HIGH);
    digitalWrite(CHANNEL_PIN[recordStatus - 1], HIGH);
  }
  //  Saving State.
  else if (recordStatus == 5)
  {
    for (int i = 0; i < CHANNEL_COUNT; i++) {
      digitalWrite(CHANNEL_PIN[i], LOW);
    }
    digitalWrite(RecordLED, HIGH);
  }
  else
  {
    digitalWrite(RecordLED, LOW);
  }
}

bool isInArray(unsigned long val, unsigned long arr[])
{
  for (int i = 0; i < CHANNEL_LENGTH; i++)
  {
    if (arr[i] == val)
      return true;
  }
  return false;
}

int maxReset(int _val, int _max)
{
  if (_val >= _max || _val < 0)
    return 0;
  else
    return _val + 1;
}

bool validIRCode(decode_results res)
{
  bool _res = false;
  switch (res.decode_type) {
    case NEC:
      _res = true;
      break ;
    case SONY:
      _res = true;
      break ;
    case RC5:
      _res = true;
      break ;
    case RC6:
      _res = true;
      break ;
    case DISH:
      _res = true;
      break ;
    case SHARP:
      _res = true;
      break ;
    case JVC:
      _res = true;
      break ;
    case SANYO:
      _res = true;
      break ;
    case MITSUBISHI:
      _res = true;
      break ;
    case SAMSUNG:
      _res = true;
      break ;
    case LG:
      _res = true;
      break ;
    case WHYNTER:
      _res = true;
      break ;
    case AIWA_RC_T501:
      _res = true;
      break ;
    case PANASONIC:
      _res = true;
      break ;
    case DENON:
      _res = true;
      break ;
    default:
    case UNKNOWN:
      _res = false;
      break ;
  }
  return _res;
}

void saveEEPROM(int addrOffset)
{
  // Concat all IR data with Comma seperator.
  String _str = "";
  for (int i = 0; i < CHANNEL_COUNT; i++)
  {
    for (int j = 0; j < CHANNEL_LENGTH; j++)
    {
      _str = _str + channelArr[i][j] + ",";
    }
  }

  // Save IR data to EEPROM
  byte _len = _str.length();

  EEPROM.write(addrOffset, _len);
  for (int i = 0; i < _len; i++)
  {
    EEPROM.write(addrOffset + 1 + i, _str[i]);
  }
}

// Read data from EEPROM
void readEEPROM(int addrOffset)
{
  int newStrLen = EEPROM.read(addrOffset);

  char c;
  int seperator = 0;
  String s = "";
  for (int i = 0; i < newStrLen; i++)
  {
    c = EEPROM.read(addrOffset + 1 + i);
    s += c;
    if (c == ',') {
      channelArr[IntDivider(seperator, CHANNEL_LENGTH, '/')][IntDivider(seperator, CHANNEL_LENGTH, '%')] = strtoul(s.c_str(), NULL, 0);
      seperator++;
      s = "";
    }
    delay(5);
  }

}

int IntDivider(int numerator, int denominator, char _operator) {
  if (numerator == 0)
    return 0;
  else if (_operator == '/')
    return numerator / denominator;
  else if (_operator == '%')
    return numerator % denominator;
}

void ClearTempData() {
  // Serial.println("Clear Temp Data Function");
  for (int i = 0; i < CHANNEL_COUNT; i++)
  {
    for (int j = 0; j < CHANNEL_LENGTH; j++)
    {
      channelArr[i][j] = (unsigned long)0;
    }
  }
}

// For Error Tracing
//void PrintTempChannelData() {
//  for (int i = 0; i < CHANNEL_COUNT; i++)
//  {
//    for (int j = 0; j < CHANNEL_LENGTH; j++)
//    {
//      Serial.println(channelArr[i][j]);
//    }
//  }
//}
