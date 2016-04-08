/* fmpeg4_avcodec.c
   It is sample code for ffmpeg application usage, to define AVCodec structure.
   Note it's not a part of fmpeg driver, but collect int the same folder
   */
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/timeb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/ipc.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/types.h>
#include <linux/videodev.h>
#include <errno.h>
#include <linux/fb.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/un.h>
#include <signal.h>
#include <semaphore.h>       //sem_t

#define CommonH
#include "common.h"
//#include "/usr/src/ffmpeg-0.4.8/libSDL/include/SDL_thread.h"

#include "fmpeg4_avcodec.h"
#include "ratecontrol.h"

#define MP4_ROIX 0
#define MP4_ROIY 0
#define FMPEG4_DECODER_DEV  "/dev/fdec"
#define FMPEG4_ENCODER_DEV  "/dev/fenc"
#define VIDEO_PICTURE_QUEUE_SIZE_MAX 20

int fmpeg4_dec_fd=0;
int fmpeg4_enc_fd=0;
//unsigned char *dec_mmap_addr;

extern struct Local1 Local;
int                     video_fd=-1;
struct video_mbuf       gb_buffers;
struct video_mmap       gb_buf;
struct video_capability video_cap;
struct video_window     win;
struct video_tuner      tuner;
unsigned char           *video_buf_virt;
unsigned char           *out_virt_buffer;
unsigned int            mmap_done=0;
int                     frame_num = 0;
int                     fidx=0;
RateControl     ratec;

int v4l_init(video_profile *profile,int seq);
int v4l_close(void);
////////paul2.0
char PriMulCast[20];

//��Ƶ
struct videobuf1 videorecbuf;    //��Ƶ�ɼ����λ���  ����?

int video_rec_flag;
int sync_play_flag;
extern int audio_play_flag;
pthread_t video_rec_deal_thread;      //��Ƶ�ɼ����ݴ����߳�
pthread_t video_rec_thread;      //��Ƶ�ɼ��߳�

void video_rec_deal_thread_func(void);
void video_rec_thread_func(void);

sem_t videorecsem;
sem_t videorec2playsem;
struct _SYNC sync_s;

struct timeval ref_time;  //��׼ʱ��,��Ƶ����Ƶ��һ֡
//�������
video_profile   enc_video_setting;
//�������
video_profile   dec_video_setting;

extern sem_t audioplaysem;
extern struct audiobuf1 playbuf;    //��Ƶ���Ż��λ���

int curr_video_timestamp;       //��ǰ�Ѳ�����Ƶ֡ʱ���
int curr_audio_timestamp;       //��ǰ�Ѳ�����Ƶ֡ʱ���

#define USE_MMAP
void StartRecVideo(int width, int height);
void StopRecVideo(void);
//Ϊ��ֹ��β������´���
int VideoRecIsStart=0;

//Ŀ¼���ļ�����
// *h�����ͷ����ָ�룬*pָ��ǰ����ǰһ����㣬*sָ��ǰ���

TempVideoNode1 * init_videonode(void); //��ʼ��������ĺ���
//������������������
int length_videonode(TempVideoNode1 *h);
//����������β�����
int creat_videonode(TempVideoNode1 *h, struct talkdata1 talkdata,
			unsigned char *r_buf , int r_length);
int add_videonode(TempVideoNode1 *p, struct talkdata1 talkdata,
			unsigned char *r_buf , int r_length);
//��Ӱ����
int creat_leavemovienode(TempVideoNode1 *h, uint32_t rframeno, uint32_t rtimestamp,
			unsigned short rframe_flag, unsigned char *r_buf , int r_length);
//����������ɾ������
int delete_videonode(TempVideoNode1 *p);
int delete_all_videonode(TempVideoNode1 *h);
int delete_lost_videonode(TempVideoNode1 *h, uint32_t currframeno, uint32_t currtimestamp); //ɾ����ȫ֡
//������������λ����
TempVideoNode1 * locate_videonode(TempVideoNode1 *h,int i);
//�������������Һ���
TempVideoNode1 * find_videonode(TempVideoNode1 *h, int currframeno, int currpackage);
//�������������Һ���
TempVideoNode1 * find_frame_videonode(TempVideoNode1 *h, int currframeno);
//�������ϵ�֡
TempVideoNode1 * search_videonode(TempVideoNode1 *h);
//
int free_videonode(TempVideoNode1 *h);
//---------------------------------------------------------------------------
//mpeg4 encode
/* seq=0 for 2D capture, seq=1 for sequential */
int v4l_init(video_profile *profile,int seq)
{
	int type,ingrab;
	int uframerate;
	video_fd = open("/dev/video0", O_RDWR);

	if(video_fd < 0)    {
		printf("open device failure");
		return -1;
	}
	//    tuner.mode = VIDEO_MODE_NTSC;
	if (!ioctl(video_fd, VIDIOCGTUNER, &tuner))    {
		tuner.mode = VIDEO_MODE_NTSC;
		ioctl(video_fd, VIDIOCSTUNER, &tuner);
	}
#ifdef _DEBUG
	printf("gb_buffers.size1=%d\n",gb_buffers.size);
#endif  
	if ( ioctl(video_fd,VIDIOCGMBUF,&gb_buffers) < 0 )	{
		printf("capture not support mbuf\n");
		return -1;
	}
#ifdef _DEBUG
	printf("gb_buffers.size2=%d\n",gb_buffers.size);
#endif  

	if(mmap_done==0)	{
		mmap_done = 1;
		video_buf_virt = mmap(0, gb_buffers.size, PROT_READ|PROT_WRITE, MAP_SHARED, video_fd, 0);
	}
	//ѡͨ��Ƶ
	//doors101112
	//	if((Local.DoorNo >= 0x31)&&(Local.DoorNo <= 0x34))
	//		ingrab = Local.DoorNo - 0x31 + 7; //using 7113 Composite
	//	else
	{
		ingrab = 7;
		//printf("7113 Local.DoorNo = %d error \n", Local.DoorNo);
	}
	ioctl(video_fd, 0x5679, &ingrab);            //grab type
	type = seq;
	//JPEG need sequential format
	// 0 for Mpeg4 2D YUV
	// 1 for JPG Sequencoal YUV
	// 2 for H.264 2D
	ioctl(video_fd, 0x5678, &type); //JPEG need sequential format

	uframerate =  profile->framerate;
	uframerate = 15;
	printf("uframerate = %d\n", uframerate);
	ioctl(video_fd, 0x5680, &uframerate); //set framerate

	gb_buf.height=profile->height;
	gb_buf.width=profile->width;
	gb_buf.format=VIDEO_PALETTE_YUV420P;//VIDEO_PALETTE_RAW;//;
	gb_buf.frame=0;
	return 0;
}

