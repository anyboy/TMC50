//----------------------------------------------------------------------------//
#ifndef __WIFI_API_H
#define __WIFI_API_H

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*wifi_evtcb_fn)(unsigned int nEvtId, void *data, unsigned int len);

void netdev_status_change_cb(void *netif);
void register_wifi_event_cb(wifi_evtcb_fn evtcb);
int run_wifi_cmd_arg(int argc, char *argv[]);
int run_wifi_cmd(char *CmdBuffer);

#ifdef __cplusplus
  }
#endif

#endif // __WIFI_API_H

//----------------------------------------------------------------------------//
