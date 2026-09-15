
.def contador = r18
.def constante = r19
.def cantidadValores = r16
.def selectorSenal = r17

.org 0x0000
rjmp start

configurar:
	ldi r20, 255
	out DDRD, r20
	ldi r20, 0xff
	out DDRB, r20
	clr r20
	out PORTC, r20
	call guardar_codigos
	ret

esperar_inicio:
	nop
	ret

start:
    ldi r16, HIGH(RAMEND)
	out SPH, r16
	ldi r16, LOW(RAMEND)
	out SPL, r16

	call configurar
	call esperar_inicio

bucle_infinito:
	call leer_derecho    
	call leer_reves      
	rjmp bucle_infinito

guardar_codigos:
	ldi r28, 0x00 ;LOW(0x0100)
	ldi r29, 0x01 ;HIGH(0x0100)
	ldi contador, 0x00
	ldi constante, 0x02
	rjmp bucle_guardar1

bucle_guardar1:
	ST Y+, contador
	cpi contador, 0xfe
	breq fin_guardar

	add contador, constante
	rjmp bucle_guardar1

fin_guardar:
	ret

leer_derecho:
	ldi r28, 0x00
	ldi r29, 0x01
	ldi cantidadValores, 128

bucle_derecho:
	ld r20, Y+
	out PORTD, r20
	dec cantidadValores
	brne bucle_derecho
	ret

leer_reves:
	ldi cantidadValores, 128

bucle_reves:
	ld r20, -Y
    out PORTD, r20      
    dec cantidadValores
    brne bucle_reves
	ret

