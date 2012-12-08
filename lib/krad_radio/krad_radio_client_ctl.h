/**
 * @file krad_radio_client_ctl.h
 * @brief Krad Radio Daemon Control API
 */

/**
 * @mainpage Krad Radio Daemon Control
 *
 * Krad Radio Daemon Control
 *
 * Launch / Check / Destroy and List local running stations 
 *
 * (Kripton!)
 *
 */
 
/** @defgroup krad_radio_client_ctl Krad Radio Daemon Control
  @{
  */

void krad_radio_launch (char *sysname);
int krad_radio_destroy (char *sysname);
int krad_radio_running (char *sysname);
char *krad_radio_running_stations ();

/**@}*/