int v4l_close(void)
{
	munmap(video_buf_virt, gb_buffers.size);
	close(video_fd);
	mmap_done=0;
	video_fd = -1;
}
//---------------------------------------------------------------------------
int enc_init(video_profile *video_setting)
{
	FMP4_ENC_PARAM  enc_param;

	if(fmpeg4_enc_fd == 0)
	  fmpeg4_enc_fd = open( FMPEG4_ENCODER_DEV,O_RDWR );

	if(fmpeg4_enc_fd == 0)
	{
		printf("Fail to open %s\n",FMPEG4_ENCODER_DEV);
		return -1;
	}


	//    mmap_addr  = mmap(0, (video_setting->width * video_setting->height * 3) / 2,PROT_READ|PROT_WRITE,MAP_SHARED,fmpeg4_enc_fd, 0);
	//    if(mmap_addr <= 0)
	//        return -1;

	memset(&enc_param, 0, sizeof(FMP4_ENC_PARAM));
	enc_param.u32API_version = 0x00020002;//0x00020002;//MP4VER;
	enc_param.u32BitRate=video_setting->bit_rate;
	enc_param.u32FrameWidth=video_setting->width;
	enc_param.u32FrameHeight=video_setting->height;
	enc_param.fFrameRate=(float)video_setting->framerate/(float)video_setting->frame_rate_base;
	enc_param.u32InitialQuant=video_setting->quant;
	enc_param.u32IPInterval=video_setting->gop_size;
#ifdef _DEBUG
	printf("enc_param.u32IPInterval=%d\n" ,enc_param.u32IPInterval);
#endif  
	enc_param.bEnable4MV=0;
	enc_param.bH263Quant=1;

#if 0 //def H263
	enc_param.bShortHeader=1;
#else
	enc_param.bShortHeader=0;
#endif
	enc_param.bResyncMarker=0;
	enc_param.u32MaxQuant=video_setting->qmax;
	enc_param.u32MinQuant=video_setting->qmin;

	enc_param.bROIEnable   = video_setting->roi_enable;     //0:disable 1:enable
	enc_param.u32ROIX      = video_setting->roi_x;     // must be a multiple of 16 pixels
	enc_param.u32ROIY      = video_setting->roi_y;     // must be a multiple of 16 pixels
	enc_param.u32ROIWidth  = video_setting->roi_width;   // must be a multiple of 16 pixels
	enc_param.u32ROIHeight = video_setting->roi_height;  // must be a multiple of 16 pixels
	//enc_param.motion_dection_enable=0;


	RateControlInit(&ratec,enc_param.u32BitRate*1000, DELAY_FACTOR, RC_AVERAGE_PERIOD,
				ARG_RC_BUFFER, get_framerate(enc_param.fFrameRate),
				enc_param.u32MaxQuant,enc_param.u32MinQuant,enc_param.u32InitialQuant);

	if (  ioctl(fmpeg4_enc_fd,FMPEG4_IOCTL_ENCODE_INIT,&enc_param) < 0)
	{
		close(fmpeg4_enc_fd);
		printf("Error to set FMPEG4_IOCTL_INIT\n");
		return -1;
	}

	return 0;
}


