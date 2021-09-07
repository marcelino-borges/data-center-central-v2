#include <DHT_U.h>
#include <DHT.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <Ethernet.h>
#include <aWOT.h>
#include <IRremote.h>
#include "HttpResponse.h"
#include "Error.h"

#define AIR1_PIN 2
#define AIR2_PIN 3
#define DHT_PIN 7
#define DHT_TYPE DHT11
#define PORT 3000

/*
 *  ======================================================================================
 *
 *  Global constants
 * 
 *  ======================================================================================
 */

const char BASE_ROUTE[] = "/server/v1";
const int DEFAULT_CONNECTION_TIMEOUT = 3000;

/*
 *  ======================================================================================
 *
 *  Global variables
 * 
 *  ======================================================================================
 */
double DEFAULT_MAX_TEMP = 35.; // TODO: get from database
double DEFAULT_MIN_TEMP = 35.; // TODO: get from database

double currentMaxTemp = 35.; // TODO: get from database
double currentMinTemp = 10.; // TODO: get from database

// From Base64 = coopanest_devbox!17.09.2021!
char *server_token = "Y29vcGFuZXN0X2RldmJveCExNy4wOS4yMDIxIQ=="; // TODO: get from database
char auth[256];
/*
 *  ======================================================================================
 *
 *  Initial setup
 * 
 *  ======================================================================================
 */

// IP address where the ethernet shield is connected to (seen in cmd > "/ipconfig")
IPAddress ip(192, 168, 15, 100);
// PORT
EthernetServer server(PORT);
// Default MAC
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
// Temperature sensor
DHT_Unified dht(DHT_PIN, DHT_TYPE);
// HTTP App
Application app;
// Base route path
Router baseRoute;

/*
 *  ======================================================================================
 *
 *  Arduino's methods
 * 
 *  ======================================================================================
 */

