gcc -Wall -pthread krad_radio.c ../krad_tools/krad_radio/krad_radio.c ../krad_tools/krad_ipc/krad_ipc_server.c \
-I../krad_tools/krad_ipc -I../krad_tools/krad_radio ../krad_ebml/krad_ebml.c -I../krad_ebml/  -o krad_radio -lrt


gcc -Wall -pthread krad_radio_cmd.c ../krad_tools/krad_ipc/krad_ipc_client.c \
-I../krad_tools/krad_ipc -I../krad_tools/krad_radio ../krad_ebml/krad_ebml.c -I../krad_ebml/ -o krad_radio_cmd -lrt
