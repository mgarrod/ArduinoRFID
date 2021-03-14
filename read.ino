#include <EEPROM.h>
#include <Wire.h>
#include "SparkFun_UHF_RFID_Reader.h" 

// start reading from the first byte (address 0) of the EEPROM
int address = 0;
byte value;
long memoryCounter = 0;
#define EEPROM_ADR 0x50

void setup() {

  Wire.begin();
  Wire.setClock(400000);
  
  // initialize serial and wait for port to open:
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  //delay(10000);
  delay(5000);
  Serial.println("hi");

  bool dataInMemory = true;

  while(dataInMemory) {
    String recievedData = readEEPROM(memoryCounter);
    //Serial.println(recievedData);
    char char_array[recievedData.length() + 1];
    recievedData.toCharArray(char_array, recievedData.length() + 1);
    char* pch = strtok (char_array, "|");
    String oneEntry = String(pch);
    Serial.println(oneEntry);
    
    if (oneEntry.indexOf("@") == -1) {
      dataInMemory = false;
      
    }
    else {
      //charcounter += oneEntry.length() + 1; // might be +2 because of \0?
      memoryCounter += oneEntry.length() + 1;
    }

  }
  
  
}

void loop() {

  delay(1000);

//  String recivedData = read_String(address);
//  //Serial.print(String(address) + ": ");
//  //delay(10000);
//  Serial.println(recivedData);
//  address += recivedData.length() + 1;
//  if (recivedData.length() == 0) {
//    Serial.println("done");
//  }
//  delay(1000);

//
//  
//  // read a byte from the current address of the EEPROM
//  value = EEPROM.read(address);
//
//  Serial.print(address);
//  Serial.print("\t");
//  Serial.print(value);
//  Serial.println();

  /***
    Advance to the next address, when at the end restart at the beginning.

    Larger AVR processors have larger EEPROM sizes, E.g:
    - Arduno Duemilanove: 512b EEPROM storage.
    - Arduino Uno:        1kb EEPROM storage.
    - Arduino Mega:       4kb EEPROM storage.

    Rather than hard-coding the length, you should use the pre-provided length function.
    This will make your code portable to all AVR processors.
  ***/
  //address = address + 1;
//  if (address == EEPROM.length()) {
//    Serial.println("max length");
//    address = 0;
//  }

  /***
    As the EEPROM sizes are powers of two, wrapping (preventing overflow) of an
    EEPROM address is also doable by a bitwise and of the length - 1.

    ++address &= EEPROM.length() - 1;
  ***/

  //delay(500);
}

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

String read_String(char add)
{
  int i;
  char data[100]; //Max 100 Bytes
  int len=0;
  unsigned char k;
  k=EEPROM.read(add);
  while(k != '\0' && len<500)   //Read until null character
  {    
    k=EEPROM.read(add+len);
    data[len]=k;
    len++;
  }
  data[len]='\0';
  return String(data);
}
