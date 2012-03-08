gcc -O3 -Wall krad_x11_test.c krad_x11.c -o krad_x11_test -std=c99 `pkg-config --libs --cflags xcb x11 gl xext cairo` -lX11-xcb -lxcb_atom -lrt -lxcb-image -lm
