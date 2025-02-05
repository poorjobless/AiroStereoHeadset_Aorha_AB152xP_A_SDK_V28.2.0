/*
 * AIROHA Bluetooth Device Firmware
 * COPYRIGHT (C) 2014 AIROHA TECHNOLOGY CORP. CO., LTD. ALL RIGHTS RESERVED
 *
 * sector_sys_local_device_control.c defines the control options of device.
 *
 * Programmer : CharlesSu@airoha.com.tw, Ext.2882
 */
#include "sector_sys_local_device_control.h"
#include "bt_config_profile.h"

#pragma userclass (HCONST = CONFIG)


////////////////////////////////////////////////////////////////////////////////
// Global Variables ////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
SECTOR_SYS_LOCAL_DEVICE_CONTROL_STRU CODE gSector_SysLocalDeviceControl =
{
    {
        0x0014,     //- os_power_saving_waiting_timer_in_half_slot;

        #ifdef HID_SOC
    //- SYS_MISC_SUPP_FEA0_HID_TESTMODE_SUPPORT_3_5_EDR |
        #endif
        #ifdef HEADSET_SOC
    //  SYS_MISC_SUPP_FEA0_CUSTOMER_CHARGER         |
        #endif

        #ifdef OPERATION_IN_BB_FPGA_VERSION
        #endif
        0x00,       //- misc_ctl0;

        #ifdef HID_SOC
        #endif
        #if (defined OPERATION_IN_BB_ASIC_VERSION)
        #endif
        0x00,       //- misc_ctl1;

        //- misc_ctl2
        //SYS_MISC_SUPP_FEA2_FORCE_BT_CONTROLLER_MODE |
        SYS_MISC_SUPP_FEA2_NOT_ALLOW_RST_BY_HCI_CMD |
        0x00,

        //- GPIO
        #if defined (OPERATION_IN_BB_FPGA_VERSION)
            //- Choose PCM Interface
            #ifdef HID_IIM_SOC
            {0x00, 0x00, 0x00, 0x00, 0x30, 0x40, 0xE4 | 0x04, 0x08, 0xCF, 0x81, 0x13, 0x03, 0x00, 0x00, 0x01},
            #elif defined (APP_PJ001_MOUSE)
            {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08 | 0x04, 0x10, 0xC7, 0x01, 0x10, 0x07, 0x12, 0x00, 0x01},
            #elif defined (HID_SOC)
            {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08 | 0x04, 0x00, 0xC7, 0x01, 0x10, 0x03, 0x12, 0x00, 0x01},
            #elif defined (STEREO_SOC)
            {0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x06, 0x00, 0x6E, 0x07, 0x00, 0x00, 0x20, 0x30, 0x01},
            #else
            {0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x06, 0x00, 0xcE, 0x00, 0x00, 0x00, 0x20, 0x30, 0x01},
            #endif
        #else //- OPERATION_IN_BB_ASIC_VERSION
            #ifdef HID_IIM_SOC
            //{0x00, 0x00, 0x00, 0x00, 0x30, 0x40, 0xE4, 0x08, 0xCF, 0x81, 0x13, 0x00, 0x00, 0x00},
            {0x30, 0x00, 0x00, 0x00, 0x70, 0xC0, 0x0E | 0x04, 0x00, 0x8F, 0x01, 0xF0, 0x07, 0x00, 0x00, 0x01},//for led input first
            #elif defined (APP_PJ001_MOUSE)
            {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08 | 0x04, 0x10, 0xC7, 0x01, 0x10, 0x04, 0x12, 0x00, 0x01},
            #elif defined (HID_SOC)
            {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08 | 0x04, 0x00, 0xC7, 0x01, 0x10, 0x00, 0x12, 0x00, 0x01},
            #elif defined (STEREO_SOC)
            //{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0xCF, 0x07, 0x00, 0x00, 0x20, 0x30, 0x01},
				#ifdef ASIC_DBG_PORT
				{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0e, 0x10, 0x00, 0x00, 0xC4, 0x00, 0x01},//-ice
				#elif defined TWS_SETTINGS && defined BLUETOOTH_SPEAKER
				    #ifdef SELFIE_App
				        #ifdef PRODUCT_TYPE_A					
    					{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3e, 0x08, 0x00, 0x00, 0xC4, 0x00, 0x01},//-ice
    					#else
    					{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1e, 0x08, 0x00, 0x00, 0xC4, 0x00, 0x01},//-ice
    					#endif
				    #else				    
    					#ifdef PRODUCT_TYPE_A					
    					{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1e, 0x08, 0x00, 0x00, 0xC4, 0x00, 0x01},//-ice
    					#else
    					{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1e, 0x08, 0x00, 0x00, 0xC4, 0x00, 0x01},//-ice
    					#endif
					#endif
				#elif defined TWS_SETTINGS
				{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01},//AiroStereo Headset
                #elif defined BLUETOOTH_SPEAKER
					#ifdef SOUND_BAR
					    #ifdef SELFIE_App
					        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3e, 0x08, 0x00, 0x00, 0xC4, 0x00, 0x01},//-ice
					    #else
    					    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2e, 0x08, 0x00, 0x00, 0xC4, 0x00, 0x01},//-ice
					    #endif
					#else
					    #ifdef SELFIE_App
					        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3e, 0x08, 0x00, 0x00, 0xC4, 0x00, 0x01},//-ice
					    #else
        					{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2e, 0x00, 0x00, 0x00, 0xC4, 0x00, 0x01},//-ice
					    #endif
					#endif
                #elif defined TRSPX_App
				{0x00, 0x00, 0x04, 0x00, 0x00, 0x01, 0x04, 0x00, 0x0E, 0x10, 0x00, 0x00, 0x00, 0x00, 0x01},//-ice  wp gpio18
				#else
                {0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x04, 0x00, 0x3E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01},//-ice  wp gpio18	//AB1520 SERIES EVK V2
                //{0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x01, 0x00, 0xce, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01},//-ice  wp gpio16
                //{0x00, 0x00, 0x00, 0x00, 0xF0, 0xFF, 0xFF, 0xFF, 0x0e, 0x00, 0x00, 0x00, 0x00, 0x03, 0x01},//-ice and tpout_sel = 30
                #endif
            #elif defined (HEADSET_SOC)
            {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 | 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x08, 0x00, 0x01},
            #else
                #ifdef VIRTEX6
                {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x01},
                #else
                {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x01},
                #endif
            #endif
        #endif

        0, //- reserved2;
        0, //reserved3


        {0,0,0},

        0x04,       //reserved;
        4, //U8 WatchDogTimer  //unit: 1s

        0x00,  // powerOnLedEnable
        {
        {  //0 : //Fast Blue Flash
            {0, 0, 0, 0, 0, 0,  0, LED_OFF, 0, 0},
            {0, (0xf0|0x02), 50, 50, 0, 0, 0, LED_ON, 0, 0},
            {0, 0, 0, 0, 0, 0,  0, LED_OFF, 0, 0},
            {0, 0, 0, 0, 0, 0,  0, LED_OFF, 0, 0},
        }
        },
        0x00,  //powerOnLedDisable

		/* Only work when ASIC_DBG_PORT is defined */
        #ifdef ASIC_DBG_PORT
		0, //tpout_sel
        {
            {	/* Do not modify, only for debug use!!!!!! */
                0x00, 0x00, 0x00,  //grp_sel_b0, grp_sel_b1, grp_sel_b2,
                0x90, 0x99, 0x09, 0x00, //sig_sel_0304, sig_sel_0506, sig_sel_0708, sig_sel_0910,
                0x00, 0x90, 0x99, 0x09, //sig_sel_1112, sig_sel_1314, sig_sel_1516, sig_sel_1718,
                0x00, 0x00, 0x00, 0x00, //sig_sel_1920, sig_sel_2122, sig_sel_2324, sig_sel_25,
                0x10, 0x00, //dbg_sel_0to3, dbg_sel_4,
                0x00, //U8 led_scl_en;
                0x00, 0x00, //U8 dmic_di0_sel, U8 dmic_di1_sel;
                0x00, 0x00, //U8 i2s0_di_sel; U8 i2s1_di_sel;
                0x00, //U8 uart_ncts_sel;
            }, //gpio_mux_sel
        }
        #elif defined KB_App
        0, //tpout_sel
        {
            {	/* Do not modify, only for debug use!!!!!! */
                0x00, 0x00, 0x00,  //grp_sel_b0, grp_sel_b1, grp_sel_b2,
                0x00, 0x00, 0x00, 0x00, //sig_sel_0304, sig_sel_0506, sig_sel_0708, sig_sel_0910,
                0x00, 0x70, 0x00, 0x00, //sig_sel_1112, sig_sel_1314, sig_sel_1516, sig_sel_1718,
                0x00, 0x00, 0x00, 0x00, //sig_sel_1920, sig_sel_2122, sig_sel_2324, sig_sel_25,
                0x00, 0x00, //dbg_sel_0to3, dbg_sel_4,
                0x00, //U8 led_scl_en;
                0x00, 0x00, //U8 dmic_di0_sel, U8 dmic_di1_sel;
                0x00, 0x00, //U8 i2s0_di_sel; U8 i2s1_di_sel;
                0x0F, //U8 uart_ncts_sel;
            }, //gpio_mux_sel
        }
        #endif
    },

    //- CRC
    {0,0}
};