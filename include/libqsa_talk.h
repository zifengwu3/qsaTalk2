#ifndef _LIB_QSA_TALK_H
#define _LIB_QSA_TALK_H
/*
 * Method:    start_call
 * Signature: (char *, char  *, int)V
 */
void start_call(const char * ip, const char * addr, int uFlag);

/*
 * Method:    stop_call
 * Signature: ()V
 */
void stop_call(void);

/*
 * Method:    find_ip
 * Signature: (char *, int)V
 */
void find_ip(const char * addr, int uFlag);

/*
 * Method:    send_video
 * Signature: (char *, int, int, int, char *)V
 */
void send_video(const char * data, int length, int frame_num, int frame_type, const char * ip);

/*
 * Method:    send_audio
 * Signature: (char *, int, int, char *)V
 */
void send_audio(const char * data, int length, int frame_num, const char * ip);
#endif

