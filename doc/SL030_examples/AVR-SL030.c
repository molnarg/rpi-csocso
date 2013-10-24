////////////////////////////////////////////////////////////
//Test SL030
//AVR C source code
//ATmega16(7.3728MHz) + EWAVR 5.20
////////////////////////////////////////////////////////////   
#include <ioavr.h>
#include <intrinsics.h>
#include <avr_macros.h>
#include <iomacro.h>
#include <string.h>

//============================================
//  Pin define
//============================================
#define CARDIN_SL030 PINC_Bit6
#define WAKEUP_SL030 PORTD_Bit7

//============================================
//  i2c define
//============================================
#define SLVADD            0xa0
#define I2CSTA_START      0x08
#define I2CSTA_MT_SLA_ACK 0x18
#define I2CSTA_MT_DAT_ACK 0x28

#define I2C_START (TWCR=(1<<TWINT)|(1<<TWSTA)|(1<<TWEN))
#define I2C_BUSY ((TWCR & (1 << TWINT))==0)
#define I2C_TX (TWCR=(1<<TWINT)|(1<<TWEN))
#define I2C_RX_ACK (TWCR=(1<<TWINT)|(1<<TWEN)|(1<<TWEA))
#define I2C_RX_NACK (TWCR=(1<<TWINT)|(1<<TWEN))
#define I2C_STOP (TWCR=(1<<TWINT)|(1<<TWEN)|(1<<TWSTO))

//============================================
//  global variable define
//============================================
unsigned char g_cCardType;
unsigned long int g_iTimer;
unsigned char g_bOverTime;
unsigned char g_cI2CRxBuf[60];

