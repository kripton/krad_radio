echo "----krad radio -----"

gcc -Wall -g -pthread krad_radio.c ../krad_tools/krad_radio/krad_radio.c ../krad_tools/krad_ipc/krad_ipc_server.c \
-I../krad_tools/krad_ipc ../krad_tools/krad_ipc/krad_ipc_client.c -I../krad_tools/krad_web/ -I../krad_tools/krad_radio \
../krad_ebml/krad_ebml.c -I../krad_ebml/ \
../krad_tools/krad_mixer/krad_mixer.c -I../krad_tools/krad_mixer/ \
../krad_tools/krad_effects/util/rms.c ../krad_tools/krad_effects/util/db.c ../krad_tools/krad_effects/digilogue.c ../krad_tools/krad_effects/djeq.c ../krad_tools/krad_effects/fastlimiter.c \
../krad_tools/krad_effects/sidechain_comp.c ../krad_tools/krad_effects/hardlimiter.c -I../krad_tools/krad_effects/ -I../krad_tools/krad_effects/util \
../krad_tools/krad_tags/krad_tags.c -I../krad_tools/krad_tags/ \
../krad_tools/krad_web/krad_websocket.c ../krad_tools/krad_web/krad_http.c -o krad_radio -lrt -lwebsockets -lm -ljack


echo "----krad radio cmd -----"

gcc -Wall -pthread krad_radio_cmd.c ../krad_tools/krad_ipc/krad_ipc_client.c \
-I../krad_tools/krad_ipc -I../krad_tools/krad_radio ../krad_ebml/krad_ebml.c -I../krad_ebml/ -o krad_radio_cmd -lrt

echo "----krad mixer gtk -----"

gcc -Wall -pthread -g krad_mixer_gtk.c ../krad_tools/krad_ipc/krad_ipc_client.c \
-I../krad_tools/krad_ipc -I../krad_tools/krad_radio ../krad_ebml/krad_ebml.c -I../krad_ebml/ -o krad_mixer_gtk \
`pkg-config --cflags --libs gtk+-3.0` -lrt
