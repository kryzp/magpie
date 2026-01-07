#pragma once

#include "core/types.h"
#include "math/vec2.h"

#define DEFAULT_WINDOW_TITLE    "Demo"
#define ENGINE_NAME             "Magpie"

#define DEFAULT_WINDOW_WIDTH     1280
#define DEFAULT_WINDOW_HEIGHT    720

#define APP_VERSION_VARIANT      0
#define APP_VERSION_MAJOR        0
#define APP_VERSION_MINOR        1
#define APP_VERSION_PATCH        0

#define ENGINE_VERSION_VARIANT   1
#define ENGINE_VERSION_MAJOR     0
#define ENGINE_VERSION_MINOR     1
#define ENGINE_VERSION_PATCH     0

#define SCRATCH_MEMORY_SIZE      MEGABYTES(2)

enum KeyboardKey {
	KEYBOARD_KEY_unknown = 0,
	KEYBOARD_KEY_a = 4,
	KEYBOARD_KEY_b = 5,
	KEYBOARD_KEY_c = 6,
	KEYBOARD_KEY_d = 7,
	KEYBOARD_KEY_e = 8,
	KEYBOARD_KEY_f = 9,
	KEYBOARD_KEY_g = 10,
	KEYBOARD_KEY_h = 11,
	KEYBOARD_KEY_i = 12,
	KEYBOARD_KEY_j = 13,
	KEYBOARD_KEY_k = 14,
	KEYBOARD_KEY_l = 15,
	KEYBOARD_KEY_m = 16,
	KEYBOARD_KEY_n = 17,
	KEYBOARD_KEY_o = 18,
	KEYBOARD_KEY_p = 19,
	KEYBOARD_KEY_q = 20,
	KEYBOARD_KEY_r = 21,
	KEYBOARD_KEY_s = 22,
	KEYBOARD_KEY_t = 23,
	KEYBOARD_KEY_u = 24,
	KEYBOARD_KEY_v = 25,
	KEYBOARD_KEY_w = 26,
	KEYBOARD_KEY_x = 27,
	KEYBOARD_KEY_y = 28,
	KEYBOARD_KEY_z = 29,
	KEYBOARD_KEY_d1 = 30,
	KEYBOARD_KEY_d2 = 31,
	KEYBOARD_KEY_d3 = 32,
	KEYBOARD_KEY_d4 = 33,
	KEYBOARD_KEY_d5 = 34,
	KEYBOARD_KEY_d6 = 35,
	KEYBOARD_KEY_d7 = 36,
	KEYBOARD_KEY_d8 = 37,
	KEYBOARD_KEY_d9 = 38,
	KEYBOARD_KEY_d0 = 39,
	KEYBOARD_KEY_enter = 40,
	KEYBOARD_KEY_escape = 41,
	KEYBOARD_KEY_backspace = 42,
	KEYBOARD_KEY_tab = 43,
	KEYBOARD_KEY_space = 44,
	KEYBOARD_KEY_minus = 45,
	KEYBOARD_KEY_equals = 46,
	KEYBOARD_KEY_left_bracket = 47,
	KEYBOARD_KEY_right_bracket = 48,
	KEYBOARD_KEY_backslash = 49,
	KEYBOARD_KEY_non_us_hash = 50,
	KEYBOARD_KEY_semicolon = 51,
	KEYBOARD_KEY_apostrophe = 52,
	KEYBOARD_KEY_grave = 53,
	KEYBOARD_KEY_comma = 54,
	KEYBOARD_KEY_period = 55,
	KEYBOARD_KEY_slash = 56,
	KEYBOARD_KEY_capslock = 57,
	KEYBOARD_KEY_f1 = 58,
	KEYBOARD_KEY_f2 = 59,
	KEYBOARD_KEY_f3 = 60,
	KEYBOARD_KEY_f4 = 61,
	KEYBOARD_KEY_f5 = 62,
	KEYBOARD_KEY_f6 = 63,
	KEYBOARD_KEY_f7 = 64,
	KEYBOARD_KEY_f8 = 65,
	KEYBOARD_KEY_f9 = 66,
	KEYBOARD_KEY_f10 = 67,
	KEYBOARD_KEY_f11 = 68,
	KEYBOARD_KEY_f12 = 69,
	KEYBOARD_KEY_print_screen = 70,
	KEYBOARD_KEY_scroll_lock = 71,
	KEYBOARD_KEY_pause = 72,
	KEYBOARD_KEY_insert = 73,
	KEYBOARD_KEY_home = 74,
	KEYBOARD_KEY_page_up = 75,
	KEYBOARD_KEY_delete = 76,
	KEYBOARD_KEY_end = 77,
	KEYBOARD_KEY_page_down = 78,
	KEYBOARD_KEY_right = 79,
	KEYBOARD_KEY_left = 80,
	KEYBOARD_KEY_down = 81,
	KEYBOARD_KEY_up = 82,
	KEYBOARD_KEY_numlock_clear = 83,
	KEYBOARD_KEY_kp_divide = 84,
	KEYBOARD_KEY_kp_multiply = 85,
	KEYBOARD_KEY_kp_minus = 86,
	KEYBOARD_KEY_kp_plus = 87,
	KEYBOARD_KEY_kp_enter = 88,
	KEYBOARD_KEY_kp1 = 89,
	KEYBOARD_KEY_kp2 = 90,
	KEYBOARD_KEY_kp3 = 91,
	KEYBOARD_KEY_kp4 = 92,
	KEYBOARD_KEY_kp5 = 93,
	KEYBOARD_KEY_kp6 = 94,
	KEYBOARD_KEY_kp7 = 95,
	KEYBOARD_KEY_kp8 = 96,
	KEYBOARD_KEY_kp9 = 97,
	KEYBOARD_KEY_kp0 = 98,
	KEYBOARD_KEY_kp_period = 99,
	KEYBOARD_KEY_non_us_backslash = 100,
	KEYBOARD_KEY_application = 101,
	KEYBOARD_KEY_power = 102,
	KEYBOARD_KEY_kp_equals = 103,
	KEYBOARD_KEY_f13 = 104,
	KEYBOARD_KEY_f14 = 105,
	KEYBOARD_KEY_f15 = 106,
	KEYBOARD_KEY_f16 = 107,
	KEYBOARD_KEY_f17 = 108,
	KEYBOARD_KEY_f18 = 109,
	KEYBOARD_KEY_f19 = 110,
	KEYBOARD_KEY_f20 = 111,
	KEYBOARD_KEY_f21 = 112,
	KEYBOARD_KEY_f22 = 113,
	KEYBOARD_KEY_f23 = 114,
	KEYBOARD_KEY_f24 = 115,
	KEYBOARD_KEY_execute = 116,
	KEYBOARD_KEY_help = 117,
	KEYBOARD_KEY_menu = 118,
	KEYBOARD_KEY_select = 119,
	KEYBOARD_KEY_stop = 120,
	KEYBOARD_KEY_again = 121,
	KEYBOARD_KEY_undo = 122,
	KEYBOARD_KEY_cut = 123,
	KEYBOARD_KEY_copy = 124,
	KEYBOARD_KEY_paste = 125,
	KEYBOARD_KEY_find = 126,
	KEYBOARD_KEY_mute = 127,
	KEYBOARD_KEY_volume_up = 128,
	KEYBOARD_KEY_volume_down = 129,
	KEYBOARD_KEY_kp_comma = 133,
	KEYBOARD_KEY_kp_equals_as_400 = 134,
	KEYBOARD_KEY_international1 = 135,
	KEYBOARD_KEY_international2 = 136,
	KEYBOARD_KEY_international3 = 137,
	KEYBOARD_KEY_international4 = 138,
	KEYBOARD_KEY_international5 = 139,
	KEYBOARD_KEY_international6 = 140,
	KEYBOARD_KEY_international7 = 141,
	KEYBOARD_KEY_international8 = 142,
	KEYBOARD_KEY_international9 = 143,
	KEYBOARD_KEY_language1 = 144,
	KEYBOARD_KEY_language2 = 145,
	KEYBOARD_KEY_language3 = 146,
	KEYBOARD_KEY_language4 = 147,
	KEYBOARD_KEY_language5 = 148,
	KEYBOARD_KEY_language6 = 149,
	KEYBOARD_KEY_language7 = 150,
	KEYBOARD_KEY_language8 = 151,
	KEYBOARD_KEY_language9 = 152,
	KEYBOARD_KEY_alt_erase = 153,
	KEYBOARD_KEY_sys_req = 154,
	KEYBOARD_KEY_cancel = 155,
	KEYBOARD_KEY_clear = 156,
	KEYBOARD_KEY_prior = 157,
	KEYBOARD_KEY_return2 = 158,
	KEYBOARD_KEY_seperator = 159,
	KEYBOARD_KEY_out = 160,
	KEYBOARD_KEY_oper = 161,
	KEYBOARD_KEY_clear_again = 162,
	KEYBOARD_KEY_crsel = 163,
	KEYBOARD_KEY_exsel = 164,
	KEYBOARD_KEY_kp00 = 176,
	KEYBOARD_KEY_kp000 = 177,
	KEYBOARD_KEY_thousands_seperator = 178,
	KEYBOARD_KEY_decimal_seperator = 179,
	KEYBOARD_KEY_currency_unit = 180,
	KEYBOARD_KEY_currency_sub_unit = 181,
	KEYBOARD_KEY_kp_left_parent = 182,
	KEYBOARD_KEY_kp_right_parent = 183,
	KEYBOARD_KEY_kp_left_brace = 184,
	KEYBOARD_KEY_kp_right_brace = 185,
	KEYBOARD_KEY_kp_tab = 186,
	KEYBOARD_KEY_kp_backspace = 187,
	KEYBOARD_KEY_kp_a = 188,
	KEYBOARD_KEY_kp_b = 189,
	KEYBOARD_KEY_kp_c = 190,
	KEYBOARD_KEY_kp_d = 191,
	KEYBOARD_KEY_kp_e = 192,
	KEYBOARD_KEY_kp_f = 193,
	KEYBOARD_KEY_kp_xor = 194,
	KEYBOARD_KEY_kp_power = 195,
	KEYBOARD_KEY_kp_percent = 196,
	KEYBOARD_KEY_kp_less = 197,
	KEYBOARD_KEY_kp_greater = 198,
	KEYBOARD_KEY_kp_ampersand = 199,
	KEYBOARD_KEY_kp_double_ampersand = 200,
	KEYBOARD_KEY_kp_vertical_bar = 201,
	KEYBOARD_KEY_kp_double_vertical_bar = 202,
	KEYBOARD_KEY_kp_colon = 203,
	KEYBOARD_KEY_kp_hash = 204,
	KEYBOARD_KEY_kp_space = 205,
	KEYBOARD_KEY_kp_at = 206,
	KEYBOARD_KEY_kp_exclaim = 207,
	KEYBOARD_KEY_kp_mem_store = 208,
	KEYBOARD_KEY_kp_mem_recall = 209,
	KEYBOARD_KEY_kp_mem_clear = 210,
	KEYBOARD_KEY_kp_mem_add = 211,
	KEYBOARD_KEY_kp_mem_subtract = 212,
	KEYBOARD_KEY_kp_mem_multiply = 213,
	KEYBOARD_KEY_kp_mem_divide = 214,
	KEYBOARD_KEY_kp_plus_minus = 215,
	KEYBOARD_KEY_kp_clear = 216,
	KEYBOARD_KEY_kp_clear_entry = 217,
	KEYBOARD_KEY_kp_binary = 218,
	KEYBOARD_KEY_kp_octal = 219,
	KEYBOARD_KEY_kp_decimal = 220,
	KEYBOARD_KEY_kp_hexadecimal = 221,
	KEYBOARD_KEY_left_control = 224,
	KEYBOARD_KEY_left_shift = 225,
	KEYBOARD_KEY_left_alt = 226,
	KEYBOARD_KEY_left_super = 227,
	KEYBOARD_KEY_right_control = 228,
	KEYBOARD_KEY_right_shift = 229,
	KEYBOARD_KEY_right_alt = 230,
	KEYBOARD_KEY_right_super = 231,
	KEYBOARD_KEY_MAX_ENUM
};

