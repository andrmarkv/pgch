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

#define MESSAGE_ANDROID_TYPE_TEST 1
#define MESSAGE_ANDROID_SCREEN_CAP 2
#define MESSAGE_ANDROID_SEND_TOUCH 3
#define MESSAGE_ANDROID_SEND_SWIPE 4
#define MESSAGE_ANDROID_EXIT 7899

int print_addresses(const int domain);
int open_fd(char* dev);
int send_event(int fd, int type, int code, int value);
int send_touch(int fd, int x, int y);
void hexdump(void *buf, int len);
int handle_message(int msgId, int type, char* msg, int len, char** msg_out);
int hangle_touch_msg(char* msg, char** msg_out);
int hangle_swipe_msg(char* msg, int len, char** msg_out);
int print_dt(struct timeval* t0, struct timeval* t1);
char* m_malloc(size_t size);
char* m_realloc(char* ptr, size_t old_size, size_t new_size);
void m_free(char* ptr, size_t size);
int capture_screen(char** msg_out);