//---------------------------------------------------------------------------
int enc_close(video_profile *video_setting)
{
	if(fmpeg4_enc_fd)
	  close(fmpeg4_enc_fd);
	fmpeg4_enc_fd = 0;
	//	free(video_setting->coded_frame);
	return 0;
}
//---------------------------------------------------------------------------
int mpeg4_encode(video_profile *video_setting, unsigned char *frame, unsigned char * data_y,
			unsigned char * data_u, unsigned char * data_v)
{
	//    AVFrame             *pict=(AVFrame *)data;
	GM_ENC_FRAME   enc_frame;

	if ( video_setting->roi_enable) {
		enc_frame.roi_X = video_setting->roi_x;
		enc_frame.roi_Y = video_setting->roi_y;
	}
	enc_frame.pu8YFrameBaseAddr = data_y;//(unsigned char *)pict->data[0];   //input user continued virtual address (Y), Y=0 when NVOP
	enc_frame.pu8UFrameBaseAddr = data_u;//(unsigned char *)pict->data[1];   //input user continued virtual address (U)
	enc_frame.pu8VFrameBaseAddr = data_v;//(unsigned char *)pict->data[2];   //input user continued virtual address (V)
	enc_frame.bitstream = frame;  //output User Virtual address
	enc_frame.quant = ratec.rtn_quant;

	enc_frame.intra = video_setting->intra;
	if (  ioctl(fmpeg4_enc_fd, FMPEG4_IOCTL_ENCODE_FRAME, &enc_frame) <0)
	{
		printf("Error to set FMPEG4_IOCTL_ENCODE_FRAME\n");
		return -1;
	}

	RateControlUpdate(&ratec, enc_frame.quant, enc_frame.length, enc_frame.intra);

	video_setting->intra = enc_frame.intra; //the return value in enc_frame.intra
	//    printf("enc_frame.intra=%d\n", enc_frame.intra);

	return enc_frame.length;
}
//---------------------------------------------------------------------------
int capture_video(video_profile *video)
{
	//	unsigned int        blength;
	// 	AVFrame             yuv_data;
	struct timeval tv;
	uint32_t nowtime,prevtime;

	uint8_t             *ptr;

	/* Setup to capture the next frame */
	if ((errno=ioctl(video_fd, VIDIOCMCAPTURE, &gb_buf)) < 0) 
	{
		return;
	}

	while (ioctl(video_fd, VIDIOCSYNC, &frame_num) < 0 &&(errno == EAGAIN || errno == EINTR))
	  printf("error\n");

	ptr = video_buf_virt + gb_buffers.offsets[frame_num];
	frame_num = gb_buf.frame;


	videorecbuf.buffer_y[videorecbuf.iput] = (unsigned char *)(ptr);      //?


	if(video->height % 16 != 0)
	  video->height = 16 * ((video->height / 16) + 1 );

	if(video->width % 16 != 0)
	  video->width = 16 * ((video->width / 16) + 1 );

	// UV    	
	videorecbuf.buffer_u[videorecbuf.iput] = (unsigned char *) (ptr + (video->width * video->height));
	videorecbuf.buffer_v[videorecbuf.iput] = (unsigned char *) (ptr + (video->width * video->height * 5 / 4));

	return 0;
}
//---------------------------------------------------------------------------
int mpeg4_encode_init(int enc_width, int enc_height, int framerate)
{
	int ret;
	int i;
	int blength;

	enc_video_setting.roi_enable = 0;
	//set the default value
	enc_video_setting.qmax = 31;
	enc_video_setting.qmin = 1;
	enc_video_setting.quant = 4;
	if(Local.RecPicSize == 2)       //D1
	  enc_video_setting.bit_rate = 1024;
	else
	{
		if(LocalCfg.bit_rate > 5)
		  LocalCfg.bit_rate = 3;
		enc_video_setting.bit_rate = Cif_Bit_rate[LocalCfg.bit_rate];
	}

	enc_video_setting.bit_rate = 1024;
	printf("enc_video_setting.bit_rate = %d\n", enc_video_setting.bit_rate);
	enc_video_setting.width = enc_width;
	enc_video_setting.height = enc_height;
	enc_video_setting.framerate = framerate;
	enc_video_setting.frame_rate_base = 1;
	enc_video_setting.gop_size = 50;

	enc_video_setting.roi_x = MP4_ROIX;
	enc_video_setting.roi_y = MP4_ROIY;
	enc_video_setting.roi_width = 320;
	enc_video_setting.roi_height = 240;

	if (enc_video_setting.roi_enable )
	  printf("roi width %d height %d\n", enc_video_setting.roi_width, enc_video_setting.roi_height);
	else
	  printf("frame width %d height %d\n", enc_video_setting.width, enc_video_setting.height);

	out_virt_buffer = (unsigned char *) malloc(enc_video_setting.width*enc_video_setting.height*3/2);
	//1 for mpeg4  ԭ���������
	//JPEG need sequential format
	// 0 for Mpeg4 2D YUV
	// 1 for JPG Sequencoal YUV
	// 2 for H.264 2D
	//    if((ret=v4l_init(&enc_video_setting,1)) < 0)
	if((ret=v4l_init(&enc_video_setting,0)) < 0)
	  return -1;

	if(enc_init(&enc_video_setting)<0)
	  return -1;

	return 0;
}
//---------------------------------------------------------------------------
void StartRecVideo(int width, int height)
{
	pthread_attr_t attr;
	int i, j;
	int framerate;
	if(VideoRecIsStart == 0)
	{
		VideoRecIsStart = 1;
		if((width == D1_W)&&(height == D1_H))
		{
			Local.RecPicSize = 2;      //720*240
			framerate = 5;
		}
		else
		{
			Local.RecPicSize = 1;      //352*288
			framerate = 30;
		}
		//��ͨ��Ƶ
		////paul  
		usleep(100*1000);   //����ͷ�ϵ磬�ȴ���Ƶ�ȶ�
		//usleep(1000*1000);

		if(VideoRecIsStart == 0)
		  return;
		//��Ƶ�ɼ�������
		//  if(mpeg4_encode_init(720, 480) !=0 )
		framerate = 25;
		if(mpeg4_encode_init(width, height, framerate) !=0 )
		{
			printf("mpeg4_encode_init error!\\n");
			return;
		}

		Local.nowvideoframeno = 1; 
		//��Ƶ�ɼ�����
		videorecbuf.iput = 0;
		videorecbuf.iget = 0;
		videorecbuf.n = 0;
		for(i=0; i<VNUM; i++)
		{
			videorecbuf.frameno[i] = 0;
			videorecbuf.timestamp[i] = 0;
		}

		sem_init(&videorecsem,0,0);
		pthread_mutex_init (&sync_s.video_rec_lock, NULL);

		pthread_mutex_init (&sync_s.lmovie_lock, NULL);

		video_rec_flag = 1;   //��Ƶ�ɼ���־
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
		pthread_create(&video_rec_deal_thread,&attr,(void *)video_rec_deal_thread_func,NULL);
		if ( video_rec_deal_thread == 0 ) {
			printf("�޷�������Ƶ�ɼ����ݴ����߳�\n");
			pthread_attr_destroy(&attr);
			return;
		}
		pthread_create(&video_rec_thread,&attr,(void *)video_rec_thread_func,NULL);
		pthread_attr_destroy(&attr);
		if ( video_rec_thread == 0 ) {
			printf("�޷�������Ƶ�ɼ��߳�\n");
			return;
		}
	}
	else
	{
#ifdef _DEBUG
		printf("�ظ� VideoRecStart\n");
#endif
	}
}
//---------------------------------------------------------------------------
void StopRecVideo(void)
{
	int i;
	int delaytime;
	delaytime=40;
	//delaytime=120;
	if(VideoRecIsStart == 1)
	{
		video_rec_flag = 0;   //��Ƶ�ɼ���־
		usleep(delaytime*1000);
		if(pthread_cancel(video_rec_thread) ==0)
		  printf("video_rec_thread cancel success\n");
		else
		  printf("video_rec_thread cancel fail\n");
		usleep(delaytime*1000);
		if(pthread_cancel(video_rec_deal_thread) ==0)
		  printf("video_rec_deal_thread cancel success\n");
		else
		  printf("video_rec_deal_thread cancel fail\n");
		usleep(delaytime*1000);
		sem_destroy(&videorecsem);
		pthread_mutex_destroy(&sync_s.video_rec_lock);
		pthread_mutex_destroy(&sync_s.lmovie_lock);    

		//����
		v4l_close();
		enc_close(&enc_video_setting);
		free(out_virt_buffer);
		VideoRecIsStart = 0;
	}
	else
	{
#ifdef _DEBUG
		printf("�ظ� VideoRecStop\n");
#endif
	}
}
//---------------------------------------------------------------------------
//MPEG4���뼰�����߳�
void video_rec_deal_thread_func(void)
{
	int dwSize;
	int i,j,k;
	int TotalPackage; //�ܰ���
	int FrameNum;
	unsigned char mpeg4_out[1600];
	//������
	int rect1width,rect1height;
	int got_picture;
	int ytop,xleft;
	char RemoteHost[20];
	//ͨ�����ݽṹ
	struct talkdata1 talkdata;
	ytop = 0;
	xleft = 0;
	got_picture=0;

#ifdef _DEBUG
	printf("video_rec_deal_thread_func�� status%d,remoteaddr%s\n", Local.Status, Remote.Addr[0] );
#endif
	////////////////////
	//	while(Local.Status == 0 );
	memcpy(mpeg4_out, UdpPackageHead, 6);
	//����

	if((Local.Status == 1)||(Local.Status == 2)||(Local.Status == 5)||(Local.Status == 6)
				||(Local.Status == 7)||(Local.Status == 8)||(Local.Status == 9)||(Local.Status == 10))  //�Խ�
	{
		if(Remote.isDirect == 1)
		  mpeg4_out[6] = VIDEOTALKTRANS;
		else
		  mpeg4_out[6] = VIDEOTALK;
	}
	if((Local.Status == 3)||(Local.Status == 4))  //����
	{
		if(Remote.isDirect == 1)
		  mpeg4_out[6] = VIDEOWATCHTRANS;
		else
		  mpeg4_out[6] = VIDEOWATCH;
	}

	mpeg4_out[7] = 1;
	//������
	if((Local.Status == 1)||(Local.Status == 3)||(Local.Status == 5)||(Local.Status == 7)||(Local.Status == 9))  //����Ϊ���з�
	{
		mpeg4_out[8] = CALLUP;
		memcpy(talkdata.HostAddr, LocalCfg.Addr, 20);
		memcpy(talkdata.HostIP, LocalCfg.IP, 4);
		memcpy(talkdata.AssiAddr, Remote.Addr[0], 20);
		if((mpeg4_out[6] == VIDEOTALKTRANS)||(mpeg4_out[6] == VIDEOWATCHTRANS))
		  memcpy(talkdata.AssiIP, Remote.DenIP, 4);
		else
		  memcpy(talkdata.AssiIP, Remote.IP[0], 4);
	}
	if((Local.Status == 2)||(Local.Status == 4)||(Local.Status == 6)||(Local.Status == 8)||(Local.Status == 10))  //����Ϊ���з�
	{
		mpeg4_out[8] = CALLDOWN;
		memcpy(talkdata.HostAddr, Remote.Addr[0], 20);
		if((mpeg4_out[6] == VIDEOTALKTRANS)||(mpeg4_out[6] == VIDEOWATCHTRANS))
		  memcpy(talkdata.HostIP, Remote.DenIP, 4);
		else
		  memcpy(talkdata.HostIP, Remote.IP[0], 4);
		memcpy(talkdata.AssiAddr, LocalCfg.Addr, 20);
		memcpy(talkdata.AssiIP, LocalCfg.IP, 4);
	}


	//////////////////////
	while(video_rec_flag == 1)
	{
		//�ȴ��ɼ��߳������ݵ��ź�
		sem_wait(&videorecsem);
		//   printf("recsem =\n");
		//����
		pthread_mutex_lock(&sync_s.video_rec_lock);
		FrameNum = videorecbuf.n;
		//   for(i=0; i<videorecbuf.n; i++)
		while(videorecbuf.n > 0)
		  //   if(videorecbuf.n > 0)
		{
			//ͷ��
			/*	
				memcpy(mpeg4_out, UdpPackageHead, 6);
			//����

			if((Local.Status == 1)||(Local.Status == 2)||(Local.Status == 5)||(Local.Status == 6)
			||(Local.Status == 7)||(Local.Status == 8)||(Local.Status == 9)||(Local.Status == 10))  //�Խ�
			{
			if(Remote.isDirect == 1)
			mpeg4_out[6] = VIDEOTALKTRANS;
			else
			mpeg4_out[6] = VIDEOTALK;
			}
			if((Local.Status == 3)||(Local.Status == 4))  //����
			{
			if(Remote.isDirect == 1)
			mpeg4_out[6] = VIDEOWATCHTRANS;
			else
			mpeg4_out[6] = VIDEOWATCH;
			}

			mpeg4_out[7] = 1;
			//������
			if((Local.Status == 1)||(Local.Status == 3)||(Local.Status == 5)||(Local.Status == 7)||(Local.Status == 9))  //����Ϊ���з�
			{
			mpeg4_out[8] = CALLUP;
			memcpy(talkdata.HostAddr, LocalCfg.Addr, 20);
			memcpy(talkdata.HostIP, LocalCfg.IP, 4);
			memcpy(talkdata.AssiAddr, Remote.Addr[0], 20);
			if((mpeg4_out[6] == VIDEOTALKTRANS)||(mpeg4_out[6] == VIDEOWATCHTRANS))
			memcpy(talkdata.AssiIP, Remote.DenIP, 4);
			else
			memcpy(talkdata.AssiIP, Remote.IP[0], 4);
			}
			if((Local.Status == 2)||(Local.Status == 4)||(Local.Status == 6)||(Local.Status == 8)||(Local.Status == 10))  //����Ϊ���з�
			{
			mpeg4_out[8] = CALLDOWN;
			memcpy(talkdata.HostAddr, Remote.Addr[0], 20);
			if((mpeg4_out[6] == VIDEOTALKTRANS)||(mpeg4_out[6] == VIDEOWATCHTRANS))
			memcpy(talkdata.HostIP, Remote.DenIP, 4);
			else
			memcpy(talkdata.HostIP, Remote.IP[0], 4);
			memcpy(talkdata.AssiAddr, LocalCfg.Addr, 20);
			memcpy(talkdata.AssiIP, LocalCfg.IP, 4);
			}
			*/
			//ʱ���
			talkdata.timestamp = videorecbuf.timestamp[videorecbuf.iget];
			//֡���
			talkdata.Frameno = videorecbuf.frameno[videorecbuf.iget];

			if(Local.ForceIFrame == 1)
			{
				enc_video_setting.intra = 1;
				Local.ForceIFrame = 0;
			}
			else
			  enc_video_setting.intra = -1;
			dwSize=mpeg4_encode(&enc_video_setting, out_virt_buffer, videorecbuf.buffer_y[videorecbuf.iget],
						videorecbuf.buffer_u[videorecbuf.iget], videorecbuf.buffer_v[videorecbuf.iget]);
			//֡���ݳ���
			talkdata.Framelen = dwSize;
			//I֡
			if(enc_video_setting.intra == 1)
			{
				//��������
				if(Local.RecPicSize == 2)
				  talkdata.DataType = 4;  //720*480 I֡
				else
				  talkdata.DataType = 2;   //352*240 I֡
			}
			else
			{
				//��������
				if(Local.RecPicSize == 2)
				  talkdata.DataType = 5;   //720*480 P֡
				else
				  talkdata.DataType = 3;   //352*240 P֡
			}

			if((videorecbuf.iget + 1) >= VNUM)
			  videorecbuf.iget = 0;
			else
			  videorecbuf.iget ++;
			videorecbuf.n --;

			if(Local.Status != 0)
			{
				talkdata.PackLen = PACKDATALEN;
				//�ܰ���
				if((dwSize%talkdata.PackLen)==0)
				  TotalPackage=dwSize/talkdata.PackLen;
				else
				  TotalPackage=dwSize/talkdata.PackLen+1;
				talkdata.TotalPackage = TotalPackage;
				sprintf(RemoteHost, "%d.%d.%d.%d\0",Remote.DenIP[0],
							Remote.DenIP[1],Remote.DenIP[2],Remote.DenIP[3]);

				for(j=1; j<=TotalPackage; j++)         //����˳��Ӵ�С
				{
					//usleep(100*1000);
					if(j==TotalPackage)
					{
						talkdata.CurrPackage = j;      //��ǰ��
						talkdata.Datalen = dwSize-(j-1)*talkdata.PackLen;     //���ݳ���
						memcpy(mpeg4_out +DeltaLen, out_virt_buffer + (j-1)*talkdata.PackLen, (dwSize-(j-1)*talkdata.PackLen));
						///paul2.x
						if(Local.Status == 1)
						{

							//	memcpy(talkdata.AssiAddr, TalkDen.Addr[0], 20);
							//	memset(talkdata.AssiIP, 0, 4);
							memcpy(mpeg4_out + 9, &talkdata, sizeof(talkdata));
                            if ((Remote.Addr[0][0] == 'S' || Remote.Addr[0][0] == 's') && (TurnToCenter == 0))
							  strcpy(RemoteHost, PriMulCast);
						}
						else
						{
							memcpy(mpeg4_out + 9, &talkdata, sizeof(talkdata));
							sprintf(RemoteHost, "%d.%d.%d.%d\0",Remote.DenIP[0],Remote.DenIP[1],Remote.DenIP[2],Remote.DenIP[3]);

						}

						//UDP����
						UdpSendBuff(m_VideoSendSocket, RemoteHost, mpeg4_out , DeltaLen + (dwSize-(j-1)*talkdata.PackLen));
					}
					else
					{
						talkdata.CurrPackage = j;           //��ǰ��
						talkdata.Datalen = talkdata.PackLen;       //���ݳ���
						memcpy(mpeg4_out +DeltaLen, out_virt_buffer + (j-1)*talkdata.PackLen, talkdata.PackLen);

						if(Local.Status == 1)
						{
							//UDP����
							//	memcpy(talkdata.AssiAddr, TalkDen.Addr[0], 20);
							//	memset(talkdata.AssiIP, 0, 4);
							memcpy(mpeg4_out + 9, &talkdata, sizeof(talkdata));
                            if ((Remote.Addr[0][0] == 'S' || Remote.Addr[0][0] == 's') && (TurnToCenter == 0))
							  strcpy(RemoteHost, PriMulCast);
						}
						else
						{
							memcpy(mpeg4_out + 9, &talkdata, sizeof(talkdata));
							sprintf(RemoteHost, "%d.%d.%d.%d\0",Remote.DenIP[0],
										Remote.DenIP[1],Remote.DenIP[2],Remote.DenIP[3]);

							//UDP����
						}
						/*
						 * if(Local.Status==1)
						 *	UdpSendBuff(m_PrivateSocket, RemoteHost, mpeg4_out , DeltaLen + talkdata.PackLen);
						 *else
						 */
						UdpSendBuff(m_VideoSendSocket, RemoteHost, mpeg4_out , DeltaLen + talkdata.PackLen);
						//printf("Remote.Addr[0][0]:%c\n",Remote.Addr[0][0]);
						//printf("Sending video to %s\n",RemoteHost);

					}
				}
			}
		}
		//����
		pthread_mutex_unlock(&sync_s.video_rec_lock);
	}
}
//---------------------------------------------------------------------------
void video_rec_thread_func(void)
{
	struct timeval tv;
	uint32_t nowtime,prevtime;
	int dwSize;
#ifdef _DEBUG
	printf("������Ƶ�ɼ��̣߳�\n" );
	printf("video_rec_flag=%d\n",video_rec_flag);
#endif
	gettimeofday(&tv, NULL);
	nowtime = tv.tv_sec *1000 + tv.tv_usec/1000;
	prevtime = nowtime;
	while(video_rec_flag == 1)
	{
		//����
		pthread_mutex_lock(&sync_s.video_rec_lock);

		videorecbuf.frameno[videorecbuf.iput] = Local.nowvideoframeno;
		Local.nowvideoframeno++;
		if(Local.nowvideoframeno >= 65536)
		  Local.nowvideoframeno = 1;

		//ʱ���
		gettimeofday(&tv, NULL);
		//��һ֡,�趨��ʼʱ���
		if((ref_time.tv_sec ==0)&&(ref_time.tv_usec ==0))
		{
			ref_time.tv_sec = tv.tv_sec;
			ref_time.tv_usec = tv.tv_usec;
		}
		nowtime = (tv.tv_sec - ref_time.tv_sec) *1000 + (tv.tv_usec - ref_time.tv_usec)/1000;
		videorecbuf.timestamp[videorecbuf.iput] = nowtime;
		//    printf("nowframeno=%d, videorecbuf.timestamp=%d\n", nowframeno, videorecbuf.timestamp[videorecbuf.iput]);

		dwSize = capture_video(&enc_video_setting);

		//    printf("Video 0 quant %d blength= %d\n",ratec.rtn_quant,dwSize);

		if((videorecbuf.iput + 1) >= VNUM)
		  videorecbuf.iput = 0;
		else
		  videorecbuf.iput ++;
		if(videorecbuf.n < VNUM)
		  videorecbuf.n ++ ;
		else
		{
			sem_post(&videorecsem);
			printf("videorecbuf.Buffer is full\n");
			///0910
			usleep(10);
		}

		//����
		pthread_mutex_unlock(&sync_s.video_rec_lock);
		sem_post(&videorecsem);
	}
}
//---------------------------------------------------------------------------
TempVideoNode1 * init_videonode(void) //��ʼ��������ĺ���
{
	TempVideoNode1 *h; // *h�����ͷ����ָ�룬*pָ��ǰ����ǰһ����㣬*sָ��ǰ���
	if((h=(TempVideoNode1 *)malloc(sizeof(TempVideoNode1)))==NULL) //����ռ䲢���
	{
		printf("���ܷ����ڴ�ռ�!");
		return NULL;
	}
	h->llink=NULL; //������
	h->rlink=NULL; //������
	return(h);
}
//---------------------------------------------------------------------------
//�������ƣ�creat
//����������������β���������
//�������ͣ��޷���ֵ
//���������� h:������ͷָ��
int creat_videonode(TempVideoNode1 *h, struct talkdata1 talkdata,
			unsigned char *r_buf , int r_length)
{
	TempVideoNode1 *t;
	TempVideoNode1 *p;
	int j;
	int DataOk;
	//    uint32_t newframeno;
	//    int currpackage;
	t=h;
	//  t=h->next;
	while(t->rlink!=NULL)    //ѭ����ֱ��tָ���
	  t=t->rlink;   //tָ����һ���
	DataOk = 1;
	if((talkdata.DataType < 1) || (talkdata.DataType > 5))
	  DataOk = 0;
	if(talkdata.Framelen > VIDEOMAX)
	  DataOk = 0;
	if(talkdata.CurrPackage > talkdata.TotalPackage)
	  DataOk = 0;
	if(t&&(DataOk == 1))
	{
		//β�巨��������
		if((p=(TempVideoNode1 *)malloc(sizeof(TempVideoNode1)))==NULL) //�����½��s���������ڴ�ռ�
		{
			printf("���ܷ����ڴ�ռ�!\n");
			return 0;
		}
		if((p->Content.buffer=(unsigned char *)malloc(talkdata.Framelen))==NULL)
		{
			printf("���ܷ�����Ƶ�����ڴ�ռ�!\n");
			return 0;
		}
		p->Content.isFull = 0;
		for(j=0; j<MAXPACKNUM; j++)
		  p->Content.CurrPackage[j] = 0;

		p->Content.frame_flag = talkdata.DataType;
		//   newframeno = (r_buf[63] << 8) + r_buf[64];
		//   currpackage = (r_buf[67] << 8) + r_buf[68];
		p->Content.frameno = talkdata.Frameno;
		p->Content.TotalPackage = talkdata.TotalPackage;
		p->Content.timestamp = talkdata.timestamp;
		p->Content.CurrPackage[talkdata.CurrPackage - 1] = 1;
		if(talkdata.CurrPackage == p->Content.TotalPackage)
		  p->Content.Len =  (talkdata.CurrPackage - 1) * talkdata.PackLen + r_length - DeltaLen;
		p->Content.Len = talkdata.Framelen;
		memcpy(p->Content.buffer + (talkdata.CurrPackage - 1) * talkdata.PackLen,
					r_buf + DeltaLen, r_length - DeltaLen);
		p->Content.isFull = 1;
		for(j=0; j< p->Content.TotalPackage; j++)
		  if(p->Content.CurrPackage[j] == 0)
		  {
			  p->Content.isFull = 0;
			  break;
		  }
		p->rlink=NULL;    //p��ָ����Ϊ��
		p->llink=t;
		t->rlink=p;       //p��nextָ��������
		t=p;             //tָ��������
		return p->Content.isFull;
	}
}
//---------------------------------------------------------------------------
//�������ƣ�add
//����������������β���������
//�������ͣ��޷���ֵ
//���������� h:������ͷָ��
int add_videonode(TempVideoNode1 *p, struct talkdata1 talkdata,
			unsigned char *r_buf , int r_length)
{
	int j;
	int DataOk;
	DataOk = 1;
	if((talkdata.DataType < 1) || (talkdata.DataType > 5))
	  DataOk = 0;
	if(talkdata.Framelen > VIDEOMAX)
	  DataOk = 0;
	if(talkdata.CurrPackage > talkdata.TotalPackage)
	  DataOk = 0;
	if(talkdata.CurrPackage <= 0)
	  DataOk = 0;
	if(talkdata.TotalPackage <= 0)
	  DataOk = 0;
	if(DataOk == 1)
	{
		p->Content.frame_flag = talkdata.DataType;
		p->Content.frameno = talkdata.Frameno;
		p->Content.TotalPackage = talkdata.TotalPackage;
		p->Content.timestamp = talkdata.timestamp;
		p->Content.CurrPackage[talkdata.CurrPackage - 1] = 1;
		if(talkdata.CurrPackage == p->Content.TotalPackage)
		  p->Content.Len =  (talkdata.CurrPackage - 1) * talkdata.PackLen + r_length - DeltaLen;
		memcpy(p->Content.buffer + (talkdata.CurrPackage - 1) * talkdata.PackLen,
					r_buf + DeltaLen, r_length - DeltaLen);
		p->Content.isFull = 1;
		for(j=0; j< p->Content.TotalPackage; j++)
		  if(p->Content.CurrPackage[j] == 0)
		  {
			  p->Content.isFull = 0;
			  break;
		  }

		return p->Content.isFull;
	}
}
//---------------------------------------------------------------------------
//�������ƣ�creat
//����������������β���������
//�������ͣ��޷���ֵ
//���������� h:������ͷָ��
int creat_leavemovienode(TempVideoNode1 *h, uint32_t rframeno, uint32_t rtimestamp,
			unsigned short rframe_flag, unsigned char *r_buf , int r_length)
{
	TempVideoNode1 *t;
	TempVideoNode1 *p;
	int j;
	uint32_t newframeno;
	int currpackage;
	t=h;
	//  t=h->next;
	while(t->rlink!=NULL)    //ѭ����ֱ��tָ���
	  t=t->rlink;   //tָ����һ���
	if(t)
	{
		//β�巨��������
		if((p=(TempVideoNode1 *)malloc(sizeof(TempVideoNode1)))==NULL) //�����½��s���������ڴ�ռ�
		{
			printf("���ܷ����ڴ�ռ�!\n");
			return 0;
		}
		if((p->Content.buffer=(unsigned char *)malloc(r_length))==NULL)
		{
			printf("���ܷ�����Ƶ�����ڴ�ռ�!\n");
			return 0;
		}       
		p->Content.frame_flag = rframe_flag;
		p->Content.frameno = rframeno;
		p->Content.timestamp = rtimestamp;

		p->Content.Len =  r_length;
		memcpy(p->Content.buffer,
					r_buf, r_length);
		p->Content.isFull = 1;
		p->rlink=NULL;    //p��ָ����Ϊ��
		p->llink=t;
		t->rlink=p;       //p��nextָ��������
		t=p;             //tָ��������
		return p->Content.isFull;
	}
}
//---------------------------------------------------------------------------
//�������ƣ�length
//������������������
//�������ͣ��޷���ֵ
//����������h:������ͷָ��
int length_videonode(TempVideoNode1 *h)
{
	TempVideoNode1 *p;
	int i=0;         //��¼������
	p=h->rlink;
	while(p!=NULL)    //ѭ����ֱ��pָ���
	{
		i=i+1;
		p=p->rlink;   //pָ����һ���
	}
	return i;
	//    printf(" %d",i); //���p��ָ�ӵ��������
}
//---------------------------------------------------------------------------
//�������ƣ�delete_
//����������ɾ������
//�������ͣ�����
//����������h:������ͷָ�� i:Ҫɾ����λ��
int delete_videonode(TempVideoNode1 *p)
{
	if(p != NULL)
	{
		//��Ϊ���һ�����
		if(p->rlink != NULL)
		{
			(p->rlink)->llink=p->llink;
			(p->llink)->rlink=p->rlink;
			if(p->Content.buffer)
			  free(p->Content.buffer);
			if(p)
			  free(p);
		}
		else
		{
			(p->llink)->rlink=p->rlink;
			if(p->Content.buffer)
			  free(p->Content.buffer);
			if(p)
			  free(p);
		}
		return(1);
	}
	else
	  printf("video delete null\n");      
	return(0);
}
//---------------------------------------------------------------------------
int delete_all_videonode(TempVideoNode1 *h)
{
	TempVideoNode1 *p,*q;
	p=h->rlink;        //��ʱpΪ�׽��
	while(p != NULL)   //�ҵ�Ҫɾ������λ��
	{
		//��Ϊ���һ�����
		q = p;
		if(p->rlink != NULL)
		{
			(p->rlink)->llink=p->llink;
			(p->llink)->rlink=p->rlink;
		}
		else
		  (p->llink)->rlink=p->rlink;
		p = p->rlink;
		if(q->Content.buffer)
		  free(q->Content.buffer);
		if(q)
		  free(q);
	}
}
//---------------------------------------------------------------------------
int delete_lost_videonode(TempVideoNode1 *h, uint32_t currframeno, uint32_t currtimestamp) //ɾ����ȫ֡
{
	TempVideoNode1 *p,*q;
	p=h->rlink;        //��ʱpΪ�׽��
	while(p != NULL)   //�ҵ�Ҫɾ������λ��
	{
		//��Ϊ���һ�����
		q = p;
		if(p->rlink != NULL)
		{
			//        if(p->Content.frameno < currframeno) //����ѭ����ֱ��pΪ�գ����ҵ�x
			if(p->Content.timestamp < currtimestamp)
			{
				(p->rlink)->llink=p->llink;
				(p->llink)->rlink=p->rlink;
				p = p->llink;
				if(q->Content.buffer)
				  free(q->Content.buffer);
				if(q)
				  free(q);
				if(temp_video_n > 0)
				  temp_video_n --;
			}
		}
		else
		{
			//        if(p->Content.frameno < currframeno) //����ѭ����ֱ��pΪ�գ����ҵ�x
			if(p->Content.timestamp < currtimestamp)
			{
				(p->llink)->rlink=p->rlink;
				p = p->llink;
				if(q->Content.buffer)
				  free(q->Content.buffer);
				if(q)
				  free(q);
				if(temp_video_n > 0)
				  temp_video_n --;             
			}
		}
		p = p->rlink;
	}
	return 1;
}
//---------------------------------------------------------------------------
//�������ƣ�locate_
//������������λ����
//�������ͣ�����
//����������h:������ͷָ�� i:Ҫ��λ��λ��
TempVideoNode1 * locate_videonode(TempVideoNode1 *h,int i)
{
	TempVideoNode1 *p;
	int j;
	p=h->rlink;    //��ʱpΪ�׽��
	j=1;
	while(p&&j<i)  //�ҵ�Ҫ��λ��λ��
	{
		++j;
		p=p->rlink;  //pָ����һ���
	}
	if(i>0&&j==i)
	  return p;
	else
	  return NULL;
}
//---------------------------------------------------------------------------
//�������ƣ�find_
//�������������Һ���
//�������ͣ�����
//����������h:������ͷָ�� x:Ҫ���ҵ�ֵ
//���Ҹ�֡�ð��Ƿ��Ѵ���
TempVideoNode1 * find_videonode(TempVideoNode1 *h, int currframeno, int currpackage)
{
	TempVideoNode1 *p;
	int PackIsExist; //���ݰ��ѽ��ձ�־
	int FrameIsNew;  //���ݰ��Ƿ�����֡�Ŀ�ʼ
	p=h->rlink;    //��ʱpΪ�׽��
	PackIsExist = 0;
	FrameIsNew = 1;
	while(p!=NULL)
	{
		if(p->Content.frameno == currframeno) //����ѭ����ֱ��pΪ�գ����ҵ�x
		{
			FrameIsNew = 0;
			if(p->Content.CurrPackage[currpackage - 1] == 1)
			{
#ifdef _DEBUG
				printf("pack exist %d\n", currframeno);
#endif
				PackIsExist = 1;
			}
			break;
		}
		p=p->rlink;   //sָ��p����һ���
	}
	if(p!=NULL)
	  return p;
	else
	  return NULL;
}
//---------------------------------------------------------------------------
TempVideoNode1 * find_frame_videonode(TempVideoNode1 *h, int currframeno)
{
	TempVideoNode1 *p;
	p=h->rlink;    //��ʱpΪ�׽��
	while(p!=NULL)
	{
		if(p->Content.frameno == currframeno) //����ѭ����ֱ��pΪ�գ���ҵ�x
		  break;
		p=p->rlink;   //sָ��p����һ���
	}
	if(p!=NULL)
	  return p;
	else
	  return NULL;
}
//---------------------------------------------------------------------------
//�������ƣ�find_
//�������������Һ���
//�������ͣ�����
//����������h:������ͷָ�� x:Ҫ���ҵ�ֵ
//�������ϵ�֡
TempVideoNode1 * search_videonode(TempVideoNode1 *h)
{
	TempVideoNode1 *p;
	TempVideoNode1 *tem_p;

	p=h->rlink;    //��ʱpΪ�׽��
	tem_p = p;

	while(p!=NULL)
	{
		if(p->Content.isFull == 1)
		  //     if(p->Content.frameno < tem_p->Content.frameno) //����ѭ����ֱ��pΪ�գ����ҵ�x
		  if(p->Content.timestamp < tem_p->Content.timestamp) //����ѭ����ֱ��pΪ�գ����ҵ�x
		  {
			  tem_p = p;
		  }
		p=p->rlink;   //sָ��p����һ���
	}

	return tem_p;

}
//---------------------------------------------------------------------------
int free_videonode(TempVideoNode1 *h)
{
	TempVideoNode1 *p,*t;
	int i=0;         //��¼������
	p=h->rlink;
	while(p!=NULL)    //ѭ����ֱ��pָ���
	{
		i=i+1;
		t = p;
		p=p->rlink;   //pָ����һ���
		free(t);
	}
	return i;
}
//---------------------------------------------------------------------------

