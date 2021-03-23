	; this forces a 512 bytes limit for the loader!
	ORG 64856

LIB ucl_uncompress

INCLUDE "loader.opt"

main:

	; clear attributes
	xor a
	ld hl, $5800
	ld bc, 768
	ld (hl), a
	push hl
	pop de
	inc de
	ldir

	; headerless
	ld a, $ff
	ld ix, 60000
	ld de, LOADING_SIZE
	push ix
	call loader

	ld hl, $4000
	push hl
	call ucl_uncompress

	; code headerless
	ld a, $ff
	ld ix, 64856 - APP_SIZE
	ld de, APP_SIZE
	push ix
	call loader

	ld hl, LOAD_ADDR
	push hl
	call ucl_uncompress

	ei
	jp LOAD_ADDR

loader:
	scf
	jp  $0556

