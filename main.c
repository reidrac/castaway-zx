/*
   Castaway, a ZX Spectrum game
   Copyright (C) 2016 Juan J. Martinez <jjm@usebox.net>

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include <spectrum.h>
#include <string.h>
#include <sprites/sp1.h>
#include <malloc.h>
#include <input.h>

// includes for local libs
#include "lib/ucl.h"

// setup

// that's 50Hz/3 -> ~16 FPS
#define WFRAMES 3

// generated includes that are OK with contended memory
// Place all const data into contended memory
#pragma constseg CONTENDED
#include "dr_text.h"
#include "prdr.h"
#include "song1.h"
#include "song2.h"
#include "song3.h"
#include "gameover.h"
#include "menu.h"
#include "play.h"
#include "shipwreck.h"
#include "obj_tiles.h"
#include "numbersx2.h"
#include "numbers.h"
#include "font.h"


// convenient global variables (for speed)
int i, j, k;
uint key;
uchar *pt;

#define PRDR_ADDR       0xc7c0
#define PRDR_SONG       0xc8c0  // limit 1344 bytes

    void
prdr_play()
{
#asm
    ld hl, PRDR_SONG
        call PRDR_ADDR
#endasm
}

// sp height/width in tiles always adds 1
#define HEIGHT(x)       (x - 1)
#define WIDTH(x)        (x - 1)

// anything that requires the global variables after this point
#include "int.h"

const uchar sprite_pad[] = { 0xff, 0x0, 0xff, 0x0, 0xff, 0x0, 0xff, 0x0 };
#include "phead.h"
#include "pbody.h"
#include "platform.h"
#include "objects.h"
const uchar sprite_pad2[] = { 0xff, 0x0, 0xff, 0x0, 0xff, 0x0, 0xff, 0x0 };
#include "zap.h"
#include "zwall.h"
const uchar sprite_pad3[] = { 0xff, 0x0, 0xff, 0x0, 0xff, 0x0, 0xff, 0x0 };
#include "explo.h"
#include "alien.h"
#include "fly_robot.h"
#include "sentinel.h"
#include "sentinel2.h"

#include "sprites.h"
#include "misc.h"
#include "stage.h"
#include "beeper.h"

#define BUFFER_ADDR     0xd101

#if ROOM_SIZE > 208
#error "ROOM_SIZE is larger than the 208 bytes available in 0xd101"
#endif
// working map
uchar *wmap = (uchar *)BUFFER_ADDR; // there are 208 bytes free in here
uchar cmap;

// remember: up to 8 pick up objects per room!
uchar *persistence = (uchar *)(BUFFER_ADDR + ROOM_SIZE);

// malloc

#define MALLOC_TOP 0xce00
#define MALLOC_SIZE 1700
#define MALLOC_ADDR (MALLOC_TOP-MALLOC_SIZE)
long heap;

    void
*u_malloc(uint size)
{
    return malloc(size);
}
    void
u_free(void *addr)
{
    free(addr);
}

// fullscreen
const struct sp1_Rect cr = { 0, 0, 32, 24 };
// game screen
const struct sp1_Rect gr = { 0, 0, 32, 20 };

// keys
uint keys[5] = { 'o', 'p', 'q', ' ', 'h' };
uint (*joyfunc)(struct in_UDK *) __z88dk_fastcall;
struct in_UDK joy_k;

// FIXME: set an initial hiscore!
uint hiscore = 0;

const char *redefine_texts[5] = { "LEFT",
    "RIGHT",
    "JUMP ",
    "FIRE",
    "PAUSE"
};

const struct sp1_Rect rdk = { 8, 2, 30, 12 };

    void
run_redefine_keys()
{
    sp1_ClearRectInv(&rdk, INK_BLACK | PAPER_BLACK, 32, SP1_RFLAG_TILE | SP1_RFLAG_COLOUR);

    print(10, 9, INK_WHITE|BRIGHT, "REDEFINE KEYS");
    print(9, 13, INK_WHITE|BRIGHT, "PRESS KEY\x3a");

    for (i = 0; i < 5; ++i)
    {
        print(20, 13, INK_CYAN|BRIGHT, redefine_texts[i]);
        sp1_UpdateNow();

        in_WaitForKey();
        playfx(FX_BIP);
        keys[i] = in_Inkey();
        in_WaitForNoKey();
    }
}

    void
run_intro()
{
    for (i = 0; i < 32; ++i)
        wait();

    fade_out(768);

    print(3, 11, INK_WHITE|BRIGHT, "SOMEWHERE IN DEEP SPACE...");
    sp1_UpdateNow();

    ucl_uncompress(prdr, ((uchar*)PRDR_ADDR));
    ucl_uncompress(song1, ((uchar*)PRDR_SONG));
    prdr_play();
}

const struct sp1_Rect drr = { 0, 0, 32, 6 };

    uint __CALLEE__
show_message(uchar *text, uchar x, uchar y, uchar c) __naked
{
#asm
    pop hl
        pop de
        ld (message_col + 1), de
        pop bc
        pop de
        ex (sp), hl

        dec hl

        .show_message_next_char
        inc hl

        ld a, (hl)
        cp 1
        jr z, show_message_end
        cp 0xa
        jr nz, show_message_print

        ld e, 0
        inc c
        jr show_message_next_char

        .show_message_print

        push hl
        push bc
        push de
        push af

        push bc
        push de
        .message_col
        ld hl, INK_WHITE | BRIGHT
        push hl
        ld l, a
        push hl
        call sp1_PrintAtInv_callee

        pop af

        cp 33
        jr c, show_message_not_visible

        call in_Inkey
        ld a, h
        or l
        jr nz, show_message_not_visible

        ld hl, FX_BIP
        call _playfx
        call _wait
        call sp1_UpdateNow

        .show_message_not_visible

        pop de
        inc e
        pop bc
        pop hl

        jp show_message_next_char

        .show_message_end
	ret
#endasm
}

    void
run_dr_message()
{
    /*
    // careful, we may step over the stack!
    pt = ((uchar*)0xce00);
    ucl_uncompress(dr_text, pt);
    while (*pt)
    {
    sp1_ClearRectInv(&drr, INK_BLACK | PAPER_BLACK, 32, SP1_RFLAG_TILE | SP1_RFLAG_COLOUR);

    i = 0;
    j = 0;
    c = INK_WHITE | BRIGHT;

    for (; *pt != 1; pt++)
    {
    if (*pt == 0xa)
    {
    i = 1;
    j++;
    continue;
    }

    sp1_PrintAtInv(j, i++, INK_CYAN|BRIGHT, *pt);
    if (*pt > 32)
    {
    playfx(FX_BIP);
    wait();
    sp1_UpdateNow();
    }
    }
    pt++;
    print(18, 4, INK_WHITE|BRIGHT, press_any_key);
    sp1_UpdateNow();
    in_WaitForKey();
    in_WaitForNoKey();
    }
     */
#asm
    call in_WaitForNoKey

        ld hl, 0xce00
        push hl

        ld de, _dr_text
        push de
        push hl
        call ucl_uncompress

        pop hl

        .run_dr_message_loop
        ld a, (hl)
        or a
        jr z, run_dr_message_ends

        push hl

        ld hl, _drr
        push hl
        ld hl, INK_BLACK | PAPER_BLACK
        push hl
        ld l, 32
        push hl
        ld l, SP1_RFLAG_TILE | SP1_RFLAG_COLOUR
        push hl
        call sp1_ClearRectInv_callee

        pop hl

        push hl
        ld de, 0
        push de
        push de
        ld de, INK_CYAN|BRIGHT
        push de
        call _show_message

        inc hl
        push hl

        ld hl, 18
        push hl
        ld l, 4
        push hl
        ld l, INK_WHITE | BRIGHT
        push hl
        ld hl, _press_any_key
        push hl
        call _print

        call sp1_UpdateNow
        call in_WaitForNoKey
        call in_WaitForKey
        call in_WaitForNoKey

        pop hl
        jp run_dr_message_loop

        .run_dr_message_ends

#endasm
}

extern uint tgame;

    void
endgame_time()
{
    if (tgame == 0xffff)
        return;

    tgame /= 960;

    pad_numbers(((uchar*)PRDR_SONG), 2, tgame);
    print(9, 13, INK_CYAN|BRIGHT, "PLAYED");
    print(16, 13, INK_YELLOW|BRIGHT, ((uchar*)PRDR_SONG));
    print(19, 13, INK_CYAN|BRIGHT, "MINS");
}

#asm
.endgame_text
defm "YOU DID IT! YOU BEAT THE GAME."
defb 0xa, 0xa
defm "NOW IS TIME TO REPAIR YOUR SHIP"
defb 0xa
defm "AND LEAVE THIS ROCK."
defb 0xa
defb 0xa
defm "THANKS FOR PLAYING CASTAWAY!"
defb 0xa
defb 1
#endasm

    void
