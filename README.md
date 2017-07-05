# SimGpsTransmitter
Simple project which logs data from GPS module (NEO 6M) into a web service using data HTTP GET requests through SIM800L module. 
Location is sent for each interval given as configuration variable 'frequency'. 

Connecting modules:
Pin3 -> GPS-module-RX
Pin4 -> GPS-module-TX
Pin5 -> SIM-module-TX
Pin6 -> SIM-module-RX
  
Dependency(TinyGPS++ library): <http://arduiniana.org/libraries/tinygpsplus/>
