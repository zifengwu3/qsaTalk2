#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <semaphore.h>       //sem_t
#include <dirent.h>
#include <errno.h>

#define VAR_STATIC

#define CommonH
#include "common.h"
#include "sndtools.h"
#include "g711common.h"

int devrecfd = (-1);
int devplayfd = (-1);
extern int g_ossPlayFlag;
extern g_iOutputLocalWavVol;

int SndRecAudioOpen = 0;
int SndPlayAudioOpen = 1;

int audio, abuf_size, zbuf_size;

int PlayPcmTotalNum;
extern int AudioMuteFlag;
extern int SipAudioOpen;
extern pthread_mutex_t audio_open_lock;
extern pthread_mutex_t audio_close_lock;
extern pthread_mutex_t audio_lock;

extern int SndCardDetect(void);
extern int SndCardAudioRecordInit(void);
extern int AudioRecord(char *readBuf, int len);
extern int SndCardAudioRecordUninit(void);
extern int SndCardAudioPlayInit(void);
extern int AudioPlay(char *readBuf, int writeLen);
extern int SndCardAudioPlayUnnit(void);

int curr_audio_timestamp;
int audio_rec_flag;
int audio_play_flag;
pthread_t audio_rec_deal_thread;
pthread_t audio_play_deal_thread;
pthread_t audio_rec_thread;
pthread_t audio_play_thread;
void audio_rec_deal_thread_func(void);
void audio_play_deal_thread_func(void);
void audio_rec_thread_func(void);
void audio_play_thread_func(void);

struct audiobuf1 playbuf;
struct audiobuf1 recbuf;

sem_t audioplaysem;
sem_t audiorecsem;
sem_t audiorec2playsem;

void StartRecAudio(void);
void StopRecAudio(void);
void StartPlayAudio(void);
void StopPlayAudio(void);

void WaitAudioUnuse(int MaxTime);

struct _SYNC sync_s;
struct timeval ref_time;

extern int curr_audio_timestamp;

int AudioRecIsStart = 0;
int AudioPlayIsStart = 0;

struct pcm *pcm_play;

void InitAudioParam(void);
void SyncPlay(void);

struct wavbuf1 wavbuf;
int audio_play_wav_flag;
pthread_t audio_play_wav_thread;

void audio_play_wav_thread_func(int PlayFlag);
struct WaveFileHeader hWaveFileHeader;

void StartPlayWav(char *srcname, int PlayFlag);
//void StartPlayWavFile(void);
void StopPlayWavFile(void);

TempAudioNode1 * init_audionode(void);
int length_audionode(TempAudioNode1 *h);
int creat_audionode(TempAudioNode1 *h, struct talkdata1 talkdata,
		unsigned char *r_buf, int r_length);

int delete_audionode(TempAudioNode1 *p);
int delete_all_audionode(TempAudioNode1 *h);
int delete_lost_audionode(TempAudioNode1 *h, uint32_t currframeno,
		uint32_t currtimestamp);
TempAudioNode1 * find_audionode(TempAudioNode1 *h, int currframeno,
		int currpackage);
TempAudioNode1 * search_audionode(TempAudioNode1 *h);
//
int free_audionode(TempAudioNode1 *h);
//---------------------------------------------------------------------------
void InitAudioParam(void)
{
}
//---------------------------------------------------------------------------
/*
 * Set Record an Playback format
 * return 1 if success, else return 0.
 * bits -- FMT8BITS(8bits), FMT16BITS(16bits)
 * hz -- FMT8K(8000HZ), FMT16K(16000HZ), FMT22K(22000HZ), FMT44K(44000HZ)
 chn -- MONO 1 STERO 2
 */
int SetFormat(int nWhich, int bits, int hz, int chn) {
	int samplesize;
	int tmp;
	int dsp_stereo;
	int setting;

	int arg; // ����ioctl���õĲ���
	int status; // ϵͳ���õķ���ֵ
	int devfd;
	if (nWhich == 1)
		devfd = devrecfd;
	else
		devfd = devplayfd;

	// ���ò���ʱ������λ��       FIC8120ֻ֧��16λ
	sprintf(Local.DebugInfo, "bits = %d, hz = %d, chn = %d, devfd = %d\n", bits,
			hz, chn, devfd);
	printf(Local.DebugInfo);
	if (devfd <= 0)
		return -1;
#if 0
	arg = bits;
	ioctl(devfd, SNDCTL_DSP_SAMPLESIZE, &arg);
#else
	status = ioctl(devfd, SOUND_PCM_READ_BITS, &arg);
	if (arg != bits) {
		arg = bits;
		status = ioctl(devfd, SOUND_PCM_WRITE_BITS, &arg);
		if (status == -1)
			printf("SOUND_PCM_WRITE_BITS ioctl failed\n");
		if (arg != bits)
			printf("unable to set sample size\n");
	}
#endif

	// ���ò���ʱ�Ĳ���Ƶ��
#if 0
	arg = hz;
	status = ioctl(devfd, SNDCTL_DSP_SPEED, &arg);
#else
	status = ioctl(devfd, SOUND_PCM_READ_RATE, &arg);
	if (arg != hz) {
		arg = hz;
		status = ioctl(devfd, SOUND_PCM_WRITE_RATE, &arg);
		if (status == -1)
			perror("SOUND_PCM_WRITE_RATE ioctl failed");
	}
#endif

	// ���ò���ʱ�������Ŀ
#if 0
	arg = chn;
	arg = 0;
	ioctl(devfd, SNDCTL_DSP_STEREO, &arg);
#else
	status = ioctl(devfd, SOUND_PCM_READ_CHANNELS, &arg);
	if (arg != chn) {
		arg = chn;
		status = ioctl(devfd, SOUND_PCM_WRITE_CHANNELS, &arg);
		if (status == -1)
			perror("SOUND_PCM_WRITE_CHANNELS ioctl failed");
		if (arg != chn)
			perror("unable to set number of channels");
	}
#endif

	abuf_size = AUDIOBLK;
	ioctl(devfd, SNDCTL_DSP_GETBLKSIZE, &abuf_size);
	if (DebugMode == 1) {
		sprintf(Local.DebugInfo, "abuf_size= %d\n", abuf_size);
		printf(Local.DebugInfo);
	}
	if (abuf_size < 4 || abuf_size > 65536) {
		sprintf(Local.DebugInfo, "Invalid audio buffers size %d\n", nWhich);
		printf(Local.DebugInfo);
		exit(-1);
	}

	return 1;
}

//---------------------------------------------------------------------------
/*
 * Record
 * return numbers of byte for read.
 */
int Record(char *buf, int size) {
	int status;
	status = 0;
	if (devrecfd > 0) {
		//printf("Record 1, devrecfd = %d\n", devrecfd);
		//pthread_mutex_lock(&sync_s.audio_lock);
		status = read(devrecfd, buf, size);
		//printf("Record 2, status = %d\n", status);
		//pthread_mutex_unlock(&sync_s.audio_lock);
	}
	return status;
}
//---------------------------------------------------------------------------
/* 
 * Playback
 * return numbers of byte for write.
 */

int Play(char *buf, int size) {
	int status;
	status = 0;

	if (devplayfd > 0) {
		//pthread_mutex_lock(&sync_s.audio_lock);
		//printf("Play 1\n");

		/*if(SipRecAudioOpen == 1)
		 {
		 status = write(fdNull, buf, size);
		 }
		 else
		 {
		 status = write(devplayfd, buf, size);
		 }*/

		status = write(devplayfd, buf, size);
		//printf("the status is %d\n", status);

		//printf("Play 2\n");
		//pthread_mutex_unlock(&sync_s.audio_lock);
	}
	return status;
}

