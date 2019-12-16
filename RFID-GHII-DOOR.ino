#include <deprecated.h>
#include <MFRC522.h>
#include <MFRC522Extended.h>
#include <require_cpp11.h>

#include <MFRC522.h>
#include <HTTPClient.h>
#include <EEPROM.h>  
#define COMMON_ANODE
#include <SPI.h> //library responsible for communicating of SPI bus
#include <Wire.h>  
#include "SSD1306.h" 
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h> 
#define SS_PIN    21
#define RST_PIN   22 
#define greenPin     32
#define EEPROM_SIZE 100
#define ledPin  27
#define buttonPin 12
//#define redPin       16 
#define relay 14    // Set Relay Pin
//#define buttonPin  18 // Set button  Pin
int buttonState = 0;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "mw.pool.ntp.org", 3600, 60000);//
//NTPClient timeClient(ntpUDP, "za.pool.ntp.org", 3600, 60000);//
MFRC522 mfrc522(SS_PIN, RST_PIN); 
boolean match = false;          // initialize card match to false
boolean programMode = false;  // initialize programming mode to false
int successRead;    // variable integer to keep if we have Successful Read from Reader
byte storedCard[4];    // Stores an ID read from EEPROM
byte readCard[4];   // Stores scanned ID read from RFID Module
byte masterCard[4];   // Stores master card's ID read from EEPROM
int MINUTES;
String SMINUTE;
int HOURS;
String SHOUR;
int SECOND;
String SSECOND;
char LEVEL [4];
String s = " ";
 String USER = "michael";
 
SSD1306  display(0x3C, 5, 4);

const char* ssid     = "micro-processor";
const char* password = "12341237";
 
void setup()
{
  Serial.begin(115200);
  EEPROM.begin(EEPROM_SIZE);
   SPI.begin(); // Init SPI bus
   pinMode(greenPin, OUTPUT); 
    pinMode(ledPin, OUTPUT); 
   pinMode(relay, OUTPUT);
   pinMode (buttonPin, INPUT);
  digitalWrite(relay, LOW);
  digitalWrite(ledPin, LOW);
  mfrc522.PCD_Init(); 

  


      Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println();
    display.init();
    
    display.setFont(ArialMT_Plain_10);

  display.clear();
  for (int i = 0; i < 500; i++) {
    int progress = (i / 5) % 100;
    // draw the progress bar
    display.drawProgressBar(0, 32, 120, 10, progress);

    // draw the percentage as String
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.drawString(64, 15, String(progress) + "%");
    //delay(5);
    display.setTextAlignment(TEXT_ALIGN_RIGHT);
    display.drawString(10, 128, String(millis()));
    // write the buffer to the display
    display.display();

    delay(8);
    display.clear();
  }

 


   if (EEPROM.read(1) != 143) {
   
    Serial.println(F("No Master Card Set"));
    Serial.println(F("Scan A RFID Card to Set as Master Card"));
    do {
      successRead = getID();

    }
    while (!successRead);                  // Program will not go further while you not get a successful read
    for ( int j = 0; j < 4; j++ ) {        // Loop 4 times
      EEPROM.write( 2 + j, readCard[j] );  // Write scanned PICC's UID to EEPROM, start from address 3
    }
    EEPROM.write(1, 143);                  // Write to EEPROM we defined Master Card.
    Serial.println(F("Master Card Set"));
  }
  Serial.println(F("-------------------"));
  Serial.println(F("Master Card's UID = "));
  for ( int i = 0; i < 4; i++ ) {          // Read Master Card's UID from EEPROM
    masterCard[i] = EEPROM.read(2 + i);    // Write it to masterCard
    Serial.print(masterCard[i], HEX);
  }
  Serial.println("");
  Serial.println(F("-------------------"));
  Serial.println(F("Everything Ready"));
  Serial.println(F("Waiting for Keys or cards to be scanned"));
   //lcd.println("SCAN CARDS");
   delay(4000);
//   lcd.clear();
 cycleLeds();    // Everything ready lets give v some feedback by cycling leds
}


