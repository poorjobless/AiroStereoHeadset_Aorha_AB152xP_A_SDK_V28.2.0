#include "reside_flash.inc"

#define _HID_C_

#include "HID_interface.h"
#include "..\L2CAP\L2CAP_interface.h"
#include "..\MMI\MMI_HID.h"
#include "..\MMI\MMI_interface.h"
#include "..\MMI\MMI.h"
#include "..\MMI\MMI_Reconnect.h"
#include "..\MMI\MMI_Base.h"
#include "..\MMI\MMI_Protocol.h"
#include "..\SECTOR\Sector_mmi_driver_variation_nvram.h"
#include "..\SDAP\SDAP_interface.h"

//establish: HID_CONTRL -> HID_INTERRUPT

#ifdef HID_Profile

PRIVATE MMI_PROFILE_INFO XDATA_PTR HID_ConnectRequest(U8 linkIndex, U8 profileId);
PRIVATE U8 HID_DisconnectRequest(U8 linkIndex, MMI_PROFILE_INFO XDATA_PTR profileInfo);

static MMI_PROFILE_DESC CODE gMMIHID_ProfileDesc = {
	PROFILE_HID,
	NULL,					    //ProfileTaskFunc
	HID_ConnectRequest,	        //ProfileConnectRequest
	HID_DisconnectRequest,	    //ProfileDisconnectRequest
	NULL,	                    //ProfileLinkEvent
	NULL,	                    //ProfileDataEvent
};

static MMI_PROFILE_NODE XDATA gMMIHID_ProfileNode = {
	&gMMIHID_ProfileDesc,
	(MMI_PROFILE_NODE XDATA_PTR) NULL
};

PRIVATE void HID_SetL2capConnReqParams(L2CAPConnReqParams XDATA_PTR paramPtr);
PRIVATE VOID HID_RecvDataBackup (U8 linkIndex, HID_DATA_STRU XDATA_PTR dataPtr, U8 dataContIndex);
PRIVATE BOOL HID_RemoteCmdHdlrUnsupport(L2CAP_CHANNEL_HDL l2capChannelHdl, HID_DATA_STRU XDATA_PTR dataPtr);

U8 CODE gHID_kbBaseData[] =
{
	HID_DATA_TO_L2CAP_OFFSET, 0, 0, 0, 0, 0, 0, 0, 
	10, 0, 0, 0, 
	HID_TRANTYPE_DATA_INPUT, HID_REPORT_ID_KB, 0, 0, 0, 0, 0, 0, 0, 0  //add report ID

};

U8 CODE gHID_mouseBaseData[] =
{
	HID_DATA_TO_L2CAP_OFFSET, 0, 0, 0, 0, 0, 0, 0, 
	//5, 0, 0, 0, 
	6, 0, 0, 0, 
	HID_TRANTYPE_DATA_INPUT, HID_REPORT_ID_MOUSE, 0 ,0 ,0	 //add report ID

};

PUBLIC void HID_SetCmdHeadFromRemoteEvent(HID_REPORT_DATA_STRU XDATA_PTR dataPtr , U8 commandOp , U8 payloadLen)
{	
    dataPtr->offset = HID_DATA_TO_L2CAP_OFFSET;
    dataPtr->l2capHdr.len = ENDIAN_TRANSFORM_U16((U16)payloadLen);
    dataPtr->request = commandOp;
}

PUBLIC void HID_SetCmdHead(HID_REPORT_DATA_STRU XDATA_PTR dataPtr, U8 payloadLen)
{	
    dataPtr->offset = HID_DATA_TO_L2CAP_OFFSET;
    dataPtr->l2capHdr.len = ENDIAN_TRANSFORM_U16((U16)payloadLen);    
}

PRIVATE void HID_SendRemoteCmd(L2CAP_CHANNEL_HDL l2capChannelHdl, HID_DATA_STRU XDATA_PTR dataPtr , U8 commandOp , U8 payloadLen)
{
    HID_SetCmdHeadFromRemoteEvent((HID_REPORT_DATA_STRU XDATA_PTR)dataPtr, commandOp, payloadLen);
    UpperLayer_SendL2capChannelData(l2capChannelHdl, (U8 XDATA_PTR)dataPtr);
}

PRIVATE L2CAP_CHANNEL_HDL HID_CreateCtlChannel(U8 linkIndex)
{
    L2CAPLocalConnReqType LDATA reqMsg;
    L2CAP_CHANNEL_HDL l2capChannelHdl;
    
    reqMsg.psm = PSM_HID_CTL;
    HID_SetL2capConnReqParams(&reqMsg.params);
    l2capChannelHdl = UpperLayer_SendL2capConnectionRequest(linkIndex, &reqMsg);
    if (l2capChannelHdl != INVALID_L2CAP_CHANNEL)
    {
        gHID_channel_ctl[linkIndex].channelHDLIndex[HID_CONTROL_CHANNEL] = l2capChannelHdl;
        gHID_channel_ctl[linkIndex].mmiProfileInfo.state = HID_STATE_WAIT_CTL_CHANNEL_CONN_CMP;
        
        return l2capChannelHdl;
    }
    return (L2CAP_CHANNEL_HDL)NULL;
}

PRIVATE BOOL HID_CreateIntChannel(U8 linkIndex)
{
    L2CAPLocalConnReqType LDATA reqMsg;
    L2CAP_CHANNEL_HDL l2capChannelHdl;
    
    reqMsg.psm = PSM_HID_INT;
    HID_SetL2capConnReqParams(&reqMsg.params);
    l2capChannelHdl = UpperLayer_SendL2capConnectionRequest(linkIndex, &reqMsg);
    if (l2capChannelHdl != INVALID_L2CAP_CHANNEL)
    {
        gHID_channel_ctl[linkIndex].channelHDLIndex[HID_INTERRUPT_CHANNEL] = l2capChannelHdl;
        gHID_channel_ctl[linkIndex].mmiProfileInfo.state = HID_STATE_WAIT_INT_CHANNEL_CONN_CMP;
        
        return TRUE;
    }
    return FALSE;
}

PRIVATE VOID HID_DisconnectCtlChannel(U8 linkIndex)
{
    UpperLayer_SendL2capDisconnectionRequest(gHID_channel_ctl[linkIndex].channelHDLIndex[HID_CONTROL_CHANNEL]);

    gHID_channel_ctl[linkIndex].mmiProfileInfo.state = HID_STATE_WAIT_CTL_CHANNEL_DISCONN_CMP;
}

PRIVATE VOID HID_DisconnectIntChannel(U8 linkIndex)
{
    UpperLayer_SendL2capDisconnectionRequest(gHID_channel_ctl[linkIndex].channelHDLIndex[HID_INTERRUPT_CHANNEL]);

    gHID_channel_ctl[linkIndex].mmiProfileInfo.state = HID_STATE_WAIT_INT_CHANNEL_DISCONN_CMP;
}

