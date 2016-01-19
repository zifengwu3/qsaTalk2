#include <inttypes.h>
#include <signal.h>
#include <semaphore.h>       //sem_t
#include <sys/stat.h>
#include <pthread.h>
#include "sndtools.h"

#define LIFT 110

#define _DEBUG           //µ÷ÊÔÄ£Ê½

//#define _TESTNSSERVER        //²âÊÔ·şÎñÆ÷½âÎöÄ£Ê½
//#define _TESTTRANS           //²âÊÔÊÓÆµÖĞ×ªÄ£Ê½

#define SOFTWAREVER "1.00.00"    

#define NSMULTIADDR  "238.9.9.1"//"192.168.10.255"  //NS×é²¥µØÖ·
#define LFTMULTIADDR  "238.9.9.2"//"192.168.10.255"  //NS×é²¥µØÖ·

#define ZOOMMAXTIME 2000   //·Å´óËõĞ¡ÑÓ³Ù´¦ÀíÊ±¼ä
#define TOUCHMAXTIME 300   //´¥ÃşÆÁ´¦ÀíÑÓ³Ù´¦ÀíÊ±¼ä

#define INTRTIME 50       //Ïß³Ì50ms
#define INTRPERSEC 20       //Ã¿Ãë20´ÎÏß³Ì
#define BUFFER_SIZE 1024
#define FRAMEBUFFERMAX  4
#define COMMMAX 1024     //´®¿Ú»º³åÇø×î´óÖµ

#define INFOROWLEN   32    //ĞÅÏ¢Ã¿ĞĞ³¤¶È
#define MAXROW  12          //×î´óĞĞÊı
#define PAGEPERROW  3          //Ò³ĞĞÊı
//#define PAGEPERROW  4          //Ò³ĞĞÊı
//
#define WATCHTIMEOUT  30*(1000/INTRTIME)    //¼àÊÓ×î³¤Ê±¼ä
#define CALLTIMEOUT  25*(1000/INTRTIME)     //ºô½Ğ×î³¤Ê±¼ä
#define TALKTIMEOUT  130*(1000/INTRTIME)//30*20     //Í¨»°×î³¤Ê±¼ä
#define PREPARETIMEOUT  10*(1000/INTRTIME)     //ÁôÓ°ÁôÑÔÔ¤±¸×î³¤Ê±¼ä
#define RECORDTIMEOUT  30*(1000/INTRTIME)     //ÁôÓ°ÁôÑÔ×î³¤Ê±¼ä

#define FORBIDTIMEOUT (1000/INTRTIME)
//ÃüÁî ¹ÜÀíÖĞĞÄ
#define ALARM         1    //±¨¾¯
#define CANCELALARM   2    //È¡Ïû±¨¾¯
#define SENDMESSAGE   3   //·¢ËÍĞÅÏ¢
#define REPORTSTATUS  4   //Éè±¸¶¨Ê±±¨¸æ×´Ì¬
#define QUERYSTATUS   5   //¹ÜÀíÖĞĞÄ²éÑ¯Éè±¸×´Ì¬
#define ALARMHANDLE   111
#define CHECKTIME     7
///////////paul0415
#define SENDPICINFO   6
#define PICINFONUM    8
//////////paul0812////////////////////////////
#define CHECKMAC	161
#define SETPARAM 	162
#define MAKEOLD		163
#define READMAC		164
#define MCUUPDATE	165
////////////////////
#define REMOTEDEFENCE   20   //Ô¶³Ì²¼·À
#define RESETPASS       30   //¸´Î»ÃÜÂë
#define WRITEADDRESS   40   //Ğ´µØÖ·ÉèÖÃ
#define READADDRESS    41   //¶ÁµØÖ·ÉèÖÃ
#define WRITEROOMSETUP     44   //Ğ´ÊÒÄÚ»ú¹¤³ÌÉèÖÃ
#define READROOMSETUP      45   //¶ÁÊÒÄÚ»ú¹¤³ÌÉèÖÃ
#define WRITESETUP     52   //Ğ´µ¥ÔªÃÅ¿Ú»ú¡¢Î§Ç½»úÉèÖÃĞÅÏ¢
#define READSETUP      53   //¶Áµ¥ÔªÃÅ¿Ú»ú¡¢Î§Ç½»úÉèÖÃĞÅÏ¢
//¶Ô½²
#define VIDEOTALK      150 //¾ÖÓòÍø¿ÉÊÓ¶Ô½²
#define VIDEOTALKTRANS 151 //¾ÖÓòÍø¿ÉÊÓ¶Ô½²ÖĞ×ª·şÎñ
#define VIDEOWATCH     152 //¾ÖÓòÍø¼à¿Ø
#define VIDEOWATCHTRANS   153 //¾ÖÓòÍø¼à¿ØÖĞ×ª·şÎñ
#define NSORDER        154 //Ö÷»úÃû½âÎö£¨×ÓÍøÄÚ¹ã²¥£©
#define NSSERVERORDER  155 //Ö÷»úÃû½âÎö(NS·şÎñÆ÷)
#define FINDEQUIP      170 //²éÕÒÉè±¸

#define ASK              1     //ÃüÁîÀàĞÍ Ö÷½Ğ
#define REPLY            2     //ÃüÁîÀàĞÍ Ó¦´ğ

