/*...................................................................*/
/*                                                                   */
/*   Module:  virtual_world.c                                        */
/*   Version: 2019.0                                                 */
/*   Purpose: virtual world game logic                               */
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
#include <string.h>
#include <board.h>
#include "game_grid.h"
#include "denzi_data.h"

#if ENABLE_GAME

#define CHARACTER_AUTO_ATTACK FALSE
#define MAX_CREATURES         GAME_GRID_WIDTH
#define MAX_ITEMS             16

// Random world flags
#define WORLD_WEST_COAST      (1 << 0)
#define WORLD_EAST_COAST      (1 << 1)
#define WORLD_NORTH_COAST     (1 << 2)
#define WORLD_SOUTH_COAST     (1 << 3)
#define WORLD_MOUNTAINOUS     (1 << 4)

// Creature flags
#define AGGRESSIVE            (1 << 0)
#define AMBUSH                (1 << 1)
#define CAUTIOUS              (1 << 2)
#define VISION                (1 << 3)
#define TRACKING              (1 << 4)
#define FLYING                (1 << 5)
#define IS_DEAD               (1 << 7)

// Character flags
// Race
#define RACE_HUMAN            (1 << 8)
#define RACE_ELF              (1 << 9)
#define RACE_DWARF            (1 << 10)
#define RACE_HOBIT            (1 << 11)
// Class
#define CLASS_RANGER          (1 << 16)
#define CLASS_SCOUT           (1 << 17)
#define CLASS_WIZARD          (1 << 18)
#define CLASS_CLERIC          (1 << 19)


int GamePeriod;

BackgroundTile MiddleEarth[GAME_GRID_WIDTH * GAME_GRID_HEIGHT];
PlayerTile TheCharacter;
SpriteTile TheCreatures[MAX_CREATURES];
World TheWorld;

/*...................................................................*/
/* player_display: display the players current statistics            */
/*                                                                   */
/*   Input: character is pointer to the player                       */
/*...................................................................*/
static void player_display(PlayerTile *character)
{
  int i, len;
  int high, low;

    len = strlen(character->player.name);
    for (i = 0; i < len; ++i)
      DisplayCursorChar(character->player.name[i], i * 8, 10, COLOR_WHITE);

    if (character->player.stats.flags & RACE_HUMAN)
    {
      DisplayCursorChar('H', 0, 30, COLOR_WHITE);
      DisplayCursorChar('u', 8, 30, COLOR_WHITE);
      DisplayCursorChar('m', 16, 30, COLOR_WHITE);
      DisplayCursorChar('a', 24, 30, COLOR_WHITE);
      DisplayCursorChar('n', 32, 30, COLOR_WHITE);
      DisplayCursorChar(' ', 40, 30, COLOR_WHITE);
    }
    else if (character->player.stats.flags & RACE_ELF)
    {
      DisplayCursorChar(' ', 0, 30, COLOR_WHITE);
      DisplayCursorChar('E', 8, 30, COLOR_WHITE);
      DisplayCursorChar('l', 16, 30, COLOR_WHITE);
      DisplayCursorChar('f', 24, 30, COLOR_WHITE);
      DisplayCursorChar(' ', 32, 30, COLOR_WHITE);
      DisplayCursorChar(' ', 40, 30, COLOR_WHITE);
    }
    else if (character->player.stats.flags & RACE_DWARF)
    {
      DisplayCursorChar('D', 0, 30, COLOR_WHITE);
      DisplayCursorChar('w', 8, 30, COLOR_WHITE);
      DisplayCursorChar('a', 16, 30, COLOR_WHITE);
      DisplayCursorChar('r', 24, 30, COLOR_WHITE);
      DisplayCursorChar('f', 32, 30, COLOR_WHITE);
      DisplayCursorChar(' ', 40, 30, COLOR_WHITE);
    }
    else if (character->player.stats.flags & RACE_HOBIT)
    {
      DisplayCursorChar('H', 0, 30, COLOR_WHITE);
      DisplayCursorChar('o', 8, 30, COLOR_WHITE);
      DisplayCursorChar('b', 16, 30, COLOR_WHITE);
      DisplayCursorChar('i', 24, 30, COLOR_WHITE);
      DisplayCursorChar('t', 32, 30, COLOR_WHITE);
      DisplayCursorChar(' ', 40, 30, COLOR_WHITE);
    }

    if (character->player.stats.flags & CLASS_RANGER)
    {
      DisplayCursorChar('R', 48, 30, COLOR_WHITE);
      DisplayCursorChar('a', 56, 30, COLOR_WHITE);
      DisplayCursorChar('n', 64, 30, COLOR_WHITE);
      DisplayCursorChar('g', 72, 30, COLOR_WHITE);
      DisplayCursorChar('e', 80, 30, COLOR_WHITE);
      DisplayCursorChar('r', 88, 30, COLOR_WHITE);
    }
    else if (character->player.stats.flags & CLASS_SCOUT)
    {
      DisplayCursorChar('S', 48, 30, COLOR_WHITE);
      DisplayCursorChar('c', 56, 30, COLOR_WHITE);
      DisplayCursorChar('o', 64, 30, COLOR_WHITE);
      DisplayCursorChar('u', 72, 30, COLOR_WHITE);
      DisplayCursorChar('t', 80, 30, COLOR_WHITE);
    }
    else if (character->player.stats.flags & CLASS_WIZARD)
    {
      DisplayCursorChar('W', 48, 30, COLOR_WHITE);
      DisplayCursorChar('i', 56, 30, COLOR_WHITE);
      DisplayCursorChar('z', 64, 30, COLOR_WHITE);
      DisplayCursorChar('a', 72, 30, COLOR_WHITE);
      DisplayCursorChar('r', 80, 30, COLOR_WHITE);
      DisplayCursorChar('d', 88, 30, COLOR_WHITE);
    }
    else if (character->player.stats.flags & CLASS_CLERIC)
    {
      DisplayCursorChar('C', 48, 30, COLOR_WHITE);
      DisplayCursorChar('l', 56, 30, COLOR_WHITE);
      DisplayCursorChar('e', 64, 30, COLOR_WHITE);
      DisplayCursorChar('r', 72, 30, COLOR_WHITE);
      DisplayCursorChar('i', 80, 30, COLOR_WHITE);
      DisplayCursorChar('c', 88, 30, COLOR_WHITE);
    }

    // Strength
    DisplayCursorChar('S', 0, 50, COLOR_WHITE);
    DisplayCursorChar(':', 8, 50, COLOR_WHITE);

    high = (character->player.stats.str >> 4) & 0xF;
    low = character->player.stats.str & 0xF;
    if (high <= 9)
      DisplayCursorChar('0' + high, 16, 50, COLOR_WHITE);
    else
      DisplayCursorChar('A' + high - 10, 16, 50, COLOR_WHITE);
    if (low <= 9)
      DisplayCursorChar('0' + low, 24, 50, COLOR_WHITE);
    else
      DisplayCursorChar('A' + low - 10, 24, 50, COLOR_WHITE);

    // Dexterity
    DisplayCursorChar(' ', 32, 50, COLOR_WHITE);
    DisplayCursorChar('D', 40, 50, COLOR_WHITE);
    DisplayCursorChar(':', 48, 50, COLOR_WHITE);

    high = (character->player.stats.dex >> 4) & 0xF;
    low = character->player.stats.dex & 0xF;
    if (high <= 9)
      DisplayCursorChar('0' + high, 56, 50, COLOR_WHITE);
    else
      DisplayCursorChar('A' + high - 10, 56, 50, COLOR_WHITE);
    if (low <= 9)
      DisplayCursorChar('0' + low, 64, 50, COLOR_WHITE);
    else
      DisplayCursorChar('A' + low - 10, 64, 50, COLOR_WHITE);

    // Constitution
    DisplayCursorChar(' ', 72, 50, COLOR_WHITE);
    DisplayCursorChar('C', 80, 50, COLOR_WHITE);
    DisplayCursorChar(':', 88, 50, COLOR_WHITE);
    high = (character->player.stats.con >> 4) & 0xF;
    low = character->player.stats.con & 0xF;
    if (high <= 9)
      DisplayCursorChar('0' + high, 96, 50, COLOR_WHITE);
    else
      DisplayCursorChar('A' + high - 10, 96, 50, COLOR_WHITE);
    if (low <= 9)
      DisplayCursorChar('0' + low, 104, 50, COLOR_WHITE);
    else
      DisplayCursorChar('A' + low - 10, 104, 50, COLOR_WHITE);

    // Intellegence
    DisplayCursorChar(' ', 112, 50, COLOR_WHITE);
    DisplayCursorChar('I', 120, 50, COLOR_WHITE);
    DisplayCursorChar(':', 128, 50, COLOR_WHITE);
    high = (character->player.stats.intel >> 4) & 0xF;
    low = character->player.stats.intel & 0xF;
    if (high <= 9)
      DisplayCursorChar('0' + high, 136, 50, COLOR_WHITE);
    else
      DisplayCursorChar('A' + high - 10, 136, 50, COLOR_WHITE);
    if (low <= 9)
      DisplayCursorChar('0' + low, 144, 50, COLOR_WHITE);
    else
      DisplayCursorChar('A' + low - 10, 144, 50, COLOR_WHITE);

    // Wisdom
    DisplayCursorChar('W', 0, 70, COLOR_WHITE);
    DisplayCursorChar(':', 8, 70, COLOR_WHITE);
    high = (character->player.stats.wis >> 4) & 0xF;
    low = character->player.stats.wis & 0xF;
    if (high <= 9)
      DisplayCursorChar('0' + high, 16, 70, COLOR_WHITE);
    else
      DisplayCursorChar('A' + high - 10, 16, 70, COLOR_WHITE);
    if (low <= 9)
      DisplayCursorChar('0' + low, 24, 70, COLOR_WHITE);
    else
      DisplayCursorChar('A' + low - 10, 24, 70, COLOR_WHITE);

    // Armour Class
    DisplayCursorChar(' ', 32, 70, COLOR_WHITE);
    DisplayCursorChar(' ', 40, 70, COLOR_WHITE);
    DisplayCursorChar('A', 48, 70, COLOR_WHITE);
    DisplayCursorChar('C', 56, 70, COLOR_WHITE);
    DisplayCursorChar(':', 64, 70, COLOR_WHITE);
    DisplayCursorChar(' ', 72, 70, COLOR_WHITE);
    DisplayCursorChar(' ', 80, 70, COLOR_WHITE);
    high = (character->player.stats.ac >> 4) & 0xF;
    low = character->player.stats.ac & 0xF;
    if (high <= 9)
      DisplayCursorChar('0' + high, 72, 70, COLOR_WHITE);
    else
      DisplayCursorChar('A' + high - 10, 72, 70, COLOR_WHITE);
    if (low <= 9)
      DisplayCursorChar('0' + low, 80, 70, COLOR_WHITE);
    else
      DisplayCursorChar('A' + low - 10, 80, 70, COLOR_WHITE);

    // Hit Points
    DisplayCursorChar('H', 0, 90, COLOR_WHITE);
    DisplayCursorChar('P', 8, 90, COLOR_WHITE);
    DisplayCursorChar(':', 16, 90, COLOR_WHITE);
    DisplayCursorChar(' ', 24, 90, COLOR_WHITE);
    DisplayCursorChar(' ', 32, 90, COLOR_WHITE);
    high = (character->player.stats.hp >> 4) & 0xF;
    low = character->player.stats.hp & 0xF;
    if (high <= 9)
      DisplayCursorChar('0' + high, 24, 90, COLOR_WHITE);
    else
      DisplayCursorChar('A' + high - 10, 24, 90, COLOR_WHITE);
    if (low <= 9)
      DisplayCursorChar('0' + low, 32, 90, COLOR_WHITE);
    else
      DisplayCursorChar('A' + low - 10, 32, 90, COLOR_WHITE);

    // eXperience Points
    DisplayCursorChar(' ', 40, 90, COLOR_WHITE);
    DisplayCursorChar('X', 48, 90, COLOR_WHITE);
    DisplayCursorChar('P', 56, 90, COLOR_WHITE);
    DisplayCursorChar(':', 64, 90, COLOR_WHITE);
    DisplayCursorChar(' ', 72, 90, COLOR_WHITE);
    DisplayCursorChar(' ', 80, 90, COLOR_WHITE);
    high = (character->score >> 4) & 0xF;
    low = character->score & 0xF;
    if (high <= 9)
      DisplayCursorChar('0' + high, 72, 90, COLOR_WHITE);
    else
      DisplayCursorChar('A' + high - 10, 72, 90, COLOR_WHITE);
    if (low <= 9)
      DisplayCursorChar('0' + low, 80, 90, COLOR_WHITE);
    else
      DisplayCursorChar('A' + low - 10, 80, 90, COLOR_WHITE);

    // Level
    DisplayCursorChar(' ', 88, 90, COLOR_WHITE);
    DisplayCursorChar('L', 96, 90, COLOR_WHITE);
    DisplayCursorChar('V', 104, 90, COLOR_WHITE);
    DisplayCursorChar(':', 112, 90, COLOR_WHITE);
    DisplayCursorChar(' ', 120, 90, COLOR_WHITE);
    DisplayCursorChar(' ', 128, 90, COLOR_WHITE);
    high = ((character->level) >> 4) & 0xF;
    low = character->level & 0xF;
    if (high <= 9)
      DisplayCursorChar('0' + high, 120, 90, COLOR_WHITE);
    else
      DisplayCursorChar('A' + high - 10, 120, 90, COLOR_WHITE);
    if (low <= 9)
      DisplayCursorChar('0' + low, 128, 90, COLOR_WHITE);
    else
      DisplayCursorChar('A' + low - 10, 128, 90, COLOR_WHITE);

    // Display Equipment
    for (i = 0; i < character->num_items; ++i)
    {
      if (i < 8)
        ItemTileDisplay(i * 16, 110,character->items[i].tile.color,
                        character->items[i].tile.tileNum);
      else
        ItemTileDisplay((i - 8) * 16, 130,
                        character->items[i].tile.color,
                        character->items[i].tile.tileNum);
    }

    // Hardcode a seperator
    for (i = 0; i < 21; ++i)
      DisplayCursorChar('-', i * 8, 170, COLOR_WHITE);
    DisplayCursorChar('|', 168, 170, COLOR_WHITE);
}

