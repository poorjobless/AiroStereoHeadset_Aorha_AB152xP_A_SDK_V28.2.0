#include "reside_flash.inc"

#define _DEMOSOUND_C_
#include "rc.h"
#include "bt_config_profile.h"

#ifdef DEMOSOUND
#include "drv_sector.h"
#include "Driver_Interface.h"
#include "Driver.h"
#include "Driver_1520.h"
#include "Audio_MailBox.h"
#include "AudioControl.h"
#include "DemoSound.h"
#include "ExternalFlash_Sector.h"
#include "..\SECTOR\sector_a2dp_nvram_ctl.h"
#include "..\A2DP\a2dp_interface.h"
#include "..\A2DP\A2DP.h"
#include "..\AVDTP\AVDTP.h"

//#define LOOP_ONE_SONG_ONLY

#ifdef AIR_MODULE
	extern PUBLIC BOOL DRIVER_CheckA2DPOffReady(void);
	extern PUBLIC BOOL DRIVER_CheckAUXOffReady(void);
#endif

#ifdef TWS_SETTINGS
extern PUBLIC void DRIVER_TWSClkSetting(BOOL isRelay, U8 linkIndex);
#endif
extern PUBLIC void DRIVER_BackUpOgfAppCmd(DriverMsg XDATA_PTR msgPtr);

#define DEMOSOUND_STATE_NO_DATA			0
#define DEMOSOUND_STATE_DATA_OK			1
#define DEMOSOUND_STATE_WAIT_DSP_OPEN	2
#define DEMOSOUND_STATE_DSP_OPENED		3
#define DEMOSOUND_STATE_WAITING_CHANGE	4
#define DEMOSOUND_STATE_STREAM_STOPPED	5


typedef struct
{
	U8 songIndex;
	U32 frameIndex;
	U32 frameOffset;
	OST waitingTimer;
	DriverMsg XDATA_PTR waitingMsgPtr;
}SongCtlType;

typedef struct
{
	SongCtlType songCtl;
	
	U32	frameSector;
	U32	songDescSector;
	U8 songNum;
	
	SONG_DESCRIPTOR	currSongDesc;
}DemoSoundSongCtl;

typedef struct
{
	U8 state;
	DemoSoundSongCtl XDATA_PTR songCtlPtr;
}DemoSoundCtlType;

DemoSoundCtlType XDATA gDemoSound_ctl;

PRIVATE void DemoSound_LoadCurrentSongDesc(void)
{
	U32 flhPtr;
	
	if(!gDemoSound_ctl.songCtlPtr)
		return;
	LightDbgPrint("Demo LoadSong:%d",(U8)gDemoSound_ctl.songCtlPtr->songCtl.songIndex);
	flhPtr = gDemoSound_ctl.songCtlPtr->songDescSector + sizeof(U8) + sizeof(SONG_DESCRIPTOR)* gDemoSound_ctl.songCtlPtr->songCtl.songIndex;
	DRV_SPIFLH_ReadBytes(flhPtr, (U8 XDATA_PTR)&gDemoSound_ctl.songCtlPtr->currSongDesc, sizeof(SONG_DESCRIPTOR));
	gDemoSound_ctl.songCtlPtr->currSongDesc.startAddress += gDemoSound_ctl.songCtlPtr->frameSector;
	
	#if SUPPORT_AAC_SNK
	if(gDemoSound_ctl.songCtlPtr->currSongDesc.codecType == EXT_FLASH_SONG_CODEC_AAC)
	{
		U16 LDATA numOfFrame;
		flhPtr = gDemoSound_ctl.songCtlPtr->currSongDesc.startAddress;
		DRV_SPIFLH_ReadBytes(flhPtr, (U8 XDATA_PTR)&numOfFrame, sizeof(U16));
		gDemoSound_ctl.songCtlPtr->currSongDesc.numOfFrame = numOfFrame;
		gDemoSound_ctl.songCtlPtr->currSongDesc.startAddress += sizeof(U16);
	}
	#endif
}

