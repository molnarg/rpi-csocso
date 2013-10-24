////////////////////////////////////////////////////////////
//Test SL030
//ARM C source code
//LM3S600(6MHz) + EWARM 5.30
////////////////////////////////////////////////////////////   
#include "hw_types.h"
#include "hw_memmap.h"
#include "hw_ints.h"
#include "string.h"
#include "sysctl.h"
#include "gpio.h"
#include "systick.h"
#include "interrupt.h"
#include "i2c.h"

//============================================
//  Command List, preamble + length + command 
//============================================
const unsigned char __packed ComSelectCard[]=           {1,1};
const unsigned char __packed ComLoginSector0[]=         {9,2,0+2,0xAA,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
const unsigned char __packed ComReadBlock1[]=           {2,3,1+8};
const unsigned char __packed ComWriteBlock1[]=          {18,4,1+8,0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xAA,0xBB,0xCC,0xDD,0xEE,0xFF};
const unsigned char __packed ComIntiPurse1[]=           {6,6,1+8,0x78,0x56,0x34,0x12};
const unsigned char __packed ComReadPurse1[]=           {2,5,1+8};
const unsigned char __packed ComIncrPurse1[]=           {6,8,1+8,0x02,0x00,0x00,0x00};
const unsigned char __packed ComDecrPurse1[]=           {6,9,1+8,0x01,0x00,0x00,0x00};
const unsigned char __packed ComCopyValue[]=            {3,0x0A,1+8,2+8};              
const unsigned char __packed ComReadUltralightPage5[]=  {2,0x10,0x05};
const unsigned char __packed ComWriteUltralightPage5[]= {6,0x11,0x05,0x12,0x34,0x56,0x78};
const unsigned char __packed ComHalt[]=                 {1,0x50};

//============================================
//  global variable define
//============================================
unsigned char g_cCardType;
unsigned long int g_iTimer;
unsigned char g_bOverTime;
unsigned char g_cI2CRxBuf[60];

//============================================
//  i/o port define
//============================================
#define WAKEUP_SL030 GPIO_PORTE_BASE,GPIO_PIN_0
#define CARDIN_SL030 GPIO_PORTB_BASE,GPIO_PIN_1

//============================================
//  i2c slave address define
//============================================
#define SLVADD 0x50

//============================================
//  procedure define
//============================================
void InitializeSystem(); 
void Start_Time(unsigned long int ms);
void Stop_Time();
void Test_SL030();

void main()
{ InitializeSystem();
  g_iTimer=0;
  while(1) Test_SL030();
}

//============================================
//	system clock interrupt procedure
//============================================
void SysTickIntHandler(void)
{ if(g_iTimer!=0)
  { g_iTimer--;
    if(g_iTimer==0) g_bOverTime=1;
  }
}

//============================================
//  initialize system hardware config
//  system clock,i2c interface
//============================================
void InitializeSystem()
{	
  //init sysclock
  //period=1ms
  SysCtlClockSet(SYSCTL_SYSDIV_4|SYSCTL_USE_PLL|SYSCTL_OSC_MAIN|SYSCTL_XTAL_6MHZ);
  SysTickPeriodSet(SysCtlClockGet()/1000);
  SysTickEnable();
  SysTickIntEnable();
  
  //init io port
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
  GPIODirModeSet(GPIO_PORTB_BASE,GPIO_PIN_1,GPIO_DIR_MODE_IN);
  GPIOPadConfigSet(GPIO_PORTB_BASE,GPIO_PIN_1,GPIO_STRENGTH_2MA,GPIO_PIN_TYPE_STD_WPU);
  GPIODirModeSet(GPIO_PORTE_BASE,GPIO_PIN_0,GPIO_DIR_MODE_OUT);
  GPIOPadConfigSet(GPIO_PORTE_BASE,GPIO_PIN_0,GPIO_STRENGTH_8MA,GPIO_PIN_TYPE_STD);
  
  //init i2c interface
  //SCL=PB2,SDA=PB3
  GPIOPinTypeI2C(GPIO_PORTB_BASE,GPIO_PIN_2|GPIO_PIN_3);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C);
  I2CMasterInitExpClk(I2C_MASTER_BASE,SysCtlClockGet(),true);
  
  //interrupt enable
  IntMasterEnable();
}

//============================================
//  start timer
//============================================
void Start_Time(unsigned long int ms)
{ g_iTimer=ms;
  g_bOverTime=0;
}