void G711Encoder(short *pPcmBuf, char *pAudioBuf, int AudioLen, int type) {
	int i;

	for (i = 0; i < AudioLen; i++) {
		*pAudioBuf = s16_to_ulaw(*pPcmBuf);
		pPcmBuf++;
		pAudioBuf++;
	}
	return;
}

//---------------------------------------------------------------------------
void audio_rec_deal_thread_func(void) {
	short *rec_pcm;
	short recppcm[AUDIOBLK];
	char *input_buf;
	char e_buf[AUDIOBLK * 2 + 1];
	char *ref_buf;
	struct timeval tv, tv1;
	int dwSize;
	int i;
	unsigned char adpcm_out[3072];
	char RemoteHost[20];
	int RemotePort;
	int FrameNum;
	//ͨ����ݽṹ
	struct talkdata1 talkdata;

	if (DebugMode == 1)
		printf("don't create audio recv deal thread \n");
	while (audio_rec_flag == 1) {
		//�ȴ�ɼ��߳�����ݵ��ź�
		sem_wait(&audiorecsem);
		//����
		pthread_mutex_lock(&sync_s.audio_rec_lock);
		FrameNum = recbuf.n / AUDIOBLK;

		// if(FrameNum > 3)
		//   FrameNum -= 3;
		for (i = 0; i < FrameNum; i++) {
			//ͷ��
			memcpy(adpcm_out, UdpPackageHead, 6);
			//����
			if (Remote.isDirect == 0) //ֱͨ����
				adpcm_out[6] = VIDEOTALK;
			else
				//��ת����
				adpcm_out[6] = VIDEOTALKTRANS;
			adpcm_out[7] = 1;
			//������
			if ((Local.Status == 1) || (Local.Status == 3)
					|| (Local.Status == 5) || (Local.Status == 7)
					|| (Local.Status == 9)) //����Ϊ���з�
					{
				adpcm_out[8] = CALLUP;
				memcpy(talkdata.HostAddr, LocalCfg.Addr, 20);
				memcpy(talkdata.HostIP, LocalCfg.IP, 4);
				memcpy(talkdata.AssiAddr, Remote.Addr[0], 20);
				if (Remote.DenNum == 1)
					memcpy(talkdata.AssiIP, Remote.IP[0], 4);
				else
					memcpy(talkdata.AssiIP, Remote.GroupIP, 4);
			}
			if ((Local.Status == 2) || (Local.Status == 4)
					|| (Local.Status == 6) || (Local.Status == 8)
					|| (Local.Status == 10)) //����Ϊ���з�
					{
				adpcm_out[8] = CALLDOWN;
				memcpy(talkdata.HostAddr, Remote.Addr[0], 20);
				if (Remote.DenNum == 1)
					memcpy(talkdata.HostIP, Remote.IP[0], 4);
				else
					memcpy(talkdata.HostIP, Remote.GroupIP, 4);
				memcpy(talkdata.AssiAddr, LocalCfg.Addr, 20);
				memcpy(talkdata.AssiIP, LocalCfg.IP, 4);
			}
			//ʱ���
			talkdata.timestamp = recbuf.timestamp[recbuf.iget / AUDIOBLK];

			//�������
			talkdata.DataType = 1;

			//֡���
			talkdata.Frameno = recbuf.frameno[recbuf.iget / AUDIOBLK];

			//֡��ݳ���
			talkdata.Framelen = AUDIOBLK / 2;

			//�ܰ���
			talkdata.TotalPackage = 1;

			//��ǰ��
			talkdata.CurrPackage = 1;

			//��ݳ���
			talkdata.Datalen = AUDIOBLK / 2;

			talkdata.PackLen = PACKDATALEN;

			memcpy(adpcm_out + 9, &talkdata, sizeof(talkdata));

			G711Encoder((short *) (recbuf.buffer + recbuf.iget),
					adpcm_out + DeltaLen, AUDIOBLK / 2, 1);

			if ((recbuf.iget + AUDIOBLK) >= NMAX)
				recbuf.iget = 0;
			else
				recbuf.iget += AUDIOBLK;
			recbuf.n -= AUDIOBLK;
			//UDP����
			sprintf(RemoteHost, "%d.%d.%d.%d", Remote.DenIP[0], Remote.DenIP[1],
					Remote.DenIP[2], Remote.DenIP[3]);
			//printf("%d.%d.%d.%d\n",Remote.DenIP[0], Remote.DenIP[1],Remote.DenIP[2],Remote.DenIP[3]);
			if (Remote.isDirect == 0) //ֱͨ����
				RemotePort = RemoteVideoPort;
			else
				//��ת����
				RemotePort = RemoteVideoServerPort;
			UdpSendBuff(m_VideoSocket, RemoteHost, RemotePort, adpcm_out,
					DeltaLen + AUDIOBLK / 2);
		}
		//����
		pthread_mutex_unlock(&sync_s.audio_rec_lock);
		//������Ϣ��������ݴ����߳�,������
		//  sem_post(&audioplaysem);
	}

}

//#define _RECORD_PCM_FILE

//---------------------------------------------------------------------------
//int iAudioCount = 0;
//int iSize = 0;
int recEndFlg = 0;
void audio_rec_thread_func(void) {
//	iSize = 0;
	recEndFlg = 0;
	struct timeval tv, tv1;
	uint32_t nowtime;
	int dwSize;

	int i;
	int RecPcmTotalNum;
	short recppcm[AUDIOBLK / 2];

	short nullrecppcm[AUDIOBLK / 2];
	for (i = 0; i < AUDIOBLK / 2; i++)
		nullrecppcm[i] = 0;

	if (DebugMode == 1) {
		printf("create audio recv thread \n");
		sprintf(Local.DebugInfo, "********************audio_rec_flag=%d\n",
				audio_rec_flag);
		printf(Local.DebugInfo);
	}
	gettimeofday(&tv, NULL);
	nowtime = tv.tv_sec * 1000 + tv.tv_usec / 1000;

	while (audio_rec_flag == 1) {
		recbuf.frameno[recbuf.iput / AUDIOBLK] = Local.nowaudioframeno;
		Local.nowaudioframeno++;
		if (Local.nowaudioframeno >= 65536)
			Local.nowaudioframeno = 0;
		gettimeofday(&tv, NULL);
		if ((ref_time.tv_sec == 0) && (ref_time.tv_usec == 0)) {
			ref_time.tv_sec = tv.tv_sec;
			ref_time.tv_usec = tv.tv_usec;
		}
		nowtime = (tv.tv_sec - ref_time.tv_sec) * 1000
				+ (tv.tv_usec - ref_time.tv_usec) / 1000;
		recbuf.timestamp[recbuf.iput / AUDIOBLK] = nowtime;

		//   printf("tv.tv_sec=%d, tv.tv_usec=%d\n", tv.tv_sec, tv.tv_usec);

//		dwSize = record_pcm((char *) (recbuf.buffer + recbuf.iput), AUDIOBLK, 0.4);
//		dwSize = Record((char *) (recbuf.buffer + recbuf.iput), AUDIOBLK);
//		dwSize = AudioRecord((char *) (recbuf.buffer + recbuf.iput), AUDIOBLK);

//				if(dwSize == AUDIOBLK)
//				{
////					printf("dwSize == 160\n");
//					memcpy((char *) (recbuf.buffer + recbuf.iput), tmpBuf, AUDIOBLK);
//				}
//				else
//				{
//					continue;
//				}

		//printf("after record\n");
		if (dwSize != AUDIOBLK) {
//			sprintf(Local.DebugInfo, "AUDIOBLK != dwSize  %d, %d\n", AUDIOBLK, dwSize);
//			printf(Local.DebugInfo);
			continue;
		} else {
//			if(++iAudioCount  % 10 == 0)
//			{
//				iAudioCount = 0;
//				iSize += 1;
//				printf("******************AUDIOBLK == dwSize  %d, %d, audio_rec_flag=(%d), size=%d\n", AUDIOBLK, dwSize, audio_rec_flag, iSize);
//			}
		}

		//printf("************************audio_rec_lock try,  audio_rec_flag=(%d)\n", audio_rec_flag);
		pthread_mutex_lock(&sync_s.audio_rec_lock);
		//printf("************************audio_rec_lock ok,  audio_rec_flag=(%d)\n", audio_rec_flag);

		if ((recbuf.iput + AUDIOBLK) >= NMAX)
			recbuf.iput = 0;
		else
			recbuf.iput += AUDIOBLK;
		if (recbuf.n < NMAX)
			recbuf.n += AUDIOBLK;
		else
			printf("rec.Buffer is full\n");

		//printf("************************audio_rec_lock unlock  try,  audio_rec_flag=(%d)\n", audio_rec_flag);
		pthread_mutex_unlock(&sync_s.audio_rec_lock);
		//printf("************************audio_rec_lock unlock  ok,  audio_rec_flag=(%d)\n", audio_rec_flag);
		sem_post(&audiorecsem);
		//printf("************************ sem_post  ok,  audio_rec_flag=(%d)\n", audio_rec_flag);
	}
	recEndFlg = 1;
	//LOG_RUNLOG_DEBUG("AndroidAudio recEndFlg == 1 rec function end");
}

