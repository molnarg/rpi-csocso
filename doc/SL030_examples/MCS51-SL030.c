/////////////////////////////////////////////////////////////////////
//TestSl030
//MCS51 Code
/////////////////////////////////////////////////////////////////////
#include <reg52.h>
#include <string.h>
#include <intrins.h>

sbit SDA    = P1^1;
sbit SCL    = P1^2;
sbit CARDIN = P1^0;
sbit WAKEUP = P1^3;
sbit OKLED  = P2^0;
sbit ERRLED = P2^1;


#define true     1
#define false    0
#define trun_on  0
#define trun_off 1

#define SL030ADR 0xA0

#define SomeNOP(); _nop_();_nop_();_nop_();_nop_();

#define OSC_FREQ         22118400L
#define TIME0_10ms       65536L - OSC_FREQ/1200L

char ReadWriteUltralight(void);
char ReadWriteMifareStd(void);
char WriteSL030(unsigned char *pData);
char ReadSL030(unsigned char *pData);
void InitializeSystem();                                                           
void StartTime(unsigned int _MS);                                                  
void StopTime();
void I2CStart(void);
void I2CStop(void);
unsigned char WaitAck(void);
void SendAck(void);
void SendNotAck(void);
void I2CSendByte(unsigned char ch);
unsigned char I2CReceiveByte(void);


                                     
/*Command*/
unsigned char code ComSelectCard[3]    = {1,1};                                               

