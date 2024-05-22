// **************************************************************************
// IT8951Display_I80_Example.c
//
// Implementation of example code for IT8951 Display via I80/SPI/I2C Bus (Host side)
//
// Copyright (c) 2008 ITE Tech. Inc. All Rights Reserved.
//
// Author: Eric Su, June 29, 2016
// **************************************************************************

#include "ite_display_i80_example.h"



//-----------------------------------------------------------
//Host controller function 1 ¡V Wait for host data Bus Ready
//-----------------------------------------------------------
void LCDWaitForReady()
{
	//Regarding to HRDY
	//you may need to use a GPIO pin connected to HRDY of IT8951
    TDWord ulData = HRDY;
    while(ulData == 0)
    {
        //Get status of HRDY
        ulData = HRDY;
    }
}

//-----------------------------------------------------------
//   Select Host  interface 
//-----------------------------------------------------------
#define __SPI_2_I80_INF__   //Enable SPI interface , else i80 interface
//#define __I2C_2_I80_INF     //Enable I2C interface

//-----------------------------------------------------------
//
//-----------------------------------------------------------

#if defined(__SPI_2_I80_INF__) //{__SPI_2_I80_INF__

//-------------------------------------------------------------------
// SPI Interface basic function
//-------------------------------------------------------------------

    #define __HOST_LITTLE_ENDIAN__  //Big or Little Endian for your Host platform

    #ifdef __HOST_LITTLE_ENDIAN__
    	//Little Endian => its needs to convert for SPI to I80
    	#define  MY_WORD_SWAP(x) ( ((x & 0xff00)>>8) | ((x & 0x00ff)<<8) )
    #else
    	//Big Endian => No need to convert
    	#define  MY_WORD_SWAP(x) (x)
    #endif

//-------------------------------------------------------------------
//  Write Command code
//-------------------------------------------------------------------
void LCDWriteCmdCode (TWord wCmd)
{	
	WORD wPreamble = 0; 

	//Set Preamble for Write Command
	wPreamble = 0x6000; 
	
	//Send Preamble
	wPreamble	= MY_WORD_SWAP(wPreamble);
	LCDWaitForReady();
	SPIWrite((TByte*)&wPreamble, 1*2, CS_L);	

	//Send Command
	wCmd		= MY_WORD_SWAP(wCmd);
	LCDWaitForReady();
	SPIWrite((TByte*)&wCmd 1*2, CS_H);	
}
//-------------------------------------------------------------------
//  Write 1 Word Data(2-bytes)
//-------------------------------------------------------------------
void LCDWriteData(TWord usData)
{
	WORD wPreamble	= 0;

	//set type
	wPreamble = 0x0000;
	
	//Send Preamble
	wPreamble = MY_WORD_SWAP(wPreamble);
	LCDWaitForReady();
	SPIWrite((TByte*)&wPreamble, 1*2, CS_L);
	//Send Data
	usData = MY_WORD_SWAP(usData);
	LCDWaitForReady();
	SPIWrite((TByte*)&usData, 1*2, CS_H);
}   
//-------------------------------------------------------------------
//  Burst Write Data
//-------------------------------------------------------------------
void LCDWriteNData(TWord* pwBuf, TDWord ulSizeWordCnt)
{
	WORD wPreamble	= 0;
	TDWord i;

	//set type
	wPreamble = 0x0000;
	//Send Preamble
	wPreamble = MY_WORD_SWAP(wPreamble);
	LCDWaitForReady();
	SPIWrite((TByte*)&wPreamble, 1*2, CS_L);
	
	#ifdef __HOST_LITTLE_ENDIAN__
	//Convert Little to Big Endian for each Word
	for(i=0;i<ulSizeWordCnt;i++)
	{
		pwBuf[i] = MY_WORD_SWAP(pwBuf[i]);
	}
	#endif
	
	//Send Data
	LCDWaitForReady();
	SPIWrite((TByte*)pwBuf, ulSizeWordCnt*2, CS_H);
}   
//-------------------------------------------------------------------
// Read 1 Word Data
//-------------------------------------------------------------------
TWord LCDReadData()
{
	TWord wPreamble	= 0;
    TWord wRData; 
	TWord wDummy;

	//set type and direction
	wPreamble = 0x1000;
	
	//Send Preamble before reading data
	wPreamble = MY_WORD_SWAP(wPreamble);
	LCDWaitForReady();
	SPIWrite((TByte*)&wPreamble, 1*2, CS_L);

	//Read Dummy (under IT8951 SPI to I80 spec)
	LCDWaitForReady();
	SPIRead((TByte*)&wDummy, 1*2, CS_L );
	
	//Read Data
	LCDWaitForReady();
	SPIRead((TByte*)&wRData, 1*2, CS_H);
	
    wRData = MY_WORD_SWAP(wRData);
	return wRData;
}
//-------------------------------------------------------------------
//  Read Burst N words Data
//-------------------------------------------------------------------
TWord LCDReadNData(TWord* pwBuf, TDWord ulSizeWordCnt)
{
	TWord wPreamble	= 0;
    TWord wRData; 
	TWord wDummy;
     TDWord i;

	//set type and direction
	wPreamble = 0x1000;
	
	//Send Preamble before reading data
	wPreamble = MY_WORD_SWAP(wPreamble);
	LCDWaitForReady();
	SPIWrite((TByte*)&wPreamble, 1*2, CS_L);

	//Read Dummy (under IT8951 SPI to I80 spec)
	LCDWaitForReady();
	SPIRead((TByte*)&wDummy, 1*2, CS_L );
	
	//Read Data
	LCDWaitForReady();
	SPIRead((TByte*)pwBuf, ulSizeWordCnt *2, CS_H);

     //Convert Endian (depends on your host)
	for(i=0;i< ulSizeWordCnt ; i++)
    {
        pwBuf[i] = MY_WORD_SWAP(pwBuf[i]);
    }
}

