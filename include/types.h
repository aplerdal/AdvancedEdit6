// From https://github.com/aplerdal/MkscRE

// types.h
// Contains several known types from mksc
#pragma once

#ifdef _cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;

typedef int8_t   s8;
typedef int16_t  s16;
typedef int32_t  s32;

// Offset types
typedef uint8_t  o8;
typedef uint16_t o16;
typedef uint32_t o32;

// Pointer type to support 64 bit OSes
typedef uint32_t p32;

// Bool Sizes
typedef uint8_t  b8;
typedef uint32_t b32;

typedef uint16_t BGR;

typedef BGR palette256[256];
typedef BGR palette16[16];
typedef u8 track_behaviors[256];

#define TRACK_COUNT 49

typedef struct {
    u32 entry_point;
    u8 logo[156];
    char title[12];
    char code[4];
    char maker[2];
} Entry;

#define TRACK_TABLE_ADDRESS 0x258000
typedef struct {
    o32 track_offsets[TRACK_COUNT];
    char date[16];
} TrackTable;

#define CUP_DEFINITION_ADRESS 0xe7464
typedef u32 CupDefinitions[52];

// Do not need this as it isn't techically used in the game, rather it has a constant table (Definition pointer table)
// in memory instead at e7ff0 that points to each indiviual element.
#define DEFINITION_TABLE_ADDRESS 0xE7534

typedef struct {
    u32 track_id;
    u32 background_id;
    u32 background_behavior;
    u32 animation;
    u32 material;
    p32 turn_instructions;
    u32 music_id;
    p32 target_set_table;
    p32 unk;
    p32 track_cover_gfx;
    p32 track_cover_pal;
    p32 locked_palette;
    p32 track_name_gfx;
    u32 unused_lap_count; // Can be turned useable via a patch, but unused in base game.
} TrackDefinition;

#define DEFINITION_POINTER_TABLE 0xe7ff0
// Should not be directly casted
typedef TrackDefinition* DefinitionTable[TRACK_COUNT];

#define TRACK_FLAGS_SPLIT_TILESET (1 << 0)
#define TRACK_FLAGS_SPLIT_LAYOUT  (1 << 1)
#define TRACK_FLAGS_SPLIT_OBJECTS (1 << 2)

#pragma pack(1)
typedef struct TrackHeader{ /*0x100*/
    //Offset
    /*0x00*/ u8 magic;
    /*0x01*/ b8 compressed_tileset;
    /*0x02*/ u8 padding1;
    /*0x03*/ u8 track_flags;
    /*0x04*/ u8 width; // These could be the wrong way around
    /*0x05*/ u8 height;
    /*0x06*/ u8 padding2[2+40];
    /*0x30*/ u32 reused_tileset;
    /*0x34*/ u8 padding3[12];
    /*0x40*/ o32 layout_offset;
    /*0x44*/ u8 padding4[60];
    /*0x80*/ o32 tileset_offset;
    /*0x84*/ o32 palette_offset;
    /*0x88*/ o32 behaviors_offset;
    /*0x8C*/ o32 objects_offset;
    /*0x90*/ o32 overlay_offset;
    /*0x94*/ o32 item_boxes_offset;
    /*0x98*/ o32 finish_line_offset;
    /*0x9C*/ u32 unk1;
    /*0xA0*/ u8 padding5[4];
    /*0xA8*/ u8 battle_unk1[28];
    /*0xC4*/ u32 track_routine;
    /*0xC8*/ o32 minimap_offset;
    /*0xCC*/ u8 battle_unk2[4];
    /*0xD0*/ o32 ai_offset;
    /*0xD4*/ u8 padding6[20];
    /*0xE8*/ o32 object_gfx_offset;
    /*0xEC*/ o32 object_pal_offset;
    /*0xF0*/ u32 reused_object;
    /*0xF4*/ u8 padding7[16];
} TrackHeader;

#pragma pack(1)
typedef struct{
    u8 count;
    o16 zones_offset;
    o16 targets_offset;
} AiHeader;

#pragma pack(1)
typedef struct{
    u16 x;
    u16 y;
    u16 speed;
    u16 flags;
} AiTarget;

#define ZONE_SHAPE_RECTANGLE 0
#define ZONE_SHAPE_TRIANGLE_TOP_LEFT 1
#define ZONE_SHAPE_TRIANGLE_TOP_RIGHT 2
#define ZONE_SHAPE_TRIANGLE_BOTTOM_LEFT 3
#define ZONE_SHAPE_TRIANGLE_BOTTOM_RIGHT 4

#pragma pack(1)
typedef struct{
    u8 shape;
    u16 half_x;
    u16 half_y;
    u16 half_width;
    u16 half_height;
} AiZone;

typedef struct{
    u8 id;
    u8 x;
    u8 y;
    u8 zone;
} Object;

#ifdef _cplusplus
}
#endif