gcc -g -Wall -pthread -I/usr/local/include -I../krad_ebml_tools/krad_mixer -I../krad_ebml_tools/krad_ipc -I../krad_ebml_tools/krad_effects -okrad_mixerd \
krad_mixerd.c ../krad_ebml_tools/krad_mixer/krad_mixer.c ../krad_ebml_tools/krad_ipc/krad_ipc_server.c ../krad_ebml_tools/krad_effects/util/rms.c ../krad_ebml_tools/krad_effects/util/db.c ../krad_ebml_tools/krad_effects/util/iir.c ../krad_ebml_tools/krad_effects/pass.c \
../krad_ebml_tools/krad_effects/digilogue.c ../krad_ebml_tools/krad_effects/djeq.c ../krad_ebml_tools/krad_effects/fastlimiter.c ../krad_ebml_tools/krad_effects/sidechain_comp.c ../krad_ebml_tools/krad_effects/hardlimiter.c \
-ljack -lm -lpthread


gcc -g -Wall -pthread -I../krad_ebml_tools/krad_mixer -I../krad_ebml_tools/krad_ipc -okrad_mixer \
krad_mixer.c ../krad_ebml_tools/krad_mixer/krad_mixer_clientlib.c ../krad_ebml_tools/krad_ipc/krad_ipc_client.c -lm -lpthread

