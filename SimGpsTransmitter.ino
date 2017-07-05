/*
  SIM GPS Transmitter

  Simple project which logs data from GPS module (NEO 6M) into a web service using data HTTP GET requests through SIM800L module. 
  Location is sent for each interval given as configuration variable 'frequency'. 

  Connecting modules:
  Pin3 -> GPS-module-RX
  Pin4 -> GPS-module-TX
  Pin5 -> SIM-module-TX
  Pin6 -> SIM-module-RX
  
  Dependency(TinyGPS++ library): http://arduiniana.org/libraries/tinygpsplus/
  
  created   Jul 2017
  by CheapskateProjects

  ---------------------------
  The MIT License (MIT)

  Copyright (c) 2017 CheapskateProjects

  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <SoftwareSerial.h>
#include <TinyGPS++.h>

// Config (Use APN corresponding to your service providers configs)
static String apn="internet";
static String loggingPassword="qwerty";
static String serverIP="1.2.3.4";

// Pins where GPS and SIM modules are connected
static const int SimRXPin = 5, SimTXPin = 6;
static const int GPSRXPin = 4, GPSTXPin = 3;

// Used baud rates (define based on used modules)
static const uint32_t SimBaudrate = 9600;
static const uint32_t GPSBaud = 9600;
static const uint32_t SerialBaudrate = 9600;

// How frequently we want to send the location (milliseconds)
static const unsigned long frequency = 15000;

String responseString;
TinyGPSPlus gps;
unsigned long previous=0;
SoftwareSerial sim_ss(SimRXPin, SimTXPin);
SoftwareSerial gps_ss(GPSRXPin, GPSTXPin);

void setup()
{
  /*
   * Start serial communications. We can only listen to one ss at a time so changing that 
   * between sim and gps as needed
   */
  Serial.begin(SerialBaudrate);
  sim_ss.begin(SimBaudrate);
  gps_ss.begin(GPSBaud);
  sim_ss.listen();

  Serial.println("Waiting for init");
  // Wait few seconds so that module is able to take AT commands
  delay(5000);
  Serial.println("Init... waiting until module has connected to network");

  // Start AT communication. This sets auto baud and enables module to send data
  sim_ss.println("AT");
  // Wait until module is connected and ready
  waitUntilResponse("SMS Ready");
  

  // Full mode
  sim_ss.println("AT+CFUN=1");
  waitUntilResponse("OK");
  // Set credentials (TODO username and password are not configurable from variables). This may work without CSTT and CIICR but sometimes it caused error without them even though APN is given by SAPBR 
  sim_ss.write("AT+CSTT=\"");
  sim_ss.print(apn);
  sim_ss.write("\",\"\",\"\"\r\n");
  waitUntilResponse("OK");
  // Connect and get IP
  sim_ss.println("AT+CIICR");
  waitUntilResponse("OK");
  // Some more credentials
  sim_ss.write("AT+SAPBR=3,1,\"APN\",\"");
  sim_ss.print(apn);
  sim_ss.write("\"\r\n");
  waitUntilResponse("OK");
  sim_ss.println("AT+SAPBR=3,1,\"USER\",\"\"");
  waitUntilResponse("OK");
  sim_ss.println("AT+SAPBR=3,1,\"PWD\",\"\"");
  waitUntilResponse("OK");
  sim_ss.println("AT+SAPBR=1,1");
  waitUntilResponse("OK");
  sim_ss.println("AT+HTTPINIT");
  waitUntilResponse("OK");

  gps_ss.listen();
  previous = millis();
  Serial.println("starting loop!");
}

/*
 *  Read from SIM serial until we get known response. TODO error handling!
 * */
void waitUntilResponse(String response)
{
  responseString="";
  while(responseString.indexOf(response) < 0)
  {
    readResponse();
    Serial.println(responseString);
  }
}

/*
 * Read from serial until we get response line ending with line separator
 * */
void readResponse()
{
  responseString = "";
  while(responseString.length() <= 0 || !responseString.endsWith("\n"))
  {
    tryToRead();
  }
}

/*
 * If we have anything available on the serial, append it to response string
 * */
void tryToRead()
{
  while(sim_ss.available())
  {
    char c = sim_ss.read();  //gets one byte from serial buffer
    responseString += c; //makes the string readString
  }
}

void loop()
{
  // If we have data, decode and log the data
  while (gps_ss.available() > 0)
   if (gps.encode(gps_ss.read()))
    logInfo();

  // Test that we have had something from GPS module within first 10 seconds
  if (millis() - previous > 10000 && gps.charsProcessed() < 10)
  {
    Serial.println("GPS wiring error!");
    while(true);
  }
}

void logInfo()
{
  // Causes us to wait until we have satelite fix
  if(!gps.location.isValid())
  {
    Serial.println("Not a valid location. Waiting for satelite data.");
    return;
  }

  // Only log once per frequency
  if(millis() - previous > frequency)
  {
    sim_ss.listen();
    previous = millis();
    String url = "AT+HTTPPARA=\"URL\",\"http://";
    url += serverIP;
    url += "/map/log.php?key=";
    url += loggingPassword;
    url += "&coordinates=";
    url += String(gps.location.lat(), DEC);
    url += ",";
    url += String(gps.location.lng(), DEC);
    url += "\"";
    sim_ss.println(url);
    waitUntilResponse("OK");
    sim_ss.println("AT+HTTPACTION=0");
    waitUntilResponse("+HTTPACTION:");
    gps_ss.listen();
  }
}