run_endgame()
{
#asm
    call _hide_all_sprites
        ld hl, 640
        call _fade_out

        ld hl, endgame_text
        push hl
        ld hl,0
        push hl
        ld l, 5
        push hl
        ld de, INK_WHITE|BRIGHT
        push de
        call _show_message

        call _endgame_time
        call sp1_UpdateNow

        call in_WaitForNoKey

        ld hl, _prdr
        push hl
        ld hl, PRDR_ADDR
        push hl
        call ucl_uncompress

        ld hl, _song2
        push hl
        ld hl, PRDR_SONG
        push hl
        call ucl_uncompress

        call _prdr_play
        call in_WaitForNoKey
#endasm
}

const struct sp1_Rect mr = { 10, 2, 30, 10 };

const uchar *menu_opts[] = { "1.KEYBOARD",
    "2.KEMPSTON",
    "3.SINCLAIR",
    "4.REDEFINE" };

const uchar press_any_key[] = "PRESS ANY KEY!";

    void
draw_menu()
{
    fade_out(768);

    // this will wait for vsync
    wait();
    ucl_uncompress(menu, ((uchar*)0x4000));
    sp1_Validate(cr);

    print(5, 11, INK_WHITE|BRIGHT, "CODE, GRAPHICS & SOUND");
    print(3, 12, INK_YELLOW|BRIGHT, "JUAN J. MARTINEZ, @REIDRAC");

    print(9, 17, INK_WHITE|BRIGHT, press_any_key);

    print(5, 23, INK_CYAN, "\x7f 2016-2017 USEBOX.NET");
    sp1_UpdateNow();

    ucl_uncompress(prdr, ((uchar*)PRDR_ADDR));
    ucl_uncompress(song2, ((uchar*)PRDR_SONG));
    prdr_play();

    // avoid the player pressing one of the options and triggering the
    // associated code
    in_WaitForNoKey();

    sp1_ClearRectInv(&mr, INK_BLACK | PAPER_BLACK, 32, SP1_RFLAG_TILE | SP1_RFLAG_COLOUR);

    for (i = 0; i < 4; ++i)
        print(11, 11 + (i << 1), INK_WHITE|BRIGHT, menu_opts[i]);

    pad_numbers(((uchar*)PRDR_SONG), 5, hiscore);
    print(12, 8, INK_WHITE|BRIGHT, "HI:");
    print(15, 8, INK_YELLOW|BRIGHT, ((uchar*)PRDR_SONG));

    // will update in the main loop
}

#define DIR_RIGHT               0
#define DIR_LEFT                6

#define JUMP_SEQ                9
#define JUMP_UP                 5
const char jump_seq[] = { -16, -12, -8, -6, -4, 2, 4, 6, 8 };

#define JUMP_FRAME              4
const uchar player_anim[12] = {
    1, 0, 1, 2, 3, 4,
    6, 5, 6, 7, 8, 9
};

#define MAX_HITS                5
#define INVULNERABLE    32

uchar hits;
extern uchar invulnerable;
extern uchar boots, gun;
extern uint score;
extern uchar gameover, paused, endgame;

uchar player_dir;
extern uchar player_moved, player_h, player_frame;

extern uchar inv_top;

extern uchar game_data;

#asm
._game_data
._invulnerable
defb    0
._boots
defb    0
._gun
defb    0
._score
defw    0
._gameover
defb    0
._paused
defb    0
._player_moved
defb    0
._player_h
defb    0
._player_frame
defb    0
._inv_top
defb    0
._tgame
defw    0
._endgame
defb    0
#endasm

    void
set_body_frame()
{
    sprites[PBODY].s->frame = pbody + (player_anim[player_dir + player_frame] << 6);
}

    void
hide_player()
{
    sp1_MoveSprPix(sprites[PHEAD].s, &gr, sprites[PHEAD].s->frame, 0, 200);
    sp1_MoveSprPix(sprites[PBODY].s, &gr, sprites[PBODY].s->frame, 0, 200);
}

#define MAX_INV 4
uchar inv[MAX_INV];

const uchar obj_tiles_attr[3] = { INK_CYAN, INK_GREEN, INK_YELLOW };

    void
update_hud()
{
    uchar index, c;

    pt = (uchar *) (0x5800 + 675);
    hits = hits < MAX_HITS ? hits : MAX_HITS;

    if (!hits)
        gameover = 1;

    for (i = 0; i < MAX_HITS; ++i, pt++)
    {
        *pt = hits > i ? 0x42 : 0;
        pt[32] = hits > i ? 0x2 : 0;
    }

    for (k = 0; k < inv_top; ++k)
    {
        index = OBJ_TILES_BASE + (inv[k] << 2);
        c = obj_tiles_attr[inv[k]];

        // gun is discharged
        if (inv[k] == 2 && gun > 1)
            c = INK_BLUE;

        for (j = 0; j < 2; ++j)
            for (i = 0; i < 2; ++i)
                sp1_PrintAtInv(21 + j, 12 + (k << 1) + i, c, index++);
    }

    pt = (uchar *) (BUFFER_ADDR + ROOM_SIZE + TOTAL_ROOMS);
    pad_numbers(pt, 5, score);
    for (i = 0; i < 5; ++i)
    {
        index = *pt++ - '0';
        sp1_PrintAtInv(21, 24 + i, PAPER_BLACK | INK_WHITE | BRIGHT, NUMBERSX2_BASE + numbersx2_m[index]);
        sp1_PrintAtInv(22, 24 + i, PAPER_BLACK | INK_CYAN, NUMBERSX2_BASE + numbersx2_m[index + 10]);
    }
}

    void
player_hit()
{
    zx_border(INK_RED);
    hits--;
    invulnerable = INVULNERABLE;
    update_hud();
    play_queue = FX_DAMAGE;
}

    void
put_tile(uchar index, uchar x, uchar y)
{
    pt =  map_tile_mapping + (index << 3);

    sp1_PrintAtInv(y, x, *pt++, *pt++);
    sp1_PrintAtInv(y, x + 1, *pt++, *pt++);
    sp1_PrintAtInv(1 + y, x, *pt++, *pt++);
    sp1_PrintAtInv(1 + y, x + 1, *pt++, *pt);
}

    void __CALLEE__
put_shadow(uchar x, uchar y) __naked
{
    /*
       struct sp1_update *shadow_sp;

       shadow_sp = sp1_GetUpdateStruct(y, x);
       shadow_sp->colour = PAPER_BLACK | INK_YELLOW;
     */
#asm
    pop hl
        pop     de
        ex (sp), hl
        push de
        push hl
        call sp1_GetUpdateStruct_callee
        inc hl
        ld a, 6
        ld (hl), a
	ret
#endasm
}

    uchar __CALLEE__
map_xy(uchar x, uchar y) __naked
{
    /*
    // optimized for ROOM_WIDTH 16

    if (x & 1)
    return wmap[(x >> 1) + (y << 3)] & 15;

    return (wmap[(x >> 1) + (y << 3)] >> 4);
     */
#asm
    pop hl
        pop     de              ; y
        ex (sp), hl     ; x

        ld a, l ; x >> 1
        and a
        rra

        and a
        rl e
        rl e
        rl e ; y << 3

        ld c, l
        ld hl, (_wmap)

        add a, e
        ld e, a
        ld d, 0
        add hl, de

        ld a, (hl)

        bit 0, c
        jp nz, map_xy_odd

        and a
        rra
        rra
        rra
        rra

        .map_xy_odd
        and 15

        ld h, 0
        ld l, a
	ret
#endasm
}

    void __CALLEE__
set_map_xy(uchar x, uchar y, uchar tile) __naked
{
    /*
    // optimized for ROOM_WIDTH 16

    if (x & 1)
    tile |= wmap[(x >> 1) + (y << 3)] & 0xf0;
    else
    tile = (tile << 4) | (wmap[(x >> 1) + (y << 3)] & 0x0f);

    wmap[(x >> 1) + (y << 3)] = tile;
     */

#asm
    pop hl
        pop     bc              ; tile
        pop     de              ; y
        ex (sp), hl     ; x

        ld a, l ; x >> 1
        and a
        rra

        and a
        rl e
        rl e
        rl e ; y << 3

        ld b, l
        ld hl, (_wmap)

        add a, e
        ld e, a
        ld d, 0
        add hl, de

        ld a, (hl)

        bit 0, b
        jp nz, set_map_xy_odd

        and 15

        and a
        rl c
        rl c
        rl c
        rl c

        jp set_map_exit

        .set_map_xy_odd
        and 240

        .set_map_exit

        or c
        ld (hl), a
	ret
#endasm
}

// for the shipwreck
const struct sp1_Rect swr = { 9, 11, 5, 3 };

    void