PRIVATE void HID_CTL(L2CAP_CHANNEL_HDL l2capChannelHdl)
{
    UNUSED(l2capChannelHdl);
}

PRIVATE VOID HID_Reset(U8 linkIndex)
{
    OSMEM_memset_xdata(&gHID_ctl[linkIndex], 0, sizeof(HIDCtlType));
    OSMEM_memset_xdata(&gHID_channel_ctl[linkIndex], 0, sizeof(HIDChannelCtlType));
    gHID_ctl[linkIndex].curProtocolMode = HID_PROTOCOL_MODE_REPORT;
}

PRIVATE VOID HID_UpdateVirtualCabled(U8 linkIndex, U8 plugged)
{
    LinkHistoryType XDATA_PTR linkListPtr;

    linkListPtr = MMI_LinkKey_SearchHistoryByBdAddr((U8 XDATA_PTR)&gMMI_ctl.mmiInfo[linkIndex].bdAddr[0], TRUE, FALSE);
	if (linkListPtr != (LinkHistoryType XDATA_PTR)NULL)
	{
	    LightDbgPrint("vc %x %x", linkIndex, plugged);
	    if (plugged)
	    {
	        linkListPtr->miscMask |= NVRAM_LINK_KEY_MISC_MASK_HID;
	    }
	    else
	    {
		    linkListPtr->miscMask &= (~NVRAM_LINK_KEY_MISC_MASK_HID);
            linkListPtr->key_status = NVRAM_KEY_NG;
        }
		MMI_SECTOR_SetUpdateFlag(TRUE);
	}
}

PRIVATE VOID HID_CTL_RemoteConnReqEvt(L2CAP_CHANNEL_HDL l2capChannelHdl, L2CAPRemoteConnReqType XDATA_PTR reqPtr)
{
    U8 linkIndex = L2CAP_GetChannelLinkIndex(l2capChannelHdl);

    if (!HID_IsEnable())
    {
        reqPtr->status = L2CAP_STATUS_REJECT_BY_REMOTE;
    }
    else if (gHID_channel_ctl[linkIndex].mmiProfileInfo.state == HID_STATE_IDLE)
    {
        MMI_LoadProfile(linkIndex, &gHID_channel_ctl[linkIndex].mmiProfileInfo, &gMMIHID_ProfileDesc);
        reqPtr->status = L2CAP_STATUS_SUCCESS;
        HID_SetL2capConnReqParams(&reqPtr->params);
        gHID_channel_ctl[linkIndex].channelHDLIndex[HID_CONTROL_CHANNEL] = l2capChannelHdl;
        gHID_channel_ctl[linkIndex].mmiProfileInfo.state = HID_STATE_WAIT_INT_CHANNEL_CONN_CMP;        
    }
    else
    {
        reqPtr->status = L2CAP_STATUS_REJECT_BY_REMOTE;  // already establish profile
    }
}

PRIVATE VOID HID_CTL_ConnCplEvt(L2CAP_CHANNEL_HDL l2capChannelHdl, U8 status)
{
    U8 linkIndex = L2CAP_GetChannelLinkIndex(l2capChannelHdl);

    LightDbgPrint("HID CTL cpl state: %x status %x", (U8)gHID_channel_ctl[linkIndex].mmiProfileInfo.state, (U8)status);
    if (gHID_channel_ctl[linkIndex].mmiProfileInfo.state == HID_STATE_WAIT_CTL_CHANNEL_CONN_CMP)    //active
    {
        switch (status)
        {
            case L2CAP_STATUS_SUCCESS:
            {
                gHID_channel_ctl[linkIndex].channelHDLIndex[HID_CONTROL_CHANNEL] = l2capChannelHdl;
            
                HID_CreateIntChannel(linkIndex);
            }
            break;

            case L2CAP_STATUS_PENDING:
            case L2CAP_STATUS_CONNECTION_REFUSED_SECURITY_BLOCK:
                // do nothing, wait for next response
                break;
            
            default:
            {
                gHID_channel_ctl[linkIndex].mmiProfileInfo.state = HID_STATE_IDLE;
                MMI_UnloadProfile(linkIndex, &gHID_channel_ctl[linkIndex].mmiProfileInfo);
                //cttodo
                #if 0
                if (gHID_VendorCB)
                {
                    gHID_VendorCB(linkIndex, channelIndex, HID_TO_APP_CREATE_PROFILE_FAIL, (U8 XDATA_PTR)&status);
                }    
                #endif
            }
            break;
        }               
    }
    else if (gHID_channel_ctl[linkIndex].mmiProfileInfo.state == HID_STATE_WAIT_INT_CHANNEL_CONN_CMP)   //passive
    {
        if (status != L2CAP_STATUS_SUCCESS)
        {
            gHID_channel_ctl[linkIndex].mmiProfileInfo.state = HID_STATE_IDLE;
            MMI_UnloadProfile(linkIndex, &gHID_channel_ctl[linkIndex].mmiProfileInfo);
        }
    }
}

PRIVATE void HID_CTL_DisconnectCplEvt(L2CAP_CHANNEL_HDL l2capChannelHdl, U8 reason)
{    
    U8 linkIndex = L2CAP_GetChannelLinkIndex(l2capChannelHdl);
    
    UNUSED(reason);
    if (gHID_channel_ctl[linkIndex].channelHDLIndex[HID_CONTROL_CHANNEL] == l2capChannelHdl)
    {        
        MMI_HID_EventHandler(linkIndex, HID_DISCONNECTION_COMPLETE_EVENT);

        //MMI_SniffDisable(linkIndex, MMI_SNIFF_HID);        

        HID_Reset(linkIndex);
        
        //cttodo
        //MMI_CheckPowerOffOrDetach(linkIndex);
        
        #if 0
        if (gHID_VendorCB)
        {
            gHID_VendorCB(linkIndex, channelIndex, HID_TO_APP_DISCONNECTED, NULL);
        }
        #endif
    }
}

PRIVATE VOID HID_INT(L2CAP_CHANNEL_HDL l2capChannelHdl)
{
    UNUSED(l2capChannelHdl);
}

PRIVATE VOID HID_HandleINTConnCmp(L2CAP_CHANNEL_HDL l2capChannelHdl)
{
    U8 linkIndex = L2CAP_GetChannelLinkIndex(l2capChannelHdl);

    gHID_channel_ctl[linkIndex].channelHDLIndex[HID_INTERRUPT_CHANNEL] = l2capChannelHdl;
    gHID_channel_ctl[linkIndex].channelState= HID_CHANNEL_STATE_OPEN;
    
    gHID_channel_ctl[linkIndex].mmiProfileInfo.state = HID_STATE_CONNECTED;
    //gHID_channel_ctl[linkIndex].mmiProfileInfo.profileDesc = gMMIHID_ProfileNode.profileDesc;

    MMI_HID_EventHandler(linkIndex, HID_CONNECTION_COMPLETE_EVENT);

    MMI_SniffEnable(linkIndex, MMI_SNIFF_HID);

    HID_UpdateVirtualCabled(linkIndex, HID_VIRTUAL_PLUGGED);

    //cttodo
    #if 0
    if (gHID_VendorCB)
    {
        gHID_VendorCB(linkIndex, channelIndex, HID_TO_APP_CONNECTED, NULL);
    }
    #endif
}