enum MouseButton {
	MBUTTON_unknown = 0,
	MBUTTON_left = 1,
	MBUTTON_middle = 2,
	MBUTTON_right = 3,
	MBUTTON_side_bottom = 4,
	MBUTTON_side_top = 5,
	MBUTTON_MAX_ENUM
};

#define MAX_GAMEPADS 4

enum GamepadButton {
	GAMEPAD_BUTTON_a,
	GAMEPAD_BUTTON_b,
	GAMEPAD_BUTTON_x,
	GAMEPAD_BUTTON_y,
	GAMEPAD_BUTTON_back,
	GAMEPAD_BUTTON_select,
	GAMEPAD_BUTTON_start,
	GAMEPAD_BUTTON_left_stick,
	GAMEPAD_BUTTON_right_stick,
	GAMEPAD_BUTTON_left_shoulder,
	GAMEPAD_BUTTON_right_shoulder,
	GAMEPAD_BUTTON_up,
	GAMEPAD_BUTTON_down,
	GAMEPAD_BUTTON_left,
	GAMEPAD_BUTTON_right,
	GAMEPAD_BUTTON_MAX_ENUM
};

enum GamepadAxis {
	GAMEPAD_AXIS_left_x,
	GAMEPAD_AXIS_left_y,
	GAMEPAD_AXIS_right_x,
	GAMEPAD_AXIS_right_y,
	GAMEPAD_AXIS_trigger_left,
	GAMEPAD_AXIS_trigger_right,
	GAMEPAD_AXIS_MAX_ENUM
};

