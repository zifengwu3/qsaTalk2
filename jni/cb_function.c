#include <stdio.h>
#include <stdlib.h>

#define _LIB_QSA_DEF_H
#include "libqsa_common.h"


/* default defined */
void default_cb_audio_data(void * data, int length, int type);
void default_cb_info(const void * data, const void * ip, int length, int port);
void default_cb_devip(const void * address, const void * ip, int uFlag);
void default_cb_opt(int value, int status);

void set_cb_function_default(struct _cb_function * p);
void set_cb_function(struct _cb_function * p);
int get_device_status(void);
void set_device_status(int uStatus);

void set_cb_function_default(struct _cb_function * p) {

    if (!(p->cb_audio_data)) {
        p->cb_audio_data = &default_cb_audio_data;
    }

    if (!(p->cb_info)) {
        p->cb_info = &default_cb_info;
    }

    if (!(p->cb_devip)) {
        p->cb_devip = &default_cb_devip;
    }

    if (!(p->cb_curr_opt)) {
        p->cb_curr_opt = &default_cb_opt;
    }

    return;
}

void set_cb_function(struct _cb_function * p) {

    if (p->cb_audio_data) {
        LOGD("p->cb_audio_data is OK\n");
        cb_opt_function.cb_audio_data = p->cb_audio_data;
    } else {
        LOGD("p->cb_audio_data is NULL\n");
    }

    if (p->cb_info) {
        LOGD("p->cb_info is OK\n");
       cb_opt_function.cb_info = p->cb_info;
    } else {
        LOGD("p->cb_info is NULL\n");
    }
 
    if (p->cb_devip) {
        LOGD("p->cb_devip is OK\n");
        cb_opt_function.cb_devip = p->cb_devip;
    } else {
        LOGD("p->cb_devip is NULL\n");
    }

    if (p->cb_curr_opt) {
        LOGD("p->cb_curr_opt is OK\n");
        cb_opt_function.cb_curr_opt = p->cb_curr_opt;
    } else {
        LOGD("p->cb_curr_opt is NULL\n");
    }

    return;
}

/* default defined */
void default_cb_audio_data(void * data, int length, int type) {
    LOGD("default_cb_audio_data\n");
    return;
}

void default_cb_info(const void * data, const void * ip, int length, int port) {
    LOGD("default_cb_info\n");
    return;
}

void default_cb_devip(const void * address, const void * ip, int uFlag) {
    LOGD("default_cb_devip\n");
    return;
}

void default_cb_opt(int value, int status) {
    LOGD("default_cb_opt\n");
    return;
}

int get_device_status(void) {
    LOGD("%s: Status = %d\n", __FUNCTION__, Local.Status);
    return Local.Status;
}

void set_device_status(int uStatus) {
    Local.Status = uStatus;
}



