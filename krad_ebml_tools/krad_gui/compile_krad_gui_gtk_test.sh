gcc -Wall -g -o krad_gui_gtk_test krad_gui_gtk_test.c krad_gui.c krad_gui_gtk.c `pkg-config --cflags --libs gtk+-2.0` -lrt