#define CALL             1     //ºô½Ğ
#define LINEUSE          2     //Õ¼Ïß
#define QUERYFAIL        3      //Í¨ĞÅÊ§°Ü
#define CALLANSWER       4     //ºô½ĞÓ¦´ğ
#define CALLSTART        6     //¿ªÊ¼Í¨»°

#define CALLUP           7     //Í¨»°Êı¾İ1£¨Ö÷½Ğ·½->±»½Ğ·½£©
#define CALLDOWN         8     //Í¨»°Êı¾İ2£¨±»½Ğ·½->Ö÷½Ğ·½£©
#define CALLCONFIRM      9     //Í¨»°ÔÚÏßÈ·ÈÏ£¨½ÓÊÕ·½·¢ËÍ£¬ÒÔ±ã·¢ËÍ·½È·ÈÏÔÚÏß£©
#define REMOTEOPENLOCK   10     //Ô¶³Ì¿ªËø

#define FORCEIFRAME      11     //Ç¿ÖÆIÖ¡ÇëÇó
#define ZOOMOUT          15     //·Å´ó(720*480)
#define ZOOMIN           16     //ËõĞ¡(352*288)
#define OPENLOCK         17
#define CALLEND          30     //Í¨»°½áÊø
#define REMOTECALLLIFT   20
#define REMOTELIFT		 30
#define MAINPICNUM  24      //Ê×Ò³Í¼Æ¬ÊıÁ¿
#define MAINLABELNUM  2     //Ê×Ò³LabelÊıÁ¿
#define DOWNLOAD  220      //ÏÂÔØ
#define ASK  1
#define REPLY  2

#define SAVEMAX  50     //FLASH´æ´¢»º³å×î´óÖµ
#define UDPSENDMAX  50  //UDP¶à´Î·¢ËÍ»º³å×î´óÖµ
#define COMMSENDMAX  10  //COMM¶à´Î·¢ËÍ»º³å×î´óÖµ
#define MAXSENDNUM  6  //×î´ó·¢ËÍ´ÎÊı

//°´Å¥Ñ¹ÏÂÊ±¼ä
#define DELAYTIME  200
//°´Å¥ÊıÁ¿
#define InfoButtonMax  16
//¶ÌĞÅÏ¢////////////paul
#define INFOTYPENUM  4 //4    //¶ÌĞÅÏ¢ÀàĞÍ
#define INFOMAXITEM  50 //200    //¶ÌĞÅÏ¢×î´óÌõÊı
#define INFOMAXSIZE  400 //¶ÌĞÅÏ¢ÄÚÈİ×î´óÈİÁ¿
#define INFONUMPERPAGE 3  //Ò»Ò³ÏÔÊ¾ĞÅÏ¢Êı

#define NMAX 512*64  //AUDIOBLK*64  //ÒôÆµ»·ĞÎ»º³åÇø´óĞ¡
#define G711NUM  64*512/AUDIOBLK       //ÒôÆµ½ÓÊÕ»º³åÇø¸öÊı Î´½âÂë   10

//#define VIDEOMAX 720*480
#define VIDEOMAX 720*576
#define VNUM  3         //ÊÓÆµ²É¼¯»º³åÇø´óĞ¡
#define VPLAYNUM  10         //ÊÓÆµ²¥·Å»º³åÇø´óĞ¡         6
#define MP4VNUM  20         //ÊÓÆµ½ÓÊÕ»º³åÇø¸öÊı Î´½âÂë   10
#define PACKDATALEN  1200   //Êı¾İ°ü´óĞ¡
#define MAXPACKNUM  100     //Ö¡×î´óÊı¾İ°üÊıÁ¿

//////////////FOR UPDATE//////////////
struct downfile1
{
   char FlagText[20];     //±êÖ¾×Ö·û´®
   char FileName[20];
   unsigned int Filelen;            //ÎÄ¼ş´óĞ¡
   unsigned short TotalPackage;      //×Ü°üÊı
   unsigned short CurrPackage;       //µ±Ç°°üÊı
   unsigned short Datalen;           //Êı¾İ³¤¶È
}__attribute__ ((packed));
//////////////////////////////////////

