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
int PlayPcmTotalNum;   //һ��������Ƶ���ܵķ���ֵ
extern int AudioMuteFlag;   //������־
//��Ƶ
int audio_rec_flag;
int audio_play_flag;
pthread_t audio_rec_deal_thread;      //��Ƶ�ɼ����ݴ����߳�
pthread_t audio_play_deal_thread;     //��Ƶ�������ݴ����߳�
pthread_t audio_rec_thread;      //��Ƶ�ɼ��߳�
pthread_t audio_play_thread;     //��Ƶ�����߳�
void audio_rec_deal_thread_func(void);
void audio_play_deal_thread_func(void);
void audio_rec_thread_func(void);
void audio_play_thread_func(void);


struct audiobuf1 playbuf;    //��Ƶ���Ż��λ���
struct audiobuf1 recbuf;     //��Ƶ�ɼ����λ���

sem_t audioplaysem;
sem_t audiorecsem;
sem_t audiorec2playsem;

void StartRecAudio(void);
void StopRecAudio(void);
void StartPlayAudio(void);
void StopPlayAudio(void);

extern struct _SYNC sync_s;
extern struct timeval ref_time;  //��׼ʱ��,��Ƶ����Ƶ��һ֡

extern int curr_audio_timestamp;

//Ϊ��ֹ��β������´���
int AudioRecIsStart=0;
int AudioPlayIsStart=0;

//������������
struct wavbuf1 wavbuf;       //�������Ż��λ���
int audio_play_wav_flag=0;
pthread_t audio_play_wav_thread;     //���Բ����߳�
void audio_play_wav_thread_func(int PlayFlag);
struct WaveFileHeader hWaveFileHeader;
//������������
void StartPlayWav(char *srcname, int PlayFlag); //0 ���β���  1 ѭ������
void StopPlayWavFile(void);

//Ŀ¼���ļ�����
// *h�����ͷ����ָ�룬*pָ��ǰ����ǰһ����㣬*sָ��ǰ���

TempAudioNode1 * init_audionode(void); //��ʼ��������ĺ���
//������������������
int length_audionode(TempAudioNode1 *h);
//����������β�����
int creat_audionode(TempAudioNode1 *h, struct talkdata1 talkdata, unsigned char *r_buf ,
			int r_length);

//��Ӱ����
int creat_leavemovieaudionode(TempAudioNode1 *h, uint32_t rframeno, uint32_t rtimestamp,
			int rframe_flag, unsigned char *r_buf , int r_length);
//����������ɾ������
int delete_audionode(TempAudioNode1 *p);
int delete_all_audionode(TempAudioNode1 *h);
int delete_lost_audionode(TempAudioNode1 *h, uint32_t currframeno, uint32_t currtimestamp); //ɾ����ȫ֡
//������������λ����
TempAudioNode1 * locate_audionode(TempAudioNode1 *h,int i);
//�������������Һ���
TempAudioNode1 * find_audionode(TempAudioNode1 *h, int currframeno, int currpackage);
//�������������Һ���
TempAudioNode1 * find_frame_audionode(TempAudioNode1 *h, int currframeno);
//�������ϵ�֡
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
	int status;   // ϵͳ���õķ���ֵ
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
		// �ȴ��طŽ���
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

	//  int fd;	// �����豸���ļ�������
	int arg;	// ����ioctl���õĲ���
	int status;   // ϵͳ���õķ���ֵ
	int devfd;
	if(nWhich == 1)
	  devfd = devrecfd;
	else
	  devfd = devplayfd;

	/*  arg = 0;
		ioctl (devfd, SNDCTL_DSP_RESET,(char *)&arg) ;
		arg = 1;
		ioctl (devfd, SNDCTL_DSP_SYNC,(char *)&arg);    */

	// ���ò���ʱ������λ��       FIC8120ֻ֧��16λ
	arg = bits;
	//  printf("set bits is = %d\n", arg);
	status = ioctl(devfd, SOUND_PCM_WRITE_BITS, &arg);
	if (status == -1)
	  printf("SOUND_PCM_WRITE_BITS ioctl failed");
	if (arg != bits)
	  printf("unable to set sample size");
	//  status = ioctl(devfd, SOUND_PCM_READ_BITS, &arg);
	//  printf("get bits is = %d\n", arg);


	// ���ò���ʱ��������Ŀ
	arg = chn;
	status = ioctl(devfd, SOUND_PCM_WRITE_CHANNELS, &arg);
	if (status == -1)
	  perror("SOUND_PCM_WRITE_CHANNELS ioctl failed");
	if (arg != chn)
	  perror("unable to set number of channels");
	// ���ò���ʱ�Ĳ���Ƶ�� 
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
	//ͨ�����ݽṹ
	struct talkdata1 talkdata;
