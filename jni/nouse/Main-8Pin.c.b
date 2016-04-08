#include "mb95200.h"
#include "CRAM.h"
#include "PRAM.h"
#define LOW 0 
#define HIGH 1
#define FALSE 1
#define TRUE 0
#define SDA PDR0_P06  /* port 06 used for SDA */
#define SCL PDR0_P05  /* port 05 used for SCL */

void I2C_Init(void);                        /* initilizes the IO-lines            */
void I2C_Start(void);                       /* Start condition                    */
void I2C_Stop(void);                        /* Stop condition                     */
void I2C_Write_Byte(unsigned char data);    /* writes the byte "data" to a device */
unsigned char I2C_Read_Byte(void);          /* read a byte from a device          */
unsigned char I2C_Read_Ack(void);           /* read an acknoledge from a device   */
unsigned char I2C_Read_Device(unsigned char MemAddr, unsigned char DevSlaveAddr);    
void I2C_Write_Device(unsigned char data); 
void Wait(long int waitcnt);                     /* dummy wait function                */

const char str0[]={0x30,0xC0,0xA6};//a6:10 10 011 0
const char str1[]={0x30,0xC1,0x20};//20:00100000
const char str2[]={0x30,0xC2,0xC6};//c6:11000110  /*0xE6,AK7742 Slave; 0xC6,AK7742 Master*/
const char str3[]={0x30,0xC3,0xCA};//ca:11001010
const char str4[]={0x30,0xC4,0x88};//88:10001000
const char str5[]={0x30,0xC5,0x18};//18:00011000
const char str6[]={0x30,0xC6,0x00};//
const char str7[]={0x30,0xC7,0x00};
const char str8[]={0x30,0xC8,0x80};//80:10000000
void main(void)
{  unsigned char I2C_memaddr;  
   int cnt;
   InitIrqLevels();   
   __set_il(7);
   __EI(); 
   SYCC = 0x00; 
   Wait(1000);            
   DDR1_P12 = 1;     /*Bluetooth reset*/
   DDR0_P04 = 1;       /*RESET OUTPUT*/
   PDR1_P12 = 0;
   PDR0_P04 = 0;
   Wait(25);      /*~25ms*/
   PDR1_P12 = 1;
   
   Wait(1200);

   PDR0_P04 = 1;
 
   Wait(80);
   
   I2C_Init();        /* initialise Port3 for I2C  */
   Wait(1);
   I2C_Start();
   for(cnt=0; cnt<sizeof(strpram); cnt++)
   {
       I2C_Write_Device(strpram[cnt]);
   }
   I2C_Stop();
   
   I2C_Start();
   for(cnt=0; cnt<sizeof(strcram); cnt++)
   {
       I2C_Write_Device(strcram[cnt]);
   }
   I2C_Stop();
   
   I2C_Start();
   for(cnt=0; cnt<sizeof(str0); cnt++) 
   {
       I2C_Write_Device( str0[cnt]);
   }
    I2C_Stop();  
   
   I2C_Start();
   for(cnt=0; cnt<sizeof(str1); cnt++) 
   {
       I2C_Write_Device( str1[cnt]);
   }
    I2C_Stop();          /* end of transfer */
    
   I2C_Start();
   for(cnt=0; cnt<sizeof(str2); cnt++) 
   {
       I2C_Write_Device(str2[cnt]);
   }
    I2C_Stop();          /* end of transfer */
    
   I2C_Start();
   for(cnt=0; cnt<sizeof(str3); cnt++) 
   {
       I2C_Write_Device( str3[cnt]);
   }
   I2C_Stop();          /* end of transfer */
    
   I2C_Start();
   for(cnt=0; cnt<sizeof(str4); cnt++) 
   {
       I2C_Write_Device(str4[cnt]);
   }
    I2C_Stop();          /* end of transfer */
    
   I2C_Start();
   for(cnt=0; cnt<sizeof(str5); cnt++) 
   {
       I2C_Write_Device(str5[cnt]);
   }
    I2C_Stop();          /* end of transfer */
    
   I2C_Start();
   for(cnt=0; cnt<sizeof(str6); cnt++) 
   {
       I2C_Write_Device(str6[cnt]);
   }
    I2C_Stop();          /* end of transfer */
    
   I2C_Start();
   for(cnt=0; cnt<sizeof(str7); cnt++) 
   {
       I2C_Write_Device(str7[cnt]);
   }
    I2C_Stop();          /* end of transfer */
    
   I2C_Start();
   for(cnt=0; cnt<sizeof(str8); cnt++) 
   {
       I2C_Write_Device(str8[cnt]);
   }
    I2C_Stop();          /* end of transfer */
   
   
  /* I2C_Start();
   for(cnt=0; cnt<sizeof(str9); cnt++)
   {
       I2C_Write_Device(str9[cnt]);
   }
   I2C_Stop();
   for(cnt=0; cnt<strlen(str); cnt++)
   {
       *buf = I2C_Read_Device(I2C_memaddr, DeviceAddress);
       buf++;
       I2C_memaddr++;
   }*/

   while(1){;}

}
void I2C_Init(void)
  { DDR0 |= 0x60; 
    SDA = HIGH;     /* set SDA High */
    SCL = HIGH;     /* set SCL High */
  }
