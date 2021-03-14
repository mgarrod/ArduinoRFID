

#include <Wire.h>
#define eeprom1 0x50    //Address of 24LC256 eeprom chip
#define WRITE_CNT 5

char rdata[32];

void setup(void)
{
  Serial.begin(115200);
  Wire.begin();  

  //uncomment to erase
  for (long x = 0 ; x < 300 ; x++) //Read all 131,071 bytes from EERPOM
  {
    eraseEEPROM(eeprom1,x,0xFF);
    //byte val = readEEPROM(disk1, x);
    //Serial.println(val);
  }
 
  // define large string of data to be written
  String data = "e0c1104eec972601201c7,100,120000|";
  //data += "\0";
  char charBuf[data.length() + 1];
  data.toCharArray(charBuf, data.length() + 1) ;
  //writeEEPROM(eeprom1,0,charBuf);

  String data2 = "e0c1104eec988991201c7,101,120000|";
  //data2 += "\0";
  char charBuf2[data2.length() + 1];
  data2.toCharArray(charBuf2, data2.length() + 1) ;
  //writeEEPROM(eeprom1,data2.length(),charBuf2);

  // read back the data 28 bytes at a time
  // reading data doesn't suffer from the page boundary rules
  Serial.println("DATA READ");
  //String recievedData = readEEPROM(eeprom1, 0);
  //Serial.println(recievedData);

//  for(int i=0;i<10;i++) {
//    readEEPROM3(eeprom1, (i*28), rdata, 28);
//    Serial.write(rdata,28);
//  } 
  
  //Serial.println(oneEntry);
  long memoryCounter = 0;
  bool dataInMemory = true;
  while(dataInMemory) {
  //for(int i=0;i<10;i++) {
    //readEEPROM(eeprom1, (i*30), rdata, 30);
    String recievedData = readEEPROM2(eeprom1, memoryCounter);
    char char_array[recievedData.length() + 1];
    recievedData.toCharArray(char_array, recievedData.length() + 1);
    char* pch = strtok (char_array, "|");
    String oneEntry = String(pch);

    if (oneEntry.indexOf("@") == -1) {
      dataInMemory = false;
    }
    else {
      Serial.println(oneEntry);
      memoryCounter += oneEntry.length() + 1;
    }
    
    delay(5000);
    Serial.write(rdata,30);
    Serial.println(String(rdata));
    Serial.println(String(rdata).indexOf("\0"));
  }  

}
 
void loop(){
}

String readEEPROM(int deviceaddress, unsigned int eeaddress) 
{
  unsigned char i=0;
  char data[40];
  int len=0;
  byte k = 0xFF;
  while(len<40)   //Read until null character
  {    
    eeaddress += len;
    Wire.beginTransmission(deviceaddress);
    Wire.write((int)(eeaddress >> 8));   // MSB
    Wire.write((int)(eeaddress & 0xFF)); // LSB
    Wire.endTransmission();
   
    Wire.requestFrom(deviceaddress,1);
   
    while(Wire.available()) k = Wire.read();
    data[len]=k;
    len++;
  }
  //data[len]='\0';
  return String(data);
  
}

void readEEPROM3(int deviceaddress, unsigned int eeaddress,  
                 unsigned char* data, unsigned int num_chars) 
{
  unsigned char i=0;
  Wire.beginTransmission(deviceaddress);
  Wire.write((int)(eeaddress >> 8));   // MSB
  Wire.write((int)(eeaddress & 0xFF)); // LSB
  Wire.endTransmission();
 
  Wire.requestFrom(deviceaddress,num_chars);
 
  while(Wire.available()) data[i++] = Wire.read();

}

