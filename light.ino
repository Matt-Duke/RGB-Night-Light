#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_NeoPixel.h>
#include <Encoder.h>
#include <EEPROM.h>

const char* ssid = "Soft Skulled Liberals";
//const char* ssid = "La Casa Del Ellerbeck";
const char* password =  "Closer420420";
//const char* password =  "squirrel";
const char* mqtt_server = "m14.cloudmqtt.com";
const int mqtt_port = 10825;
const char* mqtt_user = "welccmff";
const char* mqtt_pass = "L0r_BlQ6adla";

byte colour[4];

WiFiClient espClient;
PubSubClient client(espClient);

Encoder enc(12, 13);
long oldPosition  = -999;
const int buttonPin = 4;
bool buttonState;
bool prevButtonState = LOW;
const int debounceDelay = 50;
unsigned long prevTime = 0;
const int holdTime = 3000;

const int neoPin = 14;
const int numpixels = 1;
const int threshold = 300; //brightness threshold
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(numpixels, neoPin, NEO_GRB + NEO_KHZ800);

void setup() {
  Serial.begin(115200);
  EEPROM.begin(512);
  pinMode(buttonPin, INPUT);
  pinMode(A0, INPUT);
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  for (int i = 0; i < 4; i++) {
    colour[i] = EEPROM.read(i);
  }
  pixels.begin();
  updateLED(colour, 0);
}
void updateLED(byte temp_colour[4], bool eeprom) {
  if (analogRead(A0) > threshold) {
    for (int i = 0; i < numpixels; i++) {
      pixels.setPixelColor(1, pixels.Color(temp_colour[0], temp_colour[1], temp_colour[2], temp_colour[3]));
    }
  }
  if (eeprom) {
    Serial.println("storing to EEPROM");
    EEPROM.write(0, colour[0]);
    EEPROM.write(1, colour[1]);
    EEPROM.write(2, colour[2]);
    EEPROM.write(3, colour[3]);
    EEPROM.commit();
  }
}
void setup_wifi() {
  delay(10);
  byte load[4] = {0, 0, 100, 10};
  updateLED(load, 0);
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
  updateLED(colour, 0);
}
void callback(char* topic, byte* payload, unsigned int length) {
  int i;
  if (topic == "red") {
    i = 0;
  }
  else if (topic == "green") {
    i = 1;
  }
  else if (topic == "blue") {
    i = 2;
  }
  else if (topic == "white") {
    i = 3;
  }
  colour[i] = 0;
  for (int j = 0; j < length; j++) {
    colour[i] += ((payload[j] - 48) * (pow(10, (length - j - 1))));
  }
  updateLED(colour, 1);
}
void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("Matt", mqtt_user, mqtt_pass)) {
      Serial.println("connected");
      client.subscribe("red");
      client.subscribe("green");
      client.subscribe("blue");
      client.subscribe("white");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
void changeColour() {
  byte temp_colour[4]= {0,0,0,0};
  delay(100); //avoid bounce issues
  prevButtonState = buttonState;
  Serial.println("Change Colour - Begin");
  int button_count = 0;
  long currTime = 0;
  long prevTime = 0;
  int waitTime = 5000;
  while (1) {
    long newPosition = enc.read();
    if (newPosition != oldPosition) {
      temp_colour[button_count] += newPosition - oldPosition;
      Serial.println(button_count);
      Serial.println(colour[button_count]);
      oldPosition = newPosition;
      currTime=millis();
      updateLED(temp_colour, 0);
    }
    buttonState = digitalRead(buttonPin);
    if (buttonState != prevButtonState) {
      delay(debounceDelay);
      if (buttonState == LOW) {
        Serial.println("Click");
        currTime = millis();
        button_count++;
        if (button_count > 4)
        {
          buttonState = 0;
        }
        delay(500);
      }
      prevButtonState = buttonState;
    }
    if ((currTime - prevTime) > waitTime) {
      if (button_count == 4) {
        Serial.println("Send update");
        sendUpdate();
        break;
      }
      else { //timeout
        Serial.println("Timeout");
        updateLED(colour,0);
        break;
      }
    }
    delay(30);
  }
}
void sendUpdate() {
  int intensity = 100;
  byte red[4] = {intensity, 0, 0, 0};
  updateLED(red,0);
  client.publish("red", (char*)(colour[0]));
  delay(200);

  byte green[4] = {0, intensity, 0, 0};
  updateLED(green,0);
  client.publish("green", (char*)(colour[1]));
  delay(200);

  byte blue[4] = {0, 0, intensity, 0};
  updateLED(blue,0);
  client.publish("blue", (char*)(colour[2]));
  delay(200);

  byte white[4] = {0, 0, 0, intensity};
  updateLED(white,0);
  client.publish("white", (char*)(colour[3]));
  delay(200);
  byte success[4] = {50, 100, 0, 50};
  updateLED(success,0);
  delay(1000);
  updateLED(colour,1);
}
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  buttonState = digitalRead(buttonPin);
  if (buttonState != prevButtonState) {
    delay(debounceDelay);
    if (buttonState == LOW) {
      Serial.println("Click");
      if (abs(millis() - prevTime) > holdTime) {
        sendUpdate();
      }
      else {
        changeColour();
      }
    }
    else {
      prevTime = millis();
    }
    prevButtonState = buttonState;
  }
}