void G711Decoder(short *pPcmBuf, char *pAudioBuf, int AudioLen, int type) {
	int i;

	for (i = 0; i < AudioLen; i++) {
		*pPcmBuf = ulaw_to_s16(*pAudioBuf);
		pPcmBuf++;
		pAudioBuf++;
	}

	return;
}

//---------------------------------------------------------------------------
void audio_play_deal_thread_func(void) {
	short playppcm[AUDIOBLK * 2];
	int i, j;
	TempAudioNode1 * tmp_audionode;
	uint32_t dellostframeno;
	uint32_t dellosttimestamp;
	if (DebugMode == 1)
		printf("create audio play deal thread \n");

	while (audio_play_flag == 1) {
		//�ȴ�ɼ��߳�����ݵ��ź�, ������
		//�ȴ�UDP�����߳�����ݵ��ź�
		sem_wait(&audiorec2playsem);
		if (audio_play_flag == 0)
			break;
		//����
		pthread_mutex_lock(&sync_s.audio_play_lock);
//    while(temp_audio_n > 0)
		if (temp_audio_n > 0) {
			//����
			//�������ϵ�֡
			tmp_audionode = search_audionode(TempAudioNode_h);
//			sprintf(Local.DebugInfo, "Content.frameno = %d, Content.timestamp = %d \n", tmp_audionode->Content.frameno, tmp_audionode->Content.timestamp);
//			printf(Local.DebugInfo);
			if (tmp_audionode != NULL) {
				playbuf.frameno[playbuf.iput / AUDIOBLK] =
						tmp_audionode->Content.frameno;
				playbuf.timestamp[playbuf.iput / AUDIOBLK] =
						tmp_audionode->Content.timestamp;
				//memcpy(playbuf.buffer + playbuf.iput, tmp_audionode->Content.buffer, AUDIOBLK);
				G711Decoder((short *) (playbuf.buffer + playbuf.iput),
						tmp_audionode->Content.buffer, AUDIOBLK / 2, 1);
				sprintf(Local.DebugInfo, "playbuf.iput = %d, playbuf.n = %d", playbuf.iput, playbuf.n);
				printf(Local.DebugInfo);

				if ((playbuf.iput + AUDIOBLK) >= NMAX)
					playbuf.iput = 0;
				else
					playbuf.iput += AUDIOBLK;
				if (playbuf.n < NMAX) {
					playbuf.n += AUDIOBLK;
				} else {
					printf("play1.Buffer is full 1\n");
				}

				sem_post(&audioplaysem);

				if (temp_audio_n > 0) {
					temp_audio_n--;
				}
				dellostframeno = tmp_audionode->Content.frameno;
				dellosttimestamp = tmp_audionode->Content.timestamp;
				delete_audionode(tmp_audionode);
			}
			//ɾ��ȫ֡
			delete_lost_audionode(TempAudioNode_h, dellostframeno,
					dellosttimestamp);
		}
		//����
		pthread_mutex_unlock(&sync_s.audio_play_lock);
	}
}
//---------------------------------------------------------------------------
int playEndFlg = 0;
void audio_play_thread_func(void) {
	playEndFlg = 0;
	short *play_pcmbuf;
	int dwSize;
	int i;
	int jump_buf; //�ѽ��뻺������֡��
	int jump_tmp; //���ջ�������֡��
	int jump_frame;
	int aframe;
	struct timeval tv;
	TempAudioNode1 * tmp_audionode;

	if (DebugMode == 1) {
		printf("create audio play thread");
		sprintf(Local.DebugInfo, "audio_play_flag=%d", audio_play_flag);
		printf(Local.DebugInfo);
	}

	aframe = AFRAMETIME;
	while (audio_play_flag == 1) {
		sem_wait(&audioplaysem);
		//printf("audio_play_thread_func \n");
		if (audio_play_flag == 0)
			break;
		//  while(playbuf.n > 0)
		if (playbuf.n > 0) {
//         printf("playbuf.timestamp[playbuf.iget/AUDIOBLK] =%d\n",playbuf.timestamp[playbuf.iget/AUDIOBLK]);
			curr_audio_timestamp = playbuf.timestamp[playbuf.iget / AUDIOBLK];

//			sprintf(Local.DebugInfo, "audio_play_thread_func: pcm_play = %d\n", pcm_play);
//								printf(Local.DebugInfo);
			//dwSize = Play((char *) (playbuf.buffer + playbuf.iget), AUDIOBLK);

			//AudioPlay((char *) (playbuf.buffer + playbuf.iget), AUDIOBLK);

			//dwSize = play_pcm(pcm_play, (char *) (playbuf.buffer + playbuf.iget), AUDIOBLK);		//modify by xuqd

			printf("play thread dwSize = %d", dwSize);
//			sprintf(Local.DebugInfo, "audio_play_thread_func: dwSize = %d\n", dwSize);
//					printf(Local.DebugInfo);
//			if (dwSize != AUDIOBLK)
//			{
//				sprintf(Local.DebugInfo, "dwSize = %d\n", dwSize);
//				printf(Local.DebugInfo);
//				continue;
//			}

			//����
			pthread_mutex_lock(&sync_s.audio_play_lock);

			dwSize = AUDIOBLK;
			if ((playbuf.iget + dwSize) >= NMAX) {
				playbuf.iget = 0;
			} else {
				playbuf.iget += dwSize;
			}
			if (playbuf.n >= dwSize) {
				playbuf.n -= dwSize;
			} else {
				printf("play2.Buffer is full\n");
			}

#if 0
			if ((TimeStamp.OldCurrAudio != TimeStamp.CurrAudio) //��һ�ε�ǰ��Ƶʱ��
			&& (TimeStamp.OldCurrAudio != 0) && (TimeStamp.CurrAudio != 0)) {
				if ((TimeStamp.CurrAudio - curr_audio_timestamp) > 32 * 4) {
					jump_frame = (TimeStamp.CurrAudio - curr_audio_timestamp
							- 40) / aframe;
					if ((playbuf.n / AUDIOBLK) >= jump_frame) {
						jump_buf = jump_frame;
						jump_tmp = 0;
					} else {
						temp_audio_n = length_audionode(TempAudioNode_h);
						jump_buf = (playbuf.n / AUDIOBLK);
						if (temp_audio_n
								> (jump_frame - (playbuf.n / AUDIOBLK)))
							jump_tmp = jump_frame - (playbuf.n / AUDIOBLK);
						else
							jump_tmp = temp_audio_n;
					}

//					sprintf(Local.DebugInfo, "audio jump_buf =%d , jump_tmp = %d, jump_frame = %d\n",
//							jump_buf, jump_tmp, jump_frame);
//					printf(Local.DebugInfo);

					for (i = 0; i < jump_buf; i++) {
						if ((playbuf.iget + AUDIOBLK) >= NMAX)
							playbuf.iget = 0;
						else
							playbuf.iget += AUDIOBLK;
						if (playbuf.n >= AUDIOBLK)
							playbuf.n -= AUDIOBLK;
					}
					for (i = 0; i < jump_tmp; i++) {
						//�������ϵ�֡
						tmp_audionode = search_audionode(TempAudioNode_h);
						if ((tmp_audionode != NULL) && (temp_audio_n > 0)) {
							delete_audionode(tmp_audionode);
							temp_audio_n--;
						}
					}
				}
			}
#endif
			//����
			pthread_mutex_unlock(&sync_s.audio_play_lock);
		}

	}
	playEndFlg = 1;

}
//---------------------------------------------------------------------------
static int iRecInitFlg = 0;
void StartRecAudio(void) {
	int i;
	pthread_attr_t attr;
	if (AudioRecIsStart == 0) {
		AudioRecIsStart = 1;

#ifdef _ECHO_CANCEL_AUDIO   //�������
		//Init_Echo_Audio(FMT8K, AUDIOBLK);
		rec_total_num = 0;
#endif

		PlayPcmTotalNum = 0; //һ��������Ƶ���ܵķ��ֵ

#if 1
		//��Ƶ
		//if (!OpenSnd(AUDIODSP))
//		if(record_open(FMT8K, 1) == -1)        //modify by xuqd
//		{
//			printf("******StartRecAudio()******Open record sound device error!\\n");
//			return;
//		}
#endif
		//SetFormat(AUDIODSP, FMT16BITS, FMT8K, 1/*STERO, WAVOUTDEV*/); //¼�� del by zlin

#if 0
//		if(iRecInitFlg == 0)
//		{
//			iRecInitFlg++;
		if(SndCardAudioRecordInit() == -1)
		{
			printf("snd card init failed.");
		}
//		}
#endif

		recbuf.iput = 0;
		recbuf.iget = 0;
		recbuf.n = 0;
		for (i = 0; i < NMAX / AUDIOBLK; i++) {
			recbuf.frameno[i] = 0;
			recbuf.timestamp[i] = 0;
		}

		sem_init(&audiorecsem, 0, 0);

		audio_rec_flag = 1;
		pthread_mutex_init(&sync_s.audio_rec_lock, NULL);

		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
		pthread_create(&audio_rec_deal_thread, &attr,
				(void *) audio_rec_deal_thread_func, NULL);
		if (audio_rec_deal_thread == 0) {
			printf("don't create audio recv deal thread \n");
			pthread_attr_destroy(&attr);
			return;
		}

		pthread_create(&audio_rec_thread, &attr, (void *) audio_rec_thread_func,
				NULL);
		pthread_attr_destroy(&attr);
		if (audio_rec_thread == 0) {
			printf("don't create audio recv thread \n");
			return;
		}
		printf("StartRecAudio finished\n");
	} else {
		if (DebugMode == 1)
			printf("repeat AudioRecIsStart\n");
	}
}
//---------------------------------------------------------------------------
void StopRecAudio(void) {
	int delaytime;
	delaytime = 40;
//	sprintf(Local.DebugInfo,"AudioRecIsStart=%d\n", AudioRecIsStart);
//	printf(Local.DebugInfo);

	if (AudioRecIsStart == 1) {
		printf("cccccccccc");
		ref_time.tv_sec = 0; //��ʼʱ���
		ref_time.tv_usec = 0;

		AudioRecIsStart = 0;
		audio_rec_flag = 0;
		usleep(delaytime * 1000);

		/*if (pthread_cancel(audio_rec_thread) == 0)
		 printf("audio_rec_thread cancel success\n");
		 else
		 printf("audio_rec_thread cancel fail\n");
		 usleep(delaytime * 1000);
		 if (pthread_cancel(audio_rec_deal_thread) == 0)
		 printf("audio_rec_deal_thread cancel success\n");
		 else
		 printf("audio_rec_deal_thread cancel fail\n");
		 usleep(delaytime * 1000);*/
#if 1
//		record_close();			//modify by xuqd
		//CloseSnd(AUDIODSP);
#endif    
		//����MIC
//    ioctl(gpio_fd, IO_PUT, 4);
//		while(!recEndFlg)
//		{
//			LOG_RUNLOG_DEBUG("AndroidAudio recEndFlg != 1 will usleep 1000");
//			usleep(1000);
//		}
		//LOG_RUNLOG_DEBUG("AndroidAudio will SndCardAudioRecordUninit");
		//sleep(5);
		//SndCardAudioRecordUninit();

		sem_destroy(&audiorecsem);
		pthread_mutex_destroy(&sync_s.audio_rec_lock);

#ifdef _ECHO_CANCEL_AUDIO   //�������
		//UnInit_Echo_Audio();
#endif
	} else {
		if (DebugMode == 1)
			printf("repeat AudioRecIsStart\n");
	}
}
//---------------------------------------------------------------------------
static int iPlayInitFlg = 0;
void StartPlayAudio(void) {
	pthread_attr_t attr;
	int i;
	printf("StartPlayAudio \n");
	if (AudioPlayIsStart == 0) {
		TimeStamp.OldCurrVideo = 0; //��һ�ε�ǰ��Ƶʱ��
		TimeStamp.CurrVideo = 0;
		TimeStamp.OldCurrAudio = 0; //��һ�ε�ǰ��Ƶʱ��
		TimeStamp.CurrAudio = 0;

		AudioPlayIsStart = 1;
		PlayPcmTotalNum = 0; //һ��������Ƶ���ܵķ��ֵ
		//��Ƶ
		//if (!OpenSnd(AUDIODSP1))
		//pcm_play = play_open(FMT8K, 1);		//modify xuqd
		sprintf(Local.DebugInfo, "StartPlayAudio: pcm_play = %d", pcm_play);
		printf(Local.DebugInfo);
		if(pcm_play == NULL) {
			printf("******StartPlayAudio()****Open play sound device error!");
			return;
		}

		//SetFormat(AUDIODSP1, FMT16BITS, FMT8K, 1); //���� STERO, WAVOUTDEV del by zlin

		playbuf.iput = 0;
		playbuf.iget = 0;
		playbuf.i_echo = 0;
		playbuf.n = 0;
		for (i = 0; i < NMAX / AUDIOBLK; i++) {
			playbuf.frameno[i] = 0;
			playbuf.timestamp[i] = 0;
		}

		temp_audio_n = 0;
		//��Ƶ���ջ�������
		delete_all_audionode(TempAudioNode_h);

		sem_init(&audioplaysem, 0, 0);
		sem_init(&audiorec2playsem, 0, 0);

		audio_play_flag = 1;
		pthread_mutex_init(&sync_s.audio_play_lock, NULL);
		pthread_mutex_init(&sync_s.audio_lock, NULL);

		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
		pthread_create(&audio_play_deal_thread, &attr,
				(void *) audio_play_deal_thread_func, NULL);
		if (audio_play_deal_thread == 0) {
			printf("don't create audio play deal thread \n");
			pthread_attr_destroy(&attr);
			return;
		}
		pthread_create(&audio_play_thread, &attr,
				(void *) audio_play_thread_func, NULL);
		pthread_attr_destroy(&attr);
		if (audio_play_thread == 0) {
			printf("don't create audio play thread \n");
			return;
		}
	} else {
		if (DebugMode == 1)
			printf("repeat AudioPlayStart \n");
	}
}

