echo "----krad radio -----"

cd ../
xxd -i krad_tools/krad_web/res/krad_radio.html krad_tools/krad_web/res/krad_radio.html.h
xxd -i krad_tools/krad_web/res/krad_radio.js krad_tools/krad_web/res/krad_radio.js.h
cd krad_apps

rm *.o
rm ../krad_tools/krad_decklink/krad_decklink_capture.a

g++ -Wall -Wno-multichar -fno-rtti -c ../krad_tools/krad_decklink/krad_decklink_capture.cpp ../krad_tools/krad_decklink/vendor/DeckLinkAPIDispatch.cpp -I../krad_tools/krad_decklink/ -I../krad_tools/krad_decklink/vendor
ar -cvq ../krad_tools/krad_decklink/krad_decklink_capture.a krad_decklink_capture.o DeckLinkAPIDispatch.o

rm *.o

gcc -g -Wall krad_radio.c \
-I../krad_tools/krad_web/res/ \
-I../krad_tools/krad_ipc ../krad_tools/krad_ipc/krad_ipc_client.c -I../krad_tools/krad_web/ -I../krad_tools/krad_radio \
../krad_tools/krad_effects/util/rms.c ../krad_tools/krad_effects/util/db.c ../krad_tools/krad_effects/digilogue.c ../krad_tools/krad_effects/djeq.c ../krad_tools/krad_effects/fastlimiter.c \
../krad_tools/krad_effects/sidechain_comp.c ../krad_tools/krad_effects/hardlimiter.c -I../krad_tools/krad_effects/ -I../krad_tools/krad_effects/util \
../krad_tools/krad_tags/krad_tags.c -I../krad_tools/krad_tags/ \
../krad_tools/krad_web/krad_websocket.c ../krad_tools/krad_web/krad_http.c \
-I../krad_tools/krad_opus \
-I../krad_tools/krad_system \
../krad_tools/krad_system/krad_system.c \
../krad_tools/krad_dirac/krad_dirac.c \
-I../krad_tools/krad_dirac \
../krad_tools/krad_flac/krad_flac.c \
../krad_tools/krad_opus/krad_opus.c \
../krad_tools/krad_opus/opus_header.c \
-I../krad_tools/krad_flac \
-I../krad_tools/krad_v4l2 \
../krad_tools/krad_v4l2/krad_v4l2.c \
../krad_tools/krad_vorbis/krad_vorbis.c \
-I../krad_tools/krad_vorbis \
../krad_tools/krad_vpx/krad_vpx.c \
-I../krad_tools/krad_vpx \
../krad_tools/krad_tone/krad_tone.c \
-I../krad_tools/krad_tone \
../krad_tools/krad_pulse/krad_pulse.c \
-I../krad_tools/krad_pulse \
../krad_tools/krad_jack/krad_jack.c \
-I../krad_tools/krad_jack \
../krad_tools/krad_audio/krad_audio.c \
-I../krad_tools/krad_audio \
../krad_tools/krad_mixer/krad_mixer.c -I../krad_tools/krad_mixer/ \
../krad_tools/krad_ring/krad_ring.c \
-I../krad_tools/krad_ring \
../krad_tools/krad_alsa/krad_alsa.c \
-I../krad_tools/krad_alsa \
../krad_tools/krad_codec2/krad_codec2.c \
-I../krad_tools/krad_codec2 \
../krad_tools/krad_theora/krad_theora.c \
-I../krad_tools/krad_theora \
../krad_tools/krad_io/krad_io.c \
../krad_tools/krad_ogg/krad_ogg.c \
../krad_tools/krad_udp/krad_udp.c \
-I../krad_tools/krad_decklink \
-I../krad_tools/krad_framepool \
../krad_tools/krad_framepool/krad_framepool.c \
-I../krad_tools/krad_compositor \
../krad_tools/krad_compositor/krad_compositor.c \
../krad_tools/krad_decklink/krad_decklink.c \
../krad_tools/krad_decklink/krad_decklink_capture.a \
-I../krad_tools/krad_udp \
../krad_tools/krad_x11/krad_x11.c \
-I../krad_tools/krad_x11 \
../krad_tools/krad_container/krad_container.c \
-I../krad_tools/krad_container \
../krad_tools/krad_link/krad_link.c \
-I../krad_tools/krad_link \
-I../krad_tools/krad_io \
-I../krad_tools/krad_ogg \
../krad_ebml/krad_ebml.c \
-I../krad_ebml/ \
-I. \
-I /usr/local/include/schroedinger-1.0/ \
-I /usr/local/include/orc-0.4/ \
-I /usr/local/include/opus/ \
-I../krad_tools/krad_gui/ \
../krad_tools/krad_gui/krad_gui.c \
/usr/lib/x86_64-linux-gnu/libswscale.a \
-D_REENTRANT -Wl,-rpath,/usr/lib -L/usr/lib -lm \
/usr/lib/x86_64-linux-gnu/libavutil.a \
/usr/lib/libspeexdsp.a \
/usr/local/lib/libvpx.a \
/usr/local/lib/libtheora.a \
/usr/local/lib/libtheoradec.a \
/usr/local/lib/libtheoraenc.a \
/usr/local/lib/libschroedinger-1.0.a \
/usr/local/lib/liborc-0.4.a \
/usr/local/lib/libopus.a \
/usr/local/lib/liborc-test-0.4.a \
/usr/local/lib/libturbojpeg.a \
/usr/local/lib/libvpx.a \
/usr/local/lib/libwebsockets.a \
../krad_tools/krad_radio/krad_radio.c ../krad_tools/krad_ipc/krad_ipc_server.c \
-o krad_radio `pkg-config --cflags --libs gtk+-3.0 xcb x11 gl xext xcb-util xcb-aux xcb-atom cairo` \
-lX11-xcb -lrt -lxcb-image -lm -ljack -lpulse -lasound -lcodec2 -lsamplerate -lwebsockets


echo "----krad radio cmd -----"

gcc -Wall -pthread krad_radio_cmd.c ../krad_tools/krad_ipc/krad_ipc_client.c \
-I../krad_tools/krad_ipc -I../krad_tools/krad_radio ../krad_ebml/krad_ebml.c -I../krad_ebml/ -o krad_radio_cmd -lrt -lm

sh compile_krad_radio_gtk_static.sh