void I2C_Start(void)
  {
  DDR0 |= 0x60;   /* set P34, P35 to output */
  SDA = LOW;      /* SCL high, SDA low */
  SCL = HIGH;
  Wait(1);
  SDA = LOW;      /* SCL low, SDA low */
  SCL = LOW;
  }

void I2C_Stop(void)
  {
  DDR0 |= 0x60; /* set P05, P06 to output */
  
  SDA = LOW;    /* SCL low, SDA low */
  SCL = LOW;
  
  SDA = LOW;    /* SCL high, SDA low */
  SCL = HIGH;
  
  SCL = HIGH;   /* SCL high, SDA high */
  SDA = HIGH;
  
  DDR0 &= 0x9F; /* set SDA, SCL to tristate (switch to input) */
   
  }

void I2C_Write_Byte(unsigned char data)
  {   unsigned char i; 
      DDR0 |= 0x60;               /* set P05, P06 to output */
      SCL = LOW;
    
      for (i=0; i<8; i++)
          {
              if (data & 0x80)
                 {
                    SDA = HIGH;   /* change data */
                 }
              else
                 {
                    SDA = LOW;    /* change data */
                 }
              SCL = HIGH;     /* set SCL high 此时数据发送出去*/
              SCL = LOW;      /* set SCL low */
              data <<= 1;     /* shift to next bit */

          }
     SDA = HIGH;
     DDR0 &= 0xBF;     /* set port06 (SDA) to input to allow I2C device to give acknowledge */
   
  }
unsigned char I2C_Read_Byte(void)
  {
  unsigned char read_byte = 0;
  unsigned char i;
  
  DDR0 &= 0xBF;  
  DDR0 |= 0x20;  
  SCL = LOW;      
  SCL = HIGH;     
  

  for (i=0; i<8; i++)
    {
    read_byte = read_byte << 1;
    SCL = HIGH;        
    read_byte |= SDA;  
    SCL = LOW;        
    }
    
  SDA = HIGH;    
  SCL = HIGH; 
  
  return (read_byte);
  }

unsigned char I2C_Read_Ack(void)
  {
  DDR0 &= 0xBF;  
  DDR0 |= 0x20;  
  SCL = LOW;     
  SCL = HIGH;    
  if (SDA==1)
    {
    SCL = HIGH;   
  
    return FALSE;
   
    }
  else
    {
    SCL = LOW;   
    return TRUE;
    }
  }
void I2C_Write_Device (unsigned char data)
  { 
    I2C_Write_Byte(data);
    if (I2C_Read_Ack()==1)
    {
    I2C_Stop();        /*Error*/
    return ;
    }
 
  }
unsigned char I2C_Read_Device (unsigned char MemAddr, unsigned char DevSlaveAddr)
  {
  unsigned char read_data;
  PDR0_P04 =1;
  Wait(1000);
  I2C_Start();                           /* start I2C transfer     */
  I2C_Write_Byte(DevSlaveAddr & 0xFE);   /* transfer slave address */
  if (I2C_Read_Ack()==1)
    {
    I2C_Stop();                /* Error */
    return FALSE;
    }
  I2C_Write_Byte(MemAddr);     /* transfer address */
  if (I2C_Read_Ack()==1)
    {
    I2C_Stop;                  /* Error */
    return FALSE;
    }
  
  I2C_Start();                 /* start I2C again */
  
  I2C_Write_Byte(DevSlaveAddr | 0x01); /* transfer slave address and define read cycle */
  if (I2C_Read_Ack()==1)
    {
    I2C_Stop();                /* Error */
    return FALSE;
    }
  read_data = I2C_Read_Byte(); /* read the byte   */
  I2C_Stop();                  /* end of transfer */
  return read_data;
  }

void Wait(long int waitcnt) 
{
 long int loopcnt;
 int i;
 for(i=0;i<10;i++)
  { 
     for( loopcnt=0; loopcnt< waitcnt; loopcnt++); 
  }
}