#ifdef _DEBUG
	printf("�����ɼ����ݴ����̣߳�\n" );
#endif
	while(audio_rec_flag == 1)
	{
		//�ȴ��ɼ��߳������ݵ��ź�
		sem_wait(&audiorecsem);
		//����
		pthread_mutex_lock(&sync_s.audio_rec_lock);
		FrameNum = recbuf.n/AUDIOBLK;

		for(i=0; i<FrameNum; i++)
		{
			//ͷ��
			memcpy(adpcm_out, UdpPackageHead, 6);
			//����
			//  if(Remote.isDirect == 1)
			//    adpcm_out[6] = VIDEOTALKTRANS;
			//  else
			adpcm_out[6] = VIDEOTALK;
			adpcm_out[7] = 1;
			//������
			//������
			if((Local.Status == 1)||(Local.Status == 3)||(Local.Status == 5))  //����Ϊ���з�
			{
				adpcm_out[8] = CALLUP;
				memcpy(talkdata.HostAddr, LocalCfg.Addr, 20);
				memcpy(talkdata.HostIP, LocalCfg.IP, 4);
				memcpy(talkdata.AssiAddr, Remote.Addr[0], 20);
				memcpy(talkdata.AssiIP, Remote.IP[0], 4);
			}
			if((Local.Status == 2)||(Local.Status == 4)||(Local.Status == 6))  //����Ϊ���з�
			{
				adpcm_out[8] = CALLDOWN;
				memcpy(talkdata.HostAddr, Remote.Addr[0], 20);
				memcpy(talkdata.HostIP, Remote.IP[0], 4);
				memcpy(talkdata.AssiAddr, LocalCfg.Addr, 20);
				memcpy(talkdata.AssiIP, LocalCfg.IP, 4);
			}

			//ʱ���
			talkdata.timestamp = recbuf.timestamp[recbuf.iget/AUDIOBLK];
			//��������
			talkdata.DataType = 1;
			//֡���
			talkdata.Frameno = recbuf.frameno[recbuf.iget/AUDIOBLK];
			//֡���ݳ���
			talkdata.Framelen = AUDIOBLK/2;
			//�ܰ���
			talkdata.TotalPackage = 1;
			//��ǰ��
			talkdata.CurrPackage = 1;
			//���ݳ���
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
				//UDP����
				sprintf(RemoteHost, "%d.%d.%d.%d\0",Remote.DenIP[0],
							Remote.DenIP[1],Remote.DenIP[2],Remote.DenIP[3]);
				UdpSendBuff(m_VideoSendSocket, RemoteHost, adpcm_out, DeltaLen + AUDIOBLK/2);
			}
		}
		//����
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
	printf("����¼���̣߳�\n" );
	printf("audio_rec_flag=%d\n",audio_rec_flag);
