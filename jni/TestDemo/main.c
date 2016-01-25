#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>       //sem_t

#include "../include/libqsa_callback.h"

extern void default_cb_audio_data1(void * data, int length, int type);
extern void default_cb_info1(const void * data, const void * ip, int length, int port);
extern int default_cb_status1(int type);
extern void default_cb_devip1(const void * address, const void * ip, int uflag);
extern void default_cb_opt1(int value);
extern int InitUdpSocketDemo(short lPort);

int g_Status = 0;
unsigned char g_addr[30];
unsigned char g_ip[20];

int main() {

    struct dev_config config;
    struct _cb_function p;
	int ch;
	int programrun;//程序运行标志
    uint32_t Ip_Int;
    int uFlag = 0;

    memset(&config, 0x00, sizeof(struct dev_config));
    strcpy(config.address, "M00010100000000000000");
    Ip_Int = inet_addr("192.168.10.97");
    memcpy(config.ip, &Ip_Int, 4);
    init_param_task(&config);

    p.cb_audio_data = &default_cb_audio_data1;
    p.cb_info = NULL;
    p.cb_curr_st = &default_cb_status1;
    p.cb_curr_opt = &default_cb_opt1;
    p.cb_devip = &default_cb_devip1;

    set_cb_function(&p);

    g_Status = CB_ST_NULL;
    strcpy(g_addr, "0606");
    //strcpy(g_ip, "192.168.11.188");

    usleep(100*1000);
    programrun = 1;
	ch = ' ';
    printf("\n\n");
    printf("Please select: 'Q' is exit!\n");
    printf("***** ***** ***** ***** ***** ***** ***** ***** ***** *****\n");
    printf("\nplease input operate_code:  ");

	do {
        // 以无限回圈不断等待键盘操作
        ch = getchar();
        if (ch == 0x0A) {
            /* The Input Key isn't Enter Key */
            printf("\nplease input operate_code:  ");
            continue;
        }

        if ((ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'z')) {
            switch (ch) {
                case 'A':
                    {
                        find_ip(g_addr, uFlag);
                    }
                    break;
                case 'B':
                    {
                        start_call(g_ip, g_addr, 2);
                    }
                    break;
                case 'C':
                    {
                        stop_talk();
                    }
                    break;
                case 'D':
                    {
                    }
                    break;
                case 'E':
                    break;
                case 'F':
                    break;
                case 'G':
                    break;
                case 'H':
                    break;
                case 'I':
                    break;
                case 'J':
                    break;
                case 'K':
                    break;
                case 'L':
                    break;
                case 'M':
                    break;
                case 'N':
                    break;
                case 'O':
                    break;
                case 'P':
                    break;
                case 'Q':                     // 判断是否[q]键被按下
                    {
                        programrun = 0;     //退出程序
                    }
                    break;
                case 'S':
                    break;
                case 'T':
                    break;
                case 'U':
                    break;
                case 'V':
                    break;
                case 'W':
                    break;
                case 'X':
                    break;
                case 'Y':
                    break;
                case 'Z':
                    break;
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                    break;
                case 'a': //0
                case 'b': //1
                case 'c': //2
                case 'd': //3
                case 'e': //4
                case 'f': //5
                case 'g': //6
                case 'h': //7
                case 'i': //8
                case 'j': //9
                case 'k': //10
                case 'l': //11
                case 'm': //12
                case 'n': //13
                case 'o': //14
                case 'p':  //15
                case 'q':
                case 'r':
                case 's':
                case 't':
                case 'u':
                case 'v':
                case 'w':
                case 'x':
                case 'y':
                case 'z':
                    break;
                default:
                    break;
            }
        }
    } while (programrun);


    /*
    start_call(ip, addr, 2);
    stop_talk();
    */

    uninit_task();

    exit(0);
}



