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

		globalTimer.add(this);
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