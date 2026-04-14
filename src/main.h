#pragma once
#include <stdint.h>
#include "raylib.h"
#include "raymath.h"

// Global Constants
#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080

#define MAX_ENTITIES_AMOUNT 20000

#define MAX_SPAWN_DEFINITION 7

#define MAP_SIZE 10000
#define MAP_HALF_SIZE 5000

//~ Begin of Utility Structs
typedef struct FloatRange {
    float min;
    float max;
} FloatRange;

typedef struct Uint16Range {
    uint16_t min;
    uint16_t max;
} Uint16Range;
//~ End of Utility Structs

// ~Begin of Enums
typedef enum EntityType : uint8_t {
    ENTITY_TYPE_UNDEFINED = 0,
    ENTITY_TYPE_CHARACTER = 1,
    ENTITY_TYPE_PLAYER = 2,
    ENTITY_TYPE_PROJECTILE = 3,
    ENTITY_TYPE_ENEMY = 4
} EntityType;

typedef enum ProjectileType : uint8_t {
    PROJECTILE_TYPE_UNDEFINED = 0,
    PROJECTILE_TYPE_CRYSTAL_SHARD = 1,
    PROJECTILE_TYPE_FIREBALL = 2,
    PROJECTILE_TYPE_BOMB = 3,
    PROJECTILE_TYPE_NATURE_SPIKE = 4,
} ProjectileType;

typedef enum EnemyType : uint8_t {
    ENEMY_TYPE_NORMAL = 0,
    ENEMY_TYPE_FAST = 1,
    ENEMY_TYPE_TANK = 2,
    ENEMY_TYPE_BOSS = 3
} EnemyType;

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

typedef enum SpawnType : uint8_t {
    SPAWN_TYPE_SINGLE = 0,
    SPAWN_TYPE_CLUSTER = 1,
    SPAWN_TYPE_LINE = 2,
    SPAWN_TYPE_AROUND = 3
} SpawnType;
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

typedef struct EnemyCharacter{
    float health;
    float speed;

    EnemyType enemyType;
    float xpDropAmount;
} EnemyCharacter;

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
        EnemyCharacter enemyCharacter;
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

typedef struct SpawnDefinition{
    EnemyType enemyType;
    SpawnType spawnType;
    Uint16Range amountToSpawnRange;
    FloatRange distanceToSpawnRange;
    uint16_t chanceToSpawn;
    float Difficulty;
} SpawnDefinition;

typedef struct SpawnerData{
    float delayBetweenSpawns;
    float spawnTimer;
    SpawnDefinition spawnsDefinitions[MAX_SPAWN_DEFINITION];
} SpawnerData;

typedef struct PlayerStats{
    float currentXP;
    float nextLevelXP;
    uint16_t level;

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
    uint16_t lastEntityIndex;

    uint16_t playerIndex;

    SpawnerData spawnerData;
    float gameTimer;
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

//~ Begin of Enemy Implementation
Entity Enemy_GenerateEnemy(EnemyType enemyType);
void Enemy_ProcessAllMovement(float deltaTime);
//~ End of Enemy Implementation

//~ Begin of HUD Implementation
void HUD_Init();
void HUD_UpdateData();
void HUD_Draw();
//~ End of HUD Implementation

// ~Begin of Player Implementation
Camera2D Player_GenerateCamera();
Entity Player_GeneratePlayer();
PlayerStats Player_GeneratePlayerStats();
void Player_ProcessMovement(Entity* player, float deltaTime);
void Player_AnimateMovement(Entity* player, float deltaTime);
// ~End of Player Implementation

// ~Begin of Render Implementation
void Render_DrawMap();
void Render_DrawAllEntitiesSorted();
void Render_DrawEntity(Entity* entity);
// ~End of Render Implementation

// ~Begin of Spawner Implementation
SpawnerData Spawner_GenerateSpawnerData();
void Spawner_ProcessSpawnLogic(float deltaTime);
// ~End of Spawner Implementation

//~ Begin of Global Implementation
void Global_UpdateGameTimer(float deltaTime);
inline static Entity* Global_GetPlayer()
{
    return &globalVariables.entities[globalVariables.playerIndex];
}
inline static bool Global_AddEntity(Entity* entity)
{
    if (!entity) return false;
    if (globalVariables.lastEntityIndex >= MAX_ENTITIES_AMOUNT) return false;

    globalVariables.entities[globalVariables.lastEntityIndex] = *entity;
    globalVariables.lastEntityIndex++;

    return true;
}
inline static bool Global_DestroyEntity(uint16_t entityIndex)
{
    if (entityIndex < 0 || entityIndex >= globalVariables.lastEntityIndex || entityIndex >= MAX_ENTITIES_AMOUNT) return false;

    globalVariables.entities[entityIndex] = globalVariables.entities[globalVariables.lastEntityIndex];
    globalVariables.entities[globalVariables.lastEntityIndex].bIsActive = false;
    globalVariables.lastEntityIndex--;

    return true;
}
// ~End of Global Implementation

// ~Begin of Helpers Implementation
inline static float Helper_GetRandomFloatInRange(FloatRange range)
{
    return range.min + (float)GetRandomValue(0, 10000) / 10000.0f * (range.max - range.min);
}
inline static uint16_t Helper_GetRandomUint16InRange(Uint16Range range)
{
    return (uint16_t)GetRandomValue(range.min, range.max);
}
// ~End of Helpers Implementation