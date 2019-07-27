#include <SoftwareSerial.h>
#include <Wire.h>
#include "SparkFun_UHF_RFID_Reader.h" //Library for controlling the M6E Nano module

SoftwareSerial OpenLCD(6, 7); //RX, TX
SoftwareSerial softSerial(2, 3); //RX, TX

RFID nano; //Create instance

#define BUZZER1 9
//#define BUZZER1 0 //For testing quietly
#define BUZZER2 10

// where the external memory is located
#define EEPROM_ADR 0x50

#define RUNNER_SIZE 2
#define LAP_LENGTH 400 // in meters

// musical notes for buzzer
const int C = 2093;
const int D = 2349;
const int E = 2637;

class Runner
{
  // Class Member Variables
  // These are initialized at startup
  public: 
  String runnerName;
  String tagId;
  int buzz1;
  int buzz2;
  int buzz3;
  
  int lapCounter;
  unsigned long lapTime;
  unsigned long delayStart;
  bool delayStartB;

  // Constructor - creates a Runner 
  // and initializes the member variables and state
  public:
  Runner() {}
  Runner(String pname, String ptagId, int pbuzz1, int pbuzz2, int pbuzz3)
  {
    runnerName = pname;
    tagId = ptagId;     
    buzz1 = pbuzz1;
    buzz2 = pbuzz2;
    buzz3 = pbuzz3;
      
    lapCounter = -1;
    lapTime = 0;
    delayStart = 0;
    delayStartB = true;
  }

  String getDisplayText();

};

String Runner::getDisplayText() {
  if (lapCounter == 0) lapTime = 0;
  String distance = "0.00";
  if (lapCounter > 0) distance = String(round(((lapCounter * LAP_LENGTH) * 0.000621371) * 100)/100);
  String displayText = runnerName + " " + String(lapCounter) + " " + distance + " " + String(lapTime / 1000);
  // LCD is 20 chars long, need to pad it for additional lines
  String spaces = "";
  for (int i=0;i < (20 - displayText.length());i++) {
    spaces += " ";
  }
  return displayText + spaces;
}

Runner runners[2];

long memoryCounter = 0;

byte contrast = 2; //Lower is more contrast. 0 to 5 works for most displays.
long charcounter = 0;
unsigned long totalTimeMil = 0;

void setup() {

  Wire.begin();
  Wire.setClock(400000);
  
  Serial.begin(115200); //Start serial communication at 9600 for debug statements
  while (!Serial); //Wait for the serial port to come online
   
  Serial.println("RFID Reader");

  OpenLCD.begin(9600); //Start communication with OpenLCD

  //Send contrast setting
  OpenLCD.write('|'); //Put LCD into setting mode
  OpenLCD.write('-');
  OpenLCD.write(24); //Send contrast command
  OpenLCD.write(contrast);
  OpenLCD.print("RFID Reader");

  pinMode(BUZZER1, OUTPUT);
  pinMode(BUZZER2, OUTPUT);

  digitalWrite(BUZZER2, LOW); //Pull half the buzzer to ground and drive the other half.

  if (setupNano(38400) == false) //Configure nano to run at 38400bps
  {
    Serial.println(F("Module failed to respond. Please check wiring."));
    while (1); //Freeze!
  }

  nano.setRegion(REGION_NORTHAMERICA); //Set to North America

  //nano.setReadPower(500); //5.00 dBm. Higher values may caues USB port to brown out
  nano.setReadPower(2400); //You'll need an external power supply for this setting
  //Max Read TX Power is 27.00 dBm and may cause temperature-limit throttling

  // setup the runners
  runners[0] = Runner("Ava", "e0c1104eec972601201c7", C, D, E);
  //runners[1] = Runner("Mad", "e0c1104eec988991201c7", E, D, C);
  //runners[0] = Runner("Matt", "ee2000019630200660420e8de", C, D, E);
  runners[1] = Runner("Kara", "ee2000019630200710420e8d8", E, D, C);

  // determine if data is in memory
  bool dataInMemory = true;
  String firstTag = "";
  String secondTag = "";
  unsigned long firstTagTime = 0;
  unsigned long secondTagTime = 0;
  while(dataInMemory) {
    String recievedData = readEEPROM(memoryCounter);
    char char_array[recievedData.length() + 1];
    recievedData.toCharArray(char_array, recievedData.length() + 1);
    char* pch = strtok (char_array, "|");
    String oneEntry = String(pch);
    //Serial.println(recievedData);
    
    if (oneEntry.indexOf("@") == -1) {
      dataInMemory = false;
    }
    else {
      char char_array[oneEntry.length() + 1];
      oneEntry.toCharArray(char_array, oneEntry.length() + 1);
      char* pch = strtok (char_array, ",");
      String at = String(pch);
      pch = strtok(NULL, ",");
      String mTagId = String(pch);
      pch = strtok(NULL, ",");
      int mLapCounter = String(pch).toInt();
      pch = strtok(NULL, ",");
      int mLapTime = String(pch).toInt();

      if (firstTag == "") {
        firstTag = mTagId;
      }
      else if (secondTag == "") {
        secondTag = mTagId;
      }

      if (totalTimeMil == 0) {
        totalTimeMil = millis();
      }

      for (int x = 0 ; x < RUNNER_SIZE ; x++) {
        
        if (mTagId == runners[x].runnerName) {
          runners[x].lapCounter = mLapCounter;
          runners[x].lapTime = mLapTime;
          runners[x].delayStart = millis();
          //runners[x].delayStartB = false;
        }
        if (firstTag == runners[x].runnerName && firstTag == mTagId) {
          firstTagTime += mLapTime;
        }
        else if (secondTag == runners[x].runnerName && secondTag == mTagId) {
          secondTagTime += mLapTime;
        }
      }

      charcounter += oneEntry.length() + 1; // might be +2 because of \0?
      memoryCounter += oneEntry.length() + 1;
    }
  }

  if (totalTimeMil > 0) {
    if (firstTagTime > secondTagTime) {
      totalTimeMil -= firstTagTime;
    }
    else {
      totalTimeMil -= secondTagTime;
    }
  }
  String totalTime = getTotalTime();
  
  delay(2000);
  OpenLCD.write('|'); //Put LCD into setting mode
  OpenLCD.write('-');
  OpenLCD.print("Name Laps Dis Time  " + runners[0].getDisplayText() + runners[1].getDisplayText() + totalTime);
  nano.startReading(); //Begin scanning for tags
}