struct TimeStamp1
{
    unsigned int OldCurrVideo;     //ÉÏÒ»´Îµ±Ç°ÊÓÆµÊ±¼ä
    unsigned int CurrVideo;
    unsigned int OldCurrAudio;     //ÉÏÒ»´Îµ±Ç°ÒôÆµÊ±¼ä
    unsigned int CurrAudio;
};
//ÊÓÆµ²É¼¯»º³å
struct videobuf1
{
    int iput; // »·ĞÎ»º³åÇøµÄµ±Ç°·ÅÈëÎ»ÖÃ
    int iget; // »º³åÇøµÄµ±Ç°È¡³öÎ»ÖÃ
    int n; // »·ĞÎ»º³åÇøÖĞµÄÔªËØ×ÜÊıÁ¿
    uint32_t timestamp[VNUM]; //Ê±¼ä´Á
    uint32_t frameno[VNUM];   //Ö¡ĞòºÅ
    unsigned char *buffer_y[VNUM];//[VIDEOMAX];
    unsigned char *buffer_u[VNUM];//[VIDEOMAX/4];
    unsigned char *buffer_v[VNUM];//[VIDEOMAX/4];
};
//ÊÓÆµ½ÓÊÕ»º³å  Î´½âÂë
struct tempvideobuf1
{
//  int iput;                     // »·ĞÎ»º³åÇøµÄµ±Ç°·ÅÈëÎ»ÖÃ
//  int iget;                     // »º³åÇøµÄµ±Ç°È¡³öÎ»ÖÃ
//  int n;                        // »·ĞÎ»º³åÇøÖĞµÄÔªËØ×ÜÊıÁ¿
    uint32_t timestamp;  //Ê±¼ä´Á
    uint32_t frameno;       //Ö¡ĞòºÅ
    short TotalPackage;     //×Ü°üÊı
    uint8_t CurrPackage[MAXPACKNUM]; //µ±Ç°°ü   1 ÒÑ½ÓÊÕ  0 Î´½ÓÊÕ
    int Len;                //Ö¡Êı¾İ³¤¶È
    uint8_t isFull;                  //¸ÃÖ¡ÒÑ½ÓÊÕÍêÈ«
    unsigned char *buffer;//[VIDEOMAX];
    unsigned char frame_flag;             //Ö¡±êÖ¾ ÒôÆµÖ¡ IÖ¡ PÖ¡
};                            //     [MP4VNUM]
//////////////paul0416

typedef struct temppicinfobuf1
{
    uint32_t frameno;       //Ö¡ĞòºÅ
    short TotalPackage;     //×Ü°üÊı
    uint8_t CurrPackage[MAXPACKNUM]; //µ±Ç°°ü   1 ÒÑ½ÓÊÕ  0 Î´½ÓÊÕ
    int Len;                //Ö¡Êı¾İ³¤¶È
    uint8_t isFull;                  //¸ÃÖ¡ÒÑ½ÓÊÕÍêÈ«
    unsigned char *buffer;//[VIDEOMAX];
    unsigned char frame_flag;             //1 for private 2 for common 3 for alarm 4 for picture
} TempPicInfoBuff1 ;
////////////////////////

//ÊÓÆµ½ÓÊÕ»º³å Á´±í
typedef struct node2
{
    struct tempvideobuf1 Content;
    struct node2 *llink, *rlink;
}TempVideoNode1;
//ÊÓÆµ²¥·Å»º³å
struct videoplaybuf1
{
    uint8_t isUse;     //¸ÃÖ¡ÒÑ½âÂëÎ´²¥·Å,»º³åÇø²»¿ÉÓÃ
    uint32_t timestamp; //Ê±¼ä´Á
    uint32_t frameno;   //Ö¡ĞòºÅ
    unsigned char *buffer;//[VIDEOMAX];
    unsigned char frame_flag;             //Ö¡±êÖ¾ ÒôÆµÖ¡ IÖ¡ PÖ¡
};
//Í¬²½²¥·Å½á¹¹
struct _SYNC
{
    pthread_cond_t cond;       //Í¬²½Ïß³ÌÌõ¼ş±äÁ¿
    pthread_condattr_t cond_attr;
    pthread_mutex_t lock;      //»¥³âËø
    pthread_mutex_t audio_rec_lock;//[VPLAYNUM];//ÒôÆµÂ¼ÖÆ»¥³âËø
    pthread_mutex_t audio_play_lock;//[VPLAYNUM];//ÒôÆµ²¥·Å»¥³âËø
    pthread_mutex_t video_rec_lock;//[VPLAYNUM];//ÊÓÆµÂ¼ÖÆ»¥³âËø
    pthread_mutex_t video_play_lock;//[VPLAYNUM];//ÊÓÆµ²¥·Å»¥³âËø
    pthread_mutex_t lmovie_lock;////added from door///paul0326//////
    unsigned int count;        //¼ÆÊı
    uint8_t isDecodeVideo;     //ÊÓÆµÒÑ½âÂëÒ»Ö¡  ½âÂëÏß³Ì-->Í¬²½Ïß³Ì
    uint8_t isPlayVideo;       //ÊÓÆµÒÑ²¥·ÅÒ»Ö¡  ²¥·ÅÏß³Ì-->Í¬²½Ïß³Ì
    uint8_t isDecodeAudio;     //ÒôÆµÒÑ½âÂëÒ»Ö¡  ½âÂëÏß³Ì-->Í¬²½Ïß³Ì
    uint8_t isPlayAudio;       //ÒôÆµÒÑ²¥·ÅÒ»Ö¡  ²¥·ÅÏß³Ì-->Í¬²½Ïß³Ì
};

//¼Ó»º³åËø?
struct audiobuf1
{
    int iput; // »·ĞÎ»º³åÇøµÄµ±Ç°·ÅÈëÎ»ÖÃ
    int iget; // »º³åÇøµÄµ±Ç°È¡³öÎ»ÖÃ
    int n; // »·ĞÎ»º³åÇøÖĞµÄÔªËØ×ÜÊıÁ¿
    uint32_t timestamp[NMAX/AUDIOBLK]; //Ê±¼ä´Á
    uint32_t frameno[NMAX/AUDIOBLK];   //Ö¡ĞòºÅ
    unsigned char buffer[NMAX];
};

