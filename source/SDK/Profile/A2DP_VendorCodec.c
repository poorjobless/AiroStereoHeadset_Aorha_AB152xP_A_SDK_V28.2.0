#include "reside_flash.inc"

#include "rc.h"
#include "..\SDK_Proxy\SDK_Proxy.h"
#include "..\SDK\include\SDK_Config.h"

#if SDK_VENDOR_CODEC


#include "..\SECTOR\sector_a2dp_nvram_ctl.h"
#include "..\MMI\MMI_A2DP.h"
#include "..\A2DP\A2DP_Interface.h"

#if CODEC_AAC_DEMO
	#include "..\A2DP\A2DP.h"
	#include "align_flash.h"
	extern void A2DP_DBG_DisplayMediaPktMessage(U8 linkIndex);
#endif


enum
{
	#if CODEC_AAC_DEMO
		VEDNOR_CODEC_DEMO_AAC,
	#endif
	#if CODEC_HIGH_QUALITY
		VEDNOR_CODEC_HIGH_QUALITY,
	#endif
	VEDNOR_CODEC_NUM,
};

#if CODEC_AAC_DEMO
#define AAC_DEMO_SEID	2
#endif


#define VENDOR_SNK_SEID_HIGH_QUALITY	11	//(SEID number: 0 - 3 and 0x3E are reserved for system use.)
#define VENDOR_CODEC_ID_HIGH_QUALITY	0x0011


AcpStreamEndPointIdCtl CODE vendorCodecSep[VEDNOR_CODEC_NUM] =
{
	#if CODEC_AAC_DEMO
	{
		0, 					//U8 rfa2:1;
		0, 					//U8 inUse:1;
		AAC_DEMO_SEID,	//U8 firstAcpSEID:6;
		0,					//U8 rfa3:3;
		A2DP_SEP_SNK,		//U8 tsep:1;
		A2DP_AUDIO,			//U8 mediaType:4;
	},	
	#endif
	#if CODEC_HIGH_QUALITY
	{
		0, 					//U8 rfa2:1;
		0, 					//U8 inUse:1;
		VENDOR_SNK_SEID_HIGH_QUALITY,	//U8 firstAcpSEID:6;
		0,					//U8 rfa3:3;
		A2DP_SEP_SNK,		//U8 tsep:1;
		A2DP_AUDIO,			//U8 mediaType:4;
	},
	#endif
};

PUBLIC U8 SDK_A2DP_VC_DiscoverCmd(U8 linkIndex, U8 XDATA_PTR seidPtr)
{
	U8 sep_num, i;

	for (sep_num = 0, i = 0; i < VEDNOR_CODEC_NUM; i++)
	{
		if (MMI_A2DP_IsHighQualityCodecDisbaled())
		{
			if (vendorCodecSep[i].firstAcpSEID == VENDOR_SNK_SEID_HIGH_QUALITY)
				continue;	//skip
		}

		OSMEM_memcpy_xdata_code(seidPtr, &vendorCodecSep[i], sizeof(AcpStreamEndPointIdCtl));
		if ( A2DP_CheckSeidInUse(linkIndex, ((AcpStreamEndPointIdCtl XDATA_PTR)seidPtr)->firstAcpSEID) )
		{
			((AcpStreamEndPointIdCtl XDATA_PTR)seidPtr)->inUse = 1;	//inUse bit
		}
		seidPtr += sizeof(AcpStreamEndPointIdCtl);
		sep_num++;
	}
	return sep_num;
}

#if CODEC_AAC_DEMO
	typedef struct
	{
		U8 objectType;
		U8 sampleRate1;
		U8 sampleRate2;
		U8 bitRate1;
		U8 bitRate2;
		U8 bitRate3;
	}AACDemoCapabilities;

	#define AAC_DEMO_LENGTH 	(2 + 4 + (4 + sizeof(AACDemoCapabilities))+ 2)
	
	U8 CODE aacDemoCapability[AAC_DEMO_LENGTH] =
	{ //aacServiceCapabilities
		MEDIA_TRANSPORT, 0x00, // media transport : 0x01, length is 0x00
		CONTENT_PROTECTION, 02,	// content protection: 0x04
		(U8)(A2DP_CP_SCMS_T), (U8)(A2DP_CP_SCMS_T >> 8),
		MEDIA_CODEC , 0x08,
		AUDIO_TYPE,
		AAC_CODEC,
		A2DP_AAC_OBJECT_TYPE_MPEG2_AAC_LC,  //ObjectType
		A2DP_AAC_SAMPLE_FREQ_44100|A2DP_AAC_SAMPLE_FREQ_32000|A2DP_AAC_SAMPLE_FREQ_24000|A2DP_AAC_SAMPLE_FREQ_22050|A2DP_AAC_SAMPLE_FREQ_16000|A2DP_AAC_SAMPLE_FREQ_12000|A2DP_AAC_SAMPLE_FREQ_11025|A2DP_AAC_SAMPLE_FREQ_8000,  // Sample Freq
		A2DP_AAC_SAMPLE_FREQ_48000|A2DP_AAC_CHANNELS_1|A2DP_AAC_CHANNELS_2,// Sample Freq + Channels
		A2DP_AAC_VARIABLE_BIT_RATE|((U8)(A2DP_AAC_BIT_RATE>>16)),
		(U8)(A2DP_AAC_BIT_RATE>>8),
		(U8)A2DP_AAC_BIT_RATE,
		DELAY_REPORTING, 0x00,	// delay reporting
	};
