#include <NMEAGPS.h>  //parsing the comma seperated values to be more human readable
#include <stdlib.h>   //the lib is needed for the floatToString() helper function
#include <NeoSWSerial.h>  //it is used instead of SoftwareSerial

//NOTE: THE CODE WILL ONLY WORK IF YOU HAVE ALREADY UNLOCKED YOUR SIM CARD - SO THE PHONE WON'T ASK FOR PIN CODE WHENEVER YOU USE IT

//if there are more devices, the server must differentiate them by the deviceID
int deviceId = 13;

//SoftwareSerial variables
static const int gpsRX = 3, gpsTX = 4;
static const uint32_t gpsBaud = 9600;
static const int simRX = 10, simTX = 11;
static const uint32_t simBaud = 9600;

//SoftwareSerial instances
NeoSWSerial gpsPort(gpsRX, gpsTX);
NeoSWSerial Sim800l(simRX, simTX);

//GPS instance
NMEAGPS gps;
gps_fix fix;

//for sending data to a remote webserver
String ipAddress = ""; //this is the ip address of our server - e.g. "123.123.123.123"
String APN = ""; //check your Internet provider's website for the APN e.g. "internet"

//helper variables for waitUntilResponse() function
String response = "";
static long maxResponseTime = 5000;
unsigned long lastTime;

//The frequency of http requests (seconds)
int refreshRate = 15; //SECONDS
//variables for a well-scheduled loop - in which the sendData() gets called every 15 secs (refresh rate)
unsigned long last;
unsigned long current;
unsigned long elapsed;

//if there is an error in sendLocation() function after the GPRS Connection is setup - and the number of errors exceeds 3 - the system reboots. (with the help of the reboot pin) 
int maxNumberOfErrors = 3;
boolean gprsConnectionSetup = false;
boolean reboot = false;
int errorsEncountered = 0; //number of errors encountered after gprsConnection is set up - if exceeds the max number of errors the system reboots
int resetPin = 12;

//if any error occurs with the gsm module or gps module, the corresponding LED will light up - until they don't get resolved
int sim800Pin = 2; //error pin
int gpsPin = 6; //error pin



//a helper function which converts a float to a string with a given precision
String floatToString(float x, byte precision = 2) {
  char tmp[50];
  dtostrf(x, 0, precision, tmp);
  return String(tmp);
}


void setup(){

  //setup resetPin
  digitalWrite(resetPin, HIGH);
  delay(200);
  pinMode(resetPin, OUTPUT);
  
  //init
  Serial.begin(9600);
  gpsPort.begin(gpsBaud);
  Sim800l.begin(simBaud);
  
  pinMode(sim800Pin, OUTPUT);
  pinMode(gpsPin, OUTPUT);
  
  Sim800l.write(27); //Clears buffer for safety
  Serial.println("Beginning...");
  delay(15000); //Waiting for Sim800L to get signal
  Serial.println("SIM800L should have booted up");
  
  Sim800l.listen(); //The GSM module and GPS module can't communicate with the arduino board at once - so they need to get focus once we need them
  setupGPRSConnection(); //Enable the internet connection to the SIM card
  Serial.println("Connection set up");
  
  gpsPort.listen();

  last = millis();
  
}

void loop(){

  current = millis();
  elapsed += current - last;
  last = current;
  Serial.println(elapsed);
  while (gps.available(gpsPort)) {
      fix = gps.read();
    }
  if(elapsed >= (refreshRate * 1000)) {
    sendData();
    elapsed -= (refreshRate * 1000);
  }

  if ((gps.statistics.chars < 10)) {
     //no gps detected (maybe wiring)
     Serial.println("NO GPS DETECTED OR BEFORE FIRST HTTP REQUEST");
  }

  if(reboot){
    digitalWrite(resetPin, LOW);   
  }
  
}

void sendData(){
 Serial.println("data to be sent");
  if (fix.valid.location) {
    digitalWrite(gpsPin, LOW);
    String lat = floatToString(fix.latitude(), 5);
    String lon = floatToString(fix.longitude(), 5);
    sendLocation(lat, lon);
  } else {
    sendLocation("-1", "-1");
    digitalWrite(gpsPin, HIGH);   
  }
}

void setupGPRSConnection(){
 Sim800l.println("AT+SAPBR=3,1,\"Contype\",\"GPRS\""); //Connection type: GPRS
 waitUntilResponse("OK");
 Sim800l.println("AT+SAPBR=3,1,\"APN\",\"" + APN + "\""); //We need to set the APN which our internet provider gives us
 waitUntilResponse("OK");
 Sim800l.println("AT+SAPBR=1,1"); //Enable the GPRS
 waitUntilResponse("OK");
 Sim800l.println("AT+HTTPINIT"); //Enabling HTTP mode
 waitUntilResponse("OK");
 gprsConnectionSetup = true;
}

//ERROR handler - exits if error arises or a given time exceeds with no answer - or when everything is OK
void waitUntilResponse(String resp){ 
  lastTime = millis();
  response="";
  String totalResponse = "";
  while(response.indexOf(resp) < 0 && millis() - lastTime < maxResponseTime)
  { 
    readResponse();
    totalResponse = totalResponse + response;
    Serial.println(response);
  }
  
  if(totalResponse.length() <= 0)
  { 
    Serial.println("NO RESPONSE");
    digitalWrite(sim800Pin, HIGH);
    if (gprsConnectionSetup == true){
      Serial.println("error");
      errorsEncountered++;
    }
  }
  else if (response.indexOf(resp) < 0)
  { 
    if (gprsConnectionSetup == true){
      Serial.println("error");
      errorsEncountered++;
    }
    Serial.println("UNEXPECTED RESPONSE");
    Serial.println(totalResponse);
    digitalWrite(sim800Pin, HIGH);
  }else{
    Serial.println("SUCCESSFUL");
    digitalWrite(sim800Pin, LOW);
    errorsEncountered = 0;
  }

  //if there are more errors or equal than previously set ==> reboot!
  if (errorsEncountered>= maxNumberOfErrors){
    reboot = true;
  }
}

//the function - which is responsible for sending data to the webserver
void sendLocation(String lat, String lon){
 Sim800l.listen();
 //The line below sets the URL we want to connect to
 Sim800l.println("AT+HTTPPARA=\"URL\", \"http://" + ipAddress +  "/log_info.php?dev_id=13&lat=" + lat + "&lon=" + lon + "\"");
 waitUntilResponse("OK");
 //GO
 Sim800l.println("AT+HTTPACTION=0");
 waitUntilResponse("200");
 Serial.println("Location sent");
 gpsPort.listen();
}

void readResponse(){
  response = "";
  while(response.length() <= 0 || !response.endsWith("\n"))
  {
    tryToRead();
    if(millis() - lastTime > maxResponseTime)
    {
      return;
    }
  }
}

void tryToRead(){ 
  while(Sim800l.available()){
    char c = Sim800l.read();  //gets one byte from serial buffer
    response += c; //makes the string readString
  }
}
