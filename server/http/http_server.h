#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <stdbool.h>

int http_server_start(int port);
void http_server_stop();

#endif // HTTP_SERVER_H