#elif defined(__I2C_2_I80_INF)  
//-------------------------------------------------------------------
//                              I2C Inteface
//-------------------------------------------------------------------
#define I2C_SLAVE_ID	         0x46 //7-bits

#define I80_I2C_CMD_TYPE_CMD     0x00 //Preamble of Command
#define I80_I2C_CMD_TYPE_DATA    0x80 //Preamble of Data
//------------------------------------------------------
//Pseudo Code - Basic i2c Write function
//------------------------------------------------------
void i2c_master_tx(TByte ucSlaveID, TByte Premable, TByte ulSize, TByte* pWBuf)
{
	int i;
	
	//0. Start
	//1. Send: (SlaveID << 1)| 0 for Write 
	i2c_send_byte(ucSlaveID << 1 | 0);
	
	//2. Send Premable
	i2c_send_byte(Premable);
	
	//3. Send WBuf Data for ulSize bytes
	for(i=0;i<ulSize;i++)
	{
		i2c_send_byte(pWBuf[i]);
	}
	
	//4. Stop
}
//------------------------------------------------------
//Pseudo Code - Basic i2c Read function
//------------------------------------------------------
void i2c_master_rx(TByte ucSlaveID, TByte Premable, TByte ulSize, TByte* pRBuf)
{
	int i;
	TByte ucDummy[2];
	
	//0. Start
	//1. Send: (SlaveID << 1)| with Write 
	i2c_send_byte(ucSlaveID << 1 | 0);
	
	//2. Send Premable
	i2c_send_byte(Premable);
	
	//3. Send: (SlaveID << 1) with Read 
	i2c_send_byte(ucSlaveID << 1 | 1);
	
	//4. Read Dummy (1 Word = 2-bytes) and ignored
	ucDummy[0] = i2c_recv_byte();
	ucDummy[1] = i2c_recv_byte();
	
	//5. Recieve Data and store to Read Buffer
	for(i=0;i<ulSize;i++)
	{
		pRBuf[i] = i2c_recv_byte();
	}
	
	//6. Stop
}
//-------------------------------------------------------------------
//  Write Command code
//-------------------------------------------------------------------
void LCDWriteCmdCode (TWord wCmd)
{	
	
	LCDWaitForReady();
	
	wCmd = SWAP_16(wCmd);//I2C to I80 => Little Endian format
	//Send Preamble and Command
    i2c_master_tx(I2C_SLAVE_ID, I80_I2C_CMD_TYPE_CMD, 2, (TByte*)&wCmd); 
	
}
//-------------------------------------------------------------------
//  Write 1 Word Data(2-bytes)
//-------------------------------------------------------------------
void LCDWriteData(TWord usData)
{
	LCDWaitForReady();
	
	usData = SWAP_16(usData);//I2C to I80 => Little Endian format
	//Send Preamble and Data
    i2c_master_tx(I2C_SLAVE_ID, I80_I2C_CMD_TYPE_DATA, 2, (TByte*)&usData); 
	
}   
//-------------------------------------------------------------------
//  Burst Write Data
//-------------------------------------------------------------------
void LCDWriteNData(TWord* pwBuf, TDWord ulSizeWordCnt)
{
	
	TDWord i;
	
	#ifdef __HOST_LITTLE_ENDIAN__
	//Convert Little to Big Endian for each Word
	for(i=0;i<ulSizeWordCnt;i++)
	{
		pwBuf[i] = MY_WORD_SWAP(pwBuf[i]);
	}
	#endif
	
	//Send Data
	LCDWaitForReady();
	//Send Preamble and Data
    i2c_master_tx(I2C_SLAVE_ID, I80_I2C_CMD_TYPE_DATA, ulSizeWordCnt*2, (TByte*)pwBuf); 
	
}   
//-------------------------------------------------------------------
// Read 1 Word Data
//-------------------------------------------------------------------
TWord LCDReadData()
{
    TWord wRData; 
	TWord wDummy;

	
	LCDWaitForReady();
	
	//Read 1 16-bits Data
	i2c_master_rx(I2C_SLAVE_ID, I80_I2C_CMD_TYPE_DATA, 2, (TByte*)&wRData); 
	
    wRData = MY_WORD_SWAP(wRData);//Endian Convert if need
    
	return wRData;
}
//-------------------------------------------------------------------
//  Read Burst N words Data
//-------------------------------------------------------------------
TWord LCDReadNData(TWord* pwBuf, TDWord ulSizeWordCnt)
{
	
     TDWord i;

	LCDWaitForReady();
	
	//Read 1 16-bits Data
	i2c_master_rx(I2C_SLAVE_ID, I80_I2C_CMD_TYPE_DATA, ulSizeWordCnt*2, (TByte*)pwBuf); 

     //Convert Endian (depends on your host)
	for(i=0;i< ulSizeWordCnt ; i++)
    {
        pwBuf[i] = MY_WORD_SWAP(pwBuf[i]);
    }
}


