#include "pgch.h"

unsigned long long mcounter = 0; //Counter of the allocated bytes
int fd; //file descriptor for the touch events

int print_addresses(const int domain) {
	int s;
	struct ifconf ifconf;
	struct ifreq ifr[50];
	int ifs;
	int i;

	s = socket(domain, SOCK_STREAM, 0);
	if (s < 0) {
		printf("Error in socket\n");
		return 0;
	}

	ifconf.ifc_buf = (char *) ifr;
	ifconf.ifc_len = sizeof ifr;

	if (ioctl(s, SIOCGIFCONF, &ifconf) == -1) {
		printf("Error in ioctl\n");
		return 0;
	}

	ifs = ifconf.ifc_len / sizeof(ifr[0]);
	printf("interfaces = %d:\n", ifs);
	for (i = 0; i < ifs; i++) {
		char ip[INET_ADDRSTRLEN];
		struct sockaddr_in *s_in = (struct sockaddr_in *) &ifr[i].ifr_addr;

		if (!inet_ntop(domain, &s_in->sin_addr, ip, sizeof(ip))) {
			printf("Error in inet_ntop\n");
			return 0;
		}

		printf("%s - %s\n", ifr[i].ifr_name, ip);
	}

	close(s);

	return 1;
}

int open_fd(char* dev) {
	int fd;
	int version;
	fd = open(dev, O_RDWR);
	if (fd < 0) {
		printf("could not open %s, %s\n", dev, strerror(errno));
		return fd;
	}
	if (ioctl(fd, EVIOCGVERSION, &version)) {
		printf("ioctl failed for %s, %s\n", dev, strerror(errno));
		return 0;
	}

	return fd;
}

int send_event(int fd, int type, int code, int value) {
	struct input_event event;
	int ret;

	memset(&event, 0, sizeof(event));
	event.type = type;
	event.code = code;
	event.value = value;

	ret = write(fd, &event, sizeof(event));
	if (ret < (ssize_t) sizeof(event)) {
		printf("write event failed, %s\n", strerror(errno));
		return 0;
	}

	return 1;
}

int send_touch(int fd, int x, int y) {
	int ret = 1;

	ret = ret & send_event(fd, 3, 57, 1522);
	ret = ret & send_event(fd, 3, 53, x);
	ret = ret & send_event(fd, 3, 54, y);
	ret = ret & send_event(fd, 1, 330, 1);
	ret = ret & send_event(fd, 0, 0, 0);
	ret = ret & send_event(fd, 3, 57, 0xffffffff);
	ret = ret & send_event(fd, 1, 330, 0);
	ret = ret & send_event(fd, 0, 0, 0);

	return ret;
}

int hangle_touch_msg(char* msg, char** msg_out) {
	int x, y;

	memcpy(&x, msg, 4);
	memcpy(&y, msg + 4, 4);

	printf("Sending touch event x:%d, y:%d\n", x, y);

	send_touch(fd, x, y);

	*msg_out = m_malloc(4);
	sprintf(*msg_out, "%s", "OK!");

	return 4;
}

int hangle_swipe_msg(char* msg, int len, char** msg_out) {
	uint type, code, value;
	int i = 0;
	//hexdump(msg, len);

	for (i = 0; i < len / 12; i++) {
		memcpy(&type, msg + (i * 12), 4);
		memcpy(&code, msg + (i * 12) + 4, 4);
		memcpy(&value, msg + (i * 12) + 8, 4);

//		printf("event: %x %x %x\n", type, code, value);
		usleep(5000);

		send_event(fd, type, code, value);
	}

	*msg_out = m_malloc(4);
	sprintf(*msg_out, "%s", "OK!");

	return 4;
}

int capture_screen(char** msg_out) {
	FILE *pipe_fp;
	char readbuf[4096];
	int bsize = sizeof(readbuf);
	int c;
	int bcount = 0;
	int fd;

	/* Create one way pipe line with call to popen() */
	if ((pipe_fp = popen("/system/bin/screencap -p", "r")) == NULL) {
		printf("Error in popen\n");
		return -1;
	}

	/* Processing loop */
	fd = fileno(pipe_fp);
	do {
		c = read(fd, readbuf, bsize);
		if (c == 0) {
			break;
		}

		//Allocate memory for new portion of data
		if (bcount > 0) {
			*msg_out = m_realloc(*msg_out, bcount, bcount + c);
		} else {
			*msg_out = m_malloc(c);
		}

		//Copy new data into result buffer
		memcpy(*msg_out + bcount, readbuf, c);
		bcount = bcount + c;
	} while (1);

	pclose(pipe_fp);

	printf("got png bytes: %d\n", bcount);
//	hexdump(*msg_out, 256);

	return bcount;
}

void hexdump(void *buf, int len) {
	int i = 0;
	unsigned char buff[17];
	unsigned char *pc = (unsigned char*) buf;

	if(len == 0) return;

	// Process every byte in the data.
	for (i = 0; i < len; i++) {
		// Multiple of 16 means new line (with line offset).

		if ((i % 16) == 0) {
			// Just don't print ASCII for the zero line.
			if (i != 0)
				printf("  %s\n", buff);

			// Output the offset.
			printf("  %04x ", i);
		}

		// Now the hex code for the specific character.
		printf(" %02x", pc[i]);

		// And store a printable ASCII character for later.
		if ((pc[i] < 0x20) || (pc[i] > 0x7e))
			buff[i % 16] = '.';
		else
			buff[i % 16] = pc[i];
		buff[(i % 16) + 1] = '\0';
	}

	// Pad out last line if not exactly 16 characters.
	while ((i % 16) != 0) {
		printf("   ");
		i++;
	}

	// And print the final ASCII bit.
	printf("  %s\n", buff);
}

