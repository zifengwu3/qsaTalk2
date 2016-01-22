#include <stdio.h>
#include <stdlib.h>

#define _LIB_QSA_DEF_H
#include "include/libqsa_common.h"


/* default defined */
void default_cb_audio_data(void * data, int length, int type);
void default_cb_info(const void * data, const void * ip, int length, int port);
int default_cb_status(int type);
void default_cb_devip(const void * address, const void * ip, int uflag);
void default_cb_opt(int value);

void set_cb_function_default(struct _cb_function * p);
void set_cb_function(struct _cb_function * p);
int get_device_status(int uFlag);

void set_cb_function_default(struct _cb_function * p) {

    p->cb_audio_data = &default_cb_audio_data;
    p->cb_info = &default_cb_info;
    p->cb_curr_st = &default_cb_status;
    p->cb_devip = &default_cb_devip;
    p->cb_curr_opt = &default_cb_opt;

    return;
}

void set_cb_function(struct _cb_function * p) {

    if (p->cb_audio_data) {
        printf("p->cb_audio_data\n");
        cb_opt_function.cb_audio_data = p->cb_audio_data;
    } else {
        printf("p->cb_audio_data is NULL\n");
    }

    if (p->cb_info) {
        printf("p->cb_info\n");
       cb_opt_function.cb_info = p->cb_info;
    } else {
        printf("p->cb_info is NULL\n");
    }
 

    if (p->cb_curr_st) {
        printf("p->cb_curr_st\n");
        cb_opt_function.cb_curr_st = p->cb_curr_st;
    } else {
        printf("p->cb_curr_st is NULL\n");
    }

    if (p->cb_devip) {
        printf("p->cb_devip\n");
        cb_opt_function.cb_devip = p->cb_devip;
    } else {
        printf("p->cb_devip is NULL\n");
    }

    if (p->cb_curr_opt) {
        printf("p->cb_curr_opt\n");
        cb_opt_function.cb_curr_opt = p->cb_curr_opt;
    } else {
        printf("p->cb_curr_opt is NULL\n");
    }

    return;
}

/* default defined */
void default_cb_audio_data(void * data, int length, int type) {
    printf("default_cb_audio_data\n");
    return;
}

void default_cb_info(const void * data, const void * ip, int length, int port) {
    printf("default_cb_info\n");
    return;
}

int default_cb_status(int type) {
    printf("default_cb_status\n");
    return CB_ST_NULL;
}

void default_cb_devip(const void * address, const void * ip, int uflag) {
    printf("default_cb_devip\n");
    return;
}

void default_cb_opt(int value) {
    printf("default_cb_opt\n");
    return;
}

int get_device_status(int uFlag) {
    if (cb_opt_function.cb_curr_st != NULL) {
        return cb_opt_function.cb_curr_st(uFlag);
    } else {
        printf("call back fail, use default value <0>\n");
        return CB_ST_NULL;
    }
}