#else//__SPI_2_I80_INF__

//-------------------------------------------------------------------
//            I80 interface
//-------------------------------------------------------------------

//-------------------------------------------------------------------
//Host controller Write command code for 16 bits using GPIO simulation
//-------------------------------------------------------------------
void gpio_i80_16b_cmd_out(TWord usCmd)
{
    LCDWaitForReady();
    //Set GPIO 0~7 to Output mode
    See your host setting of GPIO
    //Switch C/D to CMD => CMD - L
    GPIO_SET_L(CD);
    //CS-L
    GPIO_SET_L(CS);
    //WR Enable
    GPIO_SET_L(WEN);
    //Set Data output (Parallel output request)
    //See your host setting of GPIO 
    GPIO_I80_Bus[16] = usCmd;
    
    //WR Enable - H
    GPIO_SET_H(WEN);
    //CS-H
    GPIO_SET_H(CS);
}
//-------------------------------------------------------------------
//Host controller Write Data for 16 bits using GPIO simulation
//-------------------------------------------------------------------
void gpio_i80_16b_data_out(TWord usData)
{
    LCDWaitForReady();
    //e.g. - Set GPIO 0~7 to Output mode
    See your host setting of GPIO
    GPIO_I80_Bus[16] = usData;
    
    //Switch C/D to Data => Data - H
    GPIO_SET_H(CD);
    //CS-L
    GPIO_SET_L(CS);
    //WR Enable
    GPIO_SET_L(WEN);
    //Set 16 bits Bus Data
    See your host setting of GPIO
    //WR Enable - H
    GPIO_SET_H(WEN);
    //CS-H
    GPIO_SET_H(CS);
}
//-------------------------------------------------------------------
//Host controller Read Data for 16 bits using GPIO simulation
//-------------------------------------------------------------------
TWord gpio_i80_16b_data_in()
{
    TWord usData;
    
    LCDWaitForReady();
    //Set GPIO 0~7 to input mode
    See your host setting of GPIO
    //Switch C/D to Data - DATA - H
    GPIO_SET_H(CD);
    //CS-L
    GPIO_SET_L(CS);
    //RD Enable
    GPIO_SET_L(REN);
    //Get 8-bits Bus Data (Collect 8 GPIO pins to Byte Data)
    See your host setting of GPIO
    usData = GPIO_I80_Bus[16];
    //WR Enable - H
    GPIO_SET_H(WEN);
    //CS-H
    GPIO_SET_H(CS);
    return ucData;
}



//-----------------------------------------------------------------
//Host controller function 2 ¡V Write command code to host data Bus
//-----------------------------------------------------------------
void LCDWriteCmdCode(TWord usCmdCode)
{
    //wait for ready
    LCDWaitForReady();
    //write cmd code
   gpio_i80_16b_cmd_out(usCmdCode);
}

//-----------------------------------------------------------
//Host controller function 3 ¡V Write Data to host data Bus
//-----------------------------------------------------------
void LCDWriteData(TWord usData)
{
    //wait for ready
    LCDWaitForReady();
    //write data
   gpio_i80_16b_data_out(usData);
}

//-----------------------------------------------------------
//Host controller function 4 ¡V Read Data from host data Bus
//-----------------------------------------------------------
TWord LCDReadData()
{
    TWord usData;
    //wait for ready
    LCDWaitForReady();
    //read data from host data bus
    usData = gpio_i80_16b_data_in();
    return usData;
}

#endif //}__SPI_2_I80_INF__

