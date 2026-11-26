#include "os/application.h"
#include "os/platform.h"
#include "os/logger.h"
#include "os/job.h"

#include <SDL3/SDL.h>

#include <imgui.h>
#include <backends/imgui_impl_sdl3.h>

#include <cstdio>
#include <memory>
#include <mutex>

#define OS_MAX_PENDING_EVENTS 256

struct PlatformState {
	os::IApplication *app = nullptr;
	os::RunConfig config;

	os::Window *window = nullptr;

	LogChannel log_channel;

	bool quit_requested = false;
	bool imgui_enabled = false;

	std::mutex event_mutex;
	unsigned pending_event_count = 0;
	SDL_Event pending_events[OS_MAX_PENDING_EVENTS];

	unsigned gamepad_count = 0;
	SDL_Gamepad *gamepads[OS_MAX_GAMEPADS] = {};

	os::InputState prev_input;
};

static PlatformState *state = nullptr;

LogChannel os::GetLogChannel()
{
	return state ? state->log_channel : LogChannel();
}

void os::RequestQuit()
{
	if (state)
		state->quit_requested = true;
}

static void InitImGui()
{
	if (!state->window || !state->config.enable_imgui)
		return;

	IMGUI_CHECKVERSION();

	ImGui::CreateContext();

	ImGuiIO &io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	ImGui_ImplSDL3_InitForVulkan(state->window->GetSDLWindow());

	state->imgui_enabled = true;
}

static void DestroyImGui()
{
	if (!state->imgui_enabled)
		return;

	ImGui_ImplSDL3_Shutdown();
	ImGui::DestroyContext();

	state->imgui_enabled = false;
}

static void ImGuiNewFrame()
{
	if (state->imgui_enabled)
		ImGui_ImplSDL3_NewFrame();
}

static void CloseAllGamepads()
{
	for (unsigned i = 0; i < state->gamepad_count; i++) {
		SDL_CloseGamepad(state->gamepads[i]);
		state->gamepads[i] = nullptr;
	}

	state->gamepad_count = 0;
}

static void ReconnectAllGamepads()
{
	if (state->gamepad_count > 0)
		CloseAllGamepads();

	int sdl_gp_count = 0;

	SDL_JoystickID *ids = SDL_GetGamepads(&sdl_gp_count);

	if (!ids)
		return;

	if (sdl_gp_count > OS_MAX_GAMEPADS) {
		DebugLogW(
			state->log_channel,
			"Found %d gamepads, but we only support a maximum of %d gamepads, so we're capping it and only opening %d gamepads.",
			sdl_gp_count,
			OS_MAX_GAMEPADS,
			OS_MAX_GAMEPADS
		);

		sdl_gp_count = OS_MAX_GAMEPADS;
	}

	state->gamepad_count = sdl_gp_count;

	for (unsigned i = 0; i < state->gamepad_count; i++) {
		state->gamepads[i] = SDL_OpenGamepad(ids[i]);

		if (state->gamepads[i])
			DebugLogD(state->log_channel, "Added gamepad with player index: %d", SDL_GetGamepadPlayerIndex(state->gamepads[i]));
		else
			DebugLogD(state->log_channel, "Failed to open gamepad %u: %s", static_cast<unsigned>(ids[i]), SDL_GetError());
	}

	SDL_free(ids);
}

static void MessagePump(void *)
{
	SDL_Event local_events[OS_MAX_PENDING_EVENTS];
	unsigned local_event_count = 0;

	SDL_Event ev;

	while (local_event_count < ArraySize(local_events) && SDL_PollEvent(&ev)) {
		local_events[local_event_count++] = ev;
	}

	if (local_event_count <= 0)
		return;

	std::lock_guard<std::mutex> guard(state->event_mutex);

	DebugLogAssert(state->log_channel, state->pending_event_count <= OS_MAX_PENDING_EVENTS, "Ran out of event buffer space SHIT.");

	unsigned available_space = OS_MAX_PENDING_EVENTS - state->pending_event_count;
	unsigned copy_count = (local_event_count > available_space) ? available_space : local_event_count;

	memcpy(state->pending_events + state->pending_event_count, local_events, copy_count * sizeof(SDL_Event));
	state->pending_event_count += copy_count;
}

