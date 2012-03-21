gcc -Wall -g -pthread krad_radio.c ../krad_tools/krad_radio/krad_radio.c ../krad_tools/krad_ipc/krad_ipc_server.c \
-I../krad_tools/krad_ipc ../krad_tools/krad_ipc/krad_ipc_client.c -I../krad_tools/krad_web/ -I../krad_tools/krad_radio \
../krad_ebml/krad_ebml.c -I../krad_ebml/ \
../krad_tools/krad_web/krad_websocket.c ../krad_tools/krad_web/krad_http.c -o krad_radio -lrt -lwebsockets


gcc -Wall -pthread krad_radio_cmd.c ../krad_tools/krad_ipc/krad_ipc_client.c \
-I../krad_tools/krad_ipc -I../krad_tools/krad_radio ../krad_ebml/krad_ebml.c -I../krad_ebml/ -o krad_radio_cmd -lrt


gcc -Wall -pthread -g krad_mixer_gtk.c ../krad_tools/krad_ipc/krad_ipc_client.c \
-I../krad_tools/krad_ipc -I../krad_tools/krad_radio ../krad_ebml/krad_ebml.c -I../krad_ebml/ -o krad_mixer_gtk \
`pkg-config --cflags --libs gtk+-3.0` -lrt
