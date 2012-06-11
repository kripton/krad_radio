#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char *argv[]) {

        int mfd;
        char b[3];
        mfd = open("/dev/input/mice", O_RDONLY);
        int xd=0,yd=0; //x/y movement delta
        int xo=0,yo=0; //x/y overflow (out of range -255 to +255)
        int lb=0,mb=0,rb=0,hs=0,vs=0; //left/middle/right mousebutton
        int run=0;
        while(!run){
                read(mfd, b, sizeof(char) * 3);
                lb=(b[0]&1)>0;
                rb=(b[0]&2)>0;
                mb=(b[0]&4)>0;
                hs=(b[0]&16)>0;
                vs=(b[0]&32)>0;
                xo=(b[0]&64)>0;
                yo=(b[0]&128)>0;
                xd=b[1];
                yd=b[2];
//                printf("hs=%d,vs=%d,lb=%d rm=%d mb=%d xo=%d yo=%d xd=%5d yd=%5d\r",hs,vs,lb,rb,mb,xo,yo,xd,yd);


		if (xd > 0) {
			printf("X+%d", xd);
		} else {
			printf("X%d", xd);
		}
		if (yd > 0) {
			printf("Y+%d", yd);
		} else {
			printf("Y%d", yd);
		}
	
		if (lb > 0) {
			printf("CCC");
		}
	
	


		fflush(stdout);
        }
        close(mfd);

	return 0;

}