draw_map()
{
    uchar c;

    for (j = 0; j < ROOM_HEIGHT; ++j)
        for (i = 0; i < ROOM_WIDTH / 2; ++i)
        {
            c = map_xy(i << 1, j);
            put_tile(c, i << 2, j << 1);

            // shadows
            if (c > 7)
            {
                // left-down
                if (i && map_xy((i << 1) - 1, j) < 6)
                {
                    if (!j || (j && map_xy((i << 1) - 1, j - 1) < 6))
                        put_shadow(i << 2, j << 1);
                    put_shadow(i << 2, 1 + (j << 1));
                }
                // top-left
                if (j && map_xy(i << 1, j - 1) < 6)
                {
                    if (!i || (i && map_xy((i << 1) - 1, j - 1) < 6))
                        put_shadow(i << 2, j << 1);
                    put_shadow(1 + (i << 2), j << 1);
                }
                // corner
                if (j && i && map_xy((i << 1) - 1, j - 1) < 6
                        && map_xy(i << 1, j - 1) > 7)
                    put_shadow(i << 2, j << 1);
            }

            c = map_xy(1 + (i << 1), j);
            put_tile(c, 2 + (i << 2), j << 1);

            // shadows
            if (c > 7)
            {
                // left-down
                if (map_xy(i << 1, j) < 6)
                {
                    if (!j || (j && map_xy(i << 1, j - 1) < 6))
                        put_shadow(2 + (i << 2), j << 1);
                    put_shadow(2 + (i << 2), 1 + (j << 1));
                }
                // top-left
                if (j && map_xy(1 + (i << 1), j - 1) < 6)
                {
                    if (!j || map_xy(i << 1, j - 1) < 6)
                        put_shadow(2 + (i << 2), j << 1);
                    put_shadow(3 + (i << 2), j << 1);
                }
                // corner
                if (j && map_xy(i << 1, j - 1) < 6
                        && map_xy(1 + (i << 1), j - 1) > 7)
                    put_shadow(2 + (i << 2), j << 1);
            }
        }

    // special
    if (!cmap)
        sp1_PutTilesInv(&swr, wreck_tbl);
    if (cmap == 17)
        print(12, 9, INK_RED|BRIGHT|FLASH, "ERROR#");

}

#define OB_END                  255
#define OB_VPLATFORM    0
#define OB_HPLATFORM    1
#define OB_VPLATFORM2   2
#define OB_HPLATFORM2   3
#define OB_PICKUP               4
#define OB_ZWALL_LEFT   5
#define OB_ZWALL_RIGHT  6
#define OB_ENEMY                10
#define OB_VDOOR                11
#define OB_HDOOR                12

    void
add_platform(uchar *def)
{
    if (!add_sprite())
        return;

    sp_used->s = sp1_CreateSpr(SP1_DRAW_MASK2LB, SP1_TYPE_2BYTE, 2, 8, 2);
    sp1_AddColSpr(sp_used->s, SP1_DRAW_MASK2, 0, 40, 2);
    sp1_AddColSpr(sp_used->s, SP1_DRAW_MASK2RB, 0, 0, 2);
    sp_used->type = ST_PLATFORM;

    switch (*def++)
    {
        case OB_VPLATFORM:
            sp_used->iy = 2;
            break;
        case OB_VPLATFORM2:
            sp_used->iy = -2;
            break;
        case OB_HPLATFORM:
            sp_used->ix = 2;
            break;
        case OB_HPLATFORM2:
            sp_used->ix = -2;
            break;
    }

    sp_used->frame = (*def++) << 3; // limit
    sp_used->x = (*def++) << 4;
    sp_used->y = 8 + ((*def) << 4);
    sp_used->s->frame = platform;
}

#define OBT_HEALTH              0
#define OBT_BOOTS               1
#define OBT_KEY                 2
#define OBT_GUN                 3
#define OBT_DR                  4       // not a pickup

    void
sprite_wrapper_16NR()
{
    sp_used->s = sp1_CreateSpr(SP1_DRAW_LOAD1NR, SP1_TYPE_1BYTE, 3, 0, 3);
    sp1_AddColSpr(sp_used->s, SP1_DRAW_LOAD1NR, 0, 24, 3);
    sp1_AddColSpr(sp_used->s, SP1_DRAW_LOAD1NR, 0, 0, 3);
}

    void __CALLEE__
add_pickup(uchar *def) __naked
{
    /*
       if (!add_sprite())
       return;

       sprite_wrapper_16NR();
       sp_used->type = ST_OBJECT;

       def++;

    // pickup type
    sp_used->subtype = *def++;
    sp_used->x = (*def++) << 4;
    sp_used->y = (*def) << 4;
    sp_used->s->frame = objects + (sp_used->subtype << 4) * 3;
     */
#asm
    call _add_sprite
        ld a, h
        or l
        jr nz, add_pickup_add_sprite_ok

        pop hl
        pop af
        push hl
        ret

        .add_pickup_add_sprite_ok

        call _sprite_wrapper_16NR

        pop de
        pop hl
        push de

        inc hl ; def ++

        ld de, (_sp_used)
        inc de
        inc de

        ld c, (hl) ; subtype
        inc hl

        ld a, (hl) ; x
        inc hl
        and a
        rla
        rla
        rla
        rla
        ld (de), a
        inc de

        ld a, (hl) ; y
        and a
        rla
        rla
        rla
        rla
        ld (de), a

        ex de, hl
        ld de, 6
        add hl, de
        ld (hl), ST_OBJECT
        inc hl
        ld (hl), c
        ex de, hl

        ld de, (_sp_used)
        ex de, hl
        ld e, (hl)
        inc hl
        ld d, (hl)
        ex de, hl
        ld de, 6
        add hl, de ; frame

        ld b, 0
        and a
        rl c
        rl c
        rl c
        rl c
        ex de, hl
        ld hl, 0
        add hl, bc
        add hl, bc
        add hl, bc
        ld bc, _objects
        add hl, bc
        ex de, hl
        ld (hl), e
        inc hl
        ld (hl), d
	ret
#endasm
}

    void __CALLEE__
add_zwall(uchar *def) __naked
{
    /*
       if (!add_sprite())
       return;

       sprite_wrapper_16NR();
       sp_used->type = ST_WZAP;

       if (*def == OB_ZWALL_LEFT)
       {
       sp_used->ix = 1;
       sp_used->s->frame = zwall;
       }
       else
       sp_used->s->frame = zwall + 12 * 4;

       def++;

    // timer
    sp_used->frame = *def++;
    sp_used->x = (*def++) << 4;
    sp_used->y = (*def) << 4;
     */
#asm
    call _add_sprite
        ld a, h
        or l
        jr nz, add_zwall_add_sprite_ok

        pop hl
        pop af
        push hl
        ret

        .add_zwall_add_sprite_ok

        call _sprite_wrapper_16NR

        pop de
        pop hl
        push de

        ld a, (hl)

        ex de, hl
        ld hl, (_sp_used)
        push de

        cp OB_ZWALL_LEFT
        jr nz, add_zwall_right

        push hl
        ld a, (hl)
        inc hl
        ld h, (hl)
        ld l, a
        ld bc, 6
        add hl, bc ; frame
        ld de, _zwall
        ld (hl), e
        inc hl
        ld (hl), d
        pop hl

        ld bc, 4
        add hl, bc ; ix
        ld (hl), 1

        jr add_zwall_left_done

        .add_zwall_right

        ex de, hl
        ld hl, _zwall
        ld bc, 48
        add hl, bc
        ex de, hl
        ld a, (hl)
        inc hl
        ld h, (hl)
        ld l, a
        ld bc, 6
        add hl, bc
        ld (hl), e
        inc hl
        ld (hl), d

        .add_zwall_left_done
        pop de

        ex de, hl
        inc hl ; def ++

        ex de, hl
        ld hl, (_sp_used)
        ld bc, 6
        add hl, bc
        ex de, hl

        ld c, (hl) ; frame
        ex de, hl
        ld (hl), c
        ex de, hl
        inc hl

        ld de, (_sp_used)
        inc de
        inc de

        ld a, (hl) ; x
        inc hl
        and a
        rla
        rla
        rla
        rla
        ld (de), a
        inc de

        ld a, (hl) ; y
        and a
        rla
        rla
        rla
        rla
        ld (de), a

        ex de, hl
        ld de, 6
        add hl, de
        ld (hl), ST_WZAP
        inc hl
        ld (hl), c
        ex de, hl
	ret
#endasm
}

    void
add_zap(int x, int y, uchar dir, uchar type)
{
    if (!add_sprite())
        return;

    sp_used->s = sp1_CreateSpr(SP1_DRAW_MASK2LB, SP1_TYPE_2BYTE, 2, 8, 3);
    sp1_AddColSpr(sp_used->s, SP1_DRAW_MASK2, 0, 40, 3);
    sp1_AddColSpr(sp_used->s, SP1_DRAW_MASK2RB, 0, 0, 3);

    sp_used->type = type;
    sp_used->ix = dir ? 6 : -6;
    sp_used->x = x;
    sp_used->y = y;
    sp_used->s->frame = zap;

    play_queue = FX_FIRE;
}