//ÒôÆµ½ÓÊÕ»º³å  Î´½âÂë
struct tempaudiobuf1
{
    uint32_t timestamp;  //Ê±¼ä´Á
    uint32_t frameno;       //Ö¡ĞòºÅ
    short TotalPackage;     //×Ü°üÊı
    uint8_t CurrPackage[MAXPACKNUM]; //µ±Ç°°ü   1 ÒÑ½ÓÊÕ  0 Î´½ÓÊÕ
    int Len;                //Ö¡Êı¾İ³¤¶È
    uint8_t isFull;                  //¸ÃÖ¡ÒÑ½ÓÊÕÍêÈ«
    unsigned char *buffer;//[AUDIOBLK];
    unsigned char frame_flag;             //Ö¡±êÖ¾ ÒôÆµÖ¡ IÖ¡ PÖ¡
};                            //     [MP4VNUM]
//ÒôÆµ½ÓÊÕ»º³å Á´±í
typedef struct node3
{
    struct tempaudiobuf1 Content;
    struct node3 *llink, *rlink;
}TempAudioNode1;

//ÒôÆµ²¥·Å»º³å
struct audioplaybuf1
{
    uint8_t isUse;     //¸ÃÖ¡ÒÑ½âÂëÎ´²¥·Å,»º³åÇø²»¿ÉÓÃ
    uint32_t timestamp; //Ê±¼ä´Á
    uint32_t frameno;   //Ö¡ĞòºÅ
    unsigned char *buffer;//[VIDEOMAX];
};

//¼ÒÍ¥ÁôÑÔ»º³å
struct wavbuf1
{
    int iput; // »·ĞÎ»º³åÇøµÄµ±Ç°·ÅÈëÎ»ÖÃ
    int iget; // »º³åÇøµÄµ±Ç°È¡³öÎ»ÖÃ
    int n; // »·ĞÎ»º³åÇøÖĞµÄÔªËØ×ÜÊıÁ¿
    unsigned char *buffer;
};

struct WaveFileHeader
{
    char chRIFF[4];
    uint32_t dwRIFFLen;
    char chWAVE[4];

    char chFMT[4];
    uint32_t dwFMTLen;
    uint16_t wFormatTag;
    uint16_t nChannels;
    uint32_t nSamplesPerSec;
    uint32_t nAvgBytesPerSec;
    uint16_t nBlockAlign;
    uint16_t wBitsPerSample;

    char chFACT[4];
    uint32_t dwFACTLen;

    char chDATA[4];
    uint32_t dwDATALen;
};

////////////copy from sound/////paul0326//////////////
/*struct flcd_data
{
    unsigned int buf_len;
    unsigned int uv_offset;
    unsigned int frame_no;
    unsigned int mp4_map_dma[VIDEO_PICTURE_QUEUE_SIZE_MAX];
};*/

struct Local1 {
    int Status;
    //×´Ì¬ 0 ¿ÕÏĞ 1 Ö÷½Ğ¶Ô½²  2 ±»½Ğ¶Ô½²  3 ¼àÊÓ  4 ±»¼àÊÓ  5 Ö÷½ĞÍ¨»°
    //6 ±»½ĞÍ¨»°
	int RecordPic;  //ÁôÕÕÆ¬  0 ²»Áô  1 ºô½ĞÁôÕÕÆ¬  2 Í¨»°ÁôÕÕÆ¬
    unsigned char IP_Group[4];  //×é²¥µØÖ·

    int CallConfirmFlag; //ÔÚÏß±êÖ¾
    int Timer1Num;  //¶¨Ê±Æ÷1¼ÆÊı
    int OnlineFlag; //Ğè¼ì²éÔÚÏßÈ·ÈÏ
    int OnlineNum;  //ÔÚÏßÈ·ÈÏĞòºÅ
    int TimeOut;    //¼àÊÓ³¬Ê±,  Í¨»°³¬Ê±,  ºô½Ğ³¬Ê±£¬ÎŞÈË½ÓÌı
    int TalkTimeOut; //Í¨»°×î³¤Ê±¼ä

    pthread_mutex_t udp_lock;//»¥³âËø
};

