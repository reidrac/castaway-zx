;prdr beeper music engine by Shiru, 08'15
;two channels of periodic noise-based tone with configurable period and slide down
;
; ld hl, ptr_music_data
; call prdr_play

	org $c7c0

.prdr_play

	di
	ld (ptr + 1),hl

	xor a
	ld (enableA + 1),a
	ld (enableB + 1),a

	ld b,a
	ld c,a
	ld d,a
	ld e,a
	ld h,a
	ld l,a

	inc a
	ld (periodA + 1),a
	ld (periodB + 1),a
	ld a,12
	ld (slideEnableA + 1),a
	ld (slideEnableB + 1),a

	exx
	push hl

.ptr
	ld hl,0

	ld b,16
	ld ixl,0

.readRow

	xor a
	in a,($fe)
	cpl
	and $31
	jp nz,stopPlayer

	ld c,(hl)
	inc hl

	rl c
	jr nc,noLoop
	ld a,(hl)
	inc hl
	ld h,(hl)
	ld l,a
	inc h
	jp z,stopPlayer
	dec h
	jp readRow

.noLoop

	rl c
	jr nc,noSpeed
	ld a,(hl)
	inc hl
	ld (speed + 2),a

.noSpeed

	rl c
	jr nc,noNote1
	ld a,(hl)
	inc hl
	ld (freqA + 1),a
	or a
._skip0
	jr z,_skip0+3
	ld a,b
	ld (enableA + 1),a

.noNote1

	rl c
	jr nc,noNote2
	ld a,(hl)
	inc hl
	ld (freqB + 1),a
	or a
._skip1
	jr z,_skip1+3
	ld a,b
	ld (enableB + 1),a

.noNote2

	rl c
	jr nc,noPeriod1
	ld a,(hl)
	inc hl
	ld (periodA + 1),a

.noPeriod1

	rl c
	jr nc,noPeriod2
	ld a,(hl)
	inc hl
	ld (periodB + 1),a

.noPeriod2

	rl c
	jr nc,noSlide1
	ld a,(hl)
	inc hl
	ld (slideA + 1),a
	or a
	ld a,12
._skip2
	jr z,_skip2+3
	xor a
	ld (slideEnableA + 1),a

.noSlide1

	rl c
	jr nc,noSlide2
	ld a,(hl)
	inc hl
	ld (slideB + 1),a
	or a
	ld a,12
._skip3
	jr z,_skip3+3
	xor a
	ld (slideEnableB + 1),a

.noSlide2

	exx

.speed
	ld ixh,100

.soundLoop

	dec b			;4
	jp nz,skip1		;10
.freqB
	ld b,0			;7
	inc l			;4
.back1

	ld a,l			;4
.periodB
	and 0			;7
	ld l,a			;4
	ld a,(hl)		;7
.enableB
	and 16			;7

	dec c			;4
	jr nz,skip2		;10
.freqA
	ld c,0			;7
	inc e			;4
.back2

	out ($fe),a		;11 moved here to create different volume levels
	nop				;4	for 8t alignment

	ld a,e			;4
.periodA
	and 0			;7
	ld e,a			;4
	ld a,(de)		;7
.enableA
	and 16			;7
	out ($fe),a		;11

	dec ixl			;8
	jr nz,soundLoop	;7/12=152t

.slideEnableA
	jr slideEnableA+14

	ld a,(freqA + 1)
.slideA
	sub 0
._skip4
	jr c,_skip4+4
	ld a,255
	ld (freqA + 1),a

.slideEnableB
	jr slideEnableB+14

	ld a,(freqB + 1)
.slideB
	sub 0
._skip5
	jr c,_skip5+4
	ld a,255
	ld (freqB + 1),a

	dec ixh
	jp nz,soundLoop

	exx

	jp readRow

.skip1
	jp back1
.skip2
	jp back2

.stopPlayer

	pop hl
	exx
	ei
	ret