PRIVATE U8 DemoSound_Enable(DriverMsg XDATA_PTR msgPtr)
{
	if(!DemoSound_IsAvailable())
		return MEMORY_PUT;

	if(gDemoSound_ctl.state == DEMOSOUND_STATE_DATA_OK)
	{
		DemoSound_LoadCurrentSongDesc();
		gDemoSound_ctl.state = DEMOSOUND_STATE_WAIT_DSP_OPEN;
		
		if(!gDemoSound_ctl.songCtlPtr->songCtl.waitingMsgPtr)
			gDemoSound_ctl.songCtlPtr->songCtl.waitingMsgPtr = (DriverMsg XDATA_PTR)OSMEM_Get(OSMEM_ptr1);
			
		OSMEM_memcpy_xdata_code(gDemoSound_ctl.songCtlPtr->songCtl.waitingMsgPtr, msgPtr, OSMEM1_BLKSIZE);
	}
	else if(gDemoSound_ctl.state == DEMOSOUND_STATE_WAITING_CHANGE)
	{
		gDemoSound_ctl.state = DEMOSOUND_STATE_WAIT_DSP_OPEN;
		OSMEM_memcpy_xdata_code(gDemoSound_ctl.songCtlPtr->songCtl.waitingMsgPtr, msgPtr, OSMEM1_BLKSIZE);
	}
	
	if(gDemoSound_ctl.state == DEMOSOUND_STATE_WAIT_DSP_OPEN)
	{
		U8 codecType;
		
		if(IS_DSP_BUSY)
			return QUE_PUTFRONT;
			
		#ifdef FM_ENABLE
		if(AUDIO_COMPONENT_IS_ACTIVE(AUDIO_FM))
		{
			DRIVER_SendRequestCloseFMEvent();
			return QUE_PUTFRONT;
		}
		#endif

		if(DRIVER_IsRingToneOrVPOrATPlaying())
		{
			return QUE_PUTFRONT;
		}
		#ifdef SUPPORT_VOICE_COMMAND
		if(AUDIO_COMPONENT_IS_ACTIVE(AUDIO_VOICECOMMAND))
		{
			return QUE_PUTFRONT;
		}
		#endif
		if(AUDIO_COMPONENT_IS_ACTIVE(AUDIO_LINE_IN) || AUDIO_COMPONENT_IS_ACTIVE(AUDIO_SCO) || AUDIO_COMPONENT_IS_ACTIVE(AUDIO_ASYNC_SCO))
		{
			#ifdef AIR_MODULE
			if(!DRIVER_CheckAUXOffReady())
				return QUE_PUTFRONT;
			#endif
			DRIVER_SendAppIdleToMailBox();
			return QUE_PUTFRONT;
		}

		codecType = (gDemoSound_ctl.songCtlPtr->currSongDesc.codecType == EXT_FLASH_SONG_CODEC_SBC)?SBC_SNK_SEID:AAC_SNK_SEID;
		
		#if !SUPPORT_AAC_SNK
		if(codecType == AAC_SNK_SEID)
			return MEMORY_PUT;
		#endif
		if(AUDIO_COMPONENT_IS_ACTIVE (AUDIO_A2DP))
		{
			if(gDriver_ctl.dspLink == DEMOSOUND_LINKINDEX && gDriver_ctl.musicCodecType == codecType)
			{
				gDemoSound_ctl.state = DEMOSOUND_STATE_DSP_OPENED;
				return MEMORY_PUT;
			}
			else
			{
				#ifdef AIR_MODULE
				if(!DRIVER_CheckA2DPOffReady())
					return QUE_PUTFRONT;
				#endif

				DRIVER_SendAppIdleToMailBox();
				return QUE_PUTFRONT;
			}
		}

		gDriver_ctl.dspLink = DEMOSOUND_LINKINDEX;
		gDriver_ctl.musicCodecType = codecType;
		msgPtr->msgBodyPtr.driverCmd.a2dpCmd.m2d_ctl.para.stereo_start_para.codec_type = codecType;
		msgPtr->msgBodyPtr.driverCmd.a2dpCmd.m2d_ctl.para.stereo_start_para.tws_mode = msgPtr->msgBodyPtr.driverCmd.a2dpCmd.enablePara.a2dpPara.twsMode;

		#ifdef TWS_SETTINGS
		if(msgPtr->msgBodyPtr.driverCmd.a2dpCmd.enablePara.a2dpPara.twsMode == A2DP_TWS_MODE_RELAYER)
			DRIVER_TWSClkSetting(TRUE, gDriver_ctl.dspLink);
		else if(msgPtr->msgBodyPtr.driverCmd.a2dpCmd.enablePara.a2dpPara.twsMode == A2DP_TWS_MODE_FOLLOWER)
			DRIVER_TWSClkSetting(FALSE, gDriver_ctl.dspLink);
		#endif

		DRIVER_BackUpOgfAppCmd(msgPtr);
		AUDIO_SetAudioOn(AUDIO_A2DP);
		Mailbox_TaskOgfAPP(msgPtr, OCF_APP_STEREO_MODE);
		AUDIO_SetAudioInOutToMailBoxCmd(AUDIO_A2DP, &msgPtr->msgBodyPtr.driverCmd.a2dpCmd.m2d_ctl.para.stereo_start_para.audio_scenario);
		DRIVER_SendCmdToMailBox(msgPtr);
		gOS_PowerSavingMCUPauseRequest = TRUE;
		DRIVER_SetPowerSaving(FALSE, DRIVER_POWER_SAVING_DSP_A2DP);
		#ifdef PEQ_ENABLE
		DRIVER_PEQReload();
		#endif
		gDemoSound_ctl.state = DEMOSOUND_STATE_DSP_OPENED;
		A2DP_MediaStart();
		return RETURN_FUNCTION;
	}
	return MEMORY_PUT;
}

