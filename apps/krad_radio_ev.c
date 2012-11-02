#include <linux/input.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include "krad_ipc_client.h"
#include "krad_alsa.h"

void kr_alsa_test () {

  krad_alsa_t *krad_alsa;

  krad_alsa = krad_alsa_create (NULL, 1);

  //krad_alsa_control_test (krad_alsa);

  krad_alsa_destroy (krad_alsa);

}

static inline float scramp_value (short value, float range, float min) {

  float ret;

  if (value > 4082) {
    value = 4096;
  } else if (value < 16) {
    value = 0;
  }

  ret = ((value / 4096.0) * range) + min;

  return ret;

}

void prun (char *cmd) {

  FILE *fp;
  char buf[64];

  fp = popen(cmd, "r");
  if (fp == NULL) {
    return;
  }

  while (fgets(buf, 64, fp) != NULL) {
  //   printf("%s", path);
  }

  pclose (fp);

}

int main(int argc, char **argv) {

  int fd;
  float val;
  char cmd[128];
  struct input_event ev;
	kr_client_t *client;
  int total_cmds;

  total_cmds = 0;

  client = NULL;

  if (argc < 3) {
    printf("usage: krad_radio_ev station inputdev\n");
    return 1;
  }

  //kr_alsa_test ();

  client = kr_connect (argv[1]);
	if (client == NULL) {
    printf ("Could not connect to krad radio :(\n");
    return 1;
  }

  fd = open (argv[2], O_RDONLY);

  printf ("Seems to be working.\n");

  while (1) {
    
    read (fd, &ev, sizeof (struct input_event));

    if ((ev.type == 3) && (ev.code == 23)) {
      val = scramp_value(ev.value, 200.0, -100.0);
      krad_ipc_set_control (client, "Music", "crossfade", val);
      total_cmds++;
      //printf ("type %d key %i value %f   \r", ev.type, ev.code, val);
      //fflush (stdout);
      continue;
    }

    if ((ev.type == 3) && ((ev.code == 19) || (ev.code == 18) || (ev.code == 17) || (ev.code == 16))) {
      val = scramp_value(ev.value, 100.0, 0.0);

      if (ev.code == 19) {
        krad_ipc_set_control (client, "Music", "volume", val);
      } else {
        krad_ipc_set_control (client, "Music2", "volume", val);
      }
      total_cmds++;
      //printf ("type %d key %i value %f   \r", ev.type, ev.code, val);
      //fflush (stdout);
      continue;
    }

    if ((ev.type == 3) && ((ev.code == 41) || (ev.code == 42) || (ev.code == 43))) {

      val = scramp_value(ev.value, 24.0, -12.0);

      if (ev.code == 41) {
  			krad_ipc_set_effect_control (client, "Music", 0, "db", 0, val);
      }
      if (ev.code == 42) {
				krad_ipc_set_effect_control (client, "Music", 0, "db", 1, val);
      }
      if (ev.code == 43) {
				krad_ipc_set_effect_control (client, "Music", 0, "db", 2, val);
      }

      total_cmds++;
      continue;
    }

    if ((ev.type == 3) && ((ev.code == 37) || (ev.code == 38) || (ev.code == 39))) {

      val = scramp_value(ev.value, 24.0, -12.0);

      if (ev.code == 37) {
  			krad_ipc_set_effect_control (client, "Music2", 0, "db", 0, val);
      }
      if (ev.code == 38) {
				krad_ipc_set_effect_control (client, "Music2", 0, "db", 1, val);
      }
      if (ev.code == 39) {
				krad_ipc_set_effect_control (client, "Music2", 0, "db", 2, val);
      }

      total_cmds++;
      continue;
    }

    if ((ev.type == 1) && ((ev.code == 265) || (ev.code == 267))) {
      if (ev.value == 1) {

        if (ev.code == 267) {
          sprintf(cmd, "XMMS_PATH=/tmp/music1 xmms2 seek -10");
        } else {
          sprintf(cmd, "XMMS_PATH=/tmp/music1 xmms2 seek -20");
        }
        prun (cmd);
      }
      continue;
    }

    if ((ev.type == 1) && ((ev.code == 269) || (ev.code == 271))) {
      if (ev.value == 1) {

        if (ev.code == 269) {
          sprintf(cmd, "XMMS_PATH=/tmp/music1 xmms2 seek +10");
        } else {
          sprintf(cmd, "XMMS_PATH=/tmp/music1 xmms2 seek +20");
        }
        prun (cmd);
      }
      continue;
    }

    if ((ev.type == 1) && ((ev.code == 305) || (ev.code == 307))) {
      if (ev.value == 1) {

        if (ev.code == 307) {
          sprintf(cmd, "XMMS_PATH=/tmp/music2 xmms2 seek -10");
        } else {
          sprintf(cmd, "XMMS_PATH=/tmp/music2 xmms2 seek -20");
        }
        prun (cmd);
      }
      continue;
    }

    if ((ev.type == 1) && ((ev.code == 309) || (ev.code == 311))) {
      if (ev.value == 1) {

        if (ev.code == 309) {
          sprintf(cmd, "XMMS_PATH=/tmp/music2 xmms2 seek +10");
        } else {
          sprintf(cmd, "XMMS_PATH=/tmp/music2 xmms2 seek +20");
        }
        prun (cmd);
      }
      continue;
    }

    if ((ev.type == 1) && ((ev.code == 259) || (ev.code == 261))) {
      if (ev.value == 1) {

        if (ev.code == 259) {
          krad_ipc_mixer_portgroup_xmms2_cmd (client, "Music", "prev");
        } else {
          krad_ipc_mixer_portgroup_xmms2_cmd (client, "Music", "next");
        }
        prun (cmd);
      }
      continue;
    }

    if ((ev.type == 1) && ((ev.code == 315) || (ev.code == 317))) {
      if (ev.value == 1) {

        if (ev.code == 315) {
          krad_ipc_mixer_portgroup_xmms2_cmd (client, "Music2", "prev");
        } else {
          krad_ipc_mixer_portgroup_xmms2_cmd (client, "Music2", "next");
        }
        prun (cmd);
      }
      continue;
    }

    if ((ev.type == 1) && ((ev.code == 263) || (ev.code == 319))) {
      if (ev.value == 1) {

        if (ev.code == 319) {
          sprintf(cmd, "XMMS_PATH=/tmp/music2 xmms2 toggle");
        } else {
          sprintf(cmd, "XMMS_PATH=/tmp/music1 xmms2 toggle");
        }
        prun (cmd);

      } else {
        //printf ("chicken!\n");
      }
    } else {
      if (argc == 4) {
        printf ("type %d key %i value %i\n", ev.type, ev.code, ev.value);
      }
    }

    if ((ev.type == 1) && (ev.code == 349)) {

      break;
    }

  }

  close (fd);

  kr_disconnect (client);

  printf ("total commands: %d", total_cmds);

  return 0;

}
