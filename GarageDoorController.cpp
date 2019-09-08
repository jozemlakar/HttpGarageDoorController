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

#include "GarageDoorController.h"

GarageDoorController::GarageDoorController()
{
  this->_sensorOpenDebouncer = new Bounce();
  this->_sensorClosedDebouncer = new Bounce();
  this->_lightInputDebouncer = new Bounce();
}

GarageDoorController::~GarageDoorController()
{
}

void GarageDoorController::setup()
{
  // Configure input pins and debouncing
  pinMode(SENSOR_OPEN_INPUT_PIN, INPUT_PULLUP);
  this->_sensorOpenDebouncer->attach(SENSOR_OPEN_INPUT_PIN);
  this->_sensorOpenDebouncer->interval(50);

  pinMode(SENSOR_CLOSED_INPUT_PIN, INPUT_PULLUP);
  this->_sensorClosedDebouncer->attach(SENSOR_CLOSED_INPUT_PIN);
  this->_sensorClosedDebouncer->interval(50);

  pinMode(LIGHT_INPUT_PIN, INPUT_PULLUP);
  this->_lightInputDebouncer->attach(LIGHT_INPUT_PIN);
  this->_lightInputDebouncer->interval(50);

  // Configure output pins
  pinMode(LED_OUTPUT_PIN, OUTPUT);
  digitalWrite(LED_OUTPUT_PIN, LED_BUILTIN_OFF);

  pinMode(DOOR_OUTPUT_OPEN_PIN, OUTPUT);
  pinMode(DOOR_OUTPUT_CLOSE_PIN, OUTPUT);
  pinMode(LIGHT_OUTPUT_PIN, OUTPUT);
}

void GarageDoorController::loop()
{
  // Handle debouncing
  LOGPRINTLN_TRACE("GDC01");
  this->_sensorOpenDebouncer->update();
  this->_sensorClosedDebouncer->update();
  this->_lightInputDebouncer->update();

  // Update states
  LOGPRINTLN_TRACE("GDC02");
  this->_monitorInputs();
}

void GarageDoorController::operateDoor(bool state)
{
  LOGPRINTLN_TRACE("GDC03");

  uint8_t pin = 0;
  uint8_t cycles = 0;
  uint8_t DOOR_OUTPUT_PIN = DOOR_OUTPUT_OPEN_PIN;

  if (state) {
    if (DOOR_OUTPUT_OPEN_PIN != DOOR_OUTPUT_CLOSE_PIN) {
      pin = DOOR_OUTPUT_OPEN_PIN;
      cycles = 1;

    } else if ((this->DoorState == DOORSTATE_CLOSED) || (this->DoorState == DOORSTATE_STOPPED_CLOSING) || (this->DoorState == DOORSTATE_UNKNOWN)) {
      pin = DOOR_OUTPUT_PIN;
      cycles = 1;

    } else if (this->DoorState == DOORSTATE_CLOSING) {
      pin = DOOR_OUTPUT_PIN;
      cycles = 2;

    } else if (this->DoorState == DOORSTATE_STOPPED_OPENING) {
      pin = DOOR_OUTPUT_PIN;
      cycles = 3;

    }

    this->DoorState = DOORSTATE_OPENING;

  } else {
    if (DOOR_OUTPUT_OPEN_PIN != DOOR_OUTPUT_CLOSE_PIN) {
      pin = DOOR_OUTPUT_CLOSE_PIN;
      cycles = 1;

    } else if ((this->DoorState == DOORSTATE_OPEN) || (this->DoorState == DOORSTATE_STOPPED_OPENING) || (this->DoorState == DOORSTATE_UNKNOWN)) {
      pin = DOOR_OUTPUT_PIN;
      cycles = 1;

    } else if (this->DoorState == DOORSTATE_OPENING) {
      pin = DOOR_OUTPUT_PIN;
      cycles = 2;

    } else if (this->DoorState == DOORSTATE_STOPPED_CLOSING) {
      pin = DOOR_OUTPUT_PIN;
      cycles = 3;

    }

    this->DoorState = DOORSTATE_CLOSING;
  }

  if ((pin > 0) && (cycles > 0)) {
    LOGPRINT_INFO("\nO:d:");
    LOGPRINTLN_INFO(GarageDoorController::stringFromDoorState(this->DoorState));
    this->_operateDoor(pin, cycles);
  }
}

