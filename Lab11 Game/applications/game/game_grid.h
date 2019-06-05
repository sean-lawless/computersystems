/*.....................................................................*/
/*                                                                     */
/*   Module:  game_grid.h                                              */
/*   Version: 2019.0                                                   */
/*   Purpose: virtual world game grid structures                       */
/*                                                                     */
/*.....................................................................*/
/*                                                                     */
/*                   Copyright 2019, Sean Lawless                      */
/*                                                                     */
/*                      ALL RIGHTS RESERVED                            */
/*                                                                     */
/* Redistribution and use in source and binary forms, with or without  */
/* modification, are permitted provided that the following conditions  */
/* are met:                                                            */
/*                                                                     */
/*  1. Redistributions in any form, including but not limited to       */
/*     source code, binary, or derived works, must include the above   */
/*     copyright notice, this list of conditions and the following     */
/*     disclaimer.                                                     */
/*                                                                     */
/*  2. Any change to this copyright notice requires the prior written  */
/*     permission of the above copyright holder.                       */
/*                                                                     */
/* THIS SOFTWARE IS PROVIDED ''AS IS''. ANY EXPRESS OR IMPLIED         */
/* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES   */
/* OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE         */
/* DISCLAIMED. IN NO EVENT SHALL ANY AUTHOR AND/OR COPYRIGHT HOLDER BE */
/* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR */
/* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT   */
/* OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR  */
/* BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF          */
/* LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT           */
/* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE   */
/* USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH    */
/* DAMAGE.                                                             */
/*.....................................................................*/
#ifndef _GAME_GRID_H
#define _GAME_GRID_H

#include <system.h>

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
#define WORLD_EXIT    (1 << 24)
#define IS_VISIBLE    (1 << 25)
#define IS_PLAYER     (1 << 26)

typedef struct
{
  int tileNum; // Index into bitmap
  u32 color;   // Color of binary bitmap
  u32 flags;
} Tile;

typedef struct
{
  int x, y; // size of the rectangular world
  void *tiles; // array of background tiles representing game grid
} World;

typedef struct
{
  const char *name;
  int affect; // Type or details of action
  int range;  // Distance, in tiles, action can be performed
} ActionType;

typedef struct {
  ActionType first; // Primary action (button one)
  ActionType second;// Secondary action (button two)
  u32 flags;        // Flag bits for behavior, state, etc.
  int hp;           // Health Points
  u8 str, dex, con, intel, wis; // Optional abilities
                                // Can be used with PRNG to determine
                                // if actions are successful
  u8 ac, scratch, pad; // Optional Armour Class and game specific
} SpriteStats;

typedef struct
{
  Tile tile;
  SpriteStats stats;
  int locationX, locationY; // in current world grids
  World *currentWorld;
  char *name;
} SpriteTile;

typedef struct
{
  Tile tile;
} ItemTile;

typedef struct
{
  SpriteTile player;
  int score; // Current player score if any
  int level; // Current game level, if any
  int num_items;
  ItemTile *items;
} PlayerTile;

typedef struct
{
  Tile tile;

  // Sprite occupying this background tile or NULL
  SpriteTile *sprite;

  // Item occupying this background tile or NULL
  ItemTile *item;

  // link to new world if applicable
  void *newWorld;
} BackgroundTile;

// Tile and sprite interface
void TileDisplay(BackgroundTile *terrain, int x, int y);
SpriteTile *SpriteMove(SpriteTile *spriteTile, int tileX,
                       int tileY);

// External Shell Game Interface
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
