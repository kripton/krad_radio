gcc -g -Wall -pthread krad_ogg.c krad_ogg_test.c -o krad_ogg_test `pkg-config --libs --cflags ogg vorbis theora` -lm -lpthread

