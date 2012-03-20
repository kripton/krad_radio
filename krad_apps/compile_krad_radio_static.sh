gcc -Wall -pthread krad_radio.c ../krad_tools/krad_radio/krad_radio.c ../krad_tools/krad_ipc/krad_ipc_server.c \
-I../krad_tools/krad_ipc -I../krad_tools/krad_radio -o krad_radio -lrt
