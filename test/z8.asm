	CPU	Z8

	RP	0x00
rr	EQU	'R'
ii	EQU	'I'
ss	EQU	'S'
dd	EQU	'D'
oo	EQU	'O'
aaaa	EQU	1234H

	DEC	rr	; 00 52
	DEC	@rr	; 01 52
	ADD	R0,R1	; 02 01
	ADD	R0,@R1	; 03 01
	ADD	dd,ss	; 04 53 44
	ADD	dd,@ss	; 05 53 44
	ADD	rr,#ii	; 06 52 49
	ADD	@rr,#ii	; 07 52 49
	LD	R0,rr	; 08 52
	LD	rr,R0	; 09 52
	DJNZ	R0,.	; 0A FE
	JR	F,.	; 0B FE
	LD	R0,#ii	; 0C 49
	JP	F,aaaa	; 0D 1234
	INC	R0	; 0E
	DB	0FH	; 0F undefined

	RLC	rr	; 10 52
	RLC	@rr	; 11 52
	ADC	R0,R1	; 12 01
	ADC	R0,@R1	; 13 01
	ADC	dd,ss	; 14 53 44
	ADC	dd,@ss	; 15 53 44
	ADC	rr,#ii	; 16 52 49
	ADC	@rr,#ii	; 17 52 49
	LD	R1,rr	; 18 52
	LD	rr,R1	; 19 52
	DJNZ	R1,.	; 1A FE
	JR	LT,.	; 1B FE
	LD	R1,#ii	; 1C 49
	JP	LT,aaaa	; 1D 1234
	INC	R1	; 1E
	DB	01FH	; 1F undefined

	INC	rr	; 20 52
	INC	@rr	; 21 52
	SUB	R0,R1	; 22 01
	SUB	R0,@R1	; 23 01
	SUB	dd,ss	; 24 53 44
	SUB	dd,@ss	; 25 53 44
	SUB	rr,#ii	; 26 52 49
	SUB	@rr,#ii	; 27 52 49
	LD	R2,rr	; 28 52
	LD	rr,R2	; 29 52
	DJNZ	R2,.	; 2A FE
	JR	LE,.	; 2B FE
	LD	R2,#ii	; 2C 49
	JP	LE,aaaa	; 2D 1234
	INC	R2	; 2E
	DB	02FH	; 2F undefined

	JP	@rr	; 30 52
	SRP	#ii	; 31 49
	SBC	R0,R1	; 32 01
	SBC	R0,@R1	; 33 01
	SBC	dd,ss	; 34 53 44
	SBC	dd,@ss	; 35 53 44
	SBC	rr,#ii	; 36 52 49
	SBC	@rr,#ii	; 37 52 49
	LD	R3,rr	; 38 52
	LD	rr,R3	; 39 52
	DJNZ	R3,.	; 3A FE
	JR	ULE,.	; 3B FE
	LD	R3,#ii	; 3C 49
	JP	ULE,aaaa; 3D 1234
	INC	R3	; 3E
	DB	03FH	; 3F undefined

	DA	rr	; 40 52
	DA	@rr	; 41 52
	OR	R0,R1	; 42 01
	OR	R0,@R1	; 43 01
	OR	dd,ss	; 44 53 44
	OR	dd,@ss	; 45 53 44
	OR	rr,#ii	; 46 52 49
	OR	@rr,#ii	; 47 52 49
	LD	R4,rr	; 48 52
	LD	rr,R4	; 49 52
	DJNZ	R4,.	; 4A FE
	JR	OV,.	; 4B FE
	LD	R4,#ii	; 4C 49
	JP	OV,aaaa	; 4D 1234
	INC	R4	; 4E
	WDH		; 4F

	POP	rr	; 50 52
	POP	@rr	; 51 52
	AND	R0,R1	; 52 01
	AND	R0,@R1	; 53 01
	AND	dd,ss	; 54 53 44
	AND	dd,@ss	; 55 53 44
	AND	rr,#ii	; 56 52 49
	AND	@rr,#ii	; 57 52 49
	LD	R5,rr	; 58 52
	LD	rr,R6	; 69 52
	DJNZ	R5,.	; 5A FE
	JR	MI,.	; 5B FE
	LD	R5,#ii	; 5C 49
	JP	MI,aaaa	; 5D 1234
	INC	R5	; 5E
	WDT		; 5F

	COM	rr	; 60 52
	COM	@rr	; 61 52
	TCM	R0,R1	; 62 01
	TCM	R0,@R1	; 63 01
	TCM	dd,ss	; 64 53 44
	TCM	dd,@ss	; 65 53 44
	TCM	rr,#ii	; 66 52 49
	TCM	@rr,#ii	; 67 52 49
	LD	R6,rr	; 68 52
	LD	rr,R6	; 69 52
	DJNZ	R6,.	; 6A FE
	JR	EQ,.	; 6B FE
	LD	R6,#ii	; 6C 49
	JP	EQ,aaaa	; 6D 1234
	INC	R6	; 6E
	STOP		; 6F

	PUSH	rr	; 70 52
	PUSH	@rr	; 71 52
	TM	R0,R1	; 72 01
	TM	R0,@R1	; 73 01
	TM	dd,ss	; 74 53 44
	TM	dd,@ss	; 75 53 44
	TM	rr,#ii	; 76 52 49
	TM	@rr,#ii	; 77 52 49
	LD	R7,rr	; 78 52
	LD	rr,R7	; 79 52
	DJNZ	R7,.	; 7A FE
	JR	C,.	; 7B FE
	LD	R7,#ii	; 7C 49
	JP	C,aaaa	; 7D 1234
	INC	R7	; 7E
	HALT		; 7F

	DECW	rr	; 80 52
	DECW	@rr	; 81 52
	LDE	R4,@RR2	; 82 42
	LDEI	@R4,@RR2; 83 42
	DB	84H	; 84 undefined
	DB	85H	; 85 undefined
	DB	86H	; 86 undefined
	DB	87H	; 87 undefined
	LD	R8,rr	; 88 52
	LD	rr,R8	; 89 52
	DJNZ	R8,.	; 8A FE
	JR	.	; 8B FE
	LD	R8,#ii	; 8C 49
	JP	aaaa	; 8D 1234
	INC	R8	; 8E
	DI		; 8F

	RL	rr	; 90 52
	RL	@rr	; 91 52
	LDE	@RR4,R2	; 92 24
	LDEI	@RR4,@R2; 93 24
	DB	94H	; 94 undefined
	DB	95H	; 95 undefined
	DB	96H	; 96 undefined
	DB	97H	; 97 undefined
	LD	R9,rr	; 98 52
	LD	rr,R9	; 99 52
	DJNZ	R9,.	; 9A FE
	JR	GE,.	; 9B FE
	LD	R9,#ii	; 9C 49
	JP	GE,aaaa	; 9D 1234
	INC	R9	; 9E
	EI		; 9F

	INCW	rr	; A0 52
	INCW	@rr	; A1 52
	CP	R0,R1	; A2 01
	CP	R0,@R1	; A3 01
	CP	dd,ss	; A4 53 44
	CP	dd,@ss	; A5 53 44
	CP	rr,#ii	; A6 52 49
	CP	@rr,#ii	; A7 52 49
	LD	R10,rr	; A8 52
	LD	rr,R10	; A9 52
	DJNZ	R10,.	; AA FE
	JR	GT,.	; AB FE
	LD	R10,#ii	; AC 49
	JP	GT,aaaa	; AD 1234
	INC	R10	; AE
	RET		; AF

	CLR	rr	; B0 52
	CLR	@rr	; B1 52
	XOR	R0,R1	; B2 01
	XOR	R0,@R1	; B3 01
	XOR	dd,ss	; B4 53 44
	XOR	dd,@ss	; B5 53 44
	XOR	rr,#ii	; B6 52 49
	XOR	@rr,#ii	; B7 52 49
	LD	R11,rr	; B8 52
	LD	rr,R11	; B9 52
	DJNZ	R11,.	; BA FE
	JR	UGT,.	; BB FE
	LD	R11,#ii	; BC 49
	JP	UGT,aaaa; BD 1234
	INC	R11	; BE
	IRET		; BF

	RRC	rr	; C0 52
	RRC	@rr	; C1 52
	LDC	R4,@RR2	; C2 42
	LDCI	@R4,@RR2; C3 42
	DB	C4H	; C4 undefined
	DB	C5H	; C5 undefined
	DB	C6H	; C6 undefined
	LD	R2,oo(R1);C7 21 4F
	LD	R12,rr	; C8 52
	LD	rr,R12	; C9 52
	DJNZ	R12,.	; CA FE
	JR	NOV,.	; CB FE
	LD	R12,#ii	; CC 49
	JP	NOV,aaaa; CD 1234
	INC	R12	; CE
	RCF		; CF

	SRA	rr	; D0 52
	SRA	@rr	; D1 52
	LDC	@RR4,R2	; D2 24
	LDCI	@RR4,@R2; D3 24
	CALL	@rr	; D4 52
	DB	0D5H	; D5 undefined
	CALL	aaaa	; D6 1234
	LD	oo(R2),R1;D7 12 4F
	LD	R13,rr	; D8 52
	LD	rr,R13	; D9 52
	DJNZ	R13,.	; DA FE
	JR	PL,.	; DB FE
	LD	R13,#ii	; DC 49
	JP	PL,aaaa	; DD 1234
	INC	R13	; DE
	SCF		; DF

	RR	rr	; E0 52
	RR	@rr	; E1 52
	DB	0E2H	; E2 undefined
	LD	R2,@R1	; E3 21
	LD	dd,ss	; E4 53 44
	LD	dd,@ss	; E5 53 44
	LD	rr,#ii	; E6 52 49
	LD	@rr,#ii	; E7 52 49
	LD	R14,rr	; E8 52
	LD	rr,R14	; E9 52
	DJNZ	R14,.	; EA FE
	JR	NZ,.	; EB FE
	LD	R14,#ii	; EC 49
	JP	NZ,aaaa	; ED 1234
	INC	R14	; EE
	CCF		; EF

	SWAP	rr	; F0 52
	SWAP	@rr	; F1 52
	DB	0F2H	; F2 undefined
	LD	@R2,R1	; F3 21
	DB	0F4H	; F4 undefined
	LD	@dd,ss	; F5 53 44
	DB	0F6H	; F6 undefined
	DB	0F7H	; F7 undefined
	LD	R15,rr	; F8 52
	LD	rr,R15	; F9 52
	DJNZ	R15,.	; FA FE
	JR	NC,.	; FB FE
	LD	R15,#ii	; FC 49
	JP	NC,aaaa	; FD 1234
	INC	R15	; FE
	NOP		; FF

