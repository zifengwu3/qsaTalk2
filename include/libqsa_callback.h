
//CALLBACK Function
typedef void ( * _cb_audio_data)(void *, int, int);
typedef void ( * _cb_info)(const char *, const char *, int, int);
typedef int ( * _cb_status)(int);
typedef void ( * _cb_devip)(const char *, const char *, int);
typedef void ( * _cb_opt)(int);

struct _cb_function{
    _cb_audio_data cb_audio_data;
    _cb_status cb_curr_st;
    _cb_opt cb_curr_opt;
    _cb_devip cb_devip;
    _cb_info cb_info;
};

extern int get_device_status(int uFlag);

extern void set_cb_function(struct _cb_function * p);
/* find ip of the device */
extern void find_ip(const char * addr, int uFlag);
/* start call a user device */
extern void start_call(const char * ip, const char * addr, int uFlag);
/* stop a call or a talk */
extern void stop_talk(void);
/* send the public message or the private message */
extern void send_info(const char * data, struct _send_info * info);
/* send audio or video data to the user device */
extern void send_audio(const char * data, int length, int frame_num, const char * ip);
extern void send_video(const char * data, int length, int frame_num, int frame_type, const char * ip);