PRIVATE VOID HID_INT_RemoteConnReqEvt(L2CAP_CHANNEL_HDL l2capChannelHdl, L2CAPRemoteConnReqType XDATA_PTR reqPtr)
{
    U8 linkIndex = L2CAP_GetChannelLinkIndex(l2capChannelHdl);

    LightDbgPrint("HID INT remConn state %x", (U8)gHID_channel_ctl[linkIndex].mmiProfileInfo.state);
    if (!HID_IsEnable())
    {
        reqPtr->status = L2CAP_STATUS_REJECT_BY_REMOTE;
        gHID_channel_ctl[linkIndex].mmiProfileInfo.state = HID_STATE_IDLE;
    }
    else if (gHID_channel_ctl[linkIndex].mmiProfileInfo.state == HID_STATE_WAIT_INT_CHANNEL_CONN_CMP)
    {
        reqPtr->status = L2CAP_STATUS_SUCCESS;
        HID_SetL2capConnReqParams(&reqPtr->params);        
        gHID_channel_ctl[linkIndex].mmiProfileInfo.state = HID_STATE_WAIT_INT_CHANNEL_CONN_CMP;
    }
    else
    {
        reqPtr->status = L2CAP_STATUS_REJECT_BY_REMOTE;  // already establish profile
    }
}

PRIVATE VOID HID_INT_ConnCplEvt(L2CAP_CHANNEL_HDL l2capChannelHdl, U8 status)
{
    U8 linkIndex = L2CAP_GetChannelLinkIndex(l2capChannelHdl);

    LightDbgPrint("HID INT cpl state: %x status %x", (U8)gHID_channel_ctl[linkIndex].mmiProfileInfo.state, (U8)status);
    if (gHID_channel_ctl[linkIndex].mmiProfileInfo.state == HID_STATE_WAIT_INT_CHANNEL_CONN_CMP)
    {
        if (status == L2CAP_STATUS_SUCCESS)
        {
            HID_HandleINTConnCmp(l2capChannelHdl);
            //MMI_ActivateProfile(linkIndex, &gHID_channel_ctl[linkIndex].mmiProfileInfo);
        }
        else
        {
            gHID_channel_ctl[linkIndex].mmiProfileInfo.state = HID_STATE_IDLE;            
            //cttodo
            #if 0
            if (gHID_VendorCB)
            {
                gHID_VendorCB(linkIndex, channelIndex, HID_TO_APP_CREATE_PROFILE_FAIL, (U8 XDATA_PTR)&status);
            }
            #endif
        } 
    }
}

PRIVATE void HID_INT_DisconnectCplEvt(L2CAP_CHANNEL_HDL l2capChannelHdl, U8 reason)
{
    U8 linkIndex = L2CAP_GetChannelLinkIndex(l2capChannelHdl);
    
    UNUSED(reason);
    if (gHID_channel_ctl[linkIndex].channelHDLIndex[HID_INTERRUPT_CHANNEL] == l2capChannelHdl)
    {        
        gHID_channel_ctl[linkIndex].channelState = HID_CHANNEL_STATE_DISCONNECT;  // after interrupt channel established

        if (gHID_channel_ctl[linkIndex].mmiProfileInfo.state == HID_STATE_WAIT_INT_CHANNEL_DISCONN_CMP)
        {
            HID_DisconnectCtlChannel(linkIndex);
        }
    }
#if 0
    UNUSED(reason);
    U8 linkIndex = L2CAP_GetChannelLinkIndex(l2capChannelHdl);
    
    if (gHID_channel_ctl[linkIndex].channelHDLIndex[HID_INTERRUPT_CHANNEL] == l2capChannelHdl)
    {
        gHID_channel_ctl[linkIndex].channelState = HID_CHANNEL_STATE_DISCONNECT;  // after interrupt channel established        

        if (gHID_channel_ctl[linkIndex].mmiProfileInfo.state == HID_STATE_WAIT_INT_CHANNEL_DISCONN_CMP)
        {
            HID_DisconnectCtlChannel(linkIndex);
        }
    }
#endif
}

PUBLIC L2CAP_CHANNEL_HDL HID_ProfileConnectionRequest(U8 linkIndex)
{
    if (gHID_channel_ctl[linkIndex].mmiProfileInfo.state == HID_STATE_IDLE)
    {
        return HID_CreateCtlChannel(linkIndex); 
    }
    return (L2CAP_CHANNEL_HDL)NULL;
}

PUBLIC BOOL HID_ProfileDisconnectionRequest(U8 linkIndex)
{
    if (gHID_channel_ctl[linkIndex].mmiProfileInfo.state != HID_STATE_CONNECTED)
    {
        return FALSE;
    }
    
    HID_DisconnectIntChannel(linkIndex);

    return TRUE;
}

PUBLIC VOID HID_DataReport(U8 linkIndex, HID_REPORT_DATA_STRU XDATA_PTR dataPtr, BOOL bCtlChannel)
{       
    if (gHID_channel_ctl[linkIndex].channelState == HID_CHANNEL_STATE_OPEN)
    {
        if (bCtlChannel)
        {
            UpperLayer_SendL2capChannelData(gHID_channel_ctl[linkIndex].channelHDLIndex[HID_CONTROL_CHANNEL], (U8 XDATA_PTR)dataPtr);
        }
        else
        {
            UpperLayer_SendL2capChannelData(gHID_channel_ctl[linkIndex].channelHDLIndex[HID_INTERRUPT_CHANNEL], (U8 XDATA_PTR)dataPtr);
        }        
    }
    else if (gHID_channel_ctl[linkIndex].channelState == HID_CHANNEL_STATE_INTERRUPT_DISCONNECT)
    {
        if (bCtlChannel)
        {
            UpperLayer_SendL2capChannelData(gHID_channel_ctl[linkIndex].channelHDLIndex[HID_CONTROL_CHANNEL], (U8 XDATA_PTR)dataPtr);
        }
        else
        {
            HID_CreateIntChannel(linkIndex);
            OSMEM_Put((U8 XDATA_PTR)dataPtr);   // tmp solution
        }
    }
    else
    {
        OSMEM_Put((U8 XDATA_PTR)dataPtr);
    }
}

