#include <stdio.h>
#include <stdlib.h>

#include "../include/libqsa_callback.h"
extern int g_Status;
extern unsigned char g_addr[30];
extern unsigned char g_ip[20];

/* default defined */
void default_cb_audio_data1(void * data, int length, int type) {
    printf("default_cb_audio_data1\n");
    return;
}

void default_cb_info1(const void * data, const void * ip, int length, int port) {
    printf("default_cb_info1\n");
    return;
}

int default_cb_status1(int type) {
    printf("%s : type = %d, g_Status = %d\n", __FUNCTION__, type, g_Status);
    return g_Status;
}

void default_cb_devip1(const void * address, const void * ip, int uflag) {
    int rtnval = 0;
    printf("default_cb_devip1\n");
    
    rtnval = memcmp(g_addr, address, 4);
    if (rtnval == 0) {
        memcpy(g_ip, ip, 4);
    }
    printf("ip:%d.%d.%d.%d\n", g_ip[0], g_ip[1], g_ip[2], g_ip[3]);
    return;
}

void default_cb_opt1(int value) {
    printf("<%s> value = %d\n", __FUNCTION__, value);

    switch (value) {
        case CB_CALL_OK:
            g_Status = CB_ST_CALLING;
            break;
        case CB_CALL_BUSY:
            g_Status = CB_ST_NULL;
            break;
        case CB_TALK_OK:
            g_Status = CB_ST_TALKING;
            break;
        case CB_TALK_STOP:
            g_Status = CB_ST_NULL;
            break;
        case CB_OPEN_LOCK:
            break;
        case CB_FORCE_IFRAME:
            printf("force iframe\n");
            break;
        case CB_CALL_FAIL:
            g_Status = CB_ST_NULL;
            break;
        case CB_TALK_TIMEOUT:     
            g_Status = CB_ST_NULL;
            break;
        case CB_CALL_TIMEOUT:
            g_Status = CB_ST_NULL;
            break;
        case CB_ACK_TIMEOUT: 
            g_Status = CB_ST_NULL;
            break;
        default:
            break;
    }

    printf("g_Status = %d\n", g_Status);

    return;
}
