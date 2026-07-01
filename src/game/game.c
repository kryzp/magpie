
static void GameRegisterEntities(Game *game, E_World *world)
{
#define GameEntityDef(type, max)										\
	{																	\
		E_TypeDesc desc = {												\
			.name              = String8Lit(STRINGIFY(type)),			\
			.stride            = sizeof(type),							\
			.max_instances     = (max),									\
			.OnInit            = (E_TypeDescInitFn *)type##Init, 		\
			.OnDestroy         = (E_TypeDescDestroyFn *)type##Destroy,	\
			.OnPreAnimTick     = (E_TypeDescTickFn *)type##PreAnimTick, \
			.OnPostAnimTick    = (E_TypeDescTickFn *)type##PostAnimTick, \
			.OnPostPhysicsTick = (E_TypeDescTickFn *)type##PostPhysicsTick, \
			.OnSerialize       = (E_TypeDescSerializeFn *)type##Serialize, \
			.OnDeserialize     = (E_TypeDescDeserializeFn *)type##Deserialize \
		};																\
		game->entity_types[GameEntityType_##type] = E_WorldRegisterType(world, &desc); \
	}

#include "game_entity_xmacro.inc"

#undef GameEntityDef
}

static void GameInit(Game *game, E_World *world)
{
	GM_StackInit(&game->game_mode_stack);

	game->camera = R_CameraPerspective(v3x(0.f), v3(0.f, 1.f, 0.f), 90.f, 1280.f / 720.f, .1f, 100.f);

	CameraDriverConfig camera_driver_cfg = {0};
	camera_driver_cfg.mode = CameraDriverMode_Unrestricted;
	game->camera_driver = CameraDriverInit(&camera_driver_cfg);

	game->player_handle = E_WorldSpawn(world, game->entity_types[GameEntityType_Player], E_TransformIdentity());
}

static void GameTick(Game *game, const OS_InputState *input, f32 dt, f32 elapsed)
{
	GM_StackTick(&game->game_mode_stack, game, dt, input);
	
	CameraDriverDrive(&game->camera_driver, &game->camera, input, dt);
}
