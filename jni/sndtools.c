#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <semaphore.h>       //sem_t
#include <dirent.h>

#define VAR_STATIC

#define CommonH
#include "common.h"
#include "./include/g711.h"
#include "sndtools.h"

//UDP

int devrecfd = 0;
int devplayfd = 0;

int audio, abuf_size, zbuf_size;
int PlayPcmTotalNum;   //一个接收音频包总的幅度值
extern int AudioMuteFlag;   //静音标志
//音频
int audio_rec_flag;
int audio_play_flag;
pthread_t audio_rec_deal_thread;      //音频采集数据处理线程
pthread_t audio_play_deal_thread;     //音频播放数据处理线程
pthread_t audio_rec_thread;      //音频采集线程
pthread_t audio_play_thread;     //音频播放线程
void audio_rec_deal_thread_func(void);
void audio_play_deal_thread_func(void);
void audio_rec_thread_func(void);
void audio_play_thread_func(void);


struct audiobuf1 playbuf;    //音频播放环形缓冲
struct audiobuf1 recbuf;     //音频采集环形缓冲

sem_t audioplaysem;
sem_t audiorecsem;
sem_t audiorec2playsem;

void StartRecAudio(void);
void StopRecAudio(void);
void StartPlayAudio(void);
void StopPlayAudio(void);

extern struct _SYNC sync_s;
extern struct timeval ref_time;  //基准时间,音频或视频第一帧

extern int curr_audio_timestamp;

//为防止多次操作导致错误
int AudioRecIsStart=0;
int AudioPlayIsStart=0;

//用于声音播放
struct wavbuf1 wavbuf;       //声音播放环形缓冲
int audio_play_wav_flag=0;
pthread_t audio_play_wav_thread;     //留言播放线程
void audio_play_wav_thread_func(int PlayFlag);
struct WaveFileHeader hWaveFileHeader;
//用于声音播放
void StartPlayWav(char *srcname, int PlayFlag); //0 单次播放  1 循环播放
void StopPlayWavFile(void);

//目录及文件链表
// *h保存表头结点的指针，*p指向当前结点的前一个结点，*s指向当前结点

TempAudioNode1 * init_audionode(void); //初始化单链表的函数
//功能描述：求单链表长度
int length_audionode(TempAudioNode1 *h);
//功能描述：尾部添加
int creat_audionode(TempAudioNode1 *h, struct talkdata1 talkdata, unsigned char *r_buf ,
			int r_length);

//留影播放
int creat_leavemovieaudionode(TempAudioNode1 *h, uint32_t rframeno, uint32_t rtimestamp,
			int rframe_flag, unsigned char *r_buf , int r_length);
//功能描述：删除函数
int delete_audionode(TempAudioNode1 *p);
int delete_all_audionode(TempAudioNode1 *h);
int delete_lost_audionode(TempAudioNode1 *h, uint32_t currframeno, uint32_t currtimestamp); //删除不全帧
//功能描述：定位函数
TempAudioNode1 * locate_audionode(TempAudioNode1 *h,int i);
//功能描述：查找函数
TempAudioNode1 * find_audionode(TempAudioNode1 *h, int currframeno, int currpackage);
//功能描述：查找函数
TempAudioNode1 * find_frame_audionode(TempAudioNode1 *h, int currframeno);
//查找最老的帧
TempAudioNode1 * search_audionode(TempAudioNode1 *h);
//
int free_audionode(TempAudioNode1 *h);
//---------------------------------------------------------------------------
/*
 * Open Sound device
 * Return 1 if success, else return 0.
 */
int OpenSnd(/* add by new version */int nWhich)
{
	int status;   // 系统调用的返回值
	int setting;
	if(nWhich == 1)
	{
		if(devrecfd == 0)
		{
			devrecfd = open ("/dev/dsp", O_RDONLY);//, 0);//open("/dev/dsp", O_RDWR);
			if(devrecfd < 0)
			{
				devrecfd = 0;
				return 0;
			}
		}
	}
	else
	{
		if(devplayfd == 0)
		{
			devplayfd = open ("/dev/dsp1", O_WRONLY);//, 0);//open("/dev/dsp", O_RDWR);
			if(devplayfd < 0)
			{
				devplayfd = 0;
				return 0;
			}
			setting = 0x00040009;
			status = ioctl(devplayfd, SNDCTL_DSP_SETFRAGMENT, &setting);
			if (status == -1) {
				perror("ioctl buffer size");
				return -1;
			}   
		} 
	}

	return 1;
}
//---------------------------------------------------------------------------
/*
 * Close Sound device
 * return 1 if success, else return 0.
 */
int CloseSnd(/* add by new version */int nWhich)
{
	int status;
	//  int devfd;
	if(nWhich == 1)
	{
		close(devrecfd);
		devrecfd = 0;
	}
	else
	{
		// 等待回放结束
		status = ioctl(devplayfd, SOUND_PCM_SYNC, 0);
		if (status == -1)
		  perror("SOUND_PCM_SYNC ioctl failed");   
		close(devplayfd);
		devplayfd = 0;
	}

	return 1;
}

//---------------------------------------------------------------------------
/*
 * Set Record an Playback format
 * return 1 if success, else return 0.
 * bits -- FMT8BITS(8bits), FMT16BITS(16bits)
 * hz -- FMT8K(8000HZ), FMT16K(16000HZ), FMT22K(22000HZ), FMT44K(44000HZ)
 chn -- MONO 1 STERO 2
 */
int SetFormat(int nWhich, int bits, int hz, int chn)
{
	int samplesize;
	int tmp;
	int dsp_stereo;
	int setting;

	//  int fd;	// 声音设备的文件描述符
	int arg;	// 用于ioctl调用的参数
	int status;   // 系统调用的返回值
	int devfd;
	if(nWhich == 1)
	  devfd = devrecfd;
	else
	  devfd = devplayfd;

	/*  arg = 0;
		ioctl (devfd, SNDCTL_DSP_RESET,(char *)&arg) ;
		arg = 1;
		ioctl (devfd, SNDCTL_DSP_SYNC,(char *)&arg);    */

	// 设置采样时的量化位数       FIC8120只支持16位
	arg = bits;
	//  printf("set bits is = %d\n", arg);
	status = ioctl(devfd, SOUND_PCM_WRITE_BITS, &arg);
	if (status == -1)
	  printf("SOUND_PCM_WRITE_BITS ioctl failed");
	if (arg != bits)
	  printf("unable to set sample size");
	//  status = ioctl(devfd, SOUND_PCM_READ_BITS, &arg);
	//  printf("get bits is = %d\n", arg);


	// 设置采样时的声道数目
	arg = chn;
	status = ioctl(devfd, SOUND_PCM_WRITE_CHANNELS, &arg);
	if (status == -1)
	  perror("SOUND_PCM_WRITE_CHANNELS ioctl failed");
	if (arg != chn)
	  perror("unable to set number of channels");
	// 设置采样时的采样频率 
	arg = hz;
	status = ioctl(devfd, SOUND_PCM_WRITE_RATE, &arg);
	if (status == -1)
	  perror("SOUND_PCM_WRITE_WRITE ioctl failed");

	abuf_size = AUDIOBLK;
	if(nWhich == 1)
	{
		setting = 0x00040009;//0x000F0009;
		status = ioctl(devfd, SNDCTL_DSP_SETFRAGMENT, &setting);
		if (status == -1) {
			perror("ioctl buffer size");
			return -1;
		}
	}
	/*  else
		{
		setting = 0x00040009;//0x000F0009;
		status = ioctl(devfd, SNDCTL_DSP_SETFRAGMENT, &setting);
		if (status == -1) {
		perror("ioctl buffer size");
		return -1;
		}
		}     */
	ioctl(devfd, SNDCTL_DSP_GETBLKSIZE, &abuf_size);
#ifdef _DEBUG
	printf("abuf_size= %d\n",abuf_size);
#endif  
	if (abuf_size < 4 || abuf_size > 65536)
	{
		//   if (abuf_size == -1)
		printf ( "Invalid audio buffers size %d\n", nWhich);
		exit (-1);
	}

	return 1;
}