/*...................................................................*/
/* world_create_random: Create a game grid of random terrain tiles   */
/*                                                                   */
/*   Input: world is pointer to the game grid                        */
/*          flags indicate world creation preferences                */
/*                                                                   */
/*  Return: Zero (0)                                                 */
/*...................................................................*/
static int world_create_random(World *world, u32 flags)
{
  BackgroundTile *middleEarth = world->tiles;

  int x, y;
  int aboveTile = 0, aboveTileRight = 0, aboveTileLeft = 0;

  // Generate the virtual world
  for (y = 0; y < GAME_GRID_HEIGHT; ++y)
  {
    for (x = 0; x < GAME_GRID_WIDTH; ++x)
    {
      u32 random = rand();
      u32 color = COLOR_GREEN;

      if ((flags & WORLD_NORTH_COAST) && (y == 0))
      {
          random = TERRAIN_WATER;
          color = COLOR_DEEPSKYBLUE;
      }
      else if ((flags & WORLD_SOUTH_COAST) && (y >= GAME_GRID_HEIGHT - 2))
      {
          random = TERRAIN_WATER;
          color = COLOR_DEEPSKYBLUE;
      }
      else if ((flags & WORLD_WEST_COAST) && (x == 0))
      {
          random = TERRAIN_WATER;
          color = COLOR_DEEPSKYBLUE;
      }
      else if ((flags & WORLD_EAST_COAST) && (x >= GAME_GRID_WIDTH - 2))
      {
          random = TERRAIN_WATER;
          color = COLOR_DEEPSKYBLUE;
      }
      else
      {
        random = (((random & 0xF) ^ ((random & 0xF0) >> 8)) & 0xF) + 90;
        if (random > 94)
        {
          if (random == 95)
          {
            random = TERRAIN_CRAGS;
            color = COLOR_SIENNA;
          }
          else if (random == 96)
          {
            random = TERRAIN_HILLS;
            color = COLOR_GOLDEN;
          }
          else if (random == 97)
          {
            random = TERRAIN_GRASSLANDS;
            color = COLOR_GOLDEN;
          }
          else if (random == 98)
          {
            random = TERRAIN_WATER;
            color = COLOR_DEEPSKYBLUE;
          }
          else if (random == 99)
          {
            random = TERRAIN_WATER;
            color = COLOR_DEEPSKYBLUE;
          }
          else if (random == 100)
          {
            // Make towns very rare
            if ((rand() & 0xF) == 0xF)
            {
              random = STRUCTURE_TOWN;
              color = COLOR_ORANGERED;
            }
            else
            {
              random = TERRAIN_WATER;
              color = COLOR_DEEPSKYBLUE;
            }
          }
          else if (random >= 101)
          {
            random -= 11;
          }
        }

        if (y > 0)
        {
          aboveTile = middleEarth[x + (y - 1) * GAME_GRID_WIDTH].tile.tileNum;
          if (x > 0)
            aboveTileLeft = middleEarth[(x - 1) + (y - 1) * GAME_GRID_WIDTH].tile.tileNum;
          else
            aboveTileLeft = 0;

          if (x < GAME_GRID_WIDTH - 1)
            aboveTileRight = middleEarth[(x + 1) + (y - 1) * GAME_GRID_WIDTH].tile.tileNum;
          else
            aboveTileRight = 0;

          if (random == TERRAIN_GRASSLANDS)
          {
            color = COLOR_GOLDEN;
          }

          else if ((random != TERRAIN_WATER) &&
                 (((flags & (WORLD_WEST_COAST | WORLD_NORTH_COAST)) &&
                  (aboveTile == TERRAIN_WATER) &&
                     (aboveTileLeft == TERRAIN_WATER)) ||
                 ((flags & (WORLD_EAST_COAST | WORLD_SOUTH_COAST)) &&
                  (aboveTile == TERRAIN_WATER) &&
                     (aboveTileRight == TERRAIN_WATER))))
          {
              random = TERRAIN_WATER;
              color = COLOR_DEEPSKYBLUE;
          }
          else if ((random != TERRAIN_MOUNTAINS) &&
                   (flags & WORLD_MOUNTAINOUS) &&
                   ((aboveTile == TERRAIN_MOUNTAINS) &&
                    (aboveTileLeft == TERRAIN_MOUNTAINS)))
          {
            random = TERRAIN_MOUNTAINS;
            color = COLOR_WHITE;
          }
        }
      }

      // Change color to white if mountains
      if (random == TERRAIN_MOUNTAINS)
        color = COLOR_WHITE;

      // Assign the new terrain tile to the world
      middleEarth[x + y * GAME_GRID_WIDTH].tile.color = color;
      middleEarth[x + y * GAME_GRID_WIDTH].tile.tileNum = random;
      middleEarth[x + y * GAME_GRID_WIDTH].tile.flags = 0;
      middleEarth[x + y * GAME_GRID_WIDTH].item = NULL;
      middleEarth[x + y * GAME_GRID_WIDTH].sprite = NULL;
    }
  }

  return 0;
}

