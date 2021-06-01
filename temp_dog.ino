#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP8266WiFi.h> 
#include <ESP8266HTTPClient.h>
#include <TinyGPS++.h>        // include TinyGPS++ library
#include <TimeLib.h>          // include Time library
#include <SoftwareSerial.h>   // include Software serial library


//  Variables
//Helix and internet
const char* orionAddressPath = "3.88.180.168:1026/v2"; //Helix IP Address 
const char* deviceID = "urn:ngsi-ld:entity:001"; //Device ID (example: urn:ngsi-ld:entity:001) 
const char* ssid = "SSID"; //Wi-Fi Credentials - replace with your SSID
const char* password = "PASSWORD"; //Wi-Fi Credentials - replace with your Password

//Pulse Sensor
const int PulseSensorPurplePin = A0;        // Pulse Sensor PURPLE WIRE connected to ANALOG PIN 0
const int Threshold = 550;            // Determine which Signal to "count as a beat", and which to ingore.
int pulse;   // holds the incoming raw data. Signal value can range from 0-1024

//DS18B20
static const int TXPin = 5, RXPin = 16; //Pinagem TX para pino D1 e RX para o pino D2
const int oneWireBus = 4; // GPIO where the DS18B20 is connected to 
float temperature;
OneWire oneWire(oneWireBus); // Setup a oneWire instance to communicate with any OneWire devices
DallasTemperature sensors(&oneWire); // Pass our oneWire reference to Dallas Temperature sensor 

//Module GPS
SoftwareSerial Serial_GPS(TXPin, RXPin); //Conexão serial do module GPS

WiFiClient espClient;
HTTPClient http;
TinyGPSPlus gps;

// The SetUp Function:
void setup() {

  Serial.begin(115200);         // Set's up Serial Communication at certain speed.
  Serial_GPS.begin(9600); // initialize software serial at 9600 bps

  //Wi-Fi access
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }

  Serial.println("Connected WiFi network!");
  delay(2000);

  //creating the device in the Helix Sandbox (plug&play) 
  orionCreateEntitie(deviceID);

  // Start the DS18B20 sensor
  sensors.begin();

  Serial.println(F("Pulse Sensor"));
  Serial.println(F("Module GPS GY-NEO6MV2"));
  Serial.print(F("Temperature Sensor DS18B20")); 
  Serial.println(); 
}

void loop() {

  //Pulse Sensor
  pulse = analogRead(PulseSensorPurplePin);  // Read the PulseSensor's value.
                                              // Assign this value to the "Signal" variable
  Serial.println(pulse);                    // Send the Signal value to Serial Plotter.
  
  //Temperature Sensor
  sensors.requestTemperatures(); 
  temperature = sensors.getTempCByIndex(0);
  Serial.print("Temperatura ambiente: ");
  Serial.print(temperature);
  Serial.println("ºC");

  orionUpdate(temperature, pulse, deviceID);

  while (Serial_GPS.available() > 0){
    if (gps.encode(Serial_GPS.read())){
      Serial.print(gps.location.lat(), 6); //latitude
      Serial.print(F(","));
      Serial.println(gps.location.lng(), 6); //longitude
    }
  }
  
  delay(10000);
}

//plug&play
void orionCreateEntitie(String entitieName) {

    String bodyRequest = "{\"id\": \"" + entitieName + "\", \"type\": \"iot\", \"temperature\": { \"value\": \"0\", \"type\": \"integer\"},\"pulse\": { \"value\": \"0\", \"type\": \"integer\"}}";
    httpRequest("/entities", bodyRequest);
}

//update 
void orionUpdate(float temperature, int pulse, String entitieName){

//    String bodyRequest = "{ \"temperature\": "+ String(temperature) + ", \"pulse\": "+ String(pulse) +" }";
    String bodyRequest = "{\"temperature\": { \"value\": \""+ String(temperature) + "\", \"type\": \"float\"}, \"pulse\": { \"value\": \""+ String(pulse) +"\", \"type\": \"float\"}}";
//    String pathRequest = "/dogma/verify/";
    String pathRequest = "/entities/" + entitieName + "/attrs?options=forcedUpdate";
    httpRequest(pathRequest, bodyRequest);
}

//request
void httpRequest(String path, String data)
{
  String payload = makeRequest(path, data);

  if (!payload) {
    return;
  }

  // Serial.println("##[RESULT]## ==> " + payload);

}

//request
String makeRequest(String path, String bodyRequest)
{
  String fullAddress = "http://" + String(orionAddressPath) + path;
  http.begin(fullAddress);

  http.addHeader("Content-Type", "application/json"); 
  http.addHeader("Accept", "application/json"); 
  http.addHeader("fiware-service", "helixiot"); 
  http.addHeader("fiware-servicepath", "/"); 

  int httpCode = http.POST(bodyRequest);

  String response =  http.getString();
  
  if (httpCode < 0) {
    Serial.println("request error - " + httpCode);
    return "";
  }

  if (httpCode != HTTP_CODE_OK) {
    return "";
  }

  http.end();

  return response;
}
