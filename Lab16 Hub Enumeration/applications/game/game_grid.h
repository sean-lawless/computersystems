/*...................................................................*/
/*                                                                   */
/*   Module:  game_grid.h                                            */
/*   Version: 2019.0                                                 */
/*   Purpose: 2D game grad structures and interface                  */
/*                                                                   */
/*...................................................................*/
/*                                                                   */
/*                   Copyright 2019, Sean Lawless                    */
/*                                                                   */
/*                      ALL RIGHTS RESERVED                          */
/*                                                                   */
/* Redistribution and use in source, binary or derived forms, with   */
/* or without modification, are permitted provided that the          */
/* following conditions are met:                                     */
/*                                                                   */
/*  1. Redistributions in any form, including but not limited to     */
/*     source code, binary, or derived works, must include the above */
/*     copyright notice, this list of conditions and the following   */
/*     disclaimer.                                                   */
/*                                                                   */
/*  2. Any change or addition to this copyright notice requires the  */
/*     prior written permission of the above copyright holder.       */
/*                                                                   */
/* THIS SOFTWARE IS PROVIDED ''AS IS''. ANY EXPRESS OR IMPLIED       */
/* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES */
/* OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE       */
/* DISCLAIMED. IN NO EVENT SHALL ANY AUTHOR AND/OR COPYRIGHT HOLDER  */
/* BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,          */
/* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED   */
/* TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,     */
/* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON */
/* ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,   */
/* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY    */
/* OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE           */
/* POSSIBILITY OF SUCH DAMAGE.                                       */
/*...................................................................*/
#ifndef _GAME_GRID_H
#define _GAME_GRID_H

#include <system.h>

#define CIRCULAR_LIST     TRUE

// Tile size
#define TILE_WIDTH        16 // In pixels
#define TILE_HEIGHT       16 // In pixels

// Determine grid onfiguration based on screen size
#if PIXEL_WIDTH == 640 // 640x480
#define GAME_GRID_WIDTH   28
#define GAME_GRID_HEIGHT  28
#define GAME_GRID_START_X 180
#define GAME_GRID_START_Y 20
#elif PIXEL_WIDTH == 1024 // 1024x768
#define GAME_GRID_WIDTH   48
#define GAME_GRID_HEIGHT  48
#define GAME_GRID_START_X 256
#define GAME_GRID_START_Y 0
#else
#error "Game grid supports screen resolution of 640x480 or 1024x768"
#endif

// Tile flags
#define IS_VISIBLE    (1 << 24)
#define SHOW_REVERSE  (1 << 25)
#define SHOW_FLIPPED  (1 << 26)
#define NO_FLIP       (1 << 27)
#define NO_REVERSE    (1 << 28)

typedef struct Tile
{
  struct double_link list;
  u8 *bitmap;
  int tileNum; // Index into bitmap
  u32 color;   // Color of binary bitmap
  u32 flags;
} Tile;

typedef struct
{
  int x, y; // size of the rectangular world
  Tile *tiles; // array of tiles representing game grid
  int score; // Current game score, if any
  int level; // Current game level, if any
  struct timer_task *round; // Game round periodic timer
  int period; // Game period
} World;

typedef struct
{
  Tile tile;
  u32 count, total;  // total must be multiple of 2
  u32 period;
  int deltaX, deltaY; // in pixels
  int locationX, locationY; // current (pre-effect) world grids
  int destinationX, destinationY; // next (post-effect) world grids
  int (*poll) (u32 id, void *data, void *context);
} EffectTile;

extern Tile GameGrid[];

// Tile and Game Grid interface
void TileClear(u32 x, u32 y);
void TileDisplayScreen(Tile *tile, u32 x, u32 y);
void TileDisplayGrid(int x, int y);
int TileEffectStart(EffectTile *effect, int newX, int newY);
int TileEffectNext(EffectTile *effect);
Tile *GameGridTile(int x, int y);

// External Tile Interface
int TilePixel(u8 *tileSetData, int tileNum, u32 x, u32 y);

// External Game Interface
extern int GameStart(const char *command);
extern int GamePause(const char *command);

// External Shell Player Interface
extern int Action(const char *command);
extern int North(const char *command);
extern int South(const char *command);
extern int East(const char *command);
extern int West(const char *command);
extern int NorthEast(const char *command);
extern int NorthWest(const char *command);
extern int SouthEast(const char *command);
extern int SouthWest(const char *command);

#endif /* _GAME_GRID_H */