unsigned char code ComLoginSector0[10] = {9,2,0+2,0xAA,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

unsigned char code ComReadBlock1[3]    = {2,3,1+8};       
 
unsigned char code ComWriteBlock1[19]  = {18,4,1+8,
                                          0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,
                                          0x88,0x99,0xAA,0xBB,0xCC,0xDD,0xEE,0xFF};                 
                               
unsigned char code ComIntiPurse1[7]    = {6,6,1+8,0x78,0x56,0x34,0x12};                               
                               
unsigned char code ComReadPurse1[3]    = {2,5,1+8};       
                            
unsigned char code ComIncrPurse1[7]    = {6,8,1+8,0x02,0x00,0x00,0x00};                                
                               
unsigned char code ComDecrPurse1[7]    = {6,9,1+8,0x01,0x00,0x00,0x00};                                

unsigned char code ComCopyValue[4]     = {3,0x0A,1+8,2+8};
              
unsigned char code ComReadUltralightPage5[3]  = {2,0x10,0x05};

unsigned char code ComWriteUltralightPage5[7] = {6,0x11,0x05,0x12,0x34,0x56,0x78};

unsigned char code ComHalt[2]     = {1,0x50};


unsigned char g_ucTempBuf[26];
unsigned char g_ucWatchTime;
unsigned char g_ucCardType;
char g_cStatus;
char g_cErr;
bit  g_bTimeOut;                                                  

void main( )
{  
     InitializeSystem();	
     while(1)
     {

         while (!CARDIN)
         {
              g_cErr = 0;

              g_cStatus = WriteSL030(ComSelectCard);
              if (g_cStatus != true)
              {    
              	  g_cErr = 1;
              	  continue;
    	      }
              g_cStatus = ReadSL030(g_ucTempBuf); 
              if ((g_cStatus != true) || (g_ucTempBuf[1] != 1) || (g_ucTempBuf[2] != 0))
              {    
              	  g_cErr = 2;
              	  continue;
    	      }
    	      if (g_ucTempBuf[0] == 7)
    	      {
    	      	  g_ucCardType = g_ucTempBuf[7];
    	      }
    	      else
    	      {
    	      	  g_ucCardType = g_ucTempBuf[10];
    	      }
    	      
    	      switch(g_ucCardType)
              {  
                  case 1://Mifare 1k 4 byte UID
                  case 2://Mifare 1k 7 byte UID
                      g_cStatus = ReadWriteMifareStd( );
                      if (g_cStatus != true)
                      {    g_cErr = 3;    }
                      break;
                  case 3://Mifare_UltraLight
                      g_cStatus = ReadWriteUltralight( );
                      if (g_cStatus != true)
                      {    g_cErr = 3;    }
                      break;
                  case 4://Mifare 4k 4 byte UID
                  case 5://Mifare 4k 7 byte UID
                      g_cStatus = ReadWriteMifareStd( );
                      if (g_cStatus != true)
                      {    g_cErr = 3;    }
                      break;
                  case 6://Mifare_DesFire 
                      g_cErr = 4;
                      break;                 
    	          default:
    	              g_cErr = 3;
                      break;
              }
               
              g_cStatus = WriteSL030(ComHalt);
              if (g_cStatus != true)
              {    g_cErr = 1;   }
              StartTime(20);                                
              while (!g_bTimeOut);
              StopTime();
              WAKEUP = 0;
              SomeNOP();
              WAKEUP = 1;   
               
               if (g_cErr == 0)
               {
               	   OKLED = trun_on;
               	   StartTime(200);
               	   while (!g_bTimeOut);
               	   StopTime();
               	   OKLED = trun_off; 
                   StartTime(100);
               	   while (!g_bTimeOut);
               	   StopTime();
               }
               else
               {
               	   ERRLED = trun_on;
               	   StartTime(200);
               	   while (!g_bTimeOut);
               	   StopTime();
               	   ERRLED = trun_off; 
                   StartTime(100);
               	   while (!g_bTimeOut);
               	   StopTime();
               }
         }
    }
}

char ReadWriteUltralight( )
{
    char cStatus;

    cStatus = WriteSL030(ComWriteUltralightPage5);
    if (cStatus == true)
    {
        cStatus = ReadSL030(g_ucTempBuf);
        if ((g_cStatus != true) 
            || (g_ucTempBuf[1] != 0x11)//Command code 
            || (g_ucTempBuf[2] != 0)//status
           )
        {    return false;   }
    }
 
    cStatus = WriteSL030(ComReadUltralightPage5);
    if (cStatus == true)
    {
        g_cStatus = ReadSL030(g_ucTempBuf);
        if ((g_cStatus != true) 
            || (g_ucTempBuf[1] != 0x10)//Command code 
            || (g_ucTempBuf[2] != 0)//status
           )
        {    return false;   }
    }
             
          
    if (memcmp(&ComWriteUltralightPage5[3], &g_ucTempBuf[3], 4) != 0 )
    {    return false;   }
            
    return cStatus;   

}

char ReadWriteMifareStd( )
{
    char cStatus;
    long lPurseValue;

    cStatus = WriteSL030(ComLoginSector0);
    if (cStatus == true)
    {
        cStatus = ReadSL030(g_ucTempBuf);
        if ((g_cStatus != true) 
            || (g_ucTempBuf[1] != 2)//Command code 
            || (g_ucTempBuf[2] != 2)//status, 0x02: Login success
           )
        {    return false;   }
    }
 
    cStatus = WriteSL030(ComWriteBlock1);
    if (cStatus == true)
    {
        g_cStatus = ReadSL030(g_ucTempBuf);
        if ((g_cStatus != true) 
            || (g_ucTempBuf[1] != 4)//Command code 
            || (g_ucTempBuf[2] != 0)//status
           )
        {    return false;   }
    }
             
    cStatus = WriteSL030(ComReadBlock1);
    if (cStatus == true)
    {
    	g_cStatus = ReadSL030(g_ucTempBuf);
        if ((g_cStatus != true) 
            || (g_ucTempBuf[1] != 3)//Command code 
            || (g_ucTempBuf[2] != 0)//status
           )
        {    return false;   }
    }   
        
    if (memcmp(&ComWriteBlock1[3], &g_ucTempBuf[3], 16) != 0 )
    {    return false;   }
              
    cStatus = WriteSL030(ComIntiPurse1);
    if (cStatus == true)
    {
    	g_cStatus = ReadSL030(g_ucTempBuf);
        if ((g_cStatus != true) 
            || (g_ucTempBuf[1] != 6)//Command code 
            || (g_ucTempBuf[2] != 0)//status
           )
        {    return false;   }
    }   
          
    cStatus = WriteSL030(ComIncrPurse1);
    if (cStatus == true)
    {
    	g_cStatus = ReadSL030(g_ucTempBuf);
        if ((g_cStatus != true) 
            || (g_ucTempBuf[1] != 8)//Command code 
            || (g_ucTempBuf[2] != 0)//status
           )
        {    return false;   }
    }   
              
    cStatus = WriteSL030(ComDecrPurse1);
    if (cStatus == true)
    {
    	g_cStatus = ReadSL030(g_ucTempBuf);
        if ((g_cStatus != true) 
            || (g_ucTempBuf[1] != 9)//Command code 
            || (g_ucTempBuf[2] != 0)//status
           )
        {    return false;   }
    }   
              
    cStatus = WriteSL030(ComReadPurse1);
    if (cStatus == true)
    {
    	g_cStatus = ReadSL030(g_ucTempBuf);
        if ((g_cStatus != true) 
            || (g_ucTempBuf[1] != 5)//Command code 
            || (g_ucTempBuf[2] != 0)//status
           )
        {    return false;   }
    }   
    lPurseValue = g_ucTempBuf[6]*0x1000000 + g_ucTempBuf[5]*0x10000 + g_ucTempBuf[4]*0x100 + g_ucTempBuf[3];
    if (lPurseValue != 0x12345678 + 0x00000002 - 0x00000001)
    {    return false;   }
                            
    cStatus = WriteSL030(ComCopyValue);
    if (cStatus == true)
    {
    	g_cStatus = ReadSL030(g_ucTempBuf);
        if ((g_cStatus != true) 
            || (g_ucTempBuf[1] != 0x0A)//Command code 
            || (g_ucTempBuf[2] != 0)//status
           )
        {    return false;   }
    }
    
    return cStatus;   

}

/////////////////////////////////////////////////////////////////////
//Initialze T0 16 bit Timer
/////////////////////////////////////////////////////////////////////
void InitializeSystem()
{
    TMOD = 0x21; 
    IE   = 0x80;
}

/////////////////////////////////////////////////////////////////////
//Send Command to SL030
/////////////////////////////////////////////////////////////////////
char WriteSL030(unsigned char *pCommand)
{
     unsigned char j;
     char status;

     I2CStart();
     I2CSendByte(SL030ADR);
     
     status = WaitAck();
     
     j = 0;
     while ((status == true) && (j < pCommand[0]+1))
     {	  
          I2CSendByte(*(pCommand + j));
          status = WaitAck();
     	  j++;
     }
     I2CStop();
     return status;
}

/////////////////////////////////////////////////////////////////////
//Get result from SL030
/////////////////////////////////////////////////////////////////////
char ReadSL030(unsigned char *pData)
{
     unsigned char i,j;
     char status;
     
     StartTime(8);
     
     do
     {
         I2CStart();
         I2CSendByte(SL030ADR | 1);
         status = WaitAck();
     }while((status != true) && !g_bTimeOut);

     if ((status == false) || g_bTimeOut)  
     {
         I2CStop();
         return false;
     }   
          
     i = I2CReceiveByte();
     
     
     if ((i == 0) || (i > 18))
     {
         I2CStop();
         return false;
     }   
     else
     {   
     	 SendAck();
         *pData = i;
         for (j=1; j<i; j++)
         {    
              *(pData + j) = I2CReceiveByte();
              SendAck();
         }
         *(pData + j) = I2CReceiveByte();
         SendNotAck(); 
         I2CStop();
         return true;
     }
}

/////////////////////////////////////////////////////////////////////
void I2CStart()
{
   // EA  = 0;
    SDA = 1; 
    SomeNOP();
    SomeNOP();
    SomeNOP();
    SCL = 1;
    SomeNOP();
    SomeNOP();
    SomeNOP();
    SDA = 0;
    SomeNOP();
    SCL = 0;
}

/////////////////////////////////////////////////////////////////////
void I2CStop()
{
    SCL = 0;
    SDA = 0; 
    SomeNOP(); //INI
    SCL = 1;
    SomeNOP(); 
    SDA = 1; //STOP
  //  EA=1;
}

/////////////////////////////////////////////////////////////////////
unsigned char WaitAck()
{
    unsigned char errtime = 255;
    SDA = 1;
    SomeNOP();
    SCL = 1;
    SomeNOP();
    while(SDA)
    {
        errtime--; 
        if (!errtime)
        {
            I2CStop();
            return false;
        }
    }
    SCL = 0;
    return true;
}

/////////////////////////////////////////////////////////////////////
void SendAck(void)
{
    SDA = 0; 
    SomeNOP();
    SCL = 1; 
    SomeNOP();
    SCL = 0;
}

/////////////////////////////////////////////////////////////////////
void SendNotAck(void)
{
    SDA = 1; 
    SomeNOP();
    SCL = 1; 
    SomeNOP();
    SCL = 0;
}

/////////////////////////////////////////////////////////////////////
void I2CSendByte(unsigned char ch)
{
    unsigned char i = 8;
    while (i--)
    {
        SCL = 0;
        SomeNOP();
        SDA = (bit)(ch&0x80); 
        ch <<= 1;
        SomeNOP();
        SCL = 1; 
        SomeNOP();
    }
    SomeNOP();
    SCL = 0;
}

/////////////////////////////////////////////////////////////////////
unsigned char I2CReceiveByte()
{
    unsigned char i = 8;
    unsigned char ddata = 0;
    
    SDA = 1;
    while (i--)
    {
        ddata <<= 1;
        SCL = 0;
        SomeNOP();
        SomeNOP();
        SomeNOP();
        SCL = 1;
        SomeNOP();
        SomeNOP();
        SomeNOP();
        ddata |= SDA;
    }
    SomeNOP();
    SCL = 0;
    
    return ddata;
}


/////////////////////////////////////////////////////////////////////
void StartTime(unsigned int _MS)
{
    TH0 = (unsigned char)((TIME0_10ms>>8)&0xFF);
    TL0 = (unsigned char)(TIME0_10ms&0xFF);
    g_ucWatchTime = _MS;
    g_bTimeOut = 0;
    ET0 = 1;
    TR0 = 1;
}
	
/////////////////////////////////////////////////////////////////////
time0_int () interrupt 1 using 1
{
    TH0 = (unsigned char)((TIME0_10ms>>8) & 0xFF);
    TL0 = (unsigned char)(TIME0_10ms & 0xFF);
    if (g_ucWatchTime--==0)
    {    g_bTimeOut = 1;    }
}

/////////////////////////////////////////////////////////////////////
void StopTime()
{
    ET0 = 0;
    TR0 = 0;
}
