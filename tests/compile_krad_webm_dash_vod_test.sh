gcc -g -Wall -I../lib/krad_list/ -I../lib/krad_system/ ../lib/krad_list/krad_webm_dash_vod.c \
../lib/krad_system/krad_system.c krad_webm_dash_vod_test.c -o krad_webm_dash_vod_test \
-pthread -lm -lrt `pkg-config --libs --cflags libxml-2.0`
