/*
 * Copyright ©2025 Hal Perkins.  All rights reserved.  Permission is
 * hereby granted to students registered for University of Washington
 * CSE 333 for use solely during Winter Quarter 2025 for purposes of
 * the course.  No other use, copying, distribution, or modification
 * is permitted without prior written consent. Copyrights for
 * third-party components of this work must be honored.  Instructors
 * interested in reusing these course materials should contact the
 * author.
 */

#include <stdio.h>       // for snprintf()
#include <unistd.h>      // for close(), fcntl()
#include <sys/types.h>   // for socket(), getaddrinfo(), etc.
#include <sys/socket.h>  // for socket(), getaddrinfo(), etc.
#include <arpa/inet.h>   // for inet_ntop()
#include <netdb.h>       // for getaddrinfo()
#include <errno.h>       // for errno, used by strerror()
#include <string.h>      // for memset, strerror()
#include <iostream>      // for std::cerr, etc.

#include "./ServerSocket.h"

extern "C" {
  #include "libhw1/CSE333.h"
}

using std::string;

namespace hw4 {

#define BUF_SIZE 1024

ServerSocket::ServerSocket(uint16_t port) {
  port_ = port;
  listen_sock_fd_ = -1;
}

ServerSocket::~ServerSocket() {
  // Close the listening socket if it's not zero.  The rest of this
  // class will make sure to zero out the socket if it is closed
  // elsewhere.
  if (listen_sock_fd_ != -1)
    close(listen_sock_fd_);
  listen_sock_fd_ = -1;
}

bool ServerSocket::BindAndListen(int ai_family, int *const listen_fd) {
  // Use "getaddrinfo," "socket," "bind," and "listen" to
  // create a listening socket on port port_.  Return the
  // listening socket through the output parameter "listen_fd"
  // and set the ServerSocket data member "listen_sock_fd_"

  // STEP 1:

  struct addrinfo hints{};
  memset(&hints, 0, sizeof(hints));

  if (ai_family != AF_UNSPEC && ai_family != AF_INET && ai_family != AF_INET6) {
      return false;
  }

  hints.ai_family = ai_family;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;
  hints.ai_protocol = IPPROTO_TCP;

  std::string port_str = std::to_string(port_);
  struct addrinfo* result = nullptr;

  if (getaddrinfo(nullptr, port_str.c_str(), &hints, &result) != 0) {
      return false;
  }

  int server_socket = -1;

  for (struct addrinfo* addr = result; addr != nullptr; addr = addr->ai_next) {
      server_socket = socket(
        addr->ai_family, addr->ai_socktype, addr->ai_protocol);
      if (server_socket < 0) {
          continue;
      }
      int enable = 1;
      if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR,
            &enable, sizeof(enable)) != 0) {
          close(server_socket);
          server_socket = -1;
          continue;
      }
      if (bind(server_socket, addr->ai_addr, addr->ai_addrlen) == 0) {
          sock_family_ = addr->ai_family;
          break;
      }
      close(server_socket);
      server_socket = -1;
  }
  freeaddrinfo(result);

  if (server_socket == -1) {
      return false;
  }
  if (listen(server_socket, SOMAXCONN) != 0) {
      close(server_socket);
      return false;
  }
  listen_sock_fd_ = server_socket;
  *listen_fd = server_socket;
  return true;
}

bool ServerSocket::Accept(int *const accepted_fd,
                          string *const client_addr,
                          uint16_t *const client_port,
                          string *const client_dns_name,
                          string *const server_addr,
                          string *const server_dns_name) const {
  // STEP 2:

  sockaddr_storage client_info{};
  sockaddr* client_addr_ptr = reinterpret_cast<sockaddr*>(&client_info);
  socklen_t client_addr_len = sizeof(client_info);

  int client_socket = -1;

  while (true) {
      client_socket = accept(
        listen_sock_fd_, client_addr_ptr, &client_addr_len);

      if (client_socket < 0) {
          if (errno == EINTR || errno == EAGAIN) {
              continue;
          }
          return false;
      }
      break;
  }

  *accepted_fd = client_socket;

  if (client_addr_ptr->sa_family == AF_INET) {
      char ip_buffer[INET_ADDRSTRLEN];
      auto* ipv4_addr = reinterpret_cast<sockaddr_in*>(client_addr_ptr);
      inet_ntop(AF_INET, &(ipv4_addr->sin_addr), ip_buffer, sizeof(ip_buffer));
      *client_addr = std::string(ip_buffer);
      *client_port = ntohs(ipv4_addr->sin_port);

  } else if (client_addr_ptr->sa_family == AF_INET6) {
      char ip_buffer[INET6_ADDRSTRLEN];
      auto* ipv6_addr = reinterpret_cast<sockaddr_in6*>(client_addr_ptr);
      inet_ntop(AF_INET6, &(ipv6_addr->sin6_addr),
        ip_buffer, sizeof(ip_buffer));
      *client_addr = std::string(ip_buffer);
      *client_port = ntohs(ipv6_addr->sin6_port);
  }

  char dns_host[BUF_SIZE];
  if (getnameinfo(client_addr_ptr, client_addr_len,
        dns_host, BUF_SIZE, nullptr, 0, 0) == 0) {
      *client_dns_name = std::string(dns_host);
  }

  char server_ip_buffer[INET6_ADDRSTRLEN];
  char server_dns_buffer[BUF_SIZE];

  if (sock_family_ == AF_INET) {
      sockaddr_in server_addr_info{};
      socklen_t server_addr_len = sizeof(server_addr_info);

      getsockname(client_socket, reinterpret_cast<sockaddr*>
        (&server_addr_info), &server_addr_len);
      inet_ntop(AF_INET, &server_addr_info.sin_addr,
        server_ip_buffer, sizeof(server_ip_buffer));
      getnameinfo(reinterpret_cast<sockaddr*>
        (&server_addr_info), server_addr_len,
        server_dns_buffer, BUF_SIZE, nullptr, 0, 0);

  } else if (sock_family_ == AF_INET6) {
      sockaddr_in6 server_addr_info{};
      socklen_t server_addr_len = sizeof(server_addr_info);

      getsockname(client_socket, reinterpret_cast<sockaddr*>
        (&server_addr_info), &server_addr_len);
      inet_ntop(AF_INET6, &server_addr_info.sin6_addr,
        server_ip_buffer, sizeof(server_ip_buffer));
      getnameinfo(reinterpret_cast<sockaddr*>
        (&server_addr_info), server_addr_len,
                  server_dns_buffer, BUF_SIZE, nullptr, 0, 0);
  }

  *server_addr = std::string(server_ip_buffer);
  *server_dns_name = std::string(server_dns_buffer);

  return true;
}

}  // namespace hw4