String readEEPROM2(int deviceaddress, unsigned int eeaddress) 
{
  char rdata[30];
  unsigned char i=0;
  Wire.beginTransmission(deviceaddress);
  Wire.write((int)(eeaddress >> 8));   // MSB
  Wire.write((int)(eeaddress & 0xFF)); // LSB
  Wire.endTransmission();
 
  Wire.requestFrom(deviceaddress,30);
 
  while(Wire.available()) rdata[i++] = Wire.read();

  return String(rdata);
}
 
void writeEEPROM(int deviceaddress, unsigned int eeaddress, char* data) 
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
  
     Wire.beginTransmission(deviceaddress);
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
 


void eraseEEPROM(int deviceaddress, unsigned int eeaddress, byte data ) 
{
  Wire.beginTransmission(deviceaddress);
  Wire.write((int)(eeaddress >> 8));   // MSB
  Wire.write((int)(eeaddress & 0xFF)); // LSB
  Wire.write(data);
  Wire.endTransmission();
 
  delay(5);
}


/*

#include <Wire.h>    
 
#define disk1 0x50    //Address of 24LC256 eeprom chip



long counter = 0;

const int maxCharsPerLine = 5;
unsigned char dataFromEeprom[maxCharsPerLine];

void setup(void)
{
  Serial.begin(115200);
  Wire.begin();  
 
  unsigned int address = 0;

//  for (long x = 0 ; x < 0x7D00 ; x++) //Read all 131,071 bytes from EERPOM
//  {
//    writeNEEPROM(disk1,x,0);
//    //byte val = readEEPROM(disk1, x);
//    //Serial.println(val);
//  }
 
  //writeEEPROM(disk1, address, 254);
  //writeNewEEPROM(disk1, "hello");
  String tempStringBuffer = "epc[0c 11 04 ee c9 72 60 12 01 c7],100,120";
  writeNEEPROM(disk1,address,tempStringBuffer);
  //writeNewEEPROM(disk1, tempStringBuffer);

  Serial.print("1 : ");             // This works
  Serial.println(tempStringBuffer);
  
  Serial.print("2 : ");
  Serial.println("OK let's see if we can read this back");

  int numCharsToRead = tempStringBuffer.length();
  readNEEPROM(disk1,address,dataFromEeprom,numCharsToRead);

  for(int j=0;j<numCharsToRead;j++){
    Serial.print((char)dataFromEeprom[j]);
  }
  Serial.println();
          
  //*tempStringBuffer = (dataFromEeprom,numCharsToRead);

//  Serial.print("3 : ");               // This does NOT work always looses the first byte?
//  Serial.println(tempStringBuffer);
  //Serial.print(readEEPROM(disk1, address), DEC);
}
 
void loop(){}

void writeNEEPROM(int deviceaddress, unsigned int eeaddress, String data) 
{
  // Write a string of chars to eeprom
  // Only works upto page boundary
  // This is OK here as I'm only storing strings of max length 20 chars  
  unsigned char i=0;
  
  Wire.beginTransmission(deviceaddress);
  Wire.write((int)((eeaddress) >> 8));   // MSB
  Wire.write((int)((eeaddress) & 0xFF)); // LSB

  do{ 
     Wire.write((byte) data[i]);
     i++;
  } while(data[i]);  
  //Wire.write((byte) "\0");
  Wire.endTransmission();
     
  delay(6);  // needs 5ms for page write
}

void readNEEPROM(int deviceaddress, unsigned int eeaddress,  
                 unsigned char* data, unsigned int num_chars) 
{
  unsigned char i=0;
  Wire.beginTransmission(deviceaddress);
  Wire.write((int)(eeaddress >> 8));   // MSB
  Wire.write((int)(eeaddress & 0xFF)); // LSB
  Wire.endTransmission();
 
  Wire.requestFrom(deviceaddress,num_chars);
 
  const uint8_t error =  Wire.endTransmission();
  for (uint16_t i =0; (i < num_chars); i++)
    { 
      *data++ = Wire.read();
    }

}
 
void writeEEPROM(int deviceaddress, unsigned int eeaddress, byte data ) 
{
  Wire.beginTransmission(deviceaddress);
  Wire.write((int)(eeaddress >> 8));   // MSB
  Wire.write((int)(eeaddress & 0xFF)); // LSB
  Wire.write(data);
  Wire.endTransmission();
 
  delay(5);
}

void writeNewEEPROM(int deviceaddress, String data)
{


//  for (int i=0; i < data.length(); i++){
//      Wire.beginTransmission(deviceaddress);
//      Wire.write((int)(counter >> 8));  // MSB
//      Wire.write((int)(counter & 0xFF));// LSB
//      Wire.write(data[i]);
//      Wire.endTransmission();
//      counter++;
//      Serial.println(data[i]);
//      delay(5);
//  }
//  Wire.beginTransmission(deviceaddress);
//  Wire.write((int)(counter >> 8));  // MSB
//  Wire.write((int)(counter & 0xFF));// LSB
//  Wire.write('\0');
//  Wire.endTransmission();
//  counter++;
//  
//
  Wire.beginTransmission(deviceaddress);

  Wire.write(data.c_str());
  Wire.write('\0');
  
  Wire.endTransmission(); //Send stop condition
}
 
byte readEEPROM(int deviceaddress, unsigned int eeaddress ) 
{

//  Wire.beginTransmission(deviceaddress);
//  Wire.write((int)(eeaddress >> 8));   // MSB
//  Wire.write((int)(eeaddress & 0xFF)); // LSB
//  Wire.endTransmission();
//  
//  char data[100]; //Max 100 Bytes
//  int len=0;
//  unsigned char k;
//  //k=EEPROM.read(add);
//  if (Wire.available()) k = Wire.read();
//  while(k != '\0' && len<500)   //Read until null character
//  {    
//    k=EEPROM.read(add+len);
//    data[len]=k;
//    len++;
//  }
//  data[len]='\0';
//  return String(data);
//
//  
  byte rdata = 0xFF;
 
  Wire.beginTransmission(deviceaddress);
  Wire.write((int)(eeaddress >> 8));   // MSB
  Wire.write((int)(eeaddress & 0xFF)); // LSB
  Wire.endTransmission();
 
  Wire.requestFrom(deviceaddress,5);
 
  if (Wire.available()) rdata = Wire.read();
 
  return rdata;
}*/

