gcc -g -Wall krad_cam.c \
-I../krad_opus \
../krad_dirac/krad_dirac.c \
-I../krad_dirac \
../krad_flac/krad_flac.c \
../krad_opus/krad_opus.c \
../krad_opus/opus_header.c \
-I../krad_flac \
-I../krad_v4l2 \
../krad_v4l2/krad_v4l2.c \
../krad_vorbis/krad_vorbis.c \
-I../krad_vorbis \
../krad_vpx/krad_vpx.c \
-I../krad_vpx \
../krad_tone/krad_tone.c \
-I../krad_tone \
../krad_audio/krad_audio.c \
-I../krad_audio \
../krad_pulse/krad_pulse.c \
-I../krad_pulse \
../krad_jack/krad_jack.c \
-I../krad_jack \
../krad_alsa/krad_alsa.c \
-I../krad_alsa \
../../krad_ebml/krad_ebml.c \
-I../../krad_ebml/halloc/ \
../../krad_ebml/halloc/src/halloc.c \
-I../../krad_ebml/ \
-I../krad_display/ \
../krad_display/krad_sdl_opengl_display.c \
-I. \
-I /usr/local/include/schroedinger-1.0/ \
-I /usr/local/include/orc-0.4/ \
-I /usr/local/include/opus/ \
-I../krad_gui/ \
../krad_gui/krad_gui.c \
/usr/lib/libswscale.a \
-D_REENTRANT -I/usr/include/SDL  -Wl,-rpath,/usr/lib -L/usr/lib -lSDL -lm \
/usr/lib/libavutil.a \
/usr/lib/libspeexdsp.a \
/usr/local/lib/libvpx.a \
/usr/local/lib/libschroedinger-1.0.a \
/usr/local/lib/liborc-0.4.a \
/usr/local/lib/libopus.a \
/usr/local/lib/liborc-test-0.4.a \
/usr/local/lib/libturbojpeg.a \
/usr/local/lib/libvpx.a \
-o krad_cam `pkg-config --cflags --libs gtk+-3.0` \
-lXext -lX11 -lXmu -lXi -lGL -lGLU -lm -ljack -lpulse -lasound
