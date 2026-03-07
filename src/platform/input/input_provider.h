#if 0

#pragma once

namespace inp
{
	struct InputActions {
		int data;
	};
	
	void dialogue_input_controller(const InputState &state, InputActions *actions) { }
	void navigation_input_controller(const InputState &state, InputActions *actions) { }
	void player_input_controller(const InputState &state, InputActions *actions) { state->data = state->key_pressed(key_enter); }

	InputActions player_input_provider(const InputState &state)
	{
		InputActions actions = {};

		dialogue_input_controller(state, &actions);
		navigation_input_controller(state, &actions);
		player_input_controller(state, &actions);

		return actions;
	}
}

#endif
