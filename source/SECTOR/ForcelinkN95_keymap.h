#if defined(SELFIE_App)
#define KEY_TAP_ACTION	\
		{KEY1, KEY_ACPCALL,(KEY_MAP_MMI_HFP_INCOMMING)},														\
		{KEY1, KEY_ENDCALL, (KEY_MAP_MMI_HFP_CALLACTIVE|KEY_MAP_MMI_HFP_CALLACTIVE_WITHOUT_SCO)},				\
		{KEY5, KEY_RECONNECT_USER_INIT, (KEY_MAP_MMI_CONDISCABLE|KEY_MAP_MMI_CONNECTABLE)},						\
		{KEY3, KEY_VOICEUP,(KEY_MAP_MMI_CONNECTED|KEY_MAP_MMI_HFP_INCOMMING|									\
							KEY_MAP_MMI_HFP_OUTGOING|KEY_MAP_MMI_HFP_CALLACTIVE|								\
							KEY_MAP_MMI_HFP_CAIMG|KEY_MAP_MMI_HFP_CAOGG|										\
							KEY_MAP_MMI_HFP_CAMULTY|KEY_MAP_MMI_LINE_IN)},										\
		{KEY4, KEY_VOICEDN,(KEY_MAP_MMI_CONNECTED|KEY_MAP_MMI_HFP_INCOMMING|									\
							KEY_MAP_MMI_HFP_OUTGOING|KEY_MAP_MMI_HFP_CALLACTIVE|								\
							KEY_MAP_MMI_HFP_CAIMG|KEY_MAP_MMI_HFP_CAOGG|										\
							KEY_MAP_MMI_HFP_CAMULTY|KEY_MAP_MMI_LINE_IN)},										\
		{KEY5, KEY_CNLOUTGOING, (KEY_MAP_MMI_HFP_OUTGOING|KEY_MAP_MMI_HFP_CAOGG)},								\
		{KEY3, KEY_TEST_VOICE_PROMPT_NEXT, KEY_MAP_MMI_CONDISCABLE|KEY_MAP_MMI_CONNECTABLE},					\
		{KEY4, KEY_TEST_VOICE_PROMPT_PLAY, KEY_MAP_MMI_CONDISCABLE|KEY_MAP_MMI_CONNECTABLE},					\
		/* A2DP */																								\
		{KEY6, KEY_AVRCP_PLAY, KEY_MAP_MMI_CONNECTED},															\
		{KEY7, KEY_AVRCP_FORWARD,KEY_MAP_MMI_CONNECTED},														\
		/* 3 way calling */																						\
		{KEY6, KEY_3WAYRELNACP,(KEY_MAP_MMI_HFP_CAIMG|KEY_MAP_MMI_HFP_CAMULTY)},								\
		{KEY7, KEY_3WAYHOLDNACP, (KEY_MAP_MMI_HFP_CAIMG|KEY_MAP_MMI_HFP_CAMULTY)},								\
		/* Test mode */																							\
		{KEY1, KEY_SWITCH_TEST_MODE_STATE, KEY_MAP_MMI_TEST_MODE},												\
		{KEY3, KEY_SWITCH_TEST_MODE_CHANNEL, KEY_MAP_MMI_TEST_MODE},											\
		{KEY4, KEY_SWITCH_TEST_MODE_POWER, KEY_MAP_MMI_TEST_MODE},												\
		{KEY5, KEY_SWITCH_TEST_MODE_TX_PACKET_TYPE, KEY_MAP_MMI_TEST_MODE},										\
		{KEY6, KEY_MICUP,KEY_MAP_MMI_HFP_CALLACTIVE},															\
		{KEY7, KEY_MICDN, KEY_MAP_MMI_HFP_CALLACTIVE},															\
		/* HID */                                                                                   			\
		{KEY5, KEY_SELFIE, KEY_MAP_MMI_CONNECTED},   															\
		{KEY6, KEY_SWITCH_SELFIE, KEY_MAP_MMI_CONDISCABLE},
