?EP?P017_f1_0xFFBE69?P017 SEGMENT 'ECODE_PATCH'
PUBLIC P017_f1_0xFFBE69_1??
PUBLIC P017_f1_0xFFBE69_2??
RSEG ?EP?P017_f1_0xFFBE69?P017	;program segment

P017_f1_0xFFBE69_1??:
DB		0x68, 0xBE, 0xFF
DB		0
DB		0x2A
EJMP	P017_f1_patch

P017_f1_0xFFBE69_2??:
DB		0x6C, 0xBE, 0xFF
DB		3
EJMP	P017_f1_patch
DB		0x3E, 0x34, 0x7E

P017_f1_patch:
CLR     0x88.4
CLR     0xA8.7
MOV     0xC1,#0xC9
MOV     A,0xC2
SETB    0xA8.7
ANL     A,#0x01
JZ      P017_f1_1
SLL     WR6
P017_f1_1:
SLL     WR6
EJMP	0xFFBE6D