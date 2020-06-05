/**
 * Fist bump fo realz
 * 
 * Modifications cpyright (c) 2020 Michael Ang <contactmang@gmail.com>
 */

/**
 * IotWebConf06MqttApp.ino -- IotWebConf is an ESP8266/ESP32
 *   non blocking WiFi/AP web configuration library for Arduino.
 *   https://github.com/prampec/IotWebConf 
 *
 * Copyright (C) 2018 Balazs Kelemen <prampec+arduino@gmail.com>
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

/**
 * Example: MQTT Demo Application
 * Description:
 *   All IotWebConf specific aspects of this example are described in
 *   previous examples, so please get familiar with IotWebConf before
 *   starting this example. So nothing new will be explained here, 
 *   but a complete demo application will be build.
 *   It is also expected from the reader to have a basic knowledge over
 *   MQTT to understand this code.
 *   
 *   This example starts an MQTT client with the configured
 *   connection settings.
 *   Will post the status changes of the D2 pin in channel "/test/status".
 *   Receives messages appears in channel "/test/action", and writes them to serial.
 *   This example also provides the firmware update option.
 *   (See previous examples for more details!)
 * 
 * Software setup for this example:
 *   This example utilizes Joel Gaehwiler's MQTT library.
 *   https://github.com/256dpi/arduino-mqtt
 * 
 * Hardware setup for this example:
 *   - An LED is attached to LED_BUILTIN pin with setup On=LOW.
 *   - [Optional] A push button is attached to pin D2, the other leg of the
 *     button should be attached to GND.
 */

#include <MQTT.h>
#include <IotWebConf.h>

#include <ESP32Servo.h>

Servo myservo;  // create servo object to control a servo
int pos = 0;    // variable to store the servo position
// Recommended PWM GPIO pins on the ESP32 include 2,4,12-19,21-23,25-27,32-33
int servoPin = 18;

// -- Initial name of the Thing. Used e.g. as SSID of the own Access Point.
const char thingName[] = "FistBump";

// -- Initial password to connect to the Thing, when it creates an own Access Point.conne
const char wifiInitialApPassword[] = "fistbumper";

// This is the MQTT topic we listen for
const char bumpTopic[] = "/fistbump";

#define STRING_LEN 128

// -- Configuration specific key. The value should be modified if config structure was changed.
#define CONFIG_VERSION "mqt1"

// -- When CONFIG_PIN is pulled to ground on startup, the Thing will use the initial
//      password to buld an AP. (E.g. in case of lost password)
#define CONFIG_PIN 2

// -- Status indicator pin.
//      First it will light up (kept LOW), on Wifi connection it will blink,
//      when connected to the Wifi it will turn off (kept HIGH).
#define STATUS_PIN LED_BUILTIN

// -- Callback method declarations.
void wifiConnected();
void configSaved();
boolean formValidator();
void mqttMessageReceived(String &topic, String &payload);

DNSServer dnsServer;
WebServer server(80);
HTTPUpdateServer httpUpdater;
WiFiClient net;
MQTTClient mqttClient;

char mqttServerValue[STRING_LEN];
char mqttUserNameValue[STRING_LEN];
char mqttUserPasswordValue[STRING_LEN];

IotWebConf iotWebConf(thingName, &dnsServer, &server, wifiInitialApPassword, CONFIG_VERSION);
IotWebConfParameter mqttServerParam = IotWebConfParameter("MQTT server", "mqttServer", mqttServerValue, STRING_LEN);
IotWebConfParameter mqttUserNameParam = IotWebConfParameter("MQTT user", "mqttUser", mqttUserNameValue, STRING_LEN);
IotWebConfParameter mqttUserPasswordParam = IotWebConfParameter("MQTT password", "mqttPass", mqttUserPasswordValue, STRING_LEN);

boolean needMqttConnect = false;
boolean needReset = false;
int pinState = HIGH;
unsigned long lastReport = 0;
unsigned long lastMqttConnectionAttempt = 0;

void setup() 
{
  Serial.begin(115200);
  Serial.println();
  Serial.println("Fist Bump starting up...");

  Serial.println("Setting up servo");
  setupServo();

  iotWebConf.setStatusPin(STATUS_PIN);
  iotWebConf.setConfigPin(CONFIG_PIN);
  iotWebConf.addParameter(&mqttServerParam);
  iotWebConf.addParameter(&mqttUserNameParam);
  iotWebConf.addParameter(&mqttUserPasswordParam);
  iotWebConf.setConfigSavedCallback(&configSaved);
  iotWebConf.setFormValidator(&formValidator);
  iotWebConf.setWifiConnectionCallback(&wifiConnected);
  iotWebConf.setupUpdateServer(&httpUpdater);

  // -- Initializing the configuration.
  boolean validConfig = iotWebConf.init();
  if (!validConfig)
  {
    mqttServerValue[0] = '\0';
    mqttUserNameValue[0] = '\0';
    mqttUserPasswordValue[0] = '\0';
  }

  // -- Set up required URL handlers on the web server.
  server.on("/", handleRoot);
  server.on("/config", []{ iotWebConf.handleConfig(); });
  server.onNotFound([](){ iotWebConf.handleNotFound(); });

  mqttClient.begin(mqttServerValue, net);
  mqttClient.onMessage(mqttMessageReceived);
  
  Serial.println("Ready.");
}

