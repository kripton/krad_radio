gcc -g -Wall krad_signal_generator.c \
../krad_vorbis/krad_vorbis.c \
-I../krad_vorbis \
../krad_vpx/krad_vpx.c \
../krad_audio/krad_audio.c \
-I../krad_audio \
../krad_pulse/krad_pulse.c \
-I../krad_pulse \
../krad_jack/krad_jack.c \
-I../krad_jack \
../krad_alsa/krad_alsa.c \
-I../krad_alsa \
-I../krad_vpx \
../krad_tone/krad_tone.c \
-I../krad_tone \
../../krad_ebml/krad_ebml.c \
-I../../krad_ebml/halloc/ \
../../krad_ebml/halloc/src/halloc.c \
-I../../krad_ebml/ \
-I. \
-I../krad_gui/ \
../krad_gui/krad_gui.c \
/usr/lib/libswscale.a \
/usr/lib/libavutil.a \
/usr/local/lib/libvpx.a \
/usr/local/lib/libvpx.a \
-o krad_signal_generator_cmd `pkg-config --cflags --libs gtk+-3.0` \
-lXext -lX11 -lXmu -lXi -lGL -lGLU -lm -ljack -lpulse -lasound