/*...................................................................*/
/* player_look: Have player examine world, ie current game grid      */
/*                                                                   */
/*   Input: character is pointer to the player                       */
/*...................................................................*/
static void player_look(PlayerTile *character)
{
  World *world = character->player.currentWorld;
  BackgroundTile *tiles = world->tiles;
  int x = character->player.locationX,
      y = character->player.locationY;
  const char *terrain;
  int distance, startX, startY, endX, endY, tileNum;

  tileNum = tiles[x + y * GAME_GRID_WIDTH].tile.tileNum;

  // Output the name of the current character occupied terrain
  if (tileNum == TERRAIN_MOUNTAINS)
    terrain = "Mountains";
  else if (tileNum == TERRAIN_OAK_FOREST)
    terrain = "Oak forest";
  else if (tileNum == TERRAIN_OAK_PRAIRIE)
    terrain = "Oak prairie";
  else if (tileNum == TERRAIN_PINE_FOREST)
    terrain = "Pine forest";
  else if (tileNum == TERRAIN_PINE_PRAIRIE)
    terrain = "Pine prarie";
  else if (tileNum == TERRAIN_CRAGS)
    terrain = "Crags";
  else if (tileNum == TERRAIN_HILLS)
    terrain = "Rolling hills";
  else if (tileNum == TERRAIN_GRASSLANDS)
    terrain = "Grassland";
  else if (tileNum == TERRAIN_WATER)
    terrain = "Water";
  else if (tileNum == STRUCTURE_TOWN)
    terrain = "Town";
  else
    terrain = "Unknown";
  puts(terrain);

  //
  if (character->player.stats.flags & TRACKING)
    distance = 5;
  else if (character->player.stats.flags & VISION)
    distance = 3;
  else
    distance = 1;

  startX = x - distance;
  if (startX < 0)
    startX = 0;
  endX = x + distance;
  if (endX >= GAME_GRID_WIDTH)
    endX = GAME_GRID_WIDTH - 1;

  startY = y - distance;
  if (startY < 0)
    startY = 0;
  endY = y + distance;
  if (endY >= GAME_GRID_HEIGHT)
    endY = GAME_GRID_HEIGHT - 1;

  // Display all tiles around the character if not already
  for (y = startY; y <= endY; ++y)
  {
    for (x = startX; x <= endX; ++x)
    {
      BackgroundTile *theTerrain = character->player.currentWorld->tiles;

      theTerrain = &theTerrain[x + y * GAME_GRID_WIDTH];

      // If not already visible, show tile
      if (!(theTerrain->tile.flags & IS_VISIBLE))
        TileDisplay(theTerrain, x, y);

      // If world exit tile, display gate
      else if (theTerrain->tile.flags & WORLD_EXIT)
        BackgroundTileDisplay(GAME_GRID_START_X + (x * 16),
                              GAME_GRID_START_Y + (y * 16),
                              COLOR_MAGENTA, STRUCTURE_DOOR_OPEN);
    }
  }
}

static ActionType *attack_range(SpriteTile *creatureTile,
                                SpriteTile *nearbyCreatureTile)
{
  int deltaX, deltaY;

  if (creatureTile->locationX > nearbyCreatureTile->locationX)
    deltaX = creatureTile->locationX - nearbyCreatureTile->locationX;
  else
    deltaX = nearbyCreatureTile->locationX - creatureTile->locationX;

  if (creatureTile->locationY > nearbyCreatureTile->locationY)
    deltaY = creatureTile->locationY - nearbyCreatureTile->locationY;
  else
    deltaY = nearbyCreatureTile->locationY - creatureTile->locationY;

  // If more that one space away then check ranged attacks
  if ((deltaX > 1) || (deltaY > 1))
  {
    // If a ranged weapon is present
    if (creatureTile->stats.second.affect > 0)
    {
      if (!(creatureTile->stats.flags & FLYING))
      {
        // Ensure player is not trying to attack from the water
        BackgroundTile *theTerrain = creatureTile->currentWorld->tiles;

        theTerrain = &(theTerrain[creatureTile->locationX +
                         (creatureTile->locationY * GAME_GRID_WIDTH)]);

        if (theTerrain->tile.tileNum == TERRAIN_WATER)
        {
          if (creatureTile->stats.flags & IS_PLAYER)
            puts("Cannot fire while Swimming");
          return NULL;
        }

        // Ensure player is not trying to attack into the mountains
        theTerrain = creatureTile->currentWorld->tiles;
        theTerrain = &(theTerrain[nearbyCreatureTile->locationX +
                    (nearbyCreatureTile->locationY * GAME_GRID_WIDTH)]);
        if (theTerrain->tile.tileNum == TERRAIN_MOUNTAINS)
        {
          if (creatureTile == &TheCharacter.player)
            puts("Mountains hide opponent");
          return NULL;
        }
      }

      // Return no attack if out of range
      if ((deltaX > creatureTile->stats.second.range) ||
          (deltaY > creatureTile->stats.second.range))
        return NULL;

      // Otherwise return ranged attack
      return &creatureTile->stats.second;
    }
    return NULL;
  }

  // Otherwise prefer melee (primary) attack
  else if (creatureTile->stats.first.affect > 0)
  {
    // If this is the character ensure this is a valid attack
    if (creatureTile == &TheCharacter.player)
    {
      // Check to ensure player is not trying to attack from the water
      BackgroundTile *theTerrain = creatureTile->currentWorld->tiles;
      PlayerTile *characterTile = &TheCharacter;

      theTerrain = &(theTerrain[creatureTile->locationX +
              (creatureTile->locationY * GAME_GRID_WIDTH)]);

      if (theTerrain->tile.tileNum == TERRAIN_WATER)
      {
        puts("Cannot attack while Swimming");
        return NULL;
      }

      // Ensure player is not trying to attack into the mountains
      theTerrain = creatureTile->currentWorld->tiles;
      theTerrain = &(theTerrain[nearbyCreatureTile->locationX +
                (nearbyCreatureTile->locationY * GAME_GRID_WIDTH)]);
      if (theTerrain->tile.tileNum == TERRAIN_MOUNTAINS)
      {
        int i;

        for (i = 0; i < characterTile->num_items; ++i)
          if (characterTile->items[i].tile.tileNum == ITEM_MAGIC_BOOTS)
            break;

        if (!i || (i == characterTile->num_items))
        {
          if (creatureTile->stats.second.affect)
            return &creatureTile->stats.second;
          else
          {
            puts("Mountains out of reach");
            return NULL;
          }
        }
      }
    }
    return &creatureTile->stats.first;
  }
  else
    return NULL;
}

// Global functions

/*...................................................................*/
/* sprite_move: Move sprite to a new background tile                 */
/*                                                                   */
/*   Input: creatureTile is pointer to creature sprite to move       */
/*          tileX indicate the new X tile position                   */
/*          tileY indicate the new Y tile position                   */
/*                                                                   */
/*  Return: Zero (0) on success, -1 if error                         */
/*...................................................................*/
static int sprite_move(SpriteTile *creatureTile, int tileX, int tileY)
{
  creatureTile->tile.flags |= IS_VISIBLE;

  // Flying creatures can go anywhere, so only check non-flyers
  if (!(creatureTile->stats.flags & FLYING))
  {
    BackgroundTile *theTerrain = creatureTile->currentWorld->tiles;

    theTerrain = &theTerrain[tileX + tileY * GAME_GRID_WIDTH];

    // Land squids can't go in the mountains
    if (theTerrain->tile.tileNum == TERRAIN_MOUNTAINS)
    {
      if (creatureTile->tile.tileNum == CREATURE_LAND_SQUID)
        return -1;
    }

    // Giant bugs cannot swim
    if (theTerrain->tile.tileNum == TERRAIN_WATER)
    {
      if (creatureTile->tile.tileNum == CREATURE_GIANT_BUG)
        return -1;
    }

    // And sharks cannot leave the water
    else if (creatureTile->tile.tileNum == CREATURE_SHARK)
        return -1;

  }

  // Move the creature within the game grid and return if failure
  if (SpriteMove(creatureTile, tileX, tileY))
    return -1;

  return 0;
}

