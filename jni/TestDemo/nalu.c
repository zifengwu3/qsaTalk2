// NAL.c : Defines the entry point for the console application.   
//   

#include <stdio.h>   
#include <stdlib.h>   
#include <string.h>   

//#include "stdafx.h"

typedef struct   
{   
    int startcodeprefix_len;      //! 4 for parameter sets and first slice in picture, 3 for everything else (suggested)   
    unsigned len;                 //! Length of the NAL unit (Excluding the start code, which does not belong to the NALU)   
    unsigned max_size;            //! Nal Unit Buffer size   
    int forbidden_bit;            //! should be always FALSE   
    int nal_reference_idc;        //! NALU_PRIORITY_xxxx   
    int nal_unit_type;            //! NALU_TYPE_xxxx       
    char *buf;                    //! contains the first byte followed by the EBSP   
    unsigned short lost_packets;  //! true, if packet loss is detected   
} NALU_t;   

FILE *bits = NULL;                //!< the bit stream file   
FILE *bits1 = NULL;                //!< the bit stream file   
static int FindStartCode2 (unsigned char *Buf);//查找开始字符0x000001   
static int FindStartCode3 (unsigned char *Buf);//查找开始字符0x00000001   
//static bool flag = true;   
static int info2=0, info3=0;   
extern unsigned char g_ip[20];
int send_video_flag = 0;

char framebuffer[1024*2000];

NALU_t *AllocNALU(int buffersize)   
{   
    NALU_t *n;   

    if ((n = (NALU_t*)calloc (1, sizeof (NALU_t))) == NULL)   
    {   
        printf("AllocNALU: n");   
        exit(0);   
    }   

    n->max_size=buffersize;   

    if ((n->buf = (char*)calloc (buffersize, sizeof (char))) == NULL)   
    {   
        free (n);   
        printf ("AllocNALU: n->buf");   
        exit(0);   
    }   

    return n;   
}   

void FreeNALU(NALU_t *n)   
{   
    if (n)   
    {   
        if (n->buf)   
        {   
            free(n->buf);   
            n->buf=NULL;   
        }   
        free (n);   
    }   
}   

void OpenBitstreamFile (char *fn)   
{   
    if (NULL == (bits=fopen(fn, "rb")))   
    {   
        printf("open file error\n");   
        exit(0);   
    }   
}   

void OpenBitstreamFile1 (char *fn)   
{   
    if (NULL == (bits1 = fopen(fn, "rb")))   
    {   
        printf("open file error\n");   
        exit(0);   
    }   
}

int GetAnnexbNALU (NALU_t *nalu)   
{   
    int pos = 0;   
    int StartCodeFound, rewind;   
    unsigned char *Buf;   

    if ((Buf = (unsigned char*)calloc (nalu->max_size , sizeof(char))) == NULL)    
        printf ("GetAnnexbNALU: Could not allocate Buf memory\n");   

    nalu->startcodeprefix_len=3;//初始化码流序列的开始字符为3个字节   

    if (3 != fread (Buf, 1, 3, bits))//从码流中读3个字节   
    {   
        free(Buf);   
        return 0;   
    }   
    info2 = FindStartCode2 (Buf);//判断是否为0x000001    
    if(info2 != 1) {//如果不是，再读一个字节   
        if(1 != fread(Buf+3, 1, 1, bits))//读一个字节   
        {   
            free(Buf);   
            return 0;   
        }   
        info3 = FindStartCode3 (Buf);//判断是否为0x00000001   
        if (info3 != 1)//如果不是，返回-1   
        {    
            free(Buf);   
            return -1;   
        }   
        else {//如果是0x00000001,得到开始字符为4个字节   
            pos = 4;   
            nalu->startcodeprefix_len = 4;   
        }   
    } else {
        //如果是0x000001,得到开始字符为3个字节   
        nalu->startcodeprefix_len = 3;   
        pos = 3;   
    }   

    StartCodeFound = 0;//查找下一个开始字符的标志位   
    info2 = 0;   
    info3 = 0;   

    while (!StartCodeFound)   
    {   
        if (feof (bits))//判断是否到了文件尾   
        {   
            nalu->len = (pos-1)-nalu->startcodeprefix_len;   
            memcpy (nalu->buf, &Buf[nalu->startcodeprefix_len], nalu->len);        
            nalu->forbidden_bit = (nalu->buf[0]>>7) & 1;   
            nalu->nal_reference_idc = (nalu->buf[0]>>5) & 3;   
            nalu->nal_unit_type = (nalu->buf[0]) & 0x1f;   
            free(Buf);   
            return pos-1;   
        }   
        Buf[pos++] = fgetc (bits);//读一个字节到BUF中   
        info3 = FindStartCode3(&Buf[pos-4]);//判断是否为0x00000001   
        if(info3 != 1)   
            info2 = FindStartCode2(&Buf[pos-3]);//判断是否为0x000001   
        StartCodeFound = (info2 == 1 || info3 == 1);   
    }   
    //flag = false;   


    // Here, we have found another start code (and read length of startcode bytes more than we should   
    // have.  Hence, go back in the file   
    rewind = (info3 == 1)? -4 : -3;   

    if (0 != fseek (bits, rewind, SEEK_CUR))//把文件指针向后退开始字节的字节数   
    {   
        free(Buf);   
        printf("GetAnnexbNALU: Cannot fseek in the bit stream file");   
    }   

    // Here the Start code, the complete NALU, and the next start code is in the Buf.     
    // The size of Buf is pos, pos+rewind are the number of bytes excluding the next   
    // start code, and (pos+rewind)-startcodeprefix_len is the size of the NALU   

    nalu->len = (pos+rewind)-nalu->startcodeprefix_len;   
    memcpy (nalu->buf, &Buf[nalu->startcodeprefix_len], nalu->len);   
    nalu->forbidden_bit = nalu->buf[0] & 0x80; //(nalu->buf[0]>>7) & 1;   
    nalu->nal_reference_idc = nalu->buf[0] & 0x60; //(nalu->buf[0]>>5) & 3;   
    nalu->nal_unit_type = (nalu->buf[0]) & 0x1f;   
    free(Buf);   

    return (pos+rewind);//返回两个开始字符之间间隔的字节数   
}   