#endif  
	gettimeofday(&tv, NULL);
	nowtime = tv.tv_sec *1000 + tv.tv_usec/1000;  
	while(audio_rec_flag == 1)
	{
		//����
		pthread_mutex_lock(&sync_s.audio_rec_lock);

		recbuf.frameno[recbuf.iput/AUDIOBLK] = Local.nowaudioframeno;
		Local.nowaudioframeno++;
		if(Local.nowaudioframeno >= 65536)
		  Local.nowaudioframeno = 1;

		//ʱ���
		gettimeofday(&tv, NULL);
		//��һ֡,�趨��ʼʱ���
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

		//����
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
	printf("\naudio_play_deal_thread_func leftmessage��%d\n\n",Local.leftmessage );
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
		//�ȴ��ɼ��߳������ݵ��ź�, ������
		//�ȴ�UDP�����߳������ݵ��ź�
		if(Local.leftmessage != 0x01)
		  sem_wait(&audiorec2playsem);
		//����
		pthread_mutex_lock(&sync_s.audio_play_lock);
		//    while(temp_audio_n > 0)
		if(temp_audio_n > 0 || Local.leftmessage == 0x01)
		{
			//����
			//�������ϵ�֡
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

					//һ��������Ƶ���ܵķ���ֵ
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
			//ɾ����ȫ֡
			delete_lost_audionode(TempAudioNode_h, dellostframeno, dellosttimestamp);
		}
		//����
		pthread_mutex_unlock(&sync_s.audio_play_lock);
	}
}
//---------------------------------------------------------------------------
void audio_play_thread_func(void)
{
	char *audio_out;
	int dwSize;
	int i;
	int jump_buf;  //�ѽ��뻺������֡��
	int jump_tmp;  //���ջ�������֡��
	int jump_frame;
	int aframe;
	TempAudioNode1 * tmp_audionode;
#ifdef _DEBUG
	printf("CREATING AUDIO_PLAY_THREAD��\n" );
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
			//����
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
			if((TimeStamp.OldCurrAudio != TimeStamp.CurrAudio) //��һ�ε�ǰ��Ƶʱ��
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
						//�������ϵ�֡
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

			//����
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
	printf("\naudio_play_deal_thread_func leftmessage��%d\n\n",Local.leftmessage );
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
		//�ȴ��ɼ��߳������ݵ��ź�, ������
		//�ȴ�UDP�����߳������ݵ��ź�
		if (Local.leftmessage != 0x01) {
            sem_wait(&audiorec2playsem);
        }
		//����
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
                //����
                //�������ϵ�֡
                tmp_audionode = search_audionode(TempAudioNode_h);
                if (tmp_audionode != NULL) {
                    //printf("DECODing\n");
                    playbuf.frameno[playbuf.iput/AUDIOBLK] = tmp_audionode->Content.frameno;
                    playbuf.timestamp[playbuf.iput/AUDIOBLK] = tmp_audionode->Content.timestamp;
                    G711Decoder(playppcm, tmp_audionode->Content.buffer, AUDIOBLK/2,1);

                    //һ��������Ƶ���ܵķ���ֵ
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
            //ɾ����ȫ֡
            delete_lost_audionode(TempAudioNode_h, dellostframeno, dellosttimestamp);
        }
        //����
        pthread_mutex_unlock(&sync_s.audio_play_lock);
    }
}
//---------------------------------------------------------------------------
void audio_play_thread_func(void)
{
	char *audio_out;
	int dwSize;
	int i;
	int jump_buf;  //�ѽ��뻺������֡��
	int jump_tmp;  //���ջ�������֡��
	int jump_frame;
	int aframe;
	TempAudioNode1 * tmp_audionode;
#ifdef _DEBUG
	printf("CREATING AUDIO_PLAY_THREAD��\n" );
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
			//����
			//   dwSize = AUDIOBLK;
			if((playbuf.iget + dwSize) >= NMAX)
			  playbuf.iget = 0;
			else
			  playbuf.iget += dwSize;
			if(playbuf.n >= dwSize)
			  playbuf.n -=  dwSize;
			else
			  printf("play2.Buffer is full\n");

			//����
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
		PlayPcmTotalNum = 0;   //һ��������Ƶ���ܵķ���ֵ
		//ѡͨ��Ƶ ,�ߵ�ƽ��Ч
		//doors
		//	if((Local.DoorNo >= 0x31)&&(Local.DoorNo <= 0x32))
		{
			ioctl(gpio_fd, IO_PUT, 0);
			//ioctl(gpio_fd, IO_PUT, Local.DoorNo - 0x31);
		}
		//	else
		//		printf("��ʼͨ�� Local.DoorNo = 0x%X, �쳣\n", Local.DoorNo);
		//��Ƶ
		if(!OpenSnd(AUDIODSP))
		{
			printf("Open record sound device error!\\n");
			return;
		}

		SetFormat(AUDIODSP, FMT16BITS, FMT8K, 1/*STERO, WAVOUTDEV*/);    //¼��

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
			printf("�޷�������Ƶ�ɼ����ݴ����߳�\n");
			pthread_attr_destroy(&attr);
			return;
		}

		pthread_create(&audio_rec_thread,&attr,(void *)audio_rec_thread_func,NULL);
		pthread_attr_destroy(&attr);
		if ( audio_rec_thread == 0 ) {
			printf("�޷�������Ƶ�ɼ��߳�\n");
			return;
		}
	}  
	else
	{
#ifdef _DEBUG
		printf("�ظ� AudioRecIsStart\n");
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
		ref_time.tv_sec = 0;   //��ʼʱ���
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
		//�ر���Ƶ ,�ߵ�ƽ��Ч
		//doorsif((Local.DoorNo >= 0x31)&&(Local.DoorNo <= 0x34))
		{
			ioctl(gpio_fd, IO_CLEAR, 0);
			//ioctl(gpio_fd, IO_CLEAR, Local.DoorNo - 0x31);
		}
		//	else
		//		printf("�ر���Ƶ Local.DoorNo = 0x%X, �쳣\n", Local.DoorNo);
		sem_destroy(&audiorecsem);
		pthread_mutex_destroy(&sync_s.audio_rec_lock);
	}
	else
	{
#ifdef _DEBUG
		printf("�ظ� AudioRecIsStart\n");
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
		TimeStamp.OldCurrVideo = 0;       //��һ�ε�ǰ��Ƶʱ��
		TimeStamp.CurrVideo = 0;
		TimeStamp.OldCurrAudio = 0;       //��һ�ε�ǰ��Ƶʱ��
		TimeStamp.CurrAudio = 0;

		AudioPlayIsStart = 1;
		PlayPcmTotalNum = 0;   //һ��������Ƶ���ܵķ���ֵ
		//��Ƶ

		//////////////////////paul0302////////////////////////
		if(!OpenSnd(AUDIODSP1))
		{
			printf("Open play sound device error!\n");
			return;
		}     
		////////////////////////////////////////////////////

		SetFormat(AUDIODSP1, FMT16BITS, FMT8K, 1);    //���� STERO, WAVOUTDEV

		playbuf.iput = 0;
		playbuf.iget = 0;
		playbuf.n = 0;
		for(i=0; i<NMAX/AUDIOBLK; i++)
		{
			playbuf.frameno[i] = 0;
			playbuf.timestamp[i] = 0;
		}

		temp_audio_n = 0;
		//��Ƶ���ջ�������
		delete_all_audionode(TempAudioNode_h);     

		sem_init(&audioplaysem,0,0);
		sem_init(&audiorec2playsem,0,0);
		//�鿴ͬ�������߳��Ƿ��Ѵ���
		// sync_play_init();

		audio_play_flag = 1;
		pthread_mutex_init (&sync_s.audio_play_lock, NULL); 

#if 1
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
		pthread_create(&audio_play_deal_thread,&attr,(void *)audio_play_deal_thread_func,NULL);
		if ( audio_play_deal_thread == 0 ) {
			printf("�޷������������ݴ����߳�\n");
			pthread_attr_destroy(&attr);
			return;
		}
		pthread_create(&audio_play_thread,&attr, (void *)audio_play_thread_func, NULL);
		pthread_attr_destroy(&attr);
		if ( audio_play_thread == 0 ) {
			printf("�޷�������Ƶ�����߳�\n");
			return;
		}         
#endif
	}  
	else
	{
#ifdef _DEBUG
		printf("�ظ� AudioPlayStart\n");
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
		//�ͷ�ͬ�������̼߳�����
		//  sync_play_destroy();

		temp_audio_n = 0;
		//��Ƶ���ջ�������
		delete_all_audionode(TempAudioNode_h);    
	}
	else
	{
#ifdef _DEBUG
		printf("�ظ� AudioPlayStop\n");
#endif
	}
}
//---------------------------------------------------------------------------
void StartPlayWav(char *srcname, int PlayFlag) //0 ���β���  1 ѭ������
{
	pthread_attr_t attr;
	int i;
	FILE* fd;
	int bytes_read;
	//�鿴��Ƶ�豸�Ƿ����
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
		//�����ʱ��30��,�ڷ����ڴ�ʱ�����һ��
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
		//��Ƶ
		if(!OpenSnd(AUDIODSP1))
		{
			printf("Open play sound device error!\n");
			return;
		}      

		SetFormat(AUDIODSP1, hWaveFileHeader.wBitsPerSample,
					hWaveFileHeader.nSamplesPerSec, hWaveFileHeader.nChannels);    //����
		//  SetFormat(AUDIODSP1, FMT16BITS, FMT8K, 1);    //����   STERO, WAVOUTDEV

		audio_play_wav_flag = 1;
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
		pthread_create(&audio_play_wav_thread,&attr,(void *)audio_play_wav_thread_func, (int *)PlayFlag);
		pthread_attr_destroy(&attr);
		if ( audio_play_wav_thread == 0 ) {
			printf("�޷��������������߳�\n");
			return;
		}
	}
	else
	{
		printf("������æ\n");
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
	int arg;	// ����ioctl���õĲ���
	int status;   // ϵͳ���õķ���ֵ
	struct timeval tv1;    

	int dwSize;
	int PrevStep = 0;
	int CurrStep = 0;
	int i;
	int AudioLen;
#ifdef _DEBUG
	printf("�������������̣߳�\n" );
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

		// �ڼ���¼��ǰ�ȴ��طŽ���
		//   status = ioctl(devplayfd, SOUND_PCM_SYNC, 0);
		//   if (status == -1)
		//     printf("SOUND_PCM_WRITE_CHANNELS ioctl failed");


		if(wavbuf.iget >= wavbuf.n)
		{
			if(PlayFlag == 0)    //���β���
			  StopPlayWavFile();
			else                 //ѭ������
			{
				wavbuf.iget = 0;
			}
		}
	}
	//  if(PlayFlag == 0)    //���β���
	//    CloseSnd(AUDIODSP1);
	if(wavbuf.buffer != NULL)
	{
		printf("free wavbuf.buffer\n");
		free(wavbuf.buffer);
		wavbuf.buffer = NULL;
	}