/*...................................................................*/
/* creature_attack: One creature attacks another                     */
/*                                                                   */
/*   Input: creatureTile is pointer to attacking creature/sprite     */
/*          attack the type of attack action                         */
/*          attackedCreatureTile is the creature being attacked      */
/*...................................................................*/
static void creature_attack(SpriteTile *creatureTile,
                            ActionType *attack,
                            SpriteTile *attackedCreatureTile)
{
  // Roll virtual dice (sample PRNG)
  int d20roll, modifier = 0;

  // If attacking creature not yet visible, display it now
  if (!(creatureTile->tile.flags & IS_VISIBLE))
  {
    BackgroundTile *theTerrain;

    creatureTile->tile.flags |= IS_VISIBLE;
    theTerrain = creatureTile->currentWorld->tiles;
    theTerrain = &theTerrain[creatureTile->locationX +
                        (creatureTile->locationY * GAME_GRID_WIDTH)];

    // Only display creature if player character has tile visibility
    if (theTerrain->tile.flags & IS_VISIBLE)
      SpriteTileDisplay(GAME_GRID_START_X +(creatureTile->locationX * 16),
                        GAME_GRID_START_Y +(creatureTile->locationY * 16),
                    creatureTile->tile.color, creatureTile->tile.tileNum);
  }

  // Simulate 20 side dice roll with 8 bits worth or randomness
  d20roll = (rand() % 31);
  if (d20roll == 0)
    d20roll = 20;

  // If overflow, trim the value
  if (d20roll > 20)
    d20roll -= d20roll - 20;

  // If an ambush, give advantage to the roll
  if (!(creatureTile->tile.flags & IS_VISIBLE))
  {
    int roll;

    roll = (rand() % 31);
    if (roll == 0)
      roll = 20;
    if (roll > 20)
      roll -= roll - 20;

    // Use second roll if higher
    if (roll > d20roll)
      d20roll = roll;
  }

  // Apply dexterity modifier if ranged attack
  if (attack->range > 1)
  {
    if (creatureTile->stats.dex > 10)
      modifier = (creatureTile->stats.dex - 10) >> 1;
  }

  // Otherwise apply strength modifier
  else if (creatureTile->stats.str > 10)
    modifier = (creatureTile->stats.str - 10) >> 1;

  // If character, apply proficiency modifier
  //   2 plus one every four levels
  if (creatureTile == &TheCharacter.player)
  {
    // Apply more class/race specific additions here
    if ((TheCharacter.player.stats.flags & CLASS_WIZARD) &&
        (creatureTile->stats.intel > 10))
      modifier = 2 + (TheCharacter.score >> 3) +
                 ((creatureTile->stats.intel - 10) >> 1);

    else if (TheCharacter.player.stats.flags & CLASS_RANGER)
      modifier += 2 + (TheCharacter.score >> 3);

    // Cleric has no base proficiency but experience
    else if (TheCharacter.player.stats.flags & CLASS_CLERIC)
      modifier += (TheCharacter.score >> 3);

    // Scout has proficiency and uses dexterity for proficiency
    else if (TheCharacter.player.stats.flags & CLASS_SCOUT)
    {
      if (creatureTile->stats.dex > 10)
        modifier = 2 + (TheCharacter.score >> 3) +
                 ((creatureTile->stats.dex - 10) >> 1);
      else
        modifier = 2 + (TheCharacter.score >> 3);
    }
  }

  // If character involved, display creature name at start of attack
  if ((creatureTile == &TheCharacter.player) ||
      (attackedCreatureTile == &TheCharacter.player))
    puts(creatureTile->name);

  // Compare the modified roll to the attackedCreatureTile armour class
  if (d20roll + modifier >= attackedCreatureTile->stats.ac)
  {
    int i, damage;

    // Loop for each d6 roll for the attack
    for (i = 0, damage = 0; i < attack->affect; ++i)
      damage += (rand() % 6) + 1;

    // If an ambush, double the damage
    if (!(creatureTile->tile.flags & IS_VISIBLE))
    {
      for (i = 0; i < attack->affect; ++i)
        damage += (rand() % 6) + 1;

      putbyte(damage);

      // Add the visible flag as it is one time use
      creatureTile->tile.flags |= IS_VISIBLE;
    }

    // Apply attack bonus for creatureTile to the virtual roll
    attackedCreatureTile->stats.hp -= damage + modifier;

    // If character involved, display damage from attack
    if ((creatureTile == &TheCharacter.player) ||
        (attackedCreatureTile == &TheCharacter.player))
    {
      putbyte(damage + modifier);
      puts(" damage!");
    }

    // Check if attacked creature dies
    if (attackedCreatureTile->stats.hp < 0)
    {
      int x = attackedCreatureTile->locationX,
          y = attackedCreatureTile->locationY;
      BackgroundTile *theTerrain =
                              attackedCreatureTile->currentWorld->tiles;

      // Clear the creature from the terrain, flag creature as dead
      theTerrain[x + (y * GAME_GRID_WIDTH)].sprite = NULL;
      attackedCreatureTile->tile.flags |= IS_DEAD;

      // Clear the tile if visible
      if (theTerrain[x + (y * GAME_GRID_WIDTH)].tile.flags & IS_VISIBLE)
      {
        TileClear(GAME_GRID_START_X + (x * 16),
                  GAME_GRID_START_Y + (y * 16));

        // Display the terrain again
        BackgroundTileDisplay(GAME_GRID_START_X + (x * 16), GAME_GRID_START_Y + (y * 16),
                              theTerrain[x + (y * GAME_GRID_WIDTH)].tile.color,
                              theTerrain[x + (y * GAME_GRID_WIDTH)].tile.tileNum);
      }

      // If the attacking creature is the character
      if (creatureTile->stats.flags & IS_PLAYER)
      {
        int d16roll;
        PlayerTile *characterTile = &TheCharacter;

        if (creatureTile != &TheCharacter.player)
          puts("ERROR: Creature is not player but has flag set!");

        // Increase character experience
        characterTile->score += 1;

        // Check if creature has treasure

        // Roll 16 sided dice
        d16roll = rand() % 16;

        // Check if an item is dropped (more than half)
        if (d16roll > 6)
        {
          BackgroundTile *theTerrain =
                                 attackedCreatureTile->currentWorld->tiles;

          puts("Treasure dropped!");
          theTerrain = &theTerrain[attackedCreatureTile->locationX +
                 attackedCreatureTile->locationY * GAME_GRID_WIDTH];

          // Drop a random item

          // Re-roll, equipment is 0 through 15
          d16roll = (rand() % 8);

          //Give out potions most often
          if (d16roll < 4)
            d16roll = 0;

          // Re-role and morph roll to this games items (denzi_data.h)
          else if ((d16roll = (((rand() % 8) +
                                  (characterTile->score >> 2)) % 16)) == 4)
            ++d16roll;
          else if (d16roll == 5)
            d16roll += 2;
          else
          {
            if (d16roll == 6)
              d16roll = ITEM_HELMET;
            else if (d16roll == 7)
              d16roll = ITEM_CHAINMAIL;
            else if (d16roll == 8)
              d16roll = ITEM_BATTLEAXE;
            else if (d16roll == 9)
              d16roll = ITEM_PLATEMAIL;
            else if (d16roll == 10)
              d16roll = ITEM_SPLINTMAIL;
            else if (d16roll == 11)
              d16roll = ITEM_MAGIC_BELT;
            else if (d16roll == 12)
              d16roll = ITEM_POWER_STAFF;
            else if (d16roll == 13)
              d16roll = ITEM_FLAMING_SWORD;
            else if (d16roll == 14)
              d16roll = ITEM_LIGHTNING_SWORD;
            else if (d16roll == 15)
              d16roll = ITEM_POWER_SWORD;
          }

          // Convert defeated creature into equipment item on terrain
          theTerrain->item = (void *)attackedCreatureTile;
          theTerrain->item->tile.color = COLOR_WHITE;
          theTerrain->item->tile.tileNum = d16roll;

          // Display the item/equipment on the screen
          ItemTileDisplay(GAME_GRID_START_X + (x * 16),
                          GAME_GRID_START_Y + (y * 16),
                          COLOR_WHITE, d16roll);
        }
        puts("Creature defeated!");

        // Update XP on display
        player_display(&TheCharacter);
      }
    }

    // If the attacked creature is the character, update HP
    if (attackedCreatureTile == &TheCharacter.player)
      player_display(&TheCharacter);
  }
  else
  {
    // If character involved, display attack missed
    if ((creatureTile == &TheCharacter.player) ||
        (attackedCreatureTile == &TheCharacter.player))
      puts("missed!");
  }
}

/*...................................................................*/
/* sprite_action: Perform an action for a sprite                     */
/*                                                                   */
/*   Input: creatureTile is pointer to creature sprite to act        */
/*          nearestCreatureTile the sprite nearest on game grid      */
/*                                                                   */
/*  Return: TRUE (1) if action performed, FALSE (0) if no action     */
/*...................................................................*/
static int sprite_action(SpriteTile *creatureTile,
                         SpriteTile *nearestCreatureTile)
{
  BackgroundTile *theTerrain;
  ActionType *attack = attack_range(creatureTile, nearestCreatureTile);
  int x, y;

  // Allow creatures to hide in towns
  theTerrain = creatureTile->currentWorld->tiles;
  if (theTerrain[nearestCreatureTile->locationX +
     (nearestCreatureTile->locationY * GAME_GRID_WIDTH)].tile.tileNum ==
                                                         STRUCTURE_TOWN)
    return FALSE;


    // Determine x offset toward distantCreature
    if (creatureTile->locationX < nearestCreatureTile->locationX)
      x = 1;
    else if (creatureTile->locationX == nearestCreatureTile->locationX)
      x = 0;
    else
      x = -1;

    // Determine y offset toward distantCreature
    if (creatureTile->locationY < nearestCreatureTile->locationY)
      y = 1;
    else if (creatureTile->locationY == nearestCreatureTile->locationY)
      y = 0;
    else
      y = -1;

    // If creature is aggressive, attack or pursue
    if (creatureTile->stats.flags & AGGRESSIVE)
    {
      if (attack)
        creature_attack(creatureTile, attack, nearestCreatureTile);
      else
        sprite_move(creatureTile, creatureTile->locationX + x,
                    creatureTile->locationY + y);
      return TRUE;
    }

    // Otherwise if cautious, attack if in range or move away
    else if (creatureTile->stats.flags & CAUTIOUS)
    {
      if (attack)
        creature_attack(creatureTile, attack, nearestCreatureTile);
      else
        sprite_move(creatureTile, creatureTile->locationX - x,
                    creatureTile->locationY - y);
      return TRUE;
    }

    // Otherwise act as undetected (ignore the nearby creature)
    return FALSE;
}

/*...................................................................*/
/* sprite_move_random: Move sprite one tile in random direction      */
/*                                                                   */
/*   Input: creatureTile is pointer to creature sprite to move       */
/*...................................................................*/
static void sprite_move_random(SpriteTile *creatureTile)
{
  int random, randX, randY;

  // If creature is out of bounds (or dead) do not move
  if ((creatureTile->locationX < 0) || (creatureTile->locationY < 0) ||
      (creatureTile->stats.flags & IS_DEAD) ||
      (creatureTile->stats.hp <= 0))
    return;

  // Generate randX and randY bits, zero or one
  random = rand();
  randX = ((random & 0x0F) >= 8) ? 1 : 0;
  if (random < 0)
    randX = -randX;

  random = rand();
  randY = ((random & 0x0F) < 8) ? 1 : 0;
  if (random < 0)
    randY = -randY;

  // Move the creature randomly
  if (randX || randY)
  {
    if ((creatureTile->locationX + randX > (GAME_GRID_WIDTH - 1)) ||
        (creatureTile->locationX + randX < 0))
      randX = -randX;
    if ((creatureTile->locationY + randY > (GAME_GRID_HEIGHT - 1)) ||
        (creatureTile->locationY + randY < 0))
      randY = -randY;

    sprite_move(creatureTile, creatureTile->locationX + randX,
                  creatureTile->locationY + randY);
  }
}