#define ET_ALIEN                0
#define ET_FLY_ROBOT    1
#define ET_SENTINEL             2
#define ET_SENTINEL2    3

const uchar sentinel_frames[] = { 0, 1, 2, 1 };

    void
add_enemy(uchar *def)
{
    if (!add_sprite())
        return;

    sp_used->type = ST_ENEMY;

    def++;

    // default to horizontal movement
    if (*def >> 4)
        sp_used->ix = 1;
    else
        sp_used->ix = -1;

    sp_used->subtype = *def & 0x0f;
    def++;

    if (sp_used->subtype == ET_ALIEN)
    {
        sp_used->s = sp1_CreateSpr(SP1_DRAW_MASK2LB, SP1_TYPE_2BYTE, 2, 8, 2);
        sp1_AddColSpr(sp_used->s, SP1_DRAW_MASK2, 0, 40, 2);
    }
    else
    {
        sp_used->s = sp1_CreateSpr(SP1_DRAW_MASK2LB, SP1_TYPE_2BYTE, 3, 16, 2);
        sp1_AddColSpr(sp_used->s, SP1_DRAW_MASK2, 0, 64, 2);
    }
    sp1_AddColSpr(sp_used->s, SP1_DRAW_MASK2RB, 0, 0, 2);

    sp_used->x = (*def++) << 4;
    sp_used->y = (*def) << 4;

    switch (sp_used->subtype)
    {
        case ET_FLY_ROBOT:
            if (sp_used->ix > 0)
                sp_used->frame = 1;
            sp_used->s->frame = fly_robot + ((sp_used->frame << 5) * 3);
            break;

        case ET_SENTINEL:
            sp_used->s->frame = sentinel;
            sp_used->iy = -sp_used->ix;
            break;

        case ET_SENTINEL2:
            sp_used->s->frame = sentinel2;
            sp_used->iy = -sp_used->ix;
            break;

        case ET_ALIEN:
            sp_used->s->frame = alien;
            sp_used->y += 8;
            break;
    }
}

    void
add_explo(int x, int y)
{
    // recycles sprite in sp_iter that was already marked to be collected
    sp_iter->s = sp1_CreateSpr(SP1_DRAW_MASK2LB, SP1_TYPE_2BYTE, 2, 8, 2);
    sp1_AddColSpr(sp_iter->s, SP1_DRAW_MASK2RB, 0, 0, 2);
    sp_iter->type = ST_EFX;
    sp_iter->x = x;
    sp_iter->y = y;
    sp_iter->s->frame = explo;
    sp_iter->frame = 0;
    sp_iter->alive = 1;
    --sp_collect;

    play_queue = FX_EXPLO;
}

    uchar __FASTCALL__
is_door_tile(uint t) __naked
{
#asm
    ld a, 4
        cp l
        jr z, is_door_tile0
        ld a, 5
        cp l
        jr z, is_door_tile0
        ld hl, 0
        .is_door_tile0
	ret
#endasm
}

    void
add_door(uchar *def)
{
    uchar tile;

    if (*def++ == OB_HDOOR)
        tile = 5;
    else
        tile = 4;

    k = 1 + *def++;
    i = *def++;
    j = *def;

    while (k--)
    {
        set_map_xy(i, j, tile);
        if (tile == 5)
            ++i;
        else
            ++j;
    }
}

    void __FASTCALL__ 
door_effect() __naked
{
#asm
    ld b, 16

        .door_effect_rep
        push bc
        ld hl, FX_BIP
        call _playfx
        pop bc

        ld hl, BUFFER_ADDR + ROOM_SIZE + TOTAL_ROOMS
        halt

        jp door_effect_loop_entry

        .door_effect_loop

        ld a, b
        inc a
        and 3

        push hl
        ex de, hl
        ld (hl), a
        inc hl
        ld (hl), a
        ld de, 31
        add hl, de
        ld (hl), a
        inc hl
        ld (hl), a
        ex de, hl
        pop hl

        .door_effect_loop_entry
        ld e, (hl)
        inc hl
        ld d, (hl)
        inc hl

        ld a, e
        or d
        jr nz, door_effect_loop

        djnz door_effect_rep
	ret
#endasm
}

    void
use_key()
{
    uint *pt;

    for (i = 0; i < inv_top; ++i)
        if (inv[i] == OBT_KEY - 1) // KEY
        {
            for (j = i; j < inv_top - 1; ++j)
                inv[j] = inv[j + 1];
            inv_top--;

            for (j = 21; j < 21 + 2; ++j)
                for (i = 12; i < 12 + 2; ++i)
                    sp1_PrintAtInv(j, i + (inv_top << 1), INK_BLACK, 32);

            score += 50;
            update_hud();

            // one door!
            persistence[cmap] |= 1;

            pt = (uint *)(BUFFER_ADDR + ROOM_SIZE + TOTAL_ROOMS);
            for (j = 0; j < ROOM_HEIGHT; ++j)
                for (i = 0; i < ROOM_WIDTH; ++i)
                    if (is_door_tile(map_xy(i, j)))
                        *pt++ = 0x5800 + (i << 1) + (j << 6);

            *pt = 0; // END

            // remove the door
            ucl_uncompress(map[cmap], wmap);
            door_effect();

            draw_map();
            play_queue = FX_SELECT;

            if (cmap == 17)
                endgame = 1;
            return;
        }
}

    void
setup_map(uchar m)
{
    uchar index = 0, *pt;

    cmap = m;

    ucl_uncompress(map[cmap], wmap);

    destroy_type_sprite(ST_ALL);
    collect_sprites();
    hide_all_sprites();

    sp1_Validate(&gr);
    sp1_UpdateNow();
    sp1_Invalidate(&gr);

    // specials
    if (map_objects[cmap])
    {
        // process doors first
        pt = map_objects[cmap];
        if (*pt == OB_HDOOR || *pt == OB_VDOOR)
        {
            if (!(persistence[cmap] & (1 << index)))
                add_door(pt);
            ++index;
            pt += 4;
        }

        while (*pt != OB_END)
        {
            switch (*pt)
            {
                case OB_VPLATFORM:
                case OB_HPLATFORM:
                case OB_VPLATFORM2:
                case OB_HPLATFORM2:
                    add_platform(pt);
                    break;
                case OB_PICKUP:
                    if (!(persistence[cmap] & (1 << index)))
                    {
                        add_pickup(pt);
                        // track persitence
                        sp_used->delay = index;
                    }
                    ++index;
                    break;
                case OB_ZWALL_LEFT:
                case OB_ZWALL_RIGHT:
                    add_zwall(pt);
                    break;
                case OB_ENEMY:
                    add_enemy(pt);
                    break;
            }
            pt += 4;
        }
    }

    draw_map();
}

uchar gcol;

    uchar __CALLEE__
check_collision_point(struct ssprites *a, uchar x, uchar y) __naked
{
    /*
       gcol = (y < a->y + (HEIGHT(a->s->height) << 3) && a->y <= y
       && x < a->x + (WIDTH(a->s->width) << 3) && a->x <= x);
       return gcol;
     */
#asm
    pop hl
        pop de ; y
        pop bc ; x
        pop ix ; a
        push hl

        ld a, e
        cp (ix + 3) ; y >= a->y
        jp c, check_collision_point_nay

        ld l, (ix + 0)
        ld h, (ix + 1)
        inc hl
        inc hl
        inc hl
        ld a, (hl)

        dec a
        and a
        rla
        rla
        rla

        add a, (ix + 3)

        ld d, a
        ld a, e
        cp d ; y < ...
        jp nc, check_collision_point_nay

        ld a, c
        cp (ix + 2) ; x >= a->x
        jp c, check_collision_point_nay

        dec hl
        ld a, (hl)

        dec a
        and a
        rla
        rla
        rla

        add a, (ix + 2)

        ld b, a
        ld a, c
        cp b ; x < ...
        jp nc, check_collision_point_nay

        ld hl, 1
        ret

        .check_collision_point_nay
        ld hl, 0
	ret
#endasm
}

    uchar
check_collision(struct ssprites *a, struct ssprites *b)
{
    gcol = (a->y < b->y + (HEIGHT(b->s->height) << 3) && b->y <= a->y + (HEIGHT(a->s->height) << 3)
            && a->x < b->x + (WIDTH(b->s->width) << 3) && b->x <= a-> x + (WIDTH(a->s->width) << 3));
    return gcol;
}

    uchar
check_player_collision(struct ssprites *p)
{
    gcol = 0;
    for (i = 0; i < 2; ++i)
        gcol |= (sprites[i].y < p->y + (HEIGHT(p->s->height) << 3) - 2 && p->y + 2 < sprites[i].y + (HEIGHT(sprites[i].s->height) << 3)
                && sprites[i].x + 4 < p->x + (WIDTH(p->s->width) << 3) - 4 && p->x + 4 < sprites[i].x + (WIDTH(sprites[i].s->width) << 3) - 4);
    return gcol;
}

    uchar __CALLEE__