#endif

#if CODEC_HIGH_QUALITY
	// delay report is manditory item
	#define VC_HQ_CODEC_ELEMENT_LENGTH	1      // Note  VC_HQ_CODEC_ELEMENT_LENGTH should be less than 6

	typedef struct
	{
		U8 vendorID[4];
		U16 vendorCodecID;
		U8 Content[VC_HQ_CODEC_ELEMENT_LENGTH];	//Media Transport + Content Protection + Media Codec + Delay Reporting(Must)
	}VCHQServiceCapabilities;
	
	#define VEDNOR_CODEC_HQ_CAPABILITY_LENGTH 	(2 + 4 + (4 + sizeof(VCHQServiceCapabilities))+ 2)

	U8 CODE vendorHQCapability[VEDNOR_CODEC_HQ_CAPABILITY_LENGTH] =
	{
		MEDIA_TRANSPORT, 0x00,  // media transport : 0x01, length is 0x00
		CONTENT_PROTECTION, 02,	// content protection: 0x04
		(U8)(A2DP_CP_SCMS_T), (U8)(A2DP_CP_SCMS_T >> 8),
		MEDIA_CODEC, 0x09, 		// media codec  : 0x07, length is 0x09
		AUDIO_TYPE, 			// Audio type
		NON_A2DP_CODEC, 		// Codec type
		0x94, 0x00, 0x00, 0x00,	// vendorID; //Airoha Comany assign number
		VENDOR_CODEC_ID_HIGH_QUALITY,	// U16 vendorCodecID; Here we set codec ID the same as SEID.
		0, 						//	Media Codec Specific Information Elements[VC_HQ_CODEC_ELEMENT_LENGTH]
		DELAY_REPORTING, 0x00,	// delay reporting
	};
#endif

PUBLIC U8 SDK_A2DP_VC_GetCapabilityCmd(U8 seid, U8 XDATA_PTR capaPtr)
{
#if CODEC_AAC_DEMO
	if( seid == AAC_DEMO_SEID)
	{
		OSMEM_memcpy_xdata_code((U8 XDATA_PTR)capaPtr, &aacDemoCapability, AAC_DEMO_LENGTH);
		return AAC_DEMO_LENGTH;
	}
#endif
#if CODEC_HIGH_QUALITY
	if( seid == VENDOR_SNK_SEID_HIGH_QUALITY)
	{
		OSMEM_memcpy_xdata_code((U8 XDATA_PTR)capaPtr, &vendorHQCapability, VEDNOR_CODEC_HQ_CAPABILITY_LENGTH);
		return VEDNOR_CODEC_HQ_CAPABILITY_LENGTH;
	}
#endif
	return 0;
}

#if CODEC_AAC_DEMO
PUBLIC BOOL SDK_A2DP_VC_AACDemoRxMediaPacket(U8 linkIndex, U8 XDATA_PTR ptr1)
{
	A2dpSBCRxMediaPktType XDATA_PTR rxPktPtr = (A2dpSBCRxMediaPktType XDATA_PTR)(ptr1 + *ptr1);
	A2DP_LINK_INFO XDATA_PTR a2dpLinkInfo = A2DP_GetLinkInfo(linkIndex);

	if(a2dpLinkInfo->enableSCMS)
	{
		rxPktPtr->L2capHeader.cid = ENDIAN_TRANSFORM_U16(A2DP_CONTENT_PROTECTION_ID);
	}
	OSMQ_MCU_DSP_Put(OSMQ_DSP_L2CAP_Received_PDU_ptr,ptr1);

	A2DP_DBG_DisplayMediaPktMessage(linkIndex);

	return FALSE;
}
#endif

#if CODEC_HIGH_QUALITY
PUBLIC BOOL SDK_A2DP_VC_HQMediaDecoder(U8 linkIndex, U8 XDATA_PTR ptr1)
{
	OSMEM_Put(ptr1);
	UNUSED(linkIndex);
	return FALSE;
}
#endif