/*...................................................................*/
/* sprite_find_nearest: Find the nearest sprite on game grid         */
/*                                                                   */
/*   Input: creatureTile is pointer to creature to search nearby     */
/*                                                                   */
/*  Return: Pointer to the nearest SpriteTile, or NULL               */
/*...................................................................*/
static SpriteTile *sprite_find_nearest(SpriteTile *creatureTile)
{
  int x, y, distance, startX, startY, endX, endY, curr_distance;
  BackgroundTile *theTerrain;

  // Determine distance creature can perceive
  if (creatureTile->stats.flags & TRACKING)
    distance = 5;
  else if (creatureTile->stats.flags & VISION)
    distance = 3;
  else
    distance = 1;

  // Check in concentric rings around the sprite
  for (curr_distance = 1; curr_distance <= distance; ++curr_distance)
  {
    startX = creatureTile->locationX - curr_distance;
    if (startX < 0)
      startX = 0;
    endX = creatureTile->locationX + curr_distance;
    if (endX >= GAME_GRID_WIDTH)
      endX = GAME_GRID_WIDTH - 1;

    startY = creatureTile->locationY - curr_distance;
    if (startY < 0)
      startY = 0;
    endY = creatureTile->locationY + curr_distance;
    if (endY >= GAME_GRID_HEIGHT)
      endY = GAME_GRID_HEIGHT - 1;

    for (y = startY; y <= endY; ++y)
    {
      for (x = startX; x <= endX; ++x)
      {
        // Find the specific terrain tile for this location
        theTerrain = creatureTile->currentWorld->tiles;
        theTerrain = &(theTerrain[x + y * GAME_GRID_WIDTH]);

        // Check terrain tile for creature (sprite)
        if (theTerrain->sprite && (theTerrain->sprite != creatureTile) &&
            (theTerrain->sprite->tile.flags & IS_VISIBLE))
          return theTerrain->sprite;
      }
    }
  }

  // Return no nearby creature
  return NULL;
}

/*...................................................................*/
/* game_round: Timer callback to perform round of sprite actions     */
/*                                                                   */
/*   Input: unused is an unused parameter                            */
/*          character is void pointer to PlayerTile                  */
/*          creatures is void pointer to array of CreatureTile's     */
/*                                                                   */
/*  Return: TASK_FINISHED                                            */
/*...................................................................*/
static int game_round(u32 unused, void *character, void *creatures)
{
  static int lastX = 0, lastY = 0, maxHp = 0;
  SpriteTile *creatureTiles = creatures;
  PlayerTile *characterTile = character;
  SpriteTile *nearbyCreature;
  int i;

  StdioState = &ConsoleState;

  // Perform an action for the creatures
  for (i = 0; i < characterTile->level + 1; ++i)
  {
    if (!(creatureTiles[i].tile.flags & IS_DEAD))
    {
      if (creatureTiles[i].stats.hp > 0)
      {
        // Check if there are any nearby creatures
        nearbyCreature = sprite_find_nearest(&creatureTiles[i]);

        // If no nearby creature or no action, have creature wander
        if (!(nearbyCreature &&
              sprite_action(&creatureTiles[i], nearbyCreature)))
        {
          if (creatureTiles[i].stats.flags & AMBUSH)
          {
            // If we have not moved for three rounds, hide
            if (creatureTiles[i].stats.scratch >= 3)
            {
              BackgroundTile *theTerrain;

              creatureTiles[i].tile.flags &= ~IS_VISIBLE;

              theTerrain = creatureTiles[i].currentWorld->tiles;
              theTerrain = &theTerrain[creatureTiles[i].locationX +
                  (creatureTiles[i].locationY * GAME_GRID_WIDTH)];

              // If background tile visible, redraw it
              if (theTerrain->tile.flags & IS_VISIBLE)
                TileDisplay(theTerrain, creatureTiles[i].locationX,
                            creatureTiles[i].locationY);
            }
            else if (creatureTiles[i].tile.flags & IS_VISIBLE)
              ++creatureTiles[i].stats.scratch;
          }
          else
            sprite_move_random(&creatureTiles[i]);
        }
      }
      else
        creatureTiles[i].tile.flags |= IS_DEAD;
    }
  }

  // Perform an action for the character
  if (characterTile)
  {
    // The character must be conscious to act
    if (characterTile->player.stats.hp > 0)
    {
#if CHARACTER_AUTO_ATTACK
      // Check if there are any nearby creatures
      nearbyCreature = sprite_find_nearest(&characterTile->character);

      // If nearby attempt to detect and act on the nearby creature
      if (nearbyCreature)
      {
        AttackType *attack = attack_range(&characterTile->character,
                                          nearbyCreature);

        puts(nearbyCreature->name);
        if (attack)
          creature_attack(&characterTile->character, attack,
                         nearbyCreature);
      }
#endif /* CHARACTER_AUTO_ATTACK */

      // If cleric, check to see if no movement long enough to heal
      if (characterTile->player.stats.flags & CLASS_CLERIC)
      {
        if (characterTile->player.stats.hp > maxHp)
          maxHp = characterTile->player.stats.hp;

        if ((lastX == characterTile->player.locationX) &&
            (lastY == characterTile->player.locationY))
          ++creatureTiles[i].stats.scratch;
        else
          creatureTiles[i].stats.scratch = 0;

        if ((characterTile->player.stats.hp < maxHp) &&
            (creatureTiles[i].stats.scratch > 4))
        {
          // Roll healing and add proficiency
          int hp = (rand() % 8) + 2 + (characterTile->score >> 4);

          // Add wisdom modifier
          if (characterTile->player.stats.wis > 10)
            hp += (characterTile->player.stats.wis - 10) >> 1;

          if (hp + characterTile->player.stats.hp < maxHp)
            characterTile->player.stats.hp += hp;
          else
            characterTile->player.stats.hp = maxHp;

          putbyte(hp);
          puts("You are healed");
          creatureTiles[i].stats.scratch = 0;
          player_display(characterTile);
        }

        lastX = characterTile->player.locationX;
        lastY = characterTile->player.locationY;
      }

      // If scout check to see if we stay still long enough to hide
      else if (characterTile->player.stats.flags & CLASS_SCOUT)
      {
        if ((lastX == characterTile->player.locationX) &&
            (lastY == characterTile->player.locationY))
          ++characterTile->player.stats.scratch;
        else
        {
          characterTile->player.stats.scratch = 0;
          characterTile->player.tile.flags |= IS_VISIBLE;
        }
        if (characterTile->player.stats.scratch > 3)
        {
          if (characterTile->player.tile.flags & IS_VISIBLE)
            puts("Hiding");
          characterTile->player.tile.flags &= ~IS_VISIBLE;
        }

        lastX = characterTile->player.locationX;
        lastY = characterTile->player.locationY;
      }
    }
    else
    {
      puts("You died, game over");
      TimerCancel(game_round, character);
      return TASK_FINISHED;
    }
  }

  // Schedule next game round and return finished
  TimerSchedule(GamePeriod, game_round, character, creatures);
  return TASK_FINISHED;
}