//---------------------------------------------------------------------------
/*
 * Record
 * return numbers of byte for read.
 */
int Record(char *buf, int size)
{
	int status;
	status = 0;
	if(devrecfd > 0)
	  status=read(devrecfd, buf, size);
	//  printf("record audio %d", status);
	return status;
}
//---------------------------------------------------------------------------
/* 
 * Playback
 * return numbers of byte for write.
 */
int Play(char *buf, int size)
{
	int status;
	status = 0;
	if(devplayfd > 0)
	  status = write(devplayfd, buf, size);
	return status;
}
//---------------------------------------------------------------------------
void audio_rec_deal_thread_func(void)
{

	short recppcm[AUDIOBLK];
	int dwSize;
	int i;
	unsigned char adpcm_out[3072];
	char RemoteHost[20];
	int FrameNum;
	//通话数据结构
	struct talkdata1 talkdata;
#ifdef _DEBUG
	printf("创建采集数据处理线程：\n" );
#endif
	while(audio_rec_flag == 1)
	{
		//等待采集线程有数据的信号
		sem_wait(&audiorecsem);
		//加锁
		pthread_mutex_lock(&sync_s.audio_rec_lock);
		FrameNum = recbuf.n/AUDIOBLK;

		for(i=0; i<FrameNum; i++)
		{
			//头部
			memcpy(adpcm_out, UdpPackageHead, 6);
			//命令
			//  if(Remote.isDirect == 1)
			//    adpcm_out[6] = VIDEOTALKTRANS;
			//  else
			adpcm_out[6] = VIDEOTALK;
			adpcm_out[7] = 1;
			//子命令
			//子命令
			if((Local.Status == 1)||(Local.Status == 3)||(Local.Status == 5))  //本机为主叫方
			{
				adpcm_out[8] = CALLUP;
				memcpy(talkdata.HostAddr, LocalCfg.Addr, 20);
				memcpy(talkdata.HostIP, LocalCfg.IP, 4);
				memcpy(talkdata.AssiAddr, Remote.Addr[0], 20);
				memcpy(talkdata.AssiIP, Remote.IP[0], 4);
			}
			if((Local.Status == 2)||(Local.Status == 4)||(Local.Status == 6))  //本机为被叫方
			{
				adpcm_out[8] = CALLDOWN;
				memcpy(talkdata.HostAddr, Remote.Addr[0], 20);
				memcpy(talkdata.HostIP, Remote.IP[0], 4);
				memcpy(talkdata.AssiAddr, LocalCfg.Addr, 20);
				memcpy(talkdata.AssiIP, LocalCfg.IP, 4);
			}

			//时间戳
			talkdata.timestamp = recbuf.timestamp[recbuf.iget/AUDIOBLK];
			//数据类型
			talkdata.DataType = 1;
			//帧序号
			talkdata.Frameno = recbuf.frameno[recbuf.iget/AUDIOBLK];
			//帧数据长度
			talkdata.Framelen = AUDIOBLK/2;
			//总包数
			talkdata.TotalPackage = 1;
			//当前包
			talkdata.CurrPackage = 1;
			//数据长度
			talkdata.Datalen = AUDIOBLK/2;
			talkdata.PackLen = PACKDATALEN;

			memcpy(adpcm_out + 9, &talkdata, sizeof(talkdata));

			G711Encoder((short *)(recbuf.buffer + recbuf.iget), (unsigned char *)(adpcm_out + DeltaLen), AUDIOBLK/2, 1);
#ifdef _DEBUG
			///////////////////////paul0312///////////////////////////////////////
			///  printf("recbuf.iput = %d,devrecfd = %d\n",recbuf.iput, devrecfd);
			/////////////////////////////////////////////////////////////
#endif
			if((recbuf.iget + AUDIOBLK) >= NMAX)
			  recbuf.iget = 0;
			else
			  recbuf.iget += AUDIOBLK;
			recbuf.n -=  AUDIOBLK;

			if(Local.Status != 0)
			{
				//UDP发送
				sprintf(RemoteHost, "%d.%d.%d.%d\0",Remote.DenIP[0],
							Remote.DenIP[1],Remote.DenIP[2],Remote.DenIP[3]);
				UdpSendBuff(m_VideoSendSocket, RemoteHost, adpcm_out, DeltaLen + AUDIOBLK/2);
			}
		}
		//解锁
		pthread_mutex_unlock(&sync_s.audio_rec_lock);
	}
}
//---------------------------------------------------------------------------
void audio_rec_thread_func(void)
{
	//  struct timeval tv;
	struct timeval tv, tv1;
	uint32_t nowtime;
	//  struct timezone tz;
	//  struct adpcm_state sadpcm;
	int dwSize;
	//  char adpcm_out[3072];
	int i;
	int RecPcmTotalNum;
	short recppcm[AUDIOBLK/2];

	short nullrecppcm[AUDIOBLK/2];
	for(i = 0; i < AUDIOBLK/2; i++)
	  nullrecppcm[i] = 0;

#ifdef _DEBUG
	printf("创建录音线程：\n" );
	printf("audio_rec_flag=%d\n",audio_rec_flag);
#endif  
	gettimeofday(&tv, NULL);
	nowtime = tv.tv_sec *1000 + tv.tv_usec/1000;  
	while(audio_rec_flag == 1)
	{
		//加锁
		pthread_mutex_lock(&sync_s.audio_rec_lock);

		recbuf.frameno[recbuf.iput/AUDIOBLK] = Local.nowaudioframeno;
		Local.nowaudioframeno++;
		if(Local.nowaudioframeno >= 65536)
		  Local.nowaudioframeno = 1;

		//时间戳
		gettimeofday(&tv, NULL);
		//第一帧,设定起始时间戳
		if((ref_time.tv_sec ==0)&&(ref_time.tv_usec ==0))
		{
			ref_time.tv_sec = tv.tv_sec;
			ref_time.tv_usec = tv.tv_usec;
		}
		nowtime = (tv.tv_sec - ref_time.tv_sec) *1000 + (tv.tv_usec - ref_time.tv_usec)/1000;
		recbuf.timestamp[recbuf.iput/AUDIOBLK] = nowtime;
		//    printf("nowframeno=%d, recbuf.timestamp=%d\n", nowframeno, recbuf.timestamp[recbuf.iput/AUDIOBLK]);
		dwSize = Record(recbuf.buffer + recbuf.iput, AUDIOBLK);
		if(dwSize != AUDIOBLK)
		  printf("AUDIOBLK != dwSize");

		if((recbuf.iput + AUDIOBLK) >= NMAX)
		  recbuf.iput = 0;
		else
		  recbuf.iput += AUDIOBLK;
		if(recbuf.n < NMAX)
		  recbuf.n +=  AUDIOBLK;
		else
		  printf("rec.Buffer is full\n");

		//解锁
		pthread_mutex_unlock(&sync_s.audio_rec_lock);
		sem_post(&audiorecsem);
	}
}
#if 1
//---------------------------------------------------------------------------
void audio_play_deal_thread_func(void)
{
	short playppcm[AUDIOBLK*2];
	int i,j;
	int fn;
	char filename[50];
	char buf[AUDIOBLK];
	TempAudioNode1 * tmp_audionode;
	uint32_t dellostframeno;
	uint32_t dellosttimestamp;
#ifdef _DEBUG
	printf("\naudio_play_deal_thread_func leftmessage：%d\n\n",Local.leftmessage );
#endif  
	if((Local.leftmessage == 0x01) )
	{
		strcpy(filename,"/mnt/audiotest\0");
		//strcpy(filename,"/mnt/mtd/audiotest\0");

		fn = open(filename, O_RDONLY );// , S_IWRITE);
		//afp = open(curravm.FileName, O_WRONLY | O_APPEND | O_CREAT);// , S_IWRITE);
		if(fn == -1)
		{
			printf("/mnt/mtd/audiotest file open faile...\n");
			return;
		}

		printf("OPEN %s OK\n",filename);
		//		playbuf.frameno[playbuf.iput/AUDIOBLK] = 0;
		//		playbuf.timestamp[playbuf.iput/AUDIOBLK] = 0;

	}

	while(audio_play_flag == 1)
	{
		//等待采集线程有数据的信号, 测试用
		//等待UDP接收线程有数据的信号
		if(Local.leftmessage != 0x01)
		  sem_wait(&audiorec2playsem);
		//加锁
		pthread_mutex_lock(&sync_s.audio_play_lock);
		//    while(temp_audio_n > 0)
		if(temp_audio_n > 0 || Local.leftmessage == 0x01)
		{
			//解码
			//查找最老的帧
			if(Local.leftmessage == 0x00)
			  tmp_audionode = search_audionode(TempAudioNode_h);
			if(tmp_audionode != NULL || Local.leftmessage == 0x01)
			{
				//playbuf.frameno[playbuf.iput/AUDIOBLK] = tmp_audionode->Content.frameno;
				//playbuf.timestamp[playbuf.iput/AUDIOBLK] = tmp_audionode->Content.timestamp;
				////////////////0727
				// SAVE audio when I'm being called //

				// Open audiofile when I'm being checked//
				if(Local.leftmessage == 0x01)
				{

					i = read(fn, buf, AUDIOBLK/2);
					if(i == -1)
					{
						//	printf("audio file read faile...%d\n",Local.TimeOut);
						//	Local.TimeOut = 0;
						Local.leftmessage = 0x00;
						//while(1);

					}
					else if(i == 0)
					{
						printf("audio file read done...%d\n",Local.TimeOut);
						close (fn);
						Local.leftmessage = 0x00;
						/////////0917 kill myself
						pthread_exit((void *)1);
						//while(1);
						//close (curravm.Sn);
						//	pthread_mutex_lock(&sync_s.audio_play_lock);////make a dead lock here, wait to be kill
					}
					else
					{

						G711Decoder((short *)playbuf.buffer , (unsigned char *)buf, AUDIOBLK/2,1);
						Play(playbuf.buffer , AUDIOBLK);
					}
				}
				else if(Local.leftmessage == 0x00)
				{
					//printf("DECODing\n");
					playbuf.frameno[playbuf.iput/AUDIOBLK] = tmp_audionode->Content.frameno;
					playbuf.timestamp[playbuf.iput/AUDIOBLK] = tmp_audionode->Content.timestamp;
					//		G711Decoder(playbuf.buffer + playbuf.iput, tmp_audionode->Content.buffer, AUDIOBLK/2,1);
					G711Decoder(playppcm, tmp_audionode->Content.buffer, AUDIOBLK/2,1);

					//一个接收音频包总的幅度值
					PlayPcmTotalNum = 0;
					for(j=0; j< AUDIOBLK/2; j++)
					  PlayPcmTotalNum += abs(playppcm[j]);

					memcpy(playbuf.buffer + playbuf.iput, playppcm, AUDIOBLK);

					if((playbuf.iput + AUDIOBLK) >= NMAX)
					  playbuf.iput = 0;
					else
					  playbuf.iput += AUDIOBLK;
					if(playbuf.n < NMAX)
					  playbuf.n +=  AUDIOBLK;
					else
					  printf("play1.Buffer is full\n");
					sem_post(&audioplaysem);
					if(temp_audio_n > 0)
					  temp_audio_n --;
					dellostframeno = tmp_audionode->Content.frameno;
					dellosttimestamp = tmp_audionode->Content.timestamp;
					delete_audionode(tmp_audionode);
				}
			}
			//删除不全帧
			delete_lost_audionode(TempAudioNode_h, dellostframeno, dellosttimestamp);
		}
		//解锁
		pthread_mutex_unlock(&sync_s.audio_play_lock);
	}
}
//---------------------------------------------------------------------------
void audio_play_thread_func(void)
{
	char *audio_out;
	int dwSize;
	int i;
	int jump_buf;  //已解码缓冲区跳帧数
	int jump_tmp;  //接收缓冲区跳帧数
	int jump_frame;
	int aframe;
	TempAudioNode1 * tmp_audionode;
#ifdef _DEBUG
	printf("CREATING AUDIO_PLAY_THREAD：\n" );
	printf("audio_play_flag=%d\n",audio_play_flag);
#endif
	aframe = AFRAMETIME;
	while(audio_play_flag == 1)
	{
		sem_wait(&audioplaysem);

		//  while(playbuf.n > 0)
		if(playbuf.n > 0)
		{
			//         printf("playbuf.timestamp[playbuf.iget/AUDIOBLK] =%d\n",playbuf.timestamp[playbuf.iget/AUDIOBLK]);
			pthread_mutex_lock(&sync_s.audio_play_lock);
			curr_audio_timestamp = playbuf.timestamp[playbuf.iget/AUDIOBLK];
			dwSize = Play(playbuf.buffer +  playbuf.iget, AUDIOBLK);
			//printf("play thread dwSize = %d\n", dwSize);
			//加锁
			//   dwSize = AUDIOBLK;
			if((playbuf.iget + dwSize) >= NMAX)
			  playbuf.iget = 0;
			else
			  playbuf.iget += dwSize;
			if(playbuf.n >= dwSize)
			  playbuf.n -=  dwSize;
			else
			  printf("play2.Buffer is full\n");
/*
			if((TimeStamp.OldCurrAudio != TimeStamp.CurrAudio) //上一次当前视频时间
						&&(TimeStamp.OldCurrAudio != 0)&&(TimeStamp.CurrAudio != 0))
			{
				if((TimeStamp.CurrAudio - curr_audio_timestamp) > 32*8)
				{
					jump_frame = (TimeStamp.CurrAudio - curr_audio_timestamp - 160)/aframe;
					if((playbuf.n/AUDIOBLK) >= jump_frame)
					{
						jump_buf = jump_frame;
						jump_tmp = 0;
					}
					else
					{
						temp_audio_n = length_audionode(TempAudioNode_h);
						jump_buf = (playbuf.n/AUDIOBLK);
						if(temp_audio_n > (jump_frame - (playbuf.n/AUDIOBLK)))
						  jump_tmp = jump_frame - (playbuf.n/AUDIOBLK);
						else
						  jump_tmp = temp_audio_n;
					}

					printf("audio jump_buf =%d , jump_tmp = %d, jump_frame = %d\n", jump_buf, jump_tmp, jump_frame);

					for(i=0; i<jump_buf; i++)
					{
						if((playbuf.iget + AUDIOBLK) >= NMAX)
						  playbuf.iget = 0;
						else
						  playbuf.iget += AUDIOBLK;
						if(playbuf.n >= AUDIOBLK)
						  playbuf.n -=  AUDIOBLK;
					}
					for(i=0; i<jump_tmp; i++)
					{
						//查找最老的帧
						tmp_audionode = search_audionode(TempAudioNode_h);
						if((tmp_audionode != NULL)&&(temp_audio_n > 0))
						{
							delete_audionode(tmp_audionode);
							temp_audio_n --;
						}
					}
				}
			}
*/

			//解锁
			pthread_mutex_unlock(&sync_s.audio_play_lock);
		}

	}
}
#else
//---------------------------------------------------------------------------
void audio_play_deal_thread_func(void)
{
	short playppcm[AUDIOBLK*2];
	int i,j;
	int fn;
	char filename[50];
	char buf[AUDIOBLK];
	TempAudioNode1 * tmp_audionode;
	uint32_t dellostframeno;
	uint32_t dellosttimestamp;
#ifdef _DEBUG
	printf("\naudio_play_deal_thread_func leftmessage：%d\n\n",Local.leftmessage );
#endif  
	if((Local.leftmessage == 0x01) )
	{
		strcpy(filename,"/mnt/audiotest\0");
		fn = open(filename, O_RDONLY );// , S_IWRITE);
		if (fn == -1) {
			printf("/mnt/mtd/audiotest file open faile...\n");
			return;
		}
		printf("OPEN %s OK\n",filename);
	}

	while(audio_play_flag == 1)
	{
		//等待采集线程有数据的信号, 测试用
		//等待UDP接收线程有数据的信号
		if (Local.leftmessage != 0x01) {
            sem_wait(&audiorec2playsem);
        }
		//加锁
		pthread_mutex_lock(&sync_s.audio_play_lock);
        if (Local.leftmessage == 0x01) {
            // Open audiofile when I'm being checked//
            i = read(fn, buf, AUDIOBLK/2);
            if (i == -1) {
                Local.leftmessage = 0x00;
            } else if (i == 0) {
                printf("audio file read done...%d\n",Local.TimeOut);
                close (fn);
                Local.leftmessage = 0x00;
                pthread_exit((void *)1);
            } else {
                G711Decoder((short *)playbuf.buffer , (unsigned char *)buf, AUDIOBLK/2,1);
                Play(playbuf.buffer , AUDIOBLK);
            }
        } else {
            if (temp_audio_n > 0) {
                //解码
                //查找最老的帧
                tmp_audionode = search_audionode(TempAudioNode_h);
                if (tmp_audionode != NULL) {
                    //printf("DECODing\n");
                    playbuf.frameno[playbuf.iput/AUDIOBLK] = tmp_audionode->Content.frameno;
                    playbuf.timestamp[playbuf.iput/AUDIOBLK] = tmp_audionode->Content.timestamp;
                    G711Decoder(playppcm, tmp_audionode->Content.buffer, AUDIOBLK/2,1);

                    //一个接收音频包总的幅度值
                    PlayPcmTotalNum = 0;
                    for (j=0; j< AUDIOBLK/2; j++) {
                        PlayPcmTotalNum += abs(playppcm[j]);
                    }

                    Play(playppcm, AUDIOBLK);

                    if(temp_audio_n > 0)
                        temp_audio_n --;
                    dellostframeno = tmp_audionode->Content.frameno;
                    dellosttimestamp = tmp_audionode->Content.timestamp;
                    delete_audionode(tmp_audionode);
                }
            }
            //删除不全帧
            delete_lost_audionode(TempAudioNode_h, dellostframeno, dellosttimestamp);
        }
        //解锁
        pthread_mutex_unlock(&sync_s.audio_play_lock);
    }
}
//---------------------------------------------------------------------------
void audio_play_thread_func(void)
{
	char *audio_out;
	int dwSize;
	int i;
	int jump_buf;  //已解码缓冲区跳帧数
	int jump_tmp;  //接收缓冲区跳帧数
	int jump_frame;
	int aframe;
	TempAudioNode1 * tmp_audionode;
#ifdef _DEBUG
	printf("CREATING AUDIO_PLAY_THREAD：\n" );
	printf("audio_play_flag=%d\n",audio_play_flag);
#endif
	aframe = AFRAMETIME;
	while(audio_play_flag == 1)
	{
		sem_wait(&audioplaysem);

		//  while(playbuf.n > 0)
		if(playbuf.n > 0)
		{
			pthread_mutex_lock(&sync_s.audio_play_lock);
			curr_audio_timestamp = playbuf.timestamp[playbuf.iget/AUDIOBLK];
			dwSize = Play(playbuf.buffer +  playbuf.iget, AUDIOBLK);
			//printf("play thread dwSize = %d\n", dwSize);
			//加锁
			//   dwSize = AUDIOBLK;
			if((playbuf.iget + dwSize) >= NMAX)
			  playbuf.iget = 0;
			else
			  playbuf.iget += dwSize;
			if(playbuf.n >= dwSize)
			  playbuf.n -=  dwSize;
			else
			  printf("play2.Buffer is full\n");

			//解锁
			pthread_mutex_unlock(&sync_s.audio_play_lock);
		}

	}
}
#endif
//---------------------------------------------------------------------------
void StartRecAudio(void)
{
	pthread_attr_t attr;
	int i;
	if(AudioRecIsStart == 0)
	{
		AudioRecIsStart = 1;
		PlayPcmTotalNum = 0;   //一个接收音频包总的幅度值
		//选通音频 ,高电平有效
		//doors
		//	if((Local.DoorNo >= 0x31)&&(Local.DoorNo <= 0x32))
		{
			ioctl(gpio_fd, IO_PUT, 0);
			//ioctl(gpio_fd, IO_PUT, Local.DoorNo - 0x31);
		}
		//	else
		//		printf("开始通话 Local.DoorNo = 0x%X, 异常\n", Local.DoorNo);
		//音频
		if(!OpenSnd(AUDIODSP))
		{
			printf("Open record sound device error!\\n");
			return;
		}

		SetFormat(AUDIODSP, FMT16BITS, FMT8K, 1/*STERO, WAVOUTDEV*/);    //录音

		Local.nowaudioframeno = 1;
		recbuf.iput = 0;
		recbuf.iget = 0;
		recbuf.n = 0;
		for(i=0; i<NMAX/AUDIOBLK; i++)
		{
			recbuf.frameno[i] = 0;
			recbuf.timestamp[i] = 0;
		}

		sem_init(&audiorecsem,0,0);

		audio_rec_flag = 1;
		pthread_mutex_init (&sync_s.audio_rec_lock, NULL);

		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
		pthread_create(&audio_rec_deal_thread,&attr,(void *)audio_rec_deal_thread_func,NULL);
		if ( audio_rec_deal_thread == 0 ) {
			printf("无法创建音频采集数据处理线程\n");
			pthread_attr_destroy(&attr);
			return;
		}

		pthread_create(&audio_rec_thread,&attr,(void *)audio_rec_thread_func,NULL);
		pthread_attr_destroy(&attr);
		if ( audio_rec_thread == 0 ) {
			printf("无法创建音频采集线程\n");
			return;
		}
	}  
	else
	{
#ifdef _DEBUG
		printf("重复 AudioRecIsStart\n");
#endif
	}
}
//---------------------------------------------------------------------------
void StopRecAudio(void)
{
	int delaytime;
	delaytime=40;
	printf("AudioRecIsStart=%d\n", AudioRecIsStart);
	if(AudioRecIsStart == 1)
	{
		ref_time.tv_sec = 0;   //初始时间戳
		ref_time.tv_usec = 0;

		AudioRecIsStart = 0;
		audio_rec_flag = 0;
		usleep(delaytime*1000);
		if(pthread_cancel(audio_rec_thread) ==0)
		  printf("audio_rec_thread cancel success\n");
		else
		  printf("audio_rec_thread cancel fail\n");
		usleep(delaytime*1000);
		if(pthread_cancel(audio_rec_deal_thread) ==0)
		  printf("audio_rec_deal_thread cancel success\n");
		else
		  printf("audio_rec_deal_thread cancel fail\n");
		usleep(delaytime*1000);
		CloseSnd(AUDIODSP);
		//关闭音频 ,高电平有效
		//doorsif((Local.DoorNo >= 0x31)&&(Local.DoorNo <= 0x34))
		{
			ioctl(gpio_fd, IO_CLEAR, 0);
			//ioctl(gpio_fd, IO_CLEAR, Local.DoorNo - 0x31);
		}
		//	else
		//		printf("关闭音频 Local.DoorNo = 0x%X, 异常\n", Local.DoorNo);
		sem_destroy(&audiorecsem);
		pthread_mutex_destroy(&sync_s.audio_rec_lock);
	}
	else
	{
#ifdef _DEBUG
		printf("重复 AudioRecIsStart\n");
#endif
	}
}
//---------------------------------------------------------------------------
void StartPlayAudio(void)
{
	pthread_attr_t attr;
	int i;
	if(AudioPlayIsStart == 0)
	{
		TimeStamp.OldCurrVideo = 0;       //上一次当前视频时间
		TimeStamp.CurrVideo = 0;
		TimeStamp.OldCurrAudio = 0;       //上一次当前音频时间
		TimeStamp.CurrAudio = 0;

		AudioPlayIsStart = 1;
		PlayPcmTotalNum = 0;   //一个接收音频包总的幅度值
		//音频

		//////////////////////paul0302////////////////////////
		if(!OpenSnd(AUDIODSP1))
		{
			printf("Open play sound device error!\n");
			return;
		}     
		////////////////////////////////////////////////////

		SetFormat(AUDIODSP1, FMT16BITS, FMT8K, 1);    //放音 STERO, WAVOUTDEV

		playbuf.iput = 0;
		playbuf.iget = 0;
		playbuf.n = 0;
		for(i=0; i<NMAX/AUDIOBLK; i++)
		{
			playbuf.frameno[i] = 0;
			playbuf.timestamp[i] = 0;
		}

		temp_audio_n = 0;
		//音频接收缓冲链表
		delete_all_audionode(TempAudioNode_h);     

		sem_init(&audioplaysem,0,0);
		sem_init(&audiorec2playsem,0,0);
		//查看同步播放线程是否已创建
		// sync_play_init();

		audio_play_flag = 1;
		pthread_mutex_init (&sync_s.audio_play_lock, NULL); 

#if 1
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
		pthread_create(&audio_play_deal_thread,&attr,(void *)audio_play_deal_thread_func,NULL);
		if ( audio_play_deal_thread == 0 ) {
			printf("无法创建放音数据处理线程\n");
			pthread_attr_destroy(&attr);
			return;
		}
		pthread_create(&audio_play_thread,&attr, (void *)audio_play_thread_func, NULL);
		pthread_attr_destroy(&attr);
		if ( audio_play_thread == 0 ) {
			printf("无法创建音频播放线程\n");
			return;
		}         
#endif
	}  
	else
	{
#ifdef _DEBUG
		printf("重复 AudioPlayStart\n");
#endif
	}
}