PUBLIC BOOL HID_IsVirtualPlugged(U8 linkIndex)
{
    LinkHistoryType XDATA_PTR linkListPtr;

    linkListPtr = MMI_LinkKey_SearchHistoryByBdAddr((U8 XDATA_PTR)&gMMI_ctl.mmiInfo[linkIndex].bdAddr[0], TRUE, FALSE);
    if((linkListPtr != (LinkHistoryType XDATA_PTR)NULL) && (linkListPtr->miscMask & NVRAM_LINK_KEY_MISC_MASK_HID))
    {
        return TRUE;
    }
    return FALSE;
}

// wait for host send L2CAP disconnection request
PUBLIC VOID HID_LocalUnplug(L2CAP_CHANNEL_HDL l2capChannelHdl)
{
    U8 XDATA_PTR dataPtr;
    U8 linkIndex = L2CAP_GetChannelLinkIndex(l2capChannelHdl);
    
    if ((gHID_channel_ctl[linkIndex].mmiProfileInfo.state == HID_STATE_CONNECTED) && ((dataPtr = OSMEM_Get(OSMEM_ptr1)) != NULL))
    {        
    	HID_UpdateVirtualCabled(linkIndex, HID_VIRTUAL_UNPLUGGED);
        HID_SendRemoteCmd(gHID_channel_ctl[linkIndex].channelHDLIndex[HID_CONTROL_CHANNEL], (HID_DATA_STRU DATA_PTR)dataPtr, HID_TRANTYPE_HIDCONTROL_VIRTUALCABLEUNPLUG, 0x01);        
    }
}

PRIVATE BOOL HID_RemoteCmdHdlrHandshake(L2CAP_CHANNEL_HDL l2capChannelHdl, HID_DATA_STRU XDATA_PTR dataPtr)
{
    // The HANDSHAKE message shall not be sent on the Interrupt channel, and 
    // shall not be sent by a Bluetooth HID Host.
    return HID_RemoteCmdHdlrUnsupport(l2capChannelHdl, dataPtr);
}

PRIVATE BOOL HID_RemoteCmdHdlrControl(L2CAP_CHANNEL_HDL l2capChannelHdl, HID_DATA_STRU XDATA_PTR dataPtr)
{
    U8 linkIndex = L2CAP_GetChannelLinkIndex(l2capChannelHdl);
    switch (dataPtr->request & HID_THDR_PARAMETER_MASK)
    {
//      //HID1.1 DEPRECATED    
//      case HID_CONTROL_NOP:			
//			break;

//		case HID_CONTROL_HARD_RESET:
//            MMI_HID_EventHandler(linkIndex, HID_REMOTE_CONTROL_HARD_RESET);
//			break;

//		case HID_CONTROL_SOFT_RESET:
//            MMI_HID_EventHandler(linkIndex, HID_REMOTE_CONTROL_SOFT_RESET);
//			break;

		case HID_CONTROL_SUSPEND:            
            // inform DRIVER DO SOMETHING
			break;

		case HID_CONTROL_EXIT_SUSPEND:            
            // inform DRIVER DO SOMETHING
			break;

		case HID_CONTROL_VIRTUAL_CABLE_UNPLUG:
            HID_UpdateVirtualCabled(linkIndex, HID_VIRTUAL_UNPLUGGED);            
            HID_ProfileDisconnectionRequest(linkIndex);            
			break;
    }
    //cttodo
    #if 0
    if(gHID_VendorCB)
    {
        if (gHID_VendorCB(linkIndex, channelIndex, HID_TO_APP_CONTROL, (U8 XDATA_PTR)dataPtr))
        {
            return TRUE;
        }
    }
    #endif
    return FALSE;
}

PRIVATE BOOL HID_RemoteCmdHdlrUnsupport(L2CAP_CHANNEL_HDL l2capChannelHdl, HID_DATA_STRU XDATA_PTR dataPtr)
{
    HID_SendRemoteCmd(l2capChannelHdl, dataPtr, HID_TRANTYPE_HANDSHAKE_ERR_UNSUPPORTED_REQUEST, 0x01);
    
    return TRUE;
}

PRIVATE BOOL HID_RemoteCmdHdlrGetReport(L2CAP_CHANNEL_HDL l2capChannelHdl, HID_DATA_STRU XDATA_PTR dataPtr)
{
    U16 bufferSize;
    U8 linkIndex = L2CAP_GetChannelLinkIndex(l2capChannelHdl);
    //cttodo
    #if 0
    if(gHID_VendorCB)
    {
        if (gHID_VendorCB(linkIndex, channelIndex, HID_TO_APP_GET_REPORT, (U8 XDATA_PTR)dataPtr))
        {
            return TRUE;
        }
    }
    #endif

    if (dataPtr->request & HID_THDR_PARAMETER_REPORT_SIZE_MASK)
    {
        bufferSize = ENDIAN_TRANSFORM_U16(dataPtr->para.reportPara.bufferSize);

        if (gHID_ctl[linkIndex].curProtocolMode == HID_PROTOCOL_MODE_BOOT)
        {
            if ((bufferSize + HID_THDR_LEN + HID_REPORT_ID_LEN) >= HID_MTU_SIZE)
            {
                HID_SetCmdHeadFromRemoteEvent((HID_REPORT_DATA_STRU XDATA_PTR)dataPtr , HID_TRANTYPE_HANDSHAKE_ERR_INVALID_PARAMETER , 0x01);
                goto SEND_DATA_TO_L2CAP;
            }
        }
        else
        {
            // suppose no report id. if there is a report id in HID descriptor, app_hid can handle the case
            if (bufferSize + HID_THDR_LEN >= HID_MTU_SIZE)
            {
                HID_SetCmdHeadFromRemoteEvent((HID_REPORT_DATA_STRU XDATA_PTR)dataPtr , HID_TRANTYPE_HANDSHAKE_ERR_INVALID_PARAMETER , 0x01);
                goto SEND_DATA_TO_L2CAP;
            }
        }
    }

    switch (dataPtr->request & HID_THDR_PARAMETER_REPORT_TYPE_MASK)
    {
        case HID_THDR_REPORT_TYPE_INPUT:
        {                
            if (gHID_ctl[linkIndex].curProtocolMode == HID_PROTOCOL_MODE_BOOT)
            {
                if (dataPtr->para.reportPara.reportId == HID_KB_BOOT_MODE_REPORT_ID)
                {
                    OSMEM_memcpy_xdata_code (dataPtr, gHID_kbBaseData, sizeof(gHID_kbBaseData));
                    goto SEND_DATA_TO_L2CAP;
                }
                else if (dataPtr->para.reportPara.reportId == HID_MOUSE_BOOT_MODE_REPORT_ID)
                {
                    OSMEM_memcpy_xdata_code (dataPtr, gHID_mouseBaseData, sizeof(gHID_mouseBaseData));
                    goto SEND_DATA_TO_L2CAP;
                }
                else
                {
                     HID_SetCmdHeadFromRemoteEvent((HID_REPORT_DATA_STRU XDATA_PTR)dataPtr , HID_TRANTYPE_HANDSHAKE_ERR_INVALID_REPORT_ID , 0x01);
                     goto SEND_DATA_TO_L2CAP;
                }        
            }
        }
        break;

        // retrieval of an output report returns a copy of the last report received down the interrupt pipe
        // if no report, return default value or the instantaneous state of the fields
        case HID_THDR_REPORT_TYPE_OUTPUT:
            gHID_ctl[linkIndex].curReportType = HID_THDR_REPORT_TYPE_OUTPUT;
            
            if (((dataPtr->para.reportPara.reportId & 0x03) != 0x00) && gHID_ctl[linkIndex].curProtocolMode == HID_PROTOCOL_MODE_REPORT)
            {
                HID_SetCmdHeadFromRemoteEvent((HID_REPORT_DATA_STRU XDATA_PTR)dataPtr , HID_TRANTYPE_HANDSHAKE_ERR_INVALID_REPORT_ID , 0x01);
                goto SEND_DATA_TO_L2CAP;
            }
            else
            {
                HID_SetCmdHeadFromRemoteEvent((HID_REPORT_DATA_STRU XDATA_PTR)dataPtr , HID_TRANTYPE_HANDSHAKE_ERR_INVALID_PARAMETER , 0x01);
                goto SEND_DATA_TO_L2CAP;
            }
            break;    

        // default value or the instantaneous state of the fields
        case HID_THDR_REPORT_TYPE_FEATURE:
            break;                        
    }
        
    HID_SetCmdHeadFromRemoteEvent((HID_REPORT_DATA_STRU XDATA_PTR)dataPtr , HID_TRANTYPE_HANDSHAKE_ERR_INVALID_PARAMETER , 0x01);
    

    SEND_DATA_TO_L2CAP:
    UpperLayer_SendL2capChannelData(l2capChannelHdl, (U8 XDATA_PTR)dataPtr);	

    return TRUE;
}

