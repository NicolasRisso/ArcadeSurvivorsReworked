#pragma once
#include <stdint.h>
#include "raylib.h"
#include "raymath.h"

// Global Constants
#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080

#define MAX_ENTITIES_AMOUNT 20000

#define MAP_SIZE 10000
#define MAP_HALF_SIZE 5000

// ~Begin of Enums
typedef enum EntityType : uint8_t {
    ENTITY_TYPE_UNDEFINED = 0,
    ENTITY_TYPE_CHARACTER = 1,
    ENTITY_TYPE_PLAYER = 2,
    ENTITY_TYPE_PROJECTILE = 3,
} EntityType;

typedef enum ProjectileType : uint8_t {
    PROJECTILE_TYPE_UNDEFINED = 0,
    PROJECTILE_TYPE_CRYSTAL_SHARD = 1,
    PROJECTILE_TYPE_FIREBALL = 2,
    PROJECTILE_TYPE_BOMB = 3,
    PROJECTILE_TYPE_NATURE_SPIKE = 4,
} ProjectileType;

typedef enum AssetSpriteType : uint8_t {
    ASSET_SPRITE_TYPE_PLAYER = 0,
    ASSET_SPRITE_TYPE_GRASS = 1,
    ASSET_SPRITE_TYPE_BAT = 2,
    ASSET_SPRITE_TYPE_COUNT = 3
} AssetSpriteType;

typedef enum AssetSoundType : uint8_t {
    ASSET_SOUND_TYPE_DAMAGE = 0,
    ASSET_SOUND_TYPE_EXPLOSION = 1,
    ASSET_SOUND_TYPE_LEVEL_UP = 2,
    ASSET_SOUND_TYPE_XP_GAIN = 3,
    ASSET_SOUND_TYPE_PLAYER_DAMAGE = 4,
    ASSET_SOUND_TYPE_COUNT = 5
} AssetSoundType;

typedef enum AssetMusicType : uint8_t {
    ASSET_MUSIC_TYPE_COMBAT = 0,
    ASSET_MUSIC_TYPE_COUNT = 1
} AssetMusicType;

typedef enum VisualType : uint8_t {
    VISUAL_TYPE_NONE = 0, // For entities that have no sprite
    VISUAL_TYPE_SPRITE = 1, // For entities with sprites
    VISUAL_TYPE_ANIMATED_SPRITE = 2 // For entities with animated sprites
} VisualType;
// ~End of Enums

// ~Begin of Structs

typedef struct SpriteData {
    uint8_t spriteID;
    bool flipX;
} SpriteData;

typedef struct AnimatedSpriteData {
    uint8_t spriteID;
    uint8_t frameCount;
    uint8_t currentFrame;
    float frameTimer;
    float frameTime;
    bool flipX;
} AnimatedSpriteData;

typedef struct Character{
    float health;
    float maxHealth;
    float speed;
} Character;

typedef struct Projectile{
    float damage;
    float lifeTime;
    uint8_t penetration;
    uint16_t ownerID;
} Projectile;

typedef struct Entity{
    EntityType type;
    uint16_t id;
    uint8_t spriteID;
    bool bIsActive;
    Vector2 position;
    Vector2 velocity;
    Vector2 scale;
    float radius;
    union
    {
        Character character;
        Projectile projectile;
    };

    // Sprite Data
    VisualType visualType;
    union
    {
        SpriteData sprite;
        AnimatedSpriteData animatedSprite;
    };
} Entity;

typedef struct Assets{
    Texture2D sprites[ASSET_SPRITE_TYPE_COUNT];
    Sound sounds[ASSET_SOUND_TYPE_COUNT];
    Music musics[ASSET_MUSIC_TYPE_COUNT];
} Assets;

typedef struct PlayerStats{
    float currentXP;
    float nextLevelXP;

    float healthMultiplier;
    float damageMultiplier;
    float attackSpeedMultiplier;
    float movementSpeedMultiplier;
    float sizeMultiplier;
    float lifeStealMultiplier;
    float xpMultiplier;
} PlayerStats;

typedef struct GlobalVariables{
    Assets assets;
    
    Camera2D camera;
    PlayerStats playerStats;

    Entity entities[MAX_ENTITIES_AMOUNT];
    uint16_t entityCount;

    uint16_t playerIndex;
} GlobalVariables;
// ~End of Structs

// Declaration of Global Variables
extern GlobalVariables globalVariables;

// The game main loop
int main(void);

// ~Begin of Core Implementation
void Core_InitGame();
void Core_ProcessInput();
void Core_UpdateGame(float deltaTime);
void Core_RenderGraphics();
void Core_CloseGame();

int Core_IsGameReadyToClose();
// ~End of Core Implementation

// ~Begin of Assets Implementation
void Assets_Init();
void Assets_Unload();
Texture2D Assets_GetSprite(AssetSpriteType spriteID);
Sound Assets_GetSound(AssetSoundType soundID);
Music Assets_GetMusic(AssetMusicType musicID);
// ~End of Assets Implementation

//~ Begin of Collision Implementation
void Collision_MapBorder(Entity* entity);
//~ End of Collision Implementation

// ~Begin of Player Implementation
Camera2D Player_GenerateCamera();
Entity Player_GeneratePlayer();
PlayerStats Player_GeneratePlayerStats();
void Player_ProcessMovement(Entity* player, float deltaTime);
void Player_AnimateMovement(Entity* player, float deltaTime);
// ~End of Player Implementation

// ~Begin of Render Implementation
void Render_DrawMap();
void Render_DrawPlayer();
// ~End of Render Implementation

// ~Helpers

// ~Begin of Global Implementation
inline static Entity* Global_GetPlayer()
{
    return &globalVariables.entities[globalVariables.playerIndex];
}
// ~End of Global Implementation