is_map_blocked(uchar x, uchar y) __naked
{
    /*
    // uses sp_iter2!!!
    if (!(wmap[(x >> 5) + (y >> 4) * ROOM_WIDTH / 2] & (((x >> 4) & 1) ? 8 : 128)))
    return 1;

    for (sp_iter2 = sp_used; sp_iter2; sp_iter2 = sp_iter2->n)
    if (sp_iter2->type == ST_PLATFORM && check_collision_point(sp_iter2, x, y))
    return 1;

    return 0;
     */
#asm
    pop hl
        pop     de              ; y (max 159)
        ex (sp), hl     ; x (max 255)

        push hl         ; x
        push de         ; y

        ; FIXME
        ld a, e
        srl d
        rra
        srl d
        rra
        srl d
        rra
        srl d
        rra
        ld e, a ; y >> 4

        ex de, hl
        add hl, hl
        add hl, hl
        add hl, hl
        ex de, hl ; (y >> 4) * ROOM_WIDTH / 2

        ld a, l
        srl h
        rra
        srl h
        rra
        srl h
        rra
        srl h
        rra
        ld l, a ; x >> 4
        ld c, a
        srl h   ; x >> 5
        rra
        ld l, a

        add hl, de
        ld de, (_wmap)
        add hl, de
        ld a, (hl)

        bit 0, c
        jp nz, is_map_blocked_odd

        and 128
        or a
        jp nz, is_map_blocked_prepare

        .is_map_blocked_yay
        pop af
        pop af

        ld hl, 1
        ret

        .is_map_blocked_odd

        and 8
        or a
        jp z, is_map_blocked_yay

        .is_map_blocked_prepare

        ld de, (_sp_used)

        .is_map_blocked_next
        ld a, e
        or d
        jr z, is_map_blocked_nope

        ld ix, 0
        add ix, de

        ld a, (ix + 9)
        cp 2
        jr nz, is_map_blocked_prepare_next

        pop de ; y
        pop hl ; x
        push hl
        push de

        push ix ; save ix
        push ix ; iter
        push hl ; x
        push de ; y
        call _check_collision_point
        pop ix
        ld a, l
        or a
        jr z, is_map_blocked_prepare_next

        pop af
        pop af

        ret

        .is_map_blocked_prepare_next
        ld e, (ix + 11)
        ld d, (ix + 12)
        jp is_map_blocked_next

        .is_map_blocked_nope
        pop af
        pop af

        ld hl, 0
        ret
#endasm
}

    void
draw_entities()
{
    /*
       for (sp_iter = sp_used; sp_iter; sp_iter = sp_iter->n)
       sp1_MoveSprPix(sp_iter->s, &cr, sp_iter->sprite, sp_iter->x, sp_iter->y);
     */
#asm
    ld hl, (_sp_used)

        .draw_entities_next
        ld a, h
        or l
        jr nz, draw_entities_continue
        ret

        .draw_entities_continue
        push hl
        push hl
        pop ix

        ld e, (hl)
        inc hl
        ld d, (hl)
        push de ; s

        ld de, _gr
        push de ; &gr

        ld e, (ix + 0)
        ld d, (ix + 1)
        ex de, hl
        ld bc, 6
        add hl, bc
        ld a, (hl)
        inc hl
        ld h, (hl)
        ld l, a
        ex de, hl
        push de ; sprite

        ld d, 0
        inc hl
        ld e, (hl)
        push de ; x

        inc hl
        ld e, (hl)
        push de ; y

        call sp1_MoveSprPix_callee

        pop ix

        ld l, (ix + 11)
        ld h, (ix + 12)
        jp draw_entities_next
#endasm
}

uchar gh, gw;

    void __FASTCALL__
update_efx(const struct ssprites *s) __naked
{
#asm
    ex de, hl
        ld hl, 7
        add hl, de
        ld a, (hl)
        inc (hl) ; delay++
        cp 2
        jr nz, update_efx_done

        xor a
        ld (hl), a
        dec hl
        ld a, (hl)
        inc (hl) ; frame++
        or a
        jr z, update_efx_inc_s_frame

        push de
        call _remove_sprite
        pop de
        ret

        .update_efx_inc_s_frame
        ex de, hl
        ld e, (hl)
        inc hl
        ld d, (hl)
        ld hl, 6
        add hl, de ; s->frame

        ld e, (hl)
        inc hl
        ld d, (hl)
        ex de, hl
        ld bc, 32
        add hl, bc
        ex de, hl
        dec hl
        ld (hl), e
        inc hl
        ld (hl), d

        .update_efx_done
	ret
#endasm
}

    void __FASTCALL__
update_wzap(const struct ssprites *s) __naked
{
#asm
    ex de, hl
        ld hl, 7
        add hl, de
        ld a, (hl)
        inc (hl) ; delay++
        dec hl
        cp (hl)
        jr nz, update_wzap_done

        xor a
        inc hl
        ld (hl), a

        ld hl, 4
        add hl, de
        ld a, (hl)

        ld b, 0
        ld d, 0
        dec hl
        ld c, (hl) ; y
        inc c
        inc c
        inc c
        inc c
        dec hl
        ld e, (hl) ; x

        or a
        jr z, update_wzap_right

        ld a, 16
        add a, e
        ld e, a
        push de
        push bc
        ld hl, 1
        push hl
        jr update_wzap_call

        .update_wzap_right

        ld a, -16
        add a, e
        ld e, a
        push de
        push bc
        ld hl, 0
        push hl

        .update_wzap_call
        ld hl, ST_EZAP
        push hl
        call _add_zap

        pop af
        pop af
        pop af
        pop af

        .update_wzap_done
	ret
#endasm
}

    uchar
check_enemy_collision()
{
    /*
       k = 0;

    // basic wall collision
    if (sp_iter->ix > 0)
    {
    k = sp_iter->x == (ROOM_WIDTH << 4) - gw
    || is_map_blocked(sp_iter->x + gw, sp_iter->y + (gh >> 1));
    if (!sp_iter->iy)
    k += !is_map_blocked(sp_iter->x + gw, sp_iter->y + gh);

    }

    if (!k && sp_iter->ix < 0)
    {
    k = !sp_iter->x
    || is_map_blocked(sp_iter->x, sp_iter->y + (gh >> 1));
    if (!sp_iter->iy)
    k += !is_map_blocked(sp_iter->x, sp_iter->y + gh);
    }

    if (k)
    {
    sp_iter->ix *= -1;
    return 1;
    }

    if (!sp_iter->iy)
    return 0;

    // sentinel
    if (sp_iter->iy > 0)
    {
    if (sp_iter->y == (ROOM_HEIGHT << 4) - gh
    || is_map_blocked(sp_iter->x + (gw >> 1), sp_iter->y + gh))
    {
    sp_iter->iy *= -1;
    return 1;
    }
    }

    if (sp_iter->iy < 0)
    {
    if (!sp_iter->y
    || is_map_blocked(sp_iter->x + (gw >> 1), sp_iter->y))
    {
    sp_iter->iy *= -1;
    return 1;
    }
    }

    return 0;
     */

#asm
    ld ix, (_sp_iter)
        ld a, (ix + 4) ; ix
        and 128
        jr nz, check_enemy_collision_ix_neg

        ld hl, _gw
        ld l, (hl)
        ld a, 255
        sub l

        ld l, (ix + 2) ; x
        cp l
        jr nz, check_enemy_collision_ix_pos0

        .check_enemy_collision_pos_change
        ld a, 255 ; change ix dir
        ld (ix + 4), a
        jp check_enemy_collision_yay

        .check_enemy_collision_ix_pos0
        push ix
        ld de, _gw
        ld h, 0
        ld a, (de)
        add (ix + 2)
        ld l, a
        push hl
        ld de, _gh
        ld a, (de)
        and a
        rra
        add (ix + 3)
        ld l, a
        push hl
        call _is_map_blocked
        pop ix

        ld a, h
        or l
        jr nz, check_enemy_collision_pos_change

        ld a, (ix + 5) ; iy
        or a
        jr nz, check_enemy_collision_iy

        push ix
        ld de, _gw
        ld h, 0
        ld a, (de)
        add (ix + 2)
        ld l, a
        push hl
        ld de, _gh
        ld a, (de)
        add (ix + 3)
        ld l, a
        push hl
        call _is_map_blocked
        pop ix

        ld a, h
        or l
        jr z, check_enemy_collision_pos_change

        jp check_enemy_collision_nay

        .check_enemy_collision_ix_neg

        ld a, (ix + 2) ; x
        or a
        jr nz, check_enemy_collision_ix_pos1

        .check_enemy_collision_neg_change
        ld a, 1
        ld (ix + 4), a
        jp check_enemy_collision_yay

        .check_enemy_collision_ix_pos1
        push ix
        ld h, 0
        ld l, (ix + 2)
        push hl
        ld de, _gh
        ld a, (de)
        and a
        rra
        add (ix + 3)
        ld l, a
        push hl
        call _is_map_blocked
        pop ix

        ld a, h
        or l
        jr nz, check_enemy_collision_neg_change

        ld a, (ix + 5) ; iy
        or a
        jr nz, check_enemy_collision_iy

        push ix
        ld h, 0
        ld l, (ix + 2)
        push hl
        ld de, _gh
        ld a, (de)
        add (ix + 3)
        ld l, a
        push hl
        call _is_map_blocked
        pop ix

        ld a, h
        or l
        jr z, check_enemy_collision_neg_change

        .check_enemy_collision_nay
        ld hl, 0
        ret

        .check_enemy_collision_iy

        ld a, (ix + 5) ; iy
        and 128
        jr nz, check_enemy_collision_iy_neg

        ld hl, _gh
        ld l, (hl)
        ld a, 160
        sub l

        ld l, (ix + 3) ; y
        cp l
        jr nz, check_enemy_collision_iy_pos0

        .check_enemy_collision_y_pos_change
        ld a, 255 ; change iy dir
        ld (ix + 5), a
        jr check_enemy_collision_yay

        .check_enemy_collision_iy_pos0
        push ix
        ld de, _gw
        ld h, 0
        ld a, (de)
        and a
        rra
        add (ix + 2)
        ld l, a
        push hl
        ld de, _gh
        ld a, (de)
        add (ix + 3)
        ld l, a
        push hl
        call _is_map_blocked
        pop ix

        ld a, h
        or l
        jr nz, check_enemy_collision_y_pos_change

        jr check_enemy_collision_nay

        .check_enemy_collision_iy_neg
        ld a, (ix + 3) ; y
        or a
        jr nz, check_enemy_collision_iy_neg0

        .check_enemy_collision_y_neg_change
        ld a, 1
        ld (ix + 5), a
        jr check_enemy_collision_yay

        .check_enemy_collision_iy_neg0
        push ix
        ld de, _gw
        ld h, 0
        ld a, (de)
        and a
        rra
        add (ix + 2)
        ld l, a
        push hl
        ld l, (ix + 3)
        push hl
        call _is_map_blocked
        pop ix

        ld a, h
        or l
        jr nz, check_enemy_collision_y_neg_change

        jr check_enemy_collision_nay

        .check_enemy_collision_yay
        ld hl, 1
#endasm
}

    void