void setup()
{
  Serial.begin(9600);
  while (!Serial)
  {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  Serial.println("Devbox Engineering - Data Center Central - Cloud");
  dht.begin();
  pinMode(AIR1_PIN, OUTPUT);
  pinMode(AIR2_PIN, OUTPUT);
  digitalWrite(AIR1_PIN, LOW);
  digitalWrite(AIR2_PIN, LOW);

  // start the Ethernet connection and the server:
  Ethernet.begin(mac, ip);

  validateEthernet();
  initializeServer();
  setRoutes();
  getSettingsFromDB();
}

void loop()
{
  // listen for incoming clients
  EthernetClient client = server.available();
  if (client)
  {
    Serial.println("New client incoming.");
    client.setConnectionTimeout(DEFAULT_CONNECTION_TIMEOUT);

    if (client.connected())
    {
      app.process(&client);
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
    Serial.println("Client disconnected!");
  }
}

/*
 *  ======================================================================================
 *
 *  Solution methods
 * 
 *  ======================================================================================
 */

//Validates if we have hardware and ethernet cable plugged in
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

// Calls the Begin() from the server and logs to Serial it's initialization
void initializeServer()
{
  server.begin();
  Serial.print("Server at ");
  Serial.print(Ethernet.localIP());
  Serial.print(":");
  Serial.println(PORT);
}

void getSettingsFromDB()
{
  // TODO: Implement
  // dbSettings = getDBSettings();
  // currentMaxTemp = dbSettings.currentMaxTemp;
  // currentMinTemp = dbSettings.currentMinTemp;
  // DEFAULT_MAX_TEMP = dbSettings.defaultMaxTemp;
  // DEFAULT_MIN_TEMP = dbSettings.defaultMinTemp;
  // server_key = dbSettings.token;
  // if(erro)
  //   Serial.println("Error xxxxxx");
}

// Logs an error to wherever we need to
void logError(const char *error)
{
  Serial.println("ERROR:");
  Serial.println(error);
  // TODO: Log to database all errors
}

// Gets the temperature read by our internal sensor DHT11.
float readTemperatureSensor()
{
  sensors_event_t event;
  dht.temperature().getEvent(&event);
  if (isnan(event.temperature))
  {
    return -1.0;
  }
  else
  {
    return event.temperature;
  }
}

// Effectively turns an air-conditioner ON by it's number (1 or 2).
// If [solo] parameter is passed as true, the air passed is turned
// on and the other one is turned off.
boolean turnAirConditionerOn(int airNumber, boolean solo = false)
{
  if (airNumber == 1)
  {
    digitalWrite(AIR1_PIN, HIGH);
    if (solo)
      digitalWrite(AIR2_PIN, LOW);

    return true;
  }
  else if (airNumber == 2)
  {
    digitalWrite(AIR2_PIN, HIGH);
    if (solo)
      digitalWrite(AIR1_PIN, LOW);
  }
  else
  {
    return false;
  }

  Serial.println("-----------------------------------------------------");
  Serial.print("Activated air number ");
  Serial.print(airNumber);
  Serial.println(solo ? " solo." : ".");

  return true;
}

// Effectively turns an air-conditioner OFF by it's number (1 or 2).
boolean turnAirConditionerOff(int airNumber)
{
  if (airNumber == 1)
    digitalWrite(AIR1_PIN, LOW);
  else if (airNumber == 2)
    digitalWrite(AIR2_PIN, LOW);
  else
    return false;

  Serial.println("-----------------------------------------------------");
  Serial.print("Deactivated air number ");
  Serial.print(airNumber);

  return true;
}

// Sets the variable [CURRENT_MIN_TEMP] to the value of the paremeter's value
void setMinTemp(double newTemp)
{
  currentMinTemp = newTemp;
}

// Sets the variable [CURRENT_MAX_TEMP] to the value of the paremeter's value
void setMaxTemp(double newTemp)
{
  currentMaxTemp = newTemp;
}

// Resets both [CURRENT_MIN_TEMP] and [CURRENT_MAX_TEMP] temperatures variables
// to default values: [DEFAULT_MIN_TEMP] and [DEFAULT_MAX_TEMP]
void resetTemps()
{
  currentMaxTemp = DEFAULT_MAX_TEMP;
  currentMinTemp = DEFAULT_MIN_TEMP;
}

/*
 * =======================================================================================
 *
 * Routes & Routes handlers
 * 
 * =======================================================================================
 */

// Creates all the routes of the application
void setRoutes()
{
  baseRoute.get("/temp", &handleGetTempRoute);
  baseRoute.post("/turnon/single/:airNumber", &handleTurnOnSingleAirRoute);
  baseRoute.post("/turnon/all", &handleTurnOnAllAirRoute);
  baseRoute.post("/turnoff/single/:airNumber", &handleTurnOffSingleAirRoute);
  baseRoute.post("/turnoff/all", &handleTurnOffAllAirRoute);
  baseRoute.post("/setvar/mintemp/:temp", &handleSetMinTempRoute);
  baseRoute.post("/setvar/maxtemp/:temp", &handleSetMaxTempRoute);
  baseRoute.get("/getvar/mintemp", &handleGetMinTempRoute);
  baseRoute.get("/getvar/maxtemp", &handleGetMaxTempRoute);
  baseRoute.post("/setvar/resettemps", &handleResetTempsRoute);

  app.options(BASE_ROUTE, &handleCors);

  app.header("Authorization", auth, sizeof(auth));
  app.use(&handleAccessSecurity);
  app.use(BASE_ROUTE, &baseRoute);
}

void handleAccessSecurity(Request &req, Response &res)
{
  char *tokenSent = req.get("Authorization");

  Serial.print("Token sent: ");
  Serial.println(tokenSent);
  if (!isTokenVerified(tokenSent))
  {
    Serial.print("Authentication failed for the route: ");
    Serial.println(req.path());
    res.sendStatus(401);
    res.end();
  }
  free(tokenSent);
  free(auth);
}

void handleCors(Request &req, Response &res)
{
  req.setTimeout(DEFAULT_CONNECTION_TIMEOUT);
  res.set("Access-Control-Allow-Origin", "*");
  res.set("Access-Control-Allow-Methods", "GET, POST, HEAD");
}

// Handles the GET method of the "/temp" to read temperature.
void handleGetTempRoute(Request &req, Response &res)
{
  float temp = readTemperatureSensor();
  if (temp >= 0 && temp <= 100)
  {
    res.set("Content-Type", "application/json");
    res.print("{ \"data\": { \"temperature\": ");
    res.print(temp);
    res.print(" } }");
    Serial.print("getTemp: ");
    Serial.print(temp);
    Serial.println("*C");
  }
  else
  {
    //logError(Error::ERROR_MSG_READ_TEMP);
    res.sendStatus(500);
  }
}

// Handles the POST method of the "/turnon/single/:airNumber"
// route to turn on a single air-conditioner.
void handleTurnOnSingleAirRoute(Request &req, Response &res)
{
  // Extracts the a number from the request
  int airNumber = getAirNumberAsIntFromRequest(req);
  //Turns on the air-conditioner
  if (turnAirConditionerOn(airNumber))
  {
    // Logs to serial
    Serial.println("----------------------------------------------------------------");
    Serial.print("Turning on air number ");
    Serial.println(airNumber);
    res.sendStatus(200);
  }
  else
  {
    // Logs to serial
    logError("Error turning on air number " + airNumber);
    res.sendStatus(400);
  }
}

// Handles the POST method of the route "/turnoff/single/:airNumber"
// to turn off a single air-conditioner.
void handleTurnOffSingleAirRoute(Request &req, Response &res)
{
  // Extracts the air number from the request
  int airNumber = getAirNumberAsIntFromRequest(req);
  //Turns off the air-conditioner
  if (turnAirConditionerOff(airNumber))
  {
    // Logs to serial
    logError("Turning off air number " + airNumber);
    res.sendStatus(200);
  }
  else
  {
    // Logs to serial
    logError("Error turning off air number " + airNumber);
    res.sendStatus(400);
  }
}

// Handles the POST method of the route "/turnoff/all"
// to turn on all the air-conditioners.
void handleTurnOnAllAirRoute(Request &req, Response &res)
{
  // Turn devices on
  if (turnAirConditionerOn(1) &&
      turnAirConditionerOn(2))
  {
    res.sendStatus(200);
  }
  else
  {
    logError("Error turning on all air-conditioners");
    res.sendStatus(400);
  }
}

// Handles the POST method of the route "/turnoff/all"
// to turn off all the air-conditioners.
void handleTurnOffAllAirRoute(Request &req, Response &res)
{
  // Turn devices off
  if (turnAirConditionerOff(1) &&
      turnAirConditionerOff(2))
  {
    res.sendStatus(200);
  }
  else
  {
    logError("Error turning off all air-conditioners");
    res.sendStatus(400);
  }
}

// Handles the POST method of the route "/setvar/mintemp"
// to set the var of the minimum tempeperature allowed
void handleSetMinTempRoute(Request &req, Response &res)
{
  setMinTemp(getTempAsDoubleFromRequest(req));
  // TODO: handle error
  res.sendStatus(200);
  // Logs to Serial
  Serial.println("----------------------------------------------------------------");
  Serial.println("The minimum temperature was set to ");
  Serial.println(currentMinTemp);
  Serial.println("*C.");
}

// Handles the POST method of the route "/setvar/mintemp"
// to set the var of the minimum tempeperature allowed
void handleSetMaxTempRoute(Request &req, Response &res)
{
  setMaxTemp(getTempAsDoubleFromRequest(req));
  // TODO: handle error
  res.sendStatus(200);

  // Logs to Serial
  Serial.println("----------------------------------------------------------------");
  Serial.println("The maximum temperature was set to ");
  Serial.println(currentMaxTemp);
  Serial.println("*C.");
}

// Handles the GET method of the route "/getvar/mintemp"
// to get the var of the minimum tempeperature allowed
void handleGetMinTempRoute(Request &req, Response &res)
{
  // TODO: handle error

  res.set("Content-Type", "application/json");
  res.print("{ \"data\": { \"minTemperature\": ");
  res.print(currentMinTemp);
  res.print(" } }");
  // Logs to Serial
  Serial.println("----------------------------------------------------------------");
  Serial.print("The minimum temperature is ");
  Serial.print(currentMinTemp);
  Serial.println("*C.");
}

// Handles the GET method of the route "/getvar/mintemp"
// to get the var of the minimum tempeperature allowed
void handleGetMaxTempRoute(Request &req, Response &res)
{
  // TODO: handle error

  res.set("Content-Type", "application/json");
  res.print("{ \"data\": { \"maxTemperature\": ");
  res.print(currentMaxTemp);
  res.print(" } }");
  // Logs to Serial
  Serial.println("----------------------------------------------------------------");
  Serial.print("The maximum temperature is ");
  Serial.print(currentMinTemp);
  Serial.println("*C.");
}

// Handles the POST method of the route "/setvar/mintemp"
// to set the var of the minimum tempeperature allowed
void handleResetTempsRoute(Request &req, Response &res)
{
  resetTemps();
  // TODO: handle error

  // Logs to Serial
  Serial.println("----------------------------------------------------------------");
  Serial.println("Bounding temperatures were reset to MIN[");
  Serial.println(currentMinTemp);
  Serial.println("*C] and MAX[");
  Serial.println(currentMaxTemp);
  Serial.println("*C].");
}

// Extracts the number (as an INT) of the air-conditioner
// from a request that awaits for an "airNumber" parameter.
int getAirNumberAsIntFromRequest(Request &req)
{
  // Buffer
  char airNumber[2];
  // Passing the name of the parameter we want to find
  req.route("airNumber", airNumber, sizeof(airNumber));
  // Cast from char[] to int
  int number = atoi(airNumber);
  // Free memory space
  free(airNumber);

  return number;
}

// Extracts the number (as an INT) of the air-conditioner
// from a request that awaits for an "airNumber" parameter.
int getTempAsDoubleFromRequest(Request &req)
{
  // Buffer
  char temp[16];
  // Passing the name of the parameter we want to find
  req.route("temp", temp, sizeof(temp));
  // Cast from char[] to int
  double tempAsDouble = atof(temp);
  // Free memory space
  free(temp);

  return tempAsDouble;
}

// Verify if the given token equals the server secret
boolean isTokenVerified(char *token)
{
  const char *key = token;
  return strcmp(token, server_token) == 0;
}