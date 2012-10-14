gcc -g -Wall -pthread -I../tools/krad_transmitter/ -I../tools/krad_radio/ -I../tools/krad_ring/ -I../tools/krad_system/ \
../tools/krad_ring/krad_ring.c ../tools/krad_transmitter/krad_transmitter.c \
../tools/krad_system/krad_system.c krad_transmitter_test.c -o krad_transmitter_test \
-lm -lrt