//============================================
//  Command List, preamble + length + command 
//============================================
const unsigned char ComSelectCard[]           ={1,1};
const unsigned char ComLoginSector0[]         ={9,2,0+2,0xAA,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
const unsigned char ComReadBlock1[]           ={2,3,1+8};
const unsigned char ComWriteBlock1[]          ={18,4,1+8,0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xAA,0xBB,0xCC,0xDD,0xEE,0xFF};
const unsigned char ComIntiPurse1[]           ={6,6,1+8,0x78,0x56,0x34,0x12};
const unsigned char ComReadPurse1[]           ={2,5,1+8};
const unsigned char ComIncrPurse1[]           ={6,8,1+8,0x02,0x00,0x00,0x00};
const unsigned char ComDecrPurse1[]           ={6,9,1+8,0x01,0x00,0x00,0x00};
const unsigned char ComCopyValue[]            ={3,0x0A,1+8,2+8};              
const unsigned char ComReadUltralightPage5[]  ={2,0x10,0x05};
const unsigned char ComWriteUltralightPage5[] ={6,0x11,0x05,0x12,0x34,0x56,0x78};
const unsigned char ComHalt[]                 ={1,0x50};

#define DELAYTIME 5

//============================================
//  procedure define
//============================================
void Init_Hardware();
void Start_Time(unsigned long int ms);
void Stop_Time();
void SendBuf_I2C(unsigned char * dat, unsigned char len);
unsigned char ReadBuf_I2C(unsigned char * dst, unsigned char BufSize);
void Test_SL030();

void main()
{ 
  Init_Hardware();
  g_iTimer=0;
  while(1) Test_SL030();
}

//============================================
//  initialize system hardware config
//  timer,port
//============================================
void Init_Hardware()
{
  __disable_interrupt();
  DDRA=0x00;PORTA=0xff;
  DDRB=0x00;PORTB=0xff;
  DDRC=0x80;PORTC=0x7f;
  DDRD=0xe0;PORTD=0xdf;
  TCCR0=0;
  TCNT0=184;
  TIMSK_TOIE0=1;
  TCCR0=0x05;
  __enable_interrupt();
}

//============================================
//  start timer
//============================================
void Start_Time(unsigned long int ms)
{ 
  g_iTimer=ms;
  g_bOverTime=0;
}

//============================================
//  stop timer
//============================================
void Stop_Time()
{ 
  g_iTimer=0;
}

void SendBuf_I2C(unsigned char *dat,unsigned char len)
{
  unsigned char counter;
  TWBR=2;
  I2C_START;
  while(I2C_BUSY);
  TWDR=SLVADD;
  I2C_TX;
  while(I2C_BUSY);
  for(counter=0;counter<len;counter++)
  {
    TWDR=dat[counter];
    I2C_TX;
    while(I2C_BUSY);
  }
  I2C_STOP;
}

unsigned char ReadBuf_I2C(unsigned char *dst,unsigned char BufSize)
{	
  unsigned char counter;
  TWBR=2;
  I2C_START;
  while(I2C_BUSY);
  TWDR=SLVADD|1;
  I2C_TX;
  while(I2C_BUSY);
  I2C_RX_ACK;
  while(I2C_BUSY);
  dst[0]=TWDR;
  if(dst[0]>=BufSize-1) return 0;
  for(counter=1;counter<dst[0];counter++)
  {	
    I2C_RX_ACK;
    while(I2C_BUSY);
    dst[counter]=TWDR;
  }
  I2C_RX_NACK;
  while(I2C_BUSY);
  dst[counter]=TWDR;
  I2C_STOP;
  return 1;
}

//============================================
//  timer0 overflow interrupt
//  10ms
//============================================
#pragma vector = TIMER0_OVF_vect
__interrupt void Timer0_ISR(void)
{
  TCNT0=184;
  if(g_iTimer!=0)
  {
    g_iTimer--;
    if(g_iTimer==0) g_bOverTime=1;
  }
}

void Test_SL030()
{	
  #define RCVCMD_SL030 g_cI2CRxBuf[1]
  #define RCVSTA_SL030 g_cI2CRxBuf[2]
  unsigned long int lPurseValue;
  g_cCardType=0xff;
  
  if(CARDIN_SL030!=0) return;
  
  SendBuf_I2C((unsigned char *)(ComSelectCard),sizeof(ComSelectCard));
  Start_Time(DELAYTIME);
  while(g_bOverTime==0);
  Stop_Time();
  if(ReadBuf_I2C(g_cI2CRxBuf,sizeof(g_cI2CRxBuf))==0) return;
  if((RCVCMD_SL030!=0x01)||(RCVSTA_SL030!=0)) return;
  g_cCardType=g_cI2CRxBuf[g_cI2CRxBuf[0]];
  
  switch(g_cCardType)
  {	
    case 1:                     //Mifare 1k 4 byte UID
    case 2:                     //Mifare 1k 7 byte UID
    case 4:                     //Mifare 4k 4 byte UID
    case 5:                     //Mifare 4k 7 byte UID
      SendBuf_I2C((unsigned char *)(ComLoginSector0),sizeof(ComLoginSector0));
      Start_Time(DELAYTIME);
      while(g_bOverTime==0);
      Stop_Time();
      if(ReadBuf_I2C(g_cI2CRxBuf,sizeof(g_cI2CRxBuf))==0) return;
      if((RCVCMD_SL030!=0x02)||(RCVSTA_SL030!=0x02)) return;
      
      SendBuf_I2C((unsigned char *)(ComWriteBlock1),sizeof(ComWriteBlock1));
      Start_Time(DELAYTIME);
      while(g_bOverTime==0);
      Stop_Time();
      if(ReadBuf_I2C(g_cI2CRxBuf,sizeof(g_cI2CRxBuf))==0) return;
      if((RCVCMD_SL030!=0x04)||(RCVSTA_SL030!=0x00)) return;

      SendBuf_I2C((unsigned char *)(ComReadBlock1),sizeof(ComReadBlock1));
      Start_Time(DELAYTIME);
      while(g_bOverTime==0);
      Stop_Time();
      if(ReadBuf_I2C(g_cI2CRxBuf,sizeof(g_cI2CRxBuf))==0) return;
      if((RCVCMD_SL030!=0x03)||(RCVSTA_SL030!=0x00)) return;
      if(memcmp(&ComWriteBlock1[3],&g_cI2CRxBuf[3],16)!=0) return;

      SendBuf_I2C((unsigned char *)(ComIntiPurse1),sizeof(ComIntiPurse1));
      Start_Time(DELAYTIME);
      while(g_bOverTime==0);
      Stop_Time();
      if(ReadBuf_I2C(g_cI2CRxBuf,sizeof(g_cI2CRxBuf))==0) return;
      if((RCVCMD_SL030!=0x06)||(RCVSTA_SL030!=0x00)) return;

      SendBuf_I2C((unsigned char *)(ComIncrPurse1),sizeof(ComIncrPurse1));
      Start_Time(DELAYTIME);
      while(g_bOverTime==0);
      Stop_Time();
      if(ReadBuf_I2C(g_cI2CRxBuf,sizeof(g_cI2CRxBuf))==0) return;
      if((RCVCMD_SL030!=0x08)||(RCVSTA_SL030!=0x00)) return;

      SendBuf_I2C((unsigned char *)(ComDecrPurse1),sizeof(ComDecrPurse1));
      Start_Time(DELAYTIME);
      while(g_bOverTime==0);
      Stop_Time();
      if(ReadBuf_I2C(g_cI2CRxBuf,sizeof(g_cI2CRxBuf))==0) return;
      if((RCVCMD_SL030!=0x09)||(RCVSTA_SL030!=0x00)) return;

      SendBuf_I2C((unsigned char *)(ComReadPurse1),sizeof(ComReadPurse1));
      Start_Time(DELAYTIME);
      while(g_bOverTime==0);
      Stop_Time();
      if(ReadBuf_I2C(g_cI2CRxBuf,sizeof(g_cI2CRxBuf))==0) return;
      if((RCVCMD_SL030!=0x05)||(RCVSTA_SL030!=0x00)) return;

      //Check value	 
      lPurseValue=g_cI2CRxBuf[6];
      lPurseValue=(lPurseValue<<8)+g_cI2CRxBuf[5];
      lPurseValue=(lPurseValue<<8)+g_cI2CRxBuf[4];
      lPurseValue=(lPurseValue<<8)+g_cI2CRxBuf[3];
      if(lPurseValue!=0x12345678+0x00000002-0x00000001) return;

      SendBuf_I2C((unsigned char *)(ComCopyValue),sizeof(ComCopyValue));
      Start_Time(DELAYTIME);
      while(g_bOverTime==0);
      Stop_Time();
      if(ReadBuf_I2C(g_cI2CRxBuf,sizeof(g_cI2CRxBuf))==0) return;
      if((RCVCMD_SL030!=0x0a)||(RCVSTA_SL030!=0x00)) return;

      SendBuf_I2C((unsigned char *)(ComHalt),sizeof(ComHalt));
      Start_Time(DELAYTIME);
      while(g_bOverTime==0);
      Stop_Time();

      WAKEUP_SL030=0;
      Start_Time(1);
      while(g_bOverTime==0);
      Stop_Time();
      WAKEUP_SL030=1;
      Start_Time(10);
      while(g_bOverTime==0);
      Stop_Time();
      break;
    

    case 3:    //Mifare Ultralight
      SendBuf_I2C((unsigned char *)(ComWriteUltralightPage5),sizeof(ComWriteUltralightPage5));
      Start_Time(DELAYTIME);
      while(g_bOverTime==0);
      Stop_Time();
      if(ReadBuf_I2C(g_cI2CRxBuf,sizeof(g_cI2CRxBuf))==0) return;
      if((RCVCMD_SL030!=0x11)||(RCVSTA_SL030!=0x00)) return;

      SendBuf_I2C((unsigned char *)(ComReadUltralightPage5),sizeof(ComReadUltralightPage5));
      Start_Time(DELAYTIME);
      while(g_bOverTime==0);
      Stop_Time();
      if(ReadBuf_I2C(g_cI2CRxBuf,sizeof(g_cI2CRxBuf))==0) return;
      if((RCVCMD_SL030!=0x10)||(RCVSTA_SL030!=0x00)) return;
      if(memcmp(&ComWriteUltralightPage5[3],&g_cI2CRxBuf[3],4)!=0) return;

      SendBuf_I2C((unsigned char *)(ComHalt),sizeof(ComHalt));
      Start_Time(DELAYTIME);
      while(g_bOverTime==0);
      Stop_Time();

      WAKEUP_SL030=0;
      Start_Time(1);
      while(g_bOverTime==0);
      Stop_Time();
      WAKEUP_SL030=1;
      Start_Time(10);
      while(g_bOverTime==0);
      Stop_Time();
      break;
    

    default:
      break;
  }
}

