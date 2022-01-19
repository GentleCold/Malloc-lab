/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 *
 * 采用隐式链表，下一次适配，立即合并，省略已分配块的脚部的策略
 * 
 * 2022.1.18~2022.1.19
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "666",
    /* First member's full name */
    "Gentlecold",
    /* First member's email address */
    "666@qq.com",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* My function */
static void *extend(size_t size);
static void *coalesce(char *bp);
static void *find(size_t size);
static void place(char *bp, size_t newsize);
// static void coalesceAll(void);

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* My macros */
#define WSIZE 4
#define DSIZE 8
#define CHUNKSIZE (1<<12)

#define PACK(size, alloc) ((size)|(alloc))
#define PUT(p, val) (*(unsigned int *)(p)=(IFSET(p))?((val)|0x2):(val))
#define SET(bp) (*(unsigned int *)(bp)|=0x2)
#define UNSET(bp) (*(unsigned int *)(bp)&=~0x2)

#define SIZE(bp) (*(unsigned int *)(bp)&~0x7)
#define IFSET(bp) (*(unsigned int *)(bp)&0x2)
#define ALLOC(bp) (*(unsigned int *)(bp)&0x1)

#define HPTR(bp) ((char *)(bp)-WSIZE)
#define FPTR(bp) ((char *)(bp)+SIZE(HPTR(bp))-DSIZE)

#define PREV(bp) ((char *)(bp)-SIZE((char *)(bp)-DSIZE))
#define NEXT(bp) (FPTR(bp)+DSIZE)

static char *headlist_p;
static char *prevlist_p;

/*
 * extend the heap
 */
static void *extend(size_t size)
{
    char *bp=0;
    size=(size%2)?(size+1):size;
    size*=WSIZE;
    if((bp=mem_sbrk(size))==(void*)-1)
        return NULL;
    //UNSET(HPTR(bp));
    PUT(HPTR(bp), PACK(size, 0));
    PUT(FPTR(bp), PACK(size, 0));
    UNSET(HPTR(NEXT(bp)));
    PUT(HPTR(NEXT(bp)), PACK(0, 1));

    return coalesce(bp);
}

/*
 * coalesce the blank
 */
static void *coalesce(char *bp)
{
    size_t flag1;
    size_t flag2=ALLOC(HPTR(NEXT(bp)));
    size_t size=SIZE(HPTR(bp));

    if(IFSET(HPTR(bp)))
        flag1=1;
    else
        flag1=ALLOC(HPTR(PREV(bp)));

    if(flag1&&flag2)
        return bp;
    else if(!flag1&&flag2)
    {
        if(prevlist_p==bp)                 //remember to change prevlist_p!!!
            prevlist_p=PREV(bp);
        size+=SIZE(HPTR(PREV(bp)));
        PUT(HPTR(PREV(bp)), PACK(size, 0));
        bp=PREV(bp);
        PUT(FPTR(bp), PACK(size, 0));
    }
    else if(flag1&&!flag2)
    {
        if(prevlist_p==NEXT(bp))           //remember to change prevlist_p!!!!!
            prevlist_p=bp;
        size+=SIZE(HPTR(NEXT(bp)));
        PUT(HPTR(bp), PACK(size, 0));
        PUT(FPTR(bp), PACK(size, 0));
    }
    else
    {
        if(prevlist_p==NEXT(bp)||prevlist_p==bp)//remember to change prevlist_p!!!!!!!
            prevlist_p=PREV(bp);
        size+=SIZE(HPTR(PREV(bp)))+SIZE(HPTR(NEXT(bp)));
        PUT(HPTR(PREV(bp)), PACK(size, 0));
        bp=PREV(bp);
        PUT(FPTR(bp), PACK(size, 0));
    }
    return bp;
}

/*
 * coalesce all blank
 */
// static void coalesceAll(void)
// {
//     char *bp=headlist_p;
//     while(SIZE(HPTR(bp)))
//     {
//         if(!ALLOC(HPTR(bp)))
//         {
//             coalesce(bp);
//         }
//         bp=NEXT(bp);
//     }
//     return;
// }