PRIVATE BOOL HID_RemoteCmdHdlrSetReport(L2CAP_CHANNEL_HDL l2capChannelHdl, HID_DATA_STRU XDATA_PTR dataPtr)
{
    U16 len = ENDIAN_TRANSFORM_U16(dataPtr->l2capHdr.len);    
    U8 linkIndex = L2CAP_GetChannelLinkIndex(l2capChannelHdl);
    switch (dataPtr->request & HID_THDR_PARAMETER_REPORT_TYPE_MASK)
    {        
        case HID_THDR_REPORT_TYPE_OUTPUT:
            gHID_ctl[linkIndex].curReportType = HID_THDR_REPORT_TYPE_OUTPUT;
            break;

        case HID_THDR_REPORT_TYPE_INPUT:
            gHID_ctl[linkIndex].curReportType = HID_THDR_REPORT_TYPE_INPUT; 
            break;
        
        case HID_THDR_REPORT_TYPE_FEATURE:
            gHID_ctl[linkIndex].curReportType = HID_THDR_REPORT_TYPE_FEATURE; 
            break;                    
    }

    if (gHID_channel_ctl[linkIndex].in_MTU == 0)
    {
        gHID_channel_ctl[linkIndex].in_MTU = HID_MTU_SIZE;
    }

    if (len == gHID_channel_ctl[linkIndex].in_MTU)
    {
        gHID_ctl[linkIndex].setReportCont = TRUE;
        HID_RecvDataBackup(linkIndex, dataPtr, HID_RECV_CONT_SETREPORT);
    }
    else if (len < gHID_channel_ctl[linkIndex].in_MTU)
    {
        //cttodo
        #if 0
        if(gHID_VendorCB)
        {
            if (gHID_VendorCB(linkIndex, channelIndex, HID_TO_APP_SET_REPORT, (U8 XDATA_PTR)dataPtr))
            {
                return TRUE;
            }
        }
        #endif

        if (gHID_ctl[linkIndex].curProtocolMode == HID_PROTOCOL_MODE_BOOT)
        {
            if (len < HID_MOUSE_BOOT_MODE_DATA_LEN)
            {
                HID_SetCmdHeadFromRemoteEvent((HID_REPORT_DATA_STRU XDATA_PTR)dataPtr, HID_TRANTYPE_HANDSHAKE_ERR_INVALID_PARAMETER, 0x01);
            }
            else
            {
                HID_SetCmdHeadFromRemoteEvent((HID_REPORT_DATA_STRU XDATA_PTR)dataPtr, HID_TRANTYPE_HANDSHAKE_SUCCESSFUL, 0x01);
            }
        }
        else
        {
            HID_SetCmdHeadFromRemoteEvent((HID_REPORT_DATA_STRU XDATA_PTR)dataPtr, HID_TRANTYPE_HANDSHAKE_SUCCESSFUL, 0x01);
        }
    }
    else
    {
        HID_SetCmdHeadFromRemoteEvent((HID_REPORT_DATA_STRU XDATA_PTR)dataPtr, HID_TRANTYPE_HANDSHAKE_ERR_INVALID_PARAMETER, 0x01);
    }    
    UpperLayer_SendL2capChannelData(l2capChannelHdl, (U8 XDATA_PTR)dataPtr);	

    return TRUE;
}

PRIVATE BOOL HID_RemoteCmdHdlrGetProtocol(L2CAP_CHANNEL_HDL l2capChannelHdl, HID_DATA_STRU XDATA_PTR dataPtr)
{
    HID_REPORT_DATA_STRU XDATA_PTR reportDataPtr;
    U8 linkIndex = L2CAP_GetChannelLinkIndex(l2capChannelHdl);
    
    //cttodo
    #if 0
    if(gHID_VendorCB)
    {
        if (gHID_VendorCB(linkIndex, channelIndex, HID_TO_APP_GET_PROTOCOL, (U8 XDATA_PTR)dataPtr))
        {
            return TRUE;
        }
    }
    #endif
    
    reportDataPtr = (HID_REPORT_DATA_STRU XDATA_PTR)dataPtr;
    reportDataPtr->para.dataPayload = gHID_ctl[linkIndex].curProtocolMode;
    HID_SendRemoteCmd(l2capChannelHdl, dataPtr , HID_TRANTYPE_DATA_OTHER, 0x02);	
    
    return TRUE;
}

