#include <TMCStepper.h>
#include <IRremote.h> // Include the IRremote library
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>

// Network credentials
const char* ssid     = "MyESP32";
const char* password = "12345678";

// Set web server port number to 80
WiFiServer server(80);


#define EN_PIN           16 // Enable pin
#define DIR_PIN          19 // Direction pin
#define STEP_PIN         18 // Step pin
#define SW_RX            15 // TMC2209 SoftwareSerial receive pin
#define SW_TX            0  // TMC2209 SoftwareSerial transmit pin
#define SERIAL_PORT Serial2 // TMC2209 HardwareSerial port
#define DRIVER_ADDRESS 0b00 // TMC2209 Driver address according to MS1 and MS2
#define IR_RECEIVE_PIN   4  // IR receiver pin
#define R_SENSE 0.11f 

int blindLength = 80000;  // Number of steps for full blind length
int blindPosition = 0;    // Initial blind position, 0 is fully open
int blindSpeed = 1;       // Delay between pulses, smaller delay, higher speed

//Boolean flags indicating whether the blind should move up or down, respectively.
bool moveUp = false;
bool moveDown = false;

TMC2209Stepper driver(&SERIAL_PORT, R_SENSE, DRIVER_ADDRESS);


void setup() {
  pinMode(EN_PIN, OUTPUT);
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  digitalWrite(EN_PIN, LOW);  // Enable driver in hardware

  driver.begin();             // initializes the TMC2209 driver.
  driver.toff(5);             //sets the driver's standby current and time-off delay.
  driver.rms_current(600);    //sets the root mean square (RMS) current for the stepper motor.
  driver.microsteps(16);      //configures the microstepping resolution of the driver.
  driver.pwm_autoscale(true); //nables automatic scaling of the driver's PWM amplitude.

  // Start the receiver
  IrReceiver.begin(IR_RECEIVE_PIN);

  Serial.begin(1152000);              // initializes the serial communication at a baud rate of 1152000.

  if (!WiFi.softAP(ssid, password)) { //creates a soft access point with the specified SSID and password.                                      
    log_e("Soft AP creation failed.");
    while(1);
  }
  IPAddress myIP = WiFi.softAPIP();   //retrieves the IP address assigned to the soft AP.
  server.begin();                     //starts the web server to listen for incoming client connections.
}

void loop() {

 WiFiClient client = server.available();   // listen for incoming clients

  if (client) {                             // if you get a client,
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available() or IrReceiver.decode()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        if (c == '\n') {                    // if the byte is a newline character

          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();

            // the content of the HTTP response follows the header:
            
            client.println("<p style=\"text-align:center;\">");
            client.println("<a href=\"/H\" style=\"font-size: 350px;\">&#9650;</a><br>");
            client.println("<a href=\"/L\" style=\"font-size: 350px;\">&#9660;</a><br>");
            client.println("</p>");
            // The HTTP response ends with another blank line:
            client.println();
            // break out of the while loop:
            break;
          } else {    // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }

        // Check to see if the client request was "GET /H" or "GET /L":
        if (currentLine.endsWith("GET /L") or IrReceiver.decodedIRData.command == 0x18) {
                moveDown = true;
                driveMotor();
                IrReceiver.resume(); // Enable receiving of the next value
                delay(200);  
        }
        if (currentLine.endsWith("GET /H") or IrReceiver.decodedIRData.command == 0x52) {
                moveUp = true;
                driveMotor();
                IrReceiver.resume(); // Enable receiving of the next value
                delay(200);
        }        
      }
    }
    // close the connection:
    client.stop();

  } else if (IrReceiver.decode()) {
    if (IrReceiver.decodedIRData.command == 0x18){
        moveDown = true;
        driveMotor();
    }else if(IrReceiver.decodedIRData.command == 0x52){
        moveUp = true;
        driveMotor();
    }
    IrReceiver.resume(); // Enable receiving of the next value
    delay(200);
  }

}

void driveMotor() {
  if (moveUp){
    if (blindPosition <= blindLength - 2000) {
      moveUp = false;
      digitalWrite(DIR_PIN, LOW);
      moveMotor(2000);
      blindPosition = blindPosition + 2000;

    }
  }else  if (moveDown){
    if (blindPosition >= 2000) {
      moveDown = false;
      digitalWrite(DIR_PIN, HIGH);
      moveMotor(2000);
      blindPosition = blindPosition - 2000;
    }
  }
}

void moveMotor(int noSteps) {

  for (int i = 0; i <= noSteps; i++) {
    digitalWrite(STEP_PIN, HIGH);
    delay(blindSpeed);
    digitalWrite(STEP_PIN, LOW);
    delay(blindSpeed);
  }
}

