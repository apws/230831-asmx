b	EQU	42H
w	EQU	1234H

	DB	00H	; alias for HLT
	DB	01H	; alias for HLT
	RLC
	RNC
	ADI	b
	RST	0
	MVI	A,b
	RET

	INR	B
	DCR	B
	RRC
	RNZ
	ACI	b
	RST	1
	MVI	B,b
	DB	0FH	; alias for RET

	INR	C
	DCR	C
	RAL
	RP
	SUI	b
	RST	2
	MVI	C,b
	DB	17H	; alias for RET

	INR	D
	DCR	D
	RAR
	RPO
	SCI	b
	RST	3
	MVI	D,b
	DB	1FH	; alias for RET


	INR	E
	DCR	E
	DB	22H
	RC
	ANI	b
	RST	4
	MVI	E,b
	DB	27H	; alias for RET

	INR	H
	DCR	H
	DB	2AH
	RZ
	XRI	b
	RST	5
	MVI	H,b
	DB	2FH	; alias for RET

	INR	L
	DCR	L
	DB	32H
	RM
	ORI	b
	RST	6
	MVI	L,b
	DB	37H	; alias for RET

	DB	38H
	DB	39H
	DB	3AH
	RPE
	CPI	b
	RST	7
	MVI	M,b
	DB	3FH	; alias for RET

	JNC	w
	IN	0
	CNC	w
	IN	1
	JMP	w
	IN	2
	CALL	w
	IN	3

	JNZ	w
	IN	4
	CNZ	w
	IN	5
	DB	4CH	; alias for JMP
	IN	6
	DB	4EH	; alias for CALL
	IN	7

	JP	w
	OUT	8
	CP	w
	OUT	9
	DB	54H	; alias for JMP
	OUT	10
	DB	56H	; alias for CALL
	OUT	11

	JPO	w
	OUT	12
	CPO	w
	OUT	13
	DB	5CH	; alias for JMP
	OUT	14
	DB	5EH	; alias for CALL
	OUT	15

	JC	w
	OUT	16
	CC	w
	OUT	17
	DB	64H	; alias for JMP
	OUT	18
	DB	66H	; alias for CALL
	OUT	19

	JZ	w
	OUT	20
	CZ	w
	OUT	21
	DB	6CH	; alias for JMP
	OUT	22
	DB	6EH	; alias for CALL
	OUT	23

	JM	w
	OUT	24
	CM	w
	OUT	25
	DB	74H	; alias for JMP
	OUT	26
	DB	76H	; alias for CALL
	OUT	27

	JPE	w
	OUT	28
	CPE	w
	OUT	29
	DB	7CH	; alias for JMP
	OUT	30
	DB	7EH	; alias for CALL
	OUT	31

	ADD	A
	ADD	B
	ADD	C
	ADD	D
	ADD	E
	ADD	H
	ADD	L
	ADD	M

	ADC	A
	ADC	B
	ADC	C
	ADC	D
	ADC	E
	ADC	H
	ADC	L
	ADC	M

	SUB	A
	SUB	B
	SUB	C
	SUB	D
	SUB	E
	SUB	H
	SUB	L
	SUB	M

	SBB	A
	SBB	B
	SBB	C
	SBB	D
	SBB	E
	SBB	H
	SBB	L
	SBB	M

	ANA	A
	ANA	B
	ANA	C
	ANA	D
	ANA	E
	ANA	H
	ANA	L
	ANA	M

	XRA	A
	XRA	B
	XRA	C
	XRA	D
	XRA	E
	XRA	H
	XRA	L
	XRA	M

	ORA	A
	ORA	B
	ORA	C
	ORA	D
	ORA	E
	ORA	H
	ORA	L
	ORA	M

	CMP	A
	CMP	B
	CMP	C
	CMP	D
	CMP	E
	CMP	H
	CMP	L
	CMP	M

	MOV	A,A
	MOV	A,B
	MOV	A,C
	MOV	A,D
	MOV	A,E
	MOV	A,H
	MOV	A,L
	MOV	A,M

	MOV	B,A
	MOV	B,B
	MOV	B,C
	MOV	B,D
	MOV	B,E
	MOV	B,H
	MOV	B,L
	MOV	B,M

	MOV	C,A
	MOV	C,B
	MOV	C,C
	MOV	C,D
	MOV	C,E
	MOV	C,H
	MOV	C,L
	MOV	C,M

	MOV	D,A
	MOV	D,B
	MOV	D,C
	MOV	D,D
	MOV	D,E
	MOV	D,H
	MOV	D,L
	MOV	D,M

	MOV	E,A
	MOV	E,B
	MOV	E,C
	MOV	E,D
	MOV	E,E
	MOV	E,H
	MOV	E,L
	MOV	E,M

	MOV	H,A
	MOV	H,B
	MOV	H,C
	MOV	H,D
	MOV	H,E
	MOV	H,H
	MOV	H,L
	MOV	H,M

	MOV	L,A
	MOV	L,B
	MOV	L,C
	MOV	L,D
	MOV	L,E
	MOV	L,H
	MOV	L,L
	MOV	L,M

	MOV	M,A
	MOV	M,B
	MOV	M,C
	MOV	M,D
	MOV	M,E
	MOV	M,H
	MOV	M,L
	HLT

; meta-ops
	LXI	B,w
	LXI	D,w
	LXI	H,w

; should be errors
;	MOV	M,M
;	INR	A
;	DCR	A
;	INR	M
;	DCR	M
;	IN	8
;	OUT	0
;	OUT	32
