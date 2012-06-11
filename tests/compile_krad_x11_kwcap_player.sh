gcc -g -Wall -I../tools/krad_wayland/ -I../tools/krad_x11/ -I../tools/krad_wayland/ext/ -I../tools/krad_gui/ -I../tools/krad_system/ ../tools/krad_gui/krad_gui.c \
../tools/krad_x11/krad_x11.c ../tools/krad_wayland/kwcap-decode.c \
../tools/krad_system/krad_system.c krad_x11_kwcap_player.c -o krad_x11_kwcap_player \
 `pkg-config --libs --cflags gtk+-3.0 x11-xcb xcb-atom xcb-image xcb x11 gl glu` -lm
