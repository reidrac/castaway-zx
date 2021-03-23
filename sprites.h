#ifndef _SPRITES_H
#define _SPRITES_H

#ifndef NULL
#define NULL    ((uchar *)0)
#endif

#define MAX_SPRITES 8

#define PHEAD		0
#define PBODY		1

#define ST_ALL     	254 // all but the player!
#define ST_PLAYER  	1
#define ST_PLATFORM	2
#define ST_OBJECT	4
#define ST_ZAP		8
#define ST_EZAP		16
#define ST_WZAP		32
#define ST_ENEMY	64
#define ST_EFX		128

#define OBJ_USED  255

// this is usually fullscreen; define it in your main
extern struct sp1_Rect cr;

struct ssprites {
    struct sp1_ss *s;

    // position
    uchar x;
    uchar y;
    // movement
    char ix;
    char iy;

    uchar frame;
    uchar delay;
    uchar alive;
    uchar type;
    uchar subtype;

    struct ssprites *n;
};

struct ssprites sprites[MAX_SPRITES];

// lists
struct ssprites *sp_free;
struct ssprites *sp_used;

// shared iterators
struct ssprites *sp_iter, *sp_iter2;

// if there are items to garbage collect
int sp_collect = 0;

void
init_sprites()
{
    // to be run ONCE

    sprites[PBODY].s = sp1_CreateSpr(SP1_DRAW_MASK2LB, SP1_TYPE_2BYTE, 2, 8, 1);
    sp1_AddColSpr(sprites[PBODY].s, SP1_DRAW_MASK2, 0, 40, 0);
    sp1_AddColSpr(sprites[PBODY].s, SP1_DRAW_MASK2RB, 0, 0, 0);

    sprites[PBODY].alive = 1;
    sprites[PBODY].n = NULL;
    sprites[PBODY].type = ST_PLAYER;

    sprites[PHEAD].s = sp1_CreateSpr(SP1_DRAW_MASK2LB, SP1_TYPE_2BYTE, 2, 8, 1);
    sp1_AddColSpr(sprites[PHEAD].s, SP1_DRAW_MASK2, 0, 40, 0);
    sp1_AddColSpr(sprites[PHEAD].s, SP1_DRAW_MASK2RB, 0, 0, 0);

    sprites[PHEAD].alive = 1;
    sprites[PHEAD].n = sprites + 1;
    sprites[PHEAD].type = ST_PLAYER;

    // init our lists

    sp_used = sprites;
    sp_free = sprites + 2; // the player is two sprites

    for (i = 2; i < MAX_SPRITES - 1; ++i)
    {
        sprites[i].n = sprites + i + 1;
        sprites[i].alive = 0;
    }

    sprites[MAX_SPRITES - 1].n = NULL;
    sprites[MAX_SPRITES - 1].alive = 0;
}

void collect_sprites();

int
add_sprite()
{
    struct ssprite *t;

    if (!sp_free)
    {
        // FIXME
        zx_border(INK_BLUE);
        return 0;
    }

    t = sp_used;
    sp_used = sp_free;
    sp_free = sp_free->n;

    memset(sp_used, 0, sizeof(struct ssprites));
    sp_used->n = t;
    sp_used->alive = 1;

    return 1;
}

void
collect_sprites()
{
    struct ssprites *t, *tp;

    // enable this if you're not checking before calling!
    //if (!sp_collect)
    //    return;

    // current, previous
    for (t = tp = sp_used; t && sp_collect;)
    {
        if (!t->alive)
        {
            if (t == sp_used)
            {
                tp = sp_free;
                sp_free = sp_used;
                sp_used = sp_used->n;
                sp_free->n = tp;
                t = tp = sp_used;
                --sp_collect;
                continue;
            }
            else
            {
                tp->n = t->n;
                t->n = sp_free;
                sp_free = t;
                t = tp->n;
                --sp_collect;
                continue;
            }
        }
        tp = t;
        t = t->n;
    }
}

void
destroy_type_sprite(uchar type)
{
    struct ssprites *t;

    if (!sp_used)
        return;

    for (t = sp_used; t; t = t->n)
        if (t->alive && t->type & type)
        {
            sp1_MoveSprPix(t->s, &cr, NULL, 0, 200);
            sp1_DeleteSpr(t->s);
            t->alive = 0;
            ++sp_collect;
        }
}

void
remove_sprite(struct ssprites *s)
{
    sp1_MoveSprPix(s->s, &cr, NULL, 0, 200);
    sp1_DeleteSpr(s->s);
    s->alive = 0;
    ++sp_collect;
}

void
hide_all_sprites()
{
    for (sp_iter = sp_used; sp_iter; sp_iter = sp_iter->n)
        sp1_MoveSprPix(sp_iter->s, &cr, sp_iter->s->frame, 0, 200);
}

#endif // _SPRITES_H