/*...................................................................*/
/* game_start: Start the virtual world                               */
/*                                                                   */
/*   Input: character is pointer to PlayerTile                       */
/*          creatures is pointer to array of CreatureTile's          */
/*          world is pointer to game grid of TerrainTile's           */
/*          period is number of micro seconds between game rounds    */
/*...................................................................*/
static void game_start(PlayerTile *character, SpriteTile *creatures,
                 World *world, int gamePeriod)
{
  int i, randX, randY, locationFound;
  BackgroundTile *middleEarth = world->tiles;
  u32 divisor = (GAME_GRID_WIDTH - 1) | 0xF;

  for (i = 0; i < character->level + 1; ++i)
  {
    if (i < 7)
    {
      int roll = rand();

      if ((roll % 100) < 50)
      {
        creatures[i].tile.color = COLOR_BROWN;
        creatures[i].tile.flags = IS_VISIBLE;
        creatures[i].tile.tileNum = CREATURE_GIANT_BUG;
        creatures[i].stats.ac = 15;
        creatures[i].stats.hp = 9 + i;
        creatures[i].stats.str = 16;
        creatures[i].stats.dex = 12;
        creatures[i].stats.intel = 4;
        creatures[i].stats.con = 10;
        creatures[i].stats.wis = 3;
        creatures[i].stats.flags = AGGRESSIVE | VISION | TRACKING;
        creatures[i].stats.first.affect = 1 + (i > 5);
        creatures[i].stats.first.range = 1;
        creatures[i].stats.first.name = "bite";
        creatures[i].stats.second.affect = 0;
        creatures[i].stats.second.range = 0;
        creatures[i].stats.second.name = "none";
        creatures[i].name = "Giant Beetle";
      }
      else
      {
        creatures[i].tile.color = COLOR_RED;
        creatures[i].tile.flags = IS_VISIBLE;
        creatures[i].tile.tileNum = CREATURE_BAT;
        creatures[i].stats.ac = 12;
        creatures[i].stats.hp = 6 + i;
        creatures[i].stats.str = 8;
        creatures[i].stats.dex = 16;
        creatures[i].stats.intel = 4;
        creatures[i].stats.con = 10;
        creatures[i].stats.wis = 3;
        creatures[i].stats.flags = CAUTIOUS | VISION | FLYING;
        creatures[i].stats.first.affect = 1;
        creatures[i].stats.first.range = 1;
        creatures[i].stats.first.name = "bite";
        creatures[i].stats.second.affect = 0;
        creatures[i].stats.second.range = 0;
        creatures[i].stats.second.name = "none";
        creatures[i].name = "Bat";
      }
    }
    else if (i < 16)
    {
      int roll = rand();

      if ((roll % 100) < 50)
      {
        creatures[i].tile.color = COLOR_ORANGERED;
        creatures[i].tile.flags = 0;
        creatures[i].tile.tileNum = CREATURE_LAND_SQUID;
        creatures[i].stats.ac = 12;
        creatures[i].stats.hp = 10 + i;
        creatures[i].stats.str = 16;
        creatures[i].stats.dex = 9;
        creatures[i].stats.intel = 4;
        creatures[i].stats.con = 10;
        creatures[i].stats.wis = 3;
        creatures[i].stats.flags = AGGRESSIVE | AMBUSH | VISION;
        creatures[i].stats.first.affect = 2;
        creatures[i].stats.first.range = 1;
        creatures[i].stats.first.name = "bite";
        creatures[i].stats.second.affect = 0;
        creatures[i].stats.second.range = 0;
        creatures[i].stats.second.name = "none";
        creatures[i].name = "Land Squid";
      }
      else
      {
        creatures[i].tile.color = COLOR_WHITE;
        creatures[i].tile.flags = 0;
        creatures[i].tile.tileNum = CREATURE_SHARK;
        creatures[i].stats.ac = 14;
        creatures[i].stats.hp = 15 + i;
        creatures[i].stats.str = 18;
        creatures[i].stats.dex = 16;
        creatures[i].stats.intel = 4;
        creatures[i].stats.con = 10;
        creatures[i].stats.wis = 3;
        creatures[i].stats.flags = AGGRESSIVE | AMBUSH | TRACKING;
        creatures[i].stats.first.affect = 2;
        creatures[i].stats.first.range = 1;
        creatures[i].stats.first.name = "bite";
        creatures[i].stats.second.affect = 0;
        creatures[i].stats.second.range = 0;
        creatures[i].stats.second.name = "none";
        creatures[i].name = "White Shark";
      }
    }
    else
    {
      int roll = rand();

      if ((roll % 100) < 45)
      {
        creatures[i].tile.color = COLOR_GOLDEN;
        creatures[i].tile.flags = IS_VISIBLE;
        creatures[i].tile.tileNum = CREATURE_MANTICORE;
        creatures[i].stats.ac = 14;
        creatures[i].stats.hp = i;
        creatures[i].stats.str = 16;
        creatures[i].stats.dex = 16;
        creatures[i].stats.intel = 4;
        creatures[i].stats.con = 10;
        creatures[i].stats.wis = 3;
        creatures[i].stats.flags = AGGRESSIVE | FLYING | VISION |AMBUSH;
        creatures[i].stats.first.affect = 2;
        creatures[i].stats.first.range = 1;
        creatures[i].stats.first.name = "bite";
        creatures[i].stats.second.affect = 2;
        creatures[i].stats.second.range = 3;
        creatures[i].stats.second.name = "spike";
        creatures[i].name = "Manticore";
      }
      else if ((roll % 100) < 90)
      {
        creatures[i].tile.color = COLOR_MAGENTA;
        creatures[i].tile.flags = IS_VISIBLE;
        creatures[i].tile.tileNum = CREATURE_GIANT_RAT;
        creatures[i].stats.ac = 14;
        creatures[i].stats.hp = 15 + i;
        creatures[i].stats.str = 16;
        creatures[i].stats.dex = 16;
        creatures[i].stats.intel = 4;
        creatures[i].stats.con = 10;
        creatures[i].stats.wis = 3;
        creatures[i].stats.flags = AGGRESSIVE | VISION;
        creatures[i].stats.first.affect = 3;
        creatures[i].stats.first.range = 1;
        creatures[i].stats.first.name = "bite";
        creatures[i].stats.second.affect = 0;
        creatures[i].stats.second.range = 0;
        creatures[i].stats.second.name = "none";
        creatures[i].name = "Giant Rat";
      }
      else
      {
        creatures[i].tile.color = COLOR_YELLOW;
        creatures[i].tile.flags = IS_VISIBLE;
        creatures[i].tile.tileNum = CREATURE_DRAGON_FLY;
        creatures[i].stats.ac = 26;
        creatures[i].stats.hp = 25 + i;
        creatures[i].stats.str = 18;
        creatures[i].stats.dex = 18;
        creatures[i].stats.intel = 10;
        creatures[i].stats.con = 18;
        creatures[i].stats.wis = 13;
        creatures[i].stats.flags = AGGRESSIVE | FLYING | VISION |
                                   TRACKING;
        creatures[i].stats.first.affect = 5;
        creatures[i].stats.first.range = 1;
        creatures[i].stats.first.name = "bite";
        creatures[i].stats.second.affect = 3;
        creatures[i].stats.second.range = 5;
        creatures[i].stats.second.name = "stinger";
        creatures[i].name = "Dragon Wasp";
      }
    }

    for (;;)
    {
      randX = rand() & divisor;
      if (randX > GAME_GRID_WIDTH - 1)
        randX -= 5;
      randY = rand() & divisor;
      if (randY > GAME_GRID_HEIGHT - 1)
        randY -= 5;

      // Flying creatures can spawn anywhere so break
      if (creatures[i].stats.flags & FLYING)
        break;

      // Check if creature can be in the mountains
      if (middleEarth[randX + randY * GAME_GRID_WIDTH].tile.tileNum ==
                                                      TERRAIN_MOUNTAINS)
      {
        if (creatures[i].tile.tileNum == CREATURE_LAND_SQUID)
          continue;
      }

      if (middleEarth[randX + randY * GAME_GRID_WIDTH].
                                          tile.tileNum == TERRAIN_WATER)
      {
        // Sharks can only be in water
        if (creatures[i].tile.tileNum == CREATURE_SHARK)
          break;

        if (creatures[i].tile.tileNum == CREATURE_GIANT_BUG)
          continue;
      }
      else if (creatures[i].tile.tileNum == CREATURE_SHARK)
        continue;

      // Otherwise this location is great so start creature here
      break;
    }

    // Assign starting location in the world and make visible
    creatures[i].currentWorld = world;
    creatures[i].locationX = randX;
    creatures[i].locationY = randY;
    creatures[i].stats.scratch = 0;

    // Update the world so the terrain knows this creatures is in it
    middleEarth[randX + randY * GAME_GRID_WIDTH].sprite = &creatures[i];
  }

  // Display the teleportation gate in a random location
  locationFound = 0;
  while (!locationFound)
  {
    randX = rand() & divisor;
    if (randX > GAME_GRID_WIDTH - 1)
      randX -= 5;
    randY = rand() & divisor;
    if (randY > GAME_GRID_HEIGHT - 1)
      randY -= 5;

    // Do not allow the gate location to be in mountains or water
    if ((middleEarth[randX + randY * GAME_GRID_WIDTH].tile.tileNum ==
         TERRAIN_MOUNTAINS) || (middleEarth[randX + randY *
                        GAME_GRID_WIDTH].tile.tileNum == TERRAIN_WATER))
      locationFound = 0;

    // Do not let the portal location be the edge
    else if (!randX || !randY ||
             (randX == GAME_GRID_WIDTH - 1) ||
             (randY == GAME_GRID_HEIGHT - 1))
      locationFound = 0;
    else
      locationFound = 1;
  }

  // Mark terrain tile as the world exit
  middleEarth[randX + randY * GAME_GRID_WIDTH].tile.flags |= WORLD_EXIT;

  // Display the character in a random location
  locationFound = 0;
  while (!locationFound)
  {
    randX = rand() & divisor;//0x2F;
    if (randX > GAME_GRID_WIDTH - 1) // Needed for 28 (640x480)
      randX -= 5;
    randY = rand() & divisor;
    if (randY > GAME_GRID_HEIGHT - 1)
      randY -= 5;
    if ((middleEarth[randX + randY * GAME_GRID_WIDTH].
                                   tile.tileNum == TERRAIN_MOUNTAINS) ||
        (middleEarth[randX + randY * GAME_GRID_WIDTH].
                                         tile.tileNum == TERRAIN_WATER))
      locationFound = 0;
    else
      locationFound = 1;
  }

  character->player.currentWorld = world;
  character->player.locationX = randX;
  character->player.locationY = randY;
  middleEarth[randX + randY * GAME_GRID_WIDTH].sprite =
                                                     &character->player;
  // Look around
  player_look(character);

  // Display the character details on top left of screen
  player_display(character);

  // Schedule next Game Tick and return success
  GamePeriod = gamePeriod;
  TimerCancel(game_round, character);
  TimerSchedule(gamePeriod, game_round, character, creatures);
}