/*
struct Local1
{
    int Status;
    //×´Ì¬ 0 ¿ÕÏĞ 1 Ö÷½Ğ¶Ô½²  2 ±»½Ğ¶Ô½²  3 ¼àÊÓ  4 ±»¼àÊÓ  5 Ö÷½ĞÍ¨»°
    //6 ±»½ĞÍ¨»°
    int KillStatus;
	int RecordPic;  //ÁôÕÕÆ¬  0 ²»Áô  1 ºô½ĞÁôÕÕÆ¬  2 Í¨»°ÁôÕÕÆ¬
    int IFrameCount; //IÖ¡¼ÆÊı
    int IFrameNo;    //ÁôµÚ¼¸¸öIÖ¡
    unsigned char yuv[2][D1_W*D1_H*3/2];
    /////unsigned char yuv[2][CIF_W*CIF_H*3/2];
    int HavePicRecorded;  //ÓĞÕÕÆ¬ÒÑÂ¼ÖÆ
    struct tm *recpic_tm_t; //ÁôÕÕÆ¬Ê±¼ä

///////////////////////paul////////////////////////////////////////
    int again;
	char Center[2];
	char udporder;//the current order from udp
    char commorder;
    char FromIP[20];
    unsigned char udpsend_buf[1520];
    unsigned char commsend_buf[100];
    char Addr[20];
	unsigned char infoAddr[20];
	unsigned char picAddr[4];//ÁôÓ°
	int alarmAddr[2];
	unsigned char OldAddr[20];
	char takepic;
	unsigned char VideoAddr[20];
	unsigned char AudioAddr[20];
	unsigned char KillRemote[20];
	unsigned char KillAddr[20];
	unsigned char KillIP[4];
	char interrupted;
    char updatemcu; 
	int Floor;
	int Room;

//	char MaxFloor;
//	char MaxRoom;
/////////////////////////////////////////////////////////////
    ///////////////0326//////////from door/////////////
    int DoorNo;
    char ClockText[20];
    int InputMax;
    int OpenDoorFlag;
    int OpenDoorTime;

    ////////////
    struct tm *call_tm_t; //±»ºô½ĞÊ±¼ä

    int CallConfirmFlag; //ÔÚÏß±êÖ¾
    int Timer1Num;  //¶¨Ê±Æ÷1¼ÆÊı
    int OnlineFlag; //Ğè¼ì²éÔÚÏßÈ·ÈÏ
    int OnlineNum;  //ÔÚÏßÈ·ÈÏĞòºÅ
    int TimeOut;    //¼àÊÓ³¬Ê±,  Í¨»°³¬Ê±,  ºô½Ğ³¬Ê±£¬ÎŞÈË½ÓÌı
    int TalkTimeOut; //Í¨»°×î³¤Ê±¼ä
    int RecPicSize;  //ÊÓÆµ´óĞ¡  1  352*288   2  720*480
    int PlayPicSize;  //ÊÓÆµ´óĞ¡  1  352*288   2  720*480
    ///////////mj
	int ForbTimeOut;
	pthread_mutex_t save_lock;//»¥³âËø
    pthread_mutex_t udp_lock;//»¥³âËø
    pthread_mutex_t comm_lock;//»¥³âËø
    int PrevWindow;      //ÉÏÒ»¸ö´°¿Ú±àºÅ
    int TmpWindow;       //Ôİ´æ´°¿Ú±àºÅ ÓÃÓÚµ¯³ö´°¿ÚÊ±
    int CurrentWindow;   //µ±Ç°´°¿Ú±àºÅ
    int DefenceDelayFlag;    //²¼·ÀÑÓÊ±±êÖ¾
    int DefenceDelayTime;   //¼ÆÊı
    int PassLen;            //ÃÜÂë³¤¶È
    int AlarmDelayFlag[2];    //±¨¾¯ÑÓÊ±±êÖ¾
    int AlarmDelayTime[2];   //¼ÆÊı

    int ForceIFrame;    //1 Ç¿ÖÆIÖ¡
    int CalibratePos;   //Ğ£×¼´¥ÃşÆÁÊ®×ÖÎ»ÖÃ 0 1 2 3
    int CalibrateSucc;  //Ğ£×¼³É¹¦
    int CurrFbPage; //µ±Ç°FbÒ³
    unsigned char IP_Group[4];  //×é²¥µØÖ·
    unsigned char Weather[3];   //ÌìÆøÔ¤±¨

    int AddrLen;          //µØÖ·³¤¶È  S 12  B 6 M 8 H 6

    int isHost;           //'0' Ö÷»ú '1' ¸±»ú '2' ...
    int ConnToHost;       //ÓëÖ÷»úÁ¬½ÓÕı³£ 1 Õı³£ 0 ²»Õı³£
    unsigned char HostIP[4]; //Ö÷»úIP
    unsigned char HostAddr[21]; //Ö÷»úAddr
    int DenNum;             //Ä¿±êÊıÁ¿  ¸±»ú
    unsigned char DenIP[10][4];    //¸±»úIP
    char DenAddr[10][21];         //¸±»úAddr

    int NetStatus;   //ÍøÂç×´Ì¬ 1 ¶Ï¿ª  0 ½ÓÍ¨
    int OldNetSpeed;  //ÍøÂçËÙ¶È
    int NoBreak;     //ÃâÈÅ×´Ì¬ 1 ÃâÈÅ  0 Õı³£

    int ReportSend;  //Éè±¸¶¨Ê±±¨¸æ×´Ì¬ÒÑ·¢ËÍ
    int RandReportTime; //Éè±¸¶¨Ê±±¨¸æ×´Ì¬Ëæ»úÊ±¼ä
    int ReportTimeNum;  //¼ÆÊ±
    //ÔÚGPIOÏß³ÌÖĞ²éÑ¯¸÷Ïß³ÌÊÇ·ñÔËĞĞ
    int Key_Press_Run_Flag;
    int Save_File_Run_Flag;
    int Dispart_Send_Run_Flag;
    int Multi_Send_Run_Flag;
    int Multi_Comm_Send_Run_Flag;

    int MenuIndex;     //µ±Ç°°´Å¥Ë÷Òı
    int MaxIndex;      //±¾½çÃæ×î´óË÷Òı
    int MainMenuIndex;     //Ö÷½çÃæ°´Å¥Ë÷Òı

    int OsdOpened;  //OSD´ò¿ª±êÖ¾

    int LcdLightFlag; //LCD±³¹â±êÖ¾
    int LcdLightTime; //Ê±¼ä

	//int NewINfo;
    int NewInfo[FLOOR][ROOM];  //ÓĞĞÂĞÅÏ¢

    int ResetPlayRingFlag;  //¸´Î»Audio Play flag

    int nowvideoframeno;   //µ±Ç°ÊÓÆµÖ¡±àºÅ
    int nowaudioframeno;   //µ±Ç°ÒôÆµÖ¡±àºÅ

    int ForceEndWatch;  //ÓĞºô½ĞÊ±£¬Ç¿ÖÆ¹Ø¼àÊÓ
    int ZoomInOutFlag;  //ÕıÔÚ·Å´óËõĞ¡ÖĞ
    uint32_t newzoomtime;
    uint32_t oldzoomtime;
    uint32_t newtouchtime;
    uint32_t oldtouchtime;    //ÉÏÒ»´Î´¥ÃşÆÁ´¦ÀíÊ±¼ä
};
*/