//-----------------------------------------------------------
//Host controller function 5 ¡V Write command to host data Bus with aruments
//-----------------------------------------------------------
void LCDSendCmdArg(TWord usCmdCode,TWord* pArg, TWord usNumArg)
{
     TWord i;
     //Send Cmd code
     LCDWriteCmdCode(usCmdCode);
     //Send Data
     for(i=0;i<usNumArg;i++)
     {
         LCDWriteData(pArg[i]);
     }
}
//-----------------------------------------------------------
//Host Cmd 1 ¡V SYS_RUN
//-----------------------------------------------------------
void IT8951SystemRun()
{
    LCDWriteCmdCode(IT8951_TCON_SYS_RUN);
}
//-----------------------------------------------------------
//Host Cmd 2 - STANDBY
//-----------------------------------------------------------
void IT8951StandBy()
{
    LCDWriteCmdCode(IT8951_TCON_STANDBY);
}
//-----------------------------------------------------------
//Host Cmd 3 - SLEEP
//-----------------------------------------------------------
void IT8951Sleep()
{
    LCDWriteCmdCode(IT8951_TCON_SLEEP);
}
//-----------------------------------------------------------
//Host Cmd 4 - REG_RD
//-----------------------------------------------------------
TWord IT8951ReadReg(TWord usRegAddr)
{
    TWord usData;
    //----------I80 Mode-------------
    //Send Cmd and Register Address
    LCDWriteCmdCode(IT8951_TCON_REG_RD);
    LCDWriteData(usRegAddr);
    //Read data from Host Data bus
    usData = LCDReadData();
    return usData;
}
//-----------------------------------------------------------
//Host Cmd 5 - REG_WR
//-----------------------------------------------------------
void IT8951WriteReg(TWord usRegAddr,TWord usValue)
{
    //I80 Mode
    //Send Cmd , Register Address and Write Value
    LCDWriteCmdCode(IT8951_TCON_REG_WR);
    LCDWriteData(usRegAddr);
    LCDWriteData(usValue);
}
//-----------------------------------------------------------
//Host Cmd 6 - MEM_BST_RD_T
//-----------------------------------------------------------
void IT8951MemBurstReadTrigger(TDWord ulMemAddr , TDWord ulReadSize)
{
    TWord usArg[4];
    //Setting Arguments for Memory Burst Read
    usArg[0] = (TWord)(ulMemAddr & 0x0000FFFF); //addr[15:0]
    usArg[1] = (TWord)( (ulMemAddr >> 16) & 0x0000FFFF ); //addr[25:16]
    usArg[2] = (TWord)(ulReadSize & 0x0000FFFF); //Cnt[15:0]
    usArg[3] = (TWord)( (ulReadSize >> 16) & 0x0000FFFF ); //Cnt[25:16]
    //Send Cmd and Arg
    LCDSendCmdArg(IT8951_TCON_MEM_BST_RD_T , usArg , 4);
}
//-----------------------------------------------------------
//Host Cmd 7 - MEM_BST_RD_S
//-----------------------------------------------------------
void IT8951MemBurstReadStart()
{
    LCDWriteCmdCode(IT8951_TCON_MEM_BST_RD_S);
}
//-----------------------------------------------------------
//Host Cmd 8 - MEM_BST_WR
//-----------------------------------------------------------
void IT8951MemBurstWrite(TDWord ulMemAddr , TDWord ulWriteSize)
{
    TWord usArg[4];
    //Setting Arguments for Memory Burst Write
    usArg[0] = (TWord)(ulMemAddr & 0x0000FFFF); //addr[15:0]
    usArg[1] = (TWord)( (ulMemAddr >> 16) & 0x0000FFFF ); //addr[25:16]
    usArg[2] = (TWord)(ulWriteSize & 0x0000FFFF); //Cnt[15:0]
    usArg[3] = (TWord)( (ulWriteSize >> 16) & 0x0000FFFF ); //Cnt[25:16]
    //Send Cmd and Arg
    LCDSendCmdArg(IT8951_TCON_MEM_BST_WR , usArg , 4);
}
//-----------------------------------------------------------
//Host Cmd 9 - MEM_BST_END
//-----------------------------------------------------------
void IT8951MemBurstEnd(void)
{
    LCDWriteCmdCode(IT8951_TCON_MEM_BST_END);
}
//-----------------------------------------------------------
//Example of Memory Burst Write
//-----------------------------------------------------------
// ****************************************************************************************
// Function name: IT8951MemBurstWriteProc( )
//
// Description:
//   IT8951 Burst Write procedure
//      
// Arguments:
//      TDWord ulMemAddr: IT8951 Memory Target Address
//      TDWord ulWriteSize: Write Size (Unit: Word)
//      TByte* pDestBuf - Buffer of Sent data
// Return Values:
//   NULL.
// Note:
//
// ****************************************************************************************
void IT8951MemBurstWriteProc(TDWord ulMemAddr , TDWord ulWriteSize, TWord* pSrcBuf )
{
    
    TDWord i;
 
    //Send Burst Write Start Cmd and Args
    IT8951MemBurstWrite(ulMemAddr , ulWriteSize);
 
 #ifdef __SPI_2_I80_INF__
    LCDWriteNData(pSrcBuf, ulWriteSize); //Please kindly note that there could be the limiationa of max transfer size for your host spi controller
 #else
    //Burst Write Data
    for(i=0;i<ulWriteSize;i++)
    {
        LCDWriteData(pSrcBuf[i]);
    }
#endif
 
    //Send Burst End Cmd
    IT8951MemBurstEnd();
}