void GarageDoorController::switchLight(bool state)
{
  LOGPRINTLN_TRACE("GDC04");
  this->LightOutput = (bool)digitalRead(LIGHT_OUTPUT_PIN);

  if (this->LightOutput != state) {
    LOGPRINT_DEBUG("\nO:l:");
    LOGPRINTLN_INFO(state);
    digitalWrite(LIGHT_OUTPUT_PIN, state);
  }
}

void GarageDoorController::getJsonStatus(char *const dest, size_t destSize)
{
  LOGPRINTLN_VERBOSE("GDC05");
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject &jsonRoot = jsonBuffer.createObject();

  jsonRoot["result"] = 200;
  jsonRoot["success"] = true;
  jsonRoot["message"] = "OK";
  jsonRoot["l-i"] = this->LightInput;
  jsonRoot["l-r"] = this->LightRequested;
  jsonRoot["l-s"] = this->LightState;
  jsonRoot["d-o"] = this->SensorOpen;
  jsonRoot["d-c"] = this->SensorClosed;
  jsonRoot["d-s"] = GarageDoorController::stringFromDoorState(this->DoorState);

  jsonRoot.printTo(dest, destSize);
}

uint16_t GarageDoorController::requestHandler(Client &client, const char *requestMethod, const char *requestUrl)
{
  LOGPRINTLN_VERBOSE("GDC06");

  if (strcmp(requestMethod, "GET") == 0) {
    if (strcasecmp(requestUrl, "/controller") == 0) {
      char jsonStatus[256];
      this->getJsonStatus(jsonStatus, sizeof(jsonStatus));
      HttpWebServer::send_response(client, 200, (uint8_t *)jsonStatus, strlen(jsonStatus));
      return 0;

    } else {
      return 404;
    }
  } else if (strcmp(requestMethod, "PUT") == 0) {
    if ((strcasecmp(requestUrl, "/controller/light/on") == 0) || (strcasecmp(requestUrl, "/controller/light/off") == 0)) {
      if (striendswith(requestUrl, "/on")) {
        this->LightRequested = true;
      } else if (striendswith(requestUrl, "off")) {
        this->LightRequested = false;
      }

      // Switch the light
      this->LightState = (this->LightInput || this->LightRequested);
      this->switchLight(this->LightState);
      return 202;

    } else if ((strcasecmp(requestUrl, "/controller/door/open") == 0) || (strcasecmp(requestUrl, "/controller/door/close") == 0)) {
      bool doorOpen = false;

      if (striendswith(requestUrl, "/open")) {
        doorOpen = true;
      } else if (striendswith(requestUrl, "/close")) {
        doorOpen = false;
      }

      // Operate the door
      this->operateDoor(doorOpen);
      return 202;

    } else {
      return 404;
    }
  } else {
    return 405;
  }
}

char *GarageDoorController::stringFromDoorState(enum DoorState doorState)
{
  LOGPRINTLN_TRACE("GDC07");

  switch (doorState) {
    case DOORSTATE_OPEN:
      return (char *)"o";

    case DOORSTATE_CLOSED:
      return (char *)"c";

    case DOORSTATE_OPENING:
      return (char *)"oing";

    case DOORSTATE_CLOSING:
      return (char *)"cing";

    case DOORSTATE_STOPPED_OPENING:
      return (char *)"s-o";

    case DOORSTATE_STOPPED_CLOSING:
      return (char *)"s-c";

    case DOORSTATE_UNKNOWN:
      return (char *)"u";

    default:
      return (char *)"u";
  }
}