void loop(){
   

     timeClient.begin();

  timeClient.update();
  HOURS = timeClient.getHours();
  MINUTES = timeClient.getMinutes();
  SECOND = timeClient.getSeconds();

   buttonState = digitalRead(buttonPin);

  if ( HOURS <= 18 && buttonState==HIGH ){
  digitalWrite(ledPin, HIGH);
    digitalWrite(relay, HIGH); 
     Serial.println("Access granted");
     delay(10000);
    }
    
  if (HOURS > 12) {
    HOURS = HOURS - 12;
    SHOUR = HOURS;
  }
   display.clear();
  display.setColor(WHITE);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(64, 0, SHOUR);
  display.display();
  Serial.println(timeClient.getFormattedTime());
  delay(1000);


      //Serial.println("Normal Mode");
      
   
  
  do {
    successRead = getID();   // sets successRead to 1 when we get read from reader otherwise 0
    if (programMode) {
      cycleLeds();              // Program Mode cycles through RGB waiting to read a new card

    }
    else {
      

         normalModeOn();     // Normal mode, blue Power LED is on, all others are off
  }
    }
  
  while (!successRead);   //the program will not go further while you not get a successful read
  if (programMode) {
    if ( isMaster(readCard) ) { //If master card scanned again exit program mode
      Serial.println(F("Master Card Scanned"));
      Serial.println(F("Exiting Programming Mode"));
      Serial.println(F("-----------------------------"));
      programMode = false;
      return;
    }
    else {
      if ( findID(readCard) ) { // If scanned card is known delete it
        Serial.println(F("I know this key, removing..."));
        deleteID(readCard);
        Serial.println("-----------------------------");
      }
      else {                    // If scanned card is not known add it
        Serial.println(F("I do not know this key, adding..."));
        writeID(readCard);
        Serial.println(F("-----------------------------"));
      }


    }



  }
  else {
    if ( isMaster(readCard) ) {   // If scanned card's ID matches Master Card's ID enter program mode
      programMode = true;
      Serial.println(F("Hello Master - Entered Programming Mode"));
      int count = EEPROM.read(0);   // Read the first Byte of EEPROM that
      Serial.print(F("I have "));     // stores the number of ID's in EEPROM
      Serial.print(count);
      Serial.print(F(" record(s) in DATABASE"));
      Serial.println("");
      Serial.println(F("Scan a Card or key to ADD or REMOvE"));
      Serial.println(F("-----------------------------"));
       EEPROM.commit(); 



    }
    else {
      if ( findID(readCard) ) { // If not, see if the card is in the EEPROM
      
        Serial.println(F("Welcome, Acces Granted"));
        Serial.println(F("access granted at : "));
        Serial.print(HOURS);
         Serial.println( " : " );
        Serial.println(MINUTES );
      
    

      
        HTTPClient http;
        http.begin("https://eps32project.000webhostapp.com/PROJ2/data.php");
        http.addHeader("Content-Type", "application/x-www-form-urlencoded");
       int httpResponseCode =http.POST("&USER=" + String(s));
           
      if (httpResponseCode >0){
          //check for a return code - This is more for debugging.
        String response = http.getString();
        Serial.println(httpResponseCode);
        Serial.println(response);
      }
      else{
        Serial.print("Error on sending post");
        Serial.println(httpResponseCode);
      }
    //closde the HTTP request.
      http.end();

         
        
          // Unlock door!
          granted(10000);         // Open the door lock for 300 ms
      }
      else {      // If not, show that the ID was not valid
        Serial.println(F("Acces Denied!"));
        denied();
      }

    }

  }


}
/////////////////////////////////////////  Access Granted    ///////////////////////////////////
void granted (int setDelay) {

  digitalWrite(relay, HIGH);
  digitalWrite(ledPin, HIGH);
  // Unlock door!
  delay(10000);           // Hold door lock open for given seconds
  digitalWrite(relay, LOW);
  digitalWrite(ledPin, LOW);// Relock door
  delay(1000);            // Hold green LED on for a second
}
///////////////////////////////////////// Access Denied  ///////////////////////////////////
void denied() {
  digitalWrite(relay, LOW);
  digitalWrite(ledPin, LOW);// Relock door
  delay(1000);            // Hold green LED on for a second

}
///////////////////////////////////////// Get PICC's UID ///////////////////////////////////
int getID() {
  // Getting ready for Reading PICCs
  
  if ( ! mfrc522.PICC_IsNewCardPresent()) { //If a new PICC placed to RFID reader continue
    return 0;
  }
  if ( ! mfrc522.PICC_ReadCardSerial()) {   //Since a PICC placed get Serial and continue
    return 0;
  }
  // There are Mifare PICCs which have 4 byte or 7 byte UID care if you use 7 byte PICC
  // I think we should assume every PICC as they have 4 byte UID
  // Until we support 7 byte PICCs
  s= "";
  Serial.println(F("Scanned KEY's UID:"));
  for (int i = 0; i < 4; i++) {  //
    readCard[i] = mfrc522.uid.uidByte[i];
    Serial.print(readCard[i], HEX);// POST
    LEVEL[i] = readCard[i];
    s += String(readCard[i],HEX);
   // Serial.print(LEVEL[i],HEX);
   

  }
  

   Serial.println("'''''''''''''");
    Serial.print(s);
   //Serial.println(S);
  
  Serial.println("");
  mfrc522.PICC_HaltA(); // Stop reading
  return 1;
}

