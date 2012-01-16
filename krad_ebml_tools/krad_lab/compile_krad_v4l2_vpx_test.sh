gcc -g -Wall -pthread -I. krad_v4l2.c krad_vpx.c krad_v4l2_vpx_test.c ../../libvpx/libvpx_g.a /usr/lib/libswscale.a /usr/lib/libavutil.a -o krad_v4l2_vpx_test -lpthread -lm