PRIVATE BOOL DemoSound_Disable(void)
{
	if(AUDIO_COMPONENT_IS_ACTIVE (AUDIO_A2DP))
	{
		U8 codecType = (gDemoSound_ctl.songCtlPtr->currSongDesc.codecType == EXT_FLASH_SONG_CODEC_SBC)? SBC_SNK_SEID : AAC_SNK_SEID;
		if(gDriver_ctl.dspLink == DEMOSOUND_LINKINDEX && gDriver_ctl.musicCodecType == codecType)
		{
			if(IS_DSP_BUSY)
				return FALSE;
			DRIVER_SendAppIdleToMailBox();
		}
	}
	return TRUE;
}

PRIVATE void DemoSound_PktHandler(void)
{
	U8 relayLink;
	
	if(gDemoSound_ctl.state != DEMOSOUND_STATE_DSP_OPENED)
		return;
	
	relayLink = A2DP_SearchRelayerLink();
	if(relayLink != MAX_MULTI_POINT_NO && (A2DP_GetLinkInfo(relayLink)->mediaChannelConnected) && A2DP_GetLinkInfo(relayLink)->mmiProfileInfo.state != A2DP_STREAMING)
	{
		return;
	}

	while(OSMQ_Entries(OSMQ_DSP_L2CAP_Received_PDU_ptr) < 2)
	{
		U16 l2capLen;
		U32 flhPtr;
		U8 XDATA_PTR pktPtr;
		
		if(gDemoSound_ctl.songCtlPtr->songCtl.frameIndex >= gDemoSound_ctl.songCtlPtr->currSongDesc.numOfFrame)
		{
			if(gDemoSound_ctl.songCtlPtr->songCtl.waitingMsgPtr)
			{
				#ifdef LOOP_ONE_SONG_ONLY
				gDemoSound_ctl.songCtlPtr->songCtl.waitingMsgPtr->msgOpcode = DRIVER_DEMOSOUND_BACKWARD_CMD;
				#else
				gDemoSound_ctl.songCtlPtr->songCtl.waitingMsgPtr->msgOpcode = DRIVER_DEMOSOUND_FORWARD_CMD;
				#endif
				OSMQ_Put(OSMQ_DRIVER_Command_ptr, (U8 XDATA_PTR)gDemoSound_ctl.songCtlPtr->songCtl.waitingMsgPtr);
				gDemoSound_ctl.songCtlPtr->songCtl.waitingMsgPtr = NULL;
			}
			break;
		}
		
		if((pktPtr = OSMEM_Get(OSMEM_ptr2_small_RX)) == (U8 XDATA_PTR)NULL)
			break;

		if(gDemoSound_ctl.songCtlPtr->currSongDesc.codecType == EXT_FLASH_SONG_CODEC_SBC)
		{
			U8 frameCnt;
			A2dpSBCRxMediaPktType XDATA_PTR mediaPtr = (A2dpSBCRxMediaPktType XDATA_PTR)&pktPtr[6];
			OSMEM_memset_xdata((U8 XDATA_PTR)mediaPtr, 0, sizeof(A2dpSBCRxMediaPktType));
			
			flhPtr = gDemoSound_ctl.songCtlPtr->currSongDesc.startAddress + gDemoSound_ctl.songCtlPtr->currSongDesc.frameSize * gDemoSound_ctl.songCtlPtr->songCtl.frameIndex;
			
			frameCnt = (MIN(OSMEM2_SMALL_RX_BLKSIZE,A2DP_MEDIA_MTU_SIZE) - (6 + sizeof(L2CAP_DATA_HDR_STRU) + sizeof(A2DPSBCRxRTPHeaderType) + sizeof(SBCPayloadInfoType))) / gDemoSound_ctl.songCtlPtr->currSongDesc.frameSize;
			if((frameCnt + gDemoSound_ctl.songCtlPtr->songCtl.frameIndex) >= gDemoSound_ctl.songCtlPtr->currSongDesc.numOfFrame)
			{
				frameCnt = gDemoSound_ctl.songCtlPtr->currSongDesc.numOfFrame - gDemoSound_ctl.songCtlPtr->songCtl.frameIndex;
			}
			
			if( frameCnt > 0x0E)
				frameCnt = 0x0E;
			
			DRV_SPIFLH_ReadBytes(flhPtr, (U8 XDATA_PTR)&mediaPtr->PLHeader.payload.frameHeader, frameCnt*gDemoSound_ctl.songCtlPtr->currSongDesc.frameSize);
			
			mediaPtr->PLHeader.payload.payloadInfo.numOfFrames = frameCnt;
			
			gDemoSound_ctl.songCtlPtr->songCtl.frameIndex += frameCnt; 
			l2capLen = sizeof(A2DPSBCRxRTPHeaderType) + sizeof(SBCPayloadInfoType) + frameCnt*gDemoSound_ctl.songCtlPtr->currSongDesc.frameSize;
			
			pktPtr[0] = 6; //offset
			pktPtr[4] = (U8)(l2capLen+4);
			pktPtr[5] = (U8)((l2capLen + 4)>>8);		
			pktPtr[6] = (U8)l2capLen;
			pktPtr[7] = (U8)(l2capLen>>8);
			A2DP_SBCDemoSoundMediaPacket(pktPtr);
		}
		#if SUPPORT_AAC_SNK
		else if(gDemoSound_ctl.songCtlPtr->currSongDesc.codecType == EXT_FLASH_SONG_CODEC_AAC)
		{
			U16 LDATA frameLen;
			A2dpAACRxMediaPktType XDATA_PTR mediaPtr = (A2dpAACRxMediaPktType XDATA_PTR)&pktPtr[6];
			OSMEM_memset_xdata((U8 XDATA_PTR)mediaPtr, 0, sizeof(A2dpAACRxMediaPktType));
			
			flhPtr = gDemoSound_ctl.songCtlPtr->currSongDesc.startAddress + gDemoSound_ctl.songCtlPtr->songCtl.frameOffset +1;
			DRV_SPIFLH_ReadBytes(flhPtr, (U8 XDATA_PTR)&frameLen, sizeof(U16));
			flhPtr += sizeof(U16);
			frameLen &= 0x1FFF;
			
			gDemoSound_ctl.songCtlPtr->songCtl.frameOffset += (1 + sizeof(U16) + frameLen);
			gDemoSound_ctl.songCtlPtr->songCtl.frameIndex ++; 

			if(frameLen > OSMEM2_SMALL_RX_BLKSIZE)
			{
				OSMEM_Put(pktPtr);
				break;			
			}
			DRV_SPIFLH_ReadBytes(flhPtr, (U8 XDATA_PTR)&mediaPtr->PLHeader, frameLen);
			l2capLen = frameLen + sizeof(StandardRTPHeaderType);
			
			pktPtr[0] = 6; //offset
			pktPtr[4] = (U8)(l2capLen+4);
			pktPtr[5] = (U8)((l2capLen + 4)>>8);		
			pktPtr[6] = (U8)l2capLen;
			pktPtr[7] = (U8)(l2capLen>>8);
			OSMQ_MCU_DSP_Put(OSMQ_DSP_L2CAP_Received_PDU_ptr,pktPtr);
		}
		#endif
		else
		{
			OSMEM_Put(pktPtr);
			break;
		}
	}
}