/*#include <Wire.h>


void setup()
{
  Wire.begin();

  Serial.begin(115200);
  while (!Serial);             // Leonardo: wait for serial monitor
  Serial.println("\nI2C Scanner");
}


void loop()
{
  byte error, address;
  int nDevices;

  Serial.println("Scanning...");

  nDevices = 0;
  for(address = 1; address < 127; address++ ) 
  {
    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0)
    {
      Serial.print("I2C device found at address 0x");
      if (address<16) 
        Serial.print("0");
      Serial.print(address,HEX);
      Serial.println("  !");

      nDevices++;
    }
    else if (error==4) 
    {
      Serial.print("Unknown error at address 0x");
      if (address<16) 
        Serial.print("0");
      Serial.println(address,HEX);
    }    
  }
  if (nDevices == 0)
    Serial.println("No I2C devices found\n");
  else
    Serial.println("done\n");

  delay(5000);           // wait 5 seconds for next scan
}
*/
/*#include <EEPROM.h>
#include <SoftwareSerial.h>

SoftwareSerial OpenLCD(6, 7); //RX, TX

void setup() {
  // put your setup code here, to run once:
  OpenLCD.begin(9600);
  for (int i = 0 ; i < EEPROM.length() ; i++) {
    EEPROM.write(i, 0);
  }
  OpenLCD.write('|'); //Put LCD into setting mode
  OpenLCD.write('-');
  OpenLCD.write(24); //Send contrast command
  OpenLCD.write(2);
  OpenLCD.print("EEPROM Cleared");
}

void loop() {
  // put your main code here, to run repeatedly:

}*/
