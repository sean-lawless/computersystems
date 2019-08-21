#ifndef _DENZI_DATA_H
#define _DENZI_DATA_H

#include <game_grid.h>

// Denzi tiles interface

// Define equipment
#define ITEM_POTION          0 // 1
#define ITEM_WEAPON_SWORD    1 // 2
#define ITEM_ARMOUR_SHEILD   2 // 3
#define ITEM_ARMOUR_LEATHER  3 // 4
#define ITEM_BAG             4
#define ITEM_WEAPON_BOW      5 // 5
#define ITEM_WEAPON_ARROW    6
#define ITEM_MAGIC_BOOTS     7 // 6
#define ITEM_RING            8
#define ITEM_NECKLACE        9
#define ITEM_NOTEBOOK        10
#define ITEM_CANDLE          11
#define ITEM_GAUNTLETS       12
#define ITEM_MUSHROOM        13
#define ITEM_KEY             14
#define ITEM_HELMET          15 // 7
#define ITEM_CHAINMAIL       19 // 8
#define ITEM_BATTLEAXE       25 // 9
#define ITEM_PLATEMAIL       32 // 10
#define ITEM_SPLINTMAIL      33 // 11
#define ITEM_DART            41
#define ITEM_MAGIC_BELT      42 // 12
#define ITEM_MAGIC_SWORD     47 // 13
#define ITEM_POWER_STAFF     48
#define ITEM_BALL            67
#define ITEM_SPARKLE_SWORD   78
#define ITEM_FLAMING_SWORD   79 // 14
#define ITEM_LIGHTNING_SWORD 80 // 15
#define ITEM_POWER_SWORD     81 // 16

// Define Creatures
#define CREATURE_SQUID       0
#define CREATURE_BAT         1  // 0
#define CREATURE_SKELETON    2
#define CREATURE_EYE         3
#define CREATURE_ZOMBIE      4
#define CREATURE_SNAKE       5
#define CREATURE_SNAIL       6
#define CREATURE_DRAGON      7
#define CREATURE_SLIME       8
#define CREATURE_GIANT_BUG   9  // 1
#define CREATURE_REAPER      10
#define CREATURE_CAT         11
#define CREATURE_DOG         12
#define CREATURE_HORSE       13
#define CREATURE_GHOUL       14
#define CREATURE_YETI        15

#define CREATURE_LAND_SQUID  18 // 2
#define CREATURE_MANTICORE   23 // 3
#define CREATURE_SHARK       26 // 4
#define CREATURE_GIANT_RAT   32 // 5
#define CREATURE_GIANT_TICK  36 // 6

#define CREATURE_DRAGON_FLY  70 // 7

// Define terrain
#define TERRAIN_NEW_BRICK    0
#define TERRAIN_USED_BRICK   1
#define TERRAIN_TILE         2
#define TERRAIN_SWIRL        6
#define TERRAIN_CRAGS        8
#define TERRAIN_HILLS        9
#define TERRAIN_GRASSLANDS   10
#define TERRAIN_DIRT_RIGHT   14
#define TERRAIN_DIRT_LEFT    15
#define TERRAIN_WATER        20

#define TERRAIN_MOUNTAINS    90
#define TERRAIN_OAK_FOREST   91
#define TERRAIN_OAK_PRAIRIE  92
#define TERRAIN_PINE_FOREST  93
#define TERRAIN_PINE_PRAIRIE 94

// Define weather
#define WEATHER_PRECIP_HEAVY 11
#define WEATHER_PRECIP_MEDIUM 12
#define WEATHER_PRECIP_LIGHT 13

// Define structures
#define STRUCTURE_BAR_WINDOW 3
#define STRUCTURE_DOOR_CLOSE 4
#define STRUCTURE_DOOR_OPEN  5
#define STRUCTURE_FOUNTAIN   7
#define STRUCTURE_BIG_TOWER  80
#define STRUCTURE_SMALL_TOWER 81
#define STRUCTURE_KEEP       82
#define STRUCTURE_CASTLE     83
#define STRUCTURE_BIG_CASTLE 84
#define STRUCTURE_TOWER      85
#define STRUCTURE_VILLAGE    86
#define STRUCTURE_TOWN       87
#define STRUCTURE_FARM       88
#define STRUCTURE_MONUMENT   89

// Define characters
#define CHARACTER_BARBARIAN  160
#define CHARACTER_WIZARD     161
#define CHARACTER_RANGER     162
#define CHARACTER_SORCERER   163
#define CHARACTER_CLERIC     164
#define CHARACTER_MONK       164
#define CHARACTER_PALADIN    166
#define CHARACTER_SCOUT      211

extern u8 TerrainCharactersData[], CreaturesData[], EquipmentData[];

#endif /* _DENZI_DATA_H */
