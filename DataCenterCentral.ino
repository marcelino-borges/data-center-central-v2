#include <DHT_U.h>
#include <DHT.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <Ethernet.h>
#include "HttpResponse.h"
#include "Error.h"

#define AIR1_PIN 2
#define AIR2_PIN 3
#define DHT_PIN 7
#define DHT_TYPE DHT11

/*
 *  ======================================================================================
 *  Solution Variables
 *  ======================================================================================
 */

const String WELCOME_MSG = "Devbox Data Center";
const String BASE_ROUTE = "/server/v1/";
const String GET_METHOD_TYPE = "GET";
const String POST_METHOD_TYPE = "POST";
const String SUBROUTE_GET_TEMP = "temp";
const String SUBROUTE_TURN_ON = "turnon";

int IP_ADDRESS[] = {192, 168, 15, 100};
int PORT = 3000;
int CONNECTION_TIMEOUT = 3000;

/*
 *  ======================================================================================
 *  Initial setup
 *  ======================================================================================
 */

// IP address where the ethernet shield is connected to (seen in cmd > "/ipconfig")
IPAddress ip(IP_ADDRESS[0], IP_ADDRESS[1], IP_ADDRESS[2], IP_ADDRESS[3]);
// PORT
EthernetServer server(PORT);
// Default MAC
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
// Temperature sensor
DHT_Unified dht(DHT_PIN, DHT_TYPE);

/*
 *  ======================================================================================
 *  Methods
 *  ======================================================================================
 */

void setup()
{
  Serial.begin(9600);
  while (!Serial)
  {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  Serial.println(WELCOME_MSG);
  dht.begin();
  pinMode(AIR1_PIN, OUTPUT);
  pinMode(AIR2_PIN, OUTPUT);
  digitalWrite(AIR1_PIN, LOW);
  digitalWrite(AIR2_PIN, LOW);

  // start the Ethernet connection and the server:
  Ethernet.begin(mac, ip);

  validateEthernet();
  initializeServer();
}

void loop()
{
  // listen for incoming clients
  EthernetClient client = server.available();
  if (client)
  {
    Serial.println("new client");
    client.setConnectionTimeout(CONNECTION_TIMEOUT);
    client.setTimeout(CONNECTION_TIMEOUT);
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    String req = "";
    while (client.connected())
    {
      if (client.available())
      {
        char c = client.read();
        req.concat(c);

        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank)
        {
          HttpResponse response = processRequest(req);
          DynamicJsonDocument json(512);

          json["data"] = response.data;
          json["error"] = response.error;
          json["code"] = response.code;

          Serial.println("-----------------------------------------------------");
          Serial.println("Json to send:");
          serializeJson(json, Serial);
          Serial.println("\n");

          client.print("HTTP/1.1 ");
          client.println(response.code);
          client.println("Content-Type: text/html");
          client.println("Content-Type: application/json");
          client.println("Connection: close"); // the connection will be closed after completion of the response
          client.println("Refresh: 5");        // refresh the page automatically every 5 sec
          client.println();
          serializeJson(json, client);
          break;
        }
        if (c == '\n')
        {
          // you're starting a new line
          currentLineIsBlank = true;
        }
        else if (c != '\r')
        {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
    Serial.println("client disconnected");
  }
}

bool validateEthernet()
{
  // Check for Ethernet hardware present
  if (Ethernet.hardwareStatus() == EthernetNoHardware)
  {
    Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
    while (true)
    {
      delay(1); // do nothing, no point running without Ethernet hardware
    }
  }
  if (Ethernet.linkStatus() == LinkOFF)
  {
    Serial.println("Ethernet cable is not connected.");
  }
}

void initializeServer()
{
  server.begin();
  Serial.print("Server at ");
  Serial.print(Ethernet.localIP());
  Serial.print(":");
  Serial.println(PORT);
}

void logError(String error)
{
  Serial.println("ERROR:");
  Serial.println(error);
}

float getTemperature()
{
  sensors_event_t event;
  dht.temperature().getEvent(&event);
  if (isnan(event.temperature))
  {
    Serial.println(Error::ERROR_MSG_READ_TEMP);
    logError(Error::ERROR_MSG_READ_TEMP);
    return -1.0;
  }
  else
  {
    float temperature = event.temperature;
    String tempMsg = String("Temperature read: ") + String(temperature) + String(" *C");
    return temperature;
  }
}

HttpResponse processRequest(String req)
{
  // Getting method type from the request
  String method_type = req.substring(0, req.indexOf(BASE_ROUTE));
  method_type.trim();
  // Getting the route from the request
  String route = req.substring(req.indexOf(BASE_ROUTE), req.indexOf("HTTP/"));
  route.trim();

  if (method_type.equals(GET_METHOD_TYPE))
  {
    String subroute = removeBaseRoute(route);

    if (subroute.equals(SUBROUTE_GET_TEMP))
    {
      float temp = getTemperature();

      Serial.println("-----------------------------------------------------");
      Serial.print("Temperature read on a GET call: ");
      Serial.println(temp);
      HttpResponse *response = new HttpResponse(temp);
      return *response;
    }
    else
    {
      HttpResponse *response = new HttpResponse(Error::ERROR_MSG_UNKNOWN_SUBROUTE +
                                                    String(" Route requested: ") +
                                                    route +
                                                    String(". Method requested: ") +
                                                    method_type,
                                                400);
      return *response;
    }
  }
  else if (method_type.equals(POST_METHOD_TYPE))
  {
    String subroute = removeBaseRoute(route);

    if (subroute.startsWith(SUBROUTE_TURN_ON))
    {
      String airNumber = subroute;
      airNumber.replace(SUBROUTE_TURN_ON + "/", "");
      Serial.println("-----------------------------------------------------");
      Serial.print("Turning on air-conditioner ");
      Serial.println(airNumber);

      // Allow only numbers 1 and 2 as air conditioner
      if (!airNumber.equals("1") && !airNumber.equals("2"))
      {
        HttpResponse *response = new HttpResponse(
            Error::ERROR_MSG_TURNON_UNKNOWN_AIR +
                String(" Route requested: ") +
                route +
                String(". Method requested: ") +
                method_type,
            400);
        Serial.println("-----------------------------------------------------");
        Serial.print("response->error: ");
        Serial.println(response->error);
        return *response;
      }
      else
      {
        turnAirConditionerOn(airNumber.toInt(), true);
        HttpResponse *response = new HttpResponse("Air-conditioner " + airNumber + " activated.");
        return *response;
      }
    }
  }
  else
  {
    HttpResponse *response = new HttpResponse(Error::ERROR_MSG_INVALID_HTTP_METHOD_TYPE +
                                                  String(" Method requested: ") +
                                                  method_type,
                                              404);

    return *response;
  }
}

String removeBaseRoute(String route)
{
  String subroute = route;
  subroute.replace(BASE_ROUTE, "");
  return subroute;
}

void turnAirConditionerOn(int airNumber, boolean solo)
{
  if (airNumber == 1)
  {
    digitalWrite(AIR1_PIN, HIGH);
    if (solo)
      digitalWrite(AIR2_PIN, LOW);
  }
  else if (airNumber == 2)
  {
    digitalWrite(AIR2_PIN, HIGH);
    if (solo)
      digitalWrite(AIR1_PIN, LOW);
  }
  Serial.println("-----------------------------------------------------");
  Serial.print("Air number ");
  Serial.print(airNumber);
  Serial.print(" activated");
  Serial.println(solo ? " solo." : ".");
}