#ifdef _DEBUG
	printf("�������������߳�\n" );
#endif
	//         gettimeofday(&tv1, NULL);
	//         printf("tv1.tv_sec=%d, tv1.tv_usec=%d\n", tv1.tv_sec,tv1.tv_usec);
}
//---------------------------------------------------------------------------
TempAudioNode1 * init_audionode(void) //��ʼ��������ĺ���
{
	TempAudioNode1 *h; // *h�����ͷ����ָ�룬*pָ��ǰ����ǰһ����㣬*sָ��ǰ���
	if((h=(TempAudioNode1 *)malloc(sizeof(TempAudioNode1)))==NULL) //����ռ䲢���
	{
		printf("���ܷ����ڴ�ռ�!");
		return NULL;
	}
	h->llink=NULL; //������
	h->rlink=NULL; //������
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
	while(t->rlink!=NULL)    //ѭ����ֱ��tָ���
	  t=t->rlink;   //tָ����һ���
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
		//β�巨��������
		if((p=(TempAudioNode1 *)malloc(sizeof(TempAudioNode1)))==NULL) //�����½��s���������ڴ�ռ�
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
		p->rlink=NULL;    //p��ָ����Ϊ��
		p->llink=t;
		t->rlink=p;       //p��nextָ��������
		//     t=p;             //tָ��������
		return p->Content.isFull;
	}
}
//---------------------------------------------------------------------------   
//�������ƣ�creat
//����������������β���������
//�������ͣ��޷���ֵ
//���������� h:������ͷָ��
int creat_leavemovieaudionode(TempAudioNode1 *h, uint32_t rframeno, uint32_t rtimestamp,
			int rframe_flag, unsigned char *r_buf , int r_length)
{
	TempAudioNode1 *t;
	TempAudioNode1 *p;
	int j;
	uint32_t newframeno;
	t=h;
	//  t=h->next;
	while(t->rlink!=NULL)    //ѭ����ֱ��tָ���
	  t=t->rlink;   //tָ����һ���
	if(t)
	{
		//β�巨��������
		if((p=(TempAudioNode1 *)malloc(sizeof(TempAudioNode1)))==NULL) //�����½��s���������ڴ�ռ�
		{
			printf("���ܷ����ڴ�ռ�!\n");
			return 0;
		}
		if((p->Content.buffer=(unsigned char *)malloc(r_length))==NULL)
		{
			printf("���ܷ�����Ƶ�����ڴ�ռ�!\n");
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
		p->rlink=NULL;    //p��ָ����Ϊ��
		p->llink=t;
		t->rlink=p;       //p��nextָ��������
		//    t=p;             //tָ��������
		return p->Content.isFull;
	}
}
//---------------------------------------------------------------------------
//�������ƣ�length
//����???����������
//�������ͣ��޷���ֵ
//����������h:������ͷָ��
int length_audionode(TempAudioNode1 *h)
{
	TempAudioNode1 *p;
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
int delete_audionode(TempAudioNode1 *p)
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
	  printf("audio delete null\n");   
	return(0);
}
//---------------------------------------------------------------------------
int delete_all_audionode(TempAudioNode1 *h)
{
	TempAudioNode1 *p,*q;
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
int delete_lost_audionode(TempAudioNode1 *h, uint32_t currframeno, uint32_t currtimestamp) //ɾ����ȫ֡
{
	TempAudioNode1 *p,*q;
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
				if(temp_audio_n > 0)
				  temp_audio_n --;
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
				if(temp_audio_n > 0)
				  temp_audio_n --;             
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
TempAudioNode1 * locate_audionode(TempAudioNode1 *h,int i)
{
	TempAudioNode1 *p;
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
TempAudioNode1 * find_audionode(TempAudioNode1 *h, int currframeno, int currpackage)
{
	TempAudioNode1 *p;
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
TempAudioNode1 * find_frame_audionode(TempAudioNode1 *h, int currframeno)
{
	TempAudioNode1 *p;
	p=h->rlink;    //��ʱpΪ�׽��
	while(p!=NULL)
	{
		if(p->Content.frameno == currframeno) //����ѭ����ֱ��pΪ�գ����ҵ�x
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
TempAudioNode1 * search_audionode(TempAudioNode1 *h)
{
	TempAudioNode1 *p;
	TempAudioNode1 *tem_p;

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
int free_audionode(TempAudioNode1 *h)
{
	TempAudioNode1 *p,*t;
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
//-------------------------------------------------------------------------




