#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../utils/socket_utils.h"

#define CLIENT_MESSAGE "Hi, Server! This is Client."
void say_hi_to_server()
{
    int sockfd = bind_client_socket_to_port_and_connect();
    write(sockfd, CLIENT_MESSAGE, strlen(CLIENT_MESSAGE) + 1);
    read_from_client(sockfd);
    close(sockfd);
}
