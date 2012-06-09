gcc -g -Wall -I../tools/krad_wayland/ ../tools/krad_wayland/krad_wayland.c krad_wayland_test.c -o krad_wayland_test `pkg-config --libs --cflags wayland-client` -lm