struct LocalCfg1
{
    char Addr[20];             //µØÖ·±àÂë
    unsigned char BuildAddr[2];
	unsigned char Mac_Addr[6]; //Íø¿¨µØÖ·
    unsigned char IP[4];       //IPµØÖ·
    unsigned char IP_Mask[4];  //×ÓÍøÑÚÂë
    unsigned char IP_Gate[4];  //Íø¹ØµØÖ·
    unsigned char IP_NS[4];    //NS£¨Ãû³Æ½âÎö£©·şÎñÆ÷µØÖ·
    unsigned char IP_Server[4];  //Ö÷·şÎñÆ÷µØÖ·£¨ÓëNS·şÎñÆ÷¿ÉÎªÍ¬Ò»¸ö£©
    unsigned char IP_Broadcast[4];  //¹ã²¥µØÖ·

    int ReportTime;      //Éè±¸¶¨Ê±±¨¸æ×´Ì¬Ê±¼ä
    unsigned char DefenceStatus;       //²¼·À×´Ì¬
    unsigned char DefenceNum;          //·ÀÇøÄ£¿é¸öÊı
    unsigned char DefenceInfo[32][10]; //·ÀÇøĞÅÏ¢

    char EngineerPass[10];             //¹¤³ÌÃÜÂë
    char OpenLockPass[10];

    int In_DelayTime;                //½øÈëÑÓÊ±
    int Out_DelayTime;               //Íâ³öÑÓÊ±
    int Alarm_DelayTime;               //±¨¾¯ÑÓÊ±

/////////////////////////0326////////////////from door/////////
    unsigned char OpenLockTime;
    unsigned char DelayLockTime;
    unsigned char PassOpenLock;
    unsigned char CardOpenLock;

    unsigned char bit_rate;
    ///////////////////////////////////////////////////
    int Ts_X0;                   //´¥ÃşÆÁ
    int Ts_Y0;
    int Ts_deltaX;
    int Ts_deltaY;
};

struct Remote1
{
    int DenNum;             //Ä¿±êÊıÁ¿  Ö÷»ú+¸±»ú
    unsigned char DenIP[4]; //¶Ô·½IP»òÊÓÆµ·şÎñÆ÷IP
    unsigned char GroupIP[4]; //GroupIP
    unsigned char IP[10][4];    //¶Ô·½IP
    int Added[10];                //ÒÑ¼ÓÈë×é
    char Addr[10][21];         //¶Ô·½Addr
    int isDirect;       //ÊÇ·ñÖ±Í¨  0 Ö±Í¨  1 ÖĞ×ª
};

struct Info1
{
    int MaxNum;   //×î´óĞÅÏ¢Êı
    int TotalNum; //ĞÅÏ¢×ÜÊı
    int NoReadedNum; //Î´¶ÁĞÅÏ¢×ÜÊı
    int TotalInfoPage;   //×ÜĞÅÏ¢Ò³Êı
    int CurrentInfoPage; //µ±Ç°ĞÅÏ¢Ò³
    int CurrNo;    //µ±Ç°ĞÅÏ¢ĞòºÅ
    int CurrPlayNo;  //µ±Ç°²¥·ÅĞòºÅ
    int TimeNum;    //¼ÆÊı
};

//µ¥ÌõĞÅÏ¢ÄÚÈİ½á¹¹Ìå
struct InfoContent1
{
    uint8_t isValid;  //ÓĞĞ§£¬Î´É¾³ı±êÖ¾   1
    uint8_t isReaded; //ÒÑ¶Á±êÖ¾    1
    uint8_t isLocked; //Ëø¶¨±êÖ¾    1
    char Time[32];    //½ÓÊÕÊ±¼ä    32
    uint8_t Type;     //ÀàĞÍ        1    ĞÅÏ¢ÀàĞÍ»òÊÂ¼şÀàĞÍ
    uint32_t Sn;      //ĞòºÅ        4
    int Length;       //³¤¶È        4
    char Content[INFOMAXSIZE];//ÄÚÈİ  400  ÄÚÈİ»òÊÂ¼ş¶ÔÏó
    //char Event[20];         //ÊÂ¼ş
    char FileName[50];
};                               //ÄÚ´æ·ÖÅäÎª444