// ****************************************************************************************
// Function name: IT8951MemBurstReadProc( )
//
// Description:
//   IT8951 Burst Read procedure
//      
// Arguments:
//      TDWord ulMemAddr: IT8951 Read Memory Address
//      TDWord ulReadSize: Read Size (Unit: Word)
//      TByte* pDestBuf - Buffer for storing Read data
// Return Values:
//   NULL.
// Note:
//
// ****************************************************************************************
void IT8951MemBurstReadProc(TDWord ulMemAddr , TDWord ulReadSize, TWord* pDestBuf )
{
    TDWord i;
    TWord CRFSRstatus;

    //Send Burst Read Start Cmd and Args
    IT8951MemBurstReadTrigger(ulMemAddr , ulReadSize);
          
    //Burst Read Fire
    IT8951MemBurstReadStart();
    
    #ifdef EN_SPI_2_I80
    
    //Burst Read Request for SPI interface only
    LCDReadNData(pusWord, ulReadSize);
    
    #else
    //Burst Read Data
    for(i=0;i<ulReadSize;i++)
    {
        pDestBuf[i] = LCDReadData();        
    }
    
    #endif

    //Send Burst End Cmd
    IT8951MemBurstEnd(); //the same with IT8951MemBurstEnd()

}



//-----------------------------------------------------------
//Host Cmd 10 - LD_IMG
//-----------------------------------------------------------
void IT8951LoadImgStart(IT8951LdImgInfo* pstLdImgInfo)
{
    TWord usArg;
    //Setting Argument for Load image start
    usArg = (pstLdImgInfo->usEndianType << 8 )
    |(pstLdImgInfo->usPixelFormat << 4)
    |(pstLdImgInfo->usRotate);
    //Send Cmd
    LCDWriteCmdCode(IT8951_TCON_LD_IMG);
    //Send Arg
    LCDWriteData(usArg);
}
//-----------------------------------------------------------
//Host Cmd 11 - LD_IMG_AREA
//-----------------------------------------------------------
void IT8951LoadImgAreaStart(IT8951LdImgInfo* pstLdImgInfo ,IT8951AreaImgInfo* pstAreaImgInfo)
{
    TWord usArg[5];
    //Setting Argument for Load image start
    usArg[0] = (pstLdImgInfo->usEndianType << 8 )
    |(pstLdImgInfo->usPixelFormat << 4)
    |(pstLdImgInfo->usRotate);
    usArg[1] = pstAreaImgInfo->usX;
    usArg[2] = pstAreaImgInfo->usY;
    usArg[3] = pstAreaImgInfo->usWidth;
    usArg[4] = pstAreaImgInfo->usHeight;
    //Send Cmd and Args
    LCDSendCmdArg(IT8951_TCON_LD_IMG_AREA , usArg , 5);
}
//-----------------------------------------------------------
//Host Cmd 12 - LD_IMG_END
//-----------------------------------------------------------
void IT8951LoadImgEnd(void)
{
    LCDWriteCmdCode(IT8951_TCON_LD_IMG_END);
}

//--------------------------------------------------
//3.5. Initial Functions
//--------------------------------------------------
//-----------------------------------------------------------
//Initial function - 1
//-----------------------------------------------------------
//Global varivale
I80IT8951DevInfo gstI80DevInfo;

