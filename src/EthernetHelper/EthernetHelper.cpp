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

#include "EthernetHelper.h"

EthernetHelper::EthernetHelper()
{
  this->_reset();
}

EthernetHelper::~EthernetHelper()
{
}

void EthernetHelper::_reset()
{
  if (this->_led_enabled && !this->_led_blink_enabled) {
    digitalWrite(this->_led_pin, this->_led_off);
  }
}

void EthernetHelper::_led_blink()
{
  if (!this->_led_blink_enabled) {
    return;
  }

  unsigned long time_now = millis();

  if ((time_now - this->_led_last_blink) >= 4000) {
    bool currentState = ((bool)digitalRead(this->_led_pin) == this->_led_on);

    digitalWrite(this->_led_pin, (currentState ? this->_led_off : this->_led_on));
    delay(50);
    digitalWrite(this->_led_pin, (currentState ? this->_led_on : this->_led_off));

    this->_led_last_blink = time_now;
  }
}

void EthernetHelper::enable_led(uint8_t led_pin, uint8_t led_on, uint8_t led_off, bool blink)
{
  this->_led_pin = led_pin;
  this->_led_on = led_on;
  this->_led_off = led_off;
  this->_led_enabled = true;
  this->_led_blink_enabled = blink;
}

bool EthernetHelper::is_connected()
{
  this->_led_blink();
  return ((Ethernet.linkStatus() != LinkOFF) && (Ethernet.localIP() != INADDR_NONE));
}

bool EthernetHelper::is_connecting()
{
  return ((Ethernet.linkStatus() != LinkOFF) || (Ethernet.localIP() == INADDR_NONE));
}

void EthernetHelper::disconnect()
{
  this->_reset();
}

bool EthernetHelper::connect()
{
  LOGPRINTLN_TRACE("EH0000");

  if (this->is_connected()) {
    return true;
  }

  LOGPRINTLN_TRACE("EH0001");
  // Check for the presence of the network hardware
  LOGPRINTLN_TRACE("EH0002");

  byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
  IPAddress ip(192, 168, 0, 12);

  Ethernet.begin(mac, ip);
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    LOGPRINTLN_ERROR("E-EH0003");
    return false;
  }
  else if (Ethernet.hardwareStatus() == EthernetW5100) {
    LOGPRINTLN_TRACE("EH0004-W5100");
  }
  else if (Ethernet.hardwareStatus() == EthernetW5200) {
    LOGPRINTLN_TRACE("EH0004-W5200");
  }
  else if (Ethernet.hardwareStatus() == EthernetW5500) {
    LOGPRINTLN_TRACE("EH0004-W5500");
  }
  // Connect to network
  LOGPRINTLN_TRACE("EH0005");

  if (Ethernet.linkStatus() == Unknown) {
    // Link status unknown. Link status detection is only available with W5200 and W5500.
    LOGPRINTLN_TRACE("EH0007-Unk");
  }
  else if (Ethernet.linkStatus() == LinkON) {
    LOGPRINTLN_TRACE("EH0007-On");  }
  else if (Ethernet.linkStatus() == LinkOFF) {
    LOGPRINTLN_TRACE("EH0007-Off");
    return false;
  }

  LOGPRINTLN_TRACE("EH0008");
  LOGPRINTLN_TRACE();

  LOGPRINT_INFO("EH0009");
  LOGPRINTLN_INFO(Ethernet.localIP());

  LOGPRINT_TRACE("EH0010");
  LOGPRINTLN_TRACE(Ethernet.gatewayIP());

  LOGPRINTLN_TRACE();

  if (this->_led_enabled && !this->_led_blink_enabled) {
    digitalWrite(this->_led_pin, this->_led_on);
  }

  return true;
}

char *EthernetHelper::get_client_ip(char *dest, size_t dest_size)
{
  return this->_get_ip_address(Ethernet.localIP(), dest, dest_size);
}

char *EthernetHelper::get_gateway_ip(char *dest, size_t dest_size)
{
  return this->_get_ip_address(Ethernet.gatewayIP(), dest, dest_size);
}

char *EthernetHelper::_get_ip_address(const IPAddress addr, char *dest, size_t dest_size)
{
  if ((dest == NULL) || (dest_size < (15 + 1))) {
    return NULL;
  }

  uint8_t bytes[4];
  bytes[0] = addr & 0xFF;
  bytes[1] = (addr >> 8) & 0xFF;
  bytes[2] = (addr >> 16) & 0xFF;
  bytes[3] = (addr >> 24) & 0xFF;

  snprintf(dest, dest_size, "%u.%u.%u.%u", bytes[0], bytes[1], bytes[2], bytes[3]);
  return dest;
}
