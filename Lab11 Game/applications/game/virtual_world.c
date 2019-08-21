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

#define MAX_CREATURES         GAME_GRID_WIDTH
#define MAX_ITEMS             16

// Random world flags
#define WORLD_WEST_COAST      (1 << 0)
#define WORLD_EAST_COAST      (1 << 1)
#define WORLD_NORTH_COAST     (1 << 2)
#define WORLD_SOUTH_COAST     (1 << 3)
#define WORLD_MOUNTAINOUS     (1 << 4)

// Sprite statistics flags
#define AGGRESSIVE            (1 << 0)
#define AMBUSH                (1 << 1)
#define CAUTIOUS              (1 << 2)
#define VISION                (1 << 3)
#define TRACKING              (1 << 4)
#define FLYING                (1 << 5)
#define IS_DEAD               (1 << 7)

//
// Game specific Tile flags
//
// Race
#define RACE_HUMAN            (1 << 8)
#define RACE_ELF              (1 << 9)
#define RACE_DWARF            (1 << 10)
#define RACE_HOBIT            (1 << 11)
// Class
#define CLASS_RANGER          (1 << 12)
#define CLASS_SCOUT           (1 << 13)
#define CLASS_WIZARD          (1 << 14)
#define CLASS_CLERIC          (1 << 15)
//Virtual World
#define IS_PLAYER             (1 << 16)
#define IS_CREATURE           (1 << 17)
#define IS_ITEM               (1 << 18)
#define WORLD_EXIT            (1 << 19)

typedef struct {
  u32 flags;        // Flag bits for behavior, state, etc.
  int hp;           // Health Points
  u8 str, dex, con, intel, wis; // Optional abilities
                                // Can be used with PRNG to determine
                                // if actions are successful
  u8 ac, scratch; // Optional Armour Class and game specific
} SpriteStats;

typedef struct
{
  Tile tile;
  SpriteStats stats;
  World *currentWorld;
  char *name;
  int locationX, locationY;
} SpriteTile;

typedef struct
{
  SpriteTile player;
  int num_items;
  Tile items[MAX_ITEMS];
} PlayerTile;

// Global variables
PlayerTile TheCharacter;
SpriteTile TheCreatures[MAX_CREATURES];
Tile ExitTile;
extern World TheWorld;

/*...................................................................*/
/* sprite_initialize: Generic SpriteTile initialization              */
/*                                                                   */
/*   Input: sprite is pointer to the SpriteTile                      */
/*          world is pointer to the World                            */
/*...................................................................*/
static void sprite_initialize(SpriteTile *sprite, World *world)
{
  // Assign world and bitmap data and initialize location
  sprite->currentWorld = world;
  sprite->tile.bitmap = CreaturesData;
  sprite->stats.scratch = 0;
  sprite->locationX = 0;
  sprite->locationY = 0;
  sprite->tile.list.next = NULL;
  sprite->tile.list.previous = NULL;
}