enum GamepadType {
	GAMEPAD_TYPE_standard,
	GAMEPAD_TYPE_xbox_360,
	GAMEPAD_TYPE_xbox_one,
	GAMEPAD_TYPE_ps3,
	GAMEPAD_TYPE_ps4,
	GAMEPAD_TYPE_ps5,
	GAMEPAD_TYPE_nintendo_switch_pro,
	GAMEPAD_TYPE_nintendo_switch_joycon_left,
	GAMEPAD_TYPE_nintendo_switch_joycon_right,
	GAMEPAD_TYPE_nintendo_switch_joycon_pair,
	GAMEPAD_TYPE_MAX_ENUM
};

struct GamepadState {
	bool down[GAMEPAD_BUTTON_MAX_ENUM];
	bool pressed[GAMEPAD_BUTTON_MAX_ENUM];
	bool released[GAMEPAD_BUTTON_MAX_ENUM];

	Vec2 left_stick;
	Vec2 right_stick;

	float left_trigger;
	float right_trigger;
};

struct Platform {
	const char *window_title;

	u32 window_width;
	u32 window_height;

	int window_pixel_width;
	int window_pixel_height;

	float window_opacity;

	bool fullscreen;
	bool borderless;

	float target_fps;
	float current_time;

	bool cursor_visible;
	bool cursor_locked;