int handle_message(int msgId, int type, char* msg, int len, char** msg_out) {
	int res = 0;
	printf("Got message, msgId: %d, type: %d, len: %d\n", msgId, type, len);
	//hexdump(msg, len);

	switch (type) {
	case MESSAGE_ANDROID_TYPE_TEST:
		printf("Got TEST, msgId: %d\n", msgId);
		break;
	case MESSAGE_ANDROID_SCREEN_CAP:
		res = capture_screen(msg_out);
		break;
	case MESSAGE_ANDROID_SEND_TOUCH:
		res = hangle_touch_msg(msg, msg_out);
		break;
	case MESSAGE_ANDROID_SEND_SWIPE:
		res = hangle_swipe_msg(msg, len, msg_out);
		break;
	default:
		break;
	}

	printf("got response bytes: %d\n", res);
//	hexdump(*msg_out, res > 256 ? 256 : res);

	return res;
}

char* m_malloc(size_t size) {
	char* buf = malloc(size);
	if (buf != NULL) {
		mcounter += size;
	}

	return buf;
}

char* m_realloc(char* ptr, size_t old_size, size_t new_size) {
	char* buf = realloc(ptr, new_size);
	if (buf != NULL) {
		mcounter = mcounter - old_size + new_size;
	}

	return buf;
}

void m_free(char* ptr, size_t size) {
	free(ptr);
	mcounter -= size;

	printf("TEST FREE: %llu\n", mcounter);
}

int run(int port) {
	int socket_desc, client_sock, c, read_size;
	struct sockaddr_in server;

	struct sockaddr_in client; /* client addr */
	char *hostaddrp; /* dotted decimal host addr string */

	int b, blen, br, rlen;
	char* msgbuf;
	int msgId;
	int type;
	char* msg_out = 0;
	int exit = 0;

	//Create socket
	socket_desc = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_desc == -1) {
		printf("Could not create socket\n");
	}
	printf("Socket created, on port: %d\n", port);

	//Prepare the sockaddr_in structure
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(port);

	//Bind
	if (bind(socket_desc, (struct sockaddr *) &server, sizeof(server)) < 0) {
		//print the error message
		printf("Error. Bind failed\n");
		return 1;
	}
	printf("bind done\n");

	//Listen
	listen(socket_desc, 3);

	//Always wait for incoming connections
	while (!exit) {
		//Accept and incoming connection
		printf("Waiting for incoming connections...\n");
		c = sizeof(struct sockaddr_in);

		//accept connection from an incoming client
		client_sock = accept(socket_desc, (struct sockaddr *) &client,
				(socklen_t*) &c);
		if (client_sock < 0) {
			printf("Error. Accept failed\n");
			continue;
		}

		hostaddrp = inet_ntoa(client.sin_addr);
		if (hostaddrp == NULL) {
			printf("Error on inet_ntoa\n");
		}
		printf("Got connection from: %s\n", hostaddrp);

		//Receive a message from client
		while (1) {
			//Read length of the message
			b = read(client_sock, &blen, sizeof(blen));
			if (b != 4) {
				printf("Can't read message length, closing socket\n");
				break;
			}

			printf("Expecting message with length: %d\n", blen);
			msgbuf = m_malloc(blen);

			//Read message
			br = 0;
			while (br < blen) {
				b = read(client_sock, msgbuf + br, blen - br);
				br += b;
				printf("b=%d, got total %d out of %d bytes\n", b, br, blen);
				if (b <= 0) {
					break;
				}
			}

			//Check if we did not manage to read message
			if (br < blen) {
				printf("Can't read message, closing socket, b=%d, error: %s\n",
						b, strerror(errno));
				break;
			}

			memcpy(&msgId, msgbuf, 4);
			memcpy(&type, msgbuf + 4, 4);

			if (type == MESSAGE_ANDROID_EXIT) {
				printf("Got exit message! Closing...\n");
				exit = 1;
				break;
			}

			rlen = handle_message(msgId, type, msgbuf + 8, blen - 8, &msg_out);

			//Release memory for the original message
			m_free(msgbuf, blen);

			printf("run, msg_out, msgId: %d, type: %d, len: %d\n", msgId, type, rlen);
			printf("run, msg_out bytes: %d\n", rlen);
			hexdump(msg_out, rlen > 256 ? 256 : rlen);

			rlen += 8; //to account for msgId and type

			//Send response back
			if (rlen > 0) {
				send(client_sock, &rlen, 4, 0);
				send(client_sock, &msgId, 4, 0);
				send(client_sock, &type, 4, 0);
				send(client_sock, msg_out, rlen - 8, 0);

				m_free(msg_out, rlen - 8);
			}

		}

		close(client_sock);
		printf("Client disconnected\n");
	}

	return 0;
}

int print_dt(struct timeval* t0, struct timeval* t1) {
	printf("dt: %ld microseconds\n",
			((t1->tv_sec - t0->tv_sec) * 1000000L + (t1->tv_usec - t0->tv_usec)));
	return 1;
}

int main(int argc, char **argv) {
	print_addresses(AF_INET);

	fd = open_fd("/dev/input/event0");
	if (fd <= 0) {
		printf("can't open, exiting...\n");
		return 1;
	}

	run(8003);
}