//////////////////paul0509
struct AlarmInfoContent1
{
    uint8_t isValid;  //ÓĞĞ§£¬Î´É¾³ı±êÖ¾   1
    uint8_t isReaded; //ÒÑ¶Á±êÖ¾    1
    uint8_t isLocked; //Ëø¶¨±êÖ¾    1
    char Time[32];    //½ÓÊÕÊ±¼ä    32
    uint8_t Type;     //ÀàĞÍ        1    ĞÅÏ¢ÀàĞÍ»òÊÂ¼şÀàĞÍ
//    uint32_t Sn;      //ĞòºÅ        4
//    int Length;       //³¤¶È        4
//    char Content[INFOMAXSIZE];//ÄÚÈİ  400  ÄÚÈİ»òÊÂ¼ş¶ÔÏó
    //char Event[20];         //ÊÂ¼ş
//    char FileName[50];
//	char AlarmInfo[20]; ///paul0509±¨¾¯ĞÅÏ¢
}; 
////////////////////////////////////////////
//µ±Ç°ĞÅÏ¢´°¿Ú×´Ì¬
struct InfoStatus1
{
    int CurrType;  //µ±Ç°ĞÅÏ¢ÀàĞÍ
    int CurrWin;   //µ±Ç°ĞÅÏ¢´°¿Ú  0 ĞÅÏ¢ÁĞ±í  1  ĞÅÏ¢ÄÚÈİ
    int CurrNo;    //µ±Ç°ĞÅÏ¢ĞòºÅ
};
//ĞÅÏ¢Á´±í
typedef struct node
{
    struct InfoContent1 Content;
    struct node *llink, *rlink;
}InfoNode1;
///////////////paul0509
typedef struct alarmnode
{
	struct AlarmInfoContent1 Content;
	struct alarmnode *llink,*rlink;
}AlarmInfoNode1;
////////////////////////////////////////////////////////////////
//´æ´¢ÎÄ¼şµ½FLASH¶ÓÁĞ Êı¾İ½á¹¹ ÓÉÓÚ´æ´¢FLASHËÙ¶È½ÏÂı ÓÃÏß³ÌÀ´²Ù×÷
struct Save_File_Buff1
{
    int isValid; //ÊÇ·ñÓĞĞ§
    int Type;    //´æ´¢ÀàĞÍ 1£­Ò»ÀàĞÅÏ¢  2£­µ¥¸öĞÅÏ¢  3£­ÎïÒµ·şÎñ  4£­±¾µØÉèÖÃ
    int InfoType;   //ĞÅÏ¢ÀàĞÍ
    int InfoNo;     //ĞÅÏ¢Î»ÖÃ
    InfoNode1 *Info_Node; //ĞÅÏ¢½áµã
};

//UDPÖ÷¶¯ÃüÁîÊı¾İ·¢ËÍ½á¹¹
struct Multi_Udp_Buff1
{
    int isValid; //ÊÇ·ñÓĞĞ§
    int SendNum; //µ±Ç°·¢ËÍ´ÎÊı
    int CurrOrder;//µ±Ç°ÃüÁî×´Ì¬,VIDEOTALK VIDEOTALKTRANS VIDEOWATCH VIDEOWATCHTRANS
    //Ö÷ÒªÓÃÓÚĞè½âÎöÊ±£¬Èçµ¥´ÎÃüÁîÖÃÎª0
    int m_Socket;
    char RemoteHost[20];
    unsigned char buf[1500];
    int DelayTime;  //µÈ´ıÊ±¼ä
    int SendDelayTime;
    int nlength;
};

//COMMÖ÷¶¯ÃüÁîÊı¾İ·¢ËÍ½á¹¹
struct Multi_Comm_Buff1
{
    int isValid; //ÊÇ·ñÓĞĞ§
    int SendNum; //µ±Ç°·¢ËÍ´ÎÊı
    int m_Comm;
    unsigned char buf[1500];
    int nlength;
//	int udporder;
//	int commorder;
};

//Í¨»°Êı¾İ½á¹¹
struct talkdata1
{
    char HostAddr[20];       //Ö÷½Ğ·½ØÖ·
    unsigned char HostIP[4]; //Ö÷½Ğ·½IPµØÖ·
    char AssiAddr[20];       //±»½Ğ·½µØÖ·
    unsigned char AssiIP[4]; //±»½Ğ·½IPµØÖ·
    unsigned int timestamp;  //Ê±¼ä´Á
    unsigned short DataType;          //Êı¾İÀàĞÍ
    unsigned short Frameno;           //Ö¡ĞòºÅ
    unsigned int Framelen;            //Ö¡Êı¾İ³¤¶È
    unsigned short TotalPackage;      //×Ü°üÊı
    unsigned short CurrPackage;       //µ±Ç°°üÊı
    unsigned short Datalen;           //Êı¾İ³¤¶È
    unsigned short PackLen;       //Êı¾İ°ü´óĞ¡
}__attribute__ ((packed));
//ĞÅÏ¢Êı¾İ½á¹¹
struct infodata1
{
    char Addr[20];       //µØÖ·±àÂë
    unsigned short Type; //ÀàĞÍ
//  	unsigned char Type;
	unsigned int  Sn;         //ĞòºÅ
    unsigned short Length;   //Êı¾İ³¤¶È
}__attribute__ ((packed));