flip_fly_robot()
{
    sp_iter->frame = !sp_iter->frame;
    sp_iter->s->frame = fly_robot + ((sp_iter->frame << 5) * 3);
}

    void
update_entities()
{
    for (sp_iter = sp_used; sp_iter; sp_iter = sp_iter->n)
    {
        switch (sp_iter->type)
        {
            case ST_PLAYER:
                continue;

            case ST_EFX:

                /*
                   if (sp_iter->delay++ == 2)
                   {
                   sp_iter->delay = 0;
                   if (sp_iter->frame++)
                   {
                   remove_sprite(sp_iter);
                   break;
                   }

                   sp_iter->s->frame += 32;
                   }
                 */

                update_efx(sp_iter);
                break;

            case ST_ENEMY:

                if (!sp_iter->alive)
                    break;

                switch (sp_iter->subtype)
                {
                    default:
                        break;

                    case ET_SENTINEL2:
                    case ET_SENTINEL:
                        if (sp_iter->delay++ == 3)
                        {
                            sp_iter->delay = 0;
                            if (sp_iter->frame++ == 3)
                                sp_iter->frame = 0;
                            if (sp_iter->subtype == ET_SENTINEL)
                                sp_iter->s->frame = sentinel;
                            else
                                sp_iter->s->frame = sentinel2;
                            sp_iter->s->frame += ((sentinel_frames[sp_iter->frame] << 5) * 3);
                        }
                        break;

                    case ET_ALIEN:
                        if (sp_iter->delay++ == 3)
                        {
                            sp_iter->delay = 0;
                            sp_iter->frame = !sp_iter->frame;
                            sp_iter->s->frame = alien + (sp_iter->frame << 6);
                        }
                        break;
                }

                if (!invulnerable && check_player_collision(sp_iter))
                {
                    player_hit();
                    sp_iter->ix *= -1;
                    sp_iter->iy *= -1;

                    if (sp_iter->subtype == ET_FLY_ROBOT)
                        flip_fly_robot();
                    break;
                }

                gh = HEIGHT(sp_iter->s->height) << 3;
                gw = WIDTH(sp_iter->s->width) << 3;

                i = 2;
                if (sp_iter->subtype > ET_SENTINEL - 1)
                    ++i;

                for (j = 0; j < i; ++j)
                {
                    sp_iter->x += sp_iter->ix;
                    sp_iter->y += sp_iter->iy;

                    if (check_enemy_collision())
                    {
                        if (sp_iter->subtype == ET_FLY_ROBOT)
                            flip_fly_robot();
                        break;
                    }
                }

                break;

            case ST_WZAP:

                /*
                   if (sp_iter->delay++ == sp_iter->frame)
                   {
                   sp_iter->delay = 0;
                   if (sp_iter->ix)
                   add_zap(sp_iter->x + 16, sp_iter->y + 4, 1, ST_EZAP);
                   else
                   add_zap(sp_iter->x - 16, sp_iter->y + 4, 0, ST_EZAP);
                   }
                 */
                update_wzap(sp_iter);

                break;

            case ST_EZAP:
            case ST_ZAP:
                if (sp_iter->delay++)
                {
                    sp_iter->delay = 0;
                    sp_iter->frame = !sp_iter->frame;
                    sp_iter->s->frame = zap + (sp_iter->frame << 6);
                }

                sp_iter->x += sp_iter->ix;

                if (sp_iter->type == ST_EZAP)
                {
                    if (!invulnerable && check_player_collision(sp_iter))
                    {
                        player_hit();
                        remove_sprite(sp_iter);
                        break;
                    }
                }
                else
                {
                    for (sp_iter2 = sp_used; sp_iter2; sp_iter2 = sp_iter2->n)
                        if (sp_iter2->type == ST_ENEMY && check_collision(sp_iter, sp_iter2))
                        {
                            if (sp_iter2->subtype < ET_SENTINEL)
                                score += 25;
                            else
                                score += 75;
                            update_hud();

                            remove_sprite(sp_iter);
                            remove_sprite(sp_iter2);

                            j = sp_iter2->y;
                            if (sp_iter2->subtype != ET_ALIEN)
                                j += 4;
                            add_explo(sp_iter2->x + 4, j);
                            break;
                        }

                    if (!sp_iter->alive)
                        break;
                }

                if (sp_iter->ix > 0)
                {
                    if (sp_iter->x < (ROOM_WIDTH << 4) - 16
                            && (is_map_blocked(sp_iter->x + 16, sp_iter->y + 2)
                                || is_map_blocked(sp_iter->x + 16, sp_iter->y + 6)))
                    {
                        remove_sprite(sp_iter);
                        add_explo(sp_iter->x + 8, sp_iter->y);
                        break;
                    }
                }
                else
                {
                    if(sp_iter->x < (ROOM_WIDTH << 4) - 16 &&
                            (is_map_blocked(sp_iter->x, sp_iter->y + 2)
                             || is_map_blocked(sp_iter->x, sp_iter->y + 6)))
                    {
                        remove_sprite(sp_iter);
                        add_explo(sp_iter->x, sp_iter->y);
                        break;
                    }
                }

                // out of screen check
                if (sp_iter->x >= (ROOM_WIDTH << 4) - 16)
                    remove_sprite(sp_iter);

                break;

            case ST_OBJECT:
                if (check_player_collision(sp_iter))
                {
                    // not inventory restricted
                    if (sp_iter->subtype == OBT_HEALTH)
                    {
                        if (hits == MAX_HITS)
                            score += 250;
                        hits = MAX_HITS;
                    }
                    else
                    {
                        // inventory items
                        if (inv_top == MAX_INV)
                            break;

                        switch (sp_iter->subtype)
                        {
                            case OBT_BOOTS:
                                boots = 1;
                                gh = 0;
                                break;
                            case OBT_GUN:
                                gun = 1;
                                gh = 2;
                                break;
                            case OBT_DR:
                                run_dr_message();
                            case OBT_KEY:
                                gh = 1;
                                break;
                        }

                        score += 250;
                        inv[inv_top++] = gh;
                    }

                    update_hud();

                    // won't respawn
                    persistence[cmap] |= (1 << sp_iter->delay);
                    remove_sprite(sp_iter);

                    play_queue = FX_ITEM;
                    if (sp_iter->subtype == OBT_DR)
                    {
                        draw_map();
                        return;
                    }
                }
                break;

            case ST_PLATFORM:

                if (sp_iter->iy)
                {
                    if (sp_iter->delay++ == sp_iter->frame)
                    {
                        sp_iter->delay = 0;
                        sp_iter->iy *= -1;
                    }

                    if (check_collision_point(sp_iter, sprites[PBODY].x + 4, sprites[PBODY].y + 8)
                            || check_collision_point(sp_iter, sprites[PBODY].x + 12, sprites[PBODY].y + 8))
                    {
                        sp_iter->y += sp_iter->iy;
                        sprites[PBODY].y = sp_iter->y - 8;
                        sprites[PHEAD].y = sprites[PBODY].y - 8;
                    }
                    else
                        sp_iter->y += sp_iter->iy;
                }
                else
                {
                    if (sp_iter->delay++ == sp_iter->frame)
                    {
                        sp_iter->delay = 0;
                        sp_iter->ix *= -1;
                    }

                    if (check_collision_point(sp_iter, sprites[PBODY].x + 4, sprites[PBODY].y + 8)
                            || check_collision_point(sp_iter, sprites[PBODY].x + 12, sprites[PBODY].y + 8))
                    {
                        if (sp_iter->ix > 0)
                            k = !is_map_blocked(sprites[PHEAD].x + 16 + 2, sprites[PHEAD].y)
                                && !is_map_blocked(sprites[PBODY].x + 16 + 2, sprites[PBODY].y)
                                && !is_map_blocked(sprites[PBODY].x + 16 + 2, sprites[PBODY].y + 7);
                        else
                            k = !is_map_blocked(sprites[PHEAD].x - 2, sprites[PHEAD].y)
                                && !is_map_blocked(sprites[PBODY].x - 2, sprites[PBODY].y)
                                && !is_map_blocked(sprites[PBODY].x - 2, sprites[PBODY].y + 7);

                        sp_iter->x += sp_iter->ix;
                        sprites[PBODY].y = sp_iter->y - 8;
                        sprites[PHEAD].y = sprites[PBODY].y - 8;
                        if (k)
                        {
                            sprites[PBODY].x += sp_iter->ix;
                            sprites[PHEAD].x = sprites[PBODY].x;
                        }
                    }
                    else
                        sp_iter->x += sp_iter->ix;
                }

                break;
        }
        if (sp_iter->alive)
            sp1_MoveSprPix(sp_iter->s, &gr, sp_iter->s->frame, sp_iter->x, sp_iter->y);
    }
}

