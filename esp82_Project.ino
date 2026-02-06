#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// Update these with values suitable for your network.

const char* ssid = "Galaxy_S25";
const char* password = "nongpee123";
const char* mqtt_server = "mqtt.netpie.io";
const int mqtt_port = 1883;
const char* mqtt_client = "b0965a6c-79d7-4009-bdbd-b03e9f868adf";
const char* mqtt_username = "1wYYLSfZVHmn6AyWodsZrb37i5kD1YhJ";
const char* mqtt_password = "QAq15KJjWdjJ2Wq8MUACAVhVpqVwxGJC";

#define TRIG_PIN D5
#define ECHO_PIN D6
#define BUZZER_PIN D2
#define SW_0 D1
#define SW_1 D4
#define LED_0 D7
#define VOL A0

long duration;
float distance;
int Maxres = 0;
int Value_Vol = 0;

bool Value_SW_0 = HIGH;
bool Value_SW_1 = HIGH;
bool isLocked = false;

WiFiClient espClient;
PubSubClient client(espClient);

unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE	(50)
char msg[MSG_BUFFER_SIZE];
int value = 0;

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Request [");
  Serial.print(topic);
  Serial.print("] ");
  String msg = "";
  for (int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }
  Serial.println(msg);

  if(String(topic)== "@msg/SW_ONOFF"){
    if(msg == "LOCK" && !isLocked){
      delay(50);
      isLocked = true;
      client.publish("@shadow/data/update", "{\"data\":{\"Switch\":\"locked\"}}");
      Serial.println("Locked");
      client.publish("@msg/SW_ONOFF","Door Locked");
      client.publish("@shadow/data/update", "{\"data\":{\"doorstatus\":\"locked\"}}");
      tone(BUZZER_PIN, 1000, 500);
      delay(500);
      digitalWrite(BUZZER_PIN, HIGH);
    }

    if(msg == "UNLOCK" && isLocked){
      delay(50);
      isLocked = false;
      client.publish("@shadow/data/update", "{\"data\":{\"Switch\":\"unlocked\"}}");
      digitalWrite(LED_0, HIGH);
      delay(1000);
      digitalWrite(LED_0, LOW);
      delay(200);
      digitalWrite(LED_0, HIGH);
      delay(200);
      digitalWrite(LED_0, LOW);
      delay(200);
      Serial.println("Unlocked");
      client.publish("@msg/SW_ONOFF","Door Unlocked");
      client.publish("@shadow/data/update", "{\"data\":{\"doorstatus\":\"unlocked\"}}");
      tone(BUZZER_PIN, 1000, 500);
      delay(100);
      tone(BUZZER_PIN, 2000, 500);
      delay(100);
      digitalWrite(BUZZER_PIN, HIGH);
    }
    
  }
}

void reconnect() {
  // Loop until we're reconnected
  if (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(mqtt_client, mqtt_username, mqtt_password)) {
      Serial.println("Connected");
      client.publish("@shadow/data/update", "{\"data\":{\"status\":\"connected\"}}");
      client.subscribe("@msg/SW_ONOFF");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
    
    }
  }
}

void setup() {
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(SW_0, INPUT_PULLUP);
  pinMode(SW_1, INPUT_PULLUP);
  pinMode(LED_0,OUTPUT);
  digitalWrite(LED_0, LOW);
  digitalWrite(BUZZER_PIN, HIGH);

}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  Value_SW_0 = digitalRead(SW_0);
  Value_SW_1 = digitalRead(SW_1);
  Value_Vol = analogRead(VOL);
  Value_Vol = constrain(Value_Vol,0,1023);
  Serial.println(Value_Vol);

  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  duration = pulseIn(ECHO_PIN, HIGH, 30000);

  if (duration == 0) {
    Serial.println("No echo (out of range)");
    digitalWrite(BUZZER_PIN, HIGH);
  } else {
    distance = (duration * 0.0343) / 2;
    Serial.print("Distance: ");
    Serial.print(distance);
    Serial.println(" cm");
  }
  delay(1000);

  if (Value_Vol < 350){
    Maxres = 50;
    client.publish("@shadow/data/update", "{\"data\":{\"range\":\"short\"}}");
  } else if(Value_Vol >= 350 && Value_Vol <= 700){
    Maxres = 100;
    client.publish("@shadow/data/update", "{\"data\":{\"range\":\"mid\"}}");
  } else if ( Value_Vol > 700){
    Maxres = 150;
    client.publish("@shadow/data/update", "{\"data\":{\"range\":\"long\"}}");
  }
  
  if (!isLocked && duration != 0 && distance > 0 && distance < Maxres) {
    digitalWrite(LED_0, HIGH);
    Serial.println(Maxres);
    Serial.println("Opened");
    client.publish("@msg/SW_ONOFF","Door Opened");
    client.publish("@shadow/data/update", "{\"data\":{\"door\":\"opened\"}}");
    tone(BUZZER_PIN, 4000); // เสียงสูงเวลากด
    delay(400);
    tone(BUZZER_PIN, 800);
    delay(800);
    digitalWrite(BUZZER_PIN, HIGH);
    delay(500);
    digitalWrite(LED_0, LOW);
    Serial.println("Closed");
    client.publish("@msg/SW_ONOFF","Door Closed");
    client.publish("@shadow/data/update", "{\"data\":{\"door\":\"closed\"}}");
    

  } if (Value_SW_1 == LOW &&Value_SW_0 == HIGH && !isLocked) {
    delay(50);
    isLocked = true;
    client.publish("@shadow/data/update", "{\"data\":{\"switch\":\"locked\"}}");
    Serial.println("Locked");
    client.publish("@msg/SW_ONOFF","Door Locked");
    client.publish("@shadow/data/update", "{\"data\":{\"doorstatus\":\"locked\"}}");
    tone(BUZZER_PIN, 1000, 500);
    delay(100);
    digitalWrite(BUZZER_PIN, HIGH);
    
  } if (Value_SW_0 == LOW && Value_SW_1 == HIGH && isLocked) {
    delay(50);
    isLocked = false;
    client.publish("@shadow/data/update", "{\"data\":{\"switch\":\"unlocked\"}}");
    digitalWrite(LED_0, HIGH);
    delay(1000);
    digitalWrite(LED_0, LOW);
    delay(200);
    digitalWrite(LED_0, HIGH);
    delay(200);
    digitalWrite(LED_0, LOW);
    delay(200);
    Serial.println("Unlocked");
    client.publish("@msg/SW_ONOFF","Door Unlocked");
    client.publish("@shadow/data/update", "{\"data\":{\"doorstatus\":\"unlocked\"}}");
    tone(BUZZER_PIN, 1000, 500);
    delay(100);
    tone(BUZZER_PIN, 2000, 500);
    delay(100);
    digitalWrite(BUZZER_PIN, HIGH);
  }

  if (isLocked){
      digitalWrite(LED_0, HIGH);
      delay(200);
      digitalWrite(LED_0, LOW);
      delay(200);
    }

}