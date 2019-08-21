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

typedef struct
{
  EffectTile effect;
  int affect; // Type or details of action
  int range;  // Distance, in tiles, action can be performed
  const char *name;
} ActionTile;

typedef struct {
  ActionTile first; // Primary action
  ActionTile second;// Secondary action
  u32 flags;        // Flag bits for behavior, state, etc.
  int hp;           // Health Points
  u8 str, dex, con, intel, wis; // Optional abilities
                                // Can be used with PRNG to determine
                                // if actions are successful
  u8 ac, scratch; // Optional Armour Class and game specific
} SpriteStats;

typedef struct
{
  EffectTile effect;
  SpriteStats stats;
  World *currentWorld;
  char *name;
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
  // Default effect tile initialization
  sprite->stats.first.effect.tile.color = COLOR_RED;
  sprite->stats.first.effect.tile.tileNum = ITEM_GAUNTLETS;
  sprite->stats.first.effect.tile.bitmap = EquipmentData;
  sprite->stats.first.effect.tile.flags = IS_VISIBLE;
  sprite->stats.first.effect.tile.list.next = NULL;
  sprite->stats.first.effect.tile.list.previous = NULL;
  sprite->stats.first.effect.count = 0;
  sprite->stats.first.effect.total = 0;
  sprite->stats.first.effect.locationX = 0;
  sprite->stats.first.effect.destinationX = 0;
  sprite->stats.second.effect.tile.color = COLOR_RED;
  sprite->stats.second.effect.tile.tileNum = ITEM_DART;
  sprite->stats.second.effect.tile.bitmap = EquipmentData;
  sprite->stats.second.effect.tile.flags = IS_VISIBLE;
  sprite->stats.second.effect.tile.list.next = NULL;
  sprite->stats.second.effect.tile.list.previous = NULL;
  sprite->stats.second.effect.count = 0;
  sprite->stats.second.effect.total = 0;
  sprite->stats.second.effect.locationX = 0;
  sprite->stats.second.effect.destinationX = 0;

  // Assign world and bitmap data and initialize location
  sprite->currentWorld = world;
  sprite->effect.tile.bitmap = CreaturesData;
  sprite->stats.scratch = 0;
  sprite->effect.locationX = sprite->effect.destinationX = 0;
  sprite->effect.locationY = sprite->effect.destinationY = 0;
  sprite->effect.count = 0;
  sprite->effect.tile.list.next = NULL;
  sprite->effect.tile.list.previous = NULL;
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
  int x = character->player.effect.locationX,
      y = character->player.effect.locationY;
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
/* attack_range: Determine range and return action that can reach    */
/*                                                                   */
/*   Input: creatureTile is the attacking creature                   */
/*          nearbyCreatureTile is the creature being attacked        */
/*                                                                   */
/*  Return: Pointer to ActionTile that can reach or NULL             */
/*...................................................................*/
static ActionTile *attack_range(SpriteTile *creatureTile,
                                SpriteTile *nearbyCreatureTile)
{
  Tile *creatureTerrain = GameGridTile(creatureTile->effect.locationX,
                                       creatureTile->effect.locationY);
  Tile *nearbyCreatureTerrain = GameGridTile(
                                 nearbyCreatureTile->effect.locationX,
                                 nearbyCreatureTile->effect.locationY);
  int deltaX, deltaY;

  if (creatureTile->effect.locationX >
      nearbyCreatureTile->effect.locationX)
    deltaX = creatureTile->effect.locationX -
             nearbyCreatureTile->effect.locationX;
  else
    deltaX = nearbyCreatureTile->effect.locationX -
             creatureTile->effect.locationX;

  if (creatureTile->effect.locationY >
      nearbyCreatureTile->effect.locationY)
    deltaY = creatureTile->effect.locationY -
             nearbyCreatureTile->effect.locationY;
  else
    deltaY = nearbyCreatureTile->effect.locationY -
             creatureTile->effect.locationY;

  // If more that one space away then check ranged attacks
  if ((deltaX > 1) || (deltaY > 1))
  {
    // If a ranged weapon is present
    if (creatureTile->stats.second.affect > 0)
    {
      if (!(creatureTile->stats.flags & FLYING))
      {
        // Ensure player is not trying to attack from the water
        if (creatureTerrain->tileNum == TERRAIN_WATER)
        {
          if (creatureTile->effect.tile.flags & IS_PLAYER)
            puts("Cannot fire while Swimming");
          return NULL;
        }

        // Ensure player is not trying to attack into the mountains
        if (nearbyCreatureTerrain->tileNum == TERRAIN_MOUNTAINS)
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
      PlayerTile *characterTile = &TheCharacter;

      if (creatureTerrain->tileNum == TERRAIN_WATER)
      {
        puts("Cannot attack while Swimming");
        return NULL;
      }

      // Ensure player is not trying to attack into the mountains
      if (nearbyCreatureTerrain->tileNum == TERRAIN_MOUNTAINS)
      {
        int i;

        for (i = 0; i < characterTile->num_items; ++i)
          if (characterTile->items[i].tileNum == ITEM_MAGIC_BOOTS)
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

    // Return the default short range melee weapon
    return &creatureTile->stats.first;
  }

  // Otherwise return to attack within range
  else
    return NULL;
}

/*...................................................................*/
/* sprite_move_callback: Timer callback for sprite move effect       */
/*                                                                   */
/*   Input: id is unused                                             */
/*          data is the ActionTile pointer                           */
/*          context is the game grid Tile pointer                    */
/*                                                                   */
/*  Return: TASK_FINISHED                                            */
/*...................................................................*/
int sprite_move_callback(u32 id, void *data, void *context)
{
  SpriteTile *spriteTile = data;

  StdioState = &ConsoleState;

  // If effect active perform the next effect and update count
  if (spriteTile->effect.count)
    TileEffectNext((EffectTile *)spriteTile);

  // If effect still active, schedule the next effect
  if (spriteTile->effect.count)
  {
    TimerSchedule(spriteTile->effect.period,
                  sprite_move_callback, spriteTile, context);
  }
  else
  {
    Tile *currentTile;

    // Remove the sprite from the previous game grid tile
    ListRemove(spriteTile->effect.tile.list);

    // Append sprite to the end of the new tile list
    currentTile = GameGridTile(spriteTile->effect.locationX,
                               spriteTile->effect.locationY);
    if (currentTile)
      ListAppend(spriteTile, currentTile);
    else
      puts("sprite destination not found, sprite lost!");
  }

  return TASK_FINISHED;
}

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

  // Silent return if effect is already in progress
  if ((creatureTile->effect.locationX !=
      creatureTile->effect.destinationX) ||
      (creatureTile->effect.count))
    return -1;

  // Flying creatures can go anywhere, so only check non-flyers
  if (!(creatureTile->stats.flags & FLYING))
  {
    Tile *newTile = GameGridTile(tileX, tileY);

    if (newTile == NULL)
      return -1;

    // Land squids can't go in the mountains
    if (newTile->tileNum == TERRAIN_MOUNTAINS)
    {
      if (creatureTile->effect.tile.tileNum == CREATURE_LAND_SQUID)
        return -1;
    }

    // Giant bugs cannot swim
    if (newTile->tileNum == TERRAIN_WATER)
    {
      if (creatureTile->effect.tile.tileNum == CREATURE_GIANT_BUG)
        return -1;
    }

    // And sharks cannot leave the water
    else if (creatureTile->effect.tile.tileNum == CREATURE_SHARK)
        return -1;

  }

  // Hide creature and redraw original tile
  creatureTile->effect.tile.flags &= ~IS_VISIBLE;
  TileDisplayGrid(creatureTile->effect.locationX,
                  creatureTile->effect.locationY);

  // Make moving creatures visible
  creatureTile->effect.tile.flags |= IS_VISIBLE;

  // Configure sprite movement effect
  // Total time of effect is effect.total * effect.period

  // Initialize the move effect
  creatureTile->effect.total = 4; // 1 or power of 2
  creatureTile->effect.period = MICROS_PER_SECOND / 4;

  // Assign the effect callback function invoked every period
  creatureTile->effect.poll = sprite_move_callback;

  // Start the move player effect tile
  return TileEffectStart(&creatureTile->effect, tileX, tileY);
}

/*...................................................................*/
/* attack_callback: Timer invoked callback for attack effect         */
/*                                                                   */
/*   Input: id is unused                                             */
/*          data is the ActionTile pointer                           */
/*          context is the game grid Tile pointer                    */
/*                                                                   */
/*  Return: Zero (0) on success, -1 if error                         */
/*...................................................................*/
int attack_callback(u32 id, void *data, void *context)
{
  ActionTile *actionTile = data;

  StdioState = &ConsoleState;

  // If effect active perform the next effect and update count
  if (actionTile->effect.count)
    TileEffectNext((EffectTile *)actionTile);

  // If effect still active, schedule the next effect
  if (actionTile->effect.count)
  {
    TimerSchedule(actionTile->effect.period,
                  attack_callback, actionTile, context);
    return TASK_FINISHED;
  }

  // Final effect so if destination visible, redraw it
  TileDisplayGrid(actionTile->effect.destinationX,
                  actionTile->effect.destinationY);

  return TASK_FINISHED;
}

/*...................................................................*/
/* defeat_creature: Player has defeated a creature                   */
/*                                                                   */
/*   Input: creatureTile is pointer to attacking creature/sprite     */
/*          attackedCreatureTile is the creature being attacked      */
/*...................................................................*/
static void defeat_creature(PlayerTile *characterTile,
                            SpriteTile *attackedCreatureTile)
{
  Tile *currentTile;
  u32 d16roll;

  // Increase character experience
  characterTile->player.currentWorld->score += 1;

  // Roll 16 sided dice
  d16roll = rand() % 16;

  // Check if an item is dropped (little more than third)
  if (d16roll > 5)
  {
    puts("Treasure dropped!");

    // Drop a random item

    // Re-roll, equipment is 0 through 15
    d16roll = (rand() % 16);

    //Give out potions most often
    if (d16roll < 8)
      d16roll = 0;

    // Re-role and morph roll to denzi_data.h
    else if ((d16roll = (rand() % 16)) == 4)
      d16roll += 1;
    else if (d16roll == 5)
      d16roll += 2;
    else  if (d16roll == 6)
      d16roll = ITEM_HELMET;
    else if (d16roll == 7)
      d16roll = ITEM_CHAINMAIL;
    else if (d16roll >= 8)
    {
      // Do not release rare items until fifth level
      if (characterTile->player.currentWorld->score <= 5)
      {
        d16roll -= 8;
      }
      else
      {
        // Reduce top four items until 10th level
        if ((characterTile->player.currentWorld->score < 10) &&
            (d16roll > 8 + 3))
          d16roll -= 4;

        // Assign the tile item to replace the die roll
        if (d16roll == 8)
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
    }

    // Convert defeated creature into equipment item on terrain
    attackedCreatureTile->effect.tile.color = COLOR_WHITE;
    attackedCreatureTile->effect.tile.tileNum = d16roll;
    attackedCreatureTile->effect.tile.bitmap = EquipmentData;
    attackedCreatureTile->effect.tile.flags = IS_ITEM | IS_VISIBLE;
    attackedCreatureTile->effect.tile.list.previous = NULL;
    attackedCreatureTile->effect.tile.list.next = NULL;

    // Add to the end of the list
    currentTile = GameGridTile(attackedCreatureTile->effect.locationX,
                              attackedCreatureTile->effect.locationY);
    if (currentTile)
      ListAppend(attackedCreatureTile, currentTile);

    // Display dropped item tile using attackedCreatureTile's tile
    TileDisplayScreen((Tile *)attackedCreatureTile, GAME_GRID_START_X +
                 (attackedCreatureTile->effect.locationX * TILE_WIDTH),
               GAME_GRID_START_Y +
               (attackedCreatureTile->effect.locationY * TILE_HEIGHT));
  }
  puts("Creature defeated!");

  // Update XP on display
  player_display(&TheCharacter);
}

/*...................................................................*/
/* creature_attack: One creature attacks another                     */
/*                                                                   */
/*   Input: creatureTile is pointer to attacking creature/sprite     */
/*          attack the type of attack action                         */
/*          attackedCreatureTile is the creature being attacked      */
/*...................................................................*/
static void creature_attack(SpriteTile *creatureTile,
                            ActionTile *attackTile,
                            SpriteTile *attackedCreatureTile)
{
  // Roll virtual dice (sample PRNG)
  int d20roll, modifier = 0;

  // Silent return if attack effect is already in progress
  if (attackTile->effect.count)
    return;

  // Simulate 20 side dice roll with 8 bits worth or randomness
  d20roll = (rand() % 31);
  if (d20roll == 0)
    d20roll = 20;

  // If overflow, trim the value
  if (d20roll > 20)
    d20roll -= d20roll - 20;

  // If an ambush, give advantage to the roll
  if (!(creatureTile->effect.tile.flags & IS_VISIBLE))
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
  if (attackTile->range > 1)
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
      modifier = 2 + (TheCharacter.player.currentWorld->score >> 3) +
                 ((creatureTile->stats.intel - 10) >> 1);

    else if (TheCharacter.player.stats.flags & CLASS_RANGER)
      modifier += 2 + (TheCharacter.player.currentWorld->score >> 3);

    // Cleric has no base proficiency but experience
    else if (TheCharacter.player.stats.flags & CLASS_CLERIC)
      modifier += (TheCharacter.player.currentWorld->score >> 3);

    // Scout has proficiency and uses dexterity for proficiency
    else if (TheCharacter.player.stats.flags & CLASS_SCOUT)
    {
      if (creatureTile->stats.dex > 10)
        modifier = 2 + (TheCharacter.player.currentWorld->score >> 3) +
                 ((creatureTile->stats.dex - 10) >> 1);
      else
        modifier = 2 + (TheCharacter.player.currentWorld->score >> 3);
    }
  }

  // If character involved, display creature name at start of attack
  if ((creatureTile == &TheCharacter.player) ||
      (attackedCreatureTile == &TheCharacter.player))
    puts(creatureTile->name);

  // Set the color of the action tile to hit (red) or miss (blue)
  if (d20roll + modifier >= attackedCreatureTile->stats.ac)
    attackTile->effect.tile.color = COLOR_RED;
  else
    attackTile->effect.tile.color = COLOR_BLUE;

  // Initialize the move effect
  attackTile->effect.total = 4; // 1 or power of 2
  attackTile->effect.period = MICROS_PER_SECOND / 16;
  attackTile->effect.locationX = creatureTile->effect.locationX;
  attackTile->effect.locationY = creatureTile->effect.locationY;

  // Assign the effect callback function invoked every period
  attackTile->effect.poll = attack_callback;
  attackTile->effect.tile.list.next = NULL;

  // Add creature as previous tile that launched effect
  attackTile->effect.tile.list.previous = (void *)creatureTile;

  // Start the move player effect tile
  TileEffectStart((EffectTile *)attackTile,
                  attackedCreatureTile->effect.locationX,
                  attackedCreatureTile->effect.locationY);

  // Compare the modified roll to the attackedCreatureTile armour class
  if (d20roll + modifier >= attackedCreatureTile->stats.ac)
  {
    int i, damage;

    // Loop for each d6 roll for the attack
    for (i = 0, damage = 0; i < attackTile->affect; ++i)
      damage += (rand() % 6) + 1;

    // If an ambush, double the damage
    if (!(creatureTile->effect.tile.flags & IS_VISIBLE))
    {
      for (i = 0; i < attackTile->affect; ++i)
        damage += (rand() % 6) + 1;

      putbyte(damage);

      // Add the visible flag as it is one time use
      creatureTile->effect.tile.flags |= IS_VISIBLE;
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
      int x = attackedCreatureTile->effect.locationX,
          y = attackedCreatureTile->effect.locationY;

      // Unlink this creature from the game grid tile
      ListRemove(attackedCreatureTile->effect.tile.list);

      // Flag creature as dead
      attackedCreatureTile->effect.tile.flags |= IS_DEAD;

      // Display the terrain again
      TileDisplayGrid(x, y);

      // If the attacking creature is the character
      if (creatureTile->effect.tile.flags & IS_PLAYER)
      {
        if (creatureTile != &TheCharacter.player)
          puts("ERROR: Creature is not player but has flag set!");

        // Update character and check if creature dropped anything
        defeat_creature(&TheCharacter, attackedCreatureTile);
      }
    }

    // If the attacked creature is the character, update HP
    if (attackedCreatureTile->effect.tile.flags & IS_PLAYER)
      player_display(&TheCharacter);
  }
  else
  {
    // If character involved, display attack missed
    if ((creatureTile->effect.tile.flags & IS_PLAYER) ||
        (attackedCreatureTile->effect.tile.flags & IS_PLAYER))
      puts("missed!");
  }

  // Make attacking sprites visible
  creatureTile->effect.tile.flags |= IS_VISIBLE;
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
  Tile *theTerrain;
  ActionTile *attack = attack_range(creatureTile, nearestCreatureTile);
  int x, y;

  // Allow creatures to hide in towns
  theTerrain = creatureTile->currentWorld->tiles;
  if (theTerrain[nearestCreatureTile->effect.locationX +
            (nearestCreatureTile->effect.locationY * GAME_GRID_WIDTH)].
                                             tileNum == STRUCTURE_TOWN)
    return FALSE;

  // Determine x offset toward distantCreature
  if (creatureTile->effect.locationX <
      nearestCreatureTile->effect.locationX)
    x = 1;
  else if (creatureTile->effect.locationX ==
           nearestCreatureTile->effect.locationX)
    x = 0;
  else
    x = -1;

  // Determine y offset toward distantCreature
  if (creatureTile->effect.locationY <
      nearestCreatureTile->effect.locationY)
    y = 1;
  else if (creatureTile->effect.locationY ==
           nearestCreatureTile->effect.locationY)
    y = 0;
  else
    y = -1;

  // If creature is aggressive, attack or pursue
  if (creatureTile->stats.flags & AGGRESSIVE)
  {
    if (attack)
      creature_attack(creatureTile, attack, nearestCreatureTile);
    else
      sprite_move(creatureTile, creatureTile->effect.locationX + x,
                  creatureTile->effect.locationY + y);
    return TRUE;
  }

  // Otherwise if cautious, attack if in range or move away
  else if (creatureTile->stats.flags & CAUTIOUS)
  {
    if (attack)
      creature_attack(creatureTile, attack, nearestCreatureTile);
    else
      sprite_move(creatureTile, creatureTile->effect.locationX - x,
                  creatureTile->effect.locationY - y);
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
  if ((creatureTile->effect.locationX < 0) ||
      (creatureTile->effect.locationY < 0) ||
      (creatureTile->stats.flags & IS_DEAD) ||
      (creatureTile->stats.hp <= 0))
    return;

  // Generate randX and randY bits, zero or one
  random = rand();
  randX = ((random & 0x0F) < 8) ? 1 : 0;
  if (random < 0)
    randX = -randX;

  random = rand();
  randY = ((random & 0x0F) < 8) ? 1 : 0;
  if (random < 0)
    randY = -randY;

  // Move the creature randomly
  if (randX || randY)
  {
    if ((creatureTile->effect.locationX + randX >
         (GAME_GRID_WIDTH - 1)) ||
        (creatureTile->effect.locationX + randX < 0))
      randX = -randX;

    if ((creatureTile->effect.locationY + randY >
         (GAME_GRID_HEIGHT - 1)) ||
        (creatureTile->effect.locationY + randY < 0))
      randY = -randY;

    sprite_move(creatureTile, creatureTile->effect.locationX + randX,
                  creatureTile->effect.locationY + randY);
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
  Tile *theTerrain;

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
    startX = creatureTile->effect.locationX - curr_distance;
    if (startX < 0)
      startX = 0;
    endX = creatureTile->effect.locationX + curr_distance;
    if (endX >= GAME_GRID_WIDTH)
      endX = GAME_GRID_WIDTH - 1;

    startY = creatureTile->effect.locationY - curr_distance;
    if (startY < 0)
      startY = 0;
    endY = creatureTile->effect.locationY + curr_distance;
    if (endY >= GAME_GRID_HEIGHT)
      endY = GAME_GRID_HEIGHT - 1;

    for (y = startY; y <= endY; ++y)
    {
      for (x = startX; x <= endX; ++x)
      {
        // Find the specific terrain tile for this location
        theTerrain = creatureTile->currentWorld->tiles;
        theTerrain = &(theTerrain[x + y * GAME_GRID_WIDTH]);

        // Check terrain tile for creatures (sprite)
        if (theTerrain->list.next)
        {
          Tile *stackedTile;

          for (stackedTile = (void *)theTerrain->list.next;
               stackedTile && (stackedTile != theTerrain);
               stackedTile = (void *)stackedTile->list.next)
          {
            if ((stackedTile != (Tile *)creatureTile) &&
                (stackedTile->flags & IS_VISIBLE) &&
                (stackedTile->flags & (IS_CREATURE | IS_PLAYER)))
              return (SpriteTile *)stackedTile;
          }
        }
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
  for (i = 0; i < characterTile->player.currentWorld->level + 1; ++i)
  {
    if (!(creatureTiles[i].effect.tile.flags & IS_DEAD))
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
              creatureTiles[i].effect.tile.flags &= ~IS_VISIBLE;

              // Redraw tile after sprite hid
              TileDisplayGrid(creatureTiles[i].effect.locationX,
                              creatureTiles[i].effect.locationY);
            }
            else if (creatureTiles[i].effect.tile.flags & IS_VISIBLE)
              ++creatureTiles[i].stats.scratch;
          }
          else
            sprite_move_random(&creatureTiles[i]);
        }
      }
      else
        creatureTiles[i].effect.tile.flags |= IS_DEAD;
    }
  }

  // Perform an action for the character
  if (characterTile)
  {
    // The character must be conscious to act
    if (characterTile->player.stats.hp > 0)
    {
      // If cleric, check to see if no movement long enough to heal
      if (characterTile->player.stats.flags & CLASS_CLERIC)
      {
        if (characterTile->player.stats.hp > maxHp)
          maxHp = characterTile->player.stats.hp;

        if ((lastX == characterTile->player.effect.locationX) &&
            (lastY == characterTile->player.effect.locationY))
          ++creatureTiles[i].stats.scratch;
        else
          creatureTiles[i].stats.scratch = 0;

        if ((characterTile->player.stats.hp < maxHp) &&
            (creatureTiles[i].stats.scratch > 4))
        {
          // Roll healing and add proficiency
          int hp = (rand() % 8) + 2 +
                   (characterTile->player.currentWorld->score >> 4);

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

        lastX = characterTile->player.effect.locationX;
        lastY = characterTile->player.effect.locationY;
      }

      // If scout check to see if we stay still long enough to hide
      else if (characterTile->player.stats.flags & CLASS_SCOUT)
      {
        if ((lastX == characterTile->player.effect.locationX) &&
            (lastY == characterTile->player.effect.locationY))
          ++characterTile->player.stats.scratch;
        else
        {
          characterTile->player.stats.scratch = 0;
          characterTile->player.effect.tile.flags |= IS_VISIBLE;
        }
        if (characterTile->player.stats.scratch > 3)
        {
          if (characterTile->player.effect.tile.flags & IS_VISIBLE)
            puts("Hiding");
          characterTile->player.effect.tile.flags &= ~IS_VISIBLE;
        }

        lastX = characterTile->player.effect.locationX;
        lastY = characterTile->player.effect.locationY;
      }
    }
    else
    {
      puts("You died, game over");
      TimerCancel(TheWorld.round);
      TheWorld.round = NULL;
      return TASK_FINISHED;
    }
  }

  // Schedule next game round and return finished
  TheWorld.round = TimerSchedule(TheWorld.period, game_round,
                                 &TheCharacter, TheCreatures);
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
        creatures[i].effect.tile.color = COLOR_BROWN;
        creatures[i].effect.tile.flags = IS_VISIBLE | IS_CREATURE |
                                         NO_FLIP;
        creatures[i].effect.tile.tileNum = CREATURE_GIANT_BUG;
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
        creatures[i].stats.first.name = "claw";
        creatures[i].stats.second.affect = 0;
        creatures[i].stats.second.range = 0;
        creatures[i].stats.second.name = "none";
        creatures[i].name = "Giant Beetle";
      }
      else
      {
        creatures[i].effect.tile.color = COLOR_RED;
        creatures[i].effect.tile.flags = IS_VISIBLE | IS_CREATURE;
        creatures[i].effect.tile.tileNum = CREATURE_BAT;
        creatures[i].stats.first.effect.tile.tileNum = ITEM_SPARKLE_SWORD;
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
        creatures[i].effect.tile.color = COLOR_ORANGERED;
        creatures[i].effect.tile.flags = IS_CREATURE | NO_FLIP;
        creatures[i].effect.tile.tileNum = CREATURE_LAND_SQUID;
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
        creatures[i].effect.tile.color = COLOR_WHITE;
        creatures[i].effect.tile.flags = IS_CREATURE;
        creatures[i].effect.tile.tileNum = CREATURE_SHARK;
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
        creatures[i].effect.tile.color = COLOR_GOLDEN;
        creatures[i].effect.tile.flags = IS_VISIBLE | IS_CREATURE |
                                         NO_FLIP;
        creatures[i].effect.tile.tileNum = CREATURE_MANTICORE;
        creatures[i].stats.ac = 20;
        creatures[i].stats.hp = i;
        creatures[i].stats.str = 16;
        creatures[i].stats.dex = 16;
        creatures[i].stats.intel = 4;
        creatures[i].stats.con = 10;
        creatures[i].stats.wis = 3;
        creatures[i].stats.flags = AGGRESSIVE | FLYING | VISION|AMBUSH;
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
        creatures[i].effect.tile.color = COLOR_MAGENTA;
        creatures[i].effect.tile.flags = IS_VISIBLE | IS_CREATURE |
                                         NO_FLIP;
        creatures[i].effect.tile.tileNum = CREATURE_GIANT_RAT;
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
        creatures[i].effect.tile.color = COLOR_YELLOW;
        creatures[i].effect.tile.flags = IS_VISIBLE | IS_CREATURE;
        creatures[i].effect.tile.tileNum = CREATURE_DRAGON_FLY;
        creatures[i].stats.ac = 24;
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
        if (creatures[i].effect.tile.tileNum == CREATURE_LAND_SQUID)
          continue;
      }

      if (middleEarth[randX + randY * GAME_GRID_WIDTH].
                                              tileNum == TERRAIN_WATER)
      {
        // Sharks can only be in water
        if (creatures[i].effect.tile.tileNum == CREATURE_SHARK)
          break;

        if (creatures[i].effect.tile.tileNum == CREATURE_GIANT_BUG)
          continue;
      }
      else if (creatures[i].effect.tile.tileNum == CREATURE_SHARK)
        continue;

      // Otherwise this location is great so start creature here
      break;
    }

    // Initialize the creature sprite and append it to the game grid
    creatures[i].effect.locationX = creatures[i].effect.destinationX = randX;
    creatures[i].effect.locationY = creatures[i].effect.destinationY = randY;
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
  character->player.effect.locationX = character->player.effect.destinationX = randX;
  character->player.effect.locationY = character->player.effect.destinationY = randY;
  ListAppend(character, &middleEarth[randX + randY *GAME_GRID_WIDTH]);

  // Look around
  player_look(character);

  // Display the character details on top left of screen
  player_display(character);

  // Schedule next Game Tick and return success
  TheWorld.period = gamePeriod;

  // Start the game round timer
  TheWorld.round = TimerSchedule(gamePeriod, game_round, &TheCharacter,
                                 TheCreatures);
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

  // Cancel the old timer if already started
  if (world->round)
  {
    TimerCancel(world->round);
    world->round = NULL;
  }

  // Clear the screen first
  ScreenClear();

  // Reseed the PRNG
  srand(TimerNow() ^ ('L' +characterTile->player.currentWorld->score));

  // The portal advances levels

  // Update wizard stats after going through portal
  if (characterTile->player.stats.flags & CLASS_WIZARD)
  {
    characterTile->player.stats.first.affect = 1 +
      (characterTile->player.currentWorld->score >> 3);

    if (characterTile->player.stats.first.affect > 1)
    {
      characterTile->player.stats.second.affect = 0 +
                                            (characterTile->player.currentWorld->score >> 3);
      characterTile->player.stats.second.range = 3 +
                                            (characterTile->player.currentWorld->score >> 4);

      // Once powerful enough change effect to a ball
      if (characterTile->player.stats.second.affect >= 2)
        characterTile->player.stats.second.effect.tile.tileNum = ITEM_BALL;
      characterTile->player.stats.second.name = "blast";
    }
  }

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
  game_start(characterTile, TheCreatures, world, world->period);
}

/*...................................................................*/
/* player_move_callback: Timer callback for player move effect       */
/*                                                                   */
/*   Input: id is unused                                             */
/*          data is the ActionTile pointer                           */
/*          context is the game grid Tile pointer                    */
/*                                                                   */
/*  Return: TASK_FINISHED                                            */
/*...................................................................*/
int player_move_callback(u32 id, void *data, void *context)
{
  PlayerTile *characterTile = data;
  Tile *destinationTerrain = context;

  StdioState = &ConsoleState;

  if (characterTile->player.effect.count)
    TileEffectNext((EffectTile *)characterTile);

  // Complete the move if this is the last effect
  if (characterTile->player.effect.count == 0)
  {
    Tile *currentTile;

    // Remove the sprite from the previous game grid tile
    ListRemove(characterTile->player.effect.tile.list);

    // Append sprite to the end of the new tile list
    currentTile = GameGridTile(characterTile->player.effect.locationX,
                               characterTile->player.effect.locationY);
    if (currentTile)
      ListAppend(characterTile, currentTile);
    else
      puts("player destination not found, player lost!");

    putchar('l');
    // Look around the players new location
    player_look(characterTile);

    // If character moved to the level up tile, recreate world
    if (destinationTerrain->flags & WORLD_EXIT)
    {
      game_level_up(characterTile);
      return TASK_FINISHED;
    }

    // Character picks up any item if room
    if (destinationTerrain->list.next &&
        (destinationTerrain->list.next != (void *)destinationTerrain)&&
        (characterTile->num_items <= MAX_ITEMS))
    {
      int i;
      Tile *stackedTile;

      // Find the item within the tile list
      for (stackedTile = (void *)destinationTerrain->list.next;
           stackedTile && (stackedTile != destinationTerrain) &&
           !(stackedTile->flags & IS_ITEM);
           stackedTile = (void *)stackedTile->list.next) ;

      // If an item was found (NULL ending or circular) get it
      if ((stackedTile) && (stackedTile != destinationTerrain))
      {
        puts("Item located");

        // Ignore duplicate items
        for (i = 0; i < characterTile->num_items; ++i)
        {
          if (characterTile->items[i].tileNum ==
                                        stackedTile->tileNum)
            break;
        }

        // If not a duplicate item, add it to the character
        if (i == characterTile->num_items)
        {
          if (stackedTile->tileNum == ITEM_POTION)
          {
            // Drink right away
            characterTile->player.stats.hp += 8;
            puts("The potion energizes you.");
          }
          else
          {
            if (stackedTile->tileNum == ITEM_ARMOUR_LEATHER)
              characterTile->player.stats.ac += 1;

            else if (!(characterTile->player.stats.flags &
                                                         CLASS_WIZARD))
            {
              if (stackedTile->tileNum == ITEM_ARMOUR_SHEILD)
                characterTile->player.stats.ac += 2;

              if (!(characterTile->player.stats.flags & CLASS_SCOUT))
              {
                if (stackedTile->tileNum == ITEM_PLATEMAIL)
                  characterTile->player.stats.ac = 18;
                else if (stackedTile->tileNum == ITEM_SPLINTMAIL)
                  characterTile->player.stats.ac = 17;
                else if (stackedTile->tileNum == ITEM_CHAINMAIL)
                  characterTile->player.stats.ac = 16;
                else if (stackedTile->tileNum == ITEM_GAUNTLETS)
                  characterTile->player.stats.ac += 1;
              }

              if (stackedTile->tileNum == ITEM_WEAPON_SWORD)
              {
                characterTile->player.stats.first.affect += 1;
                characterTile->player.stats.first.effect.tile.tileNum =
                                                     ITEM_WEAPON_SWORD;
              }
              else if (stackedTile->tileNum == ITEM_FLAMING_SWORD)
              {
                characterTile->player.stats.first.affect += 1;
                characterTile->player.stats.first.effect.tile.tileNum =
                                                    ITEM_FLAMING_SWORD;
              }
              else if (stackedTile->tileNum == ITEM_POWER_SWORD)
              {
                characterTile->player.stats.first.affect += 1;
                if (characterTile->player.stats.str >= 16)
                  characterTile->player.stats.str += 3;
                else
                  characterTile->player.stats.str = 18;
               characterTile->player.stats.first.effect.tile.tileNum =
                                                      ITEM_POWER_SWORD;
             }

              if (!(characterTile->player.stats.flags &
                                        (CLASS_CLERIC | CLASS_WIZARD)))
              {
                if (stackedTile->tileNum == ITEM_WEAPON_BOW)
                {
                  characterTile->player.stats.second.affect += 1;
                  characterTile->player.stats.second.range = 3;
                  characterTile->player.stats.second.name = "zing";
                }
              }
            }
            else
            {
              if (stackedTile->tileNum == ITEM_POWER_STAFF)
              {
                characterTile->player.stats.first.affect += 2;
                characterTile->player.stats.first.effect.tile.tileNum =
                                                      ITEM_POWER_STAFF;
                characterTile->player.stats.second.affect += 1;
                  characterTile->player.stats.second.range += 2;
                characterTile->player.stats.second.effect.tile.tileNum=
                                                             ITEM_BALL;
              }
            }

            if (stackedTile->tileNum == ITEM_HELMET)
              characterTile->player.stats.ac += 1;
            else if (stackedTile->tileNum == ITEM_LIGHTNING_SWORD)
            {
              if (characterTile->player.stats.flags & CLASS_CLERIC)
              {
                characterTile->player.stats.first.affect += 2;
                characterTile->player.stats.first.effect.tile.tileNum =
                                                      ITEM_POWER_STAFF;
                characterTile->player.stats.second.affect += 1;
                characterTile->player.stats.second.range += 2;
                characterTile->player.stats.second.effect.tile.tileNum=
                                                             ITEM_BALL;
              }
            }

            characterTile->items[characterTile->num_items++] =
                                                          *stackedTile;
            puts("Item taken");
          }

          player_display(characterTile);

          // Remove the item from the background tile
          ListRemove(stackedTile->list);
        }
      }
    }
    return TASK_FINISHED;
  }

  // Otherwise restart the timer if not the last effect
  else
  {
    TimerSchedule(characterTile->player.effect.period,
              player_move_callback, characterTile, destinationTerrain);
    return TASK_FINISHED;
  }
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

  // Silent return if effect is already in progress
  if ((characterTile->player.effect.locationX !=
      characterTile->player.effect.destinationX) ||
      (characterTile->player.effect.count))
    return -1;

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

  // Hide player and redraw original tile
  characterTile->player.effect.tile.flags &= ~IS_VISIBLE;
  TileDisplayGrid(characterTile->player.effect.locationX,
                  characterTile->player.effect.locationY);

  // Make moving creatures visible
  characterTile->player.effect.tile.flags |= IS_VISIBLE;

  // Configure sprite movement effect
  // Total time of effect is effect.total * effect.period

  // Initialize the move effect
  characterTile->player.effect.total = 4; // 1 or power of 2
  characterTile->player.effect.period = MICROS_PER_SECOND / 16;

  // Assign the effect callback function invoked every period
  characterTile->player.effect.poll = player_move_callback;

  // Start the move player effect tile
  return TileEffectStart(&characterTile->player.effect,
                         tileX, tileY);
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
    character->player.effect.tile.color = COLOR_RED;
    character->player.effect.tile.tileNum = CHARACTER_WIZARD;
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
    character->player.effect.tile.color = COLOR_RED;
    character->player.effect.tile.tileNum = CHARACTER_CLERIC;
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
    character->player.effect.tile.color = COLOR_RED;
    character->player.effect.tile.tileNum = CHARACTER_SCOUT;
  }

  // Otherwise be a ranger
  else
  {
    character->player.stats.flags |= CLASS_RANGER | TRACKING;
    character->player.effect.tile.color = COLOR_RED;
    character->player.effect.tile.tileNum = CHARACTER_BARBARIAN;
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
    character->player.stats.first.name = "zap";
    character->player.stats.first.effect.tile.tileNum = ITEM_DART;
    character->player.stats.second.name = "blast";
    character->player.stats.second.effect.tile.tileNum = ITEM_DART;
  }
  else
  {
    character->player.stats.first.name = "pummel";
    character->player.stats.first.effect.tile.tileNum = ITEM_GAUNTLETS;
    character->player.stats.second.name = "fling";
    character->player.stats.second.effect.tile.tileNum = ITEM_WEAPON_ARROW;
  }
  character->player.stats.first.affect = 1;
  character->player.stats.first.range = 1;
  character->player.stats.second.affect = 0;
  character->player.stats.second.range = 0;
  character->player.stats.second.name = "none";

  // Configure for 1st level, no experience or ranged attack
  character->player.currentWorld->level = 1;
  character->player.currentWorld->score = 0;

  character->player.name = "xlawless";
  character->player.name[0] = rand() % 32 + 'A';

  character->player.effect.tile.bitmap = TerrainCharactersData;
  character->player.effect.tile.flags = IS_PLAYER | IS_VISIBLE|NO_FLIP;
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
    TimerCancel(TheWorld.round);
    TheWorld.round = NULL;
    GameUp = FALSE;
  }

  // Otherwise start the game by starting the timer
  else
  {
    puts("Game resumed");
    TheWorld.round = TimerSchedule(TheWorld.period, game_round,
                                   &TheCharacter, TheCreatures);
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

    // Cancel the old timer if already started
    if (TheWorld.round)
    {
      TimerCancel(TheWorld.round);
      TheWorld.round = NULL;
    }

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
    game_start(&TheCharacter, TheCreatures, &TheWorld,
               MICROS_PER_SECOND);

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
      ActionTile *attack = attack_range(&TheCharacter.player,
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

    player_move(&TheCharacter, TheCharacter.player.effect.locationX,
                TheCharacter.player.effect.locationY - 1);
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

    player_move(&TheCharacter, TheCharacter.player.effect.locationX,
                TheCharacter.player.effect.locationY + 1);
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

    player_move(&TheCharacter, TheCharacter.player.effect.locationX +1,
                TheCharacter.player.effect.locationY);
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

    player_move(&TheCharacter, TheCharacter.player.effect.locationX -1,
                TheCharacter.player.effect.locationY);
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

    player_move(&TheCharacter, TheCharacter.player.effect.locationX +1,
                TheCharacter.player.effect.locationY - 1);
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

    player_move(&TheCharacter, TheCharacter.player.effect.locationX -1,
                TheCharacter.player.effect.locationY - 1);
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

    player_move(&TheCharacter, TheCharacter.player.effect.locationX + 1,
                TheCharacter.player.effect.locationY + 1);
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

    player_move(&TheCharacter, TheCharacter.player.effect.locationX - 1,
                TheCharacter.player.effect.locationY + 1);
  }
  return TASK_FINISHED;
}

#endif /* ENABLE_GAME */
