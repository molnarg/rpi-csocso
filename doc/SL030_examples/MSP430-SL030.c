//============================================
//    Test   SL030
//MSP430 C source code
//MSP430F2232 + IAR
//============================================
#include <msp430x22x2.h>
#include <string.h>

//============================================
//  PIN define
//============================================
#define BuzzerON (P4OUT |=BIT2)
#define BuzzerOFF (P4OUT &= ~BIT2)
#define CARDIN (P3IN&BIT3)
#define WAKEUP_H (P3OUT |=BIT0)
#define WAKEUP_L (P3OUT &= ~BIT0)

//============================================
//  IIC status words define
//============================================
#define I2CSTATUS_RXLEN	0
#define I2CSTATUS_RXDAT	1
#define I2CSTATUS_RXSUCC 2

//============================================
//  IIC slave address define
//============================================
#define SLA 0x50

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
//  global variable define
//============================================
unsigned char g_cTimer_Buzzer;
unsigned char g_cCardType;
unsigned long int g_iTimer;
unsigned char g_bOverTime;
unsigned char g_cModel;
unsigned char g_cRxBuf[40];
unsigned long int lPurseValue;
unsigned char g_cI2CTxCnt;
unsigned char *g_pI2CTxDat;
unsigned char g_cI2CStatus;
unsigned char g_cI2CRxCnt;
unsigned char *g_pI2CRxDat;
unsigned char g_cI2CRxLen;
unsigned char g_cI2CBufSize;

