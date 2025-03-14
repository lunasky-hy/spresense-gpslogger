#include <stdio.h>
#include <lte/lte_api.h>
#include <netutils/webclient.h>
#include "connection.h"

#define SORACOM_BEAM_URL "https://beam.soracom.io"
#define SORACOM_HARVEST_URL "http://harvest.soracom.io"

// --------------------------------------->>>>> Global variables
static int cb_status = 0;
static uint8_t my_session_id;
// Global variables <<<<<---------------------------------------

// --------------------------------------->>>>> error message
void err_msg(int errcode)
{
    switch (errcode)
    {
    case -EALREADY:
        printf("EALREADY\n");
        break;
    case -ENETDOWN:
        printf("ENETDOWN\n");
        break;
    case -EINPROGRESS:
        printf("EINPROGRESS\n");
        break;
    case -EPROTO:
        printf("EPROTO\n");
        lte_errinfo_t err_info;
        lte_get_errinfo(&err_info);
        switch (err_info.err_result_code)
        {
        case -EOPNOTSUPP:
            printf("EOPNOTSUPP\n");
            break;
        case -EALREADY:
            printf("EALREADY\n");
            break;
        case -EINVAL:
            printf("EINVAL\n");
            break;
        default:
            printf("error code: %ld\n", err_info.err_result_code);
            break;
        }
        break;

    default:
        printf("NON_DEFINED_ERROR: %d\n", errcode);
        break;
    }
}

int get_err_code(void)
{
    lte_errinfo_t err_info;
    lte_get_errinfo(&err_info);
    return err_info.err_result_code;
}

// error message <<<<<-----------------------------------

// --------------------------------------->>>>> callback functions
void callbacked(void)
{
    cb_status = CB_STATUS_ZERO;
}

void restart_cb(uint32_t r)
{
    printf("Restart: %ld\n", r);
    callbacked();
}

void netinfo_cb(lte_netinfo_t *info)
{
    printf("NW_STAT: %d\n", info->nw_stat);
    printf("PDN_NUM: %d\n", info->pdn_num);
}

void quality_cb(lte_quality_t *quality)
{
    printf("RSRQ: %d dBm (RSSI: %d dBm, RSRP: %d dBm), SINR: %d dBm\n", quality->rsrq, quality->rssi, quality->rsrp, quality->sinr);
}

// callback functions <<<<<--------------------------------------

// --------------------------------------->>>>> info functions
void show_lte_info(lte_version_t *version)
{
    printf("Version: %s\n", version->bb_product);
}

int get_imsi(char *imsi)
{
    int err = lte_get_imsi_sync(imsi, 16);
    if (err != 0)
    {
        printf("Failed to get IMSI...\n");
        return 1;
    }
    return 0;
}
// info functions <<<<<---------------------------------------

// --------------------------------------->>>>> finish process
void modem_finalize(void)
{
    lte_finalize();
    printf("Modem is finalized.\n");
}

void modem_pwoff(void)
{
    int err = lte_power_off();
    if (err != 0)
    {
        printf("Failed power off...\n");
    }
}

void modem_radiooff(void)
{
    int err = lte_radio_off_sync();
    if (err != 0)
    {
        printf("Failed radio off...\n");
    }
}

void modem_deacvtivepdn(void)
{
    printf("Deactivate PDN...\n");
    int err = lte_deactivate_pdn_sync(my_session_id);
    if (err != 0)
    {
        printf("Failed deactivate pdn...\n");
    }
}

void lte_finprocess(int state, int stop)
{
    switch (state)
    {
    case STATE_CONNECTED_PDN:
        modem_deacvtivepdn(); // PDN connected -> Radio on
        if (stop == STATE_RADIO_ON)
            break;
    case STATE_RADIO_ON:
        modem_radiooff(); // Radio on -> Power on
        if (stop == STATE_POWER_ON)
            break;
    case STATE_POWER_ON:
        modem_pwoff(); // Power On -> Initialized
        if (stop == STATE_INITIALIZED)
            break;
    case STATE_INITIALIZED:
        modem_finalize(); // Initialized -> Uninitialized
        break;
    default:
        printf("Finish Proceeds cannot execute... state: %d\n", state);
        break;
    }
}

