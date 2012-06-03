gcc -g -Wall krad_wayland.c krad_wayland_test.c -o krad_wayland_test `pkg-config --libs --cflags wayland-client` -lm
