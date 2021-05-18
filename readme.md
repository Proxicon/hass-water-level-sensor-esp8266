## Features

ESP8266 + (HC-SR04 or SN-SR04T-v3.0) collector & publish to MQTT for use in Home Assistant or Node-Red
* 10x samples read with 100ms delay between samples
* Publish sensor data to MQTT
* Apply Kalman filter to the average of the samples
* .ino file in the repo for the sn-sr04t-v3.0 waterproof sensor
* .ino file in the repo for the hc-sr04 sensor

## ESP8266 & HC-SR04 Install & setup

Follow the [Hacker.io Sample](https://www.hackster.io/AskSensors/hc-sr04-ultrasonic-distance-with-esp8266-asksensors-iot-e4ded9) used as the source for this project

## Home assitant vertical stacck in card

Check the [hass folder](https://github.com/Proxicon/hass-water-level-sensor-esp8266/commit/37599a6f347fe5dbfab31412b0ea3128ce8a007f) for the configuration.yaml used and the vertical-stackin-card.yaml for presenting the data in the UI

### Includes use of SimpleKalman filter library
![KalmanFilter](https://github.com/denyssene/SimpleKalmanFilter/blob/master/images/kalman_filter_example_1.png)

* Take a look at this [youtube video](https://www.youtube.com/watch?v=4Q5kJ96YYZ4) to see the Kalman Filter working on a stream of values!