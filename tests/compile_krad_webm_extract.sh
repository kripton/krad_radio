gcc -g -Wall -I../lib/krad_list/ -I../lib/krad_transmitter/ -I../lib/krad_radio/ -I../lib/krad_ring/ -I../lib/krad_ebml/  -I../lib/krad_container/ -I../lib/krad_system/ ../lib/krad_list/krad_webm_dash_vod.c \
../lib/krad_system/krad_system.c ../lib/krad_transmitter/krad_transmitter.c ../lib/krad_ring/krad_ring.c ../lib/krad_ebml/krad_ebml.c krad_webm_extract.c -o krad_webm_extract \
-pthread -lm -lrt `pkg-config --libs --cflags libxml-2.0`