/* 
 * find the proper blank
 */
static void *find(size_t size)//折合了最佳适配和下一次适配，提高了2分
{
    char *bp=prevlist_p;
    while(SIZE(HPTR(bp))>0)
    {
        if(!ALLOC(HPTR(bp))&&SIZE(HPTR(bp))>=size)
        {
            prevlist_p=bp;
            return bp;
        }
        else
        {
            bp=NEXT(bp);
        }
    }
    bp=headlist_p;
    size_t min=20*(1<<20);
    char *imin=prevlist_p;
    int flag=0;
    while(bp!=prevlist_p)
    {
        if(!ALLOC(HPTR(bp))&&SIZE(HPTR(bp))>=size)
        {
            flag=1;
            if(SIZE(HPTR(bp))-size<min)
            {
                min=SIZE(HPTR(bp))-size;
                imin=bp;
            }
        }   
        bp=NEXT(bp);
    }
    if(flag)
    {
        prevlist_p=imin;
        return imin;
    }
    return NULL;
}

/* 
 * allocate the blank
 */
static void place(char *bp, size_t newsize)
{
    size_t left=SIZE(HPTR(bp))-newsize;

    if(left>=DSIZE)
    {
        PUT(HPTR(bp), PACK(newsize, 1));
        PUT(HPTR(NEXT(bp)), PACK(left, 0));
        PUT(FPTR(NEXT(bp)), PACK(left, 0));
        SET(HPTR(NEXT(bp)));
    }
    else
    {
        PUT(HPTR(bp), PACK(SIZE(HPTR(bp)), 1));
        SET(HPTR(NEXT(bp)));
    }
}

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    //mem_init();
    headlist_p=mem_sbrk(4*WSIZE);
    
    PUT(headlist_p, 0);
    PUT(headlist_p+WSIZE, PACK(DSIZE, 1));
    PUT(headlist_p+2*WSIZE, PACK(DSIZE, 1));
    PUT(headlist_p+3*WSIZE, PACK(0, 1));
    SET(headlist_p+3*WSIZE);
    headlist_p+=2*WSIZE;
    prevlist_p=headlist_p;
    if(extend(2*DSIZE/WSIZE)==NULL)//提高初始块利用率(针对trace提高了1分，小trick)
        return -1;
    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    // int newsize = ALIGN(size + SIZE_T_SIZE);
    // void *p = mem_sbrk(newsize);
    // if (p == (void *)-1)
	   // return NULL;
    // else {
    //     *(size_t *)p = size;
    //     return (void *)((char *)p + SIZE_T_SIZE);
    // }
    if(size==0)
        return NULL;

    size_t newsize=ALIGN(size+WSIZE);
    size_t extendsize;
    char *bp;

    if((bp=find(newsize)))
        place(bp, newsize);
    else
    {
        // coalesceAll();
        // if((bp=find(newsize)))
        //     place(bp, newsize);
        // else
        // {
            //newsize=(newsize>CHUNKSIZE)?newsize:CHUNKSIZE;          BUG!!!!!!!
            extendsize=(newsize>CHUNKSIZE)?newsize:CHUNKSIZE;
            bp=extend(extendsize/WSIZE);
            if(bp)
                place(bp, newsize);
        // }
    }
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    size_t size=SIZE(HPTR(ptr));

    PUT(HPTR(ptr), PACK(size, 0));
    PUT(FPTR(ptr), PACK(size, 0));
    UNSET(HPTR(NEXT(ptr)));
    coalesce(ptr);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 * 这一部分的优化很乱，但是帮助提高了一分
 */
