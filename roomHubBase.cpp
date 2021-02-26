/*ESP8266 based monitoring system
Board: ESP8266 12E
Sensors: DHT11
         SR505
         HW-478
         RGB control
Monitors Temp, Humidity, detects Motion triggers light and RGB control
*/
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <DHT.h>
#include <DHT_U.h>
#include <Adafruit_SPIDevice.h>
#include <time.h>
#include <NTPClient.h>


// Replace the next variables with your SSID/Password combination
char* ssid = "YOUR_SSID_HERE";
const char* password = "YOUR_WIFI_PASSWORD";

// Add your MQTT Broker IP address, example:
const char* mqtt_server = "MQTT_SERVER_IP";
//const char* mqtt_server = "localhost";

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
long localadd;
char msg[50];
int value = 0;

//Setup Time object
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

//Week Days
String weekDays[7]={"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

//Month names
String months[12]={"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};


#define DHTPIN 0     // Digital pin connected to the DHT sensor

// Uncomment the type of sensor in use:
#define DHTTYPE    DHT11     // DHT 11
//#define DHTTYPE    DHT22     // DHT 22 (AM2302)
//#define DHTTYPE    DHT21     // DHT 21 (AM2301)

#define timeSeconds 10

// Set GPIOs for LED and PIR Motion Sensor
const int guideLights = 3;    //
const int motionSensor = 2;  //D4
const int bulb = 16;         //D0

// Set GPIOs for RGB
const int redPin = 14;
const int greenPin = 12;
const int bluePin = 13;

// Timer: Auxiliary variables
unsigned long now = millis();
unsigned long lastTrigger = 0;
boolean startTimer = false;

//Topic messages
char* motionOff = "0";
char* glightOff = "0";
char* motionOn = "1";
char* glightOn = "1";
char* rgbOff = "0";
char* rgbOn = "1";


//Current states
char* clientState = "0";
char* motionState = "0";
char* glightState = "0";
char* bulbState = "0";
char* rgbState = "0";


// current temperature & humidity, updated in loop()
float t = 0.0;
float h = 0.0;
unsigned long DHTpreviousMillis = 0;    // will store last time DHT was updated
// Updates DHT readings every 10 seconds
const long DHTinterval = 10000; 

DHT dht(DHTPIN, DHTTYPE);


// Checks if motion was detected, sets LED HIGH and starts a timer
ICACHE_RAM_ATTR void detectsMovement() {
  startTimer = true;
  lastTrigger = millis();
  motionState = "1";
  Serial.print("[TRIGGER]MOTION DETECTED at:");
  Serial.println(millis());
  client.publish("home/stairs/motion", motionOn);
  digitalWrite(guideLights, HIGH);
  glightState = "1";
  Serial.print("[TRIGGER]Turning guide lights: ON");
  client.publish("home/stairs/guideLight", glightOn);
}

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
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  
}

void RGB_color(int red_light_value, int green_light_value, int blue_light_value)
{
  analogWrite(redPin, red_light_value);
  analogWrite(greenPin, green_light_value);
  analogWrite(bluePin, blue_light_value);
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
  if (String(topic) == "home/stairs/bulb") {
    Serial.println("[ACK]Turning Stair bulb: ");
    if(messageTemp == "1"){
      Serial.println("ON");
      bulbState = "1";
      digitalWrite(bulb, HIGH);
    }
    else if(messageTemp == "0"){
      Serial.println("OFF");
      bulbState = "0";
      digitalWrite(bulb, LOW);
    }
  }


  
/////////Controlling RGB light//////////////////////////////////////
  if (String(topic) == "home/stairs/rgbColor"){
    
    Serial.print("[ACK]RGB received: ");
    Serial.println(messageTemp);

    long number = (long) strtol( &messageTemp[0], NULL, 16);
    //Split them up into r, g, b values
    long r = ((number >> 16) & 0xFF);
    long g = ((number >> 8) & 0xFF);
    long b = (number & 0xFF);

    rgbState = "1";
    RGB_color(r, g, b);
    Serial.println("[FORCE]RGB LED set to: ");
    Serial.print(r);
    Serial.print(", ");
    Serial.print(g);
    Serial.print(", ");
    Serial.print(b);
    
  }
}



void getTime(){
  //client.loop();
  timeClient.update();

  //Get time in time in HH:MM:SS
  String formattedTime = timeClient.getFormattedTime();
  Serial.print("Formatted Time: ");
  Serial.println(formattedTime);
  //Get date
  String weekDay = weekDays[timeClient.getDay()];
  Serial.print("Week Day: ");
  Serial.println(weekDay); 
  delay(1000);
}



void setup() {
  // Serial port for debugging purposes
  Serial.begin(115200);
  
  // PIR Motion Sensor mode INPUT_PULLUP
  pinMode(motionSensor, INPUT_PULLUP);
  // Set motionSensor pin as interrupt, assign interrupt function and set RISING mode
  attachInterrupt(digitalPinToInterrupt(motionSensor), detectsMovement, RISING);

  // Configure IOs
  pinMode(guideLights, OUTPUT);
  pinMode(bulb, OUTPUT);
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);

  //Set States to LOW
  digitalWrite(guideLights, LOW);
  digitalWrite(bulb, LOW);
  RGB_color(0, 0, 0);

  setup_wifi();
  client.setServer(mqtt_server, 1024);
  client.setCallback(callback);

  timeClient.begin();
  timeClient.setTimeOffset(-18000);  //-5 EST



}



void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.println("###Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("test_ELSA8266")) {
      Serial.println("+++ELSA8266+++ connected");
      // Subscribe
      client.subscribe("home/#");
      clientState = "1";
      client.publish("home/stairs/status", clientState);
      //client.publish("home/stairs/ip", localadd);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      clientState = "0";
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  getTime();

  // Current time
  now = millis();
  // Turn off the LED after the number of seconds defined in the timeSeconds variable
  if(startTimer && (now - lastTrigger > (timeSeconds*1000))) {
    Serial.println("[FORCE]Motion stopped...");
    motionState = "0";
    client.publish("home/stairs/motion", motionOff);
    digitalWrite(guideLights, LOW);
    Serial.println("[FORCE]Turning guide lights off.");
    glightState = "0";
    client.publish("home/stairs/guideLight", glightOff);
    startTimer = false;
  }


  unsigned long currentMillis = millis();
  if (currentMillis - DHTpreviousMillis >= DHTinterval) {
    // save the last time you updated the DHT values
    DHTpreviousMillis = currentMillis;
    // Read temperature as Celsius (the default)
    float newT = dht.readTemperature();
    // Read temperature as Fahrenheit (isFahrenheit = true)
    //float newT = dht.readTemperature(true);
    // if temperature read failed, don't change t value
    if (isnan(newT)) {
      Serial.println("Failed to read from DHT sensor!");
    }
    else {
      t = newT;
      //Serial.println();
      Serial.print("[UPDATE]Current temperature: ");
      Serial.println(t);
      // Convert the value to a char array
      char tempTemp[8];
      dtostrf(t, 1, 2, tempTemp);
      client.publish("home/stairs/temperature", tempTemp);
      //client.publish();
    }
    // Read Humidity
    float newH = dht.readHumidity();
    // if humidity read failed, don't change h value 
    if (isnan(newH)) {
      Serial.println("Failed to read from DHT sensor!");
    }
    else {
      h = newH;
      Serial.print("[UPDATE]Current humidity: ");
      Serial.println(h);
      char tempHum[8];
      dtostrf(h, 1, 2, tempHum);
      client.publish("home/stairs/humidity", tempHum);
    }
  }
 
}
