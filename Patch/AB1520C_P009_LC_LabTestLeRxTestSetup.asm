?EP?P009_f1_0xFF799C?P009 SEGMENT 'ECODE_FLASH'
PUBLIC P009_f1_0xFF799C_1??
PUBLIC P009_f1_0xFF799C_2??	
RSEG ?EP?P009_f1_0xFF799C?P009	;program segment

P009_f1_0xFF799C_1??:
DB		0x9C, 0x79, 0xFF
DB		0
DB		0x1C
EJMP	P009_f1_patch

P009_f1_0xFF799C_2??:
DB		0xA0, 0x79, 0xFF
DB		3
EJMP	P009_f1_patch
DB		0x37, 0x80, 0x66

P009_f1_patch:
ECALL    0xFE0520 //LC_SetupRxBuffer

MOV      0xC1,#0x2B
MOV      0xE7,#0x0B

EJMP     0xFF79B5