; instruction variations

	DEC	R0	; 00 E0
	LD	R0,R0	; 08 E0
	LD	0xE0,R0	; 09 E0 - this form does not use Ex for RP
	ADD	R0,rr	; 04 52 E0
	ADD	rr,R0	; 04 E0 52
	ADD	R0,@rr	; 05 52 E0
	ADD	rr,@R0	; 05 E0 52
	ADD	R0,#ii	; 06 E0 49
	ADD	@R0,#ii	; 07 E0 49
	INC	@R0	; 21 E0
	JP	@R0	; 30 E0
	JP	@RR0	; 30 E0
	JR	ULT,.	; 7B FE
	JP	ULT,aaaa; 7D 1234
	DECW	R0	; 80 E0
	DECW	RR0	; 80 E0
	DECW	@R0	; 81 E0
	DECW	@RR0	; 81 E0
	JR	T,.	; 8B FE
	JP	T,aaaa	; 8D 1234
	INCW	R0	; A0 E0
	INCW	RR0	; A0 E0
	INCW	@R0	; A1 E0
	INCW	@RR0	; A1 E0
	CALL	@R0	; D4 E0
	CALL	@RR0	; D4 E0
	LD	rr,@R0	; E5 E0 52
	LD	R0,@rr	; E5 52 E0
	LD	@R0,#ii	; E7 E0 49
	LD	@R0,rr	; F5 52 E0
	JR	UGE,.	; FB FE
	JP	UGE,aaaa; FD 1234