PRIVATE BOOL HID_RemoteCmdHdlrSetProtocol(L2CAP_CHANNEL_HDL l2capChannelHdl, HID_DATA_STRU XDATA_PTR dataPtr)
{
    U8 linkIndex = L2CAP_GetChannelLinkIndex(l2capChannelHdl);
    //cttodo
    #if 0
    if(gHID_VendorCB)
    {
        if (gHID_VendorCB(linkIndex, channelIndex, HID_TO_APP_SET_PROTOCOL, (U8 XDATA_PTR)dataPtr))
        {
            return TRUE;
        }
    }
    #endif
    switch(dataPtr->request & HID_THDR_PARAMETER_MASK)//Get protocol mode
	{
		case HID_PROTOCOL_MODE_BOOT:            
			gHID_ctl[linkIndex].curProtocolMode = HID_PROTOCOL_MODE_BOOT;
			HID_SetCmdHeadFromRemoteEvent((HID_REPORT_DATA_STRU XDATA_PTR)dataPtr , HID_TRANTYPE_HANDSHAKE_SUCCESSFUL, 0x01);
			break;

		case HID_PROTOCOL_MODE_REPORT:            
			gHID_ctl[linkIndex].curProtocolMode = HID_PROTOCOL_MODE_REPORT;
			HID_SetCmdHeadFromRemoteEvent((HID_REPORT_DATA_STRU XDATA_PTR)dataPtr , HID_TRANTYPE_HANDSHAKE_SUCCESSFUL, 0x01);
			break;

		default:
		 	HID_SetCmdHeadFromRemoteEvent((HID_REPORT_DATA_STRU XDATA_PTR)dataPtr , HID_TRANTYPE_HANDSHAKE_ERR_INVALID_PARAMETER, 0x01);
		    break;
	}																			
	UpperLayer_SendL2capChannelData(l2capChannelHdl, (U8 XDATA_PTR)dataPtr);	

    return TRUE;
}

PRIVATE BOOL HID_RemoteCmdHdlrGetIdle(L2CAP_CHANNEL_HDL l2capChannelHdl, HID_DATA_STRU XDATA_PTR dataPtr)
{
    HID_REPORT_DATA_STRU XDATA_PTR reportDataPtr;
    U8 linkIndex = L2CAP_GetChannelLinkIndex(l2capChannelHdl);
    
    reportDataPtr = (HID_REPORT_DATA_STRU XDATA_PTR)dataPtr;
    reportDataPtr->para.dataPayload = gHID_ctl[linkIndex].curIdleRate;
    HID_SendRemoteCmd(l2capChannelHdl, dataPtr, HID_TRANTYPE_DATA_OTHER, 0x02);													
    
    return TRUE;
}

PRIVATE BOOL HID_RemoteCmdHdlrSetIdle(L2CAP_CHANNEL_HDL l2capChannelHdl, HID_DATA_STRU XDATA_PTR dataPtr)
{
    U8 linkIndex = L2CAP_GetChannelLinkIndex(l2capChannelHdl);
    gHID_ctl[linkIndex].curIdleRate = dataPtr->para.idleRate;
	HID_SendRemoteCmd(l2capChannelHdl, dataPtr, HID_TRANTYPE_HANDSHAKE_SUCCESSFUL, 0x01);    		

    return TRUE;
}

PRIVATE VOID HID_RecvDataBackup (U8 linkIndex, HID_DATA_STRU XDATA_PTR dataPtr, U8 dataContIndex)
{
	U8 XDATA_PTR ptr1;
	U8 size;

	size = ENDIAN_TRANSFORM_U16(dataPtr->l2capHdr.len) + OS_OFFSET_OF(HID_DATA_STRU, request);

	ptr1 = OSMEM_Get(OSMEM_ptr2);

    // ptr1: L2CAP HDR + HID THDR + data playload (no HCI HDR and offset)
  	OSMEM_memcpy_xdata_xdata (ptr1, (U8 XDATA_PTR)dataPtr, size);

	if (dataContIndex == HID_RECV_CONT_SETREPORT)
	{
		if (gHID_ctl[linkIndex].setReportContDataPtr != (HID_DATA_STRU XDATA_PTR)0)
		{
			OSMEM_Put (gHID_ctl[linkIndex].setReportContDataPtr);				
			gHID_ctl[linkIndex].setReportContDataPtr = (U8 XDATA_PTR)0;
		}
		gHID_ctl[linkIndex].setReportContDataPtr = (HID_DATA_STRU XDATA_PTR)ptr1;
		gHID_ctl[linkIndex].setReportContDataCurPtr	= (U8 XDATA_PTR)gHID_ctl[linkIndex].setReportContDataPtr + gHID_channel_ctl[linkIndex].in_MTU + OS_OFFSET_OF(HID_DATA_STRU, request);
		
	}
	else if (dataContIndex == HID_RECV_CONT_DATA || dataContIndex == HID_RECV_CONT_NON)
	{
		if (gHID_ctl[linkIndex].dataPtr != (HID_DATA_STRU XDATA_PTR)0)
		{
			OSMEM_Put ((U8 XDATA_PTR)gHID_ctl[linkIndex].dataPtr);				
			gHID_ctl[linkIndex].dataPtr = (U8 XDATA_PTR)0;
		}
		gHID_ctl[linkIndex].dataPtr = (HID_DATA_STRU XDATA_PTR)ptr1;
		gHID_ctl[linkIndex].dataContDataCurPtr = (U8 XDATA_PTR)gHID_ctl[linkIndex].dataPtr + gHID_channel_ctl[linkIndex].in_MTU + OS_OFFSET_OF(HID_DATA_STRU, request);
		gHID_ctl[linkIndex].dataPtrSize.sizeU16 = (size - HID_L2CAP_HDR_LEN - HID_THDR_LEN);
	}
}

PRIVATE BOOL HID_RemoteCmdHdlrData(L2CAP_CHANNEL_HDL l2capChannelHdl, HID_DATA_STRU XDATA_PTR dataPtr)
{
    U8 linkIndex = L2CAP_GetChannelLinkIndex(l2capChannelHdl);
    
    if(gHID_channel_ctl[linkIndex].in_MTU == 0)
    {
        gHID_channel_ctl[linkIndex].in_MTU = HID_MTU_SIZE;
    }
    
    if(dataPtr->hciHdr.Len == gHID_channel_ctl[linkIndex].in_MTU)
	{
		gHID_ctl[linkIndex].dataCont = TRUE;
		HID_RecvDataBackup (linkIndex, dataPtr, HID_RECV_CONT_DATA);
		OSMEM_Put((U8 XDATA_PTR)dataPtr);
	}
	else if (dataPtr->hciHdr.Len < gHID_channel_ctl[linkIndex].in_MTU)
	{	
		//HID_RecvDataBackup (linkIndex, dataPtr, HID_RECV_CONT_NON);
		//cttodo
		#if 0
		if(gHID_VendorCB)
        {
            if (gHID_VendorCB(linkIndex, channelIndex, HID_TO_APP_DATA, (U8 XDATA_PTR)dataPtr))
            {
                return TRUE;
            }
        }
        #endif
		OSMEM_Put((U8 XDATA_PTR)dataPtr);
	}	
	else 
	{
		HID_SendRemoteCmd(l2capChannelHdl, dataPtr, HID_TRANTYPE_HANDSHAKE_ERR_INVALID_PARAMETER, 0x01 );        
	}	       

    return TRUE;
}