void loop() {
  if (nano.check() == true) //Check to see if any new data has come in from module
  {
    byte responseType = nano.parseResponse(); //Break response into tag ID, RSSI, frequency, and timestamp

    if (responseType == RESPONSE_IS_KEEPALIVE)
    {
      Serial.println(F("Scanning"));
    }
    else if (responseType == RESPONSE_IS_TAGFOUND)
    {
      //If we have a full record we can pull out the fun bits
      int rssi = nano.getTagRSSI(); //Get the RSSI for this tag read
      long freq = nano.getTagFreq(); //Get the frequency this tag was detected at
      long timeStamp = nano.getTagTimestamp(); //Get the time this was read, (ms) since last keep-alive message
      byte tagEPCBytes = nano.getTagEPCBytes(); //Get the number of bytes of EPC from response

      //Print EPC bytes, this is a subsection of bytes from the response/msg array
      String tag = "e";
      for (byte x = 0 ; x < tagEPCBytes ; x++)
      {
        if (nano.msg[31 + x] < 0x10) tag += "0"; //Serial.print(F("0")); //Pretty print
        tag += String(nano.msg[31 + x], HEX);
        //if (x != tagEPCBytes-1) tag += " ";
      }
      tag += "";

      String displayText = "";

      // find the runner and update things
      for (int x = 0 ; x < RUNNER_SIZE ; x++) {
        // tag of a runner was read and either they just started or enough time has elapsed to not get a double read
        if (tag == runners[x].tagId && (runners[x].delayStartB || (millis() - runners[x].delayStart) >= 5000)) { // change to 60000
          runners[x].delayStartB = false;
          runners[x].lapTime = millis() - runners[x].delayStart;
          runners[x].delayStart = millis();
          runners[x].lapCounter = runners[x].lapCounter + 1;
          displayText += runners[x].getDisplayText();
          // save here
          if (runners[x].lapCounter > 0) {
            //String datatowrite = String(runners[x].runnerName.charAt(0)) + "," + String(runners[x].lapCounter) + "," + String(runners[x].lapTime);
            String datatowrite = "@," + runners[x].runnerName + "," + String(runners[x].lapCounter) + "," + String(runners[x].lapTime) + "|";
            //writeEEPROM(datatowrite);
            char charBuf[datatowrite.length() + 1];
            datatowrite.toCharArray(charBuf, datatowrite.length() + 1) ;
            writeEEPROM(charcounter,charBuf);
            charcounter += datatowrite.length();
          }
          
          // start the timer on the first valid tag read
          if (totalTimeMil == 0) {
            totalTimeMil = millis();
          }
          
          Serial.print(displayText);
          Serial.println();

          // play a tone unique to the runner
          tone(BUZZER1, runners[x].buzz1, 150);
          delay(150);
          tone(BUZZER1, runners[x].buzz2, 150);
          delay(150);
          tone(BUZZER1, runners[x].buzz3, 150);
          delay(150);

        }
        else {
          //Serial.println("New tag: " + tag);
          displayText += runners[x].getDisplayText();
        }
      }
      
      String totalTime = getTotalTime();
   
      OpenLCD.write('|');
      OpenLCD.write('-');
      OpenLCD.print("Name Laps Dis Time  " + displayText + totalTime);
    }
    else if (responseType == ERROR_CORRUPT_RESPONSE)
    {
      Serial.println("Bad CRC");
    }
    else
    {
      //Unknown response
      Serial.print("Unknown error");
    }
  }
}