void GetIT8951SystemInfo(void* pBuf)
{
    TWord* pusWord = (TWord*)pBuf;
    I80IT8951DevInfo* pstDevInfo;
    //Send I80 CMD
    LCDWriteCmdCode(USDEF_I80_CMD_GET_DEV_INFO);
    #ifdef EN_SPI_2_I80
    
    //Burst Read Request for SPI interface only
    LCDReadNData(pusWord, sizeof(I80IT8951DevInfo)/2);//Polling HRDY for each words(2-bytes) if possible
    
    #else
    //I80 interface - Single Read available
    for(i=0;i<sizeof(I80IT8951DevInfo)/2;i++)
    {
        pusWord[i] = LCDReadData();
    }
    
    #endif

    //Show Device information of IT8951
    pstDevInfo = (I80IT8951DevInfo*)pBuf;
    ShowMessage("Panel(W,H) = (%d,%d)\r\n",
    pstDevInfo->usPanelW, pstDevInfo->usPanelH );
    ShowMessage("Image Buffer Address = %X\r\n",
    pstDevInfo->usImgBufAddrL | (pstDevInfo->usImgBufAddrH << 16));
    //Show Firmware and LUT Version
    ShowMessage ("FW Version = %s\r\n", stI80IT8951DevInfo.usFWVersion);
    ShowMessage ("LUT Version = %s\r\n", stI80IT8951DevInfo.usLUTVersion);
}
//-----------------------------------------------------------
//Initial function 2 ¡V Set Image buffer base address
//-----------------------------------------------------------
void IT8951SetImgBufBaseAddr(TDWord ulImgBufAddr)
{
    TWord usWordH = (TWord)((ulImgBufAddr >> 16) & 0x0000FFFF);
    TWord usWordL = (TWord)( ulImgBufAddr & 0x0000FFFF);
    //Write LISAR Reg
    IT8951WriteReg(LISAR + 2 ,usWordH);
    IT8951WriteReg(LISAR ,usWordL);
}
//-----------------------------------------------------
// 3.6. Display Functions
//-----------------------------------------------------

//-----------------------------------------------------------
//Display function 1 - Wait for LUT Engine Finish
//                     Polling Display Engine Ready by LUTNo
//-----------------------------------------------------------
void IT8951WaitForDisplayReady()
{
    //Check IT8951 Register LUTAFSR => NonZero ¡V Busy, 0 - Free
    while(IT8951ReadReg(LUTAFSR));
}
//-----------------------------------------------------------
//Display function 2 ¡V Load Image Area process
//-----------------------------------------------------------
void IT8951HostAreaPackedPixelWrite(IT8951LdImgInfo* pstLdImgInfo,IT8951AreaImgInfo* pstAreaImgInfo)
{
    TDWord i,j;
    //Source buffer address of Host
    TWord* pusFrameBuf = (TWord*)pstLdImgInfo->ulStartFBAddr;
	
	#if 0
	//if width or height over than 2048 use memburst write instead, only allow 8bpp data 
    IT8951MemBurstWriteProc(pstLdImgInfo->ulImgBufBaseAddr,  pstAreaImgInfo->usWidth/2* pstAreaImgInfo->usHeight,   pusFrameBuf); //MemAddr, Size, Framebuffer address
	#else
    //Set Image buffer(IT8951) Base address
    IT8951SetImgBufBaseAddr(pstLdImgInfo->ulImgBufBaseAddr);
    //Send Load Image start Cmd
    IT8951LoadImgAreaStart(pstLdImgInfo , pstAreaImgInfo);
    //Host Write Data
    for(j=0;j< pstAreaImgInfo->usHeight;j++)
    {
    	#ifdef __SPI_2_I80_INF__  //{__SPI_2_I80_INF__
    	
            //Write 1 Line for each SPI transfer
            LCDWriteNData(pusFrameBuf, pstAreaImgInfo->usWidth/2);
            pusFrameBuf += pstAreaImgInfo->usWidth/2;//Change to Next line of loaded image (supposed the Continuous image content in hsot frame buffer )
        
        #else
        
        for(i=0;i< pstAreaImgInfo->usWidth/2;i++)
        {
            //Write a Word(2-Bytes) for each time
            LCDWriteData(*pusFrameBuf);
            pusFrameBuf++;
        }
        #endif//}__SPI_2_I80_INF__
    }
    //Send Load Img End Command
    IT8951LoadImgEnd();
	#endif
}
//-----------------------------------------------------------
//Display functions 3 - Application for Display panel Area
//-----------------------------------------------------------
Void IT8951DisplayArea(TWord usX, TWord usY, TWord usW, TWord usH, TWord usDpyMode)
{
    //Send I80 Display Command (User defined command of IT8951)
    LCDWriteCmd(USDEF_I80_CMD_DPY_AREA); //0x0034
    //Write arguments
    LCDWriteData(usX);
    LCDWriteData(usY);
    LCDWriteData(usW);
    LCDWriteData(usH);
    LCDWriteData(usDpyMode);
}


