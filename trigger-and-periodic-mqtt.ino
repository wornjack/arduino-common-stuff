#include <WiFi.h>
#include <PubSubClient.h>

#include <OneWire.h>
#include <DallasTemperature.h>


// # Settings
// ## Network
const char* ssid = "ssid";
const char* password = "password";
const char* mqtt_server = "192.168.178.33";
#define mqtt_port 1883
#define MQTT_USER ""
#define MQTT_PASSWORD ""

// ## Temperature Sensor
const int oneWireBus = 4;  //GPIO of the sensor

// ## Mail Sensor
const int mailSensor = 13;

// ## Topics
const char* topic_gate_state = "garden/gate/state";
const char* topic_temperature = "garden/mailbox/weather/temperature";
const char* topic_mail = "garden/mailbox/mail";

// ## Ultrasonic settings
int trigger = 33;
int echo = 32;
long dauer = 0;
long entfernung = 0;
long distanceDivider = 3.29;
int distancePublishDelay = 90;

// ## Device
int RestartDelay = 30000;

// END OF SETTINGS




int counter_loop = 0;
int counter_restart = 0;



//#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
//#define TIME_TO_SLEEP  3600        /* Time ESP32 will go to sleep (in seconds) */

OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);
WiFiClient wifiClient;
PubSubClient client(wifiClient);

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  wifiClient.setTimeout(100);
  WiFi.begin(ssid, password);
  int counter = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    counter = counter + 1;
    Serial.print(counter);
    if (counter > 20) {
      ESP.restart();
    }
  }
  randomSeed(micros());
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  // Loop until we're reconnected
  int counter2 = 0;
  while (!client.connected()) {
    if (counter2 > 4) {
      Serial.println("Unable to connect to MQTT, restarting");
      ESP.restart();
    }
    counter2 = counter2 + 1;
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}



void UltrasonicSend() {
  digitalWrite(trigger, LOW);
  delay(5);
  digitalWrite(trigger, HIGH);
  delay(10);
  digitalWrite(trigger, LOW);
  dauer = pulseIn(echo, HIGH);
  entfernung = (dauer / 2) / 29.1;
  entfernung = entfernung / distanceDivider;

  //Serial.print("GATE (no correction): ");
  //Serial.print(entfernung);
  //Serial.println(" % auf");

  if (entfernung > 99) {
    entfernung = 100.00;
  } else if (entfernung < 1) {
    entfernung = 0.00;
  }

  Serial.print("GATE: ");
  Serial.print((int) entfernung);
  Serial.println(" % auf");

  char distancechar[8];
  dtostrf(entfernung, 6, 2, distancechar);
  client.publish(topic_gate_state, distancechar);
}

void onTimer() {
  Serial.println("Sending Interval data");
  sensors.begin();
  sensors.requestTemperatures();
  float temperatureC = sensors.getTempCByIndex(0);
  char tempchar[8];
  dtostrf(temperatureC, 6, 2, tempchar); // Leave room for too large numbers!
  Serial.print("TEMPERATURE: ");
  Serial.println(tempchar);
  client.publish(topic_temperature, tempchar);

  UltrasonicSend();

}

void onNewMail() {
  Serial.println("New Mail!");
  client.publish(topic_mail, "ON");
  onTimer();
  delay(10000);
}

void setup() {
  pinMode(mailSensor, INPUT_PULLUP);
  pinMode(trigger, OUTPUT);
  pinMode(echo, INPUT);

  Serial.begin(115200);

  Serial.setTimeout(500);// Set time out for

  setup_wifi();

  client.setServer(mqtt_server, mqtt_port);
  reconnect();
}

void loop() {
  counter_loop += 1;
  counter_restart += 1;
  if (digitalRead(mailSensor) == HIGH) {
    onNewMail();
  }
  if (counter_loop == distancePublishDelay) {
    counter_loop = 0;
    onTimer();
  }
  if (counter_restart == RestartDelay) {
    counter_restart = 0;
    ESP.restart();
  }
  delay(10);
}
