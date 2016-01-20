#include <stdio.h>
#include <stdlib.h>

#define _LIB_QSA_DEF_H
#include "include/libqsa_common.h"


/* default defined */
void default_cb_audio_data(void * data, int length, int type);
void default_cb_info(const char * data, const char * ip, int length, int port);
int default_cb_status(int type);
void default_cb_devip(const char * address, const char * ip, int uflag);
void default_cb_opt(int value);

void set_cb_function_null(struct _cb_function * p);
void set_cb_function(struct _cb_function * p);
int get_device_status(int uFlag);

void set_cb_function_null(struct _cb_function * p) {

    p->cb_audio_data = &default_cb_audio_data;
    p->cb_info = &default_cb_info;
    p->cb_curr_st = &default_cb_status;
    p->cb_devip = &default_cb_devip;
    p->cb_curr_opt = &default_cb_opt;

    return;
}

void set_cb_function(struct _cb_function * p) {

    if (p->cb_audio_data) {
        cb_opt_function.cb_audio_data = p->cb_audio_data;
    }

    if (p->cb_info) {
       cb_opt_function.cb_info = p->cb_info;
    }

    if (p->cb_curr_st) {
        cb_opt_function.cb_curr_st = p->cb_curr_st;
    }

    if (p->cb_devip) {
        cb_opt_function.cb_devip = p->cb_devip;
    }

    if (p->cb_curr_opt) {
        cb_opt_function.cb_curr_opt = p->cb_curr_opt;
    }

    return;
}

/* default defined */
void default_cb_audio_data(void * data, int length, int type) {
    printf("default_cb_audio_data\n");
    return;
}

void default_cb_info(const char * data, const char * ip, int length, int port) {
    printf("default_cb_info\n");
    return;
}

int default_cb_status(int type) {
    printf("default_cb_status\n");
    return CB_ST_NULL;
}

void default_cb_devip(const char * address, const char * ip, int uflag) {
    printf("default_cb_devip\n");
    return;
}

void default_cb_opt(int value) {
    printf("default_cb_opt\n");
    return;
}

int get_device_status(int uFlag) {
    return cb_opt_function.cb_curr_st(uFlag);
}