#else
#define KEY_TAP_ACTION	\
		{KEY1, KEY_ACPCALL,(KEY_MAP_MMI_HFP_INCOMMING)},														\
		{KEY1, KEY_ENDCALL, (KEY_MAP_MMI_HFP_CALLACTIVE|KEY_MAP_MMI_HFP_CALLACTIVE_WITHOUT_SCO)},				\
		{KEY5, KEY_RECONNECT_USER_INIT, (KEY_MAP_MMI_CONDISCABLE|KEY_MAP_MMI_CONNECTABLE)},						\
		{KEY3, KEY_VOICEUP,(KEY_MAP_MMI_CONNECTED|KEY_MAP_MMI_HFP_INCOMMING|									\
							KEY_MAP_MMI_HFP_OUTGOING|KEY_MAP_MMI_HFP_CALLACTIVE|								\
							KEY_MAP_MMI_HFP_CAIMG|KEY_MAP_MMI_HFP_CAOGG|										\
							KEY_MAP_MMI_HFP_CAMULTY|KEY_MAP_MMI_LINE_IN)},										\
		{KEY4, KEY_VOICEDN,(KEY_MAP_MMI_CONNECTED|KEY_MAP_MMI_HFP_INCOMMING|									\
							KEY_MAP_MMI_HFP_OUTGOING|KEY_MAP_MMI_HFP_CALLACTIVE|								\
							KEY_MAP_MMI_HFP_CAIMG|KEY_MAP_MMI_HFP_CAOGG|										\
							KEY_MAP_MMI_HFP_CAMULTY|KEY_MAP_MMI_LINE_IN)},										\
		{KEY5, KEY_CNLOUTGOING, (KEY_MAP_MMI_HFP_OUTGOING|KEY_MAP_MMI_HFP_CAOGG)},								\
		{KEY3, KEY_TEST_VOICE_PROMPT_NEXT, KEY_MAP_MMI_CONDISCABLE|KEY_MAP_MMI_CONNECTABLE},					\
		{KEY4, KEY_TEST_VOICE_PROMPT_PLAY, KEY_MAP_MMI_CONDISCABLE|KEY_MAP_MMI_CONNECTABLE},					\
		/* A2DP */																								\
		{KEY6, KEY_AVRCP_PLAY, KEY_MAP_MMI_CONNECTED},															\
		{KEY7, KEY_AVRCP_FORWARD,KEY_MAP_MMI_CONNECTED},														\
		/* 3 way calling */																						\
		{KEY6, KEY_3WAYRELNACP,(KEY_MAP_MMI_HFP_CAIMG|KEY_MAP_MMI_HFP_CAMULTY)},								\
		{KEY7, KEY_3WAYHOLDNACP, (KEY_MAP_MMI_HFP_CAIMG|KEY_MAP_MMI_HFP_CAMULTY)},								\
		/* Test mode */																							\
		{KEY1, KEY_SWITCH_TEST_MODE_STATE, KEY_MAP_MMI_TEST_MODE},												\
		{KEY3, KEY_SWITCH_TEST_MODE_CHANNEL, KEY_MAP_MMI_TEST_MODE},											\
		{KEY4, KEY_SWITCH_TEST_MODE_POWER, KEY_MAP_MMI_TEST_MODE},												\
		{KEY5, KEY_SWITCH_TEST_MODE_TX_PACKET_TYPE, KEY_MAP_MMI_TEST_MODE},										\
		{KEY6, KEY_MICUP,KEY_MAP_MMI_HFP_CALLACTIVE},															\
		{KEY7, KEY_MICDN, KEY_MAP_MMI_HFP_CALLACTIVE},														
#endif

		
#define KEY_PRESS_ACTION	\
		{KEY5, KEY_REJCALL, KEY_MAP_MMI_HFP_INCOMMING},															\
		{KEY5, KEY_ONHOLD_CALL, KEY_MAP_MMI_HFP_CALLACTIVE|KEY_MAP_MMI_HFP_CALLACTIVE_WITHOUT_SCO},				\
		{KEY5, KEY_MEDIA_TRIGGER_1, (KEY_MAP_MMI_HFP_CAIMG|KEY_MAP_MMI_HFP_CALLACTIVE)},						\
		{KEY_COMB_12, KEY_VOICE_PROMPT_TOGGLE,																	\
						(KEY_MAP_MMI_ALL_STATES &(~(KEY_MAP_MMI_OFF|KEY_MAP_MMI_EEPROM_UPDATED_OK|				\
													KEY_MAP_MMI_EEPROM_UPDATED_FAIL|							\
													KEY_MAP_MMI_EEPROM_UPDATING)))},							\
		{KEY6, KEY_3WAYRELNUDUB, (KEY_MAP_MMI_HFP_CAIMG|KEY_MAP_MMI_HFP_CAMULTY) },								\
		{KEY_COMB_34, KEY_VOICE_PROMPT_LANG_CHANGE, (KEY_MAP_MMI_CONDISCABLE|KEY_MAP_MMI_CONNECTED|				\
													 KEY_MAP_MMI_CONNECTABLE|KEY_MAP_MMI_TWS_PAIRING)},			\

#define KEY_PRESS_RELEASE_ACTION	\
		{KEY1, KEY_RESET,(KEY_MAP_MMI_EEPROM_UPDATING)},														\

