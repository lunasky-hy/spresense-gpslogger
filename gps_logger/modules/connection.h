#pragma once
#define SORACOM_APN_NAME "soracom.io"
#define SORACOM_APN_USR_NAME "sora"
#define SORACOM_APN_PASSWD "sora"
#define SORACOM_APN_IPTYPE (LTE_IPTYPE_V4V6)
#define SORACOM_APN_AUTHTYPE (LTE_APN_AUTHTYPE_CHAP)
#define SORACOM_APN_RAT (LTE_RAT_CATM)

#define STATE_UNINITIALIZED 0
#define STATE_INITIALIZED 1
#define STATE_POWER_ON 2
#define STATE_RADIO_ON 3
#define STATE_CONNECTED_PDN 4

#define CB_STATUS_ZERO 0

extern void errMsg(int errcode);
int getErrorCode(void);

void callbacked(void);
void restart_cb(uint32_t r);
// void netinfo_cb(lte_netinfo_t *info);

// void show_lte_info(lte_version_t *version);
extern int get_imsi(char *imsi);

extern void finishProceeds(int state, int stop);
extern void startProceeds(int state, int stop);
