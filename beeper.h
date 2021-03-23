#ifndef _BEEPER_H
#define _BEEPER_H

#define FX_NONE		0
#define FX_SELECT	1
#define FX_HIT	    2
#define FX_EXPLO	3
#define FX_FIRE		4
#define FX_DAMAGE	5
#define FX_BIP		6
#define FX_ITEM		7

uchar play_queue = FX_NONE;

void __FASTCALL__ playfx(uchar fx);

#asm
;BeepFX player by Shiru
;You are free to do whatever you want with this code

._playfx
	ld a,l
	dec a ; FX go for 1 to n, but we are zero based

	ld hl,sfxData	;address of sound effects data

	di
	push ix
	push iy

	ld b,0
	ld c,a
	add hl,bc
	add hl,bc
	ld e,(hl)
	inc hl
	ld d,(hl)
	push de
	pop ix			;put it into ix

	ld a,(23624)	;get border color from BASIC vars to keep it unchanged
	rra
	rra
	rra
	and 7
	ld (sfxRoutineToneBorder  +1),a
	ld (sfxRoutineNoiseBorder +1),a


.readData
	ld a,(ix+0)		;read block type
	ld c,(ix+1)		;read duration 1
	ld b,(ix+2)
	ld e,(ix+3)		;read duration 2
	ld d,(ix+4)
	push de
	pop iy

	dec a
	jr z,sfxRoutineTone
	dec a
	jr z,sfxRoutineNoise
	pop iy
	pop ix
	ei
	ret

.nextData
	add ix,bc		;skip to the next block
	jr readData

;generate tone with many parameters

.sfxRoutineTone
	ld e,(ix+5)		;freq
	ld d,(ix+6)
	ld a,(ix+9)		;duty
	ld (sfxRoutineToneDuty+1),a
	ld hl,0

.sfxRT0
	push bc
	push iy
	pop bc
.sfxRT1
	add hl,de
	ld a,h
.sfxRoutineToneDuty
	cp 0
	sbc a,a
	and 16
.sfxRoutineToneBorder
	or 0
	out (254),a

	dec bc
	ld a,b
	or c
	jr nz,sfxRT1

	ld a,(sfxRoutineToneDuty+1)	 ;duty change
	add a,(ix+10)
	ld (sfxRoutineToneDuty+1),a

	ld c,(ix+7)		;slide
	ld b,(ix+8)
	ex de,hl
	add hl,bc
	ex de,hl

	pop bc
	dec bc
	ld a,b
	or c
	jr nz,sfxRT0

	ld c,11
	jr nextData

;generate noise with two parameters

.sfxRoutineNoise
	ld e,(ix+5)		;pitch

	ld d,1
	ld h,d
	ld l,d
.sfxRN0
	push bc
	push iy
	pop bc
.sfxRN1
	ld a,(hl)
	and 16
.sfxRoutineNoiseBorder
	or 0
	out (254),a
	dec d
	jr nz,sfxRN2
	ld d,e
	inc hl
	ld a,h
	and 31
	ld h,a
.sfxRN2
	dec bc
	ld a,b
	or c
	jr nz,sfxRN1

	ld a,e
	add a,(ix+6)	;slide
	ld e,a

	pop bc
	dec bc
	ld a,b
	or c
	jr nz,sfxRN0

	ld c,7
	jr nextData


.sfxData

.SoundEffectsData
	defw SoundEffect0Data
	defw SoundEffect1Data
	defw SoundEffect2Data
	defw SoundEffect3Data
	defw SoundEffect4Data
	defw SoundEffect5Data
	defw SoundEffect6Data

.SoundEffect0Data
	defb 1 ;tone
	defw 4,1000,1200,200,128
	defb 0
.SoundEffect1Data
	defb 2 ;noise
	defw 10,100,128
	defb 0
.SoundEffect2Data
	defb 2 ;noise
	defw 6,300,805
	defb 0
.SoundEffect3Data
	defb 1 ;tone
	defw 6,400,64,256,128
	defb 0
.SoundEffect4Data
	defb 2 ;noise
	defw 1,1000,10
	defb 2 ;noise
	defw 1,2000,16
	defb 0
.SoundEffect5Data
	defb 1 ;tone
	defw 1,100,1000,400,128
	defb 0
.SoundEffect6Data
	defb 1 ;tone
	defw 4,600,1000,400,128
	defb 0

#endasm

#endif

