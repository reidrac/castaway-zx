#ifndef _UTILS_H
#define _UTILS_H

void __FASTCALL__
fade_out(uint lines)
{
#asm
	push hl
	pop bc
	ld hl, $5800

	ld e, 7
.fade_out_all_loop0
	push hl
	push bc

	halt
.fade_out_all_loop1
	ld a, (hl)
	and 7
	jr z, no_fade_all_ink
	dec a
.no_fade_all_ink

	ld d, a

	ld a, (hl)
	and $38
	jr z, no_fade_all_paper
	sub 8
.no_fade_all_paper

	or d
	ld d, a

	ld a, (hl)
	and $c0
	or d

	ld (hl), a
	inc hl

	dec bc
	ld a, b
	or c
	jr nz, fade_out_all_loop1

	pop bc
	pop hl
	dec e
	jr nz, fade_out_all_loop0
#endasm
}

void
pad_numbers(uchar *s, uint limit, uint number)
{
    s += limit;
    *s = 0;

    while (limit--)
    {
        *--s = (number % 10) + '0';
        number /= 10;
    }
}

void __CALLEE__
setup_tiles(uchar *src, uchar base, uchar len)
{
	/*
	for (i = 0; i < len; ++i, src += 8)
		sp1_TileEntry(base + i, src);
	*/
#asm
	pop hl
	pop de		; len
	pop bc		; base
	ex (sp), hl	; src

	ld b, e

	; hl src, b len, c base

.setup_tiles_loop
	push hl
	push bc

	ld b, 0
	push bc
	push hl
	call sp1_TileEntry_callee

	pop bc
	pop hl

	ld de, 8
	add hl, de

	inc c

	djnz setup_tiles_loop
#endasm
}

void __CALLEE__
print(uchar x, uchar y, uchar attr, char *str)
{
	/*
	while(*str)
		sp1_PrintAtInv(y, x++, attr, *str++);
	*/
#asm
	ld ix, 2
	add ix, sp

	ld l, (ix + 0)
	ld h, (ix + 1)

.print_loop
	push hl
	push ix

	ld d, 0
	ld e, (ix + 4) ; y
	push de

	ld e, (ix + 6) ; x
	push de

	ld e, (ix + 2) ; attr
	push de

	ld e, (hl)
	push de ; char

	call sp1_PrintAtInv_callee

	pop ix
	pop hl

	inc hl

	ld a, (ix + 6)
	inc a
	ld (ix + 6), a

	ld a, (hl)
	or a
	jr nz, print_loop

	pop hl
	pop af
	pop af
	pop af
	pop af
	push hl
#endasm
}

#endif

