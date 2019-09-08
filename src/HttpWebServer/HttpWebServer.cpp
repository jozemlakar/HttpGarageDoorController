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

#include "HttpWebServer.h"
#include "../../config.h"

HttpWebServer::HttpWebServer(Server &server)
{
  this->_server = &server;
}

HttpWebServer::~HttpWebServer()
{
  this->stop();
}

void HttpWebServer::begin()
{
  this->stop();
  this->_server->begin();
}

void HttpWebServer::stop()
{
  if (this->_server != NULL) {
    // this->_server->stop(); // Not implemented
  }
}

void HttpWebServer::poll(Client &client, uint16_t (*request_handler)(Client &, const char *, const char *))
{
  LOGPRINTLN_TRACE("WS00");

  if (!client) {
    return;
  }

  long request_index = 0;
  long request_complete = false;
  bool current_line_empty = true;
  unsigned long connected_since = millis();
  char request[HTTP_REQUEST_BUFFER_SIZE];
  char c;

  LOGPRINTLN_TRACE("WS01");

  while (client.connected()) {
    LOGPRINTLN_TRACE("WS02");

    // Has request timed out?
    unsigned long time_now = millis();

    if (((time_now - connected_since) >= HTTP_REQUEST_TIMEOUT)) {
      LOGPRINTLN_ERROR("WS03");
      break;
    }

    // Is data available?

    if (!client.available()) {
      continue;
    }
    LOGPRINTLN_TRACE("WS04");

    c = client.read();

    if (request_index < (sizeof(request) - 1)) {
      request[request_index] = c;
      request_index++;
    }

    // Is request complete?
    request_complete = (current_line_empty && c == '\n');

    if (c == '\n') {
      current_line_empty = true;
    } else if (c != '\r') {
      current_line_empty = false;
    }

    if (!request_complete) {
      continue;
    }

    LOGPRINTLN_VERBOSE("WS05");
    request[request_index] = 0;

    // Start HTTP request processing
    // Extract and split HTTP request line
    LOGPRINTLN_DEBUG(request);
    LOGPRINTLN_VERBOSE("WS06");

    char *request_line_end = strstr(request, "\r\n");

    // get the content before first space
    char *request_method = strtok(request, " ");
    // get the content before second space
    char *request_url = strtok(NULL, " ");
    char *request_query = strstr(request_url, "?");

    size_t request_query_length = 0;

    if (request_query != NULL) {
      *request_query = 0; // Replace the ? with a NULL, request_url now excludes the request_query

      request_query++;
      request_query_length = strlen(request_query);

      if (request_query_length <= 0) {
        request_query_length = 0;
        request_query = NULL;
      }
    }

    strtoupper(request_method);
    size_t request_method_length = strlen(request_method);
    size_t request_url_length = strlen(request_url);

    // Is the request valid
    if ((request_method == NULL) || (request_url == NULL) ) {
      LOGPRINTLN_DEBUG("WS07");
      this->send_response(client, 400);
      break;
    }

    // Handle request
    LOGPRINT_VERBOSE(" ");
    LOGPRINT_VERBOSE(request_method);
    LOGPRINT_VERBOSE(" ");
    LOGPRINTLN_VERBOSE(request_url);

    uint16_t status = request_handler(client, request_method, request_url);

    if (status > 0) {
      this->send_response(client, status);
    }

    break;
  }

  if (client.connected()) {
    LOGPRINTLN_VERBOSE("WS12");
    delay(1);
    client.stop();
    LOGPRINTLN_VERBOSE("WS13");
  }
}

void HttpWebServer::send_response(Client &client, uint16_t status)
{
  HttpWebServer::send_response(client, status, NULL, 0);
}

void HttpWebServer::send_response(Client &client, uint16_t status, const uint8_t *response, size_t size)
{
  LOGPRINTLN_VERBOSE("WS14");
  LOGPRINTLN_VERBOSE(status);

  const char *status_description;

  switch (status) {
    case 200:
      status_description = "OK";
      break;

    case 202:
      status_description = "Accepted";
      break;

    case 204:
      status_description = "No Content";
      break;

    case 400:
      status_description = "Bad Request";
      break;

    case 401:
      status_description = "Unauthorized";
      break;

    case 403:
      status_description = "Forbidden";
      break;

    case 404:
      status_description = "Not Found";
      break;

    case 405:
      status_description = "Method Not Allowed";
      break;

    case 501:
    default:
      status = 501;
      status_description = "Not Implemented";
      break;
  }
  
  const char *success_string;

  if ((status >= 200) && (status <= 299)) {
    success_string = "true";
  } else {
    success_string = "false";
  }

  const char *head_format = "HTTP/1.1 %d %s\r\nContent-Type: application/json\r\nConnection: close\r\n\r\n";
  char head_buffer[strlen(head_format) + (-2 + 3) + (-2 + strlen(status_description)) + 1];
  sprintf(head_buffer, head_format, status, status_description);

  LOGPRINTLN_DEBUG(head_buffer);
  client.write((uint8_t *)head_buffer, sizeof(head_buffer) - 1);

  const char *body_format = "{\"r\":%d,\"s\":%s}";
  char body_buffer[strlen(body_format) + (-2 + 3) + (-2 + strlen(success_string) + 1)];
  sprintf(body_buffer, body_format, status, success_string);
  LOGPRINTLN_VERBOSE(body_buffer);

  if ((response == NULL) || (size <= 0)) {
    LOGPRINT_DEBUG(body_buffer);
    response = (uint8_t *)body_buffer;
    size = sizeof(body_buffer) - 1;
  } else {
    LOGPRINT_DEBUG("WS15");
  }

  LOGPRINTLN_DEBUG();
  LOGPRINTLN_DEBUG();
  client.write(response, size);
  LOGPRINTLN_DEBUG("WS16");
}