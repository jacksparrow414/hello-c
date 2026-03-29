#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../utils/socket_utils.h"

void say_hi_to_server()
{
    int sockfd = bind_client_socket_to_port_and_connect();
    char *message = "Hi, Server! This is Client.";
    write(sockfd, message, strlen(message) + 1);
}