static os::GamepadState *GamepadForID(os::InputState *input, SDL_JoystickID id)
{
	int index = SDL_GetGamepadPlayerIndexForID(id);

	if (index < 0 || index >= OS_MAX_GAMEPADS)
		return nullptr;

	return &input->gamepads[index];
}

static os::InputState ProcessEvents(const os::InputState &prev_state)
{
	os::InputState input_out = prev_state;

	MessagePump(nullptr);

	static SDL_Event events[OS_MAX_PENDING_EVENTS];
	unsigned event_count = 0;

	{
		std::lock_guard<std::mutex> guard(state->event_mutex);

		memcpy(events, state->pending_events, state->pending_event_count * sizeof(SDL_Event));
		event_count = state->pending_event_count;
		state->pending_event_count = 0;
	}

	input_out.mouse_delta = Vec2(0.f);
	input_out.mouse_wheel = Vec2(0.f);
	input_out.text_input[0] = '\0';

	unsigned text_input_length = 0;

	for (unsigned i = 0; i < event_count; i++) {
		const SDL_Event *ev = &events[i];

		if (state->imgui_enabled)
			ImGui_ImplSDL3_ProcessEvent(ev);

		switch (ev->type) {
			case SDL_EVENT_QUIT:
				state->quit_requested = true;
				break;

			case SDL_EVENT_KEY_DOWN:
				if (ev->key.scancode < ArraySize(input_out.kb_down))
					input_out.kb_down[ev->key.scancode] = true;
				break;

			case SDL_EVENT_KEY_UP:
				if (ev->key.scancode < ArraySize(input_out.kb_down))
					input_out.kb_down[ev->key.scancode] = false;
				break;

			case SDL_EVENT_TEXT_INPUT: {
				u64 len = strlen(ev->text.text);

				if (text_input_length + len < sizeof(input_out.text_input)) {
					memcpy(input_out.text_input + text_input_length, ev->text.text, len);
					text_input_length += len;
					input_out.text_input[text_input_length] = '\0';
				}
			} break;

			case SDL_EVENT_MOUSE_BUTTON_DOWN:
				if (ev->button.button < ArraySize(input_out.mb_down))
					input_out.mb_down[ev->button.button] = true;
				break;

			case SDL_EVENT_MOUSE_BUTTON_UP:
				if (ev->button.button < ArraySize(input_out.mb_down))
					input_out.mb_down[ev->button.button] = false;
				break;

			case SDL_EVENT_MOUSE_MOTION:
				SDL_GetGlobalMouseState(&input_out.mouse_screen_position.x, &input_out.mouse_screen_position.y);
				input_out.mouse_position = Vec2(ev->motion.x, ev->motion.y);
				input_out.mouse_delta += Vec2(ev->motion.xrel, ev->motion.yrel);
				break;

			case SDL_EVENT_MOUSE_WHEEL:
				input_out.mouse_wheel += Vec2(ev->wheel.x, ev->wheel.y);
				break;

			case SDL_EVENT_GAMEPAD_BUTTON_DOWN: {
				os::GamepadState *gp = GamepadForID(&input_out, ev->gbutton.which);

				if (gp && ev->gbutton.button < ArraySize(gp->down))
					gp->down[ev->gbutton.button] = true;
			} break;

			case SDL_EVENT_GAMEPAD_BUTTON_UP: {
				os::GamepadState *gp = GamepadForID(&input_out, ev->gbutton.which);

				if (gp && ev->gbutton.button < ArraySize(gp->down))
					gp->down[ev->gbutton.button] = false;
			} break;

			case SDL_EVENT_GAMEPAD_AXIS_MOTION: {
				os::GamepadState *gp = GamepadForID(&input_out, ev->gaxis.which);

				if (!gp)
					break;

				os::GamepadAxis axis = static_cast<os::GamepadAxis>(ev->gaxis.axis);

				float normalized_value = (ev->gaxis.value < 0)
					 ? static_cast<float>(ev->gaxis.value) / 32768.f
					 : static_cast<float>(ev->gaxis.value) / static_cast<float>(SDL_JOYSTICK_AXIS_MAX);

				gp->SetAxisValue(axis, normalized_value);
			} break;

			case SDL_EVENT_GAMEPAD_ADDED:
				ReconnectAllGamepads();
				break;

			case SDL_EVENT_GAMEPAD_REMOVED:
				DebugLogD(state->log_channel, "Removed Gamepad.");
				ReconnectAllGamepads();
				break;

			case SDL_EVENT_GAMEPAD_REMAPPED:
				SDL_ReloadGamepadMappings();
				ReconnectAllGamepads();
				break;

			default:
				break;
		}
	}

	for (unsigned i = 0; i < ArraySize(input_out.kb_down); i++) {
		input_out.kb_pressed[i]  =  input_out.kb_down[i] && !prev_state.kb_down[i];
		input_out.kb_released[i] = !input_out.kb_down[i] &&  prev_state.kb_down[i];
	}

	for (unsigned i = 0; i < ArraySize(input_out.mb_down); i++) {
		input_out.mb_pressed[i]  =  input_out.mb_down[i] && !prev_state.mb_down[i];
		input_out.mb_released[i] = !input_out.mb_down[i] &&  prev_state.mb_down[i];
	}

	for (unsigned p = 0; p < OS_MAX_GAMEPADS; p++) {
		for (unsigned i = 0; i < ArraySize(input_out.gamepads[p].down); i++) {
			input_out.gamepads[p].pressed[i]  =  input_out.gamepads[p].down[i] && !prev_state.gamepads[p].down[i];
			input_out.gamepads[p].released[i] = !input_out.gamepads[p].down[i] &&  prev_state.gamepads[p].down[i];
		}
	}

	return input_out;
}