const unsigned char IIC_ComSelectCard[]            ={1,1};
const unsigned char IIC_ComLoginSector0[]          ={9,2,0+2,0xAA,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
const unsigned char IIC_ComReadBlock1[]            ={2,3,1+8};
const unsigned char IIC_ComWriteBlock1[]           ={18,4,1+8,0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xAA,0xBB,0xCC,0xDD,0xEE,0xFF};
const unsigned char IIC_ComIntiPurse1[]            ={6,6,1+8,0x78,0x56,0x34,0x12};
const unsigned char IIC_ComReadPurse1[]            ={2,5,1+8};
const unsigned char IIC_ComIncrPurse1[]            ={6,8,1+8,0x02,0x00,0x00,0x00};
const unsigned char IIC_ComDecrPurse1[]            ={6,9,1+8,0x01,0x00,0x00,0x00};
const unsigned char IIC_ComCopyValue[]             ={3,0x0A,1+8,2+8};              
const unsigned char IIC_ComReadUltralightPage5[]   ={2,0x10,0x05};
const unsigned char IIC_ComWriteUltralightPage5[]  ={6,0x11,0x05,0x12,0x34,0x56,0x78};
const unsigned char IIC_ComHalt[]                  ={1,0x50};
const unsigned char IIC_ComRedLedOn[]              ={2,0x40,1};
const unsigned char IIC_ComRedLedOff[]             ={2,0x40,0};

//============================================
//  procedure define
//============================================
void Init_Hardware(void);
void BuzzerOn(void);
void Start_Time(unsigned int ms);
void SendBuf_I2C(const unsigned char *dat,unsigned char len);
void ReadBuf_I2C(unsigned char *dst,unsigned char BufSize);

//============================================
//  Timer interrupt service
//============================================
#pragma vector=TIMERA0_VECTOR
__interrupt void Timer_A(void)
{
  if(g_cTimer_Buzzer!=0)
  {
    g_cTimer_Buzzer--;
    if(g_cTimer_Buzzer==0)  BuzzerOFF;
  }
  if(g_iTimer!=0)
  {
    g_iTimer--;
    if(g_iTimer==0) g_bOverTime=1;
  }
}

#pragma vector = USCIAB0TX_VECTOR
__interrupt void USCIAB0TX_ISR(void)
{
  if(IFG2&UCB0TXIFG)
  {
    if(g_cI2CTxCnt)
    {	
      UCB0TXBUF=*g_pI2CTxDat++;
      g_cI2CTxCnt--;
    }
    else
    {	
      UCB0CTL1 |= UCTXSTP;
      IFG2 &= ~UCB0TXIFG;
    }
  }
  if(IFG2&UCB0RXIFG)
  {
    if(g_cI2CStatus==I2CSTATUS_RXLEN)
    {
      g_cI2CRxLen=UCB0RXBUF;
      if(g_cI2CRxLen>=g_cI2CBufSize) g_cI2CRxLen=g_cI2CBufSize-1;
      *g_pI2CRxDat=g_cI2CRxLen;
      g_cI2CRxCnt=g_cI2CRxLen;
      g_pI2CRxDat++;
      g_cI2CStatus=I2CSTATUS_RXDAT;
    }
    else if(g_cI2CStatus==I2CSTATUS_RXDAT)
    {
      *g_pI2CRxDat=UCB0RXBUF;
      g_pI2CRxDat++;
      g_cI2CRxCnt--;
      if(g_cI2CRxCnt==0) g_cI2CStatus=I2CSTATUS_RXSUCC;
    }
    if(g_cI2CRxCnt==1) UCB0CTL1 |= UCTXSTP;
  }
}

void Init_I2C(void)
{
  UCB0CTL1 |= UCSWRST;
  UCB0CTL0=UCMST+UCMODE_3+UCSYNC;
  UCB0CTL1=UCSSEL_2+UCSWRST;
  UCB0BR0=12;
  UCB0BR1=0;
  UCB0I2CSA=SLA;
  UCB0CTL1 &= ~UCSWRST;
  IE2 |= UCB0TXIE;
  IE2 |= UCB0RXIE;
}

//============================================
//  initialize system hardware config
//  timer,uart,port
//============================================
void Init_Hardware(void)
{
  WDTCTL=WDTPW+WDTHOLD;
  BCSCTL1=CALBC1_1MHZ;
  DCOCTL=CALDCO_1MHZ;
  P3SEL=BIT5|BIT4|BIT2|BIT1
  Init_I2C();
  //  Timer Initialize
  TACCTL0=CCIE;
  TACCR0=10000;
  TACTL=TASSEL_2+MC_1;
  //  Port Initialize
  P3DIR |= 0x01;
  P4DIR |= 0x07;
  BuzzerOFF;
  __bis_SR_register(GIE);
}

//============================================
//  trun on buzzer about 100mS
//============================================
void BuzzerOn(void)
{
  BuzzerON;
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

void SendBuf_I2C(const unsigned char *dat,unsigned char len)
{
  g_pI2CTxDat=(unsigned char*)(dat);
  g_cI2CTxCnt=len;
  UCB0CTL1 |= UCTR + UCTXSTT;
}

void ReadBuf_I2C(unsigned char *dst,unsigned char BufSize)
{
  UCB0CTL1 &= ~UCTR;
  UCB0CTL1 |= UCTXSTT;
  g_cI2CStatus=I2CSTATUS_RXLEN;
  g_pI2CRxDat=dst;
  g_cI2CBufSize=BufSize;
}

unsigned char I2C_Send(const unsigned char *inbuf,unsigned char inbufsize,unsigned char *outbuf,unsigned char outbufsize)
{
  SendBuf_I2C(inbuf,inbufsize);
  Start_Time(5);
  while(g_bOverTime==0);
  ReadBuf_I2C(outbuf,outbufsize);
  Start_Time(5);
  while((g_bOverTime==0)&&(g_cI2CStatus!=I2CSTATUS_RXSUCC));
  if(g_cI2CStatus!=I2CSTATUS_RXSUCC) return 0;
  return 1;
}

unsigned char I2C_SelectCard(unsigned char *buf,unsigned char bufsize)
{ return(I2C_Send(IIC_ComSelectCard,sizeof(IIC_ComSelectCard),buf,bufsize));}
unsigned char I2C_LoginSector0(unsigned char *buf,unsigned char bufsize)
{ return(I2C_Send(IIC_ComLoginSector0,sizeof(IIC_ComLoginSector0),buf,bufsize));}
unsigned char I2C_WriteBlock1(unsigned char *buf,unsigned char bufsize)
{ return(I2C_Send(IIC_ComWriteBlock1,sizeof(IIC_ComWriteBlock1),buf,bufsize));}
unsigned char I2C_ReadBlock1(unsigned char *buf,unsigned char bufsize)
{ return(I2C_Send(IIC_ComReadBlock1,sizeof(IIC_ComReadBlock1),buf,bufsize));}
unsigned char I2C_ComIntiPurse1(unsigned char * buf, unsigned char bufsize)
{ return(I2C_Send(IIC_ComIntiPurse1,sizeof(IIC_ComIntiPurse1),buf,bufsize));}
unsigned char I2C_ComIncrPurse1(unsigned char * buf, unsigned char bufsize)
{ return(I2C_Send(IIC_ComIncrPurse1,sizeof(IIC_ComIncrPurse1),buf,bufsize));}
unsigned char I2C_ComDecrPurse1(unsigned char * buf, unsigned char bufsize)
{ return(I2C_Send(IIC_ComDecrPurse1,sizeof(IIC_ComDecrPurse1),buf,bufsize));}
unsigned char I2C_ComReadPurse1(unsigned char * buf, unsigned char bufsize)
{ return(I2C_Send(IIC_ComReadPurse1,sizeof(IIC_ComReadPurse1),buf,bufsize));}
unsigned char I2C_ComCopyValue(unsigned char * buf, unsigned char bufsize)
{ return(I2C_Send(IIC_ComCopyValue,sizeof(IIC_ComCopyValue),buf,bufsize));}
unsigned char I2C_ComRedLedOn(unsigned char * buf, unsigned char bufsize)
{ return(I2C_Send(IIC_ComRedLedOn,sizeof(IIC_ComRedLedOn),buf,bufsize));}
unsigned char I2C_ComRedLedOff(unsigned char * buf, unsigned char bufsize)
{ return(I2C_Send(IIC_ComRedLedOff,sizeof(IIC_ComRedLedOff),buf,bufsize));}
unsigned char I2C_ComWriteUltralightPage5(unsigned char * buf, unsigned char bufsize)
{ return(I2C_Send(IIC_ComWriteUltralightPage5,sizeof(IIC_ComWriteUltralightPage5),buf,bufsize));}
unsigned char I2C_ComReadUltralightPage5(unsigned char * buf, unsigned char bufsize)
{ return(I2C_Send(IIC_ComReadUltralightPage5,sizeof(IIC_ComReadUltralightPage5),buf,bufsize));}
void I2C_ComHalt(void)
{
  SendBuf_I2C(IIC_ComHalt,sizeof(IIC_ComHalt));
  Start_Time(5);
  while(g_bOverTime==0);
}

void Test_SL030(void)
{
  #define RCVCMD g_cRxBuf[1]
  #define RCVSTA g_cRxBuf[2]
  WAKEUP_H;
  Start_Time(5);
  while(g_bOverTime==0);
  WAKEUP_L;
  Start_Time(5);
  while(g_bOverTime==0);
  WAKEUP_H;
  g_cCardType=0xff;
  if(CARDIN!=0) return;
  Init_I2C();
  if(I2C_SelectCard(g_cRxBuf,sizeof(g_cRxBuf))==0) return;
  if((RCVCMD!=0x01)||(RCVSTA!=0)) return;
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
      if(I2C_LoginSector0(g_cRxBuf,sizeof(g_cRxBuf))==0) return;
      if((RCVCMD!=0x02)||(RCVSTA!=0x02)) return;
      if(I2C_WriteBlock1(g_cRxBuf,sizeof(g_cRxBuf))==0) return;
      if((RCVCMD!=0x04)||(RCVSTA!=0x00)) return;
      if(I2C_ReadBlock1(g_cRxBuf,sizeof(g_cRxBuf))==0) return;
      if((RCVCMD!=0x03)||(RCVSTA!=0x00)) return;
      if(memcmp(&IIC_ComWriteBlock1[3],&g_cRxBuf[3],16)!=0) return;
      if(I2C_ComIntiPurse1(g_cRxBuf,sizeof(g_cRxBuf))==0) return;
      if((RCVCMD!=0x06)||(RCVSTA!=0x00)) return;
      if(I2C_ComIncrPurse1(g_cRxBuf,sizeof(g_cRxBuf))==0) return;
      if((RCVCMD!=0x08)||(RCVSTA!=0x00)) return;
      if(I2C_ComDecrPurse1(g_cRxBuf,sizeof(g_cRxBuf))==0) return;
      if((RCVCMD!=0x09)||(RCVSTA!=0x00)) return;
      if(I2C_ComReadPurse1(g_cRxBuf,sizeof(g_cRxBuf))==0) return;
      if((RCVCMD!=0x05)||(RCVSTA!=0x00)) return;
      //Check value	 
      lPurseValue=g_cRxBuf[6];
      lPurseValue=(lPurseValue<<8)+g_cRxBuf[5];
      lPurseValue=(lPurseValue<<8)+g_cRxBuf[4];
      lPurseValue=(lPurseValue<<8)+g_cRxBuf[3];
      if(lPurseValue!=0x12345678+0x00000002-0x00000001) return;
      if(I2C_ComCopyValue(g_cRxBuf,sizeof(g_cRxBuf))==0) return;
      if((RCVCMD!=0x0a)||(RCVSTA!=0x00)) return;
      I2C_ComHalt();
      BuzzerOn();
      break;
    case CARDTYPE_UL:
      if(I2C_ComWriteUltralightPage5(g_cRxBuf,sizeof(g_cRxBuf))==0) return;
      if((RCVCMD!=0x11)||(RCVSTA!=0x00)) return;
      if(I2C_ComReadUltralightPage5(g_cRxBuf,sizeof(g_cRxBuf))==0) return;
      if((RCVCMD!=0x10)||(RCVSTA!=0x00)) return;
      if(memcmp(&IIC_ComWriteUltralightPage5[3],&g_cRxBuf[3],4)!=0) return;
      I2C_ComHalt();
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