void GarageDoorController::_monitorInputs()
{
  LOGPRINTLN_TRACE("GDC08");

  uint32_t timeNow = millis();
  static uint32_t lastCall = 0;

  if ((lastCall != 0) && (timeNow - lastCall) <= 500) {
    return;
  }

  lastCall = timeNow;

  // Check door sensor inputs
  LOGPRINTLN_TRACE("GDC09");
  this->SensorOpen = !(bool)this->_sensorOpenDebouncer->read();
  this->SensorClosed = !(bool)this->_sensorClosedDebouncer->read();

  if ((this->DoorTimeLastOperated == 0) || ((timeNow - this->DoorTimeLastOperated) >= DOOR_SENSOR_REACT_TIME)) {
    if (this->SensorOpen && !this->SensorClosed) {
      this->DoorState = DOORSTATE_OPEN;
    } else if (!this->SensorOpen && this->SensorClosed) {
      this->DoorState = DOORSTATE_CLOSED;
    } else if (this->SensorOpen && this->SensorClosed) {
      this->DoorState = DOORSTATE_UNKNOWN;
    } else if (!this->SensorOpen && !this->SensorClosed) {
      if ((this->DoorState != DOORSTATE_OPENING) && (this->DoorState != DOORSTATE_CLOSING) && (this->DoorState != DOORSTATE_STOPPED_OPENING) && (this->DoorState != DOORSTATE_STOPPED_CLOSING)) {
        if (this->DoorStateLastKnown == DOORSTATE_CLOSED) {
          this->DoorState = DOORSTATE_OPENING;
        } else if (this->DoorStateLastKnown == DOORSTATE_OPEN) {
          this->DoorState = DOORSTATE_CLOSING;
        } else {
          this->DoorState = DOORSTATE_UNKNOWN;
        }

        if ((this->DoorState == DOORSTATE_OPENING) || (this->DoorState == DOORSTATE_CLOSING)) {
          // Door must have been manually operated
          this->DoorTimeLastOperated = timeNow;
        }
      }
    }
  }

  if (this->DoorState != DOORSTATE_OPEN) {
    this->DoorTimeOpenSince = 0;
  }

  if ((this->DoorState == DOORSTATE_OPEN) || (this->DoorState == DOORSTATE_CLOSED)) {
    this->DoorTimeLastOperated = 0;
    this->DoorStateLastKnown = this->DoorState;

    if (this->DoorState == DOORSTATE_OPEN) {
      if (this->DoorTimeOpenSince == 0) {
        this->DoorTimeOpenSince = timeNow;
       } else if (DOOR_AUTO_CLOSE_TIME_SECONDS > 0 && (timeNow - this->DoorTimeOpenSince) >= DOOR_AUTO_CLOSE_TIME_SECONDS * 1000) {
        // Door was open, but should now be automatically closed
        operateDoor(false);
      }
    }

  } else if ((this->DoorState == DOORSTATE_OPENING) || (this->DoorState == DOORSTATE_CLOSING)) {
    if ((timeNow - this->DoorTimeLastOperated) >= DOOR_MAX_OPEN_CLOSE_TIME) {
      // Door was opening/closing, but has now exceeded max open/close time
      this->DoorTimeLastOperated = 0;

      if (this->DoorState == DOORSTATE_OPENING) {
        this->DoorState = DOORSTATE_STOPPED_OPENING;
      } else if (this->DoorState == DOORSTATE_CLOSING) {
        this->DoorState = DOORSTATE_STOPPED_CLOSING;
      }
    }
  }

  // Check light inputs, output if necessary
  LOGPRINTLN_TRACE("GDC10");
  this->LightInput = !(bool)this->_lightInputDebouncer->read();
  this->LightState = (this->LightInput || this->LightRequested);
  this->switchLight(this->LightState);

#if defined(LOG_LEVEL) && LOG_LEVEL >= 5
  char jsonStatus[256];
  this->getJsonStatus(jsonStatus, sizeof(jsonStatus));
  LOGPRINTLN_DEBUG(jsonStatus);
#endif
}

void GarageDoorController::_operateDoor(uint8_t pin, uint8_t cycles)
{
  LOGPRINTLN_VERBOSE("GDC10");

  for (uint8_t i = 0; i < cycles; i++) {
    if (i != 0) {
      delay(DOOR_OUTPUT_PULSE_DELAY_TIME);
//      delay(1250);
    }

    digitalWrite(pin, HIGH);
    delay(DOOR_OUTPUT_PULSE_TIME);
//    delay(400);
    digitalWrite(pin, LOW);
  }

  this->DoorTimeLastOperated = millis();
}
