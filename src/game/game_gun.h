#ifndef GAME_GUN_H
#define GAME_GUN_H

typedef enum AmmoType
{
	AmmoType_Magnum357,
	AmmoType_COUNT
}
AmmoType;

typedef struct GunSpecs GunSpecs;
struct GunSpecs
{
	u32 max_ammo_in_clip;
	AmmoType ammo_type;
	f32 rechamber_time;
};

typedef struct Gun Gun;
struct Gun
{
	u32 ammo_count;
	GunSpecs specs;

	CH_Timer shoot_timer;
};

static void GunFire(Gun *gun);
static void GunReload(Gun *gun);

#endif // GAME_GUN_H