String getTotalTime() {
  
  String totalTime = "";
  if (totalTimeMil > 0) {
    unsigned long allSeconds=(millis() - totalTimeMil)/1000;
    int runHours= allSeconds/3600;
    int secsRemaining=allSeconds%3600;
    int runMinutes=secsRemaining/60;
    int runSeconds=secsRemaining%60;
    char buf[21];
    sprintf(buf,"Total Time: %02d:%02d:%02d",runHours,runMinutes,runSeconds);
    totalTime = buf;
  }
  return totalTime;
}

//Gracefully handles a reader that is already configured and already reading continuously
//Because Stream does not have a .begin() we have to do this outside the library
boolean setupNano(long baudRate)
{
  nano.begin(softSerial); //Tell the library to communicate over software serial port

  //Test to see if we are already connected to a module
  //This would be the case if the Arduino has been reprogrammed and the module has stayed powered
  softSerial.begin(baudRate); //For this test, assume module is already at our desired baud rate
  while (softSerial.isListening() == false); //Wait for port to open

  //About 200ms from power on the module will send its firmware version at 115200. We need to ignore this.
  while (softSerial.available()) softSerial.read();

  nano.getVersion();

  if (nano.msg[0] == ERROR_WRONG_OPCODE_RESPONSE)
  {
    //This happens if the baud rate is correct but the module is doing a ccontinuous read
    nano.stopReading();

    Serial.println(F("Module continuously reading. Asking it to stop..."));

    delay(1500);
  }
  else
  {
    //The module did not respond so assume it's just been powered on and communicating at 115200bps
    softSerial.begin(115200); //Start software serial at 115200

    nano.setBaud(baudRate); //Tell the module to go to the chosen baud rate. Ignore the response msg

    softSerial.begin(baudRate); //Start the software serial port, this time at user's chosen baud rate

    delay(250);
  }

  //Test the connection
  nano.getVersion();
  if (nano.msg[0] != ALL_GOOD) return (false); //Something is not right

  //The M6E has these settings no matter what
  nano.setTagProtocol(); //Set protocol to GEN2

  nano.setAntennaPort(); //Set TX/RX antenna ports to 1

  return (true); //We are ready to rock
}

// Thanks to this dude for the next 2 functions: http://www.hobbytronics.co.uk/eeprom-page-write
String readEEPROM(unsigned int eeaddress) 
{
  char rdata[30];
  unsigned char i=0;
  Wire.beginTransmission(EEPROM_ADR);
  Wire.write((int)(eeaddress >> 8));   // MSB
  Wire.write((int)(eeaddress & 0xFF)); // LSB
  Wire.endTransmission();
 
  Wire.requestFrom(EEPROM_ADR,30);
 
  while(Wire.available()) rdata[i++] = Wire.read();

  return String(rdata);
}

void writeEEPROM(unsigned int eeaddress, char* data) 
{
  // Uses Page Write for 24LC256
  // Allows for 64 byte page boundary
  // Splits string into max 16 byte writes
  unsigned char i=0, counter=0;
  unsigned int  address;
  unsigned int  page_space;
  unsigned int  page=0;
  unsigned int  num_writes;
  unsigned int  data_len=0;
  unsigned char first_write_size;
  unsigned char last_write_size;  
  unsigned char write_size;  
  
  // Calculate length of data
  do{ data_len++; } while(data[data_len]);   
   
  // Calculate space available in first page
  page_space = int(((eeaddress/64) + 1)*64)-eeaddress;

  // Calculate first write size
  if (page_space>16){
     first_write_size=page_space-((page_space/16)*16);
     if (first_write_size==0) first_write_size=16;
  }   
  else 
     first_write_size=page_space; 
    
  // calculate size of last write  
  if (data_len>first_write_size) 
     last_write_size = (data_len-first_write_size)%16;   
  
  // Calculate how many writes we need
  if (data_len>first_write_size)
     num_writes = ((data_len-first_write_size)/16)+2;
  else
     num_writes = 1;  
     
  i=0;   
  address=eeaddress;
  for(page=0;page<num_writes;page++) 
  {
     if(page==0) write_size=first_write_size;
     else if(page==(num_writes-1)) write_size=last_write_size;
     else write_size=16;
  
     Wire.beginTransmission(EEPROM_ADR);
     Wire.write((int)((address) >> 8));   // MSB
     Wire.write((int)((address) & 0xFF)); // LSB
     counter=0;
     do{ 
        Wire.write((byte) data[i]);
        i++;
        counter++;
     } while((data[i]) && (counter<write_size));  
     Wire.endTransmission();
     address+=write_size;   // Increment address for next write
     
     delay(6);  // needs 5ms for page write
  }
}
