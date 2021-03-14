#include <SoftwareSerial.h>

SoftwareSerial OpenLCD(6, 7); //RX, TX

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200); //Start serial communication at 9600 for debug statements
  while (!Serial); //Wait for the serial port to come online
   
  Serial.println("RFID Reader");
  OpenLCD.begin(9600); //Start communication with OpenLCD

  //Send contrast setting
  OpenLCD.write('|'); //Put LCD into setting mode
  OpenLCD.write('-');
  OpenLCD.write(24); //Send contrast command
  OpenLCD.write(1);
  
  OpenLCD.print("Hey Guy");
}

void loop() {
  // put your main code here, to run repeatedly:

}