PRIVATE void DemoSound_CheckTimer(void)
{
	if(gDemoSound_ctl.state == DEMOSOUND_STATE_WAITING_CHANGE)
	{
		if(OST_TimerExpired(&gDemoSound_ctl.songCtlPtr->songCtl.waitingTimer))
		{
			if(DemoSound_Disable())
			{
				gDemoSound_ctl.state = DEMOSOUND_STATE_DATA_OK;
				gDemoSound_ctl.songCtlPtr->songCtl.waitingMsgPtr->msgOpcode = DRIVER_DEMOSOUND_PLAY_CMD;
				OSMQ_Put(OSMQ_DRIVER_Command_ptr, (U8 XDATA_PTR)gDemoSound_ctl.songCtlPtr->songCtl.waitingMsgPtr);
				gDemoSound_ctl.songCtlPtr->songCtl.waitingMsgPtr = NULL;
			}
			else
			{
				OST_SetTimer(&gDemoSound_ctl.songCtlPtr->songCtl.waitingTimer, 80L);
			}			
		}
	}
	else if(gDemoSound_ctl.state == DEMOSOUND_STATE_STREAM_STOPPED)
	{
		if(OST_TimerExpired(&gDemoSound_ctl.songCtlPtr->songCtl.waitingTimer))
		{
			if(DemoSound_Disable())
			{
				gDemoSound_ctl.state = DEMOSOUND_STATE_DATA_OK;
			}
			else
			{
				OST_SetTimer(&gDemoSound_ctl.songCtlPtr->songCtl.waitingTimer, 320L);
			}
		}	
	}
}

