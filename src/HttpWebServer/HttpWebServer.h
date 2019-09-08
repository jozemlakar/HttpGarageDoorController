#ifndef HTTPWEBSERVER_H
#define HTTPWEBSERVER_H

#include "../../compile.h"

#include <Arduino.h>
#include <Client.h>
#include <Server.h>

#include "../../src/Utilities/Utilities.h"


class HttpWebServer
{
  public:
    HttpWebServer(Server &server);
    ~HttpWebServer();

    void begin();
    void poll(Client &client, uint16_t (*request_handler)(Client &, const char *, const char *));
    void stop();

    static void send_response(Client &client, uint16_t status);
    static void send_response(Client &client, uint16_t status, const uint8_t *response, size_t length);

  private:
    Server *_server;
};

#endif // HTTPWEBSERVER_H