void ShowReaderDetails() {
  // Get the MFRC522 software version
  byte U = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
  Serial.print(F("MFRC522 version: 0x"));
  Serial.print(U, HEX);
  if (U == 0x91)
    Serial.print(F(" = v1.0"));
  else if (U == 0x11)
    Serial.print(F(" = BlueCore Tech. RFID Acces v2.0"));
  else
    Serial.print(F(" (unknown)"));
  Serial.println("");
  // When 0x00 or 0xFF is returned, communication probably failed
  if ((U == 0x00) || (U == 0xFF)) {
    Serial.println(F("WARNING: Communication failure, is the RFID-MFRC522 properly connected?"));
    while (true); // do not go further
  }
}




///////////////////////////////////////// Cycle Leds (Program Mode) ///////////////////////////////////
void cycleLeds() {


  delay(600);
}
//////////////////////////////////////// Normal Mode Led  ///////////////////////////////////
void normalModeOn () {

 
    
        digitalWrite(relay, LOW); 
        digitalWrite(ledPin, LOW);
      
 
  

  
 
  
}

//////////////////////////////////////// Read an ID from EEPROM //////////////////////////////
void readID( int number ) {
  int start = (number * 4 ) + 2;     // Figure out starting position
  for ( int i = 0; i < 4; i++ ) {     // Loop 4 times to get the 4 Bytes
    storedCard[i] = EEPROM.read(start + i);   // Assign values read from EEPROM to array
  }
}

///////////////////////////////////////// Add ID to EEPROM   ///////////////////////////////////
void writeID( byte a[] ) {
  if ( !findID( a ) ) {     // Before we write to the EEPROM, check to see if we have seen this card before!
    int num = EEPROM.read(0);     // Get the numer of used spaces, position 0 stores the number of ID cards
    int start = ( num * 4 ) + 6;  // Figure out where the next slot starts
    num++;                // Increment the counter by one
    EEPROM.write( 0, num );     // Write the new count to the counter
    for ( int j = 0; j < 4; j++ ) {   // Loop 4 times
      EEPROM.write( start + j, a[j] );  // Write the array values to EEPROM in the right position
    }
    successWrite();
    Serial.println(F("Succesfully added ID record to DATABASE"));
  }
  else {
    failedWrite();
    Serial.println(F("Failed! There is something wrong with ID or bad DATABASE"));
  }

  if (programMode) {
    delay(10000);
    Serial.println(F("Master Card Scanned"));
    Serial.println(F("Exiting Programming Mode"));
    Serial.println(F("-----------------------------"));
    programMode = false;
    return;
  }

}