// finish process <<<<<-----------------------------------

// --------------------------------------->>>>> connecting process
int modem_init(void)
{
    printf("initilizing...\n");
    int err = lte_initialize();
    if (err != 0)
    {
        printf("Failed initilizeing....\n");
        err_msg(err);
        if (err != -EALREADY)
            return -1;
        else
            printf("Modem is ready");
    }
    return 0;
}

int modem_pwron(void)
{
    cb_status = 1;
    int err = 0;
    printf("Restart...\n");
    err = lte_set_report_restart(restart_cb);
    printf("Power on...\n");
    err = lte_power_on();
    if (err != 0)
    {
        printf("Failed reset and power on....\n");
        err_msg(err);
        lte_finprocess(STATE_INITIALIZED, STATE_UNINITIALIZED);
        return -1;
    }

    while (cb_status != CB_STATUS_ZERO)
    {
        printf("Modem restarting... status: %d\n", cb_status);
        sleep(1);
    }
    return 0;
}

int modem_radioon(void)
{
    printf("Connecting LTE Network...\n");
    int err = 0;

    // lte_set_report_netinfo(netinfo_cb);
    // lte_set_report_quality(quality_cb, 60);
    err = lte_radio_on_sync();
    if (err != 0)
    {
        printf("Failed radio on...\n");
        err_msg(err);
        if (err == -EPROTO && get_err_code() == -EALREADY)
        {
            printf("Modem is already.");
        }
        else
        {
            lte_finprocess(STATE_POWER_ON, STATE_UNINITIALIZED);
            return -1;
        }
    }
    return 0;
}

int modem_activepdn(void)
{
    lte_apn_setting_t apn_s;
    lte_pdn_t mypdn;
    apn_s.apn = SORACOM_APN_NAME;
    apn_s.ip_type = SORACOM_APN_IPTYPE;
    apn_s.auth_type = SORACOM_APN_AUTHTYPE;
    apn_s.apn_type = LTE_APN_TYPE_DEFAULT | LTE_APN_TYPE_IA;
    apn_s.user_name = SORACOM_APN_USR_NAME;
    apn_s.password = SORACOM_APN_PASSWD;

    int err = lte_activate_pdn_sync(&apn_s, &mypdn);
    if (err != 0)
    {
        printf("Failed connecting network....\n");
        err_msg(err);
        lte_finprocess(STATE_RADIO_ON, STATE_UNINITIALIZED);
        return -1;
    }

    my_session_id = mypdn.session_id;

    printf("Connected LTE Network!\n");
    return 0;
}

void lte_staprocess(int state, int stop)
{
    int err = 0;
    switch (state)
    {
    case STATE_UNINITIALIZED:
        err = modem_init();
        if (stop == STATE_INITIALIZED)
            break;
        if (err < 0)
            break;
    case STATE_INITIALIZED:
        err = modem_pwron();
        if (stop == STATE_POWER_ON)
            break;
        if (err < 0)
            break;
    case STATE_POWER_ON:
        err = modem_radioon();
        if (stop == STATE_RADIO_ON)
            break;
        if (err < 0)
            break;
    case STATE_RADIO_ON:
        err = modem_activepdn();
        break;
    default:
        printf("Start Proceeds cannot execute... state: %d\n", state);
        break;
    }
}
// connecting process <<<<<-----------------------------------

void wget_post_cb(char **buffer, int offset, int datend, int *buflen, void *arg)
{
    printf("Offset: %d\n", offset);
    printf("Data end: %d\n", datend);
    printf("Buffer length: %d\n", *buflen);
}

int send2beam(const char *msg)
{
    int ret = 0;
    char buffer[1024];

    ret = wget_post(SORACOM_BEAM_URL, msg, buffer, 1024, wget_post_cb, NULL);
    if (ret < 0)
    {
        printf("Failed to send message to beam\n");
    }
    return ret;
}

int send2harvest(const char *msg)
{
    int ret = 0;
    char buffer[256];

    ret = wget_post(SORACOM_HARVEST_URL, msg, buffer, 256, wget_post_cb, NULL);
    if (ret < 0)
    {
        printf("Failed to send message to harvest\n");
    }
    return ret;
}