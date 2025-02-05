SECTORS_ENUM_MACRO(SECTOR_MMI_DRIVER_V_NVRAM_INIT2, &gMMI_Driver_v_nvram_init2, sizeof(MMI_DRIVER_VARIATION_NVRAM_TYPE), CRC_NOT_CHK)

#ifdef LE_SUPPORTED_HOST_FLASH
SECTORS_ENUM_MACRO(SECTOR_MMI_LE_V_NVRAM_INIT2, &gMMI_LE_v_nvram_init2, sizeof(NVRAM_MMI_LINK_DATA_TYPE), CRC_NOT_CHK)
#else
SECTORS_ENUM_MACRO(SECTOR_MMI_LE_V_NVRAM_INIT2, NULL, 0, CRC_NOT_CHK)
#endif

#if (defined DSP_ENABLE) && (defined DUAL_MIC_ENABLE)
SECTORS_ENUM_MACRO(SECTOR_DUAL_MIC_DATA2, &gSector_DualMicData2, sizeof(DUAL_MIC_DATA_STRU), CRC_NOT_CHK)
#else
SECTORS_ENUM_MACRO(SECTOR_DUAL_MIC_DATA2, NULL, 0, CRC_NOT_CHK)
#endif

#ifndef AIR_UPDATE_DONGLE
SECTORS_ENUM_MACRO(SECTOR_CUSTOMIZE_V_NVRAM_INIT2, &gMMI_Customize_v_nvram_init2, sizeof(CUSTOMIZE_DATA_TYPE), CRC_NOT_CHK)
#else
SECTORS_ENUM_MACRO(SECTOR_CUSTOMIZE_V_NVRAM_INIT2, NULL, 0, CRC_NOT_CHK)
#endif