void *mm_realloc(void *ptr, size_t size)
{
    if(ptr==NULL)
        return mm_malloc(size);

    if(size==0)
    {
        mm_free(ptr);
        return NULL;
    }

    void *oldptr=ptr;
    void *newptr=ptr;
    size_t copysize=SIZE(HPTR(ptr))-WSIZE;
    size_t newsize=ALIGN(size+WSIZE);

    if(newsize<=SIZE(HPTR(ptr)))
    {
        if(newsize==SIZE(HPTR(ptr)))
            return newptr;
        size_t left=SIZE(HPTR(ptr))-newsize;
        PUT(HPTR(ptr), PACK(newsize, 1));
        PUT(HPTR(NEXT(ptr)), PACK(left, 0));
        PUT(FPTR(NEXT(ptr)), PACK(left, 0));
        SET(HPTR(NEXT(ptr)));
        coalesce(NEXT(ptr));
    }
    else
    {
        if(!IFSET(HPTR(ptr))&&(SIZE(HPTR(PREV(ptr)))+SIZE(HPTR(ptr)))>=newsize)
        {
            if(prevlist_p==ptr)
                prevlist_p=PREV(ptr);
            newptr=PREV(ptr);
            if((SIZE(HPTR(PREV(ptr)))+SIZE(HPTR(ptr)))==newsize)
            {
                PUT(HPTR(newptr), PACK(newsize, 1));
                memmove(newptr, ptr, copysize);
                SET(HPTR(NEXT(newptr)));
                return newptr;
            }
            size_t left=(SIZE(HPTR(PREV(ptr)))+SIZE(HPTR(ptr)))-newsize;
            PUT(HPTR(newptr), PACK(newsize, 1));
            memmove(newptr, ptr, copysize);
            PUT(HPTR(NEXT(newptr)), PACK(left, 0));
            PUT(FPTR(NEXT(newptr)), PACK(left, 0));
            SET(HPTR(NEXT(newptr)));

        }
        else if(!ALLOC(HPTR(NEXT(ptr)))&&(SIZE(HPTR(NEXT(ptr)))+SIZE(HPTR(ptr)))>=newsize)
        {
            if(prevlist_p==NEXT(ptr))
                prevlist_p=ptr;
            if((SIZE(HPTR(NEXT(ptr)))+SIZE(HPTR(ptr)))==newsize)
            {
                PUT(HPTR(ptr), PACK(newsize, 1));
                SET(HPTR(NEXT(ptr)));
                return newptr;
            }
            size_t left=(SIZE(HPTR(NEXT(ptr)))+SIZE(HPTR(ptr)))-newsize;
            PUT(HPTR(ptr), PACK(newsize, 1));
            PUT(HPTR(NEXT(ptr)), PACK(left, 0));
            PUT(FPTR(NEXT(ptr)), PACK(left, 0));
            SET(HPTR(NEXT(ptr)));
        }
        else if(!IFSET(HPTR(ptr))&&!ALLOC(HPTR(NEXT(ptr)))&&(SIZE(HPTR(PREV(ptr)))+SIZE(HPTR(ptr))+SIZE(HPTR(NEXT(ptr))))>=newsize)
        {
            if(prevlist_p==NEXT(ptr)||prevlist_p==ptr)
                prevlist_p=PREV(ptr);
            newptr=PREV(ptr);
            if((SIZE(HPTR(PREV(ptr)))+SIZE(HPTR(ptr))+SIZE(HPTR(NEXT(ptr))))==newsize)
            {
                PUT(HPTR(newptr), PACK(newsize, 1));
                memmove(newptr, ptr, copysize);
                SET(HPTR(NEXT(newptr)));
                return newptr;
            }
            size_t left=(SIZE(HPTR(PREV(ptr)))+SIZE(HPTR(ptr))+SIZE(HPTR(NEXT(ptr))))-newsize;
            PUT(HPTR(newptr), PACK(newsize, 1));
            memmove(newptr, ptr, copysize);
            PUT(HPTR(NEXT(newptr)), PACK(left, 0));
            PUT(FPTR(NEXT(newptr)), PACK(left, 0));
            SET(HPTR(NEXT(newptr)));
        }
        else
        {
            newptr = mm_malloc(size);
            if (newptr == NULL)
                return NULL;
            
            memcpy(newptr, oldptr, copysize);
            mm_free(oldptr);
        }
    }
    
    return newptr;
}