PRIVATE U8 DemoSound_SetWaitingForChange(DriverMsg XDATA_PTR msgPtr)
{
	OST_SetTimer(&gDemoSound_ctl.songCtlPtr->songCtl.waitingTimer, HALF_SEC);
	gDemoSound_ctl.state = DEMOSOUND_STATE_WAITING_CHANGE;
	if(gDemoSound_ctl.songCtlPtr->songCtl.waitingMsgPtr)
	{
		OSMEM_memcpy_xdata_code(gDemoSound_ctl.songCtlPtr->songCtl.waitingMsgPtr, msgPtr, OSMEM1_BLKSIZE);
	}
	else
	{
		gDemoSound_ctl.songCtlPtr->songCtl.waitingMsgPtr = msgPtr;
		return RETURN_FUNCTION;
	}
	return MEMORY_PUT;
}

PRIVATE void DemoSound_SetStreamStopped(void)
{
	gDemoSound_ctl.state = DEMOSOUND_STATE_STREAM_STOPPED;
	OST_SetTimer(&gDemoSound_ctl.songCtlPtr->songCtl.waitingTimer, 0);
	SYS_MemoryRelease(&gDemoSound_ctl.songCtlPtr->songCtl.waitingMsgPtr);
}

PUBLIC void DemoSound_Init(void)
{
	U8 LDATA numOfSector;
	U8 XDATA_PTR headerPtr;
	U32 flhPtr;
	BOOL isDataReady;

	if(!DRV_SPIFLH_IsExtFlhExist())
		return;
		
	if(DRV_SPIFLH_ReadBytes(0, &numOfSector, 1) || numOfSector == 0xFF)
		return;
		
	if((gDemoSound_ctl.songCtlPtr = (DemoSoundSongCtl XDATA_PTR)OSMEM_Get(OSMEM_ptr1)) == (DemoSoundSongCtl XDATA_PTR)NULL)
		return;
		
	flhPtr = 1;
	isDataReady = FALSE;
	headerPtr = OSMEM_Get(OSMEM_ptr1);
	
	while(numOfSector--)
	{
		DRV_SPIFLH_ReadBytes(flhPtr, headerPtr, sizeof(SECTOR_SCRIPT));

		if(((SECTOR_SCRIPT XDATA_PTR)headerPtr)->sectorID == EXTERNAL_FLASH_SECTOR_FRAME_ID)
		{
			gDemoSound_ctl.songCtlPtr->frameSector = ((SECTOR_SCRIPT XDATA_PTR)headerPtr)->sectorAddress;
			isDataReady = TRUE;
		}
		else if(((SECTOR_SCRIPT XDATA_PTR)headerPtr)->sectorID == EXTERNAL_FLASH_SECTOR_SONG_DECSRPITOR_ID)
		{
			gDemoSound_ctl.songCtlPtr->songDescSector = ((SECTOR_SCRIPT XDATA_PTR)headerPtr)->sectorAddress;
			isDataReady = TRUE;
		}
		flhPtr += sizeof(SECTOR_SCRIPT);
	}
	OSMEM_Put(headerPtr);

	if(isDataReady)
	{
		flhPtr = gDemoSound_ctl.songCtlPtr->songDescSector;
		DRV_SPIFLH_ReadBytes(flhPtr, &gDemoSound_ctl.songCtlPtr->songNum, sizeof(U8));
		gDemoSound_ctl.state = DEMOSOUND_STATE_DATA_OK;
		LightDbgPrint("ExtFlh SongNum:%d",(U8)gDemoSound_ctl.songCtlPtr->songNum);
		LightDbgPrint("ExtFlh SongDesc:%D",(U32)gDemoSound_ctl.songCtlPtr->songDescSector);
		LightDbgPrint("ExtFlh Frame:%D",(U32)gDemoSound_ctl.songCtlPtr->frameSector);
	}
	else
	{
		gDemoSound_ctl.state = DEMOSOUND_STATE_NO_DATA;
		SYS_MemoryRelease((U8 XDATA_PTR XDATA_PTR)&gDemoSound_ctl.songCtlPtr);
	}
}

