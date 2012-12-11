/**
 * @file krad_radio_client_ctl.h
 * @brief Krad Radio Daemon Control API
 */
 
/** @defgroup krad_radio_client_ctl Krad Radio Daemon Control API
 * @brief Launch / Check / Destroy / List and Manage local running stations
  @{
  */

/**
 * @brief launches a station identified by sysname
 * @param sysname
 */
void krad_radio_launch (char *sysname);

/**
 * @brief destroys a running session identified by sysname
 * @param sysname
 * @return 0 if daemon quit right away, 1 if SIGKILL was needed and -1 if there was no such station
 */
int krad_radio_destroy (char *sysname);

/**
 * @brief determine if there's a station running identified by sysname
 * @param sysname
 * @return 1 if the station's PID could be found, 0 otherwise
 */
int krad_radio_running (char *sysname);

/**
 * @brief get a list of running stations
 * @return list of stations as string delimited by newlines. NULL on error
 */
char *krad_radio_running_stations ();

/**
 * @brief get the uptime of the station identified by sysname
 * @param sysname
 * @return string containing the uptime in human readable form. NULL on error (station not found)
 */
char *kr_station_uptime (char *sysname);

/** @}*/