PUBLIC VFUN SDK_A2DP_VC_GetMediaCodecFunc(U8 seid)
{
#if CODEC_AAC_DEMO
	if( seid == AAC_DEMO_SEID)
	{
		return (VFUN)SDK_A2DP_VC_AACDemoRxMediaPacket;
	}
#endif
#if CODEC_HIGH_QUALITY
	if( seid == VENDOR_SNK_SEID_HIGH_QUALITY)
	{
		return (VFUN)SDK_A2DP_VC_HQMediaDecoder;
	}
#endif
	return (VFUN)FAR_NULL;
}

PUBLIC U8 SDK_A2DP_VC_SetConfigCmd(U8 cfgSeid, U8 XDATA_PTR categoryPtr)
{
#if CODEC_AAC_DEMO
	if( cfgSeid == AAC_DEMO_SEID)
	{
		U8 value;
		if (categoryPtr[3] != AAC_CODEC)
			return AVDTP_UNSUPPORTED_CONFIGURATION;
		
		value = categoryPtr[4] & A2DP_AAC_OBJECT_TYPE_MPEG2_AAC_LC;
		if (value == 0)
			return AVDTP_UNSUPPORTED_CONFIGURATION;

		value = categoryPtr[5] & (A2DP_AAC_SAMPLE_FREQ_44100|A2DP_AAC_SAMPLE_FREQ_32000|A2DP_AAC_SAMPLE_FREQ_24000|A2DP_AAC_SAMPLE_FREQ_22050|A2DP_AAC_SAMPLE_FREQ_16000|A2DP_AAC_SAMPLE_FREQ_12000|A2DP_AAC_SAMPLE_FREQ_11025|A2DP_AAC_SAMPLE_FREQ_8000);
		if (value == 0 && (categoryPtr[6] & A2DP_AAC_SAMPLE_FREQ_48000) == 0)
			return AVDTP_UNSUPPORTED_CONFIGURATION;

		if ((categoryPtr[6] & (A2DP_AAC_CHANNELS_1|A2DP_AAC_CHANNELS_2)) == 0)
			return AVDTP_UNSUPPORTED_CONFIGURATION;
		
		return sizeof(AACDemoCapabilities);
	}
#endif
#if CODEC_HIGH_QUALITY
	if( cfgSeid == VENDOR_SNK_SEID_HIGH_QUALITY)
	{
		if (categoryPtr[3] != NON_A2DP_CODEC)
			return 0;

		/*  Check if configuration is reasonable.  */
		
		return sizeof(VCHQServiceCapabilities);
	}
#endif
	ASSERT(FALSE);
	return 0;
}

PUBLIC U8 SDK_A2DP_VC_GetConfigCmd(U8 seid, U8 XDATA_PTR servicePtr, U8 XDATA_PTR localConfigPtr)
{
#if CODEC_AAC_DEMO
	if( seid == AAC_DEMO_SEID)
	{
		servicePtr[1] = AAC_DEMO_LENGTH + 2;
		servicePtr[3] = AAC_CODEC;
		OSMEM_memcpy_xdata_xdata(&servicePtr[4], localConfigPtr, AAC_DEMO_LENGTH);
		return AAC_DEMO_LENGTH;
	}
#endif
#if CODEC_HIGH_QUALITY
	if( seid == VENDOR_SNK_SEID_HIGH_QUALITY)
	{
		servicePtr[1] = VEDNOR_CODEC_HQ_CAPABILITY_LENGTH + 2;
		servicePtr[3] = NON_A2DP_CODEC;
		OSMEM_memcpy_xdata_xdata(&servicePtr[4], localConfigPtr, VEDNOR_CODEC_HQ_CAPABILITY_LENGTH);
		return VEDNOR_CODEC_HQ_CAPABILITY_LENGTH;
	}
#endif
	ASSERT(FALSE);
	return 0;
}

