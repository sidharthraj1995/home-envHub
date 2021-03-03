#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeSerif9pt7b.h>
#include <analogWrite.h>

// Replace the next variables with your SSID/Password combination
char ssid[] = "YOUR_WIFI_SSID";
const char password[] = "YOUR_WIFI_PASSWORD";

// Add your MQTT Broker IP address, example:
//const char* mqtt_server = "localhost";

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

// Payload
const char* oFF = "0"; //System state OFF
const char* oN = "1"; //System  state ON
const char* oFFLINE = "2"; // System state ON Net OFF
const char* oNLINE = "3"; //System state ON Net ON
const char* eRROR = "7"; //System state ERROR
const char* dISABLED = "9"; //System disabled
const char* bLACK = "#000000";

// MQTT Topics
const char* statusTopic = "home/room/status";
const char* motionTopic = "home/room/motion";
const char* glightTopic = "home/room/guideLight";
const char* rgbTopic = "home/room/rgb";
const char* bulbTopic = "home/room/bulb";
const char* tempTopic = "home/room/temperature";
const char* humTopic = "home/room/humidity";


// Current states
const char* clientName = "Room Hub"; //Set client's name
const char* motionState = oFF;
const char* glightState = oFF;
const char* rgbState = oFF;
const char* bulbState = oFF;
const char* clientState = oFF;
const char* clientnetState = oFF;
String currentTemp;
String currentHum;
String localIp;


// current temperature & humidity, updated in loop()
float t = 0.0;
float h = 0.0;


int dim = 0;

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);


void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    clientnetState = oFF;
  }
  //Serial.println("trying to connect to wifi");
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  clientnetState = oN;
  delay(1000);
  
}

void callback(char* topic, byte* message, unsigned int length) {
  Serial.println();
  Serial.print("[MESSAGE]***Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Length: ");
  Serial.print(length);
  Serial.print(". Message: ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.print("***");
  Serial.println();



  /////////Controlling Stairs Bulb//////////////////////////////////////
  if (String(topic) == bulbTopic) {
    Serial.print("[ACK]Room bulb was turned: ");
    if(messageTemp == oN){
      Serial.println("ON");
      bulbState = oN;
      delay(200);
    }
    else if(messageTemp == oFF){
      Serial.println("OFF");
      bulbState = oFF;
      delay(200);
    }
  }

  /////////Controlling Motion State//////////////////////////////////////
  if (String(topic) == motionTopic) {
    Serial.print("[ACK]Room Motion: ");
    if(messageTemp == oN){
      Serial.println("DETECTED");
      motionState = oN;
      delay(200);
    }
    else if(messageTemp == oFF){
      Serial.println("STOPPED");
      motionState = oFF;
      delay(200);
    }
  }


  /////////Controlling RGB light//////////////////////////////////////
  if (String(topic) == rgbTopic){
    
    Serial.print("[ACK]RGB received: ");
    Serial.println(messageTemp);
    if (messageTemp == bLACK){
      rgbState = oFF;
      Serial.print("[FORCE]RGB LED set to: OFF");
      delay(200);

    } else rgbState = oN;
    Serial.print("[FORCE]RGB LED set to: ");
    Serial.println(messageTemp);
    delay(200);
  }


  ////////Stairs Temperature//////////////////////////////////////////
  if (String(topic) == tempTopic) {
      Serial.print("[UPDATE]Current room temperature: ");
      Serial.println(messageTemp);
      currentTemp = messageTemp;
      delay(200);
  }


  ///////Stairs Humidity//////////////////////////////////////////
  if (String(topic) == humTopic) {
      Serial.print("[UPDATE]Current room humidity: ");
      Serial.println(messageTemp);
      currentHum = messageTemp;
      delay(200);
  }
  
}

void showconnect(){
    //Clear display
    display.clearDisplay();

    display.setTextSize(1);             
    display.setTextColor(WHITE);        
    display.setCursor(0,0);
    display.print("Name:");
    display.println(clientName);
    display.print("WiFi: ");
    if(clientnetState == oN)
    {
      display.println("Connected!");
      display.print("Net IP: ");
      display.println(WiFi.localIP()); 
      display.println("Subscribed to: HOME");
    } 
    else if(clientnetState == oFF) 
    {
      display.println("NOT CONNECTED!");
    }
        

    //Display bulb
    display.setCursor(0,35);             
    display.println("Bulb");
    display.drawCircle(10,50,5,WHITE);
    //Display Motion
    display.setCursor(28,35);            
    display.println("Motion");
    display.drawCircle(40,50,5,WHITE);
    //Display Tempperature
    display.setCursor(64,35);             
    display.print("Temp");
    display.drawRect(62,45,30,19,WHITE);
    display.setCursor(63,50);             
    display.print(currentTemp);
    //Display Humidity
    display.setCursor(96,35);             
    display.print("Humd");
    display.drawRect(94,45,30,19,WHITE);
    display.setCursor(95, 50);
    display.print(currentHum);
    if(bulbState == oN){
      display.drawCircle(10,50,5,WHITE);
      display.fillCircle(10,50,2,WHITE);
    } else if (bulbState == oFF)
    {
      display.fillCircle(10,50,2,BLACK);
    }
    if (motionState == oN )
    {
      display.fillCircle(40,50,2,WHITE);
    } else if (motionState == oFF)
    {
      display.fillCircle(40, 50, 2, BLACK);
    }
    
    
    

    display.display();
    delay(500);
      
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.println("###Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("Room32")) {
      Serial.println("+++Room Display ESP32+++ connected");
      // Subscribe
      client.subscribe("home/#");
      client.publish(statusTopic, oN);
      delay(500);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      display.clearDisplay();
      display.setTextSize(1);             
      display.setTextColor(WHITE);                    
      display.println("failed, rc=");
      display.print(client.state());
      display.display();
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  // Serial port for debugging purposes
  Serial.begin(115200);

  //Wire.begin(21, 22);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println("SSD1306 allocation failed");
    for(;;);
  }
  //delay(2000);

  clientState = oN;
  display.clearDisplay();
  display.setTextSize(2);             
  display.setTextColor(WHITE);        
  display.setCursor(5,20);             
  display.println("Room: ON");
  display.drawRect(0, 0, 126, 62, WHITE);
  display.display();
  delay(500);

  setup_wifi();
  client.setServer(mqtt_server,1024);
  client.setCallback(callback);
  showconnect();
  delay(2000);
}



void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  showconnect();  
}