#ifndef CommonH
#define CommonH
int DebugMode;           //µ÷ÊÔÄ£Ê½
int DeltaLen;  //Êı¾İ°üÓĞĞ§Êı¾İÆ«ÒÆÁ¿
struct tm *curr_tm_t;
struct TimeStamp1 TimeStamp;  //½ÓÊÕÊ±¼äÓë²¥·ÅÊ±¼ä£¬Í¬²½ÓÃ
int temp_video_n;      //ÊÓÆµ½ÓÊÕ»º³å¸öÊı
TempVideoNode1 *TempVideoNode_h;    //ÊÓÆµ½ÓÊÕ»º³åÁĞ±í
int temp_audio_n;      //ÒôÆµ½ÓÊÕ»º³å¸öÊı
TempAudioNode1 *TempAudioNode_h;    //ÒôÆµ½ÓÊÕ»º³åÁĞ±í

//ÏµÍ³³õÊ¼»¯±êÖ¾
int InitSuccFlag;
//±¾»ú×´Ì¬ÉèÖÃ
struct Local1 Local;
struct LocalCfg1 LocalCfg;

//Ô¶¶ËµØÖ·
struct Remote1 Remote;
char null_addr[21];   //¿Õ×Ö·û´®
char null_ip[4];   //¿Õ×Ö·û´®
//Ãâ·ÑARP
int ARP_Socket;
//¼ì²âÍøÂçÁ¬½Ó
int m_EthSocket;
//UDP
int m_DataSocket;
int m_VideoSocket;

int LocalDataPort;   //ÃüÁî¼°Êı¾İUDP¶Ë¿Ú
int LocalVideoPort;  //ÒôÊÓÆµUDP¶Ë¿Ú

int RemoteDataPort;
int RemoteVideoPort;
char RemoteHost[20];
char sPath[80];
char currpath[80];   //×Ô¶¨ÒåÂ·¾¶
char wavPath[80];
char UdpPackageHead[15];

//Ö÷¶¯ÃüÁîÊı¾İ·¢ËÍÏß³Ì£ºÖÕ¶ËÖ÷¶¯·¢ËÍÃüÁî£¬ÈçÑÓÊ±Ò»¶ÎÃ»ÊÕµ½»ØÓ¦£¬Ôò¶à´Î·¢ËÍ
//ÓÃÓÚUDPºÍCommÍ¨ĞÅ
int multi_send_flag;
pthread_t multi_send_thread;
void multi_send_thread_func(void);
sem_t multi_send_sem;
struct Multi_Udp_Buff1 Multi_Udp_Buff[UDPSENDMAX]; //10¸öUDPÖ÷¶¯·¢ËÍ»º³å

#else
extern int DebugMode;           //µ÷ÊÔÄ£Ê½
extern int DeltaLen;  //Êı¾İ°üÓĞĞ§Êı¾İÆ«ÒÆÁ¿
extern struct tm *curr_tm_t;
extern struct TimeStamp1 TimeStamp;  //½ÓÊÕÊ±¼äÓë²¥·ÅÊ±¼ä£¬Í¬²½ÓÃ
extern int temp_video_n;      //ÊÓÆµ½ÓÊÕ»º³å¸öÊı
extern TempVideoNode1 *TempVideoNode_h;    //ÊÓÆµ½ÓÊÕ»º³åÁĞ±í
extern int temp_audio_n;      //ÒôÆµ½ÓÊÕ»º³å¸öÊı
extern TempAudioNode1 *TempAudioNode_h;    //ÒôÆµ½ÓÊÕ»º³åÁĞ±í

//ÏµÍ³³õÊ¼»¯±êÖ¾
extern int InitSuccFlag;
//±¾»ú×´Ì¬ÉèÖÃ
extern struct Local1 Local;
extern struct LocalCfg1 LocalCfg;

//Ô¶¶ËµØÖ·
extern struct Remote1 Remote;
extern char null_addr[21];   //¿Õ×Ö·û´®
extern char null_ip[4];   //¿Õ×Ö·û´®
//Ãâ·ÑARP
extern int ARP_Socket;
//¼ì²âÍøÂçÁ¬½Ó
extern int m_EthSocket;
//UDP
extern int m_DataSocket;
extern int m_VideoSocket;

extern int LocalDataPort;   //ÃüÁî¼°Êı¾İUDP¶Ë¿Ú
extern int LocalVideoPort;  //ÒôÊÓÆµUDP¶Ë¿Ú

extern int RemoteDataPort;
extern int RemoteVideoPort;
extern char RemoteHost[20];
extern char sPath[80];
extern char currpath[80];   //×Ô¶¨ÒåÂ·¾¶
extern char wavPath[80];
extern char UdpPackageHead[15];

//Ö÷¶¯ÃüÁîÊı¾İ·¢ËÍÏß³Ì£ºÖÕ¶ËÖ÷¶¯·¢ËÍÃüÁî£¬ÈçÑÓÊ±Ò»¶ÎÃ»ÊÕµ½»ØÓ¦£¬Ôò¶à´Î·¢ËÍ
//ÓÃÓÚUDPÍ¨ĞÅ
extern int multi_send_flag;
extern pthread_t multi_send_thread;
extern void multi_send_thread_func(void);
extern sem_t multi_send_sem;
extern struct Multi_Udp_Buff1 Multi_Udp_Buff[UDPSENDMAX]; //10¸öUDPÖ÷¶¯·¢ËÍ»º³å
#endif