///////////////////////////////////////// Remove ID from EEPROM   ///////////////////////////////////
void deleteID( byte a[] ) {
  if ( !findID( a ) ) {     // Before we delete from the EEPROM, check to see if we have this card!
    failedWrite();      // If not
    Serial.println(F("Failed! There is something wrong with ID or bad DATABASE"));
  }
  else {
    int num = EEPROM.read(0);   // Get the numer of used spaces, position 0 stores the number of ID cards
    int slot;       // Figure out the slot number of the card
    int start;      // = ( num * 4 ) + 6; // Figure out where the next slot starts
    int looping;    // The number of times the loop repeats
    int j;
    int count = EEPROM.read(0); // Read the first Byte of EEPROM that stores number of cards
    slot = findIDSLOT( a );   // Figure out the slot number of the card to delete
    start = (slot * 4) + 2;
    looping = ((num - slot) * 4);
    num--;      // Decrement the counter by one
    EEPROM.write( 0, num );   // Write the new count to the counter
    for ( j = 0; j < looping; j++ ) {         // Loop the card shift times
      EEPROM.write( start + j, EEPROM.read(start + 4 + j));   // Shift the array values to 4 places earlier in the EEPROM
    }
    for ( int k = 0; k < 4; k++ ) {         // Shifting loop
      EEPROM.write( start + j + k, 0);
    }
    successDelete();
    Serial.println(F("Succesfully removed ID record from DATABASE"));
  }

}



///////////////////////////////////////// Check Bytes   ///////////////////////////////////
boolean checkTwo ( byte a[], byte b[] ) {
  if ( a[0] != NULL )       // Make sure there is something in the array first
    match = true;       // Assume they match at first
  for ( int k = 0; k < 4; k++ ) {   // Loop 4 times
    if ( a[k] != b[k] )     // IF a != b then set match = false, one fails, all fail
      match = false;
  }
  if ( match ) {      // Check to see if if match is still true
    return true;      // Return true
  }
  else  {
    return false;       // Return false
  }
}

///////////////////////////////////////// Find Slot   ///////////////////////////////////
int findIDSLOT( byte find[] ) {
  int count = EEPROM.read(0);       // Read the first Byte of EEPROM that
  for ( int i = 1; i <= count; i++ ) {    // Loop once for each EEPROM entry
    readID(i);                // Read an ID from EEPROM, it is stored in storedCard[4]
    if ( checkTwo( find, storedCard ) ) {   // Check to see if the storedCard read from EEPROM
      // is the same as the find[] ID card passed
      return i;         // The slot number of the card
      break;          // Stop looking we found it
    }
  }
}

///////////////////////////////////////// Find ID From EEPROM   ///////////////////////////////////
boolean findID( byte find[] ) {
  int count = EEPROM.read(0);     // Read the first Byte of EEPROM that
  for ( int i = 1; i <= count; i++ ) {    // Loop once for each EEPROM entry
    readID(i);          // Read an ID from EEPROM, it is stored in storedCard[4]
    if ( checkTwo( find, storedCard ) ) {   // Check to see if the storedCard read from EEPROM
      return true;
      break;  // Stop looking we found it
    }
    else {    // If not, return false
    }
  }
  return false;
}

///////////////////////////////////////// Write Success to EEPROM   ///////////////////////////////////
// Flashes the green LED 3 times to indicate a successful write to EEPROM
void successWrite() {
  delay(200);
}
///////////////////////////////////////// Write Failed to EEPROM   ///////////////////////////////////
// Flashes the red LED 3 times to indicate a failed write to EEPROM
void failedWrite() {

  delay(200);
}

///////////////////////////////////////// Success Remove UID From EEPROM  ///////////////////////////////////
// Flashes the blue LED 3 times to indicate a success delete to EEPROM
void successDelete() {

  delay(200);
}
////////////////////// Check readCard IF is masterCard   ///////////////////////////////////
// Check to see if the ID passed is the master programing card
boolean isMaster( byte test[] ) {
  if ( checkTwo( test, masterCard ) )
    return true;
  else

    return false;

  if (programMode) {
    delay(20000);
    Serial.println(F("time up Exiting Programming Mode"));
    Serial.println(F("-----------------------------"));
    programMode = false;
   // return;
  }

    }

  
  
  
  