/*...................................................................*/
/* game_level_up: Transition game to new level                       */
/*                                                                   */
/*   Input: characterTile is void pointer to PlayerTile              */
/*...................................................................*/
static void game_level_up(PlayerTile *characterTile)
{
  int num_creatures;

  // Decrease the game period to make the rounds go faster
  if (GamePeriod > MICROS_PER_SECOND / 2)
    GamePeriod -= MICROS_PER_SECOND / 20; /* 10 times */
  else if (GamePeriod > MICROS_PER_SECOND / 4) /* 15 times */
    GamePeriod -= MICROS_PER_SECOND / 60;
  else if (GamePeriod > MICROS_PER_SECOND / 8) /* 13 times */
    GamePeriod -= MICROS_PER_SECOND / 100;

  // Clear the screen first
  ScreenClear();

  // Reseed the PRNG
  srand(TimerNow() ^ ('L' + characterTile->score));

  // Cancel the old timer
  TimerCancel(game_round, characterTile);

  // The portal advances levels

  // Update wizard stats after going through portal
  if (characterTile->player.stats.flags & CLASS_WIZARD)
  {
    characterTile->player.stats.first.affect = 1 +
      (characterTile->score >> 3);

    if (characterTile->player.stats.first.affect > 1)
    {
      characterTile->player.stats.second.affect = 0 +
                                            (characterTile->score >> 3);
      characterTile->player.stats.second.range = 3 +
                                            (characterTile->score >> 4);
      characterTile->player.stats.second.name = "blast";
    }
  }

  if (characterTile->level < MAX_CREATURES - 1)
    ++characterTile->level;

  num_creatures = characterTile->level + 1;

  // Increase creatures per level depending on game grid size
  if (num_creatures > GAME_GRID_WIDTH)
    world_create_random(characterTile->player.currentWorld,
                        WORLD_WEST_COAST | WORLD_NORTH_COAST |
              WORLD_EAST_COAST | WORLD_SOUTH_COAST | WORLD_MOUNTAINOUS);
  else if (num_creatures > GAME_GRID_WIDTH -
           (GAME_GRID_WIDTH / 4))
    world_create_random(characterTile->player.currentWorld,
                        WORLD_NORTH_COAST | WORLD_WEST_COAST |
                        WORLD_MOUNTAINOUS);
  else if (num_creatures > GAME_GRID_WIDTH / 2)
    world_create_random(characterTile->player.currentWorld,
                        WORLD_NORTH_COAST | WORLD_WEST_COAST);
  else if (num_creatures > GAME_GRID_WIDTH / 8)
    world_create_random(characterTile->player.currentWorld,
                        WORLD_NORTH_COAST);
  else
    world_create_random(characterTile->player.currentWorld, 0);

  game_start(characterTile, TheCreatures,
             characterTile->player.currentWorld, GamePeriod);
}

/*...................................................................*/
/* player_move: Move the player sprite to a new game grid location   */
/*                                                                   */
/*   Input: unused is an unused parameter                            */
/*          character is void pointer to PlayerTile                  */
/*          creatures is void pointer to array of CreatureTile's     */
/*                                                                   */
/*  Return: Zero (0) on success, -1 on failure                       */
/*...................................................................*/
static int player_move(PlayerTile *characterTile, int tileX, int tileY)
{
  SpriteTile *neighborSprite;
  BackgroundTile *theTerrain = characterTile->player.currentWorld->tiles;

  theTerrain = &theTerrain[tileX + tileY * GAME_GRID_WIDTH];
  if (theTerrain->tile.tileNum == TERRAIN_MOUNTAINS)
  {
    int i;

    for (i = 0; i < characterTile->num_items; ++i)
      if (characterTile->items[i].tile.tileNum == ITEM_MAGIC_BOOTS)
        break;

    if (!i || (i == characterTile->num_items))
    {
      puts("Mountains impassible");
      return -1;
    }
  }
  if (theTerrain->tile.tileNum == TERRAIN_WATER)
  {
    int i;

    for (i = 0; i < characterTile->num_items; ++i)
      if (characterTile->items[i].tile.tileNum == ITEM_MAGIC_BELT)
        break;

    if (!i || (i == characterTile->num_items))
    {
      puts("Water impassible");
      return -1;
    }
    else
      puts("Belt of Swimming");
  }

  // Move the player
  neighborSprite = SpriteMove(&characterTile->player, tileX, tileY);

  // If move encountered another sprite, check if attack is warrented
  if (neighborSprite)
  {
    // Melee attack the adjacent creature if aggressive
    if ((neighborSprite != (void *)-1) &&
        (characterTile->player.stats.flags & AGGRESSIVE))
      creature_attack(&characterTile->player,
                    &characterTile->player.stats.first, neighborSprite);

    // Return a failure to move the player
    return -1;
  }

  // Look around the players new location
  player_look(characterTile);

  // If character moved to the level up tile, recreate world
  if (theTerrain->tile.flags & WORLD_EXIT)
  {
    game_level_up(characterTile);
    return 0;
  }

  // Character picks up any item if room
  if (theTerrain->item && (characterTile->num_items <= MAX_ITEMS))
  {
    int i;

    puts("Item located");

    // Ignore duplicate items
    for (i = 0; i < characterTile->num_items; ++i)
      if (characterTile->items[i].tile.tileNum ==
                                         theTerrain->item->tile.tileNum)
        break;

    // If not a duplicate item, add it to the character
    if (i == characterTile->num_items)
    {
      if (theTerrain->item->tile.tileNum == ITEM_POTION)
      {
        // Drink right away
        characterTile->player.stats.hp += 8;
        puts("The potion energizes you.");
        player_display(characterTile);
      }
      else
      {
        if (theTerrain->item->tile.tileNum == ITEM_ARMOUR_LEATHER)
          characterTile->player.stats.ac += 1;

        else if (!(characterTile->player.stats.flags & CLASS_WIZARD))
        {
          if (theTerrain->item->tile.tileNum == ITEM_ARMOUR_SHEILD)
            characterTile->player.stats.ac += 2;

          if (!(characterTile->player.stats.flags & CLASS_SCOUT))
          {
            if (theTerrain->item->tile.tileNum == ITEM_PLATEMAIL)
              characterTile->player.stats.ac = 18;
            else if (theTerrain->item->tile.tileNum == ITEM_SPLINTMAIL)
              characterTile->player.stats.ac = 17;
            else if (theTerrain->item->tile.tileNum == ITEM_CHAINMAIL)
              characterTile->player.stats.ac = 16;
            else if (theTerrain->item->tile.tileNum == ITEM_GAUNTLETS)
              characterTile->player.stats.ac += 1;
          }

          if (theTerrain->item->tile.tileNum == ITEM_WEAPON_SWORD)
            characterTile->player.stats.first.affect += 1;
          else if (theTerrain->item->tile.tileNum == ITEM_FLAMING_SWORD)
            characterTile->player.stats.first.affect += 1;
          else if (theTerrain->item->tile.tileNum == ITEM_POWER_SWORD)
          {
            characterTile->player.stats.first.affect += 1;
            if (characterTile->player.stats.str >= 16)
              characterTile->player.stats.str += 3;
            else
              characterTile->player.stats.str = 18;
          }

          if (!(characterTile->player.stats.flags & CLASS_CLERIC))
          {
            if (theTerrain->item->tile.tileNum == ITEM_WEAPON_BOW)
            {
              characterTile->player.stats.second.affect += 1;
              characterTile->player.stats.second.range = 3;
              if (characterTile->player.stats.second.affect > 1)
                characterTile->player.stats.second.name = "zing";
              else
                characterTile->player.stats.second.name = "fling";
            }
            else if (theTerrain->item->tile.tileNum == ITEM_WEAPON_ARROW)
            {
              characterTile->player.stats.second.affect += 1;
              if (characterTile->player.stats.second.range == 3)
                characterTile->player.stats.second.name = "zing";
            }
          }
        }
        else
        {
          if (theTerrain->item->tile.tileNum == ITEM_POWER_STAFF)
          {
            characterTile->player.stats.first.affect += 2;
            characterTile->player.stats.second.affect += 1;
          }
        }

        if (theTerrain->item->tile.tileNum == ITEM_HELMET)
          characterTile->player.stats.ac += 1;
        else if (theTerrain->item->tile.tileNum == ITEM_LIGHTNING_SWORD)
        {
          if (characterTile->player.stats.flags & CLASS_CLERIC)
            characterTile->player.stats.first.affect += 2;
        }

        characterTile->items[characterTile->num_items++].tile =
                                                 theTerrain->item->tile;
      }

      player_display(characterTile);
      theTerrain->item = NULL;
      puts("Item taken");
    }
  }

  return 0;
}