//---------------------------------------------------------------------------
void StopPlayAudio(void) {
	int delaytime;
	delaytime = 40;
	sprintf(Local.DebugInfo, "AudioPlayStart=%d\n", AudioPlayIsStart);
	printf(Local.DebugInfo);

	if (AudioPlayIsStart == 1) {
		AudioPlayIsStart = 0;
		audio_play_flag = 0;
		sem_post(&audiorec2playsem);
		usleep(delaytime * 1000);

		//		if (pthread_cancel(audio_play_deal_thread) == 0) {
		//			printf("audio_play_deal_thread cancel success\n");
		//		} else {
		//			printf("audio_play_deal_thread cancel fail\n");
		//		}

		sem_post(&audioplaysem);
		usleep(delaytime * 1000);

		//		while(!playEndFlg)
		//		{
		//			usleep(1000);
		//		}
		//SndCardAudioPlayUnnit();

		//		if (pthread_cancel(audio_play_thread) == 0) {
		//			printf("audio_play_thread cancel success\n");
		//		} else {
		//			printf("audio_play_thread cancel fail\n");
		//		}
		usleep(delaytime * 1000);
		// �ȴ�طŽ���
		//SyncPlay();
		//play_close();				//modify by xuqd
		//CloseSnd(AUDIODSP1); del by zlin
		sem_destroy(&audioplaysem);
		sem_destroy(&audiorec2playsem);
		pthread_mutex_destroy(&sync_s.audio_play_lock);
		pthread_mutex_destroy(&sync_s.audio_lock);
		temp_audio_n = 0;
		//��Ƶ���ջ�������
		delete_all_audionode(TempAudioNode_h);
	} else {
		if (DebugMode == 1) {
			printf("repeat AudioPlayStop");
		}
	}
}
//---------------------------------------------------------------------------
void StartPlayWav(char *srcname, int PlayFlag) //0 ���β���  1 ѭ������
{
	char acVolSet[128] = { 0x0 };

#if 0
	sprintf(acVolSet,"echo %d > /proc/asound/adda300_SPV",g_stIdbtCfg.iOutputPlayWavVol); //���ÿ���������С
	system(acVolSet);
	sprintf(Local.DebugInfo, "XT debug LOCK_AUDIO_OUT_VOL is :%s", acVolSet);
	printf(Local.DebugInfo);

#endif
#if 1
	pthread_attr_t attr;
	int i;
	FILE* fd;
	int bytes_read;
	//�鿴��Ƶ�豸�Ƿ����
	sprintf(Local.DebugInfo, "AudioPlayIsStart:%d\n", AudioPlayIsStart);
	printf(Local.DebugInfo);
	// ���Դ��� add by xuqd
	//AudioPlayIsStart = 1;

	if (AudioPlayIsStart == 0) {
		AudioPlayIsStart = 1;
		if ((fd = fopen(srcname, "rb")) == NULL) {
			sprintf(Local.DebugInfo, "Open Error:%s\n", srcname);
			printf(Local.DebugInfo);
			AudioPlayIsStart = 0;
			return;
		}

		bytes_read = fread(hWaveFileHeader.chRIFF,
				sizeof(hWaveFileHeader.chRIFF), 1, fd);
		bytes_read = fread(&hWaveFileHeader.dwRIFFLen,
				sizeof(hWaveFileHeader.dwRIFFLen), 1, fd);
		bytes_read = fread(hWaveFileHeader.chWAVE,
				sizeof(hWaveFileHeader.chWAVE), 1, fd);
		bytes_read = fread(hWaveFileHeader.chFMT, sizeof(hWaveFileHeader.chFMT),
				1, fd);
		bytes_read = fread(&hWaveFileHeader.dwFMTLen,
				sizeof(hWaveFileHeader.dwFMTLen), 1, fd);
		bytes_read = fread(&hWaveFileHeader.wFormatTag,
				sizeof(hWaveFileHeader.wFormatTag), 1, fd);
		bytes_read = fread(&hWaveFileHeader.nChannels,
				sizeof(hWaveFileHeader.nChannels), 1, fd);
		bytes_read = fread(&hWaveFileHeader.nSamplesPerSec,
				sizeof(hWaveFileHeader.nSamplesPerSec), 1, fd);
		bytes_read = fread(&hWaveFileHeader.nAvgBytesPerSec,
				sizeof(hWaveFileHeader.nAvgBytesPerSec), 1, fd);
		bytes_read = fread(&hWaveFileHeader.nBlockAlign,
				sizeof(hWaveFileHeader.nBlockAlign), 1, fd);
		bytes_read = fread(&hWaveFileHeader.wBitsPerSample,
				sizeof(hWaveFileHeader.wBitsPerSample), 1, fd);

		fseek(fd,
				sizeof(hWaveFileHeader.chRIFF)
						+ sizeof(hWaveFileHeader.dwRIFFLen)
						+ sizeof(hWaveFileHeader.chWAVE)
						+ sizeof(hWaveFileHeader.chFMT)
						+ sizeof(hWaveFileHeader.dwFMTLen)
						+ hWaveFileHeader.dwFMTLen, SEEK_SET);
		bytes_read = fread(hWaveFileHeader.chDATA,
				sizeof(hWaveFileHeader.chDATA), 1, fd);
		//   printf("hWaveFileHeader.chDATA=%s\n", hWaveFileHeader.chDATA);
		if ((hWaveFileHeader.chDATA[0] == 'f')
				&& (hWaveFileHeader.chDATA[1] == 'a')
				&& (hWaveFileHeader.chDATA[2] == 'c')
				&& (hWaveFileHeader.chDATA[3] == 't')) {
			bytes_read = fread(&hWaveFileHeader.dwFACTLen,
					sizeof(hWaveFileHeader.dwFACTLen), 1, fd);
			fseek(fd, hWaveFileHeader.dwFACTLen, SEEK_CUR);
			bytes_read = fread(hWaveFileHeader.chDATA,
					sizeof(hWaveFileHeader.chDATA), 1, fd);
			//    printf("hWaveFileHeader.chDATA=%s\n", hWaveFileHeader.chDATA);
		}
		if ((hWaveFileHeader.chDATA[0] == 'd')
				&& (hWaveFileHeader.chDATA[1] == 'a')
				&& (hWaveFileHeader.chDATA[2] == 't')
				&& (hWaveFileHeader.chDATA[3] == 'a')) {
			bytes_read = fread(&hWaveFileHeader.dwDATALen,
					sizeof(hWaveFileHeader.dwDATALen), 1, fd);
			//     printf("hWaveFileHeader.dwDATALen=%d\n", hWaveFileHeader.dwDATALen);
		}

		if ((hWaveFileHeader.chRIFF[0] != 'R')
				|| (hWaveFileHeader.chRIFF[1] != 'I')
				|| (hWaveFileHeader.chRIFF[2] != 'F')
				|| (hWaveFileHeader.chRIFF[3] != 'F')
				|| (hWaveFileHeader.chWAVE[0] != 'W')
				|| (hWaveFileHeader.chWAVE[1] != 'A')
				|| (hWaveFileHeader.chWAVE[2] != 'V')
				|| (hWaveFileHeader.chWAVE[3] != 'E')
				|| (hWaveFileHeader.chFMT[0] != 'f')
				|| (hWaveFileHeader.chFMT[1] != 'm')
				|| (hWaveFileHeader.chFMT[2] != 't')
				|| (hWaveFileHeader.chFMT[3] != ' ')) {
			printf("wav file format error\n");
			fclose(fd);
			AudioPlayIsStart = 0;
			return;
		}

		wavbuf.iput = 0;
		wavbuf.iget = 0;
		//�����ʱ��30��,�ڷ����ڴ�ʱ�����һ��
		wavbuf.buffer = (unsigned char *) malloc(hWaveFileHeader.dwDATALen);
		bytes_read = fread(wavbuf.buffer, hWaveFileHeader.dwDATALen, 1, fd);
		if (bytes_read != 1) {
			sprintf(Local.DebugInfo, "Read Error:%s\n", srcname);
			printf(Local.DebugInfo);
			if (wavbuf.buffer != NULL) {
				free(wavbuf.buffer);
				wavbuf.buffer = NULL;
			}
			fclose(fd);
			AudioPlayIsStart = 0;
			return;
		}
		wavbuf.n = hWaveFileHeader.dwDATALen;
		fclose(fd);
#if 1
		//   printf("wavbuf.n = %d\n",wavbuf.n);
		//��Ƶ
		if (!OpenSnd(AUDIODSP1)) {
			printf("******StartPlayWav()****Open play sound device error!\n");
			AudioPlayIsStart = 0; //one key open door no sound
			return;
		}
		//printf("hWaveFileHeader.wBitsPerSample=%d\n", hWaveFileHeader.wBitsPerSample);
		//printf("hWaveFileHeader.nSamplesPerSec=%d\n", hWaveFileHeader.nSamplesPerSec);
		//printf("hWaveFileHeader.nChannels=%d\n", hWaveFileHeader.nChannels);

		SetFormat(AUDIODSP1, hWaveFileHeader.wBitsPerSample,
				hWaveFileHeader.nSamplesPerSec, hWaveFileHeader.nChannels); //����
		//  SetFormat(AUDIODSP1, FMT16BITS, FMT8K, 1);    //����   STERO, WAVOUTDEV

		audio_play_wav_flag = 1;
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
		pthread_create(&audio_play_wav_thread, &attr,
				(void *) audio_play_wav_thread_func, (void *) PlayFlag);
		pthread_attr_destroy(&attr);
		if (audio_play_wav_thread == 0) {
			printf("can't create sound play thread!!!!!\n");
			AudioPlayIsStart = 0;
			return;
		}
#else
		if(wavbuf.buffer != NULL)
		{
			printf("free wavbuf.buffer\n");
			free(wavbuf.buffer);
			wavbuf.buffer = NULL;
		}
		AudioPlayIsStart = 0;
#endif      
	} else {
		printf("sound card is busy now!!!\n");
		return;
	}
#endif
}
//---------------------------------------------------------------------------
void StopPlayWavFile(void) {
	char acVolSet[128] = { 0x0 };
	int delaytime;
	delaytime = 40;
	audio_play_wav_flag = 0;
#if 0
	sprintf(acVolSet,"echo %d > /proc/asound/adda300_SPV",g_stIdbtCfg.iOutputCallVol); //�ָ�ԭ������ֵ
	system(acVolSet);
	sprintf(Local.DebugInfo, "XT debug LOCK AUDIO g_iOutputVol is :%s", acVolSet);
	printf(Local.DebugInfo);
#endif
}
//---------------------------------------------------------------------------
void audio_play_wav_thread_func(int PlayFlag) {
	int arg; // ����ioctl���õĲ���
	int status; // ϵͳ���õķ���ֵ
	struct timeval tv;

	int dwSize;
	int i;
	int AudioLen;

	if (DebugMode == 1)
		printf("audio play wav thread begin to run!!!!!!\n");
#if 1
	while (audio_play_wav_flag == 1) {
		if ((wavbuf.iget + AUDIOPLAYBLK) < wavbuf.n)
			AudioLen = AUDIOPLAYBLK;
		else
			AudioLen = wavbuf.n - wavbuf.iget;
		dwSize = Play((char *) (wavbuf.buffer + wavbuf.iget), AudioLen);

		if (dwSize != AUDIOPLAYBLK) {
			sprintf(Local.DebugInfo,
					"audio_play_wav_thread_func::dwSize = %d, AUDIOPLAYBLK = %d\n",
					dwSize, AUDIOPLAYBLK);
			printf(Local.DebugInfo);
		}
		wavbuf.iget += AudioLen;

		//if(wavbuf.iget >= wavbuf.n)
		if ((wavbuf.iget + AUDIOPLAYBLK) >= wavbuf.n) {
			if (PlayFlag == 0) //���β���
					{
				WaitAudioUnuse(1000);
				StopPlayWavFile();
			} else //ѭ������
			{
				wavbuf.iget = 0;
			}
		}
	}
#endif   

	//CloseSnd(AUDIODSP1);del by zlin
	// �ȴ�طŽ���
	//printf("wavbuf.n = %d, wavbuf.iget = %d\n", wavbuf.n, wavbuf.iget);
	gettimeofday(&tv, NULL);
	//printf("1 tv.tv_sec=%d, tv.tv_usec=%d\n",
	//         tv.tv_sec, tv.tv_usec);
	SyncPlay();
	//gettimeofday(&tv, NULL);
	//printf("2 tv.tv_sec=%d, tv.tv_usec=%d\n",
	//         tv.tv_sec, tv.tv_usec);
	if (wavbuf.buffer != NULL) {
		//printf("free wavbuf.buffer\n");
		free(wavbuf.buffer);
		wavbuf.buffer = NULL;
	}

	if (DebugMode == 1)
		printf("audio play wav thread exit now!!!!!!\n");
	AudioPlayIsStart = 0;
}
//---------------------------------------------------------------------------
TempAudioNode1 * init_audionode(void) //��ʼ��������ĺ���
{
	TempAudioNode1 *h; // *h�����ͷ����ָ�룬*pָ��ǰ����ǰһ����㣬*sָ��ǰ���
	if ((h = (TempAudioNode1 *) malloc(sizeof(TempAudioNode1))) == NULL) //����ռ䲢���
			{
		printf("don't audio malloc!");
		return NULL;
	}
	h->llink = NULL; //������
	h->rlink = NULL; //������
	return (h);
}
//---------------------------------------------------------------------------
int creat_audionode(TempAudioNode1 *h, struct talkdata1 talkdata,
		unsigned char *r_buf, int r_length) {
	TempAudioNode1 *t;
	TempAudioNode1 *p;
	int j;
	int DataOk;
//    uint32_t newframeno;
//    int currpackage;
	t = h;
	//  t=h->next;
	while (t->rlink != NULL) //ѭ����ֱ��tָ���
		t = t->rlink; //tָ����һ���
	DataOk = 1;
	if ((talkdata.DataType < 1) || (talkdata.DataType > 5))
		DataOk = 0;
	if (talkdata.Framelen > VIDEOMAX)
		DataOk = 0;
	if (talkdata.CurrPackage > talkdata.TotalPackage)
		DataOk = 0;
	if (talkdata.CurrPackage <= 0)
		DataOk = 0;
	if (talkdata.TotalPackage <= 0)
		DataOk = 0;
	if (t && (DataOk == 1)) {
		//β�巨��������
		if ((p = (TempAudioNode1 *) malloc(sizeof(TempAudioNode1))) == NULL) //����½��s���������ڴ�ռ�
				{
			printf("don't malloc memory!\n");
			return 0;
		}
		if ((p->Content.buffer = (unsigned char *) malloc(talkdata.Framelen))
				== NULL) {
			printf("don't malloc audio receive memory!\n");
			return 0;
		}
		p->Content.isFull = 0;
		for (j = 0; j < MAXPACKNUM; j++)
			p->Content.CurrPackage[j] = 0;

		p->Content.frame_flag = talkdata.DataType;
		//  newframeno = (r_buf[63] << 8) + r_buf[64];
		//  currpackage = (r_buf[67] << 8) + r_buf[68];
		p->Content.frameno = talkdata.Frameno;
		p->Content.TotalPackage = talkdata.TotalPackage;
		p->Content.timestamp = talkdata.timestamp;
		p->Content.CurrPackage[talkdata.CurrPackage - 1] = 1;

		if (talkdata.CurrPackage == p->Content.TotalPackage)
			p->Content.Len = (talkdata.CurrPackage - 1) * talkdata.PackLen
					+ talkdata.Datalen;
		memcpy(
				p->Content.buffer
						+ (talkdata.CurrPackage - 1) * talkdata.PackLen,
				r_buf + DeltaLen, talkdata.Datalen);

//      if(talkdata.CurrPackage == p->Content.TotalPackage)
//        p->Content.Len =  (talkdata.CurrPackage - 1) * talkdata.PackLen + r_length - DeltaLen;
//      memcpy(p->Content.buffer + (talkdata.CurrPackage - 1) * talkdata.PackLen,
//             r_buf + DeltaLen, r_length - DeltaLen);

		p->Content.isFull = 1;
		for (j = 0; j < p->Content.TotalPackage; j++)
			if (p->Content.CurrPackage[j] == 0) {
				p->Content.isFull = 0;
				break;
			}
		p->rlink = NULL; //p��ָ����Ϊ��
		p->llink = t;
		t->rlink = p; //p��nextָ��������
		t = p; //tָ��������
		return p->Content.isFull;
	}
}
//---------------------------------------------------------------------------
//������ƣ�length
//���������������?��
//�������ͣ��޷���ֵ
//�������h:������ͷָ��
int length_audionode(TempAudioNode1 *h) {
	TempAudioNode1 *p;
	int i = 0; //��¼���?��
	p = h->rlink;
	while (p != NULL) //ѭ����ֱ��pָ���
	{
		i = i + 1;
		p = p->rlink; //pָ����һ���
	}
	return i;
	//    printf(" %d",i); //���p��ָ�ӵ�������
}
//---------------------------------------------------------------------------
//������ƣ�delete_
//����������ɾ����
//�������ͣ�����
//�������h:������ͷָ�� i:Ҫɾ���λ��
int delete_audionode(TempAudioNode1 *p) {
	if (p != NULL) {
		//��Ϊ���һ�����
		if (p->rlink != NULL) {
			(p->rlink)->llink = p->llink;
			(p->llink)->rlink = p->rlink;
			if (p->Content.buffer)
				free(p->Content.buffer);
			if (p)
				free(p);
		} else {
			(p->llink)->rlink = p->rlink;
			if (p->Content.buffer)
				free(p->Content.buffer);
			if (p)
				free(p);
		}
		return (1);
	} else
		printf("audio delete null\n");
	return (0);
}
//---------------------------------------------------------------------------
int delete_all_audionode(TempAudioNode1 *h) {
	TempAudioNode1 *p, *q;
	p = h->rlink; //��ʱpΪ�׽��
	while (p != NULL) //�ҵ�Ҫɾ�����λ��
	{
		//��Ϊ���һ�����
		q = p;
		if (p->rlink != NULL) {
			(p->rlink)->llink = p->llink;
			(p->llink)->rlink = p->rlink;
		} else
			(p->llink)->rlink = p->rlink;
		p = p->rlink;
		if (q->Content.buffer)
			free(q->Content.buffer);
		if (q)
			free(q);
	}
}
//---------------------------------------------------------------------------
int delete_lost_audionode(TempAudioNode1 *h, uint32_t currframeno,
		uint32_t currtimestamp) //ɾ��ȫ֡
{
	TempAudioNode1 *p, *q;
	p = h->rlink; //��ʱpΪ�׽��
	while (p != NULL) //�ҵ�Ҫɾ�����λ��
	{
		//��Ϊ���һ�����
		q = p;
		if (p->rlink != NULL) {
			//      if(p->Content.frameno < currframeno) //����ѭ����ֱ��pΪ�գ����ҵ�x
			if (p->Content.timestamp < currtimestamp) {
				(p->rlink)->llink = p->llink;
				(p->llink)->rlink = p->rlink;
				p = p->llink;
				if (q->Content.buffer)
					free(q->Content.buffer);
				if (q)
					free(q);
				if (temp_audio_n > 0)
					temp_audio_n--;
			}
		} else {
			//      if(p->Content.frameno < currframeno) //����ѭ����ֱ��pΪ�գ����ҵ�x
			if (p->Content.timestamp < currtimestamp) {
				(p->llink)->rlink = p->rlink;
				p = p->llink;
				if (q->Content.buffer)
					free(q->Content.buffer);
				if (q)
					free(q);
				if (temp_audio_n > 0)
					temp_audio_n--;
			}
		}
		p = p->rlink;
	}
	return 1;

}
//---------------------------------------------------------------------------
//������ƣ�find_
//�������������Һ���
//�������ͣ�����
//�������h:������ͷָ�� x:Ҫ���ҵ�ֵ
//���Ҹ�֡�ð��Ƿ��Ѵ���
TempAudioNode1 * find_audionode(TempAudioNode1 *h, int currframeno,
		int currpackage) {
	TempAudioNode1 *p;
	int PackIsExist; //��ݰ��ѽ��ձ�־
	int FrameIsNew; //��ݰ��Ƿ�����֡�Ŀ�ʼ
	p = h->rlink; //��ʱpΪ�׽��
	PackIsExist = 0;
	FrameIsNew = 1;
	while (p != NULL) {
		if (p->Content.frameno == currframeno) //����ѭ����ֱ��pΪ�գ����ҵ�x
				{
			FrameIsNew = 0;
			if (p->Content.CurrPackage[currpackage - 1] == 1) {
				if (DebugMode == 1) {
					sprintf(Local.DebugInfo, "pack exist %d\n", currframeno);
					printf(Local.DebugInfo);
				}
				PackIsExist = 1;
			}
			break;
		}
		p = p->rlink; //sָ��p����һ���
	}
	if (p != NULL)
		return p;
	else
		return NULL;
}
//---------------------------------------------------------------------------
//������ƣ�find_
//�������������Һ���
//�������ͣ�����
//�������h:������ͷָ�� x:Ҫ���ҵ�ֵ
//�������ϵ�֡
TempAudioNode1 * search_audionode(TempAudioNode1 *h) {
	TempAudioNode1 *p;
	TempAudioNode1 *tem_p;

	p = h->rlink; //��ʱpΪ�׽��
	tem_p = p;

	while (p != NULL) {
		if (p->Content.isFull == 1)
			//   if(p->Content.frameno < tem_p->Content.frameno) //����ѭ����ֱ��pΪ�գ����ҵ�x
			if (p->Content.timestamp < tem_p->Content.timestamp) //����ѭ����ֱ��pΪ�գ����ҵ�x
					{
				tem_p = p;
			}
		p = p->rlink; //sָ��p����һ���
	}

	return tem_p;

}
//---------------------------------------------------------------------------
int free_audionode(TempAudioNode1 *h) {
	TempAudioNode1 *p, *t;
	int i = 0; //��¼���?��
	p = h->rlink;
	while (p != NULL) //ѭ����ֱ��pָ���
	{
		i = i + 1;
		t = p;
		p = p->rlink; //pָ����һ���
		free(t);
	}
	return i;
}
//-------------------------------------------------------------------------
void WaitAudioUnuse(int MaxTime) //�ȴ������
{
	//�ȴ������
	int DelayT = 0;
	while (AudioPlayIsStart == 1) {
		usleep(1000);
		DelayT++;
		if (DelayT > (MaxTime / 10))
			break;
	}
//  printf("DelayT = %d\n", DelayT);
}
//---------------------------------------------------------------------------
#if 0
void Send_Audio_Data_Func(const char * pcm_data)
{
	struct timeval tv;
	uint32_t nowtime;
	int dwSize;
	int i;

	char RemoteHost[20];
	unsigned char adpcm_out[3072];
	struct talkdata1 talkdata;
	int c_timestamp;
	int c_frameno;
	int ref_time = 0;

	gettimeofday(&tv, NULL);
	nowtime = tv.tv_sec *1000 + tv.tv_usec/1000;

	//����
	pthread_mutex_lock(&sync_s.audio_rec_lock);
	i = recbuf.iput/AUDIOBLK;
	c_frameno = Local.nowaudioframeno;
	Local.nowaudioframeno++;
	if(Local.nowaudioframeno >= 65536)
	Local.nowaudioframeno = 1;

	//ʱ���
	gettimeofday(&tv, NULL);
	nowtime = tv.tv_sec *1000 + tv.tv_usec/1000;
	if (ref_time ==0) {
		ref_time = nowtime;
	}
	c_timestamp = nowtime - ref_time;

	//ͷ��
	memcpy(adpcm_out, UdpPackageHead, 6);
	//����
	adpcm_out[6] = VIDEOTALK;
	adpcm_out[7] = 1;
	//������
	if ((Local.Status == 0) || (Local.Status == 1)||(Local.Status == 3)||(Local.Status == 5)) { //����Ϊ���з�
		adpcm_out[8] = CALLUP;
		memcpy(talkdata.HostAddr, LocalCfg.Addr, 20);
		memcpy(talkdata.HostIP, LocalCfg.IP, 4);
		memcpy(talkdata.AssiAddr, Remote.Addr[0], 20);
		memcpy(talkdata.AssiIP, Remote.IP[0], 4);
		if(Remote.Addr[0][0] == 'M' || Remote.Addr[0][0] == 'W' || Remote.Addr[0][0] == 'S')
		talkdata.HostAddr[0] = 's';
	}
	if ((Local.Status == 2)||(Local.Status == 4)||(Local.Status == 6)) { //����Ϊ���з�
		adpcm_out[8] = CALLDOWN;
		memcpy(talkdata.HostAddr, Remote.Addr[0], 20);
		memcpy(talkdata.HostIP, Remote.IP[0], 4);
		memcpy(talkdata.AssiAddr, LocalCfg.Addr, 20);
		memcpy(talkdata.AssiIP, LocalCfg.IP, 4);

		if(Remote.Addr[0][0] == 'M' || Remote.Addr[0][0] == 'W' || Remote.Addr[0][0] == 'S')
		talkdata.AssiAddr[0] = 's';
	}
	if ((recbuf.iget + AUDIOBLK) > NMAX) {
		printf("recbuf.iget overflow\n");
	}
	//ʱ���
	talkdata.timestamp = c_timestamp;
	//�������
	talkdata.DataType = 1;
	//֡���
	talkdata.Frameno = c_frameno;
	//֡��ݳ���
	talkdata.Framelen = AUDIOBLK/2;
	//�ܰ���
	talkdata.TotalPackage = 1;
	//��ǰ��
	talkdata.CurrPackage = 1;
	//��ݳ���
	talkdata.Datalen = AUDIOBLK/2;
	talkdata.PackLen = PACKDATALEN;
	memcpy(adpcm_out + 9, &talkdata, sizeof(talkdata));
	G711Encoder((short *)pcm_data, (unsigned char *)(adpcm_out + DeltaLen), AUDIOBLK/2, 1);
	if ((recbuf.iget + AUDIOBLK) >= NMAX) {
		recbuf.iget = 0;
	} else {
		recbuf.iget += AUDIOBLK;
	}
	recbuf.n -= AUDIOBLK;

	Remote.DenIP[0] = 192;
	Remote.DenIP[1] = 168;
	Remote.DenIP[2] = 10;
	Remote.DenIP[3] = 84;
	//UDP����
	sprintf(RemoteHost, "%d.%d.%d.%d",Remote.DenIP[0],
			Remote.DenIP[1],Remote.DenIP[2],Remote.DenIP[3]);

	UdpSendBuff(m_VideoSocket, RemoteHost, adpcm_out, DeltaLen + AUDIOBLK/2);
	//printf("[rec]A----recsem =%d\n",audiorecsem);
	//����
	pthread_mutex_unlock(&sync_s.audio_rec_lock);
}
#endif