//============================================
//  stop timer
//============================================
void Stop_Time()
{ g_iTimer=0;
}

//============================================
//  send buf by i2c hardware interface
//============================================
void SendBuf_I2C(unsigned char *dat,unsigned char len)
{ unsigned char counter;
  I2CMasterSlaveAddrSet(I2C_MASTER_BASE,SLVADD,false);
  I2CMasterDataPut(I2C_MASTER_BASE,dat[0]);
  I2CMasterControl(I2C_MASTER_BASE,I2C_MASTER_CMD_BURST_SEND_START);
  while(I2CMasterBusy(I2C_MASTER_BASE));
  for(counter=1;counter<len-1;counter++)
  { I2CMasterDataPut(I2C_MASTER_BASE,dat[counter]);
    I2CMasterControl(I2C_MASTER_BASE,I2C_MASTER_CMD_BURST_SEND_CONT);
    while(I2CMasterBusy(I2C_MASTER_BASE));
  }
  I2CMasterDataPut(I2C_MASTER_BASE,dat[counter]);
  I2CMasterControl(I2C_MASTER_BASE,I2C_MASTER_CMD_BURST_SEND_FINISH);
  while(I2CMasterBusy(I2C_MASTER_BASE));
}

//============================================
//  receive buf by i2c hardware interface
//============================================
unsigned char ReadBuf_I2C(unsigned char *dst,unsigned char BufSize)
{ unsigned char counter;
  unsigned long int cnt;
  I2CMasterSlaveAddrSet(I2C_MASTER_BASE,SLVADD,true);
  I2CMasterControl(I2C_MASTER_BASE,I2C_MASTER_CMD_BURST_RECEIVE_START);
  cnt=10000;
  while((I2CMasterBusy(I2C_MASTER_BASE))&&(--cnt));
  if(cnt==0) return 0;
  dst[0]=I2CMasterDataGet(I2C_MASTER_BASE);
  if(dst[0]>=BufSize) return 0;
  for(counter=1;counter<dst[0];counter++)
  { I2CMasterControl(I2C_MASTER_BASE,I2C_MASTER_CMD_BURST_RECEIVE_CONT);
    cnt=10000;
    while((I2CMasterBusy(I2C_MASTER_BASE))&&(--cnt));
    dst[counter]=I2CMasterDataGet(I2C_MASTER_BASE);
  }
  I2CMasterControl(I2C_MASTER_BASE,I2C_MASTER_CMD_BURST_RECEIVE_FINISH);
  cnt=10000;
  while((I2CMasterBusy(I2C_MASTER_BASE))&&(--cnt));
  if(cnt==0) return 0;
  dst[counter]=I2CMasterDataGet(I2C_MASTER_BASE);
  return 1;
}

//============================================
//  SL030 test procedure
//============================================
void Test_SL030()
{	
  #define RCVCMD_SL030 g_cI2CRxBuf[1]
  #define RCVSTA_SL030 g_cI2CRxBuf[2]
  #define DELAYTIME 50
  unsigned long int lPurseValue;
  g_cCardType=0xff;
  
  if((GPIOPinRead(CARDIN_SL030)&GPIO_PIN_1)==GPIO_PIN_1) return;
  
  SendBuf_I2C((unsigned char *)(ComSelectCard),sizeof(ComSelectCard));
  Start_Time(DELAYTIME);
  while(g_bOverTime==0);
  Stop_Time();
  if(ReadBuf_I2C(g_cI2CRxBuf,sizeof(g_cI2CRxBuf))==0) return;
  if((RCVCMD_SL030!=0x01)||(RCVSTA_SL030!=0)) return;
  g_cCardType=g_cI2CRxBuf[g_cI2CRxBuf[0]];
  
  switch(g_cCardType)
  { case 1:                     //Mifare 1k 4 byte UID
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
      
      GPIOPinWrite(WAKEUP_SL030,0x00);
      Start_Time(1);
      while(g_bOverTime==0);
      Stop_Time();
       
      GPIOPinWrite(WAKEUP_SL030,0xff);
      Start_Time(10);
      while(g_bOverTime==0);
      Stop_Time();
      break;
    

    case 3:                     //Ultralight
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

      GPIOPinWrite(WAKEUP_SL030,0x00);
      Start_Time(1);
      while(g_bOverTime==0);
      Stop_Time();
      GPIOPinWrite(WAKEUP_SL030,0xff);
      Start_Time(10);
      while(g_bOverTime==0);
      Stop_Time();
      break;
    

    default:
      break;
  }
}