/*...................................................................*/
/* display_string: display a string on the screen                    */
/*                                                                   */
/*   Input: string is the string of characters to display            */
/*          x is width pixel position from left                      */
/*          y is height pixel position from top                      */
/*...................................................................*/
void display_string(char *string, int startX, int startY)
{
  for (char *current = string; *current; ++current)
  {
    DisplayCursorChar(*current, startX, startY, COLOR_WHITE);
    startX += 8;
  }
}

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
    DisplayCursorChar(character->player.name[i], i * 8, 10,
                      COLOR_WHITE);

  if (character->player.stats.flags & RACE_HUMAN)
    display_string("Human ", 0, 30);
  else if (character->player.stats.flags & RACE_ELF)
    display_string(" Elf  ", 0, 30);
  else if (character->player.stats.flags & RACE_DWARF)
    display_string("Dwarf ", 0, 30);
  else if (character->player.stats.flags & RACE_HOBIT)
    display_string("Hobit ", 0, 30);

  if (character->player.stats.flags & CLASS_RANGER)
    display_string("Ranger", 48, 30);
  else if (character->player.stats.flags & CLASS_SCOUT)
    display_string("Scout", 48, 30);
  else if (character->player.stats.flags & CLASS_WIZARD)
    display_string("Wizard", 48, 30);
  else if (character->player.stats.flags & CLASS_CLERIC)
    display_string("Cleric", 48, 30);

  // Strength
  display_string("S:", 0, 50);
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
  display_string(" D:", 32, 50);
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
  display_string(" C:", 72, 50);
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
  display_string(" I:", 112, 50);
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
  display_string("W:", 0, 70);
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
  display_string("  AC:  ", 32, 70);
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
  display_string("HP:  ", 0, 90);
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
  display_string(" XP:  ", 40, 90);
  high = (character->player.currentWorld->score >> 4) & 0xF;
  low = character->player.currentWorld->score & 0xF;
  if (high <= 9)
    DisplayCursorChar('0' + high, 72, 90, COLOR_WHITE);
  else
    DisplayCursorChar('A' + high - 10, 72, 90, COLOR_WHITE);
  if (low <= 9)
    DisplayCursorChar('0' + low, 80, 90, COLOR_WHITE);
  else
    DisplayCursorChar('A' + low - 10, 80, 90, COLOR_WHITE);

  // Level
  display_string(" LV:  ", 88, 90);
  high = ((character->player.currentWorld->level) >> 4) & 0xF;
  low = character->player.currentWorld->level & 0xF;
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
      TileDisplayScreen((Tile *)(&character->items[i]), i * 16, 110);
    else
      TileDisplayScreen((Tile *)(&character->items[i]), (i - 8) * 16, 130);
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
  Tile *middleEarth = world->tiles;

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
      else if ((flags & WORLD_SOUTH_COAST) &&
               (y >= GAME_GRID_HEIGHT - 2))
      {
          random = TERRAIN_WATER;
          color = COLOR_DEEPSKYBLUE;
      }
      else if ((flags & WORLD_WEST_COAST) && (x == 0))
      {
          random = TERRAIN_WATER;
          color = COLOR_DEEPSKYBLUE;
      }
      else if ((flags & WORLD_EAST_COAST) &&
               (x >= GAME_GRID_WIDTH - 2))
      {
          random = TERRAIN_WATER;
          color = COLOR_DEEPSKYBLUE;
      }
      else
      {
        random = (((random & 0xF) ^ ((random & 0xF0) >> 8)) & 0xF) +90;
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
          aboveTile = middleEarth[x + (y - 1) *
                                         GAME_GRID_WIDTH].tileNum;
          if (x > 0)
            aboveTileLeft = middleEarth[(x - 1) + (y - 1) *
                                         GAME_GRID_WIDTH].tileNum;
          else
            aboveTileLeft = 0;

          if (x < GAME_GRID_WIDTH - 1)
            aboveTileRight = middleEarth[(x + 1) + (y - 1) *
                                         GAME_GRID_WIDTH].tileNum;
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
      middleEarth[x + y * GAME_GRID_WIDTH].color = color;
      middleEarth[x + y * GAME_GRID_WIDTH].tileNum = random;
      middleEarth[x + y * GAME_GRID_WIDTH].flags = 0;
#if CIRCULAR_LIST
      middleEarth[x + y * GAME_GRID_WIDTH].list.next =
                            &middleEarth[x + y * GAME_GRID_WIDTH].list;
      middleEarth[x + y * GAME_GRID_WIDTH].list.previous =
                            &middleEarth[x + y * GAME_GRID_WIDTH].list;
#else
      middleEarth[x + y * GAME_GRID_WIDTH].list.next = NULL;
      middleEarth[x + y * GAME_GRID_WIDTH].list.previous = NULL;
#endif
      middleEarth[x + y * GAME_GRID_WIDTH].bitmap =
                                                 TerrainCharactersData;

      // New random number and if bit zero then add reverse flag
      if (rand() & (1 << 0))
        middleEarth[x + y * GAME_GRID_WIDTH].flags = SHOW_REVERSE;

      // If not forest, town or mountains randomly add flipped flag
      if (((middleEarth[x + y * GAME_GRID_WIDTH].tileNum <
            TERRAIN_OAK_FOREST) ||
           (middleEarth[x + y * GAME_GRID_WIDTH].tileNum >
            TERRAIN_PINE_PRAIRIE)) &&
          (middleEarth[x + y * GAME_GRID_WIDTH].tileNum !=
           STRUCTURE_TOWN) && (rand() & (1 << 1)) &&
          (middleEarth[x + y * GAME_GRID_WIDTH].tileNum !=
           TERRAIN_MOUNTAINS) && (rand() & (1 << 1)))
        middleEarth[x + y * GAME_GRID_WIDTH].flags = SHOW_FLIPPED;
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
  Tile *tiles = world->tiles;
  int x = character->player.locationX,
      y = character->player.locationY;
  const char *terrain;
  int distance, startX, startY, endX, endY, tileNum;

  tileNum = tiles[x + y * GAME_GRID_WIDTH].tileNum;

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

  // Determine the distance the player can see
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
      Tile *theTerrain = GameGridTile(x, y);

      // If not already visible, show tile
      if (!(theTerrain->flags & IS_VISIBLE))
      {
        theTerrain->flags |= IS_VISIBLE;
        TileDisplayGrid(x, y);
      }
    }
  }
}

