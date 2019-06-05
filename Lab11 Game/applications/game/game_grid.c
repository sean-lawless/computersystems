/*...................................................................*/
/*                                                                   */
/*   Module:  game_grid.c                                            */
/*   Version: 2019.0                                                 */
/*   Purpose: 2D game graid engine logic                             */
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
#include "denzi_data.h"
#include "game_grid.h"

#if ENABLE_GAME

/*...................................................................*/
/* TileDisplay: Display a game grid tile (background, item, sprite)  */
/*                                                                   */
/*   Input: background is pointer to game grid background tile       */
/*          locationX indicate the X (width) game grid position      */
/*          locationY indicate the Y (height) game grid position     */
/*                                                                   */
/*  Return: NULL (0) on success, pointer to sprite or -1 on failure  */
/*...................................................................*/
void TileDisplay(BackgroundTile *background, int locationX,
                 int locationY)
{
  // Clear the tile
  TileClear(GAME_GRID_START_X + (locationX * TILE_WIDTH),
            GAME_GRID_START_Y + (locationY * TILE_HEIGHT));

  // Convert to pixels and display background again
  BackgroundTileDisplay(GAME_GRID_START_X +(locationX * TILE_WIDTH),
                        GAME_GRID_START_Y + (locationY * TILE_HEIGHT),
                      background->tile.color, background->tile.tileNum);

  // Mark background tile as visible
  background->tile.flags |= IS_VISIBLE;

  // Display an item if on this terrain
  if (background->item)
  {
    ItemTileDisplay(GAME_GRID_START_X +
        (locationX * TILE_WIDTH), GAME_GRID_START_Y +
        (locationY * TILE_HEIGHT), background->item->tile.color,
        background->item->tile.tileNum);

    // Mark the item tile as visible
    background->item->tile.flags |= IS_VISIBLE;
  }

  // Display any sprite on the tile
  if (background->sprite)
  {
    // Always display the player sprite 
    if (background->sprite->stats.flags & IS_PLAYER)
    {
      PlayerTileDisplay(GAME_GRID_START_X +(locationX * TILE_WIDTH),
                        GAME_GRID_START_Y + (locationY * TILE_HEIGHT),
                        background->sprite->tile.color,
                        background->sprite->tile.tileNum);

      // Mark the sprite tile as visible
      background->sprite->tile.flags |= IS_VISIBLE;
    }

    // Otherwise display the sprite if visisble
    else if (background->sprite->tile.flags & IS_VISIBLE)
    {
      SpriteTileDisplay(GAME_GRID_START_X +(locationX * TILE_WIDTH),
                        GAME_GRID_START_Y + (locationY * TILE_HEIGHT),
                        background->sprite->tile.color,
                        background->sprite->tile.tileNum);
    }
  }
}

/*...................................................................*/
/* SpriteMove: Move sprite to a new background tile                  */
/*                                                                   */
/*   Input: creatureTile is pointer to creature sprite to move       */
/*          tileX indicate the new X tile position                   */
/*          tileY indicate the new Y tile position                   */
/*                                                                   */
/*  Return: NULL (0) on success, pointer to sprite or -1 on failure  */
/*...................................................................*/
SpriteTile *SpriteMove(SpriteTile *spriteTile, int tileX, int tileY)
{
  BackgroundTile *theTerrain;

  // Return success if this sprite is already on this tile
  if ((tileX == spriteTile->locationX) &&
      (tileY == spriteTile->locationY))
    return NULL;

  // Ensure tile is not out of range of game grid
  if ((tileX >= spriteTile->currentWorld->x) ||
      (tileY >= spriteTile->currentWorld->y) ||
      (tileX < 0) || (tileY < 0))
  {
    puts("Move creature out of range");
    return (void *)-1;
  }

  // Return sprite if present on the new game grid
  theTerrain = spriteTile->currentWorld->tiles;
  if (theTerrain[tileX + tileY * GAME_GRID_WIDTH].sprite)
    return theTerrain[tileX + tileY * GAME_GRID_WIDTH].sprite;
  
  // Find the current background tile of the sprite
  theTerrain = &theTerrain[spriteTile->locationX +
                           spriteTile->locationY * GAME_GRID_WIDTH];
  if (theTerrain->sprite != spriteTile)
    puts("Terrain creature mismatch");          

  // Clear the sprite from this tile
  theTerrain->sprite = NULL;

  // Redraw the tile if visiable
  if (theTerrain->tile.flags & IS_VISIBLE)
    TileDisplay(theTerrain, spriteTile->locationX,
                          spriteTile->locationY);

  // Update this sprite with its new location
  spriteTile->locationX = tileX;
  spriteTile->locationY = tileY;

  // Find the new background tile of the sprite
  theTerrain = spriteTile->currentWorld->tiles;
  theTerrain = &theTerrain[tileX + tileY * GAME_GRID_WIDTH];

  // Let background tile know a sprite is on it
  theTerrain->sprite = spriteTile;

  // Redraw the tile if visiable
  if (theTerrain->tile.flags & IS_VISIBLE)
    TileDisplay(theTerrain, tileX, tileY);

  return NULL;
}

#endif /* ENABLE_GAME */