// for the deadman
const struct sp1_Rect deadr = { 6, 12, 8, 7 };

    void
run_play()
{
    fade_out(768);

    // setup the bg tiles
    setup_tiles(map_tiles, MAP_TILE_BASE, MAP_TILE_LEN);

    // 2x numbers
    setup_tiles(numbersx2, NUMBERSX2_BASE, NUMBERSX2_LEN);

    // ship wreck
    setup_tiles(wreck, WRECK_BASE, WRECK_LEN);

    // object tiles for the HUD
    setup_tiles(obj_tiles, OBJ_TILES_BASE, OBJ_TILES_LEN);

    sp1_ClearRectInv(&cr, BRIGHT | INK_BLACK | PAPER_BLACK, 32, SP1_RFLAG_TILE | SP1_RFLAG_COLOUR);
    sp1_UpdateNow();

    // this will wait for vsync
#asm
    halt
#endasm
        ucl_uncompress(play, ((uchar*)0x4000));
    sp1_Validate(cr);

    // setup the game
    memset(persistence, 0, TOTAL_ROOMS);
    memset(inv, 0, MAX_INV);

    player_dir = DIR_RIGHT;

    // all data that has to be set to 0
    memset(&game_data, 0, 14);

    hits = MAX_HITS;
    update_hud();

    sprites[PHEAD].x = 16 * 8;
    sprites[PHEAD].y = 10 * 8;
    sprites[PHEAD].s->frame = phead;

    sprites[PBODY].x = sprites[PHEAD].x;
    sprites[PBODY].y = sprites[PHEAD].y + 8;
    sprites[PBODY].s->frame = pbody + (player_anim[player_dir + player_frame] << 6);

    setup_map(0);

    while(1)
    {
        if (paused)
        {
            in_WaitForKey();
            key = in_Inkey();
            in_WaitForNoKey();

            if (key == keys[4])
            {
                paused = 0;

                draw_map();
                draw_entities();
                sp1_UpdateNow();
            }
            continue;
        }

        if (endgame || gameover || in_Inkey() == 12)
            // exit current game
            break;

        if (in_Inkey() == keys[4])
        {
            playfx(FX_SELECT);

            paused = 1;

            sp1_ClearRectInv(&gr, INK_BLACK | PAPER_BLACK, 32, SP1_RFLAG_TILE | SP1_RFLAG_COLOUR);
            hide_all_sprites();

            print(10, 10, INK_WHITE|BRIGHT, "GAME  PAUSED");
            sp1_UpdateNow();

            in_WaitForNoKey();
            continue;
        }

        player_moved = 0;

        // gravity
        if (player_frame < JUMP_FRAME
                && !is_map_blocked(sprites[PBODY].x + 4, sprites[PBODY].y + 8)
                && !is_map_blocked(sprites[PBODY].x + 12, sprites[PBODY].y + 8))
        {
            player_h = JUMP_SEQ - 1;
            player_frame = JUMP_FRAME + 1;
            set_body_frame();
            player_moved = 1;
        }

        key = (joyfunc)(&joy_k);
        if (key & in_LEFT && !(key & in_RIGHT))
        {
            if (player_dir != DIR_LEFT)
            {
                player_dir = DIR_LEFT;
                sprites[PHEAD].s->frame = phead + 8 * 8;
                if (player_frame < JUMP_FRAME)
                    player_frame = 0;
            }

            set_body_frame();
            player_moved = 1;

            for (k = 0; k < 3; ++k)
            {
                if (!sprites[PBODY].x
                        || is_map_blocked(sprites[PHEAD].x, sprites[PHEAD].y)
                        || is_map_blocked(sprites[PBODY].x, sprites[PBODY].y + 7))
                    break;

                sprites[PHEAD].x--;
                sprites[PBODY].x--;
            }

            if (!sprites[PBODY].x && cmap)
            {
                setup_map(cmap - 1);
                sprites[PHEAD].x = sprites[PBODY].x = ROOM_WIDTH * 16 - 16;
                continue;
            }

            k = map_xy((sprites[PBODY].x - 1) >> 4, (sprites[PBODY].y + 4) >> 4);
            if (is_door_tile(k))
                use_key();
        }

        if (key & in_RIGHT && !(key & in_LEFT))
        {
            if (player_dir != DIR_RIGHT)
            {
                player_dir = DIR_RIGHT;
                sprites[PHEAD].s->frame = phead;
                if (player_frame < JUMP_FRAME)
                    player_frame = 0;
            }

            set_body_frame();
            player_moved = 1;

            for (k = 0; k < 3; ++k)
            {
                if (sprites[PBODY].x == ROOM_WIDTH * 16 - 16
                        || is_map_blocked(sprites[PHEAD].x + 16, sprites[PHEAD].y)
                        || is_map_blocked(sprites[PBODY].x + 16, sprites[PBODY].y + 7))
                    break;

                sprites[PHEAD].x++;
                sprites[PBODY].x++;
            }

            if (sprites[PBODY].x == ROOM_WIDTH * 16 - 16 && cmap < TOTAL_ROOMS - 1)
            {
                setup_map(cmap + 1);
                sprites[PHEAD].x = sprites[PBODY].x = 0;
                continue;
            }

            k = map_xy((sprites[PBODY].x + 17) >> 4, (sprites[PBODY].y + 4) >> 4);
            if (is_door_tile(k))
                use_key();
        }

        // had to split this in two because Z88DK
        if (key & in_UP && boots == 1 && player_h > JUMP_UP - 1)
        {
            boots++;
            play_queue = FX_HIT;
            goto do_jump;
        }

        if (key & in_UP && !player_h)
        {
do_jump:
            player_frame = JUMP_FRAME;
            set_body_frame();
            player_h = 1;
        }

        if (gun > 1)
        {
            gun--;
            if (gun == 1)
                update_hud();
        }

        if (key & in_FIRE && gun == 1)
        {

            /*
               gh = sprites[PHEAD].y + 4;
               gw = sprites[PHEAD].x + 4;

               if (player_dir)
               {
               for (i = 0; i < 5; ++i)
               {
               if (is_map_blocked(gw, gh))
               break;
               gw -= 2;
               }
               }

               add_zap(gw, gh, player_dir ^ DIR_LEFT, ST_ZAP);

               gun = 48;
            // not very efficient
            update_hud();
             */
#asm
            ld hl, _sprites + 2

                ld d, 0
                ld e, (hl) ; x
                inc e
                inc e
                inc e
                inc e

                inc hl
                ld b, 0
                ld c, (hl) ; y
                inc c
                inc c
                inc c
                inc c

                ld a, (_player_dir)
                or a
                jr z, key_fire_not_left

                push bc

                ld a, 5

                .key_fire_left_loop
                push af
                push de
                push de
                push bc
                call _is_map_blocked
                pop de
                pop af
                dec hl
                jr z, key_fire_left_done

                dec e
                dec e

                dec a
                jr nz, key_fire_left_loop

                .key_fire_left_done
                pop bc

                .key_fire_not_left

                push de
                push bc
                ld a, (_player_dir)
                xor 6
                and 6
                ld e, a
                push de
                ld e, ST_ZAP
                push de
                call _add_zap
                pop af
                pop af
                pop af
                pop af

                ld hl, _gun
                ld (hl), 48
                call _update_hud
#endasm
        }

        if (player_h)
        {
            if (player_h > JUMP_UP)
            {
                if (player_frame != JUMP_FRAME + 1)
                {
                    player_frame = JUMP_FRAME + 1;
                    set_body_frame();
                }

                for (k = 0; k < jump_seq[player_h - 1]; ++k)
                {
                    if (sprites[PBODY].y == ROOM_HEIGHT * 16 - 8
                            || is_map_blocked(sprites[PBODY].x + 4, sprites[PBODY].y + 8)
                            || is_map_blocked(sprites[PBODY].x + 12, sprites[PBODY].y + 8))
                        break;
                    sprites[PHEAD].y++;
                    sprites[PBODY].y++;
                }

                if (sprites[PBODY].y == ROOM_HEIGHT * 16 - 8 && cmap <= TOTAL_ROOMS - MAP_WIDTH)
                {
                    setup_map(cmap + MAP_WIDTH);
                    sprites[PHEAD].y = 0;
                    sprites[PBODY].y = 8;
                    continue;
                }

                k = map_xy((sprites[PBODY].x + 8) >> 4, (sprites[PBODY].y + 9) >> 4);
                if (is_door_tile(k))
                    use_key();
            }
            else
            {
                for (k = 0; k < -jump_seq[player_h - 1]; ++k)
                {
                    if (sprites[PHEAD].y == 0
                            || is_map_blocked(sprites[PHEAD].x + 4, sprites[PHEAD].y)
                            || is_map_blocked(sprites[PHEAD].x + 12, sprites[PHEAD].y))
                    {
                        player_h++;
                        if (boots)
                            boots++;

                        play_queue = FX_HIT;
                        break;
                    }
                    sprites[PHEAD].y--;
                    sprites[PBODY].y--;
                }

                if (sprites[PHEAD].y == 0 && cmap >= MAP_WIDTH)
                {
                    setup_map(cmap - MAP_WIDTH);
                    sprites[PBODY].y = ROOM_HEIGHT * 16 - 8;
                    sprites[PHEAD].y = sprites[PBODY].y - 8;
                    continue;
                }

                k = map_xy((sprites[PHEAD].x + 8) >> 4, (sprites[PHEAD].y - 1) >> 4);
                if (is_door_tile(k))
                    use_key();
            }

            // horizontal modifier
            if (player_moved)
            {
                if (player_dir == DIR_LEFT)
                {
                    if (!is_map_blocked(sprites[PHEAD].x, sprites[PHEAD].y)
                            && !is_map_blocked(sprites[PBODY].x, sprites[PBODY].y + 7))
                    {
                        sprites[PHEAD].x--;
                        sprites[PBODY].x--;
                    }
                }
                else
                {
                    if (!is_map_blocked(sprites[PHEAD].x + 16, sprites[PHEAD].y)
                            && !is_map_blocked(sprites[PBODY].x + 16, sprites[PBODY].y + 7))
                    {
                        sprites[PHEAD].x++;
                        sprites[PBODY].x++;
                    }
                }
            }

            if (player_h < JUMP_SEQ)
            {
                if (player_h < JUMP_UP && !(key & in_UP))
                    player_h = JUMP_UP;
                else
                    player_h++;
            }

            if (is_map_blocked(sprites[PBODY].x + 4, sprites[PBODY].y + 8)
                    || is_map_blocked(sprites[PBODY].x + 12, sprites[PBODY].y + 8))
            {
                player_frame = 0;
                set_body_frame();
                player_h = 0;

                if (boots)
                    boots = 1;

                play_queue = FX_HIT;
            }
        }
        else
        {
            // not jumping
            if (player_moved)
            {
                sprites[PBODY].delay++;
                if (sprites[PBODY].delay > 1)
                {
                    player_frame++;
                    if (player_frame > 3)
                        player_frame = 0;

                    sprites[PBODY].delay = 0;
                    set_body_frame();
                }
            }
            else
            {
                sprites[PBODY].delay++;
                if (sprites[PBODY].delay > 2 && player_frame)
                {
                    sprites[PBODY].delay = 0;
                    player_frame = 0;
                    set_body_frame();
                }
            }
        }

        update_entities();
        if (sp_collect)
            collect_sprites();

        sp1_MoveSprPix(sprites[PHEAD].s, &gr, sprites[PHEAD].s->frame, sprites[PHEAD].x, sprites[PHEAD].y);
        sp1_MoveSprPix(sprites[PBODY].s, &gr, sprites[PBODY].s->frame, sprites[PBODY].x, sprites[PBODY].y);

        if (invulnerable)
        {
            invulnerable--;
            if (invulnerable & 2)
                hide_player();

            if (!invulnerable)
                zx_border(INK_BLACK);
        }

        if (play_queue)
        {
            playfx(play_queue);
            play_queue = 0;
        }

        wait();
        sp1_UpdateNow();
        if (tgame != 0xffff)
            ++tgame;
    }

    zx_border(INK_BLACK);

    if (gameover)
    {
        in_WaitForNoKey();
        fade_out(640);
        sp1_ClearRectInv(&gr, INK_BLACK | PAPER_BLACK, 32, SP1_RFLAG_TILE | SP1_RFLAG_COLOUR);
    }

    destroy_type_sprite(ST_ALL);
    if (sp_collect)
        collect_sprites();

    // the player sprite is never destroyed, so hide it
    hide_player();
    sp1_UpdateNow();

    if (endgame)
        run_endgame();

    if (gameover)
    {
        ucl_uncompress(dead, ((uchar*)PRDR_ADDR));
        setup_tiles(((uchar*)PRDR_ADDR), DEAD_BASE, DEAD_LEN);
        sp1_PutTilesInv(&deadr, dead_tbl);

        print(11, 5, INK_WHITE|BRIGHT, "GAME  OVER");
        wait();
        sp1_UpdateNow();

        ucl_uncompress(prdr, ((uchar*)PRDR_ADDR));
        ucl_uncompress(song3, ((uchar*)PRDR_SONG));
        prdr_play();
    }

    if (endgame || gameover)
    {
        if (score > hiscore)
        {
            hiscore = score;
            print(10, 14, INK_CYAN|BRIGHT, "NEW  HISCORE");
            sp1_UpdateNow();
            playfx(FX_SELECT);
        }

        delay(64);
        in_WaitForNoKey();
    }


    fade_out(768);
}