/*...................................................................*/
/* game_start: Start the virtual world                               */
/*                                                                   */
/*   Input: character is pointer to PlayerTile                       */
/*          creatures is pointer to array of CreatureTile's          */
/*          world is pointer to game grid of TerrainTile's           */
/*...................................................................*/
static void game_start(PlayerTile *character, SpriteTile *creatures,
                       World *world)
{
  int i, randX, randY, locationFound;
  Tile *middleEarth = world->tiles;
  u32 divisor = (GAME_GRID_WIDTH - 1) | 0xF;

  for (i = 0; i < character->player.currentWorld->level + 1; ++i)
  {
    if (i < 7)
    {
      int roll = rand();

      // Initialize the sprite
      sprite_initialize(&creatures[i], world);

      if ((roll % 100) < 50)
      {
        creatures[i].tile.color = COLOR_BROWN;
        creatures[i].tile.flags = IS_VISIBLE | IS_CREATURE | NO_FLIP;
        creatures[i].tile.tileNum = CREATURE_GIANT_BUG;
        creatures[i].stats.ac = 15;
        creatures[i].stats.hp = 9 + i;
        creatures[i].stats.str = 16;
        creatures[i].stats.dex = 12;
        creatures[i].stats.intel = 4;
        creatures[i].stats.con = 10;
        creatures[i].stats.wis = 3;
        creatures[i].stats.flags = AGGRESSIVE | VISION | TRACKING;
        creatures[i].name = "Giant Beetle";
      }
      else
      {
        creatures[i].tile.color = COLOR_RED;
        creatures[i].tile.flags = IS_VISIBLE | IS_CREATURE;
        creatures[i].tile.tileNum = CREATURE_BAT;
        creatures[i].stats.ac = 12;
        creatures[i].stats.hp = 6 + i;
        creatures[i].stats.str = 8;
        creatures[i].stats.dex = 16;
        creatures[i].stats.intel = 4;
        creatures[i].stats.con = 10;
        creatures[i].stats.wis = 3;
        creatures[i].stats.flags = CAUTIOUS | VISION | FLYING;
        creatures[i].name = "Bat";
      }
    }
    else if (i < 16)
    {
      int roll = rand();

      if ((roll % 100) < 50)
      {
        creatures[i].tile.color = COLOR_ORANGERED;
        creatures[i].tile.flags = IS_CREATURE | NO_FLIP;
        creatures[i].tile.tileNum = CREATURE_LAND_SQUID;
        creatures[i].stats.ac = 12;
        creatures[i].stats.hp = 10 + i;
        creatures[i].stats.str = 16;
        creatures[i].stats.dex = 9;
        creatures[i].stats.intel = 4;
        creatures[i].stats.con = 10;
        creatures[i].stats.wis = 3;
        creatures[i].stats.flags = AGGRESSIVE | AMBUSH | VISION;
        creatures[i].name = "Land Squid";
      }
      else
      {
        creatures[i].tile.color = COLOR_WHITE;
        creatures[i].tile.flags = IS_CREATURE;
        creatures[i].tile.tileNum = CREATURE_SHARK;
        creatures[i].stats.ac = 14;
        creatures[i].stats.hp = 15 + i;
        creatures[i].stats.str = 18;
        creatures[i].stats.dex = 16;
        creatures[i].stats.intel = 4;
        creatures[i].stats.con = 10;
        creatures[i].stats.wis = 3;
        creatures[i].stats.flags = AGGRESSIVE | AMBUSH | TRACKING;
        creatures[i].name = "White Shark";
      }
    }
    else
    {
      int roll = rand();

      if ((roll % 100) < 45)
      {
        creatures[i].tile.color = COLOR_GOLDEN;
        creatures[i].tile.flags = IS_VISIBLE | IS_CREATURE |
                                         NO_FLIP;
        creatures[i].tile.tileNum = CREATURE_MANTICORE;
        creatures[i].stats.ac = 20;
        creatures[i].stats.hp = i;
        creatures[i].stats.str = 16;
        creatures[i].stats.dex = 16;
        creatures[i].stats.intel = 4;
        creatures[i].stats.con = 10;
        creatures[i].stats.wis = 3;
        creatures[i].stats.flags = AGGRESSIVE | FLYING | VISION|AMBUSH;
        creatures[i].name = "Manticore";
      }
      else if ((roll % 100) < 90)
      {
        creatures[i].tile.color = COLOR_MAGENTA;
        creatures[i].tile.flags = IS_VISIBLE | IS_CREATURE |
                                         NO_FLIP;
        creatures[i].tile.tileNum = CREATURE_GIANT_RAT;
        creatures[i].stats.ac = 14;
        creatures[i].stats.hp = 15 + i;
        creatures[i].stats.str = 16;
        creatures[i].stats.dex = 16;
        creatures[i].stats.intel = 4;
        creatures[i].stats.con = 10;
        creatures[i].stats.wis = 3;
        creatures[i].stats.flags = AGGRESSIVE | VISION;
        creatures[i].name = "Giant Rat";
      }
      else
      {
        creatures[i].tile.color = COLOR_YELLOW;
        creatures[i].tile.flags = IS_VISIBLE | IS_CREATURE;
        creatures[i].tile.tileNum = CREATURE_DRAGON_FLY;
        creatures[i].stats.ac = 24;
        creatures[i].stats.hp = 25 + i;
        creatures[i].stats.str = 18;
        creatures[i].stats.dex = 18;
        creatures[i].stats.intel = 10;
        creatures[i].stats.con = 18;
        creatures[i].stats.wis = 13;
        creatures[i].stats.flags = AGGRESSIVE | FLYING | VISION |
                                   TRACKING;
        creatures[i].name = "Dragon Wasp";
      }

    }

    // Loop to find a random starting place on the game grid
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
      if (middleEarth[randX + randY * GAME_GRID_WIDTH].tileNum ==
                                                     TERRAIN_MOUNTAINS)
      {
        if (creatures[i].tile.tileNum == CREATURE_LAND_SQUID)
          continue;
      }

      if (middleEarth[randX + randY * GAME_GRID_WIDTH].
                                              tileNum == TERRAIN_WATER)
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

    // Initialize the creature sprite and append it to the game grid
    creatures[i].locationX = randX;
    creatures[i].locationY = randY;
    ListAppend(&creatures[i], GameGridTile(randX, randY));
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
    if ((middleEarth[randX + randY * GAME_GRID_WIDTH].tileNum ==
         TERRAIN_MOUNTAINS) || (middleEarth[randX + randY *
                        GAME_GRID_WIDTH].tileNum == TERRAIN_WATER))
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
  ExitTile.bitmap = TerrainCharactersData;
  ExitTile.color = COLOR_MAGENTA;
  ExitTile.flags = IS_VISIBLE;
  ExitTile.list.next = NULL;
  ExitTile.list.previous = NULL;
  ExitTile.tileNum = STRUCTURE_DOOR_OPEN;
  middleEarth[randX + randY * GAME_GRID_WIDTH].flags |= WORLD_EXIT;
  ListAppend(&ExitTile, &middleEarth[randX + randY * GAME_GRID_WIDTH]);

  // Display the character in a random location
  for (;;)
  {
    randX = rand() & divisor;
    if (randX > GAME_GRID_WIDTH - 1) // Needed for 28 (640x480)
      randX -= 5;
    randY = rand() & divisor;
    if (randY > GAME_GRID_HEIGHT - 1)
      randY -= 5;
    if ((middleEarth[randX + randY * GAME_GRID_WIDTH].
                                   tileNum == TERRAIN_MOUNTAINS) ||
        (middleEarth[randX + randY * GAME_GRID_WIDTH].
                                         tileNum == TERRAIN_WATER))
      continue;
    else
      break;
  }
  character->player.currentWorld = world;
  character->player.locationX = randX;
  character->player.locationY = randY;
  ListAppend(character, &middleEarth[randX + randY *GAME_GRID_WIDTH]);

  // Look around
  player_look(character);

  // Display the character details on top left of screen
  player_display(character);
}

/*...................................................................*/
/* game_level_up: Transition game to new level                       */
/*                                                                   */
/*   Input: characterTile is void pointer to PlayerTile              */
/*...................................................................*/
static void game_level_up(PlayerTile *characterTile)
{
  World *world = characterTile->player.currentWorld;
  int num_creatures;

  // Clear the screen first
  ScreenClear();

  // Reseed the PRNG
  srand(TimerNow() ^ ('L' +characterTile->player.currentWorld->score));

  // The portal advances levels
  if (characterTile->player.currentWorld->level < MAX_CREATURES - 1)
    ++characterTile->player.currentWorld->level;

  num_creatures = characterTile->player.currentWorld->level + 1;

  // Increase creatures per level depending on game grid size
  if (num_creatures > GAME_GRID_WIDTH)
    world_create_random(world, WORLD_WEST_COAST | WORLD_NORTH_COAST |
             WORLD_EAST_COAST | WORLD_SOUTH_COAST | WORLD_MOUNTAINOUS);
  else if (num_creatures > GAME_GRID_WIDTH -
           (GAME_GRID_WIDTH / 4))
    world_create_random(world, WORLD_NORTH_COAST | WORLD_WEST_COAST |
                        WORLD_MOUNTAINOUS);
  else if (num_creatures > GAME_GRID_WIDTH / 2)
    world_create_random(world, WORLD_NORTH_COAST | WORLD_WEST_COAST);
  else if (num_creatures > GAME_GRID_WIDTH / 8)
    world_create_random(world, WORLD_NORTH_COAST);
  else
    world_create_random(world, 0);

  // Start the virtual world game
  game_start(characterTile, TheCreatures, world);
}

/*...................................................................*/
/* player_move: Move the player sprite to a new game grid location   */
/*                                                                   */
/*   Input: characteTiler is pointer to PlayerTile                   */
/*          tileX is the players new tile X position                 */
/*          tileY is the players new tile Y position                 */
/*                                                                   */
/*  Return: Zero (0) on success, -1 on failure                       */
/*...................................................................*/
static int player_move(PlayerTile *characterTile, int tileX, int tileY)
{
  Tile *theTerrain = GameGridTile(tileX, tileY);

  if (theTerrain->tileNum == TERRAIN_MOUNTAINS)
  {
    int i;

    for (i = 0; i < characterTile->num_items; ++i)
      if (characterTile->items[i].tileNum == ITEM_MAGIC_BOOTS)
        break;

    if (!i || (i == characterTile->num_items))
    {
      puts("Mountains impassible");
      return -1;
    }
  }
  if (theTerrain->tileNum == TERRAIN_WATER)
  {
    int i;

    for (i = 0; i < characterTile->num_items; ++i)
      if (characterTile->items[i].tileNum == ITEM_MAGIC_BELT)
        break;

    if (!i || (i == characterTile->num_items))
    {
      puts("Water impassible");
      return -1;
    }
    else
      puts("Belt of Swimming");
  }

  // Move player sprite to the new game grid location if valid
  if (theTerrain)
  {
    // Remove the sprite from the previous game grid tile
    ListRemove(characterTile->player.tile.list);
    TileDisplayGrid(characterTile->player.locationX,
                    characterTile->player.locationY);

    // Append the player to the new terrain base tile
    characterTile->player.locationX = tileX;
    characterTile->player.locationY = tileY;
    ListAppend(characterTile, theTerrain);

    // Display new location and look around
    TileDisplayGrid(tileX, tileY);
    player_look(characterTile);

    // If this is a world exit, level up
    if (theTerrain->flags & WORLD_EXIT)
      game_level_up(characterTile);
  }
  else
    puts("player destination not found, player lost!");
  return 0;
}

/*...................................................................*/
/* player_create_random: Create a random player character            */
/*                                                                   */
/*   Input: character is pointer to PlayerTile                       */
/*...................................................................*/
static void player_create_random(PlayerTile *character, World *world)
{
  int d6roll;

  bzero(character, sizeof(PlayerTile));

  // Down cast and initialize the sprite
  sprite_initialize((SpriteTile *)character, world);

  // Default adventurer is aggresive with keen vision
  character->player.stats.flags = AGGRESSIVE | VISION;
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
  else if ((character->player.stats.wis >
            character->player.stats.dex) &&
           (character->player.stats.wis >
            character->player.stats.intel) &&
           (character->player.stats.wis > character->player.stats.str))
  {
    character->player.stats.flags |= CLASS_CLERIC;
    character->player.stats.hp = 8;
    character->player.tile.color = COLOR_RED;
    character->player.tile.tileNum = CHARACTER_CLERIC;
  }

  // If dexterity highest, be a scout
  else if ((character->player.stats.dex >
            character->player.stats.wis) &&
           (character->player.stats.dex >
            character->player.stats.intel) &&
           (character->player.stats.dex > character->player.stats.str))
  {
    character->player.stats.flags |= CLASS_SCOUT | TRACKING;
    character->player.stats.hp = 8;
    character->player.tile.color = COLOR_RED;
    character->player.tile.tileNum = CHARACTER_SCOUT;
  }

  // Otherwise be a ranger
  else
  {
    character->player.stats.flags |= CLASS_RANGER | TRACKING;
    character->player.tile.color = COLOR_RED;
    character->player.tile.tileNum = CHARACTER_BARBARIAN;
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

  // Configure for 1st level, no experience
  character->player.currentWorld->level = 1;
  character->player.currentWorld->score = 0;

  character->player.name = "xlawless";
  character->player.name[0] = rand() % 32 + 'A';

  character->player.tile.bitmap = TerrainCharactersData;
  character->player.tile.flags = IS_PLAYER | IS_VISIBLE|NO_FLIP;
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
    GameUp = FALSE;
  }

  // Otherwise start the game by starting the timer
  else
  {
    puts("Game resumed");
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
    StdioState = &ConsoleState;

    // Clear the screen first
    ScreenClear();

    // Seed the PRNG
    srand(TimerNow() ^ 'G');

    TheWorld.tiles = (void *)GameGrid;
    TheWorld.x = GAME_GRID_WIDTH;
    TheWorld.y = GAME_GRID_HEIGHT;
    TheWorld.level = 0;
    TheWorld.score = 0;

    // Create the world, player character and start the game
    world_create_random(&TheWorld, 0);
    player_create_random(&TheCharacter, &TheWorld);
    game_start(&TheCharacter, TheCreatures, &TheWorld);

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
    puts("No action yet");
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

    player_move(&TheCharacter, TheCharacter.player.locationX +1,
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

    player_move(&TheCharacter, TheCharacter.player.locationX -1,
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

    player_move(&TheCharacter, TheCharacter.player.locationX +1,
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

    player_move(&TheCharacter, TheCharacter.player.locationX -1,
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
