; UCLZ80:	                                              -*- asm -*-
; 2bunpack: unpack data compressed with M.F.X.J. Oberhumer''s
; UCL/2B algorithm.  Decompressor for Z80 (c) 2000 Adam D. Moss
; <adam@foxbox.org> <adam@gimp.org> - All rights reserved
;
; Version 0.99-beta2: 2000-10-25
; ** COMMENTS, FIXES ETC ARE WELCOMED ** + If you use this, I''d love to hear.
;
; Permission is hereby granted, free of charge, to any person obtaining a
; copy of this software and associated documentation files (the "Software"),
; to deal in the Software without restriction, including without limitation
; the rights to use, copy, modify, merge, publish, distribute, sublicense,
; and/or sell copies of the Software, and to permit persons to whom the
; Software is furnished to do so, subject to the following conditions:
;
; The above copyright notice and this permission notice shall be included
; in all copies or substantial portions of the Software.
;
; THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
; OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
; FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
; THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
; IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
; CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
;
; (In laymans'' terms this code is essentially public domain.)
;
; Latest versions and docs, plus pointers to associated utilities and
; M.F.X.J. Oberhumer''s *compressor* for UCL/2B streams are always
; available from the UCLZ80 home page:
;     <http://www.foxbox.org/adam/code/uclz80/>
;
;
; UPDATE: http://icculus.org/~aspirin/uclz80/
;
; RELEASE HISTORY:
;	v0.99-beta2: 2000-10-25: Rearrangement, comments, public release
;	v0.99-beta1: 2000-07-30: Optimizations, comments and fixes
;	v0.50      : 2000-07-22: Initial version

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Assembly syntax compatible with ZMAC and ASL cross-assemblers.
; Undocumented Z80 instructions have been inserted in byte form.

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; CALL:
;	L2bunpack
; INPUTS:
;	bc = PACKED DATA SOURCE ADDRESS
;	de = UNPACKED DATA DESTINATION ADDRESS
; STACK USAGE:
;	2 bytes excluding call overhead
; REGISTER USAGE:
;	leaves IY, IXH and all alternative registers intact
; SIZE:
;	245 bytes
;
; NOTES:
;	''In-place'' unpacking of overlapping src/dest is also supported;
;	use external utility to find the maximum overlap for a given
;	block of data ((UNPACKED_SIZE - n) where n is usually around 7-10
;	bytes).
;
;	No checking of data integrity is performed by this unpacker.
;
;	The length of the unpacked data is implicit in the packed stream;
;	you should know in advance the length to which a given compressed
;	stream unpacks (ie. from out-of-band metadata), to provide enough
;	space in the destination memory area.

XLIB ucl_uncompress

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; loops taken out of line
L2bloop1_0:   ; DON''T CALL HERE, these functions are just here for JR-locality
	;;;; bb = *src, src++
	ld	a,	(bc)	; get *src
	inc	bc		; src++
	scf
	rla			; ADM: could use undoc''d SLL
	defb	0DDh,6Fh	; LD IXL,A
	
	jr	C,	L2bloop1_work	; we carried, do the work -ADM:GetStats
	jp	L2bloop1_end		; otherwise drop out

L2bloop2a_0:
	;;;; bb = *src, src++
	ld	a,	(bc)	; get *src
	inc	bc		; src++
	scf
	rla			; ADM: could use undoc''d SLL
	defb	0DDh,6Fh	; LD IXL,A
	
	jr	NC,	L2bloop2a_skip_set
	jp	L2bloop2a_set
	
L2bloop2b_0:
	;;;; bb = *src, src++
	ld	a,	(bc)	; get *src
	inc	bc		; src++
	scf
	rla			; ADM: could use undoc''d SLL
	defb	0DDh,6Fh	; LD IXL,A
	
	jr	C,	L2bloop2b_end	; if !bit then end loop 
	jp	L2bloop2a		; if bit then restart big loop


L2bjmp3_0:
	;;;; bb = *src, src++
	ld	a,	(bc)	; get *src
	inc	bc		; src++
	scf
	rla			; ADM: could use undoc''d SLL
	defb	0DDh,6Fh	; LD IXL,A

	jp	L2bjmp3_r

L2b_term:		; springboard placed here to be within JR reach
	jp	L2b_end

;;;
;;; my EP
ucl_uncompress:
	pop hl
	pop de
	pop bc
	push hl

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;	
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; CALL HERE:
L2bunpack:
	; init variables
	defb	0DDh,2Eh,00h	; LD IXL, 0; bit-bucket lives in IXL
	ld	hl,	1
	ld	(prev_doffset),	hl

L2bmain:
L2bloop1:		;;;; while nextbit()==1 *dest++ = *src++;
	;;;; CF = nextbit()
	defb	0DDh,7Dh	; LD A, IXL
	add	a,	a
	jr	Z,	L2bloop1_0 ; ran out of bits: load new byte
	defb	0DDh,6Fh	; LD IXL,A
	; bit now in C flag
	jr	NC,	L2bloop1_end	; nextbit()==0, end loop ; STAT
L2bloop1_work:
	; nextbit() == 1, so *dest++ = *src++;
	ld	a,	(bc)
	ld	(de),	a
	inc	de
	inc	bc
	jp	L2bloop1
L2bloop1_end:

	
	ld	hl,	1	;;	doffset = 1
	; doffset lives in HL until further notice!
	
L2bloop2a:
	add	hl,	hl
	jr	C,	L2b_term	; terminator found. ; STAT
	
	;;;; CF = nextbit()
	defb	0DDh,7Dh	; LD A, IXL
	add	a,	a
	jr	Z,	L2bloop2a_0 ; ran out of bits: load new byte
	defb	0DDh,6Fh	; LD IXL,A
	; bit now in C flag
	jr	NC,	L2bloop2a_skip_set	; STAT
