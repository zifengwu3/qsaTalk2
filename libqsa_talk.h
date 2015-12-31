#include "libqsa_def.h"

#ifndef _LIB_QSA_TALK_H
#define _LIB_QSA_TALK_H
/*
 * Method:    init_param_task(struct dev_config *);
 * Back:      void
 */
void init_param_task(struct dev_config * q_config);

/*
 * Method:    start_call
 * Signature: (char *, char  *)V
 */
void start_call(const char * ip, const char * addr);

/*
 * Method:    stop_call
 * Signature: ()V
 */
void stop_call(void);

/*
 * Method:    start_talk
 * Signature: ()V
 */
void start_talk(void);

/*
 * Method:    open_lock
 * Signature: ()V
 */
void open_lock(void);

/*
 * Method:    send_video
 * Signature: (char *, int, char *)V
 */
void send_video(const char * data, int length, const char * ip);

/*
 * Method:    send_audio
 * Signature: (char *, int, char *)V
 */
void send_audio(const char * data, int length, const char * ip);
#endif

