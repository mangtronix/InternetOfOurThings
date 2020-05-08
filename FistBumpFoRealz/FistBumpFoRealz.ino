/*
   Simple web server that moves a servo in response to loading the page.
   Connect the servo to a papercut emoji fist to make a fist bump forealz!

   Demo for "The Internet of Our Things?" sample class
   https://www.michaelang.com/the-internet-of-our-things

   Mashup by Michael Ang / Mangtronix
   https://michaelang.com

   Instructions:
     - set the ssid and password of your wifi network
     - open the Serial Monitor (Tools -> Serial Monitor) and set 115200 baud
     - compile and upload code (Sketch -> Upload)
     - see that the Arduino connects to wifi
     - copy URL of the web server into your web brower (e.g. http://192.168.178.56:8080)
     - you should see "Fist bump sent! 1" and the servo moves

   To make the Arduino accessible from the Internet:
     - Sign up for ngrok - https://ngrok.com/
     - Download ngrok client
     - Add your ngrok authentication token - https://dashboard.ngrok.com/get-started/setup
     - Start the ngrok tunnel to the Arduino using the local IP address / port the Arduino is listening on.
       The local IP address and port is output on the Serial Monitor, and it's also in the URL (http://ipaddress:port)

       Example:
       $ ./ngrok tcp 192.168.178.56:8080

     - You should see "Session Status                online"
     - Copy the forwarding IP address and port into your web browser, with "http" instead of "tcp"

       Example:
         Forwarding                    tcp://0.tcp.ngrok.io:17526 -> 192.168.178.56:8080
         becomes http://0.tcp.ngrok.io:17526

     - Load the publicly accessible web page (e.g. http://0.tcp.ngrok.io:17526)
     - You should get a fist bump and see "Fist bump sent! 2"
     - Send the public URL to your friends!
     - The forwarding from public URL to internal web server on the Arduino relies on the ngrok
       client running. Once you end the ngrok client, the tunnel disappears and you will get
       a new public address the next time you set up the forwarding.

   Uses code from:
     HelloServer webserver example (File->Examples->Webserver->HelloServer)
     ESP32Servo library Sweep example (File->Examples->ESP32Servo->Sweep)

*/

#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>


const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";
int port = 8080;
WebServer server(port);

#include <ESP32Servo.h>

Servo myservo;  // create servo object to control a servo
int pos = 0;    // variable to store the servo position
// Recommended PWM GPIO pins on the ESP32 include 2,4,12-19,21-23,25-27,32-33
int servoPin = 18;

const int led = 13;
int hit_count = 0;

void handleRoot() {
  digitalWrite(led, 1);

  String message = "Serving page ";
  message += hit_count;
  hit_count += 1;
  Serial.println(message);

  fistBump();
  String text = "Fist bump sent! ";
  text += hit_count;
  server.send(200, "text/plain", text);
  digitalWrite(led, 0);
}

void handleNotFound() {
  digitalWrite(led, 1);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  digitalWrite(led, 0);
}

void setup(void) {
  delay(1000);

  // Setup servo
  myservo.setPeriodHertz(50);    // standard 50 hz servo
  myservo.attach(servoPin, 1000, 2000); // attaches the servo on pin 18 to the servo object
  // using default min/max of 1000us and 2000us
  // different servos may require different min/max settings
  // for an accurate 0 to 180 sweep

  //fistBump();

  delay(1000);
  pinMode(led, OUTPUT);
  digitalWrite(led, 5);
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println();
  Serial.println("Connecting to WiFi");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Port: ");
  Serial.println(port);
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.print(":");
  Serial.println(port);

  if (MDNS.begin("esp32")) {
    Serial.println("MDNS responder started");
  }

  server.on("/", handleRoot);

  server.on("/inline", []() {
    server.send(200, "text/plain", "this works as well");
  });

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
}

void loop(void) {
  server.handleClient();
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