//Display Area with bitmap on EPD
//-----------------------------------------------------------
// Display Function 4 - for Display Area for 1-bpp mode format
//   the bitmap(1bpp) mode will be enable when Display
//   and restore to Default setting (disable) after displaying finished
//-----------------------------------------------------------
void IT8951DisplayArea1bpp(TWord usX, TWord usY, TWord usW, TWord usH, TWord usDpyMode, TByte ucBGGrayVal, TByte ucFGGrayVal)
{
    //Set Display mode to 1 bpp mode - Set 0x18001138 Bit[18](0x1800113A Bit[2])to 1
    IT8951WriteReg(UP1SR+2, IT8951ReadReg(UP1SR+2) | (1<<2));
    
    //Set BitMap color table 0 and 1 , => Set Register[0x18001250]:
    //Bit[7:0]: ForeGround Color(G0~G15)  for 1
    //Bit[15:8]:Background Color(G0~G15)  for 0
    IT8951WriteReg(BGVR, (ucBGGrayVal<<8) | ucFGGrayVal);
    
    //Display
    IT8951DisplayArea( usX, usY, usW, usH, usDpyMode);
    IT8951WaitForDisplayReady();
    
    //Restore to normal mode
    IT8951WriteReg(UP1SR+2, IT8951ReadReg(UP1SR+2) & ~(1<<2));
}


 	
//-------------------------------------------------------------------------------------------------------------
// 	Command - 0x0037 for Display Base addr by User 
//  TDWord ulDpyBufAddr - Host programmer need to indicate the Image buffer address of IT8951
//                                         In current case, there is only one image buffer in IT8951 so far.
//                                         So Please set the Image buffer address you got  in initial stage.
//                                         (gulImgBufAddr by Get device information 0x0302 command)
//
//-------------------------------------------------------------------------------------------------------------
void IT8951DisplayAreaBuf(TWord usX, TWord usY, TWord usW, TWord usH, TWord usDpyMode, TDWord ulDpyBufAddr)
{
    //Send I80 Display Command (User defined command of IT8951)
    LCDWriteCmdCode(USDEF_I80_CMD_DPY_BUF_AREA); //0x0037
    
    //Write arguments
    LCDWriteData(usX);
    LCDWriteData(usY);
    LCDWriteData(usW);
    LCDWriteData(usH);
    LCDWriteData(usDpyMode);
    LCDWriteData((TWord)ulDpyBufAddr);       //Display Buffer Base address[15:0]
    LCDWriteData((TWord)(ulDpyBufAddr>>16)); //Display Buffer Base address[26:16]
 
}


//----------------------------------------------------------------
//3.7. Test Functions
//----------------------------------------------------------------

//Global structures and variables
I80IT8951DevInfo gstI80DevInfo;
TByte* gpFrameBuf; //Host Source Frame buffer
TDWord gulImgBufAddr; //IT8951 Image buffer address

//-----------------------------------------------------------
//Test function 1 ¡VSoftware Initial flow for testing
//-----------------------------------------------------------
void HostInit()
{
    //Get Device Info
    GetIT8951SystemInfo(&gstI80DevInfo)
    //Host Frame Buffer allocation
    gpFrameBuf = malloc(gstI80DevInfo.usPanelW * gstI80DevInfo.usPanelH);
    //Get Image Buffer Address of IT8951
    gulImgBufAddr = gstI80DevInfo.usImgBufAddrL | (gstI80DevInfo.usImgBufAddrH << 16);
    
    //Set to Enable I80 Packed mode
    IT8951WriteReg(I80CPCR, 0x0001);
}
//-----------------------------------------------------------
//Test function 2 ¡V Example of Display Flow
//-----------------------------------------------------------
void IT8951DisplayExample()
{
    IT8951LdImgInfo stLdImgInfo;
    IT8951AreaImgInfo stAreaImgInfo;
    
    //Host Initial
    HostInit(); //Test Function 1
    //Prepare image
    //Write pixel 0xF0(White) to Frame Buffer
    memset(gpFrameBuf, 0xF0, gstI80DevInfo.usPanelW * gstI80DevInfo.usPanelH);
    
    //Check TCon is free ? Wait TCon Ready (optional)
    IT8951WaitForDisplayReady();
    
    //--------------------------------------------------------------------------------------------
    //      initial display - Display white only
    //--------------------------------------------------------------------------------------------
    //Load Image and Display
    //Setting Load image information
    stLdImgInfo.ulStartFBAddr    = (TDWord)gpFrameBuf;
    stLdImgInfo.usEndianType     = IT8951_LDIMG_L_ENDIAN;
    stLdImgInfo.usPixelFormat    = IT8951_8BPP;
    stLdImgInfo.usRotate         = IT8951_ROTATE_0;
    stLdImgInfo.ulImgBufBaseAddr = gulImgBufAddr;
    //Set Load Area
    stAreaImgInfo.usX      = 0;
    stAreaImgInfo.usY      = 0;
    stAreaImgInfo.usWidth  = gstI80DevInfo.usPanelW;
    stAreaImgInfo.usHeight = gstI80DevInfo.usPanelH;
    
    //Load Image from Host to IT8951 Image Buffer
    IT8951HostAreaPackedPixelWrite(&stLdImgInfo, &stAreaImgInfo);//Display function 2
    //Display Area ¡V (x,y,w,h) with mode 0 for initial White to clear Panel
    IT8951DisplayArea(0,0, gstI80DevInfo.usPanelW, gstI80DevInfo.usPanelH, 0);
    
    //--------------------------------------------------------------------------------------------
    //      Regular display - Display Any Gray colors with Mode 2 or others
    //--------------------------------------------------------------------------------------------
    //Preparing buffer to All black (8 bpp image)
    //or you can create your image pattern here..
    memset(gpFrameBuf, 0x00, gstI80DevInfo.usPanelW * gstI80DevInfo.usPanelH);
     
    IT8951WaitForDisplayReady();
    
    //Setting Load image information
    stLdImgInfo.ulStartFBAddr    = (TDWord)gpFrameBuf;
    stLdImgInfo.usEndianType     = IT8951_LDIMG_L_ENDIAN;
    stLdImgInfo.usPixelFormat    = IT8951_8BPP; 
    stLdImgInfo.usRotate         = IT8951_ROTATE_0;
    stLdImgInfo.ulImgBufBaseAddr = gulImgBufAddr;
    //Set Load Area
    stAreaImgInfo.usX      = 0;
    stAreaImgInfo.usY      = 0;
    stAreaImgInfo.usWidth  = gstI80DevInfo.usPanelW;
    stAreaImgInfo.usHeight = gstI80DevInfo.usPanelH;
    
    //Load Image from Host to IT8951 Image Buffer
    IT8951HostAreaPackedPixelWrite(&stLdImgInfo, &stAreaImgInfo);//Display function 2
    //Display Area ¡V (x,y,w,h) with mode 2 for fast gray clear mode - depends on current waveform 
    IT8951DisplayArea(0,0, gstI80DevInfo.usPanelW, gstI80DevInfo.usPanelH, 2);
    
    
}


