gcc -g -Wall -pthread -I../tools/krad_xmms2/ -I../tools/krad_system/ ../tools/krad_xmms2/krad_xmms2.c \
../tools/krad_system/krad_system.c krad_xmms2_test.c -o krad_xmms2_test \
 `pkg-config --libs --cflags xmms2-client` -lm
