#include "cutscene.h"

using namespace cs;

static void cs_cmd_wait_process(CutsceneCommand &cmd, float dt)
{
	cmd.time_remaining -= dt;
	if (cmd.time_remaining <= 0.f)
		cmd.is_finished = true;
}

static void cs_cmd_move_actor_process(CutsceneCommand &cmd, float dt)
{
}

static void cs_cmd_show_dialog_process(CutsceneCommand &cmd, float dt)
{
}

CutsceneManager::CutsceneManager()
	: commands()
	, current_command_index(0)
	, is_playing(false)
{
}

CutsceneManager::~CutsceneManager()
{
}

void CutsceneManager::play(const Cutscene &cutscene)
{
	commands = cutscene.commands;
	current_command_index = 0;
	is_playing = true;
	
	// todo: load actor ids
}

void CutsceneManager::tick(float dt)
{
	if (!is_playing)
		return;

	auto &cmd = commands[current_command_index];

	switch (cmd.type) {
		case CS_CMD_WAIT:
			cs_cmd_wait_process(cmd, dt);
			break;

		case CS_CMD_MOVE_ACTOR:
			cs_cmd_move_actor_process(cmd, dt);
			break;

		case CS_CMD_SHOW_DIALOG:
			cs_cmd_show_dialog_process(cmd, dt);
			break;
	}

	if (cmd.is_finished)
		current_command_index++;

	if (current_command_index >= commands.size())
		is_playing = false;
}
