/*...................................................................*/
/*                                                                   */
/*   Module:  game_grid.c                                            */
/*   Version: 2019.0                                                 */
/*   Purpose: 2D game grid engine logic                              */
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
#include <system.h>
#include <stdlib.h>
#include <stdio.h>
#include <board.h>
#include "game_grid.h"

#if ENABLE_GAME

Tile GameGrid[GAME_GRID_WIDTH * GAME_GRID_HEIGHT];
World TheWorld;

/*...................................................................*/
/*  TileClear: Set all pixels to zero (black) for a tile location    */
/*                                                                   */
/*      Input: x the starting x (width) pixel position               */
/*             y the starting y (height) pixel position              */
/*...................................................................*/
void TileClear(u32 x, u32 y)
{
  int i, j;

  // Clear the pixels (set to zero or black)
  for (i = 0; i < TILE_HEIGHT; ++i)
    for (j = 0; j < TILE_WIDTH; ++j)
      SetPixel(x + j, y + i, 0);
}

/*...................................................................*/
/*  TileDisplayScreen: Display a Tile at a pixel position            */
/*                                                                   */
/*      Input: tile pointer to Tile structure                        */
/*             x the starting x (width) pixel position               */
/*             y the starting y (height) pixel position              */
/*...................................................................*/
void TileDisplayScreen(Tile *tile, u32 x, u32 y)
{
  u32 tileX, tileY;

  // Display the pixels based on image bit map
  for (tileY = 0; tileY < TILE_HEIGHT; tileY++)
  {
    for (tileX = 0; tileX < TILE_WIDTH; tileX++)
    {
      if ((tile->flags & SHOW_FLIPPED) && (tile->flags & SHOW_REVERSE))
      {
        if (TilePixel(tile->bitmap, tile->tileNum, (TILE_WIDTH - 1) - tileX,
                      (TILE_HEIGHT - 1) - tileY))
          SetPixel(x + tileX, y + tileY, tile->color);
      }
      else if (tile->flags & SHOW_FLIPPED)
      {
        if (TilePixel(tile->bitmap, tile->tileNum,
                      tileX, (TILE_HEIGHT - 1) - tileY))
          SetPixel(x + tileX, y + tileY, tile->color);
      }
      else if (tile->flags & SHOW_REVERSE)
      {
        if (TilePixel(tile->bitmap, tile->tileNum, (TILE_WIDTH - 1) - tileX,
                      tileY))
          SetPixel(x + tileX, y + tileY, tile->color);
      }
      else
      {
        if (TilePixel(tile->bitmap, tile->tileNum, tileX, tileY))
          SetPixel(x + tileX, y + tileY, tile->color);
      }
    }
  }
}

/*...................................................................*/
/*  TileDisplayGrid: Display a Tile at a grid position               */
/*                                                                   */
/*      Input: locationX the x (width) grid position                 */
/*             locationY the y (height) grid position                */
/*...................................................................*/
void TileDisplayGrid(int locationX, int locationY)
{
  Tile *stackedTile, *tile = GameGridTile(locationX, locationY);

  // Return if invalid game grid location or not visible
  if ((tile == NULL) || !(tile->flags & IS_VISIBLE))
    return;

#if CIRCULAR_LIST
  {
    u32 tileX, tileY;
    int x = GAME_GRID_START_X + (locationX * TILE_WIDTH);
    int y = GAME_GRID_START_Y + (locationY * TILE_HEIGHT);

    // Display the pixels based on top down rendering of stacked tiles
    for (tileY = 0; tileY < TILE_HEIGHT; tileY++)
    {
      for (tileX = 0; tileX < TILE_WIDTH; tileX++)
      {
        for (stackedTile = (void *)tile->list.previous;
             stackedTile;
             stackedTile = (void *)stackedTile->list.previous)
        {
          if (!(stackedTile->flags & IS_VISIBLE))
            continue;

          if ((stackedTile->flags & SHOW_FLIPPED) &&
              (stackedTile->flags & SHOW_REVERSE))
          {
            if (TilePixel(stackedTile->bitmap, stackedTile->tileNum,
                          (TILE_WIDTH - 1) - tileX,
                          (TILE_HEIGHT - 1) - tileY))
            {
              SetPixel(x + tileX, y + tileY, stackedTile->color);
              break;
            }
          }
          else if (stackedTile->flags & SHOW_FLIPPED)
          {
            if (TilePixel(stackedTile->bitmap, stackedTile->tileNum,
                          tileX, (TILE_HEIGHT - 1) - tileY))
            {
              SetPixel(x + tileX, y + tileY, stackedTile->color);
              break;
            }
          }
          else if (stackedTile->flags & SHOW_REVERSE)
          {
            if (TilePixel(stackedTile->bitmap, stackedTile->tileNum,
                          (TILE_WIDTH - 1) - tileX, tileY))
            {
              SetPixel(x + tileX, y + tileY, stackedTile->color);
              break;
            }
          }
          else
          {
            if (TilePixel(stackedTile->bitmap, stackedTile->tileNum,
                          tileX, tileY))
            {
              SetPixel(x + tileX, y + tileY, stackedTile->color);
              break;
            }
          }

          // If we have circled back to start of the list then break out
          if (stackedTile == tile)
          {
            SetPixel(x + tileX, y + tileY, COLOR_BLACK);
            break;
          }
        }
      }
    }
  }
#else /* CIRCULAR_LIST */

  // Clear the tile
  TileClear(GAME_GRID_START_X + (locationX * TILE_WIDTH),
            GAME_GRID_START_Y + (locationY * TILE_HEIGHT));

  // Convert to pixels and display tile
  TileDisplayScreen(tile, GAME_GRID_START_X +(locationX * TILE_WIDTH),
                    GAME_GRID_START_Y + (locationY * TILE_HEIGHT));

  // Display any stacked tiles in a NULL ending or circular list
  for (stackedTile = (void *)tile->list.next;
       stackedTile && (stackedTile != tile);
       stackedTile = (void *)stackedTile->list.next)
  {
    if (stackedTile->flags & IS_VISIBLE)
      TileDisplayScreen(stackedTile, GAME_GRID_START_X +
                        (locationX * TILE_WIDTH), GAME_GRID_START_Y +
                        (locationY * TILE_HEIGHT));
  }
#endif
}

/*...................................................................*/
/* GameGridTile: Look up a base Tile from game grid coordinates      */
/*                                                                   */
/*   Input: x is the horizontal coordinate                           */
/*          y is the vertical coordinate                             */
/*                                                                   */
/*  Return: TASK_FINISHED                                            */
/*...................................................................*/
Tile *GameGridTile(int x, int y)
{
  Tile *theTerrain;

  // Ensure tile is not out of range of game grid
  if ((x >= TheWorld.x) ||
      (y >= TheWorld.y) ||
      (x < 0) || (y < 0))
  {
    puts("Game grid out of range");
    return NULL;
  }

  // Find the tile within the array of background tiles
  theTerrain = TheWorld.tiles;
  theTerrain = &theTerrain[x + y * GAME_GRID_WIDTH];

  // Return only a pointer to the tile portion of the background
  return (Tile *)theTerrain;
}

#endif /* ENABLE_GAME */
