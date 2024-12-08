#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/epoll.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ifaddrs.h>

#include "request.h"

typedef struct sockaddr_in sockaddr_in;
typedef struct sockaddr sockaddr;
typedef struct epoll_event epoll_event;

const int port_num = 2255;
const int max_connections = 1;

int main(void)
{
    int status;

    int socket_fd;

    // Create socket
    {
	socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_fd == -1)
	{
	    perror("socket");
	    exit(1);
	}

	const int enable = 1;
	if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
	{
	    perror("setsockopt - SO_REUSEADDR");
	    exit(1);
	}
    }

    // Bind socket
    {
	sockaddr_in server_addr = {0};
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(port_num);

	status = bind(socket_fd, (sockaddr*)&server_addr, sizeof(server_addr));
	if (status == -1)
	{
	    perror("bind");
	    exit(1);
	}
    }

    // Listen for connections
    {
	status = listen(socket_fd, max_connections);
	if (status == -1)
	{
	    perror("listen");
	    exit(1);
	}
    }

    int connection_fd;
    // Get connection
    {
	sockaddr_in client_addr = {0};
	socklen_t client_len = sizeof(client_addr);
	connection_fd = accept(socket_fd, (sockaddr*)&client_addr, &client_len);
	if (connection_fd == -1)
	{
	    perror("accept");
	    exit(1);
	}

	// set to nonblocking
	status = fcntl(connection_fd, F_SETFL, fcntl(connection_fd, F_GETFL, 0) | O_NONBLOCK);
	if (status == -1)
	{
	    perror("fcntl");
	    exit(1);
	}

    }

    int epoll_fd;
    // epoll creation
    {
	epoll_fd = epoll_create1(0);
	if (epoll_fd == -1)
	{
	    perror("epoll_create1");
	    exit(1);
	}
	
	epoll_event event;
	event.events = EPOLLIN | EPOLLET;
	event.data.fd = connection_fd;
	
	status = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, connection_fd, &event);
	if (status == -1)
	{
	    perror("epoll_ctl");
	    exit(1);
	}
    }

    // Variables for noise
    int noise_threshold = 0;

    int socket_amt = 0;
    int *sockets;

    // epoll events
    const int events_amt = 5;
    epoll_event events[events_amt];

    // reading buffer
    char **read_buffers; // one per connection
    const int read_buf_size = 255;
    char read_buf[read_buf_size];
    int read_offset = 0;

    // request buffer
    char **request_data_buffers; // one per connection
    char request_data_buf[253];
    int start_of_request = 0;

    // packet send buffer
    const int packet_send_buf_size = 255;
    char packet_send_buf[packet_send_buf_size];

    while (1)
    {

	// NOTE: do gpio stuff here

	
	// Networking
	int event_count = epoll_wait(epoll_fd, events, events_amt, -1);
	if (event_count == -1)
	{
	    perror("epoll_wait");
	    exit(1);
	}

	
	for (int event_index = 0; event_index < event_count; ++event_index)
	{
	    int fd = events[event_index].data.fd;

	    while (1)
	    {

		int bytes_read = read(fd, read_buf, read_buf_size - read_offset);

		// stop if it is empty
		if (bytes_read == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
		{
		    break;
		}
		
		bytes_read += read_offset;
		read_offset = 0;
		
		int bytes_parsed = 0;
		
		while (bytes_parsed < bytes_read)
		{
		    // load request
		    uint8_t request_size = read_buf[start_of_request];
		    uint8_t request_type = read_buf[start_of_request + 1];
		    uint8_t request_options = read_buf[start_of_request + 2];
		    uint8_t request_data_size = request_size - 3;
		    request_data_buf[request_data_size] = '\0';
		    memcpy(request_data_buf, &read_buf[start_of_request + 3], request_size - 3);
		    
		    switch (request_type)
		    {
			case REQUEST_TYPE_EXIT: 
			{
			    // send ack
			    uint8_t packet_size;
			    encode_packet(packet_send_buf, &packet_size, REQUEST_TYPE_EXIT, REQUEST_OPTION_GLOBAL_ACK, "", 0);
			    status = write(fd, packet_send_buf, packet_size);
			    if (status == -1)
			    {
				perror("write");
				exit(1);
			    }

			    goto CLEANUP;
			}
			
			case REQUEST_TYPE_GET_THRESHOLD:
			{
			    // send ack
			    uint8_t packet_size;
			    encode_packet(packet_send_buf, &packet_size, REQUEST_TYPE_GET_THRESHOLD, REQUEST_OPTION_GLOBAL_ACK, "", 0);
			    status = write(fd, packet_send_buf, packet_size);
			    if (status == -1)
			    {
				perror("write");
				exit(1);
			    }

			    char num_buf[16];
			    uint8_t num_buf_size = snprintf(num_buf, 16, "%d", noise_threshold);
			    encode_packet(packet_send_buf, &packet_size, REQUEST_TYPE_GET_THRESHOLD, REQUEST_OPTION_GET_THRESHOLD_RETURN, num_buf, num_buf_size);
			    status = write(fd, packet_send_buf, packet_size);
			    if (status == -1)
			    {
				perror("write");
				exit(1);
			    }
			    
			    break;
			}
			
			case REQUEST_TYPE_SET_THRESHOLD:
			{
			    // send ack
			    uint8_t packet_size;
			    encode_packet(packet_send_buf, &packet_size, REQUEST_TYPE_SET_THRESHOLD, REQUEST_OPTION_GLOBAL_ACK, "", 0);
			    status = write(fd, packet_send_buf, packet_size);
			    if (status == -1)
			    {
				perror("write");
				exit(1);
			    }

			    noise_threshold = atoi(request_data_buf);
			    break;
			}
			
			case REQUEST_TYPE_GET_TIME_OF_LAST_ALARM:
			{
			    printf("got get time\n");
			    break;
			}

			case REQUEST_TYPE_TEST_PRINT:
			{
			    printf("%s\n", request_data_buf);

			    // send ack
			    uint8_t packet_size;
			    encode_packet(packet_send_buf, &packet_size, REQUEST_TYPE_TEST_PRINT, REQUEST_OPTION_GLOBAL_ACK, "", 0);
			    status = write(fd, packet_send_buf, packet_size);
			    if (status == -1)
			    {
				perror("write");
				exit(1);
			    }
			    break;
			}
			
		    }
		    
		    int new_bytes_parsed = bytes_parsed + request_size;
		    
		    // handle moving to next request
		    if (new_bytes_parsed < bytes_read)
		    {
			uint8_t start_of_next_request = start_of_request + request_size;
			uint8_t next_request_size = read_buf[start_of_next_request];
			
			if (new_bytes_parsed + next_request_size > bytes_read)
			{
			    // Move incomplete data to beginning, and reset the variables
			    // for next loop
			    
			    // move stuff to beginning
			    for (int i = 0; i < bytes_read - new_bytes_parsed; ++i)
			    {
				read_buf[i] = read_buf[start_of_next_request + i];
			    }
			    
			    // mark bytes moved as parsed
			    bytes_parsed = bytes_read;
			    
			    // make sure next loop is ready
			    start_of_request = 0;
			    read_offset = bytes_read - new_bytes_parsed;
			}
			else
			{
			    // Move to next request that fits
			    start_of_request = start_of_next_request;
			    bytes_parsed += request_size;
			}
		    }
		    else
		    {
			start_of_request = 0;
			bytes_parsed += request_size;
		    }
		}
		
	    }
	}
    }

  CLEANUP:

    // Cleanup

    status = close(epoll_fd);
    if (status == -1)
    {
	perror("close");
	exit(1);
    }

    status = close(connection_fd);
    if (status == -1)
    {
	perror("close");
	exit(1);
    }

    status = close(socket_fd);
    if (status == -1)
    {
	perror("close");
	exit(1);
    }

    return 0;
}
