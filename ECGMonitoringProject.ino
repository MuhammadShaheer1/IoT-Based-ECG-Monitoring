//Including the WiFi library to enable WiFi functionality
#include <WiFi.h>
//Including PubSubClient library to send data through MQTT client
#include <PubSubClient.h>

//Defining the SSID and Password of the WiFi network for esp32 to connect
#define WIFISSID "Shaheer"
#define PASSWORD "12345678"
//Defining the Token variable and its value can be copied from ubidots dashboard
#define TOKEN "BBFF-gQKkAdc5Gnq10uYmzOqDVjcU7eFgxe"
#define MQTT_CLIENT_NAME "muhammadshaheer"

//Setting the variables names similar to those used on the ubitdots dashboard
#define VARIABLE_LABEL "sensor"
#define DEVICE_LABEL "esp32"
#define SENSOR A0

char mqttBroker[] = "industrial.api.ubidots.com";
char payload[100];
char topic [150];
float sensor;
char str_sensor[10];

//Defining the ubidots client
WiFiClient ubidots;
PubSubClient client(ubidots);

void callback(char* topic, byte* payload, unsigned int length){
  char p[length + 1];
  memcpy(p, payload, length);
  p[length] = NULL;
  Serial.write(payload, length);
  Serial.println(topic);
}

//Setting up reconnection with the ubidots server through MQTT connection
void reconnect(){
  while(!client.connected()){
    Serial.println("Attempting MQTT connection...");

    if (client.connect(MQTT_CLIENT_NAME, TOKEN, "")){
      Serial.println("Connected");
    } else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 2 seconds");

      delay(2000);
    }
  }
}
//Defining two tasks for free RTOS
TaskHandle_t setupTask;
TaskHandle_t loopTask;


void setup() {
  // initialize the serial communication:
  Serial.begin(115200);
  WiFi.begin(WIFISSID, PASSWORD);
  pinMode(4, INPUT); // Setup for leads off detection LO +
  pinMode(2, INPUT); // Setup for leads off detection LO -
  pinMode(SENSOR, INPUT);
  Serial.println();
  Serial.print("Waiting for WiFi...");
  while (WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(500);
  }
  Serial.println("");
  Serial.println("WiFi Connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  client.setServer(mqttBroker, 1883);
  client.setCallback(callback);
  //Both Tasks have the same priority
  xTaskCreatePinnedToCore(setupTaskCode, "Setup Task", 10000, NULL, 1,&setupTask,  0);
  xTaskCreatePinnedToCore(loopTaskCode, "Loop Task", 10000, NULL, 1,&loopTask, 0);
}

void setupTaskCode(void *pvParameters) {
  vTaskDelete(NULL);
}

void loopTaskCode(void *pvParameters) {
  // If the esp32 is not connected with ubidots, try to call reconnect function
  while(1) {
    if (!client.connected()){
      reconnect();
    }
    
    if((digitalRead(4) == 1)||(digitalRead(2) == 1)){
      Serial.println('!');
    }
    else{
      // send the value of analog input 0:
      sensor = analogRead(SENSOR);
      Serial.println(sensor);
    }
    //Sending the data and publishing it on the server
    sprintf(topic, "%s%s", "/v1.6/devices/", DEVICE_LABEL);
    sprintf(payload, "%s", "");
    sprintf(payload, "{\"%s\":", VARIABLE_LABEL);

    dtostrf(sensor, 4, 2, str_sensor);

    sprintf(payload, "%s {\"value\": %s}}", payload, str_sensor);
    Serial.println("Publishing data to Ubidots Cloud");
    client.publish(topic, payload);
    client.loop();
    //Creating a delay of 50ms
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

void loop(){
}