PRIVATE VOID HID_SaveSetReportContData(U8 linkIndex, HID_DATA_STRU XDATA_PTR dataPtr, U8 size)
{
	gHID_ctl[linkIndex].setReportContDataPtrSize.sizeU16 += size;
    gHID_ctl[linkIndex].setReportContDataPtr->l2capHdr.len = ENDIAN_TRANSFORM_U16(gHID_ctl[linkIndex].setReportContDataPtrSize.sizeU16);	
	OSMEM_memcpy_xdata_xdata (gHID_ctl[linkIndex].setReportContDataCurPtr, &dataPtr->para.dataPayload, size);
	gHID_ctl[linkIndex].setReportContDataCurPtr = gHID_ctl[linkIndex].setReportContDataCurPtr + size;
}

PRIVATE VOID HID_SaveDataContData(U8 linkIndex, HID_DATA_STRU XDATA_PTR dataPtr, U8 size)
{
	gHID_ctl[linkIndex].dataPtrSize.sizeU16 += size;
    gHID_ctl[linkIndex].dataPtr->l2capHdr.len = ENDIAN_TRANSFORM_U16(gHID_ctl[linkIndex].dataPtrSize.sizeU16);	
	OSMEM_memcpy_xdata_xdata (gHID_ctl[linkIndex].dataContDataCurPtr, &dataPtr->para.dataPayload, size);
	gHID_ctl[linkIndex].dataContDataCurPtr = gHID_ctl[linkIndex].dataContDataCurPtr + size;
}

PRIVATE VOID HID_RecvContData (L2CAP_CHANNEL_HDL l2capChannelHdl, HID_DATA_STRU XDATA_PTR dataPtr)
{
	U8 hidDataLen;
	U8 linkIndex = L2CAP_GetChannelLinkIndex(l2capChannelHdl);
	
	hidDataLen = (U8)ENDIAN_TRANSFORM_U16(dataPtr->l2capHdr.len) - HID_THDR_LEN;

	if ((gHID_ctl[linkIndex].setReportCont == TRUE) && (dataPtr->request == HID_TRANTYPE_DATC_OUTPUT))//the DATC pkt of SET_REPORT
	{
		if ((hidDataLen + 1) == gHID_channel_ctl[linkIndex].in_MTU)
		{	
			HID_SaveSetReportContData(linkIndex, dataPtr, hidDataLen);
			OSMEM_Put(dataPtr);
		}
		else if ((hidDataLen + 1) < gHID_channel_ctl[linkIndex].in_MTU) //recv the last pkt
		{
			if (hidDataLen != 0)
			{
				HID_SaveSetReportContData(linkIndex, dataPtr, hidDataLen);
			}
			gHID_ctl[linkIndex].setReportCont = FALSE;

            //cttodo
            #if 0
            if(gHID_VendorCB)
            {
                if (gHID_VendorCB(linkIndex, channelIndex, HID_TO_APP_SET_REPORT, (U8 XDATA_PTR)gHID_ctl[linkIndex].setReportContDataPtr))
                {
                    gHID_ctl[linkIndex].setReportContDataPtr = 0;   // app releases the pointer
                    OSMEM_Put(dataPtr);// app will handle ack
                    return;
                }
            }
            #endif
            HID_SendRemoteCmd(l2capChannelHdl, dataPtr, HID_TRANTYPE_HANDSHAKE_SUCCESSFUL, 0x01);
		}
		else
		{
			HID_SendRemoteCmd(l2capChannelHdl, dataPtr, HID_TRANTYPE_HANDSHAKE_ERR_INVALID_PARAMETER, 0x01);			
		}
	}
	else if (gHID_ctl[linkIndex].dataCont == TRUE)//the DATC pkt of DATA	
	{

		if ((hidDataLen + 1) == gHID_channel_ctl[linkIndex].in_MTU)
		{
			HID_SaveDataContData(linkIndex, dataPtr, hidDataLen);
			OSMEM_Put (dataPtr);
		}
		else if ((hidDataLen + 1) < gHID_channel_ctl[linkIndex].in_MTU) //recv the last pkt
		{
			if (hidDataLen != 0)
			{
				HID_SaveDataContData(linkIndex, dataPtr, hidDataLen);
			}
			OSMEM_Put(dataPtr);							
			gHID_ctl[linkIndex].dataCont = FALSE;
            //cttodo
            #if 0
            if (gHID_VendorCB)
            {                
                if (gHID_VendorCB(linkIndex, channelIndex, HID_TO_APP_SET_REPORT, (U8 XDATA_PTR)gHID_ctl[linkIndex].dataPtr))
                {
                    gHID_ctl[linkIndex].dataPtr = 0;    // app releases the pointer
                    return;
                }                
            }
            #endif
		}
		else
		{
			HID_SendRemoteCmd(l2capChannelHdl, dataPtr , HID_TRANTYPE_HANDSHAKE_ERR_INVALID_PARAMETER, 0x01);			
		}
	}
	else
	{
		HID_SendRemoteCmd(l2capChannelHdl, dataPtr , HID_TRANTYPE_HANDSHAKE_ERR_INVALID_PARAMETER, 0x01);		
	}
}

PRIVATE BOOL HID_RemoteCmdHdlrDatc(L2CAP_CHANNEL_HDL l2capChannelHdl, HID_DATA_STRU XDATA_PTR dataPtr)
{
    U8 linkIndex = L2CAP_GetChannelLinkIndex(l2capChannelHdl);
    
    if ((gHID_ctl[linkIndex].dataCont == TRUE) || (gHID_ctl[linkIndex].setReportCont == TRUE))
    {
        HID_RecvContData(l2capChannelHdl, dataPtr);
    }
    else
    {
        HID_SendRemoteCmd(l2capChannelHdl, dataPtr, HID_TRANTYPE_HANDSHAKE_ERR_INVALID_PARAMETER, 0x01);        
    }

    return TRUE;
}

PRIVATE MMI_PROFILE_INFO XDATA_PTR HID_ConnectRequest(U8 linkIndex, U8 profileId)
{
	UNUSED(profileId);

    if (!HID_IsEnable())
    {
        LightDbgPrint("not support HID");
        return (MMI_PROFILE_INFO XDATA_PTR)NULL;
    }
    //LightDbgPrint("HID_ConnectRequest %x %x", (U8)gHID_channel_ctl[linkIndex].mmiProfileInfo.state, (U8)HID_ProfileConnectionRequest(linkIndex));
	if (gHID_channel_ctl[linkIndex].mmiProfileInfo.state == HID_STATE_IDLE)
	{
		if (HID_ProfileConnectionRequest(linkIndex) != INVALID_L2CAP_CHANNEL)
		{
			return &gHID_channel_ctl[linkIndex].mmiProfileInfo;
		}
	}
	return (MMI_PROFILE_INFO XDATA_PTR)NULL;
}

