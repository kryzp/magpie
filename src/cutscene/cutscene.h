#ifndef CUTSCENE_H
#define CUTSCENE_H

/*
Pat = "npc_pat_01"
Mat = "npc_mat_01"
Player = $player
---
[Player] "Cau"
[Pat] "Ahoj!"
[Mat] move 10.0 5.0
[Mat] dialog "Kde je barva??"
wait 1.0
if $has_paint_bucket
	[Pat] "Tady!"
else
	[Pat] "Nevim..."
*/

typedef enum CS_CommandType
{
	CS_CommandType_Wait,
	CS_CommandType_MoveActor,
	CS_CommandType_ShowDialog,
	CS_CommandType_COUNT
}
CS_CommandType;

typedef struct CS_Command CS_Command;
struct CS_Command
{
	CS_Command *cutscene_next;
	
	CS_CommandType type;
	b32 is_finished;	

	f32 time_remaining;
	
	u64 actor_tag_hash;
	u32 actor_id;

	v3 target_position;
	f32 speed;
};

typedef struct CS_Cutscene CS_Cutscene;
struct CS_Cutscene
{
	CS_Command *command_head;
};

typedef struct CS_Director CS_Director;
struct CS_Director
{
	CS_Command *command_head;
	CS_Command *current_command;
	b32 is_playing;
};

internal void CS_DirectorPlay(CS_Director *director, const CS_Cutscene *cutscene);
internal void CS_DirectorTick(CS_Director *director, float dt);

#endif // CUTSCENE_H