PUBLIC BOOL DemoSound_IsAvailable(void)
{
	return (gDemoSound_ctl.state >= DEMOSOUND_STATE_DATA_OK)?TRUE:FALSE;
}


PUBLIC void DemoSound_Polling(void)
{
	DemoSound_CheckTimer();
	DemoSound_PktHandler();
}

PUBLIC U8 DemoSound_ChangeSongForward(DriverMsg XDATA_PTR msgPtr)
{
	if(DemoSound_IsAvailable())
	{
		gDemoSound_ctl.songCtlPtr->songCtl.songIndex++;
		gDemoSound_ctl.songCtlPtr->songCtl.songIndex %= gDemoSound_ctl.songCtlPtr->songNum;
		gDemoSound_ctl.songCtlPtr->songCtl.frameIndex = 0;
		gDemoSound_ctl.songCtlPtr->songCtl.frameOffset = 0;
		
		DemoSound_LoadCurrentSongDesc();
		switch(gDemoSound_ctl.state)
		{
			case DEMOSOUND_STATE_DSP_OPENED:
			case DEMOSOUND_STATE_WAITING_CHANGE:
			case DEMOSOUND_STATE_WAIT_DSP_OPEN:
				return DemoSound_SetWaitingForChange(msgPtr);

			case DEMOSOUND_STATE_NO_DATA:
			case DEMOSOUND_STATE_DATA_OK:
			case DEMOSOUND_STATE_STREAM_STOPPED:
			default:
				break;
		}
	}
	return MEMORY_PUT;
}

