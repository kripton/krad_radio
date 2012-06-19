gcc -g -Wall -pthread -I../tools/krad_ticker/ -I../tools/krad_radio/ -I../tools/krad_system/ \
../tools/krad_system/krad_system.c krad_fps_test.c -o krad_fps_test \
-lm -lrt
