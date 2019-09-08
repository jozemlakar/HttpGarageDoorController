/*
  MIT License

  Copyright (c) 2017 Warren Ashcroft

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

//#define CONTROLLER_MODEL "GARAGEDOOR"
//#define CONTROLLER_FIRMWARE_VERSION "2.0.0"

// ARDUINO LIBRARIES
#include <Arduino.h>

#include "compile.h"
//#include "MemoryFree.h"

#include <Ethernet.h>

//#include <EthernetClient.h>
#include <EthernetServer.h>

// INCLUDES
#include "config.h"
#include "secret.h"

#include "GarageDoorController.h"
GarageDoorController gController;
#include "src/EthernetHelper/EthernetHelper.h"
#include "src/HttpWebServer/HttpWebServer.h"

// GLOBAL VARIABLES
EthernetHelper gEthernetHelper = EthernetHelper();
EthernetServer gServer(HTTP_SERVER_PORT);

HttpWebServer gHttpWebServer(gServer); // , HTTP_SERVER_PORT, HTTP_REQUEST_TIMEOUT, HTTP_REQUEST_BUFFER_SIZE

// SETUP
void setup()
{
#if LOG_LEVEL > 0
  delay(5000); // Allow time for serial monitor connection

  // Open serial port for debug
  Serial.begin(115200);
  Serial.println("Started Serial Debug");
  // Serial.println(freeMemory());
#endif

  LOGPRINTLN_TRACE("L0000");
  gEthernetHelper.enable_led(LED_BUILTIN, LED_BUILTIN_ON, LED_BUILTIN_OFF, false);

  // Setup Controller
  gController.setup();
  LOGPRINT_TRACE("L02 ");
  // Serial.println(freeMemory());
}


// METHODS
bool connectNetwork()
{
  if (gEthernetHelper.is_connected()) {
    return true;
  }
  LOGPRINTLN_DEBUG(Ethernet.linkStatus() != LinkOFF);
  LOGPRINTLN_DEBUG(Ethernet.localIP() != INADDR_NONE);
  gHttpWebServer.stop();

  LOGPRINTLN_DEBUG("L0004");
  if (!gEthernetHelper.connect()) {
    return false;
  }

  // Start HTTP web server
  LOGPRINTLN_DEBUG("L0005");
  gHttpWebServer.begin();
  LOGPRINTLN_DEBUG("L0006");
}

void getJsonDeviceInfo(char *const jsonDeviceInfo, size_t jsonDeviceInfoSize)
{
  LOGPRINTLN_VERBOSE("L0008");
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject &jsonRoot = jsonBuffer.createObject();

  char ip[16] = {0};
  gEthernetHelper.get_client_ip(ip, sizeof(ip));

  // jsonRoot["mdns"] = MDNS_NAME;
  // jsonRoot["name"] = FRIENDLY_NAME;
//  jsonRoot["model"] = CONTROLLER_MODEL;
//  jsonRoot["firmware"] = CONTROLLER_FIRMWARE_VERSION;
  jsonRoot["ip"] = ip;

  jsonRoot.printTo(jsonDeviceInfo, jsonDeviceInfoSize);
}

uint16_t requestHandler(Client &client, const char *requestMethod, const char *requestUrl)
{
  bool currentState = ((bool)digitalRead(LED_BUILTIN) == HIGH);

  digitalWrite(LED_BUILTIN, (currentState ? LOW : HIGH));
  
  if ((strcmp(requestMethod, "GET") == 0) && (strcasecmp(requestUrl, "/device") == 0)) {
    char jsonDeviceInfo[256];
    getJsonDeviceInfo(jsonDeviceInfo, sizeof(jsonDeviceInfo));
    HttpWebServer::send_response(client, 200, (uint8_t *)jsonDeviceInfo, strlen(jsonDeviceInfo));
    return 0;
  }

  return gController.requestHandler(client, requestMethod, requestUrl);
}

// MAIN LOOP
void loop()
{
  LOGPRINT_TRACE("L01 ");
  // Serial.println(freeMemory());

  // Check/reattempt connection
  bool connected = connectNetwork();
  LOGPRINT_TRACE("L09 ");
  // Serial.println(freeMemory());
  
  // Loop Controller
  gController.loop();
  LOGPRINT_TRACE("L10 ");
  // Serial.println(freeMemory());
  
  if (connected) {  
    // Handle incoming HTTP/aREST requests
    LOGPRINTLN_TRACE("L11 ");
    // Serial.println(freeMemory());
    EthernetClient client = gServer.available();  
    // Serial.println(freeMemory());
    gHttpWebServer.poll(client, requestHandler);
    LOGPRINT_TRACE("L12 ");
    // Serial.println(freeMemory());
  }
}
