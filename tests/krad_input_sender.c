#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/input.h>
#include <linux/uinput.h>

#define die(str, args...) do { \
        perror(str); \
        exit(EXIT_FAILURE); \
    } while(0)

int
main(void)
{
    int                    fd;
    struct uinput_user_dev uidev;
    struct input_event     ev;
    int                    dx, dy;
    int                    i;

//    fd = open("/dev/input/mouse0", O_RDONLY | O_NONBLOCK);
    fd = open("/dev/input/mouse0", O_RDONLY);

    if(fd < 0)
        die("error: open");
	while (1) {
	    if(read(fd, &ev, sizeof(struct input_event)) < 0)
    	    die("error: write");

		printf(" %d\n", ev.code);

	}
	


    close(fd);

    return 0;
}
