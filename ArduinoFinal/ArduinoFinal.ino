#include <ArduinoBLE.h>
#include <Arduino_HTS221.h>
#include "TimeoutTimer.h"
#define BUFSIZE 20

/*   We'll use the ArduinoBLE library to simulate a basic UART connection 
 *   following this UART service specification by Nordic Semiconductors. 
 *   More: https://learn.adafruit.com/introducing-adafruit-ble-bluetooth-low-energy-friend/uart-service
 */
BLEService uartService("6E400001-B5A3-F393-E0A9-E50E24DCCA9E");
BLEStringCharacteristic txChar("6E400002-B5A3-F393-E0A9-E50E24DCCA9E", BLEWrite, 20 );
BLEStringCharacteristic rxChar("6E400003-B5A3-F393-E0A9-E50E24DCCA9E", BLERead | BLENotify, 20 );

/*  Create a Environmental Sensing Service (ESS) and a 
 *  characteristic for its temperature value.
 */
BLEService essService("181A");
BLEShortCharacteristic tempChar("2A6E", BLERead | BLENotify );
int onOffPin = 5;
int incPin = 4;
int decPin = 3;

void setup() 
{
  Serial.begin(9600);
  while(!Serial);

  if (!HTS.begin()) {
    Serial.println("Failed to initialize humidity temperature sensor!");
    while (1);
  }
  if ( !BLE.begin() )
  {
    Serial.println("Starting BLE failed!");
    while(1);
  }

  // Get the Arduino's BT address
  String deviceAddress = BLE.address();

  // The device name we'll advertise with.
  BLE.setLocalName("ArduinoBLE Lab3");

  // Get UART service ready.
  BLE.setAdvertisedService( uartService );
  uartService.addCharacteristic( txChar );
  uartService.addCharacteristic( rxChar );
  BLE.addService( uartService );

  // Get ESS service ready.
  essService.addCharacteristic( tempChar );
  BLE.addService( essService );

  // Start advertising our new service.
  BLE.advertise();
  Serial.println("Bluetooth device (" + deviceAddress + ") active, waiting for connections...");
  pinMode(decPin, INPUT);    // declare pushbutton as input
  pinMode(incPin, INPUT);
  pinMode(onOffPin, INPUT);
}

void loop() 
{
  // Wait for a BLE central device.
  BLEDevice central = BLE.central();
  String newState = ""; //Initialize variable to hold the new state (on/off, increment or decrement)
  String currentState = "s0";//0 for off, 1 for on
  bool toggleWasDown = false;
  bool incWasDown = false;
  bool decWasDown = false;
  int intensity = 0;//Intensity on a scale of 1-10 for dimness of the lights
  // If a central device is connected to the peripheral...
  if ( central )
  {
    // Print the central's BT address.
    Serial.print("Connected to central: ");
    Serial.println( central.address() );
    
    // While the central device is connected...
    while( central.connected() )
    {
      
      bool toggleDown = readButton(onOffPin);
      bool incDown = readButton(incPin);
      bool decDown = readButton(decPin);
 
      if(toggleDown && !toggleWasDown){
        Serial.println("ON/OFF Button Pressed");
        if(currentState == "s0"){
          currentState = "s1";
        }
        else{
          currentState = "s0";
          intensity = 0;
        }
        sendToggle(currentState);
      }
      if(incDown && !incWasDown){
        Serial.println("Increment Button Pressed");
        if(intensity<10){
          intensity++;
        }
      }
      if(decDown && !decWasDown){
        Serial.println("Decrement Button Pressed");
        if(intensity>0){
          intensity--;
          if(intensity == 0){
            currentState = "s0";
            sendToggle(currentState);
          }
        }
      }
      
      toggleWasDown = toggleDown;
      incWasDown = incDown;
      decWasDown = decDown;
      
      // Receive data from central (if written is true)
      if ( txChar.written() )
      {
        newState = txChar.value();
        Serial.println("[RCVD] State from Central: "+newState);
        
        if(newState.equals("p")){
          sendIntensity(intensity);
        }
        else if(newState.equals("s0")){
          if(currentState == "s0"){//If the light was off
            currentState = "s1"; //Light is now on
          }
          else if(currentState == "s1"){//if the light was on
            currentState = "s0"; //Light is now off
            intensity = 0;
          }
          sendToggle(currentState);
        }
        else if(newState.equals("s1")){
          //increment
          if(intensity < 10){
            intensity++;
          }
          Serial.println("New Intensity: "+intensity);
        }
        else if(newState.equals("s2")){
          //decrement
          if(intensity > 0){
            intensity--;
            if(intensity == 0){
              currentState = "s0";
              sendToggle(currentState);
            }
          }
          Serial.println("New Intensity: "+intensity);
        }
      }
    }
    Serial.print("Disconnected from central: ");
    Serial.println( central.address() );
  }
}

bool readButton(int pin){
  int count = 0;
  int filterReadSize = 1000;
  for(int i=0; i<filterReadSize; i++){
    if(digitalRead(pin)==LOW){
      count++;
    }
    delayMicroseconds(1);
  }
  return count > filterReadSize-30;
}

void sendToggle(String value){
  char sendArray[BUFSIZE];
  value.toCharArray(sendArray,BUFSIZE);
  rxChar.writeValue(sendArray);
  Serial.println(sendArray);
}

void sendIntensity(int value){
  char sendArray[BUFSIZE];
  String s = String(value);
  String intensityAsString = "p" + s;
  intensityAsString.toCharArray(sendArray,BUFSIZE);
  Serial.println(sendArray);
  rxChar.writeValue(sendArray);
}

/**************************************************************************/
/*!
    @brief  Checks for user input (via the Serial Monitor)
            From: https://github.com/adafruit/Adafruit_BluefruitLE_nRF51
*/
/**************************************************************************/


//This could be used for button press info

bool getUserInput(char buffer[], uint8_t maxSize)
{
  // timeout in 100 milliseconds
  TimeoutTimer timeout(100);

  memset(buffer, 0, maxSize);
  while( (!Serial.available()) && !timeout.expired() ) { delay(1); }

  if ( timeout.expired() ) return false;

  delay(2);
  uint8_t count=0;
  do
  {
    count += Serial.readBytes(buffer+count, maxSize);
    delay(2);
  } while( (count < maxSize) && (Serial.available()) );
  
  return true;
}