L2bloop2a_set:
	inc	hl		;; doffset++
L2bloop2a_skip_set:

	;;;; CF = nextbit()
	defb	0DDh,7Dh	; LD A, IXL
	add	a,	a
	jr	Z,	L2bloop2b_0 ; ran out of bits: load new byte
	defb	0DDh,6Fh	; LD IXL,A
	; bit now in C flag
	jr	NC,	L2bloop2a ; if nextbit() loop again	; STAT
L2bloop2b_end:


	ld	a,	l
	sub	2
	or	h
	jr	NZ,	L2bjmp3		; doffset != 2 then jump ; STAT
	ld	hl,	(prev_doffset)	;; doffset = prev_doffset
	jp	L2bjmp3_end
L2bjmp3:
	dec	l
	dec	l
	dec	l		; hl is doffset again
	ld	h,	l	; *=256, now put something in l
	ld	a,	(bc)
	inc	bc		;; ilen++
	ld	l,	a
	inc	hl		;; doffset++
	ld	(prev_doffset),hl
L2bjmp3_end:

	ld	(doffset),hl
	; doffset committed to memory.

		
	;;;; CF = nextbit()
	defb	0DDh,7Dh	; LD A, IXL
	add	a,	a
	jr	Z,	L2bjmp3_0 ; ran out of bits: load new byte
	defb	0DDh,6Fh	; LD IXL,A
L2bjmp3_r:
	; bit now in C flag
	ld	hl,	0
	jp	NC,	L2bjmp3_skipset		; STAT
	ld	l,	2	;; movlen = 1, movlen *= 2
L2bjmp3_skipset:


	;;;; CF = nextbit()
	defb	0DDh,7Dh	; LD A, IXL
	add	a,	a
	jr	Z,	L2bjmp4_0 ; ran out of bits: load new byte
	defb	0DDh,6Fh	; LD IXL,A
	; bit now in C flag
	jr	NC,	L2bjmp4_zero		; STAT
L2bjmp4_nonzero:
	; movlen in HL (well, only low 2 L bits have stuff)
	inc	hl
L2bjmp4_zero:

	
	xor	a
	or	l		; movlen == 0?
	jr	NZ,	L2bjmp8_movlen_nonzero	; STAT

	; so movlen == 0
	inc	l	;; movlen = 1

	
L2bloop6:
	add	hl,	hl	;; movlen *= 2
	
	;;;; CF = nextbit()
	defb	0DDh,7Dh	; LD A, IXL
	add	a,	a
	jr	Z,	L2bloop6_0	; ran out of bits: load new byte
	defb	0DDh,6Fh	; LD IXL,A
	; bit now in C flag
	jr	NC,	L2bloop6_zero	; STAT
L2bloop6_nonzero:
	inc	hl;(hl)
L2bloop6_zero:
	;;;; CF = nextbit()
	defb	0DDh,7Dh	; LD A, IXL
	add	a,	a
	jr	Z,	L2bloop7_0	; ran out of bits: load new byte
	defb	0DDh,6Fh	; LD IXL,A
	; bit now in C flag
	jr	NC,	L2bloop6	; go around loop again ; STAT
L2bloop7_nonzero:
	; !!nextbit(), loop broken
	inc	hl
	inc	hl
		

L2bjmp8_movlen_nonzero:
L2bjmp8a_0d00_compare:
	
	push	bc

	ld	bc,	(doffset)	;20
	xor	a,	a		;4
	sub	a,	c		;4
	ld	a,	0Dh		;7
	sbc	a,	b		;4 == 42
	
	jp	NC,	L2bjmp9_noadd ; STAT

	; so (doffset > 0x0d00)
	inc	hl		;; movlen++
	
L2bjmp9_noadd:
	; movlen is in HL
	
	; reshuffle registers to be LDIR-friendly
	;want dest (de) in de
	;want src (de-doffset) in hl
	;want movlen (hl) in bc

	ld	b,	h		;4
	ld	c,	l		;4
	ld	hl,	(doffset)	;20
	
	ld	a,	e		;4
	sub	l			;4
	ld	l,	a		;4
	ld	a,	d		;4
	sbc	a,	h		;4
	ld	h,	a		;4  == 52
	; done shuffling.
		
	ldi
	inc	bc
	ldir

	pop	bc

	jp	L2bmain
				

L2b_end:
;	ld	a,	4	;DEBUG
;	out     (254),	a	;DEBUG
	ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; out-of-line jumps
L2bloop7_0:
	;;;; bb = *src, src++
	ld	a,	(bc)	; get *src
	inc	bc		; src++
	scf
	rla			; ADM: could use undoc''d SLL
	defb	0DDh,6Fh	; LD IXL,A
	
	jr	NC,	L2bloop6
	jp	L2bloop7_nonzero


L2bloop6_0:
	;;;; bb = *src, src++
	ld	a,	(bc)	; get *src
	inc	bc		; src++
	scf
	rla			; ADM: could use undoc''d SLL
	defb	0DDh,6Fh	; LD IXL,A
	
	jr	NC,	L2bloop6_zero
	jp	L2bloop6_nonzero


L2bjmp4_0:
	;;;; bb = *src, src++
	ld	a,	(bc)	; get *src
	inc	bc		; src++
	scf
	rla			; ADM: could use undoc''d SLL
	defb	0DDh,6Fh	; LD IXL,A

	jr	NC,	L2bjmp4_zero
	jp	L2bjmp4_nonzero


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
prev_doffset:	defw	0000h
doffset:		defw	0000h

L2b_unpack_ENDSYMBOL:

