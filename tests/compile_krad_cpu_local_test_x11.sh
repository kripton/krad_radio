gcc -g -Wall -I../tools/krad_x11/ -I../tools/krad_gui/ -I../tools/krad_system/ ../tools/krad_gui/krad_gui.c \
../tools/krad_x11/krad_x11.c ../tools/krad_system/krad_system.c krad_cpu_local_test_x11.c -o krad_cpu_local_test_x11 \
 `pkg-config --libs --cflags gtk+-3.0 x11-xcb xcb-atom xcb-image xcb x11 gl glu` -lm
