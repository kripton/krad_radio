gcc -g -Wall -pthread krad_ogg_test.c ../krad_io/krad_io.c -I../krad_io/ krad_ogg.c -o krad_ogg_test `pkg-config --libs --cflags ogg vorbis theora` -lm -lpthread

