#ifndef _LIB_QSA_TALK_H
#define _LIB_QSA_TALK_H

struct dev_config {
		unsigned char address[20];			//M0101010000100000000–20位
        unsigned char ip[4];				//192.168.10.XX
		unsigned char mask[4];				//255.255.0.0
		unsigned char route[4];				//192.168.10.1
		unsigned char center_ip[4];			//192.168.10.XX
		unsigned char server_ip[4]; 		//192.168.10.XX
		unsigned char undefined_ip[100];	//192.168.10.XX x 25组
		unsigned char undefined_param[100]; //未定义配置项
};

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

//CALLBACK Function
typedef void ( * _cb_data)(void *, int, int);
typedef void ( * _cb_info)(const char *, const char *, int);
typedef void ( * _cb_status)(int, int);

#endif

