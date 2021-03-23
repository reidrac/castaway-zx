#ifndef _INT_H
#define _INT_H

#ifndef WFRAMES
#define WFRAMES		4
#endif // WFRAMES

#define clock(x)	(tick)

// timer
uchar tick;
uchar timer;

extern void isr();

// update the clock
#asm
._isr
	push af
	ld a, (_tick)
	inc a
	ld (_tick), a
	pop af
	ei
	ret
#endasm


void
wait()
{
	while(tick - timer < WFRAMES)
    {
#asm
        halt
#endasm
    }
	timer = tick;
}

void
setup_int()
{
	tick = 0;
	timer = 0;

#asm
	di
#endasm

	memset(0xd000, 0xd1, 258);

#asm
	ld hl, 0xd000
	ld a, h
	ld i, a
	im 2

	ld hl, 0xd1d1
	ld de, _isr
	ld a, 0xc3
	ld (hl), a
	inc hl
	ld (hl), e
	inc hl
	ld (hl), d

	ei
#endasm
}

void __FASTCALL__
delay(uint t)
{
#asm
.delay_loop
	push hl
	call _wait
	pop hl
	dec hl
	ld a, h
	or l
	jr nz, delay_loop
#endasm
}

#endif // _INT_H