void dump(NALU_t *n)   
{   
    if (!n) return;   
    printf("a new nal:");   
    printf(" len: %d  ", n->len);   
    printf(" nal_unit_type: %x  ", n->nal_unit_type);   
    printf("\n");
}   

int user_main(int argc, char* argv[])   
{   
#if 1
    OpenBitstreamFile("TestDemo/1.h264");   
    OpenBitstreamFile1("TestDemo/2.h264");   
#else
    OpenBitstreamFile("TestDemo/test3.h264");   
    OpenBitstreamFile1("TestDemo/test4.h264");   
#endif
    unsigned int ts_current = 0;
    static int ts_length = 0;
    int w_length = 0 ;
    int count = 0;
    int i_frame = 0;
    int sendflag = 0;
    NALU_t *n;   
    n = AllocNALU(0x8000000);   

    memset(framebuffer, 0, 1024*2000);//清空buffer；

    while(!feof(bits)) {   
        if (send_video_flag == 1) {
            GetAnnexbNALU(n);//每执行一次，文件的指针指向本次找到的NALU的末尾，下一个位置即为下个NALU的起始码0x000001
            dump(n);//输出NALU长度和TYPE

            ts_length += n->startcodeprefix_len;
            ts_length += n->len;

            if (((n->nal_unit_type) == 5) || ((n->nal_unit_type) == 1)) {
#if 0
                if ((n->buf[1]&0x80) != 0x80) {
                    //判断是否为同一片数据，如果＆0x80等于0x80，下一片再出现0x80数据之前的数据为同一片数据
                    continue;
                } else {
                    sendflag = 1;
                }
#endif
                sendflag = 1;
            }

            if (sendflag == 1) {
                printf("framebuffer length = %d\n", ts_length);
                w_length = 0;
                if (!feof(bits1)) {
                    count++;
                    w_length = fread(framebuffer, 1, ts_length, bits1);
                }
                printf("w_length = %d, count = %d\n", w_length, count);
            }

            if (w_length != 0) {
                qsa_send_video(framebuffer, w_length, count, n->nal_unit_type, g_ip);
                usleep(60*1000);
                ts_length = 0;
                sendflag = 0;
            }
        }   
    }

    FreeNALU(n);   
    return 0;   
}   

static int FindStartCode2 (unsigned char *Buf)   
{   
    if(Buf[0]!=0 || Buf[1]!=0 || Buf[2] !=1) return 0; //判断是否为0x000001,如果是返回1   
    else return 1;   
}   

static int FindStartCode3 (unsigned char *Buf)   
{   
    if(Buf[0]!=0 || Buf[1]!=0 || Buf[2] !=0 || Buf[3] !=1) return 0;//判断是否为0x00000001,如果是返回1   
    else return 1;   
} 
