//============================================
//Test   SL030
//PIC C source code
//PIC16F887(8MHz) + PICC 9.70
//============================================
#include <htc.h>
#include <pic16f887.h>
#include <string.h>

__CONFIG(DEBUGEN&LVPDIS&FCMEN&IESOEN&BOREN&UNPROTECT&DUNPROTECT&MCLREN&PWRTEN&WDTDIS&INTIO);

//============================================
//  PIN define
//============================================
#define Buzzer RB2
#define CARDIN_SL030 RD2
#define WAKEUP_SL030 RC2

//============================================
//  Card type define
//============================================
#define CARDTYPE_S50 0
#define CARDTYPE_S70 1
#define CARDTYPE_ProX 2
#define CARDTYPE_Pro 3
#define CARDTYPE_UL 4
#define CARDTYPE_DES 5
#define CARDTYPE_TAG 6
#define CARDTYPE_ICODE 7

//============================================
//  IIC slave address define
//============================================
#define SLVADD 0xa0

//============================================
//  global variable define
//============================================
bank1 unsigned int g_iTimer;
bank1 unsigned char g_cTimer_Buzzer;
unsigned char g_bOverTime;
bank1 unsigned char g_cRxBuf[40];
bank1 unsigned char g_cCardType;

//============================================
//  Command List, preamble + length + command 
//============================================
const unsigned char ComSelectCard[]={1,1};
const unsigned char ComLoginSector0[]={9,2,0+2,0xAA,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
const unsigned char ComReadBlock1[]={2,3,1+8};
const unsigned char ComWriteBlock1[]={
  18,4,1+8,0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xAA,0xBB,0xCC,0xDD,0xEE,0xFF};
const unsigned char ComIntiPurse1[]={6,6,1+8,0x78,0x56,0x34,0x12};
const unsigned char ComReadPurse1[]={2,5,1+8};
const unsigned char ComIncrPurse1[]={6,8,1+8,0x02,0x00,0x00,0x00};
const unsigned char ComDecrPurse1[]={6,9,1+8,0x01,0x00,0x00,0x00};
const unsigned char ComCopyValue[]={3,0x0A,1+8,2+8};              
const unsigned char ComReadUltralightPage5[]={2,0x10,0x05};
const unsigned char ComWriteUltralightPage5[]={6,0x11,0x05,0x12,0x34,0x56,0x78};
const unsigned char ComHalt[]= {1,0x50};

//============================================
//  procedure define
//============================================
void Init_Hardware(void);
void BuzzerOn(void);
void Start_Time(unsigned int ms);
void Test_SL030(void);

//============================================
//  initialize system hardware config
//  timer,iic,port
//============================================
void Init_Hardware(void)
{	
  OSCCON=0x71;		//8M
  while(HTS==0);		
  ANSEL=0x00;
  ANSELH=0x00;  
  TRISB2=0;
  TRISC2=0;
  OPTION=0x07;
  TMR0=0xb1;
  T0IE=1;
  Buzzer=0;
  TXSTA=0x04;
  RCSTA=0x90;
  BAUDCTL=0x08;
  SPBRG=0x10;
  SPBRGH=0;
  RCIE=1;
  TXEN=1;
  TRISC3=1;
  TRISC4=1;
  SSPADD=0x12;
  SSPSTAT=0x80;
  SSPCON=0x38;
  PEIE=1;
  GIE=1;
}

//============================================
//  interrupt service
//  timer interrupt
//  uart rx interrupt
//============================================
void interrupt isr(void)
{
  unsigned char dat;
  unsigned char CheckSum;
  unsigned char counter;
  if(T0IF)
  {
    T0IF=0;
    TMR0=0xb1;
    if(g_iTimer!=0)
    {
      g_iTimer--;
      if(g_iTimer==0) g_bOverTime=1;
    }
    if(g_cTimer_Buzzer!=0)
    {
      g_cTimer_Buzzer--;
      if(g_cTimer_Buzzer==0) Buzzer=0;
    }	
  }
}

//============================================
//  trun on buzzer about 100mS
//============================================
void BuzzerOn(void)
{
  Buzzer=1;
  g_cTimer_Buzzer=10;
}

//============================================
//  start timer
//============================================
void Start_Time(unsigned int ms)
{
  g_iTimer=ms;
  g_bOverTime=0;
}

void I2C_Start(void)
{
  SEN=1;
  while(!SSPIF);
  SSPIF=0;
}

void I2C_Stop(void)
{
  PEN=1;
  while(!SSPIF);
  SSPIF=0;
}

void WaitAck(void)
{
  unsigned char counter;
  counter=0xff;
  while((!SSPIF)&&(--counter!=0));
  SSPIF=0;
}

void I2C_Ack(unsigned char ack)
{
  if(ack==0) ACKDT=0;else ACKDT=1;
  ACKEN=1;
  while(!SSPIF);
  SSPIF=0;
}

void I2C_WrByte(unsigned char dat)
{
  SSPBUF=dat;
  WaitAck();
}

unsigned char I2C_RdByte(void)
{
  unsigned char result;	
  RCEN=1;
  while(!SSPIF);
  result=SSPBUF;
  while(!SSPIF);
  SSPIF=0;
  return result;
}

void SendBuf_I2C(const unsigned char *dat,unsigned char len)
{
  unsigned char counter;
  I2C_Start();
  I2C_WrByte(SLVADD);
  for(counter=0;counter<len;counter++) I2C_WrByte(dat[counter]);
  I2C_Stop();
}

void ReadBuf_I2C(unsigned char *dst,unsigned char BufSize)
{
  unsigned char counter;
  I2C_Start();
  I2C_WrByte(SLVADD|1);
  dst[0]=I2C_RdByte();
  I2C_Ack(0);
  if(dst[0]>=BufSize-1) dst[0]=BufSize-2;
  for(counter=1;counter<dst[0];counter++) 
  {
    dst[counter]=I2C_RdByte();
    I2C_Ack(0);
  }
  dst[counter]=I2C_RdByte();
  I2C_Ack(1);
  I2C_Stop();
}

void Test_SL030(void)
{	
  #define RCVCMD_SL030 g_cRxBuf[1]
  #define RCVSTA_SL030 g_cRxBuf[2]
  #define DELAYTIME 5
  unsigned long int lPurseValue;
  g_cCardType=0xff;
  if(CARDIN_SL030!=0) return;
  SendBuf_I2C(ComSelectCard,sizeof(ComSelectCard));
  Start_Time(DELAYTIME);
  while(g_bOverTime==0);
  ReadBuf_I2C(g_cRxBuf,sizeof(g_cRxBuf));
  if((RCVCMD_SL030!=0x01)||(RCVSTA_SL030!=0)) return;
  if(g_cRxBuf[g_cRxBuf[0]]==1) g_cCardType=CARDTYPE_S50;        //Mifare 1k 4 byte UID
  else if(g_cRxBuf[g_cRxBuf[0]]==2) g_cCardType=CARDTYPE_S50;   //Mifare 1k 7 byte UID
  else if(g_cRxBuf[g_cRxBuf[0]]==3) g_cCardType=CARDTYPE_UL;    //Ultralight 7 byte UID
  else if(g_cRxBuf[g_cRxBuf[0]]==4) g_cCardType=CARDTYPE_S70;   //Mifare 4k 4 byte UID
  else if(g_cRxBuf[g_cRxBuf[0]]==5) g_cCardType=CARDTYPE_S70;   //Mifare 4k 7 byte UID
  else if(g_cRxBuf[g_cRxBuf[0]]==6) g_cCardType=CARDTYPE_DES;   //DesFire 7 byte UID
  switch(g_cCardType)
  {
    case CARDTYPE_S50:
    case CARDTYPE_S70:
      SendBuf_I2C(ComLoginSector0,sizeof(ComLoginSector0));
      Start_Time(DELAYTIME);
      while(g_bOverTime==0);
      ReadBuf_I2C(g_cRxBuf,sizeof(g_cRxBuf));
      if((RCVCMD_SL030!=0x02)||(RCVSTA_SL030!=0x02)) return;
      SendBuf_I2C((ComWriteBlock1),sizeof(ComWriteBlock1));
      Start_Time(DELAYTIME);
      while(g_bOverTime==0);
      ReadBuf_I2C(g_cRxBuf,sizeof(g_cRxBuf));
      if((RCVCMD_SL030!=0x04)||(RCVSTA_SL030!=0x00)) return;
      SendBuf_I2C((ComReadBlock1),sizeof(ComReadBlock1));
      Start_Time(DELAYTIME);
      while(g_bOverTime==0);
      ReadBuf_I2C(g_cRxBuf,sizeof(g_cRxBuf));
      if((RCVCMD_SL030!=0x03)||(RCVSTA_SL030!=0x00)) return;
      if(memcmp(&ComWriteBlock1[3],&g_cRxBuf[3],16)!=0) return;
      SendBuf_I2C((ComIntiPurse1),sizeof(ComIntiPurse1));
      Start_Time(DELAYTIME);
      while(g_bOverTime==0);
      ReadBuf_I2C(g_cRxBuf,sizeof(g_cRxBuf));
      if((RCVCMD_SL030!=0x06)||(RCVSTA_SL030!=0x00)) return;
      SendBuf_I2C((ComIncrPurse1),sizeof(ComIncrPurse1));
      Start_Time(DELAYTIME);
      while(g_bOverTime==0);
      ReadBuf_I2C(g_cRxBuf,sizeof(g_cRxBuf));
      if((RCVCMD_SL030!=0x08)||(RCVSTA_SL030!=0x00)) return;
      SendBuf_I2C((ComDecrPurse1),sizeof(ComDecrPurse1));
      Start_Time(DELAYTIME);
      while(g_bOverTime==0);
      ReadBuf_I2C(g_cRxBuf,sizeof(g_cRxBuf));
      if((RCVCMD_SL030!=0x09)||(RCVSTA_SL030!=0x00)) return;
      SendBuf_I2C((ComReadPurse1),sizeof(ComReadPurse1));
      Start_Time(DELAYTIME);
      while(g_bOverTime==0);
      ReadBuf_I2C(g_cRxBuf,sizeof(g_cRxBuf));
      if((RCVCMD_SL030!=0x05)||(RCVSTA_SL030!=0x00)) return;
      //Check value	 
      lPurseValue=g_cRxBuf[6];
      lPurseValue=(lPurseValue<<8)+g_cRxBuf[5];
      lPurseValue=(lPurseValue<<8)+g_cRxBuf[4];
      lPurseValue=(lPurseValue<<8)+g_cRxBuf[3];
      if(lPurseValue!=0x12345678+0x00000002-0x00000001) return;
      SendBuf_I2C((ComCopyValue),sizeof(ComCopyValue));
      Start_Time(DELAYTIME);
      while(g_bOverTime==0);
      ReadBuf_I2C(g_cRxBuf,sizeof(g_cRxBuf));
      if((RCVCMD_SL030!=0x0a)||(RCVSTA_SL030!=0x00)) return;
      SendBuf_I2C((ComHalt),sizeof(ComHalt));
      Start_Time(DELAYTIME);
      while(g_bOverTime==0);
      WAKEUP_SL030=0;
      Start_Time(DELAYTIME);
      while(g_bOverTime==0);
      WAKEUP_SL030=1;
      Start_Time(DELAYTIME);
      while(g_bOverTime==0);
      BuzzerOn();
      break;
    case CARDTYPE_UL:
      SendBuf_I2C((ComWriteUltralightPage5),sizeof(ComWriteUltralightPage5));
      Start_Time(DELAYTIME);
      while(g_bOverTime==0);
      ReadBuf_I2C(g_cRxBuf,sizeof(g_cRxBuf));
      if((RCVCMD_SL030!=0x11)||(RCVSTA_SL030!=0x00)) return;
      SendBuf_I2C((ComReadUltralightPage5),sizeof(ComReadUltralightPage5));
      Start_Time(DELAYTIME);
      while(g_bOverTime==0);
      ReadBuf_I2C(g_cRxBuf,sizeof(g_cRxBuf));
      if((RCVCMD_SL030!=0x10)||(RCVSTA_SL030!=0x00)) return;
      if(memcmp(&ComWriteUltralightPage5[3],&g_cRxBuf[3],4)!=0) return;
      SendBuf_I2C((ComHalt),sizeof(ComHalt));
      Start_Time(DELAYTIME);
      while(g_bOverTime==0);
      WAKEUP_SL030=0;
      Start_Time(DELAYTIME);
      while(g_bOverTime==0);
      WAKEUP_SL030=1;
      Start_Time(DELAYTIME);
      while(g_bOverTime==0);
      BuzzerOn();
      break;
    default:
      break;
  }
}

void main(void)
{
  Init_Hardware();
  g_iTimer=0;
  while(1) Test_SL030();
}

