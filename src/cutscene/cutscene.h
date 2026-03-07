#pragma once

#include "core/types.h"
#include "container/vector.h"
#include "math/vec3.h"

// Example of a .mcs cutscene file.
// Top (before ---) defines actors
// Bottom (after ---) defines the cutscene script
// [...] before a command sets the actor id of the currently being built command
// some commands don't need an actor so it's optional
// $xxx refers to a global variable

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

namespace cs
{
	enum CutsceneCommandType {
		CS_CMD_WAIT,
		CS_CMD_MOVE_ACTOR,
		CS_CMD_SHOW_DIALOG,
		CS_CMD_MAX_ENUM
	};

	struct CutsceneCommand {
		CutsceneCommandType type;
		bool is_finished;
		float time_remaining;
		u64 actor_tag_hash;
		int actor_id;
		Vec3 target_position;
		float speed;
	};

	class CutsceneCommandBuilder {
	public:
		CutsceneCommandBuilder();
		~CutsceneCommandBuilder();

		CutsceneCommand build();

		CutsceneCommandBuilder &set_type(CutsceneCommandType type);
		CutsceneCommandBuilder &set_length(float length);
		CutsceneCommandBuilder &set_actor(u64 hash);
		CutsceneCommandBuilder &set_target_position(const Vec3 &target);
		CutsceneCommandBuilder &set_speed(float speed);

	private:
		CutsceneCommand command;
	};

	struct Cutscene {
		Vector<CutsceneCommand> commands;
	};

	class CutsceneManager {
	public:
		CutsceneManager();
		~CutsceneManager();

		void play(const Cutscene &cutscene);
		void tick(float dt);

	private:
		Vector<CutsceneCommand> commands;
		int current_command_index;
		bool is_playing;
	};
}