PUBLIC U8 DemoSound_ChangeSongBackward(DriverMsg XDATA_PTR msgPtr)
{	
	if(DemoSound_IsAvailable())
	{	
		if(gDemoSound_ctl.songCtlPtr->songCtl.frameIndex)
		{
			gDemoSound_ctl.songCtlPtr->songCtl.frameIndex = 0;
			gDemoSound_ctl.songCtlPtr->songCtl.frameOffset = 0;
		}
		else
		{
			if(gDemoSound_ctl.songCtlPtr->songCtl.songIndex)
				gDemoSound_ctl.songCtlPtr->songCtl.songIndex--;
			else
				gDemoSound_ctl.songCtlPtr->songCtl.songIndex = gDemoSound_ctl.songCtlPtr->songNum - 1;
				
			DemoSound_LoadCurrentSongDesc();
		}
		switch(gDemoSound_ctl.state)
		{
			case DEMOSOUND_STATE_DSP_OPENED:
			case DEMOSOUND_STATE_WAITING_CHANGE:
			case DEMOSOUND_STATE_WAIT_DSP_OPEN:
				return DemoSound_SetWaitingForChange(msgPtr);

			case DEMOSOUND_STATE_NO_DATA:
			case DEMOSOUND_STATE_DATA_OK:
			case DEMOSOUND_STATE_STREAM_STOPPED:
			default:
				break;
		}
	}
	return MEMORY_PUT;
}

PUBLIC U8 DemoSound_Stop(DriverMsg XDATA_PTR msgPtr)
{	
	gDemoSound_ctl.songCtlPtr->songCtl.frameIndex = 0;
	gDemoSound_ctl.songCtlPtr->songCtl.frameOffset = 0;
	switch(gDemoSound_ctl.state)
	{
		case DEMOSOUND_STATE_DSP_OPENED:
		case DEMOSOUND_STATE_WAITING_CHANGE:
			DemoSound_SetStreamStopped();
			break;
		case DEMOSOUND_STATE_WAIT_DSP_OPEN:
			gDemoSound_ctl.state = DEMOSOUND_STATE_DATA_OK;
			break;
		case DEMOSOUND_STATE_NO_DATA:
		case DEMOSOUND_STATE_DATA_OK:
		case DEMOSOUND_STATE_STREAM_STOPPED:
		default:
			break;
	}
	UNUSED(msgPtr);
	return MEMORY_PUT;
}

PUBLIC U8 DemoSound_Pause(DriverMsg XDATA_PTR msgPtr)
{
	switch(gDemoSound_ctl.state)
	{
		case DEMOSOUND_STATE_DSP_OPENED:
		case DEMOSOUND_STATE_WAITING_CHANGE:
			DemoSound_SetStreamStopped();
			break;
		case DEMOSOUND_STATE_WAIT_DSP_OPEN:
			gDemoSound_ctl.state = DEMOSOUND_STATE_DATA_OK;
			break;
		case DEMOSOUND_STATE_NO_DATA:
		case DEMOSOUND_STATE_DATA_OK:
		case DEMOSOUND_STATE_STREAM_STOPPED:
		default:
			break;
	}
	UNUSED(msgPtr);
	return MEMORY_PUT;
}

PUBLIC U8 DemoSound_Play(DriverMsg XDATA_PTR msgPtr)
{
	if(DemoSound_IsAvailable())
	{	
		switch(gDemoSound_ctl.state)
		{
			case DEMOSOUND_STATE_DATA_OK:
			case DEMOSOUND_STATE_WAIT_DSP_OPEN:
				return DemoSound_Enable(msgPtr);
				break;
			case DEMOSOUND_STATE_WAITING_CHANGE:
				return DemoSound_SetWaitingForChange(msgPtr);
				break;
			case DEMOSOUND_STATE_STREAM_STOPPED:
				gDemoSound_ctl.state = DEMOSOUND_STATE_DSP_OPENED;
				break;
			case DEMOSOUND_STATE_NO_DATA:
			case DEMOSOUND_STATE_DSP_OPENED:
			default:
				break;
		}
	}
	return MEMORY_PUT;
}
#endif