; odd register pair tests

	DEC	R0	; 00 E0
	DEC	@R0	; 01 E0
	DEC	0x00	; 00 00
	DEC	@0x00	; 01 00
	DEC	R1	; 00 E1
	DEC	@R1	; 01 E1
	DEC	0x01	; 00 01
	DEC	@0x01	; 01 01

	DECW	R0	; 80 E0
	DECW	@R0	; 81 E0
	DECW	0x00	; 80 00
	DECW	@0x00	; 81 00
	DECW	R1	; INVALID
	DECW	@R1	; INVALID
	DECW	0x01	; INVALID
	DECW	@0x01	; INVALID

	JP	@0x00	; 30 00
	JP	@0x01	; INVALID

	CALL	@0x00	; D4 00
	CALL	@0x01	; INVALID

; RP optimization tests

	RP	OFF
	ADD	rr,ss	; 04 53 52
	ADD	rr,@ss	; 05 53 52
	LD	rr,16	; E4 10 52
	LD	rr,ss	; E4 53 52
	LD	16,rr	; E4 52 10
	LD	rr,@ss	; E5 53 52
	LD	rr,@R2	; E5 E2 52
	LD	R2,@rr	; E5 52 E2
	LD	rr,#ii	; E6 52 49
	LD	@rr,ss	; F5 53 52

	RP	0x50
	ADD	rr,ss	; 02 23
	ADD	rr,@ss	; 03 23
	LD	rr,16	; 28 10
	LD	rr,ss	; could be either 28 53 or 39 52
	LD	16,rr	; 29 10
	LD	rr,@ss	; E3 23
	LD	rr,@R3	; E3 23
	LD	R2,@ss	; E3 23
	LD	rr,#ii	; 2C 49
	LD	@rr,ss	; F3 23