	bool kb_down[KEYBOARD_KEY_MAX_ENUM];
	bool kb_pressed[KEYBOARD_KEY_MAX_ENUM];
	bool kb_released[KEYBOARD_KEY_MAX_ENUM];

	bool mb_down[MBUTTON_MAX_ENUM];
	bool mb_pressed[MBUTTON_MAX_ENUM];
	bool mb_released[MBUTTON_MAX_ENUM];

	Vec2 mouse_position;
	Vec2 mouse_delta;
	Vec2 mouse_screen_position;
	Vec2 mouse_wheel;

	GamepadState gamepads[MAX_GAMEPADS];

	void (*set_window_size)(u32 width, u32 height);
	void (*set_window_fullscreen)(bool b);
	void (*set_window_borderless)(bool b);

	void (*set_mouse_position)(u32 x, u32 y);

	u64 (*get_ticks)(void);
	u64 (*get_performance_counter)(void);
	u64 (*get_performance_frequency)(void);

	bool (*file_delete)(const char *path);
	bool (*file_exists)(const char *path);

	bool (*dir_create)(const char *path);
	bool (*dir_delete)(const char *path);
	bool (*dir_exists)(const char *path);

	void *(*stream_from_file)(const char *path, const char *mode);
	void *(*stream_from_memory)(void *data, u64 size);
	void *(*stream_from_const_memory)(const void *data, u64 size);
	s64 (*stream_read)(void *stream, void *dst, u64 size);
	s64 (*stream_write)(void *stream, const void *src, u64 size);
	s64 (*stream_seek)(void *stream, s64 offset);
	s64 (*stream_size)(void *stream);
	s64 (*stream_position)(void *stream);
	bool (*stream_close)(void *stream);

	void (*open_in_explorer)(const char *path);

	bool (*create_vulkan_surface)(void *instance, void *surface_pointer);
	void (*destroy_vulkan_surface)(void *instance, void *surface);
	const char *const *(*get_vulkan_instance_extensions)(u32 *count);
};