guardar2:
	ldi r28, 0x00
	ldi r29, 0x02

	ldi r20, 0x39
	ST Y+, r20
	ldi r20, 0x0c
	ST Y+, r20
	ldi r20, 0x8c
	ST Y+, r20
	ldi r20, 0x7d
	ST Y+, r20
	ldi r20, 0x72
	ST Y+, r20
	ldi r20, 0x47
	ST Y+, r20
	ldi r20, 0x34
	ST Y+, r20
	ldi r20, 0x2c
	ST Y+, r20
	ldi r20, 0xd8
	ST Y+, r20
	ldi r20, 0x10
	ST Y+, r20
	ldi r20, 0x0f
	ST Y+, r20
	ldi r20, 0x2f
	ST Y+, r20
	ldi r20, 0x6f
	ST Y+, r20
	ldi r20, 0x77
	ST Y+, r20
	ldi r20, 0x0d
	ST Y+, r20
	ldi r20, 0x65
	ST Y+, r20
	ldi r20, 0xd6
	ST Y+, r20
	ldi r20, 0x70
	ST Y+, r20
	ldi r20, 0xe5
	ST Y+, r20
	ldi r20, 0x8e
	ST Y+, r20
	ldi r20, 0x03
	ST Y+, r20
	ldi r20, 0x51
	ST Y+, r20
	ldi r20, 0xd8
	ST Y+, r20
	ldi r20, 0xae
	ST Y+, r20
	ldi r20, 0x8e
	ST Y+, r20
	ldi r20, 0x4f
	ST Y+, r20
	ldi r20, 0x6e
	ST Y+, r20
	ldi r20, 0xac
	ST Y+, r20
	ldi r20, 0x34
	ST Y+, r20
	ldi r20, 0x2f
	ST Y+, r20
	ldi r20, 0xc2
	ST Y+, r20
	ldi r20, 0x31
	ST Y+, r20
	ldi r20, 0xb7
	ST Y+, r20
	ldi r20, 0xb0
	ST Y+, r20
	ldi r20, 0x87
	ST Y+, r20
	ldi r20, 0x16
	ST Y+, r20
	ldi r20, 0xeb
	ST Y+, r20
	ldi r20, 0x3f
	ST Y+, r20
	ldi r20, 0xc1
	ST Y+, r20
	ldi r20, 0x28
	ST Y+, r20
	ldi r20, 0x96
	ST Y+, r20
	ldi r20, 0xb9
	ST Y+, r20
	ldi r20, 0x62
	ST Y+, r20
	ldi r20, 0x23
	ST Y+, r20
	ldi r20, 0x17
	ST Y+, r20
	ldi r20, 0x74
	ST Y+, r20
	ldi r20, 0x94
	ST Y+, r20
	ldi r20, 0x28
	ST Y+, r20
	ldi r20, 0x77
	ST Y+, r20
	ldi r20, 0x33
	ST Y+, r20
	ldi r20, 0xc2
	ST Y+, r20
	ldi r20, 0x8e
	ST Y+, r20
	ldi r20, 0xe8
	ST Y+, r20
	ldi r20, 0xba
	ST Y+, r20
	ldi r20, 0x53
	ST Y+, r20
	ldi r20, 0xbd
	ST Y+, r20
	ldi r20, 0xb5
	ST Y+, r20
	ldi r20, 0x6b
	ST Y+, r20
	ldi r20, 0x88
	ST Y+, r20
	ldi r20, 0x24
	ST Y+, r20
	ldi r20, 0x57
	ST Y+, r20
	ldi r20, 0x7d
	ST Y+, r20
	ldi r20, 0x53
	ST Y+, r20
	ldi r20, 0xec
	ST Y+, r20
	ldi r20, 0xc2
	ST Y+, r20
	ldi r20, 0x8a
	ST Y+, r20
	ldi r20, 0x70
	ST Y+, r20
	ldi r20, 0xa6
	ST Y+, r20
	ldi r20, 0x1c
	ST Y+, r20
	ldi r20, 0x75
	ST Y+, r20
	ldi r20, 0x10
	ST Y+, r20
	ldi r20, 0xa1
	ST Y+, r20
	ldi r20, 0xcd
	ST Y+, r20
	ldi r20, 0x89
	ST Y+, r20
	ldi r20, 0x21
	ST Y+, r20
	ldi r20, 0x6c
	ST Y+, r20
	ldi r20, 0xa1
	ST Y+, r20
	ldi r20, 0x6c
	ST Y+, r20
	ldi r20, 0xff
	ST Y+, r20
	ldi r20, 0xca
	ST Y+, r20
	ldi r20, 0xea
	ST Y+, r20
	ldi r20, 0x49
	ST Y+, r20
	ldi r20, 0x87
	ST Y+, r20
	ldi r20, 0x47
	ST Y+, r20
	ldi r20, 0x7e
	ST Y+, r20
	ldi r20, 0x86
	ST Y+, r20
	ldi r20, 0xdb
	ST Y+, r20
	ldi r20, 0xcc
	ST Y+, r20
	ldi r20, 0xb9
	ST Y+, r20
	ldi r20, 0x70
	ST Y+, r20
	ldi r20, 0x46
	ST Y+, r20
	ldi r20, 0xfc
	ST Y+, r20
	ldi r20, 0x2e
	ST Y+, r20
	ldi r20, 0x18
	ST Y+, r20
	ldi r20, 0x38
	ST Y+, r20
	ldi r20, 0x4e
	ST Y+, r20
	ldi r20, 0x51
	ST Y+, r20
	ldi r20, 0xd8
	ST Y+, r20
	ldi r20, 0x20
	ST Y+, r20
	ldi r20, 0xc5
	ST Y+, r20
	ldi r20, 0xc3
	ST Y+, r20
	ldi r20, 0xef
	ST Y+, r20
	ldi r20, 0x80
	ST Y+, r20
	ldi r20, 0x05
	ST Y+, r20
	ldi r20, 0x3a
	ST Y+, r20
	ldi r20, 0x88
	ST Y+, r20
	ldi r20, 0xae
	ST Y+, r20
	ldi r20, 0x39
	ST Y+, r20
	ldi r20, 0x96
	ST Y+, r20
	ldi r20, 0xde
	ST Y+, r20
	ldi r20, 0x50
	ST Y+, r20
	ldi r20, 0xe8
	ST Y+, r20
	ldi r20, 0x01
	ST Y+, r20
	ldi r20, 0x86
	ST Y+, r20
	ldi r20, 0x5b
	ST Y+, r20
	ldi r20, 0x36
	ST Y+, r20
	ldi r20, 0x98
	ST Y+, r20
	ldi r20, 0x65
	ST Y+, r20
	ldi r20, 0x4e
	ST Y+, r20
	ldi r20, 0xbf
	ST Y+, r20
	ldi r20, 0x52
	ST Y+, r20
	ldi r20, 0x00
	ST Y+, r20
	ldi r20, 0xa5
	ST Y+, r20
	ldi r20, 0xfa
	ST Y+, r20
	ldi r20, 0x09
	ST Y+, r20
	ldi r20, 0x39
	ST Y+, r20
	ldi r20, 0xb9
	ST Y+, r20
	ldi r20, 0x9d
	ST Y+, r20
	ldi r20, 0x7a
	ST Y+, r20
	ldi r20, 0x1d
	ST Y+, r20
	ldi r20, 0x7b
	ST Y+, r20
	ldi r20, 0x28
	ST Y+, r20
	ldi r20, 0x2b
	ST Y+, r20
	ldi r20, 0xf8
	ST Y+, r20
	ldi r20, 0x23
	ST Y+, r20
	ldi r20, 0x40
	ST Y+, r20
	ldi r20, 0x41
	ST Y+, r20
	ldi r20, 0xf3
	ST Y+, r20
	ldi r20, 0x54
	ST Y+, r20
	ldi r20, 0x87
	ST Y+, r20
	ldi r20, 0xd8
	ST Y+, r20
	ldi r20, 0x6c
	ST Y+, r20
	ldi r20, 0x66
	ST Y+, r20
	ldi r20, 0x9f
	ST Y+, r20
	ldi r20, 0xcc
	ST Y+, r20
	ldi r20, 0xbf
	ST Y+, r20
	ldi r20, 0xe0
	ST Y+, r20
	ldi r20, 0xe7
	ST Y+, r20
	ldi r20, 0x3d
	ST Y+, r20
	ldi r20, 0x7e
	ST Y+, r20
	ldi r20, 0x73
	ST Y+, r20
	ldi r20, 0x20
	ST Y+, r20
	ldi r20, 0xad
	ST Y+, r20
	ldi r20, 0x0a
	ST Y+, r20
	ldi r20, 0x75
	ST Y+, r20
	ldi r20, 0x70
	ST Y+, r20
	ldi r20, 0x03
	ST Y+, r20
	ldi r20, 0x24
	ST Y+, r20
	ldi r20, 0x1e
	ST Y+, r20
	ldi r20, 0x75
	ST Y+, r20
	ldi r20, 0x22
	ST Y+, r20
	ldi r20, 0x10
	ST Y+, r20
	ldi r20, 0xa9
	ST Y+, r20
	ldi r20, 0x24
	ST Y+, r20
	ldi r20, 0x79
	ST Y+, r20
	ldi r20, 0x8e
	ST Y+, r20
	ldi r20, 0xf8
	ST Y+, r20
	ldi r20, 0x6d
	ST Y+, r20
	ldi r20, 0x43
	ST Y+, r20
	ldi r20, 0xf2
	ST Y+, r20
	ldi r20, 0x7c
	ST Y+, r20
	ldi r20, 0xf2
	ST Y+, r20
	ldi r20, 0xd0
	ST Y+, r20
	ldi r20, 0x61
	ST Y+, r20
	ldi r20, 0x30
	ST Y+, r20
	ldi r20, 0x31
	ST Y+, r20
	ldi r20, 0xdc
	ST Y+, r20
	ldi r20, 0xb5
	ST Y+, r20
	ldi r20, 0xd8
	ST Y+, r20
	ldi r20, 0xd2
	ST Y+, r20
	ldi r20, 0xef
	ST Y+, r20
	ldi r20, 0x1b
	ST Y+, r20
	ldi r20, 0x32
	ST Y+, r20
	ldi r20, 0x1f
	ST Y+, r20
	ldi r20, 0xce
	ST Y+, r20
	ldi r20, 0xad
	ST Y+, r20
	ldi r20, 0x37
	ST Y+, r20
	ldi r20, 0x7f
	ST Y+, r20
	ldi r20, 0x62
	ST Y+, r20
	ldi r20, 0x61
	ST Y+, r20
	ldi r20, 0xe5
	ST Y+, r20
	ldi r20, 0x47
	ST Y+, r20
	ldi r20, 0xd8
	ST Y+, r20
	ldi r20, 0x5d
	ST Y+, r20
	ldi r20, 0x8e
	ST Y+, r20
	ldi r20, 0xec
	ST Y+, r20
	ldi r20, 0x7f
	ST Y+, r20
	ldi r20, 0x26
	ST Y+, r20
	ldi r20, 0xe2
	ST Y+, r20
	ldi r20, 0x32
	ST Y+, r20
	ldi r20, 0x19
	ST Y+, r20
	ldi r20, 0x07
	ST Y+, r20
	ldi r20, 0x2f
	ST Y+, r20
	ldi r20, 0x79
	ST Y+, r20
	ldi r20, 0x55
	ST Y+, r20
	ldi r20, 0xd0
	ST Y+, r20
	ldi r20, 0xf8
	ST Y+, r20
	ldi r20, 0xf6
	ST Y+, r20
	ldi r20, 0x6d
	ST Y+, r20
	ldi r20, 0xcd
	ST Y+, r20
	ldi r20, 0x1e
	ST Y+, r20
	ldi r20, 0x54
	ST Y+, r20
	ldi r20, 0xc2
	ST Y+, r20
	ldi r20, 0x01
	ST Y+, r20
	ldi r20, 0xc7
	ST Y+, r20
	ldi r20, 0x87
	ST Y+, r20
	ldi r20, 0xe8
	ST Y+, r20
	ldi r20, 0x92
	ST Y+, r20
	ldi r20, 0xd8
	ST Y+, r20
	ldi r20, 0xf9
	ST Y+, r20
	ldi r20, 0x4f
	ST Y+, r20
	ldi r20, 0x61
	ST Y+, r20
	ldi r20, 0x97
	ST Y+, r20
	ldi r20, 0x6f
	ST Y+, r20
	ldi r20, 0x1d
	ST Y+, r20
	ldi r20, 0x1f
	ST Y+, r20
	ldi r20, 0xa0
	ST Y+, r20
	ldi r20, 0x1d
	ST Y+, r20
	ldi r20, 0x19
	ST Y+, r20
	ldi r20, 0xf4
	ST Y+, r20
	ldi r20, 0x50
	ST Y+, r20
	ldi r20, 0x1d
	ST Y+, r20
	ldi r20, 0x29
	ST Y+, r20
	ldi r20, 0x5f
	ST Y+, r20
	ldi r20, 0x23
	ST Y+, r20
	ldi r20, 0x22
	ST Y+, r20
	ldi r20, 0x78
	ST Y+, r20
	ldi r20, 0xce
	ST Y+, r20
	ldi r20, 0x3d
	ST Y+, r20
	ldi r20, 0x7e
	ST Y+, r20
	ldi r20, 0x14
	ST Y+, r20
	ldi r20, 0x29
	ST Y+, r20
	ldi r20, 0xd6
	ST Y+, r20
	ldi r20, 0xa1
	ST Y+, r20
	ldi r20, 0x85
	ST Y+, r20
	ldi r20, 0x68
	ST Y+, r20
	ldi r20, 0xa0
	ST Y+, r20
	ldi r20, 0x7a
	ST Y+, r20
	ldi r20, 0x87
	ST Y+, r20
	ldi r20, 0xca
	ST Y+, r20
	ldi r20, 0x43
	ST Y+, r20
	ldi r20, 0x99
	ST Y+, r20
	ldi r20, 0xea
	ST Y+, r20
	ldi r20, 0xa1
	ST Y+, r20
	ldi r20, 0x25
	ST Y+, r20
	ldi r20, 0x04
	ST Y+, r20

	ret
	