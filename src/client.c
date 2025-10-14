#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

static ssize_t sendAll(int socketFd, const void *buffer, size_t length) {
  const unsigned char *data = (const unsigned char *)buffer;
  size_t totalSent = 0;
  while (totalSent < length) {
    ssize_t sent = send(socketFd, data + totalSent, length - totalSent, 0);
    if (sent < 0) {
      if (errno == EINTR) continue;
      return -1;
    }
    if (sent == 0) {
      break;
    }
    totalSent += (size_t)sent;
  }
  return (ssize_t)totalSent;
}

static int connectToServer(const char *host, uint16_t port) {
  int sockfd = -1;
  struct addrinfo hints;
  struct addrinfo *result = NULL;
  char portStr[16];
  snprintf(portStr, sizeof(portStr), "%u", (unsigned)port);

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;      // allow IPv4 or IPv6
  hints.ai_socktype = SOCK_STREAM;  // TCP

  int rc = getaddrinfo(host, portStr, &hints, &result);
  if (rc != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rc));
    return -1;
  }

  for (struct addrinfo *rp = result; rp != NULL; rp = rp->ai_next) {
    sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (sockfd < 0) {
      continue;
    }
    if (connect(sockfd, rp->ai_addr, rp->ai_addrlen) == 0) {
      break; // success
    }
    close(sockfd);
    sockfd = -1;
  }

  freeaddrinfo(result);
  return sockfd; // -1 if failed
}

int main(int argc, char **argv) {
  const char *host = "127.0.0.1";
  uint16_t port = 12345;

  if (argc >= 2) {
    host = argv[1];
  }
  if (argc >= 3) {
    long parsed = strtol(argv[2], NULL, 10);
    if (parsed <= 0 || parsed > 65535) {
      fprintf(stderr, "Invalid port: %s\n", argv[2]);
      return 1;
    }
    port = (uint16_t)parsed;
  }

  int sockfd = connectToServer(host, port);
  if (sockfd < 0) {
    fprintf(stderr, "Failed to connect to %s:%u\n", host, (unsigned)port);
    return 1;
  }

  printf("Connected to %s:%u. Type messages, Ctrl+D to quit.\n", host,
         (unsigned)port);

  unsigned char sendBuf[4096];
  unsigned char recvBuf[4096];

  while (true) {
    if (fgets((char *)sendBuf, sizeof(sendBuf), stdin) == NULL) {
      // EOF or error; exit loop
      break;
    }

    size_t toSend = strlen((char *)sendBuf);
    if (sendAll(sockfd, sendBuf, toSend) < 0) {
      perror("send");
      break;
    }

    size_t totalReceived = 0;
    while (totalReceived < toSend) {
      ssize_t received = recv(sockfd, recvBuf + totalReceived,
                              toSend - totalReceived, 0);
      if (received < 0) {
        if (errno == EINTR) continue;
        perror("recv");
        goto done;
      }
      if (received == 0) {
        fprintf(stderr, "Server closed connection.\n");
        goto done;
      }
      totalReceived += (size_t)received;
    }

    fwrite(recvBuf, 1, totalReceived, stdout);
    fflush(stdout);
  }

done:
  close(sockfd);
  return 0;
}