//-----------------------------------------------------------
// Load 1bpp image flow (must display with IT8951DisplayArea1bpp()
//-----------------------------------------------------------

void IT8951Load1bppImage(TByte* p1bppImgBuf, TWord usX, TWord usY, TWord usW, TWord usH)
{
    //Setting Load image information
    stLdImgInfo.ulStartFBAddr    = (TDWord) p1bppImgBuf;
    stLdImgInfo.usEndianType     = IT8951_LDIMG_L_ENDIAN;
    stLdImgInfo.usPixelFormat    = IT8951_8BPP; //we use 8bpp because IT8951 dose not support 1bpp mode for load image¡Aso we use Load 8bpp mode ,but the transfer size needs to be reduced to Size/8
    stLdImgInfo.usRotate         = IT8951_ROTATE_0;
    stLdImgInfo.ulImgBufBaseAddr = gulImgBufAddr;
    //Set Load Area
    stAreaImgInfo.usX      = usX/8;
    stAreaImgInfo.usY      = usY;
    stAreaImgInfo.usWidth  = usW/8;//1bpp, Chaning Transfer size setting to 1/8X of 8bpp mode 
    stAreaImgInfo.usHeight = usH;
    Report("IT8951HostAreaPackedPixelWrite [wait]\n\r");
    //Load Image from Host to IT8951 Image Buffer
    IT8951HostAreaPackedPixelWrite(&stLdImgInfo, &stAreaImgInfo);//Display function 2
}
 	 
//-----------------------------------------------------------
//Test function 3 - Example of Display 1bpp Flow
//-----------------------------------------------------------
void IT8951Display1bppExample()
{
    IT8951AreaImgInfo stAreaImgInfo;
    
    //Host Initial
    HostInit(); //Test Function 1
    //Prepare image
    //Write pixel 0x00(Black) to Frame Buffer
    //or you can create your image pattern here..
    memset(gpFrameBuf, 0x00, (gstI80DevInfo.usPanelW * gstI80DevInfo.usPanelH)/8);//Host Frame Buffer(Source)
    
    //Check TCon is free ? Wait TCon Ready (optional)
    IT8951WaitForDisplayReady();
    
    //Load Image and Display
    //Set Load Area
    stAreaImgInfo.usX      = 0;
    stAreaImgInfo.usY      = 0;
    stAreaImgInfo.usWidth  = gstI80DevInfo.usPanelW;
    stAreaImgInfo.usHeight = gstI80DevInfo.usPanelH;
    //Load Image from Host to IT8951 Image Buffer
    IT8951Load1bppImage(gpFrameBuf, stAreaImgInfo.usX, stAreaImgInfo.usY, stAreaImgInfo.usW, stAreaImgInfo.usH);//Display function 4, Arg
    
    //Display Area - (x,y,w,h) with mode 2 for Gray Scale
    //e.g. if we want to set b0(Background color) for Black-0x00 , Set b1(Foreground) for White-0xFF
    IT8951DisplayArea1bpp(0,0, gstI80DevInfo.usPanelW, gstI80DevInfo.usPanelH, 0, 0x00, 0xFF);
    
}



