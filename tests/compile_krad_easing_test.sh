gcc -g -Wall -I../tools/krad_compositor/ -I../tools/krad_system/ ../tools/krad_compositor/krad_easing.c \
../tools/krad_system/krad_system.c krad_easing_test.c -o krad_easing_test \
-pthread -lm