PRIVATE U8 HID_DisconnectRequest(U8 linkIndex, MMI_PROFILE_INFO XDATA_PTR profileInfo)
{
	if (&gHID_channel_ctl[linkIndex].mmiProfileInfo == profileInfo)
	{
		switch(gHID_channel_ctl[linkIndex].mmiProfileInfo.state)
		{
			case PROFILE_CONNECTED:
			case PROFILE_CONNECTING:
			case PROFILE_DISCONNECTING:
				if(CMD_WAITING == HID_ProfileDisconnectionRequest(linkIndex))
				{
					gHID_channel_ctl[linkIndex].mmiProfileInfo.state = PROFILE_DISCONNECTING; 
					return CMD_WAITING;
				}
				break;
		}
		MMI_UnloadProfile(linkIndex, &gHID_channel_ctl[linkIndex].mmiProfileInfo);
	}
	return CMD_COMPLETE;
}



PRIVATE HID_REMOTE_CMD_HDLR CODE gHIDRemoteCmdHdlr[] =
{
    HID_RemoteCmdHdlrHandshake,
    HID_RemoteCmdHdlrControl,
    HID_RemoteCmdHdlrUnsupport,
    HID_RemoteCmdHdlrUnsupport,
    HID_RemoteCmdHdlrGetReport,
    HID_RemoteCmdHdlrSetReport,
    HID_RemoteCmdHdlrGetProtocol,
    HID_RemoteCmdHdlrSetProtocol,
    HID_RemoteCmdHdlrGetIdle,
    HID_RemoteCmdHdlrSetIdle,
    HID_RemoteCmdHdlrData,
    HID_RemoteCmdHdlrDatc,
};

PRIVATE VOID HID_ChannelDataHandler(L2CAP_CHANNEL_HDL l2capChannelHdl, U8 XDATA_PTR dataPtr)
{    
    U8 transType = (((HID_DATA_STRU XDATA_PTR)dataPtr)->request & HID_THDR_TRANSACTION_TYPE_MASK) >> 4;

    if (transType < HID_REMOTE_CMD_HDR_NUM)    
    {
        if (!gHIDRemoteCmdHdlr[transType](l2capChannelHdl, (HID_DATA_STRU XDATA_PTR)dataPtr))
        {
            OSMEM_Put(dataPtr);
        }
    }
    else
    {
        HID_RemoteCmdHdlrUnsupport(l2capChannelHdl, (HID_DATA_STRU XDATA_PTR)dataPtr);
    }    	
}

PRIVATE VOID HID_SetL2capConnReqParams(L2CAPConnReqParams XDATA_PTR paramPtr)
{
	paramPtr->needAuth = TRUE;
	//paramPtr->needAuth = FALSE;
	paramPtr->mtuSize = HID_MTU_SIZE;
	paramPtr->ChannelDataHandler = HID_ChannelDataHandler;
}

static L2CAP_PROTOCOL_DESC CODE gHID_CTL_ProtocolDesc = {
	PSM_HID_CTL,
	HID_CTL,					//TaskEntry
	HID_CTL_RemoteConnReqEvt,	//RemoteConnReqEvt
	HID_CTL_ConnCplEvt,		    //ConnCplEvt
	HID_CTL_DisconnectCplEvt,   //DisconnectCplEvt
};

static L2CAP_PROTOCOL_NODE XDATA gHID_CTL_ProtocolNode = {
	&gHID_CTL_ProtocolDesc,
	(L2CAP_PROTOCOL_NODE XDATA_PTR) NULL
};

static L2CAP_PROTOCOL_DESC CODE gHID_INT_ProtocolDesc = {
	PSM_HID_INT,
	HID_INT,					//TaskEntry
	HID_INT_RemoteConnReqEvt,	//RemoteConnReqEvt
	HID_INT_ConnCplEvt,		    //ConnCplEvt
	HID_INT_DisconnectCplEvt,   //DisconnectCplEvt
};

static L2CAP_PROTOCOL_NODE XDATA gHID_INT_ProtocolNode = {
	&gHID_INT_ProtocolDesc,
	(L2CAP_PROTOCOL_NODE XDATA_PTR) NULL
};

extern SdpServiceNode gHID_ServiceNode;

PUBLIC BOOL HID_IsEnable(VOID)
{
    LightDbgPrint("HID_IsEnable %x", (U8)gMMI_driver_variation_nvram.misc_para.init.isMiscEnabled);
    if (gMMI_driver_variation_nvram.misc_para.init.isMiscEnabled & IS_HID_ENABLED)
    {
        return TRUE;
    }
    return FALSE;
}

PUBLIC VOID HID_Init(void)
{
    U8 i;    	

    LightDbgPrint("HID support %x", (U8)HID_IsEnable());
    if (HID_IsEnable())
    {        
        SDAP_RegisterServiceRecord(&gHID_ServiceNode);
    }
    L2CAP_RegisterProtocol(&gHID_CTL_ProtocolNode);
    L2CAP_RegisterProtocol(&gHID_INT_ProtocolNode);
    MMI_RegisterProfile(&gMMIHID_ProfileNode);

    for (i = 0; i < MAX_MULTI_POINT_NO; i++)
    {
        gHID_ctl[i].curProtocolMode = HID_PROTOCOL_MODE_REPORT;
    }

    //Load Vendor callback
    //TOdo
    gHID_VendorCB = NULL;
}

PUBLIC VOID HID_SetEnable(BOOL enabled)
{
    U8 linkIndex;

    if (HID_IsEnable() == enabled)
    {
        return;
    }
    if (enabled)
    {
        SDAP_RegisterServiceRecord(&gHID_ServiceNode);
    }
    else
    {
        for (linkIndex = 0; linkIndex < MAX_MULTI_POINT_NO; linkIndex++)
        {
            if ((gHID_channel_ctl[linkIndex].mmiProfileInfo.state) > HID_STATE_IDLE && (gHID_channel_ctl[linkIndex].mmiProfileInfo.state < HID_STATE_WAIT_INT_CHANNEL_DISCONN_CMP))
            {
                HID_DisconnectRequest(linkIndex, &gHID_channel_ctl[linkIndex].mmiProfileInfo);
            }
            // reset
            OSMEM_memset_xdata(&gMMI_HID_ctl.hidCtl[linkIndex], 0, sizeof(MmiHidCtlType));
        }        
        SDAP_UnregisterServiceRecord(PROFILE_HID);
    }
}

PUBLIC VOID HID_AclDisconnected(U8 linkIndex)
{
    MMI_HID_EventHandler(linkIndex, HID_DISCONNECTION_COMPLETE_EVENT);
    HID_Reset(linkIndex);
}

#endif