PUBLIC BOOL SDK_A2DP_VC_GetCapabilityResp(U8 XDATA_PTR paraPtr, U8 XDATA_PTR XDATA_PTR pConfigPtr, U8 XDATA_PTR pConfigLen, U8 XDATA_PTR pIntSeid)
{
#if CODEC_HIGH_QUALITY || CODEC_AAC_DEMO
	U8 XDATA_PTR configPtr = *pConfigPtr;
#endif
#if CODEC_AAC_DEMO
	if(paraPtr[3] == AAC_CODEC)
	{
		U32 bitrate1, bitrate2;
		
		if(paraPtr[4] & A2DP_AAC_OBJECT_TYPE_MPEG2_AAC_LC)
			configPtr[4] = A2DP_AAC_OBJECT_TYPE_MPEG2_AAC_LC;
		else
			return FALSE;

		if(paraPtr[5] & A2DP_AAC_SAMPLE_FREQ_44100)
		{
			configPtr[5] = A2DP_AAC_SAMPLE_FREQ_44100;
			configPtr[6] = 0;
		}
		else if(paraPtr[6] & A2DP_AAC_SAMPLE_FREQ_48000)
		{
			configPtr[5] = 0;
			configPtr[6] = A2DP_AAC_SAMPLE_FREQ_48000;
		}
		else
			return FALSE;

		if(paraPtr[6] & A2DP_AAC_CHANNELS_2)
			configPtr[6] |= A2DP_AAC_CHANNELS_2;
		else if(paraPtr[6] & A2DP_AAC_CHANNELS_1)
			configPtr[6] |= A2DP_AAC_CHANNELS_1;
		else
			return FALSE;

		bitrate2 = (*(U32 XDATA_PTR)&paraPtr[6]) & 0x007FFFFF;
		bitrate1 = (*(U32 XDATA_PTR)&gA2DP_nvram.seidCtl.aacDefaultSettings.sampleRate2) & 0x007FFFFF;
		if (bitrate2 > bitrate1)
		{
			bitrate2 = bitrate1;
		}
		
		configPtr[7] = (paraPtr[7] & A2DP_AAC_VARIABLE_BIT_RATE) | (U8)(bitrate2 >> 16);
		configPtr[8] = (U8)(bitrate2 >> 8);
		configPtr[9] = (U8)bitrate2;

		*pConfigPtr += 10;
		*pConfigLen += 10;
		*pIntSeid = AAC_DEMO_SEID;
		
		if (!MMI_A2DP_IsHighQualityCodecDisbaled())
			return TRUE;
	}
#endif
#if CODEC_HIGH_QUALITY
	if(paraPtr[3] == NON_A2DP_CODEC)
	{
		if(paraPtr[4] == 0x94 && paraPtr[5] == 0x00)
		{
			configPtr[4] = 0x94;
			configPtr[5] = 0x00;
			configPtr[6] = 0x00;
			configPtr[7] = 0x00;
		}
		else
		{
			return FALSE;
		}

		if(paraPtr[8] == BYTE1(VENDOR_CODEC_ID_HIGH_QUALITY) && paraPtr[9] == BYTE0(VENDOR_CODEC_ID_HIGH_QUALITY))
		{
			configPtr[8] = BYTE1(VENDOR_CODEC_ID_HIGH_QUALITY);
			configPtr[9] = BYTE0(VENDOR_CODEC_ID_HIGH_QUALITY);
		}
		else
		{
			return FALSE;
		}
		
		configPtr[10] = 0;
		
		*pConfigPtr += 11;
		*pConfigLen += 11;
		*pIntSeid = VENDOR_SNK_SEID_HIGH_QUALITY;
		
		if (!MMI_A2DP_IsHighQualityCodecDisbaled())
			return TRUE;
	}
#endif

	return FALSE;
}

#endif

PUBLIC void SDK_A2DP_VC_Init(void)
{
#if SDK_VENDOR_CODEC
	SDK_Proxy_RegisterAPI( API_GRP_PF_CB_AVDTP_DISCOVER_CMD, API_GRP_PROFILE, (VFUN)SDK_A2DP_VC_DiscoverCmd);
	SDK_Proxy_RegisterAPI( API_GRP_PF_CB_AVDTP_GET_CAPABILITY_CMD, API_GRP_PROFILE, (VFUN)SDK_A2DP_VC_GetCapabilityCmd);
	SDK_Proxy_RegisterAPI( API_GRP_PF_CB_AVDTP_SET_MEDIA_PKT_DECODER, API_GRP_PROFILE, (VFUN)SDK_A2DP_VC_GetMediaCodecFunc);
	SDK_Proxy_RegisterAPI( API_GRP_PF_CB_AVDTP_SET_CONFIG_CMD, API_GRP_PROFILE, (VFUN)SDK_A2DP_VC_SetConfigCmd);
	SDK_Proxy_RegisterAPI( API_GRP_PF_CB_AVDTP_GET_CONFIG_CMD, API_GRP_PROFILE, (VFUN)SDK_A2DP_VC_GetConfigCmd);
	SDK_Proxy_RegisterAPI( API_GRP_PF_CB_AVDTP_GET_CAPABILITY_RESP, API_GRP_PROFILE, (VFUN)SDK_A2DP_VC_GetCapabilityResp);
#endif
}