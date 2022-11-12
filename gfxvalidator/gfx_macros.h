#ifndef __GFX_GFX_MACROS_H__
#define __GFX_GFX_MACROS_H__

#define SEGMENT_UNINITIALIZED   -1

#define DMA1_LEN(gfx)       _SHIFTR((gfx)->words.w0, 0, 16)
#define DMA1_PARAM(gfx)     _SHIFTR((gfx)->words.w0, 16, 8)

#ifdef F3DEX_GBI_2
#define DMA_MM_LEN(gfx)     _SHIFTR((gfx)->words.w0, 19, 5)
#define DMA_MM_OFS(gfx)     (_SHIFTR((gfx)->words.w0, 8, 8) * 8)
#define DMA_MM_IDX(gfx)     _SHIFTR((gfx)->words.w0, 0, 8)

#define DMA_MM_EXPECTED_SIZE(actualSize)    (((actualSize) - 1) >> 3)

#define MOVE_WORD_IDX(gfx)  _SHIFTR((gfx)->words.w0, 16, 8)
#define MOVE_WORD_OFS(gfx)  _SHIFTR((gfx)->words.w0, 0, 16)
#define MOVE_WORD_DATA(gfx) ((gfx)->words.w1)

#define VERTEX_BUFFER_SIZE  32
#define MAX_VERTEX_VALUE    (VERTEX_BUFFER_SIZE * 2)

#else
#define DMA_MM_LEN(gfx)     DMA1_LEN(gfx)
#define DMA_MM_OFS(gfx)     0
#define DMA_MM_IDX(gfx)     DMA1_PARAM(gfx)

#define DMA_MM_EXPECTED_SIZE(actualSize)    (actualSize)

#define MOVE_WORD_IDX(gfx)  _SHIFTR((gfx)->words.w0, 0, 8)
#define MOVE_WORD_OFS(gfx)  _SHIFTR((gfx)->words.w0, 8, 16)
#define MOVE_WORD_DATA(gfx) ((gfx)->words.w1)

#define VERTEX_BUFFER_SIZE  16
#define MAX_VERTEX_VALUE    (VERTEX_BUFFER_SIZE * 10)

#endif

#endif