void Send_Audio_Data_Func(const char * pcm_data, int nSize) {
	struct timeval tv;
	uint32_t nowtime;
	int dwSize;
	int i;

	char RemoteHost[20];
	unsigned char adpcm_out[1024];
	struct talkdata1 talkdata;
	int c_timestamp;
	int c_frameno;
	int ref_time = 0;

	printf("nSize = %d\n", nSize);
	gettimeofday(&tv, NULL);
	nowtime = tv.tv_sec * 1000 + tv.tv_usec / 1000;
	//����
	pthread_mutex_lock(&sync_s.audio_rec_lock);
	c_frameno = Local.nowaudioframeno;
	Local.nowaudioframeno++;
	if (Local.nowaudioframeno >= 65536) {
		Local.nowaudioframeno = 1;
	}
	//ʱ���
	gettimeofday(&tv, NULL);
	nowtime = tv.tv_sec * 1000 + tv.tv_usec / 1000;
	c_timestamp = nowtime - ref_time;

	//ͷ��
	memcpy(adpcm_out, UdpPackageHead, 6);
	//����
	adpcm_out[6] = VIDEOTALK;
	adpcm_out[7] = 1;
	//������
	if ((Local.Status == 0) || (Local.Status == 1) || (Local.Status == 3)
			|| (Local.Status == 5)) { //����Ϊ���з�
		adpcm_out[8] = CALLUP;
		memcpy(talkdata.HostAddr, LocalCfg.Addr, 20);
		memcpy(talkdata.HostIP, LocalCfg.IP, 4);
		memcpy(talkdata.AssiAddr, Remote.Addr[0], 20);
		memcpy(talkdata.AssiIP, Remote.IP[0], 4);
		if (Remote.Addr[0][0] == 'M' || Remote.Addr[0][0] == 'W'
				|| Remote.Addr[0][0] == 'S')
			talkdata.HostAddr[0] = 's';
	}
	if ((Local.Status == 2) || (Local.Status == 4) || (Local.Status == 6)) { //����Ϊ���з�
		adpcm_out[8] = CALLDOWN;
		memcpy(talkdata.HostAddr, Remote.Addr[0], 20);
		memcpy(talkdata.HostIP, Remote.IP[0], 4);
		memcpy(talkdata.AssiAddr, LocalCfg.Addr, 20);
		memcpy(talkdata.AssiIP, LocalCfg.IP, 4);

		if (Remote.Addr[0][0] == 'M' || Remote.Addr[0][0] == 'W'
				|| Remote.Addr[0][0] == 'S')
			talkdata.AssiAddr[0] = 's';
	}
	//ʱ���
	talkdata.timestamp = c_timestamp;
	//�������
	talkdata.DataType = 1;
	//֡���
	talkdata.Frameno = c_frameno;
	//֡��ݳ���
	talkdata.Framelen = nSize;
	//�ܰ���
	talkdata.TotalPackage = 1;
	//��ǰ��
	talkdata.CurrPackage = 1;
	//��ݳ���
	talkdata.Datalen = nSize;
	talkdata.PackLen = PACKDATALEN;

	memcpy(adpcm_out + 9, &talkdata, sizeof(talkdata));
	//DeltaLen += sizeof(talkdata);
	memcpy((unsigned char *) (adpcm_out + sizeof(talkdata)), pcm_data, nSize);

	Remote.DenIP[0] = 192;
	Remote.DenIP[1] = 168;
	Remote.DenIP[2] = 10;
	Remote.DenIP[3] = 84;
	//UDP����
	sprintf(RemoteHost, "%d.%d.%d.%d", Remote.DenIP[0], Remote.DenIP[1],
			Remote.DenIP[2], Remote.DenIP[3]);

	printf("RemoteHost = %s\n", RemoteHost);
	UdpSendBuff(m_VideoSocket, RemoteHost, RemoteVideoPort, adpcm_out,
			(sizeof(talkdata) + nSize));
	//printf("[rec]A----recsem =%d\n",audiorecsem);
	//����
	pthread_mutex_unlock(&sync_s.audio_rec_lock);
}

