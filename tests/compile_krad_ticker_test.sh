gcc -g -Wall -pthread -I../tools/krad_ticker/ -I../tools/krad_radio/ -I../tools/krad_system/ \
../tools/krad_ticker/krad_ticker.c \
../tools/krad_system/krad_system.c krad_ticker_test.c -o krad_ticker_test \
-lm -lrt
