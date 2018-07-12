#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h> //socket
#include <arpa/inet.h> //inet_addr
#include <netinet/in.h> //arpa/inet windows equivalent
#include <netdb.h> //hostent
#include <unistd.h> // socket close
#include <sys/ioctl.h>
#include <linux/sockios.h>
#include <stdint.h>
#include <boost/format.hpp>

static const int SERVER_PORT = 1024;
static const uint32_t MESSAGES_SIZE = 6;
static uint8_t PRESET_REQUEST_1[MESSAGES_SIZE] = {0xAA, 0x00, 0x04, 0x00, 0x04, 0x55};
static uint8_t PRESET_RESPONSE_1[MESSAGES_SIZE] = {0xBB, 0x00, 0x05, 0x00, 0x20, 0x55};
static uint8_t PRESET_REQUEST_2[MESSAGES_SIZE] = {0xAA, 0x00, 0x03, 0x00, 0x00, 0x55};
static uint8_t PRESET_RESPONSE_2[MESSAGES_SIZE] = {0xBB, 0x00, 0x08, 0x00, 0x01, 0x55};

std::string bytearray_hexdump(uint8_t* array, uint32_t array_size) {
  std::string result;
  for (uint32_t i=0; i<array_size; i++)
    result.append( str(boost::format("%02X ")%(int)array[i]) );
  return result;
};

int main(int argc, char **argv) {

  int socket_desc , client_sock , c , *new_sock;
  struct sockaddr_in server , client;

  //Create socket
  socket_desc = socket(AF_INET , SOCK_STREAM , 0);
  if (socket_desc == -1) {
    std::cerr << "Could not create socket!" << std::endl;
    return -1;
  }
  int enable = 1;
  if (setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
    std::cerr << "setsockopt(SO_REUSEADDR) failed while creating socket!" << std::endl;
    return -1;
  }
  std::cout << "Socket created successfully!" << std::endl;

  //Prepare the sockaddr_in structure
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons( SERVER_PORT );

  //Bind
  if( bind(socket_desc, (struct sockaddr *)&server, sizeof(server) ) < 0) {
    std::cerr << "Socket bind failed!" << std::endl;
    return -1;
  }
  std::cout << "Socket bind done!" << std::endl;

  //Listen
  listen(socket_desc, 3);

  //accept connection from an incoming client
  std::cout << "Waiting for incoming connections..." << std::endl;
  c = sizeof(struct sockaddr_in);
  while ( (client_sock = (accept(socket_desc,(struct sockaddr*)&client,(socklen_t*)&c))) ) {
    std::cout << "Client connection accepted!" << std::endl;
    new_sock = (int*)malloc(1);
    *new_sock = client_sock;

    //prepare recv buffer
    int read_size;
    uint8_t recv_buffer[MESSAGES_SIZE]; // preparing a receive buffer of the size of the message handled
    memset(recv_buffer, 0, sizeof(recv_buffer));
    // receive data from client
    while ( (read_size=recv(*new_sock,&recv_buffer,sizeof(recv_buffer),0)) > 0 ) {
      // log received data
      std::cout << "Received data from client -> " << bytearray_hexdump(recv_buffer,read_size) << std::endl;
      // answer back to preset requests with preset responses
      if ( memcmp(recv_buffer,PRESET_REQUEST_1,MESSAGES_SIZE)==0 ) {
        if( write(*new_sock, PRESET_RESPONSE_1, MESSAGES_SIZE) != MESSAGES_SIZE )
          std::cerr << "Error sending back response 1!" << std::endl;
        else
          std::cout << "Sent back response -> " << bytearray_hexdump(PRESET_RESPONSE_1, MESSAGES_SIZE) << std::endl;


      } else if ( memcmp(recv_buffer,PRESET_REQUEST_2,MESSAGES_SIZE)==0 ) {
        if( write(*new_sock, PRESET_RESPONSE_2, MESSAGES_SIZE) != MESSAGES_SIZE )
          std::cerr << "Error sending back response 2!" << std::endl;
        else
          std::cout << "Sent back response -> " << bytearray_hexdump(PRESET_RESPONSE_2, MESSAGES_SIZE) << std::endl;
      }
      // reset recv_buf
      memset(recv_buffer, 0, sizeof(recv_buffer));
    }
    // check if client is still alive
    if(read_size == 0) {
      std::cout << "Client disconnected" << std::endl;
      fflush(stdout);
    } else if(read_size == -1) {
      std::cerr << "Socket recv error! Closing conenction to client with errno = " << strerror(errno) << std::endl;
    }
  }
  if (client_sock < 0) {
    std::cerr << "Socket accept failed!" << std::endl;
    return -1;
  }
};