void setupServo()
{
  myservo.setPeriodHertz(50);    // standard 50 hz servo
  myservo.attach(servoPin, 1000, 2000); // attaches the servo on pin 18 to the servo object
  // using default min/max of 1000us and 2000us
  // different servos may require different min/max settings
  // for an accurate 0 to 180 sweep
}

void loop() 
{
  // -- doLoop should be called as frequently as possible.
  iotWebConf.doLoop();
  mqttClient.loop();
  
  if (needMqttConnect)
  {
    if (connectMqtt())
    {
      needMqttConnect = false;
    }
  }
  else if ((iotWebConf.getState() == IOTWEBCONF_STATE_ONLINE) && (!mqttClient.connected()))
  {
    Serial.println("MQTT reconnect");
    connectMqtt();
  }

  if (needReset)
  {
    Serial.println("Rebooting after 1 second.");
    iotWebConf.delay(1000);
    ESP.restart();
  }

  unsigned long now = millis();
  if ((500 < now - lastReport) && (pinState != digitalRead(CONFIG_PIN)))
  {
    pinState = 1 - pinState; // invert pin state as it is changed
    lastReport = now;
    Serial.print("Sending on MQTT channel '/test/status' :");
    Serial.println(pinState == LOW ? "ON" : "OFF");
    mqttClient.publish("/test/status", pinState == LOW ? "ON" : "OFF");
  }
}

/**
 * Handle web requests to "/" path.
 */
void handleRoot()
{
  // -- Let IotWebConf test and handle captive portal requests.
  if (iotWebConf.handleCaptivePortal())
  {
    // -- Captive portal request were already served.
    return;
  }
  String s = "<!DOCTYPE html><html lang=\"en\"><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/>";
  s += "<title>IotWebConf 06 MQTT App</title></head><body>MQTT App demo";
  s += "<ul>";
  s += "<li>MQTT server: ";
  s += mqttServerValue;
  s += "</ul>";
  s += "Go to <a href='config'>configure page</a> to change values.";
  s += "</body></html>\n";

  server.send(200, "text/html", s);
}

void wifiConnected()
{
  needMqttConnect = true;
}

void configSaved()
{
  Serial.println("Configuration was updated.");
  needReset = true;
}

boolean formValidator()
{
  Serial.println("Validating form.");
  boolean valid = true;

  int l = server.arg(mqttServerParam.getId()).length();
  if (l < 3)
  {
    mqttServerParam.errorMessage = "Please provide at least 3 characters!";
    valid = false;
  }

  return valid;
}

boolean connectMqtt() {
  unsigned long now = millis();
  if (1000 > now - lastMqttConnectionAttempt)
  {
    // Do not repeat within 1 sec.
    return false;
  }
  Serial.println("Connecting to MQTT server...");
  if (!connectMqttOptions()) {
    lastMqttConnectionAttempt = now;
    return false;
  }
  Serial.println("Connected!");

  mqttClient.subscribe("/test/action");

  // Subscribe for fist bumps!
  Serial.print("Subscribing to ");
  Serial.println(bumpTopic);
  mqttClient.subscribe(bumpTopic);
  
  return true;
}

/*
// -- This is an alternative MQTT connection method.
boolean connectMqtt() {
  Serial.println("Connecting to MQTT server...");
  while (!connectMqttOptions()) {
    iotWebConf.delay(1000);
  }
  Serial.println("Connected!");

  mqttClient.subscribe("/test/action");
  return true;
}
*/

boolean connectMqttOptions()
{
  boolean result;
  if (mqttUserPasswordValue[0] != '\0')
  {
    result = mqttClient.connect(iotWebConf.getThingName(), mqttUserNameValue, mqttUserPasswordValue);
  }
  else if (mqttUserNameValue[0] != '\0')
  {
    result = mqttClient.connect(iotWebConf.getThingName(), mqttUserNameValue);
  }
  else
  {
    result = mqttClient.connect(iotWebConf.getThingName());
  }
  return result;
}

void mqttMessageReceived(String &topic, String &payload)
{
  Serial.println("Incoming: " + topic + " - " + payload);

  if (topic.equals(bumpTopic)) {
    // TODO send the device id in the message, check for it here
    Serial.println("Fistbump!");
    fistBump();
  }
}

void fistBump(void)
{
  /*
     This code moves the servo. On my servo (HXT900) this sweeps the servo through 90 degrees.
     See the original Examples -> ESP32Servo -> Sweep code for some reasons this might be.
  */

  for (pos = 0; pos <= 180; pos += 1) { // goes from 0 degrees to 180 degrees
    // in steps of 1 degree
    myservo.write(pos);    // tell servo to go to position in variable 'pos'
    delay(15);             // waits 15ms for the servo to reach the position
  }
  for (pos = 180; pos >= 0; pos -= 1) { // goes from 180 degrees to 0 degrees
    myservo.write(pos);    // tell servo to go to position in variable 'pos'
    delay(15);             // waits 15ms for the servo to reach the position
  }
}