void StopPlayAudio(void)
{
	int delaytime;
	delaytime=40;
	printf("AudioPlayStart=%d\n", AudioPlayIsStart);
	if(AudioPlayIsStart == 1)
	{
		AudioPlayIsStart = 0;

		audio_play_flag = 0;
		usleep(delaytime*1000);
		if(pthread_cancel(audio_play_deal_thread) ==0)
		  printf("audio_play_deal_thread cancel success\n");
		else
		  printf("audio_play_deal_thread cancel fail\n");
		usleep(delaytime*1000);
		if(pthread_cancel(audio_play_thread) ==0)
		  printf("audio_play_thread cancel success\n");
		else
		  printf("audio_play_thread cancel fail\n");
		usleep(delaytime*1000);
		////////////////////////paul0302//////////////////  
		CloseSnd(AUDIODSP1);
		//////////////////////////////////
		sem_destroy(&audioplaysem);
		sem_destroy(&audiorec2playsem);
		pthread_mutex_destroy(&sync_s.audio_play_lock);
		//释放同步播放线程及变量
		//  sync_play_destroy();

		temp_audio_n = 0;
		//音频接收缓冲链表
		delete_all_audionode(TempAudioNode_h);    
	}
	else
	{
#ifdef _DEBUG
		printf("重复 AudioPlayStop\n");
#endif
	}
}
//---------------------------------------------------------------------------
void StartPlayWav(char *srcname, int PlayFlag) //0 单次播放  1 循环播放
{
	pthread_attr_t attr;
	int i;
	FILE* fd;
	int bytes_read;
	//查看音频设备是否空闲
	if(audio_play_wav_flag == 0)
	{
		if((fd=fopen(srcname, "rb"))==NULL)
		{
			printf("Open Error:%s\n",srcname);
			return;
		}

		bytes_read=fread(hWaveFileHeader.chRIFF, sizeof(hWaveFileHeader.chRIFF), 1, fd);
		bytes_read=fread(&hWaveFileHeader.dwRIFFLen, sizeof(hWaveFileHeader.dwRIFFLen), 1, fd);
		bytes_read=fread(hWaveFileHeader.chWAVE, sizeof(hWaveFileHeader.chWAVE), 1, fd);

		bytes_read=fread(hWaveFileHeader.chFMT, sizeof(hWaveFileHeader.chFMT), 1, fd);
		bytes_read=fread(&hWaveFileHeader.dwFMTLen, sizeof(hWaveFileHeader.dwFMTLen), 1, fd);
		bytes_read=fread(&hWaveFileHeader.wFormatTag, sizeof(hWaveFileHeader.wFormatTag), 1, fd);
		bytes_read=fread(&hWaveFileHeader.nChannels, sizeof(hWaveFileHeader.nChannels), 1, fd);
		bytes_read=fread(&hWaveFileHeader.nSamplesPerSec, sizeof(hWaveFileHeader.nSamplesPerSec), 1, fd);
		bytes_read=fread(&hWaveFileHeader.nAvgBytesPerSec, sizeof(hWaveFileHeader.nAvgBytesPerSec), 1, fd);
		bytes_read=fread(&hWaveFileHeader.nBlockAlign, sizeof(hWaveFileHeader.nBlockAlign), 1, fd);
		bytes_read=fread(&hWaveFileHeader.wBitsPerSample, sizeof(hWaveFileHeader.wBitsPerSample), 1, fd);

		fseek(fd, sizeof(hWaveFileHeader.chRIFF)+sizeof(hWaveFileHeader.dwRIFFLen)+
					sizeof(hWaveFileHeader.chWAVE)+sizeof(hWaveFileHeader.chFMT)+
					sizeof(hWaveFileHeader.dwFMTLen)+hWaveFileHeader.dwFMTLen, SEEK_SET);
		bytes_read=fread(hWaveFileHeader.chDATA, sizeof(hWaveFileHeader.chDATA), 1, fd);
		printf("hWaveFileHeader.chDATA=%s\n", hWaveFileHeader.chDATA);
		if((hWaveFileHeader.chDATA[0]=='f')&&(hWaveFileHeader.chDATA[1]=='a')&&
					(hWaveFileHeader.chDATA[2]=='c')&&(hWaveFileHeader.chDATA[3]=='t'))
		{
			bytes_read=fread(&hWaveFileHeader.dwFACTLen, sizeof(hWaveFileHeader.dwFACTLen), 1, fd);
			fseek(fd, hWaveFileHeader.dwFACTLen, SEEK_CUR);
			bytes_read=fread(hWaveFileHeader.chDATA, sizeof(hWaveFileHeader.chDATA), 1, fd);
			printf("hWaveFileHeader.chDATA=%s\n", hWaveFileHeader.chDATA);
		}
		if((hWaveFileHeader.chDATA[0]=='d')&&(hWaveFileHeader.chDATA[1]=='a')&&
					(hWaveFileHeader.chDATA[2]=='t')&&(hWaveFileHeader.chDATA[3]=='a'))
		{
			bytes_read=fread(&hWaveFileHeader.dwDATALen, sizeof(hWaveFileHeader.dwDATALen), 1, fd);
			printf("hWaveFileHeader.dwDATALen=%d\n", hWaveFileHeader.dwDATALen);
		}

		if((hWaveFileHeader.chRIFF[0]!='R')||(hWaveFileHeader.chRIFF[1]!='I')||
					(hWaveFileHeader.chRIFF[2]!='F')||(hWaveFileHeader.chRIFF[3]!='F')||
					(hWaveFileHeader.chWAVE[0]!='W')||(hWaveFileHeader.chWAVE[1]!='A')||
					(hWaveFileHeader.chWAVE[2]!='V')||(hWaveFileHeader.chWAVE[3]!='E')||
					(hWaveFileHeader.chFMT[0]!='f')||(hWaveFileHeader.chFMT[1]!='m')||
					(hWaveFileHeader.chFMT[2]!='t')||(hWaveFileHeader.chFMT[3]!=' '))
		{
			printf("wav file format error\n");
			fclose(fd);
			return;
		}

		wavbuf.iput = 0;
		wavbuf.iget = 0;
		//最长留言时间30秒,在分配内存时多分配一倍
		wavbuf.buffer=(unsigned char *)malloc(hWaveFileHeader.dwDATALen);
		bytes_read=fread(wavbuf.buffer, hWaveFileHeader.dwDATALen, 1, fd);
		if(bytes_read != 1)
		{
			printf("Read Error:%s\n",srcname);
			if(wavbuf.buffer != NULL)
			{
				free(wavbuf.buffer);
				wavbuf.buffer = NULL;
			}
			fclose(fd);
			return;
		}
		wavbuf.n = hWaveFileHeader.dwDATALen;
		fclose(fd);
		printf("wavbuf.n = %d\n",wavbuf.n);
		//音频
		if(!OpenSnd(AUDIODSP1))
		{
			printf("Open play sound device error!\n");
			return;
		}      

		SetFormat(AUDIODSP1, hWaveFileHeader.wBitsPerSample,
					hWaveFileHeader.nSamplesPerSec, hWaveFileHeader.nChannels);    //放音
		//  SetFormat(AUDIODSP1, FMT16BITS, FMT8K, 1);    //放音   STERO, WAVOUTDEV

		audio_play_wav_flag = 1;
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
		pthread_create(&audio_play_wav_thread,&attr,(void *)audio_play_wav_thread_func, (int *)PlayFlag);
		pthread_attr_destroy(&attr);
		if ( audio_play_wav_thread == 0 ) {
			printf("无法创建声音播放线程\n");
			return;
		}
	}
	else
	{
		printf("声卡正忙\n");
		return;
	}
}
//---------------------------------------------------------------------------
void StopPlayWavFile(void)
{
	int delaytime;
	delaytime=40;
	audio_play_wav_flag = 0;
}
//---------------------------------------------------------------------------
void audio_play_wav_thread_func(int PlayFlag)
{
	int arg;	// 用于ioctl调用的参数
	int status;   // 系统调用的返回值
	struct timeval tv1;    

	int dwSize;
	int PrevStep = 0;
	int CurrStep = 0;
	int i;
	int AudioLen;
#ifdef _DEBUG
	printf("创建声音播放线程：\n" );
	//  printf("audio_play_wav_flag=%d\n",audio_play_wav_flag);
#endif
	//  printf("PlayFlag=%d, devplayfd = %d\n",PlayFlag, devplayfd);
	while(audio_play_wav_flag == 1)
	{
		if(wavbuf.iget == 0)
		{
			PrevStep = 0;
			CurrStep = 0;
		}
		if((wavbuf.iget + AUDIOBLK) < wavbuf.n)
		  AudioLen = AUDIOBLK;
		else
		  AudioLen = wavbuf.n - wavbuf.iget;

		//  printf("AudioLen=%d--devplayfd=%d\n",AudioLen,devplayfd);
		dwSize = Play(wavbuf.buffer +  wavbuf.iget, AudioLen);
		////////////////paul0302/////////////////
		//////////////////////////////////////////////
		//  printf("AudioLen=%d\n",AudioLen);
		wavbuf.iget += AudioLen;

		// 在继续录音前等待回放结束
		//   status = ioctl(devplayfd, SOUND_PCM_SYNC, 0);
		//   if (status == -1)
		//     printf("SOUND_PCM_WRITE_CHANNELS ioctl failed");


		if(wavbuf.iget >= wavbuf.n)
		{
			if(PlayFlag == 0)    //单次播放
			  StopPlayWavFile();
			else                 //循环播放
			{
				wavbuf.iget = 0;
			}
		}
	}
	//  if(PlayFlag == 0)    //单次播放
	//    CloseSnd(AUDIODSP1);
	if(wavbuf.buffer != NULL)
	{
		printf("free wavbuf.buffer\n");
		free(wavbuf.buffer);
		wavbuf.buffer = NULL;
	}

#ifdef _DEBUG
	printf("结束声音播放线程\n" );
#endif
	//         gettimeofday(&tv1, NULL);
	//         printf("tv1.tv_sec=%d, tv1.tv_usec=%d\n", tv1.tv_sec,tv1.tv_usec);
}
//---------------------------------------------------------------------------
TempAudioNode1 * init_audionode(void) //初始化单链表的函数
{
	TempAudioNode1 *h; // *h保存表头结点的指针，*p指向当前结点的前一个结点，*s指向当前结点
	if((h=(TempAudioNode1 *)malloc(sizeof(TempAudioNode1)))==NULL) //分配空间并检测
	{
		printf("不能分配内存空间!");
		return NULL;
	}
	h->llink=NULL; //左链域
	h->rlink=NULL; //右链域
	return(h);
}
//---------------------------------------------------------------------------
int creat_audionode(TempAudioNode1 *h, struct talkdata1 talkdata,
			unsigned char *r_buf , int r_length)
{
	TempAudioNode1 *t;
	TempAudioNode1 *p;
	int j;
	int DataOk;
	//    uint32_t newframeno;
	//    int currpackage;
	t=h;
	//  t=h->next;
	while(t->rlink!=NULL)    //循环，直到t指向空
	  t=t->rlink;   //t指向下一结点
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
	if(t&&(DataOk == 1))
	{
		//尾插法建立链表
		if((p=(TempAudioNode1 *)malloc(sizeof(TempAudioNode1)))==NULL) //生成新结点s，并分配内存空间
		{
			printf("不能分配内存空间!\n");
			return 0;
		}
		if((p->Content.buffer=(unsigned char *)malloc(talkdata.Framelen))==NULL)
		{
			printf("不能分配音频接收内存空间!\n");
			return 0;
		}
		p->Content.isFull = 0;
		for(j=0; j<MAXPACKNUM; j++)
		  p->Content.CurrPackage[j] = 0;

		p->Content.frame_flag = talkdata.DataType;
		//  newframeno = (r_buf[63] << 8) + r_buf[64];
		//  currpackage = (r_buf[67] << 8) + r_buf[68];
		p->Content.frameno = talkdata.Frameno;
		p->Content.TotalPackage = talkdata.TotalPackage;
		p->Content.timestamp = talkdata.timestamp;
		p->Content.CurrPackage[talkdata.CurrPackage - 1] = 1;
		if(talkdata.CurrPackage == p->Content.TotalPackage)
		  p->Content.Len =  (talkdata.CurrPackage - 1) * talkdata.PackLen + r_length - DeltaLen;
		//memcpy(p->Content.buffer + (talkdata.CurrPackage - 1) * talkdata.PackLen,
		//r_buf + DeltaLen, r_length - DeltaLen);
		memcpy(p->Content.buffer + (talkdata.CurrPackage - 1) * talkdata.PackLen,
					r_buf + DeltaLen,talkdata.Datalen);

		p->Content.isFull = 1;
		for(j=0; j< p->Content.TotalPackage; j++)
		  if(p->Content.CurrPackage[j] == 0)
		  {
			  p->Content.isFull = 0;
			  break;
		  }
		p->rlink=NULL;    //p的指针域为空
		p->llink=t;
		t->rlink=p;       //p的next指向这个结点
		//     t=p;             //t指向这个结点
		return p->Content.isFull;
	}
}
//---------------------------------------------------------------------------   
//函数名称：creat
//功能描述：在链表尾部添加数据
//返回类型：无返回值
//函数参数： h:单链表头指针
int creat_leavemovieaudionode(TempAudioNode1 *h, uint32_t rframeno, uint32_t rtimestamp,
			int rframe_flag, unsigned char *r_buf , int r_length)
{
	TempAudioNode1 *t;
	TempAudioNode1 *p;
	int j;
	uint32_t newframeno;
	t=h;
	//  t=h->next;
	while(t->rlink!=NULL)    //循环，直到t指向空
	  t=t->rlink;   //t指向下一结点
	if(t)
	{
		//尾插法建立链表
		if((p=(TempAudioNode1 *)malloc(sizeof(TempAudioNode1)))==NULL) //生成新结点s，并分配内存空间
		{
			printf("不能分配内存空间!\n");
			return 0;
		}
		if((p->Content.buffer=(unsigned char *)malloc(r_length))==NULL)
		{
			printf("不能分配音频接收内存空间!\n");
			return 0;
		}            
		p->Content.isFull = 0;
		for(j=0; j<MAXPACKNUM; j++)
		  p->Content.CurrPackage[j] = 0;

		p->Content.frame_flag = rframe_flag;
		p->Content.frameno = rframeno;
		p->Content.TotalPackage = (r_buf[65] << 8) + r_buf[66];
		p->Content.timestamp = rtimestamp;

		p->Content.Len =  r_length;
		memcpy(p->Content.buffer,
					r_buf, r_length);
		p->Content.isFull = 1;
		p->rlink=NULL;    //p的指针域为空
		p->llink=t;
		t->rlink=p;       //p的next指向这个结点
		//    t=p;             //t指向这个结点
		return p->Content.isFull;
	}
}
//---------------------------------------------------------------------------
//函数名称：length
//功能???：求单链表长度
//返回类型：无返回值
//函数参数：h:单链表头指针
int length_audionode(TempAudioNode1 *h)
{
	TempAudioNode1 *p;
	int i=0;         //记录链表长度
	p=h->rlink;
	while(p!=NULL)    //循环，直到p指向空
	{
		i=i+1;
		p=p->rlink;   //p指向下一结点
	}
	return i;
	//    printf(" %d",i); //输出p所指接点的数据域
}
//---------------------------------------------------------------------------
//函数名称：delete_
//功能描述：删除函数
//返回类型：整型
//函数参数：h:单链表头指针 i:要删除的位置
int delete_audionode(TempAudioNode1 *p)
{
	if(p != NULL)
	{
		//不为最后一个结点
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
	  printf("audio delete null\n");   
	return(0);
}
//---------------------------------------------------------------------------
int delete_all_audionode(TempAudioNode1 *h)
{
	TempAudioNode1 *p,*q;
	p=h->rlink;        //此时p为首结点
	while(p != NULL)   //找到要删除结点的位置
	{
		//不为最后一个结点
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
int delete_lost_audionode(TempAudioNode1 *h, uint32_t currframeno, uint32_t currtimestamp) //删除不全帧
{
	TempAudioNode1 *p,*q;
	p=h->rlink;        //此时p为首结点
	while(p != NULL)   //找到要删除结点的位置
	{
		//不为最后一个结点
		q = p;
		if(p->rlink != NULL)
		{
			//        if(p->Content.frameno < currframeno) //进入循环，直到p为空，或找到x
			if(p->Content.timestamp < currtimestamp)
			{
				(p->rlink)->llink=p->llink;
				(p->llink)->rlink=p->rlink;
				p = p->llink;
				if(q->Content.buffer)
				  free(q->Content.buffer);
				if(q)
				  free(q);
				if(temp_audio_n > 0)
				  temp_audio_n --;
			}
		}
		else
		{
			//        if(p->Content.frameno < currframeno) //进入循环，直到p为空，或找到x
			if(p->Content.timestamp < currtimestamp)
			{
				(p->llink)->rlink=p->rlink;
				p = p->llink;
				if(q->Content.buffer)
				  free(q->Content.buffer);
				if(q)
				  free(q);
				if(temp_audio_n > 0)
				  temp_audio_n --;             
			}
		}
		p = p->rlink;
	}
	return 1;
}
//---------------------------------------------------------------------------
//函数名称：locate_
//功能描述：定位函数
//返回类型：整型
//函数参数：h:单链表头指针 i:要定位的位置
TempAudioNode1 * locate_audionode(TempAudioNode1 *h,int i)
{
	TempAudioNode1 *p;
	int j;
	p=h->rlink;    //此时p为首结点
	j=1;
	while(p&&j<i)  //找到要定位的位置
	{
		++j;
		p=p->rlink;  //p指向下一结点
	}
	if(i>0&&j==i)
	  return p;
	else
	  return NULL;
}
//---------------------------------------------------------------------------
//函数名称：find_
//功能描述：查找函数
//返回类型：整型
//函数参数：h:单链表头指针 x:要查找的值
//查找该帧该包是否已存在
TempAudioNode1 * find_audionode(TempAudioNode1 *h, int currframeno, int currpackage)
{
	TempAudioNode1 *p;
	int PackIsExist; //数据包已接收标志
	int FrameIsNew;  //数据包是否是新帧的开始
	p=h->rlink;    //此时p为首结点
	PackIsExist = 0;
	FrameIsNew = 1;
	while(p!=NULL)
	{
		if(p->Content.frameno == currframeno) //进入循环，直到p为空，或找到x
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
		p=p->rlink;   //s指向p的下一结点
	}
	if(p!=NULL)
	  return p;
	else
	  return NULL;
}
//---------------------------------------------------------------------------
TempAudioNode1 * find_frame_audionode(TempAudioNode1 *h, int currframeno)
{
	TempAudioNode1 *p;
	p=h->rlink;    //此时p为首结点
	while(p!=NULL)
	{
		if(p->Content.frameno == currframeno) //进入循环，直到p为空，或找到x
		  break;
		p=p->rlink;   //s指向p的下一结点
	}
	if(p!=NULL)
	  return p;
	else
	  return NULL;
}
//---------------------------------------------------------------------------
//函数名称：find_
//功能描述：查找函数
//返回类型：整型
//函数参数：h:单链表头指针 x:要查找的值
//查找最老的帧
TempAudioNode1 * search_audionode(TempAudioNode1 *h)
{
	TempAudioNode1 *p;
	TempAudioNode1 *tem_p;

	p=h->rlink;    //此时p为首结点
	tem_p = p;

	while(p!=NULL)
	{
		if(p->Content.isFull == 1)
		  //     if(p->Content.frameno < tem_p->Content.frameno) //进入循环，直到p为空，或找到x
		  if(p->Content.timestamp < tem_p->Content.timestamp) //进入循环，直到p为空，或找到x
		  {
			  tem_p = p;
		  }
		p=p->rlink;   //s指向p的下一结点
	}

	return tem_p;

}
//---------------------------------------------------------------------------
int free_audionode(TempAudioNode1 *h)
{
	TempAudioNode1 *p,*t;
	int i=0;         //记录链表长度
	p=h->rlink;
	while(p!=NULL)    //循环，直到p指向空
	{
		i=i+1;
		t = p;
		p=p->rlink;   //p指向下一结点
		free(t);
	}
	return i;
}
//-------------------------------------------------------------------------