uchar idle = 0;

    int
main()
{
#asm
    di
#endasm

        // malloc setup
        mallinit();
    sbrk(MALLOC_ADDR, MALLOC_SIZE);

    setup_int();

    zx_border(INK_BLACK);
    // sp1.lib
    sp1_Initialize(SP1_IFLAG_MAKE_ROTTBL | SP1_IFLAG_OVERWRITE_TILES | SP1_IFLAG_OVERWRITE_DFILE, INK_BLACK | PAPER_BLACK, ' ');

    sp1_Validate(cr);
#asm
    ei
#endasm

        // setup our font
        setup_tiles(numbers, 32 + 16, NUMBERS_LEN);
    setup_tiles(font, 32 + 32, FONT_LEN);

    init_sprites();

    run_intro();
    draw_menu();

    while(1)
    {
        key = in_Inkey();
        if (key)
        {
            if (key == '4')
            {
                playfx(FX_SELECT);

                in_WaitForNoKey();
                run_redefine_keys();
                idle = 0;
                draw_menu();
            }
            if (key == '1' || key == '2' || key == '3')
            {
                joy_k.left = in_LookupKey(keys[0]);
                joy_k.right = in_LookupKey(keys[1]);
                joy_k.up = in_LookupKey(keys[2]);
                joy_k.fire = in_LookupKey(keys[3]);

                // not used
                joy_k.down = in_LookupKey(keys[0]);

                if (key == '1')
                    joyfunc = in_JoyKeyboard;
                if (key == '2')
                    joyfunc = in_JoyKempston;
                if (key == '3')
                    joyfunc = in_JoySinclair1;

                playfx(FX_SELECT);

                run_play();
                idle = 0;
                draw_menu();
            }
        }

        if (idle++ == 255)
        {
            // go back to the welcome screen after a while
            // if the player doesn't do anything
            idle = 0;
            draw_menu();
        }

        wait();
        sp1_UpdateNow();
    }
}

