#include <PubSubClient.h>
#include <WiFi.h>
#include <DHT.h>

#define TOTAL_MODE 3

const char* ssid = "Wokwi-GUEST";
const char* password = "";
WiFiClient espClient;

const char* mqttServer = "mqtt.netpie.io";
const int mqttPort = 1883;
const char* clientID = "93c35b34-a81f-460a-9f84-012b4d267221";
const char* mqttUser = "99wVJvnLUaYN6RD8goHSwrZnCY3okhsF";
const char* mqttPassword = "3bjr8T86aZSfCifqrnu2uP8EMbSF3k8Y";
const char* topic_pub = "@shadow/data/update";
const char* topic_sub = "@msg/lab_ict_kps/command";

PubSubClient client(espClient);

const int PUSHBUTTON_PIN = 25;
const int LED_MODE_1 = 12;
const int LED_MODE_2 = 14;
const int LED_MODE_3 = 27;
const int LED_PIN = 22;
const int DHT_PIN = 15;
const int PIR_PIN = 13;
const int THRESHOLD_MODE[3] = {10, 25, 40};
int pushcount, env_mode;
float temperature, humidity;
bool pirState = false;

DHT dht(DHT_PIN, DHT22);

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnectMQTT() {
  char mqttinfo[80];
  snprintf(mqttinfo, 75, "Attempting MQTT connection at %s:%d (%s/%s)...", mqttServer, mqttPort, mqttUser, mqttPassword);
  while (!client.connected()) {
    Serial.println(mqttinfo);
    String clientId = clientID;
    if (client.connect(clientId.c_str(), mqttUser, mqttPassword)) {
      Serial.println("...Connected");
      client.subscribe(topic_sub);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void messageReceivedCallback(char* topic, byte* payload, unsigned int length) {
  char payloadMsg[80];

  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
  Serial.print("Message:");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    payloadMsg[i] = (char)payload[i];
  }
  payloadMsg[length] = '\0';
  Serial.println();
  Serial.println("-----------------------");
  processMessage(payloadMsg);
}

void processMessage(String recvCommand) {
  // Add any specific processing for received messages if needed
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqttServer, mqttPort);
  client.setCallback(messageReceivedCallback);
  pinMode(PUSHBUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_MODE_1, OUTPUT);
  pinMode(LED_MODE_2, OUTPUT);
  pinMode(LED_MODE_3, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(PIR_PIN, INPUT);

  dht.begin();

  env_mode = 0;
  pushcount = 0;
}

void loop() {
  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();

  if (digitalRead(PUSHBUTTON_PIN) == 0) {
    pushcount++;
    Serial.println(env_mode);
  }

  env_mode = pushcount % TOTAL_MODE;
  switch (env_mode) {
    case 0:
      digitalWrite(LED_MODE_3, HIGH);
      digitalWrite(LED_MODE_2, LOW);
      digitalWrite(LED_MODE_1, LOW);
      break;
    case 1:
      digitalWrite(LED_MODE_3, LOW);
      digitalWrite(LED_MODE_2, LOW);
      digitalWrite(LED_MODE_1, HIGH);
      break;
    case 2:
      digitalWrite(LED_MODE_3, LOW);
      digitalWrite(LED_MODE_2, HIGH);
      digitalWrite(LED_MODE_1, LOW);
      break;
  }

  // Read DHT22 sensor values
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();

  // Read PIR sensor state
  pirState = digitalRead(PIR_PIN);

  Serial.println("Temperature: " + String(temperature) + " °C");
  Serial.println("Humidity: " + String(humidity) + " %");
  Serial.println("PIR State: " + String(pirState));

  // Check temperature threshold and turn on LED_PIN if below the threshold
  if (temperature < THRESHOLD_MODE[env_mode] || pirState) {
    digitalWrite(LED_PIN, HIGH);
  } else {
    digitalWrite(LED_PIN, LOW);
  }

  String publishMessage = "{\"data\": {\"env_mode\": " + String(env_mode) + ",\"temperature\": " + String(temperature) + ",\"humidity\": " + String(humidity) + ",\"pirState\": " + String(pirState) + "}}";

  Serial.println(publishMessage);
  client.publish(topic_pub, publishMessage.c_str());

  delay(1000); // this speeds up the simulation
}
