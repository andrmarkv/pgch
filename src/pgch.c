#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <net/if.h>
#include <errno.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <linux/if_ether.h>
#include <linux/filter.h>
#include <pthread.h>
#include <time.h>

#define BUFSIZE 1024

// from <linux/input.h>
#define EVIOCGVERSION		_IOR('E', 0x01, int)			/* get driver version */
#define EVIOCGID		_IOR('E', 0x02, struct input_id)	/* get device ID */
#define EVIOCGKEYCODE		_IOR('E', 0x04, int[2])			/* get keycode */
#define EVIOCSKEYCODE		_IOW('E', 0x04, int[2])			/* set keycode */
#define EVIOCGNAME(len)		_IOC(_IOC_READ, 'E', 0x06, len)		/* get device name */
#define EVIOCGPHYS(len)		_IOC(_IOC_READ, 'E', 0x07, len)		/* get physical location */
#define EVIOCGUNIQ(len)		_IOC(_IOC_READ, 'E', 0x08, len)		/* get unique identifier */
#define EVIOCGKEY(len)		_IOC(_IOC_READ, 'E', 0x18, len)		/* get global keystate */
#define EVIOCGLED(len)		_IOC(_IOC_READ, 'E', 0x19, len)		/* get all LEDs */
#define EVIOCGSND(len)		_IOC(_IOC_READ, 'E', 0x1a, len)		/* get all sounds status */
#define EVIOCGSW(len)		_IOC(_IOC_READ, 'E', 0x1b, len)		/* get all switch states */
#define EVIOCGBIT(ev,len)	_IOC(_IOC_READ, 'E', 0x20 + ev, len)	/* get event bits */
#define EVIOCGABS(abs)		_IOR('E', 0x40 + abs, struct input_absinfo)		/* get abs value/limits */
#define EVIOCSABS(abs)		_IOW('E', 0xc0 + abs, struct input_absinfo)		/* set abs value/limits */
#define EVIOCSFF		_IOC(_IOC_WRITE, 'E', 0x80, sizeof(struct ff_effect))	/* send a force effect to a force feedback device */
#define EVIOCRMFF		_IOW('E', 0x81, int)			/* Erase a force effect */
#define EVIOCGEFFECTS		_IOR('E', 0x84, int)			/* Report number of effects playable at the same time */
#define EVIOCGRAB		_IOW('E', 0x90, int)			/* Grab/Release device */

struct input_event {
	struct timeval time;
	__u16 type;
	__u16 code;
	__s32 value;
};
// end <linux/input.h>

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

int parse_output(char* cmd) {
	char buf[BUFSIZE];
	FILE *fp;

	if ((fp = popen(cmd, "r")) == NULL) {
		printf("Error opening pipe!\n");
		return -1;
	}

	while (fgets(buf, BUFSIZE, fp) != NULL) {
		// Do whatever you want here...
		printf("OUTPUT: %s", buf);
	}

	if (pclose(fp)) {
		printf("Command not found or exited with error status\n");
		return -1;
	}

	return 0;
}

int send_touch(int fd, int x, int y) {
	int ret = 1;

	ret = ret & send_event(fd, 3, 57, 470);
	ret = ret & send_event(fd, 3, 53, x);
	ret = ret & send_event(fd, 3, 54, y);
	ret = ret & send_event(fd, 3, 58, 47);
	ret = ret & send_event(fd, 0, 0, 0);
	ret = ret & send_event(fd, 3, 57, 0xffffffff);
	ret = ret & send_event(fd, 0, 0, 0);

	return ret;
}

int send_swipe(int fd, char* buf) {
	printf("TEST 1, %s\n", buf);
	char *p = strtok(buf, ";");

	printf("TEST 2\n");

	while (p != NULL) {
		printf("TEST 3\n");
		printf("event: %s\n", p);
		p = strtok(NULL, ";");
	}
	printf("TEST 4\n");

	return 1;
}

int test2() {
	FILE *pipe_fp;
	char readbuf[4096];
	int bsize = sizeof(readbuf);
	int c;
	int bcount;
	int fd;

	/* Create one way pipe line with call to popen() */
	if ((pipe_fp = popen("/system/bin/screencap -p", "r")) == NULL) {
		printf("Error in popen\n");
		return -1;
	}

	/* Processing loop */
//	do {
//		c = fgetc(pipe_fp);
//		if (feof(pipe_fp)) {
//			break;
//		}
//		//printf("%c", c);
//		bcount++;
//	} while (1);
	/* Processing loop */
	fd = fileno(pipe_fp);
	do {
		c = read(fd, readbuf, bsize);
		if (c == 0) {
			break;
		}
		//printf("%c", c);
		bcount = bcount + c;
	} while (1);

	pclose(pipe_fp);

	printf("got bytes: %d\n", bcount);

	return 1;
}

void hexdump(char *desc, void *buf, int len) {
	int i;
	unsigned char buff[17];
	unsigned char *pc = (unsigned char*) buf;

	// Process every byte in the data.
	for (i = 0; i < len; i++) {
		// Multiple of 16 means new line (with line offset).

		if ((i % 16) == 0) {
			// Just don't print ASCII for the zeroth line.
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

int handle_message(int msgId, int type, char* msg, int len) {
	printf("Got message, msgId: %d, type: %d, len: %d\n", msgId, type, len);
	hexdump("MSGDUMP", msg, len);
	return 1;
}

int run(int port) {
	int socket_desc, client_sock, c, read_size;
	struct sockaddr_in server;

	struct sockaddr_in client; /* client addr */
	char *hostaddrp; /* dotted decimal host addr string */

	int b, blen;
	char msgbuf[4096];
	int msgId;
	int type;

	//Create socket
	socket_desc = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_desc == -1) {
		printf("Could not create socket\n");
	}
	printf("Socket created\n");

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
	while (1) {
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

			//Read message
			b = read(client_sock, msgbuf, blen);
			if (b != blen) {
				printf("Can't read message, closing socket\n");
				break;
			}

			msgId = atoi(msgbuf);
			type = atoi(msgbuf + 4);

			handle_message(msgId, type, msgbuf + 8, blen - 8);

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

int test3() {
	int fd;
	struct timeval t0, t1;

	printf("Test of send event...\n");

	//Open file descriptor for output
	fd = open_fd("/dev/input/event1");
	if (fd <= 0) {
		printf("can't open, exiting...\n");
		return 1;
	}

	//send test events
	send_touch(fd, 600, 1000);
	send_swipe(fd, "TESTB");

	//test reading screen

	gettimeofday(&t0, NULL);
	test2();
	gettimeofday(&t1, NULL);
	print_dt(&t0, &t1);

	printf("Test of send done!\n");

	return 1;
}

int main(int argc, char **argv) {
	print_addresses(AF_INET);
	run(8003);
}

int test1(void) {
	printf("Hello World!\n");

	char *buf[128];

	int x0 = 100;
	int x1 = 1000;
	int y0 = 250;
	int y1 = 1700;

	int steps = 10;
	int dx = (x1 - x0) / steps;
	int dy = (y1 - y0) / steps;

	int x, y;

	for (int i = 0; i < steps; i++) {
		x = x0 + dx * i;
		y = y0 + dy * i;
//		sprintf(buf, "/system/bin/input touchscreen swipe %d %d %d %d %d", x0, y, x1, y, 200 * i);
//		parse_output(buf);
//		sprintf(buf, "/system/bin/input touchscreen swipe %d %d %d %d %d", x, y0, x, y1, 200 * i);
//		parse_output(buf);
	}

//	parse_output("/system/bin/input touchscreen swipe 100 500 1000 500 100");

	return 0;
}