/*...................................................................*/
/* player_create_random: Create a random player character            */
/*                                                                   */
/*   Input: character is pointer to PlayerTile                       */
/*...................................................................*/
static void player_create_random(PlayerTile *character)
{
  int d6roll;

  bzero(character, sizeof(PlayerTile));

  // Default adventurer is aggresive with keen vision
  character->player.stats.flags = AGGRESSIVE | VISION | IS_PLAYER;
  character->num_items = 0;

  // Roll 6 sided dice three times for strength
  d6roll = rand() % 6;
  character->player.stats.str = d6roll + 1;
  character->player.stats.str += rand() % 6 + 1; // add another d6
  character->player.stats.str += rand() % 6 + 1; // add another d6

  // Roll 6 sided dice for dexterity
  d6roll = rand() % 6;
  character->player.stats.dex = d6roll + 1;
  character->player.stats.dex += rand() % 6 + 1; // add another d6
  character->player.stats.dex += rand() % 6 + 1; // add another d6

  // Roll 6 sided dice for constitution
  d6roll = rand() % 6;
  character->player.stats.con = d6roll + 1;
  character->player.stats.con += rand() % 6 + 1; // add another d6
  character->player.stats.con += rand() % 6 + 1; // add another d6

  // Roll 6 sided dice for intellegence
  d6roll = rand() % 6;
  character->player.stats.intel = d6roll + 1;
  character->player.stats.intel += rand() % 6 + 1; // add another d6
  character->player.stats.intel += rand() % 6 + 1; // add another d6

  // Roll 6 sided dice for wisdom
  d6roll = rand() % 6;
  character->player.stats.wis = d6roll + 1;
  character->player.stats.wis += rand() % 6 + 1; // add another d6
  character->player.stats.wis += rand() % 6 + 1; // add another d6

  // Determine race/class to optimize abilities

 // If intellegence highest, be a wizard
  if ((character->player.stats.intel > character->player.stats.dex) &&
      (character->player.stats.intel > character->player.stats.str) &&
      (character->player.stats.intel > character->player.stats.wis))
  {
    character->player.stats.flags |= CLASS_WIZARD;
    character->player.stats.hp = 6;
    character->player.tile.color = COLOR_RED;
    character->player.tile.tileNum = CHARACTER_WIZARD;
  }

  // If wisdom highest, be a cleric
  else if ((character->player.stats.wis > character->player.stats.dex)&&
        (character->player.stats.wis > character->player.stats.intel) &&
           (character->player.stats.wis > character->player.stats.str))
  {
    character->player.stats.flags |= CLASS_CLERIC;
    character->player.stats.hp = 8;
    character->player.tile.color = COLOR_RED;
    character->player.tile.tileNum = CHARACTER_CLERIC;
  }

  // If dexterity highest, be a scout
  else if ((character->player.stats.dex > character->player.stats.wis)&&
        (character->player.stats.dex > character->player.stats.intel) &&
           (character->player.stats.dex > character->player.stats.str))
  {
    character->player.stats.flags |= CLASS_SCOUT | TRACKING;
    character->player.stats.hp = 8;
    character->player.tile.color = COLOR_RED;
    character->player.tile.tileNum = CHARACTER_MONK;
  }

  // Otherwise be a ranger
  else
  {
    character->player.stats.flags |= CLASS_RANGER | TRACKING;
    character->player.tile.color = COLOR_RED;
    character->player.tile.tileNum = CHARACTER_MONK;
    character->player.stats.hp = 10;
  }

  // If high constitution, be human
  if (character->player.stats.con > 10)
  {
    character->player.stats.flags |= RACE_HUMAN;
    character->player.stats.str += 1;
    character->player.stats.dex += 1;
    character->player.stats.con += 1;
    character->player.stats.intel += 1;
    character->player.stats.wis += 1;
  }

  // Otherwise optimize for class
  else if (character->player.stats.flags & CLASS_RANGER)
  {
    character->player.stats.flags |= RACE_DWARF;
    character->player.stats.str += 2;
    character->player.stats.con += 2;
  }

  else if (character->player.stats.flags & CLASS_WIZARD)
  {
    character->player.stats.flags |= RACE_ELF;
    character->player.stats.intel += 1;
    character->player.stats.dex += 2;
  }
  else if (character->player.stats.flags & CLASS_CLERIC)
  {
    character->player.stats.flags |= RACE_DWARF;
    character->player.stats.wis += 1;
    character->player.stats.con += 2;
  }
  else if (character->player.stats.flags & CLASS_SCOUT)
  {
    character->player.stats.flags |= RACE_HOBIT;
    character->player.stats.dex += 2;
    character->player.stats.con += 1;
  }
  else
    puts("FATAL ERROR!");

  // Compute the AC based on no armor and dexterity modifier
  character->player.stats.ac = 10;
  if (character->player.stats.dex > 10)
    character->player.stats.ac +=
       ((character->player.stats.dex - 10) >> 1); // (Dex - 10) / 2

  // Configure attack for stick/staff/club melee attack
  if (character->player.stats.flags & CLASS_WIZARD)
  {
    character->player.stats.first.affect = 1;
    character->player.stats.first.range = 1;
    character->player.stats.first.name = "zap";
  }
  else
  {
    character->player.stats.first.affect = 1;
    character->player.stats.first.range = 1;
    character->player.stats.first.name = "pummel";
  }

  // Configure for 1st level, no experience or ranged attack
  character->level = 1;
  character->score = 0;
  character->player.stats.scratch = 0;
  character->player.stats.second.affect = 0;
  character->player.stats.second.range = 0;
  character->player.stats.second.name = "none";
  character->player.name = "xlawless";
  character->player.name[0] = rand() % 32 + 'A';
}

// Global function declarations

/*...................................................................*/
/* GamePause: Shell interface command to pause the game              */
/*                                                                   */
/*   Input: command is an unused parameter                           */
/*                                                                   */
/*  Return: TASK_FINISHED                                            */
/*...................................................................*/
int GamePause(const char *command)
{
  // If game on, pause the game by cancelling the timer
  if (GameUp)
  {
    puts("Game paused");
    TimerCancel(game_round, &TheCharacter);
    GameUp = FALSE;
  }

  // Otherwise start the game by starting the timer
  else
  {
    puts("Game resumed");
    TimerSchedule(GamePeriod, game_round, &TheCharacter, TheCreatures);
    GameUp = TRUE;
  }
  return TASK_FINISHED;
}

/*...................................................................*/
/* GameStart: Shell interface command to start the game              */
/*                                                                   */
/*   Input: command is an unused parameter                           */
/*                                                                   */
/*  Return: TASK_FINISHED                                            */
/*...................................................................*/
int GameStart(const char *command)
{
  if (ScreenUp)
  {
    // Clear the screen first
    ScreenClear();

    // Seed the PRNG
    srand(TimerNow() ^ 'G');

    TheWorld.tiles = (void *)MiddleEarth;
    TheWorld.x = GAME_GRID_WIDTH;
    TheWorld.y = GAME_GRID_HEIGHT;

    player_create_random(&TheCharacter);

    world_create_random(&TheWorld, 0);
    game_start(&TheCharacter, TheCreatures, &TheWorld,
               MICROS_PER_SECOND);

    StdioState = &ConsoleState;
    GameUp = TRUE;
    puts("\nAdventure awaits!");
  }
  else
    puts("Video screen not initialized");

  return TASK_FINISHED;
}

/*...................................................................*/
/* Action: Shell interface command to perform player action          */
/*                                                                   */
/*   Input: command is an unused parameter                           */
/*                                                                   */
/*  Return: TASK_FINISHED                                            */
/*...................................................................*/
int Action(const char *command)
{
  if (GameUp)
  {
    // Check if there are any nearby creatures
    SpriteTile *nearbyCreature=sprite_find_nearest(&TheCharacter.player);

    StdioState = &ConsoleState;

    // If nearby attempt to detect and act on the nearby creature
    if (nearbyCreature)
    {
      ActionType *attack = attack_range(&TheCharacter.player,
                                        nearbyCreature);
      puts(nearbyCreature->name);

      if (attack)
        creature_attack(&TheCharacter.player, attack,
                       nearbyCreature);
    }
  }
  return TASK_FINISHED;
}

/*...................................................................*/
/*   North: Shell interface command to move player north (up)        */
/*                                                                   */
/*   Input: command is an unused parameter                           */
/*                                                                   */
/*  Return: TASK_FINISHED                                            */
/*...................................................................*/
int North(const char *command)
{
  if (GameUp)
  {
    StdioState = &ConsoleState;

    player_move(&TheCharacter, TheCharacter.player.locationX,
                TheCharacter.player.locationY - 1);
  }
  return TASK_FINISHED;
}

/*...................................................................*/
/*   South: Shell interface command to move player south (down)      */
/*                                                                   */
/*   Input: command is an unused parameter                           */
/*                                                                   */
/*  Return: TASK_FINISHED                                            */
/*...................................................................*/
int South(const char *command)
{
  if (GameUp)
  {
    StdioState = &ConsoleState;

    player_move(&TheCharacter, TheCharacter.player.locationX,
                TheCharacter.player.locationY + 1);
  }
  return TASK_FINISHED;
}

/*...................................................................*/
/*    East: Shell interface command to move player east (right)      */
/*                                                                   */
/*   Input: command is an unused parameter                           */
/*                                                                   */
/*  Return: TASK_FINISHED                                            */
/*...................................................................*/
int East(const char *command)
{
  if (GameUp)
  {
    StdioState = &ConsoleState;

    player_move(&TheCharacter, TheCharacter.player.locationX + 1,
                TheCharacter.player.locationY);
  }
  return TASK_FINISHED;
}

/*...................................................................*/
/*    West: Shell interface command to move player west (left)       */
/*                                                                   */
/*   Input: command is an unused parameter                           */
/*                                                                   */
/*  Return: TASK_FINISHED                                            */
/*...................................................................*/
int West(const char *command)
{
  if (GameUp)
  {
    StdioState = &ConsoleState;

    player_move(&TheCharacter, TheCharacter.player.locationX - 1,
                TheCharacter.player.locationY);
  }
  return TASK_FINISHED;
}

/*...................................................................*/
/* NorthEast: Shell command to move player north east (up and right) */
/*                                                                   */
/*   Input: command is an unused parameter                           */
/*                                                                   */
/*  Return: TASK_FINISHED                                            */
/*...................................................................*/
int NorthEast(const char *command)
{
  if (GameUp)
  {
    StdioState = &ConsoleState;

    player_move(&TheCharacter, TheCharacter.player.locationX + 1,
                TheCharacter.player.locationY - 1);
  }
  return TASK_FINISHED;
}

/*...................................................................*/
/* NorthWest: Shell command to move player north west (up and left)  */
/*                                                                   */
/*   Input: command is an unused parameter                           */
/*                                                                   */
/*  Return: TASK_FINISHED                                            */
/*...................................................................*/
int NorthWest(const char *command)
{
  if (GameUp)
  {
    StdioState = &ConsoleState;

    player_move(&TheCharacter, TheCharacter.player.locationX - 1,
                TheCharacter.player.locationY - 1);
  }
  return TASK_FINISHED;
}

/*...................................................................*/
/* SouthEast: Shell command to move player south east (down right)   */
/*                                                                   */
/*   Input: command is an unused parameter                           */
/*                                                                   */
/*  Return: TASK_FINISHED                                            */
/*...................................................................*/
int SouthEast(const char *command)
{
  if (GameUp)
  {
    StdioState = &ConsoleState;

    player_move(&TheCharacter, TheCharacter.player.locationX + 1,
                TheCharacter.player.locationY + 1);
  }
  return TASK_FINISHED;
}

/*...................................................................*/
/* SouthWest: Shell command to move player south west (up and left)  */
/*                                                                   */
/*   Input: command is an unused parameter                           */
/*                                                                   */
/*  Return: TASK_FINISHED                                            */
/*...................................................................*/
int SouthWest(const char *command)
{
  if (GameUp)
  {
    StdioState = &ConsoleState;

    player_move(&TheCharacter, TheCharacter.player.locationX - 1,
                TheCharacter.player.locationY + 1);
  }
  return TASK_FINISHED;
}

#endif /* ENABLE_GAME */