#define KEY_LONG_PRESS_ACTION	\
		{KEY5, KEY_SET_VOICE_RECOGNITION,KEY_MAP_MMI_CONNECTED},												\
		{KEY1, KEY_POWER_ON, (KEY_MAP_MMI_FAKEOFF|KEY_MAP_MMI_FAKEON)},											\
		{KEY1, KEY_POWER_OFF, KEY_MAP_MMI_ALL_STATES & ~(KEY_MAP_MMI_OFF|KEY_MAP_MMI_FAKEOFF|					\
								KEY_MAP_MMI_EEPROM_UPDATED_OK|KEY_MAP_MMI_EEPROM_UPDATED_FAIL|					\
								KEY_MAP_MMI_EEPROM_UPDATING|KEY_MAP_MMI_DETACHING_LINK)},						\
		{KEY_COMB_14, KEY_RESET_PAIRED_DEVICES, KEY_MAP_MMI_FAKEOFF|KEY_MAP_MMI_FAKEON},						\
		{KEY_COMB_13, KEY_MULTI_A2DP_PLAY_MODE_TOGGLE, KEY_MAP_MMI_CONDISCABLE|									\
													KEY_MAP_MMI_CONNECTABLE|KEY_MAP_MMI_CONNECTED},				\
		{KEY6, KEY_VOICE_COMMAND_ENABLE, (KEY_MAP_MMI_CONDISCABLE|KEY_MAP_MMI_CONNECTABLE|						\
											KEY_MAP_MMI_CONNECTED)},											\
		{KEY4, KEY_CONTROL_DBB_ON_OFF,KEY_MAP_MMI_CONNECTED| KEY_MAP_MMI_LINE_IN},								\

#define KEY_LONG_PRESS_RELEASE_ACTION	\
		{KEY1, KEY_RECONNECT_AFTER_POWER_ON, KEY_MAP_MMI_CONNECTABLE|KEY_MAP_MMI_FAKEON},						\

#define KEY_DOUBLE_ACTION	\
		{KEY1, KEY_RDIAL, KEY_MAP_MMI_CONNECTED},																\
		{KEY5, KEY_MIC_MUTE_TOGGLE, (KEY_MAP_MMI_HFP_CALLACTIVE|KEY_MAP_MMI_HFP_CAMULTY)},						\
		/*{KEY6, KEY_AVRCP_STOP, KEY_MAP_MMI_CONNECTED},*/														\
		{KEY7, KEY_AVRCP_BACKWARD,KEY_MAP_MMI_CONNECTED},														\
		{KEY7, KEY_3WAYADD, (KEY_MAP_MMI_HFP_CAIMG|KEY_MAP_MMI_HFP_CAMULTY)},									\
		{KEY6, KEY_3WAYCALLTRANSFER, (KEY_MAP_MMI_HFP_CAIMG|KEY_MAP_MMI_HFP_CAMULTY)},							\
		//{KEY5, KEY_PEQ_MODE_CHANGE,KEY_MAP_MMI_CONNECTED| KEY_MAP_MMI_LINE_IN},								\

#define KEY_TRIPLE_ACTION	\
		{KEY1, KEY_ENTER_TESTMODE,KEY_MAP_MMI_CONDISCABLE},														\
		{KEY5, KEY_AUDIO_TRANSFER,( KEY_MAP_MMI_HFP_CALLACTIVE|KEY_MAP_MMI_HFP_INCOMMING|KEY_MAP_MMI_HFP_OUTGOING|	\
									KEY_MAP_MMI_HFP_CALLACTIVE_WITHOUT_SCO|KEY_MAP_MMI_HFP_CAIMG|				\
									KEY_MAP_MMI_HFP_CAOGG|KEY_MAP_MMI_HFP_CAMULTY)},							\

#define KEY_LONG_LONG_PRESS_ACTION	\
		{KEY1, KEY_DISCOVERABLE, KEY_MAP_MMI_CONNECTABLE|KEY_MAP_MMI_CONNECTED|									\
									KEY_MAP_MMI_CONDISCABLE},									 				\

//#define KEY_LONG_LONG_PRESS_RELEASE_ACTION

//#define KEY_EXTREMELY_LONG_PRESS_ACTION											

//#define KEY_EXTREMELY_LONG_PRESS_RELEASE_ACTION

/*#define KEY_REPEAT_ACTION	\
		{KEY3, KEY_VOICEUP,(KEY_MAP_MMI_CONNECTED|KEY_MAP_MMI_HFP_INCOMMING|									\
							KEY_MAP_MMI_HFP_OUTGOING|KEY_MAP_MMI_HFP_CALLACTIVE|								\
							KEY_MAP_MMI_HFP_CAIMG|KEY_MAP_MMI_HFP_CAOGG|										\
							KEY_MAP_MMI_HFP_CAMULTY|KEY_MAP_MMI_LINE_IN)},										\
		{KEY4, KEY_VOICEDN,(KEY_MAP_MMI_CONNECTED|KEY_MAP_MMI_HFP_INCOMMING|									\
							KEY_MAP_MMI_HFP_OUTGOING|KEY_MAP_MMI_HFP_CALLACTIVE|								\
							KEY_MAP_MMI_HFP_CAIMG|KEY_MAP_MMI_HFP_CAOGG|										\
							KEY_MAP_MMI_HFP_CAMULTY|KEY_MAP_MMI_LINE_IN)},										\ */

#define KEY_DOWN_ACTION		\
		{KEY2, KEY_POWER_ON_THEN_ENTERDISCOVERABLE, KEY_MAP_MMI_FAKEON},										\
		{KEY2, KEY_NFC_DISCOVREABLE, KEY_MAP_MMI_CONNECTABLE},													\

//#define KEY_UP_ACTION

//#define KEY_NOACT_ACTION