static void FrameJobEntry(void *)
{
	os::InputState curr_input = ProcessEvents(state->prev_input);
	state->prev_input = curr_input;

	ImGuiNewFrame();

	bool quit = state->quit_requested || state->app->Tick(curr_input);

	if (quit) {
		state->app->Destroy();
		job::GetScheduler()->Halt();
		return;
	}

	job::Decl next_frame_job;
	next_frame_job.entry_point = FrameJobEntry;
	next_frame_job.priority = job::Priority_Normal;
	next_frame_job.flags = job::Flag_MainThreadOnly;

	job::Kick(next_frame_job, nullptr);
}

static void RootJobEntry(void *)
{
	state->app->Init(state->window);

	job::Decl frame_job;
	frame_job.entry_point = FrameJobEntry;
	frame_job.priority = job::Priority_Normal;
	frame_job.flags = job::Flag_MainThreadOnly;

	job::Kick(frame_job, nullptr);
}

int os::Run(IApplication *app, const RunConfig &config)
{
	SDL_InitFlags init_flags = SDL_INIT_EVENTS;

	if (config.create_window) {
		init_flags |=
			SDL_INIT_VIDEO |
			SDL_INIT_AUDIO |
			SDL_INIT_JOYSTICK |
			SDL_INIT_GAMEPAD |
			SDL_INIT_HAPTIC |
			SDL_INIT_SENSOR;
	}

	if (!SDL_Init(init_flags)) {
		fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
		return -1;
	}

	Unique<PlatformState> platform_state = CreateScope<PlatformState>();

	state = platform_state.get();
	state->app = app;
	state->config = config;

	Unique<Window> window;

	if (config.create_window) {
		window = CreateScope<Window>(config.window_title, config.window_width, config.window_height);

		if (!window->IsValid()) {
			SDL_Quit();
			return -1;
		}

		state->window = window.get();
	}

	int result = 0;

	{
		Logger logger(config.log_sink ? config.log_sink : "");

		state->log_channel = logs::OpenChannel("OS");

		DebugLogI(state->log_channel, "Initializing ImGui...");

		InitImGui();

		{
			job::Scheduler scheduler(logs::OpenChannel(state->log_channel, "JOB"));

			job::Decl root_job;
			root_job.entry_point = RootJobEntry;
			root_job.priority = job::Priority_Normal;
			root_job.flags = job::Flag_MainThreadOnly;

			scheduler.KickRaw(root_job, nullptr);

			scheduler.Enter(state->window ? MessagePump : nullptr, nullptr);
		}

		DestroyImGui();

		CloseAllGamepads();
	}

	window.reset();

	state = nullptr;

	SDL_Quit();

	return result;
}
