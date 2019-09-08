#ifndef ETHERNETHELPER_H
#define ETHERNETHELPER_H

#include "../../compile.h"

#include <Arduino.h>

#include <Ethernet.h>

#include "../../src/Utilities/Utilities.h"

class EthernetHelper
{
  public:
    EthernetHelper();
    ~EthernetHelper();

    bool connect();
    void disconnect();

    bool is_connecting();
    bool is_connected();

    char *get_client_ip(char *dest, size_t dest_size);
    char *get_gateway_ip(char *dest, size_t dest_size);

    void enable_led(uint8_t led_pin, uint8_t led_on, uint8_t led_off, bool blink);

  private:

    uint8_t _led_pin = 0;
    uint8_t _led_on = 0;
    uint8_t _led_off = 0;
    bool _led_enabled = false;
    bool _led_blink_enabled = false;
    unsigned long _led_last_blink = 0;

    void _reset();
    void _led_blink();
    char *_get_ip_address(const IPAddress addr, char *dest, size_t dest_size);

};

#endif // ETHERNETHELPER_H