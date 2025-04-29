// include the libraries
#include <WiFi.h>
#include "secrets.h"
#include <Wire.h>
#include "rgb_lcd.h"
#include <Keypad.h>
#include <Adafruit_AHTX0.h>
#include "DFRobot_AHT20.h"
#include <BH1750.h>
#include <ESP32Servo.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <ArduinoJson.h>
#include "index.h"
#include "ThingSpeak.h" //thingspeak last

#define MOTOR 13

//create all objects
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
DFRobot_AHT20 aht20;
rgb_lcd lcd;
BH1750 lightMeter;
Servo myservoW, myservoD;  
WiFiClient client;

const byte interruptPin = 34;//interupt button is pin 34

int posW = 0;    // variable to store the servo position
int posD = 0;    // variable to store the servo position
int servoW = 32; //pin for window
int servoD = 25  ; //pin for door

//assign pin to Moisture Sensor and light
int moisturePin = 27;
float moistureValue = 0;
float MoistureConvertedValue = 0;
float lux = 0;

//assign trig and echo pins for  ultrasonic sensor, setting trig high creates burst, echo goes high when this burst is transmitted and is high until it gets the next one, allowing distance to be measured
const int TRIG_PIN = 14;
const int ECHO_PIN = 33;
long duration;
float cms, inches;

const int length_key = 4; //passsword length is 4
char password_key[length_key] = {'1', '9', '8', '4'}; //password array stores it
char attempt_length[length_key];//create a max attempt length based on the length of the password
volatile int correctCode = 0;
volatile int setupCheck = 0;
int z = 0;
sensors_event_t humidity, temp; //captures a specific sensor reading

int door = 0;
int window = 0;

const byte ROWS = 4; //four rows
const byte COLS = 3; //three columns

//define what is on the keypad
char KeyPad[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};

byte colPins[COLS] = {17, 5, 18}; //connect to the column pinouts
byte rowPins[ROWS] = {15, 2, 4, 16}; //connect to the row pinouts

//initialize an instance of class NewKeypad
Keypad customKeypad = Keypad( makeKeymap(KeyPad), rowPins, colPins, ROWS, COLS); 

void IRAM_ATTR reset(){
  for (posD = 90; posD >= 0; posD -= 1) { // goes from 0 degrees to 90 degrees
    myservoD.write(posD);   // tell servo to go to position in variable 'pos'
    delay(15);
  }
  correctCode = 0; //reset to default state
  setupCheck = 0;
  digitalWrite(MOTOR, LOW);//turn motor off
}

void setup(){
  Serial.begin(115200);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(MOTOR, OUTPUT);
  // set up the LCD's number of columns and rows
  lcd.begin(16, 2);
  Wire.begin();
  lightMeter.begin();
  Serial.println("Hello world");
  delay(500);//wait 500ms 

  // Allow allocation of all timers
	ESP32PWM::allocateTimer(0);
	ESP32PWM::allocateTimer(1);
	ESP32PWM::allocateTimer(2);
	ESP32PWM::allocateTimer(3);
	myservoW.setPeriodHertz(50); // standard 50 hz servo
  myservoD.setPeriodHertz(50);
	myservoW.attach(servoW, 500, 2400); // attaches the servo on pin to the servo object
  myservoD.attach(servoD, 500, 2400);

  WiFi.mode(WIFI_STA);
 // ThingSpeak.begin(client);  // Initialize ThingSpeak
  attachInterrupt(interruptPin, reset, FALLING); //triggers whenever the Pin detects a high value
  ws.onEvent(onWebSocketEvent);
  server.addHandler(&ws);
  server.begin();
  server.on("/", onrequest);

}

void onrequest(AsyncWebServerRequest *request){
  request->send(200, "text/html", homePagePart1.c_str());
}


