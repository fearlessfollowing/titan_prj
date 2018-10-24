#ifndef __INS_WIFI_H__
#define __INS_WIFI_H__

extern int tx_softap_config(const char *ssid,  const char *pwd ,int bopen);
extern int tx_softap_start();
extern int tx_softap_stop();
extern int tx_softsta_start(const char *ssid, const char *pwd , int time);
extern int tx_softsta_stop();
extern int tx_wifi_status();

#endif