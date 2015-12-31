//CALLBACK Function
typedef void ( * _cb_data)(void *, int, int);
typedef void ( * _cb_info)(const char *, const char *, int);
typedef void ( * _cb_status)(int, int);

static struct _cb_function {
    _cb_data cb_audio_data;         //回调声音参数
    _cb_status cb_curr_status;      //当前呼叫状态
    _cb_info cb_info;               //转发消息给其他服务器
};

void set_cb_function(struct _cb_function * p);


