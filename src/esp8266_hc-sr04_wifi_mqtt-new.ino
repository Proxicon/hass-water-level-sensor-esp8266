  
/*
 * Description: Distance measurement using ultrasonic HC-SR04 and ESP8266
 *              & publish results to MQTT
 * 21/04-2021 - Added 3x sample averaging
 * 20-04-2021 - Initial
 */

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// utlrasonic pinout
#define ULTRASONIC_TRIG_PIN     5   // pin TRIG to D1
#define ULTRASONIC_ECHO_PIN     4   // pin ECHO to D2

// tank specific: TODO - add water level %
int tank_height = 200;    // maximum height of tank in cm
int tank_liters = 5000;   // maximum height of tank in cm
float volume;             // volume in cubic cm
float distance;           // height of water level in cm
int radius = 75;          // radius of tank in cm
int cap;                  // capacity in liters
int actual_height;        // actual height in cm
int liters_per_cm = 25;   // for the roto 5kl it is 5000(max liters)/200(height)=25L
int liters_per_wash = 80; // used to calculated washing maching cycles remaining from tank
int cycles_remaining;     // used to calculate how many washing machine wash cycles the tank can service
int percentage; 		  // % of of tank_liters
int counter;              // used for calculating average readings

// wifi config
const char* wifi_ssid = "SSID";          // SSID
const char* wifi_password = "Password";  // WIFI_Pass
const unsigned int writeInterval = 2500; // write interval (in ms)

// Instance Wifi client
WiFiClient wclient;

// Instance mqtt server
IPAddress server(10, 247, 29, 35);
PubSubClient client(wclient, server);

void callback(const MQTT::Publish& pub) {
  // handle message arrived
}

void setup() {

  Serial.begin(9600);
  Serial.println("Connect Ultrasonic HC-SR04 + ESP8266 & publish to MQTT");
  Serial.println("Wait for WiFi... ");
  Serial.print("connecting to WIFI : ");
  Serial.println(wifi_ssid);
  WiFi.begin(wifi_ssid, wifi_password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("-> WiFi connected");
  Serial.println("-> IP address: ");
  Serial.println(WiFi.localIP());

  // ultrasonic setup 
  pinMode(ULTRASONIC_TRIG_PIN, OUTPUT);
  pinMode(ULTRASONIC_ECHO_PIN, INPUT);

  client.set_callback(callback);
  
}

void loop() {

	// set local variables
	long duration, distance, averagedistance = 0;

    // wait for WiFi connection
	if (WiFi.status() == WL_CONNECTED){

		// collect 3 samples and calculate averge
		for(counter = 0;counter <= 2;counter++) {
			digitalWrite(ULTRASONIC_TRIG_PIN, LOW);  
			delayMicroseconds(2); 
			
			digitalWrite(ULTRASONIC_TRIG_PIN, HIGH);
			delayMicroseconds(10); 
			
			digitalWrite(ULTRASONIC_TRIG_PIN, LOW);
			duration = pulseIn(ULTRASONIC_ECHO_PIN, HIGH);
			distance = (duration/2) / 29.1;

			averagedistance = (averagedistance+distance);
		}

		// calulate average
		distance = averagedistance/3;
		String mqtt_dist = String(distance);

		// echo sensor data to console
		Serial.print("Ultrasonic Distance: ");
		Serial.print(distance);
		Serial.println(" cm");
			
		// calculate the capacity in liters
		// volume calculation were off - moved to liters per cm instead
		actual_height = tank_height - distance;
		// volume = ((3.14*(radius*radius))*(actual_height)); // formula to calculate volume in cubic cm
		//cap = volume/1000; // final capacity in liters
		cap = (actual_height*liters_per_cm);
		String mqtt_cap = String(cap); // using a long

		// calculate the washing machine cycles remaining
		cycles_remaining = (cap/liters_per_wash);
		String mqtt_cycles_remaining = String(cycles_remaining); // using a long

		// calculate % full
		percentage = (cap/tank_liters*100);
		String mqtt_percentage = String(percentage); // using a long
		
		// echo out calculated values
		Serial.print("Capacity: ");
		Serial.print(cap);
		Serial.println(" liters in tank");
		Serial.print("Pct free: ");
		Serial.println(percentage);
		Serial.print("Wash Cyckes remaining: ");
		Serial.println(cycles_remaining);

		// publish mqtt topics
		if (!client.connected()) {
			if (client.connect("esp8266Client")) {
				client.publish("rototank/sensor/hcrs04",mqtt_dist);
				client.publish("rototank/sensor/liters",mqtt_cap);
				client.publish("rototank/sensor/PctFree",mqtt_percentage);
				client.publish("rototank/sensor/WashCyclesRemaining",mqtt_cycles_remaining);
				// client.subscribe("inTopic"); // Future upgrade
			}
		}else{
			if (client.connect("esp8266Client")) {
				client.publish("rototank/sensor/hcrs04",mqtt_dist);
				client.publish("rototank/sensor/liters",mqtt_cap);
				client.publish("rototank/sensor/PctFree",mqtt_percentage);
				client.publish("rototank/sensor/WashCyclesRemaining",mqtt_cycles_remaining);
				// client.subscribe("inTopic"); // Future upgrade
			}     
		}
    }

    delay(writeInterval);
}
