
echo "----krad radio gtk -----"

gcc -Wall -pthread -g krad_radio_gtk.c ../krad_tools/krad_ipc/krad_ipc_client.c \
-I../krad_tools/krad_ipc -I../krad_tools/krad_radio ../krad_ebml/krad_ebml.c -I../krad_ebml/ -o krad_radio_gtk \
`pkg-config --cflags --libs gtk+-3.0` -lrt -lm