void loop(){
  if(WiFi.status() != WL_CONNECTED){//if wifi is not connected
  Serial.print("Attempting to connect to SSID: "); //print error message
    while(WiFi.status() != WL_CONNECTED){
      WiFi.begin(ssid, pass); // Connect to network
      Serial.print(".");
      delay(5000);     
    }
  Serial.println("\nConnected."); //once connected, send user a message
  Serial.println(WiFi.localIP());
  }

  
  if (setupCheck == 0){
    lcd.print("Enter PIN: ");
    lcd.setCursor(10,0); //set the cursor to the 11th column of the first row to input the PIN
  }

  setupCheck = 1; //once setup code has run, set the variable to 1 so that it will not run again until the interrupt is pressed
  
  while (correctCode == 0){
    char customKey = customKeypad.getKey();
    if(customKey){
    switch(customKey){
      case '*':
        z=0; //if the user wants to start over it will wipe the screen
        lcd.clear();
        lcd.print("Enter Pin: ");
        lcd.setCursor(10, 0); //set cursor to correct position
        break;

      case '#':
        delay(100);
        checkKEY();//check entry if user tries to enter password early
        z=0; //set correct characters number back to 0 incase of new attempt
        break;
          
      default: //if the user doesn't enter a command, consider it part of their attempt
        lcd.print('*'); //print * to represent characters entered
        attempt_length[z]=customKey; //append each character to its spot in attemptlength based on z position, ie first number entered in pos 0, then 1, then 2 etc
        z++;//for each entry, increment z
        if (z == length_key){ //if user enters 4 digits, atuocheck password
          checkKEY();
          z = 0; //reset back to 0 so the user can re-enter the code
        }

      }
    }
  }

  door = 1;//if the door is open its value is 1   
  //Speak.setField(7, door);//set the value of door to he 7th field
  //ThingSpeak.writeField(myChannelNumber, 7, door, myWriteAPIKey); //write to Thingspeak

  if(aht20.startMeasurementReady(/* crcEn = */true)){ //if aht20 is ready
    lcd.print("Temp: ");
    float temperature = aht20.getTemperature_C(); //store temp 
    lcd.print(temperature); 
    lcd.print("C");
    delay(2000);
    lcd.clear();

    lcd.print("Hum: ");
    float humidity = aht20.getHumidity_RH();
    lcd.print(humidity);
    lcd.print("%rH");
    delay(2000);
    lcd.clear();

    if(humidity >= 60){//open the window if the humidity is too high
      for (posW = 0; posW <= 90; posW += 1) { // goes from 0 degrees to 90 degrees
        myservoW.write(posW);   // tell servo to go to position in variable 'pos'
        delay(15);// waits 15ms for the servo to reach the position before moving again
	    }
        window = 1;//if the window is open, the value is 1
      } else{
        if (window == 1){ 
          for (posW = 90; posW >= 0; posW -= 1) { // goes from 0 degrees to 90 degrees
            myservoW.write(posW);   // tell servo to go to position in variable 'pos'
            delay(15);
        }
        window = 0;//if the window is closed, the value is 0
      }
    }

  moistureValue = analogRead(moisturePin);
  lcd.print("Moisture: ");
  Serial.println(moistureValue);
  MoistureConvertedValue = (moistureValue/4095)*100;
  lcd.print((MoistureConvertedValue));//print percentage value
  lcd.print("%");
  delay(2000);
  lcd.clear();

  if(moistureValue <= 50){
    digitalWrite(MOTOR, HIGH);
    delay(5000);//set motor high for 5 seconds
    digitalWrite(MOTOR, LOW);//turn motor off
  }

  lux = lightMeter.readLightLevel();
  lcd.print("Light: ");
  lcd.print(lux);
  lcd.print("lx");
  delay(2000);
  lcd.clear();

  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(5);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(5);
  digitalWrite(TRIG_PIN, LOW);


  duration = pulseIn(ECHO_PIN, HIGH);

  cms = (duration/2) / 29.1;

  lcd.print("Dist: ");
  lcd.print(cms);
  lcd.print("cm");
  delay(2000);
  lcd.clear();
  createJSON();
  }
}

void checkKEY(){
  int correct = 0; //correct digits is initially 0 before being checked
  for (int i = 0; i<length_key; i++){
    if(attempt_length[i] == password_key[i]){ //if the element at position 0 in out attempt matches the element in position 0 of our password, it is correct
      correct++;//increment the variable correct for each correct element
    }
  }

   if(correct == 4){//if all 4 elements are correct
    lcd.setCursor(0, 1);
    lcd.print("Correct");//print message
    correctCode = 1;
    delay(2000);
    lcd.clear();
    for (posD = 0; posD <= 90; posD += 1) { //open the door
      myservoD.write(posD);   // tell servo to go to position in variable 'pos'
      delay(15);             // waits 15ms for the servo to reach the position before moving again
    }
    } else{ //if all 4 are not correct
      lcd.setCursor(0, 1);
      lcd.print("Wrong, try again");//print message
      delay(1000);
      lcd.clear();
      lcd.print("Enter PIN: "); //reprint initialisation message so the user knows to try again
      lcd.setCursor(10,0); //set the cursor to the 11th column of the first row to input the PIN
    }
}

void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len){
  switch(type){
    case WS_EVT_CONNECT:
      Serial.printf("Client %u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;

    case WS_EVT_DISCONNECT:
      Serial.printf("Client %u disconnected\n", client->id());
      break;

    case WS_EVT_DATA:
      String message = String((char*)data).substring(0,len);
      Serial.printf("Recieved from client %u: %s\n", client->id(), message.c_str());

      client->text("ESP 32 recieved: "+ message);
      break;

    //case WS_EVT_PONG:
      //Serial.printf("Pong recieved from client %u\n", client->id());
      //break;

    //case WS_EVT_ERROR:
      //Serial.printf("WebSocket error from client %u\n", client->id());
      //break;

  }
}


  void createJSON(){
    char serialized[200];
    StaticJsonDocument<300> jsonDoc;
    jsonDoc["temperature"] = aht20.getTemperature_C();
    jsonDoc["humidity"] = aht20.getHumidity_RH();
    jsonDoc["moisture"] = MoistureConvertedValue;
    jsonDoc["light"] = lightMeter.readLightLevel();
    jsonDoc["distance"] = cms;

    if(window == 0){
      jsonDoc["window"] = "CLOSED";
    } else{
      jsonDoc["window"] = "OPEN";
    }

    if(door == 0){
      jsonDoc["door"] = "CLOSED";
    } else{
      jsonDoc["door"] = "OPEN";
    }

    size_t length = serializeJson(jsonDoc, serialized);
    ws.textAll(serialized, length);

  } 





