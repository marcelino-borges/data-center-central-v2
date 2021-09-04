#include <DHT_U.h>
#include <DHT.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <Ethernet.h>

#define DHT_TYPE DHT11
#define DHT_PIN DHT11

/*  ======================================================================================
 *  Solution Variables
 */
String WELCOME_MSG = "Devbox Data Center";
int IP_ADDRESS[] = {192, 168, 15, 100};
int PORT = 3000;

/*  ======================================================================================
 *  Initial setup
 */
// IP address where the ethernet shield is connected to (seen in cmd > "/ipconfig")
IPAddress ip(IP_ADDRESS[0], IP_ADDRESS[1], IP_ADDRESS[2], IP_ADDRESS[3]);
// PORT
EthernetServer server(PORT);
// Default MAC
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
// Temperature sensor
DHT_Unified dht(DHT_PIN, DHT_TYPE);

/*  ======================================================================================
 *  Methods
 */
void setup()
{
  Serial.begin(9600);
  while (!Serial)
  {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  Serial.println(WELCOME_MSG);

  // start the Ethernet connection and the server:
  Ethernet.begin(mac, ip);

  ValidateEthernet();

  // start the server
  server.begin();
  Serial.print("Server at ");
  Serial.print(Ethernet.localIP());
  Serial.print(":");
  Serial.println(PORT);
}

void loop()
{
  // listen for incoming clients
  EthernetClient client = server.available();
  if (client)
  {
    Serial.println("new client");
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    char c;
    while (client.connected())
    {
      if (client.available())
      {
        c = client.read();

        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank)
        {
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Content-Type: application/json");
          client.println("Connection: close"); // the connection will be closed after completion of the response
          client.println("Refresh: 5");        // refresh the page automatically every 5 sec
          client.println();
          client.println("{ \"name\": \"Marcelino Borges\", \"age\": 31 }");
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
    Serial.print(c);
    delay(1);
    // close the connection:
    client.stop();
    Serial.println("client disconnected");
  }
}

bool ValidateEthernet()
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