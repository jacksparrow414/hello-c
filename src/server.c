#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

static volatile sig_atomic_t keepRunning = 1;

static void handleSignal(int signum) {
  (void)signum;
  keepRunning = 0;
}

static int setReuseAddr(int socketFd) {
  int enable = 1;
  return setsockopt(socketFd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));
}

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

static int runServer(uint16_t port) {
  int serverFd = socket(AF_INET, SOCK_STREAM, 0);
  if (serverFd < 0) {
    perror("socket");
    return 1;
  }

  if (setReuseAddr(serverFd) < 0) {
    perror("setsockopt(SO_REUSEADDR)");
    close(serverFd);
    return 1;
  }

  struct sockaddr_in address;
  memset(&address, 0, sizeof(address));
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = htonl(INADDR_ANY);
  address.sin_port = htons(port);

  if (bind(serverFd, (struct sockaddr *)&address, sizeof(address)) < 0) {
    perror("bind");
    close(serverFd);
    return 1;
  }

  if (listen(serverFd, 16) < 0) {
    perror("listen");
    close(serverFd);
    return 1;
  }

  printf("Echo server listening on 0.0.0.0:%u\n", (unsigned)port);
  printf("Press Ctrl+C to stop.\n");

  while (keepRunning) {
    struct sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);
    int clientFd = accept(serverFd, (struct sockaddr *)&clientAddr, &clientLen);
    if (clientFd < 0) {
      if (errno == EINTR && !keepRunning) break;
      if (errno == EINTR) continue;
      perror("accept");
      break;
    }

    char clientIp[INET_ADDRSTRLEN];
    const char *ipStr = inet_ntop(AF_INET, &clientAddr.sin_addr, clientIp, sizeof(clientIp));
    if (ipStr == NULL) {
      strncpy(clientIp, "<unknown>", sizeof(clientIp));
      clientIp[sizeof(clientIp) - 1] = '\0';
    }
    unsigned short clientPort = ntohs(clientAddr.sin_port);
    printf("[+] Connection from %s:%u\n", clientIp, clientPort);

    unsigned char buffer[4096];
    while (keepRunning) {
      ssize_t received = recv(clientFd, buffer, sizeof(buffer), 0);
      if (received < 0) {
        if (errno == EINTR) continue;
        perror("recv");
        break;
      }
      if (received == 0) {
        // client closed
        break;
      }
      ssize_t sent = sendAll(clientFd, buffer, (size_t)received);
      if (sent < 0) {
        perror("send");
        break;
      }
    }

    close(clientFd);
    printf("[-] Disconnected %s:%u\n", clientIp, clientPort);
  }

  close(serverFd);
  printf("Server stopped.\n");
  return 0;
}

int main(int argc, char **argv) {
  signal(SIGINT, handleSignal);
  signal(SIGTERM, handleSignal);

  uint16_t port = 12345;
  if (argc >= 2) {
    long parsed = strtol(argv[1], NULL, 10);
    if (parsed <= 0 || parsed > 65535) {
      fprintf(stderr, "Invalid port: %s\n", argv[1]);
      return 1;
    }
    port = (uint16_t)parsed;
  }

  return runServer(port);
}
