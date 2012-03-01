gcc -g -Wall krad_link.c \
-I../krad_ebml_tools/krad_opus \
../krad_ebml_tools/krad_dirac/krad_dirac.c \
-I../krad_ebml_tools/krad_dirac \
../krad_ebml_tools/krad_flac/krad_flac.c \
../krad_ebml_tools/krad_opus/krad_opus.c \
../krad_ebml_tools/krad_opus/opus_header.c \
-I../krad_ebml_tools/krad_flac \
-I../krad_ebml_tools/krad_v4l2 \
../krad_ebml_tools/krad_v4l2/krad_v4l2.c \
../krad_ebml_tools/krad_vorbis/krad_vorbis.c \
-I../krad_ebml_tools/krad_vorbis \
../krad_ebml_tools/krad_vpx/krad_vpx.c \
-I../krad_ebml_tools/krad_vpx \
../krad_ebml_tools/krad_tone/krad_tone.c \
-I../krad_ebml_tools/krad_tone \
../krad_ebml_tools/krad_audio/krad_audio.c \
-I../krad_ebml_tools/krad_audio \
../krad_ebml_tools/krad_pulse/krad_pulse.c \
-I../krad_ebml_tools/krad_pulse \
../krad_ebml_tools/krad_jack/krad_jack.c \
-I../krad_ebml_tools/krad_jack \
../krad_ebml_tools/krad_ring/krad_ring.c \
-I../krad_ebml_tools/krad_ring \
../krad_ebml_tools/krad_alsa/krad_alsa.c \
-I../krad_ebml_tools/krad_alsa \
../krad_ebml_tools/krad_theora/krad_theora.c \
-I../krad_ebml_tools/krad_theora \
../krad_ebml_tools/krad_io/krad_io.c \
../krad_ebml_tools/krad_ogg/krad_ogg.c \
../krad_ebml_tools/krad_link/krad_link.c \
-I../krad_ebml_tools/krad_link \
-I../krad_ebml_tools/krad_io \
-I../krad_ebml_tools/krad_ogg \
../krad_ebml/krad_ebml.c \
-I../krad_ebml/ \
-I../krad_ebml_tools/krad_display/ \
../krad_ebml_tools/krad_display/krad_sdl_opengl_display.c \
-I. \
-I /usr/local/include/schroedinger-1.0/ \
-I /usr/local/include/orc-0.4/ \
-I /usr/local/include/opus/ \
-I../krad_ebml_tools/krad_gui/ \
../krad_ebml_tools/krad_gui/krad_gui.c \
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
-o krad_link `pkg-config --cflags --libs gtk+-3.0 theora theoraenc theoradec` \
-lXext -lX11 -lXmu -lXi -lGL -lGLU -lm -ljack -lpulse -lasound
