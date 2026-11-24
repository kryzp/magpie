#ifndef OS_ACTION_MAP_H
#define OS_ACTION_MAP_H

#include "core/types.h"
#include "os/input.h"

#include "container/hash_map.h"
#include "container/vector.h"

namespace os {

class ActionMap {
public:
	explicit ActionMap(unsigned action_count);

	void Process(const InputState &input, bool take_keyboard, bool take_mouse);

	bool Down(unsigned action) const;
	bool Pressed(unsigned action) const;
	bool Released(unsigned action) const;

	float ScrollDeltaX() const;
	float ScrollDeltaY() const;

	float CursorX() const;
	float CursorY() const;

	void EndFrame();
	void ReleaseAll();

	void BindKey(unsigned action, KeyboardKey key);
	void BindButton(unsigned action, MouseButton button);
	void Unbind(unsigned action);

private:
	void Press(const Vector<unsigned> &actions, bool *is_down, int code, int count);
	void Release(const Vector<unsigned> &actions, bool *is_down, int code, int count);

	static void AddOnce(Vector<unsigned> *list, unsigned action);
	static bool Contains(const Vector<unsigned> &list, unsigned action);
	static void RemoveFirst(Vector<unsigned> *list, unsigned action);

	HashMap<int, Vector<unsigned>> key_binds;
	HashMap<int, Vector<unsigned>> button_binds;

	Vector<int> sources_per_action;

	bool key_is_down[KeyboardKey_MaxEnum];
	bool button_is_down[MouseButton_MaxEnum];

	Vector<unsigned> frame_pressed;
	Vector<unsigned> frame_released;

	float cursor_x;
	float cursor_y;

	float scroll_delta_x;
	float scroll_delta_y;
};

}

#endif // OS_ACTION_MAP_H
