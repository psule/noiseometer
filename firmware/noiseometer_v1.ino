// noiseometer test code by Mike Machado <mike@makerhq.org>
//


#include <SPI.h>
#include <WiFi101.h>
#include <MQTTClient.h>


char *ssid = "wifi";
char *pass = "pass";

#define SOUND_PIN A0

#define SENSOR_INTERVAL 50   // in milliseconds
#define SAMPLE_INTERVAL 2    // in seconds
#define REPORT_INTERVAL 2   // in seconds



#define NUM_SENSOR_VALUES (1000/SENSOR_INTERVAL*SAMPLE_INTERVAL)
#define NUM_REPORT_VALUES (REPORT_INTERVAL/SAMPLE_INTERVAL)

int sensor_values[NUM_SENSOR_VALUES];
int report_values[NUM_REPORT_VALUES];

unsigned long last_report;
unsigned long last_sample;
unsigned long last_sensor;

unsigned long now;

int cur_sensor = 0;
int cur_sample = 0;
int sensor_idx = 0;
int report_idx = 0;

WiFiClient net;
MQTTClient client;

void setup() {
  Serial.begin(9600);
  WiFi.begin(ssid, pass);
  client.begin("io.adafruit.com", net);

  connect();

  now = millis();
  last_sensor = now;
  last_sample = now;
  last_report = now;
}

void connect() {
  Serial.print("checking wifi...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }

  Serial.print("\nconnecting...");
  while (!client.connect("AIO_USER", "AIO_USER", "AIO_PASS")) {
    Serial.print(".");
    delay(1000);
  }

  Serial.println("\nconnected!");

//  client.subscribe("AIO_USER/f/noiseometer");
}

void loop() {
  client.loop();

  if(!client.connected()) {
    connect();
  }

  now = millis();
  
  if (now >= last_sensor + SENSOR_INTERVAL) {
      cur_sensor = analogRead(SOUND_PIN);
      
      if (sensor_idx >= NUM_SENSOR_VALUES) {
          sensor_idx = 0;
      }
      sensor_values[sensor_idx] = cur_sensor;
      sensor_idx++;
      
      last_sensor = now;
  }
  
  if (now >= last_sample + SAMPLE_INTERVAL * 1000) {
      cur_sample = calc_sample();
      
      if (report_idx >= NUM_REPORT_VALUES) {
          report_idx = 0;
      }
      report_values[report_idx] = cur_sample;
      report_idx++;
      
      last_sample = now;
  }
  
  if (now >= last_report + REPORT_INTERVAL * 1000) {
      send_report();
      
      sensor_idx = 0;
      report_idx = 0;
      
      last_report = now;
  } 
}


int calc_sample() {
    int samples[NUM_SENSOR_VALUES];
    for (int i = 0; i < NUM_SENSOR_VALUES; i++) {
        samples[i] = sensor_values[i];
    }
    sort(samples, NUM_SENSOR_VALUES);
    return samples[(int)round(NUM_SENSOR_VALUES*0.95)];
}

void send_report() {
//  String report = String::format("%d", report_values[0]);
//  for (int i = 1; i < NUM_REPORT_VALUES; i++) {
//    report = String(report + String::format(",%d", report_values[i]));
//  }
  Serial.print("Sending ");
  Serial.print(String(report_values[0]));
  Serial.println(" to mqtt");
  client.publish("AIO_USER/f/noiseometer", String(report_values[0]));
}

void sort(int a[], int size) {
  for(int i=0; i<(size-1); i++) {
    for(int o=0; o<(size-(i+1)); o++) {
      if(a[o] > a[o+1]) {
        int t = a[o];
        a[o] = a[o+1];
        a[o+1] = t;
      }
    }
  }
}

void messageReceived(String topic, String payload, char * bytes, unsigned int length) {
  Serial.print("incoming: ");
  Serial.print(topic);
  Serial.print(" - ");
  Serial.print(payload);
  Serial.println();
}
