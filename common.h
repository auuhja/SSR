#pragma once

#include <cstdint>
#include <cassert>
#include <string>
#include <chrono>
#include <iostream>

#define arraysize(arr) (sizeof(arr) / sizeof(arr[0]))

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;


enum kb_button
{
	KB_0, KB_1, KB_2, KB_3, KB_4, KB_5, KB_6, KB_7, KB_8, KB_9,
	KB_A, KB_B, KB_C, KB_D, KB_E, KB_F, KB_G, KB_H, KB_I, KB_J,
	KB_K, KB_L, KB_M, KB_N, KB_O, KB_P, KB_Q, KB_R, KB_S, KB_T,
	KB_U, KB_V, KB_W, KB_X, KB_Y, KB_Z, KB_SPACE, KB_ENTER,
	KB_SHIFT, KB_ALT, KB_TAB, KB_CTRL, KB_ESC, KB_UP, KB_DOWN,
	KB_LEFT, KB_RIGHT,

	KB_BUTTONCOUNT,

	KB_UNKNOWN_BUTTON
};

struct button
{
	bool isDown;
	bool wasDown;
};

struct keyboard_input
{
	button buttons[KB_BUTTONCOUNT];
};

struct mouse_input
{
	button left;
	button right;
	button middle;
	float scroll;

	int x;
	int y;

	float relX;
	float relY;

	int dx;
	int dy;

	float reldx;
	float reldy;
};

struct raw_input
{
	keyboard_input keyboard;
	mouse_input mouse;
};

inline bool isDown(raw_input& input, kb_button buttonID)
{
	return input.keyboard.buttons[buttonID].isDown;
}

inline bool isUp(raw_input& input, kb_button buttonID)
{
	return !input.keyboard.buttons[buttonID].isDown;
}

inline bool buttonDownEvent(button& button)
{
	return button.isDown && !button.wasDown;
}

inline bool buttonUpEvent(button& button)
{
	return !button.isDown && button.wasDown;
}

inline bool buttonDownEvent(raw_input& input, kb_button buttonID)
{
	return buttonDownEvent(input.keyboard.buttons[buttonID]);
}

inline bool buttonUpEvent(raw_input& input, kb_button buttonID)
{
	return buttonUpEvent(input.keyboard.buttons[buttonID]);
}

inline kb_button charToButton(char c)
{
	kb_button result = KB_UNKNOWN_BUTTON;
	if (c >= 'a' && c <= 'z')
		result = (kb_button)(c - 'a' + KB_A);
	else if (c >= 'A' && c <= 'Z')
		result = (kb_button)(c - 'A' + KB_A);
	else if (c >= '0' && c <= '9')
		result = (kb_button)(c - '0');
	else
	{
		switch (c)
		{
			case 32: result = KB_SPACE; break;
			case 27: result = KB_ESC; break;
			case 9: result = KB_TAB; break;
				// TODO: complete this..
		}
	}

	return result;
}

inline bool buttonDownEvent(raw_input& input, char c)
{
	kb_button b = charToButton(c);
	if (b != KB_UNKNOWN_BUTTON)
		return buttonDownEvent(input.keyboard.buttons[b]);
	return false;
}

inline bool buttonUpEvent(raw_input& input, char c)
{
	kb_button b = charToButton(c);
	if (b != KB_UNKNOWN_BUTTON)
		return buttonUpEvent(input.keyboard.buttons[b]);
	return false;
}


// file IO
struct input_file
{
	const char* filename;
	uint64 size;
	void* contents;
};

input_file readFile(const char* filename);
void freeFile(input_file file);
uint64 getFileWriteTime(const char* filename);

#if 1

#include <Windows.h>

struct timed_block;

struct timer
{
	double perfFreq;

	timed_block* blocks;
	uint32 numberOfTimedBlocks = 0;

	timer();
	~timer();
	void add(timed_block* block);
	void printTimedBlocks();
};

extern timer globalTimer;

struct timed_block
{
	const char* function;
	const char* name;
	uint32 line;

	int64 start, end;

	timed_block() {}

	timed_block(const char* function, uint32 line, const char* name)
		: function(function), line(line), name(name)
	{
		LARGE_INTEGER time;
		QueryPerformanceCounter(&time);
		start = time.QuadPart;
	}

	~timed_block()
	{
		LARGE_INTEGER time;
		QueryPerformanceCounter(&time);
		end = time.QuadPart;

		globalTimer.add(this); // this adds a copy
	}
};


#define TIMED_BLOCK(name) timed_block timedBlock##__COUNTER__(__FUNCTION__, __LINE__, name);
#else
#define TIMED_BLOCK(name)
#endif

inline std::string getFileName(std::string path)
{
	size_t slashPos = path.find_last_of("/");
	if (slashPos != std::string::npos)
	{
		path.erase(0, slashPos + 1);
	}
	size_t dotPos = path.find_last_of(".");
	if (dotPos != std::string::npos)
	{
		path.erase(dotPos, path.length());
	}

	return path;
}

inline std::string getPath(std::string path)
{
	size_t slashPos = path.find_last_of("/");
	if (slashPos != std::string::npos)
	{
		path.erase(slashPos + 1, path.length());
	}

	return path;
}