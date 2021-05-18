  
/*
 * Description: Distance measurement using ultrasonic HC-SR04 and ESP8266
 *              & publish results to MQTT
 * 
 * 18-05-2021 - replaced hc-sr04 with sn-sr04t-v3.0 sensor
 * 02-05-2021 - Added SimpleKalmanFilter
 *            - added sensor_cm_above_top to factor in the CM measured we need to remove
 *              in order to get an acurate reading
 * 21-04-2021 - Added 3x sample averaging
 * 20-04-2021 - Initial
 */

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <SimpleKalmanFilter.h>

// utlrasonic pinout
#define ULTRASONIC_TRIG_PIN     5   // pin TRIG to D1
#define ULTRASONIC_ECHO_PIN     4   // pin ECHO to D2

// Tank specific: mandatory
int tank_height = 200;         // maximum height of tank in cm
int tank_liters = 5000;        // maximum liters can can hold
int sensor_cm_above_top = 20;  // CM that the sensor is above the tank top 
int radius = 75;               // radius of tank in cm
int liters_per_cm = 25;        // for the roto 5kl it is 5000(max liters)/200(height)=25L
int liters_per_wash = 80;      // used to calculated washing maching cycles remaining from tank

/* Anything below here is dynamically  generated based on the above mandatory values
   and sensor readings, contime wih wifi details next
*/

float volume;                  // volume in cubic cm
float distance;                // height of water level in cm
int cap;                       // capacity in liters
int actual_height;             // actual height in cm
int cycles_remaining;          // used to calculate how many washing machine wash cycles the tank can service
int percentage; 		       // % of of tank_liters
int counter;                   // used for calculating average readings

// wifi config
const char* wifi_ssid = "SSID";          // SSID
const char* wifi_password = "Password";  // WIFI_Pass

// Write inverval
const unsigned int writeInterval = 2000; // write interval (in ms)

// Delay inverval
const unsigned int pauseInterval = 100; // loop pause (in ms)

// Instance Wifi client
WiFiClient wclient;

// Instance mqtt server
IPAddress server(10, 247, 29, 35);
PubSubClient client(wclient, server);

/*
 SimpleKalmanFilter(e_mea, e_est, q);
 e_mea: Measurement Uncertainty 
 e_est: Estimation Uncertainty 
 q: Process Noise
 Source: https://github.com/denyssene/SimpleKalmanFilter
 */
SimpleKalmanFilter simpleKalmanFilter(2, 2, 0.01);


void callback(const MQTT::Publish& pub) {
  // handle message arrived, future upgrade, not implemented
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
	long duration, distance, true_distance, averagedistance = 0;

    // wait for WiFi connection
	if (WiFi.status() == WL_CONNECTED){


		// collect 10 samples and calculate averge
		for(counter = 0;counter <= 9;counter++) {

			// The sensor is triggered by a HIGH pulse of 10 or more microseconds.
			// Give a short LOW pulse beforehand to ensure a clean HIGH pulse:
			digitalWrite(ULTRASONIC_TRIG_PIN, LOW);  
			delayMicroseconds(5); 
			
			digitalWrite(ULTRASONIC_TRIG_PIN, HIGH);
			delayMicroseconds(10); 
			
			digitalWrite(ULTRASONIC_TRIG_PIN, LOW);

			// Read the signal from the sensor: a HIGH pulse whose
			// duration is the time (in microseconds) from the sending
			// of the ping to the reception of its echo off of an object.
			duration = pulseIn(ULTRASONIC_ECHO_PIN, HIGH);

			// the following is for the sn-sr04t-v3.0
			distance = duration*0.034/2;

			// Trim the CM the sensor is above the top of the tank
			true_distance = (distance-sensor_cm_above_top);

			// Add to average
			averagedistance = (averagedistance+true_distance);

			// pause for a little bit
			delay(pauseInterval);
		}

		Serial.print("averagedistance: ");
		Serial.println(averagedistance);

		// Calculate the average from the *10 sample set
		distance = averagedistance/10;

		// Filter estimate distance with kalman (noise filter)
		float estimated_distance = simpleKalmanFilter.updateEstimate(distance);

		// Set a distance string value that we can post to mqtt
		String mqtt_dist = String(estimated_distance);

		// echo sensor data to console
		Serial.print("Ultrasonic Distance: ");
		Serial.print(estimated_distance);
		Serial.println(" cm");
			
		// calculate the capacity in liters
		// volume calculation were off - moved to liters per cm instead
		actual_height = (tank_height-estimated_distance);

		// volume = ((3.14*(radius*radius))*(actual_height)); // formula to calculate volume in cubic cm
		//cap = volume/1000; // final capacity in liters
		cap = (actual_height*liters_per_cm);
		String mqtt_cap = String(cap); // using a long

		// calculate the washing machine cycles remaining
		cycles_remaining = (cap/liters_per_wash);
		String mqtt_cycles_remaining = String(cycles_remaining); // using a long

		// calculate % full
		percentage = (cap/tank_liters)*100;
		String mqtt_percentage = String(percentage); // using a long
		
		// echo out calculated values
		Serial.print("Capacity: ");
		Serial.print(cap);
		Serial.println(" liters in tank");
		// Serial.print("Pct free: ");
		// Serial.println(percentage);
		// Serial.print("Wash Cyckes remaining: ");
		// Serial.println(cycles_remaining);

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
