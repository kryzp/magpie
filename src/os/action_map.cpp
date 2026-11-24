#include "os/action_map.h"
#include "core/maths.h"

#include <string.h>

os::ActionMap::ActionMap(unsigned action_count)
	: sources_per_action(action_count, 0)
	, key_is_down{}
	, button_is_down{}
	, cursor_x(0.f)
	, cursor_y(0.f)
	, scroll_delta_x(0.f)
	, scroll_delta_y(0.f)
{
	memset(key_is_down, 0, sizeof(key_is_down));
	memset(button_is_down, 0, sizeof(button_is_down));
}

void os::ActionMap::Process(const InputState &input, bool take_keyboard, bool take_mouse)
{
	cursor_x = input.mouse_position.x;
	cursor_y = input.mouse_position.y;

	for (const auto &kv : key_binds) {
		int code = kv.first;

		if (take_keyboard && input.kb_pressed[code])
			Press(kv.second, key_is_down, code, KeyboardKey_MaxEnum);

		if (input.kb_released[code])
			Release(kv.second, key_is_down, code, KeyboardKey_MaxEnum);
	}

	for (const auto &kv : button_binds) {
		int code = kv.first;

		if (take_mouse && input.mb_pressed[code])
			Press(kv.second, button_is_down, code, MouseButton_MaxEnum);

		if (input.mb_released[code])
			Release(kv.second, button_is_down, code, MouseButton_MaxEnum);
	}

	if (take_mouse) {
		// one notch = one step
		scroll_delta_x += maths::Sign(input.mouse_wheel.x);
		scroll_delta_y += maths::Sign(input.mouse_wheel.y);
	}
}

bool os::ActionMap::Down(unsigned action) const
{
	return sources_per_action[action] > 0;
}

bool os::ActionMap::Pressed(unsigned action) const
{
	return Contains(frame_pressed, action);
}

bool os::ActionMap::Released(unsigned action) const
{
	return Contains(frame_released, action);
}

float os::ActionMap::ScrollDeltaX() const
{
	return scroll_delta_x;
}

float os::ActionMap::ScrollDeltaY() const
{
	return scroll_delta_y;
}

float os::ActionMap::CursorX() const
{
	return cursor_x;
}

float os::ActionMap::CursorY() const
{
	return cursor_y;
}

void os::ActionMap::EndFrame()
{
	frame_pressed.clear();
	frame_released.clear();
	scroll_delta_x = 0.f;
	scroll_delta_y = 0.f;
}

void os::ActionMap::ReleaseAll()
{
	for (unsigned i = 0; i < sources_per_action.size(); i++) {
		if (Down(i))
			AddOnce(&frame_released, i);
	}

	for (int &sources : sources_per_action)
		sources = 0;

	memset(key_is_down, 0, sizeof(key_is_down));
	memset(button_is_down, 0, sizeof(button_is_down));

	scroll_delta_y = 0.f;
}

void os::ActionMap::BindKey(unsigned action, KeyboardKey key)
{
	ReleaseAll();
	key_binds[key].push_back(action);
}

void os::ActionMap::BindButton(unsigned action, MouseButton button)
{
	ReleaseAll();
	button_binds[button].push_back(action);
}

void os::ActionMap::Unbind(unsigned action)
{
	ReleaseAll();

	for (auto &kv : key_binds)
		RemoveFirst(&kv.second, action);

	for (auto &kv : button_binds)
		RemoveFirst(&kv.second, action);
}

void os::ActionMap::Press(const Vector<unsigned> &actions, bool *is_down, int code, int count)
{
	if (code < 0 || code >= count || is_down[code])
		return;

	is_down[code] = true;

	for (unsigned action : actions) {
		if (sources_per_action[action] == 0)
			AddOnce(&frame_pressed, action);

		sources_per_action[action]++;
	}
}

void os::ActionMap::Release(const Vector<unsigned> &actions, bool *is_down, int code, int count)
{
	if (code < 0 || code >= count || !is_down[code])
		return;

	is_down[code] = false;

	for (unsigned action : actions) {
		sources_per_action[action]--;

		if (sources_per_action[action] == 0)
			AddOnce(&frame_released, action);
	}
}

void os::ActionMap::AddOnce(Vector<unsigned> *list, unsigned action)
{
	if (!Contains(*list, action)) {
		list->push_back(action);
	}
}

bool os::ActionMap::Contains(const Vector<unsigned> &list, unsigned action)
{
	for (unsigned a : list) {
		if (a == action)
			return true;
	}

	return false;
}

void os::ActionMap::RemoveFirst(Vector<unsigned> *list, unsigned action)
{
	for (u64 i = 0; i < list->size(); i++) {
		if ((*list)[i] == action) {
			list->erase(list->begin() + i);
			return;
		}
	}
}
