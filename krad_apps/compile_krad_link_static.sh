gcc -O3 -Wall krad_link.c \
-I../krad_tools/krad_opus \
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
../krad_tools/krad_audio/krad_audio.c \
-I../krad_tools/krad_audio \
../krad_tools/krad_pulse/krad_pulse.c \
-I../krad_tools/krad_pulse \
../krad_tools/krad_jack/krad_jack.c \
-I../krad_tools/krad_jack \
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
/usr/lib/libswscale.a \
-D_REENTRANT -Wl,-rpath,/usr/lib -L/usr/lib -lm \
/usr/lib/libavutil.a \
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
-o krad_link `pkg-config --cflags --libs gtk+-3.0 xcb x11 gl xext xcb-util xcb-aux xcb-atom cairo` \
-lX11-xcb -lrt -lxcb-image -lm -ljack -lpulse -lasound -lcodec2

#-lXext -lX11 -lXmu -lXi -lGL -lGLU -lm -ljack -lpulse -lasound
