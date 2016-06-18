#include "scene.h"
#include "common.h"

#include <Windows.h>
#include <Windowsx.h>

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 800

static uint32 clientWidth;
static uint32 clientHeight;
static bool running;
static uint32 currentScene = SCENE_HALLWAY;

timer globalTimer;

static LRESULT CALLBACK windowCallBack(
	_In_ HWND   hwnd,
	_In_ UINT   msg,
	_In_ WPARAM wParam,
	_In_ LPARAM lParam
	)
{
	LRESULT result = 0;

	switch (msg)
	{
		case WM_SIZE:
		{
			clientWidth = LOWORD(lParam);
			clientHeight = HIWORD(lParam);
		} break;
		case WM_CLOSE:
		case WM_DESTROY:
		{
			running = false;
		} break;
		case WM_ACTIVATEAPP:
		{
			// TODO
		} break;
		default:
		{
			result = DefWindowProc(hwnd, msg, wParam, lParam);
		} break;
	}

	return result;
}

static kb_button mapVKCodeToRawButton(uint32 vkCode)
{
	if (vkCode >= '0' && vkCode <= '9')
	{
		return (kb_button)(vkCode - 48);
	}
	else if (vkCode >= 'A' && vkCode <= 'Z')
	{
		return (kb_button)(vkCode - 55);
	}
	else
	{
		switch (vkCode)
		{
			case VK_SPACE: return KB_SPACE;
			case VK_TAB: return KB_TAB;
			case VK_RETURN: return KB_ENTER;
			case VK_SHIFT: return KB_SHIFT;
			case VK_CONTROL: return KB_CTRL;
			case VK_ESCAPE: return KB_ESC;
			case VK_UP: return KB_UP;
			case VK_DOWN: return KB_DOWN;
			case VK_LEFT: return KB_LEFT;
			case VK_RIGHT: return KB_RIGHT;
			case VK_MENU: return KB_ALT;
		}
	}
	return KB_UNKNOWN_BUTTON;
}

static void processPendingMessages(int windowWidth, int windowHeight, raw_input& lastInput, raw_input& curInput)
{
	MSG msg = { 0 };
	while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
	{
		switch (msg.message)
		{
			case WM_QUIT:
			{
				running = false;
			} break;
			case WM_SYSKEYDOWN:
			case WM_SYSKEYUP:
			case WM_KEYDOWN:
			case WM_KEYUP:
			{
				uint32 vkCode = (uint32)msg.wParam;
				bool wasDown = ((msg.lParam & (1 << 30)) != 0);
				bool isDown = ((msg.lParam & (1 << 31)) == 0);
				kb_button button = mapVKCodeToRawButton(vkCode);
				curInput.keyboard.buttons[button].isDown = isDown;
				curInput.keyboard.buttons[button].wasDown = wasDown;
			} break;
			case WM_LBUTTONDOWN:
			{
				curInput.mouse.left.isDown = true;
				curInput.mouse.left.wasDown = false;
			} break;
			case WM_LBUTTONUP:
			{
				curInput.mouse.left.isDown = false;
				curInput.mouse.left.wasDown = true;
			} break;
			case WM_RBUTTONDOWN:
			{
				curInput.mouse.right.isDown = true;
				curInput.mouse.right.wasDown = false;
			} break;
			case WM_RBUTTONUP:
			{
				curInput.mouse.right.isDown = false;
				curInput.mouse.right.wasDown = true;
			} break;
			case WM_MBUTTONDOWN:
			{
				curInput.mouse.middle.isDown = true;
				curInput.mouse.middle.wasDown = false;
			} break;
			case WM_MBUTTONUP:
			{
				curInput.mouse.middle.isDown = false;
				curInput.mouse.middle.wasDown = true;
			} break;
			case WM_MOUSEMOVE:
			{
				int32 mousePosX = GET_X_LPARAM(msg.lParam);
				int32 mousePosY = windowHeight - GET_Y_LPARAM(msg.lParam);
				float relX = (float)mousePosX / windowWidth;
				float relY = (float)mousePosY / windowHeight;
				curInput.mouse.x = mousePosX;
				curInput.mouse.y = mousePosY;
				curInput.mouse.relX = relX;
				curInput.mouse.relY = relY;
				curInput.mouse.dx = mousePosX - lastInput.mouse.x;
				curInput.mouse.dy = mousePosY - lastInput.mouse.y;
				curInput.mouse.reldx = relX - lastInput.mouse.relX;
				curInput.mouse.reldy = relY - lastInput.mouse.relY;
			} break;
			case WM_MOUSEWHEEL:
			{
				float scroll = GET_WHEEL_DELTA_WPARAM(msg.wParam) / 120.f;
				curInput.mouse.scroll = scroll;
			} break;
			default:
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			} break;
		}
	}
}

static bool initializeOpenGL(HDC windowDC)
{
	PIXELFORMATDESCRIPTOR desired;
	ZeroMemory(&desired, sizeof(PIXELFORMATDESCRIPTOR));
	desired.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	desired.nVersion = 1;
	desired.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	desired.iPixelType = PFD_TYPE_RGBA;
	desired.cColorBits = 32;
	desired.cAlphaBits = 8;
	desired.cDepthBits = 24;
	desired.cStencilBits = 8;

	int suggestedFormat = ChoosePixelFormat(windowDC, &desired);
	PIXELFORMATDESCRIPTOR suggestedPFD;
	DescribePixelFormat(windowDC, suggestedFormat, sizeof(PIXELFORMATDESCRIPTOR), &suggestedPFD);

	if (!SetPixelFormat(windowDC, suggestedFormat, &suggestedPFD))
	{
		return false;
	}

	HGLRC renderContext = wglCreateContext(windowDC);

	if (!wglMakeCurrent(windowDC, renderContext))
	{
		return false;
	}

	typedef BOOL(APIENTRY *WGLSWAPINTERVALEXT)(int);
	WGLSWAPINTERVALEXT wglSwapIntervalEXT = (WGLSWAPINTERVALEXT)wglGetProcAddress("wglSwapIntervalEXT");
	wglSwapIntervalEXT(0);

	if (glewInit() != GLEW_OK)
	{
		return false;
	}

	return true;
}

int main(int argc, char* argv[])
{
	WNDCLASSEX wndClass;
	ZeroMemory(&wndClass, sizeof(WNDCLASSEX));
	wndClass.cbSize = sizeof(WNDCLASSEX);
	wndClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
	wndClass.lpfnWndProc = windowCallBack;
	//wndClass.hInstance = instance;
	wndClass.lpszClassName = L"SSR WINDOW";

	if (!RegisterClassEx(&wndClass))
	{
		std::cerr << "failed to create window class" << std::endl;
		return 1;
	}

	DWORD windowStyle = WS_OVERLAPPEDWINDOW;

	clientWidth = SCREEN_WIDTH;
	clientHeight = SCREEN_HEIGHT;

	RECT r = { 0, 0, clientWidth, clientHeight };
	AdjustWindowRect(&r, windowStyle, FALSE);
	int32 width = r.right - r.left;
	int32 height = r.bottom - r.top;

	HWND windowHandle = CreateWindowEx(0, wndClass.lpszClassName, L"SSR", windowStyle,
		CW_USEDEFAULT, CW_USEDEFAULT, width, height,
		0, 0, 0, 0);

	if (!windowHandle)
	{
		std::cerr << "failed to create window" << std::endl;
		return 1;
	}

	HDC windowDC = GetDC(windowHandle);

	if (!initializeOpenGL(windowDC))
	{
		std::cerr << "failed to initialize opengl" << std::endl;
		return 1;
	}

	ShowWindow(windowHandle, SW_SHOW);

	opengl_renderer renderer;
	initializeRenderer(renderer, clientWidth, clientHeight);

	scene_state scenes[SCENE_COUNT];
	for (uint32 i = 0; i < SCENE_COUNT; ++i)
		initializeScene(scenes[i], (scene_name)i, clientWidth, clientHeight);

	LARGE_INTEGER perfFreqResult;
	QueryPerformanceFrequency(&perfFreqResult);
	int64 perfFreq = perfFreqResult.QuadPart;


	raw_input input[2] = { 0 };
	raw_input* lastInput = &input[0];
	raw_input* curInput = &input[1];


	LARGE_INTEGER lastTime;
	QueryPerformanceCounter(&lastTime);
	float secondsElapsed = 1.f / 60.f;

	running = true;
	while (running)
	{
		{	//TIMED_BLOCK("input")
			*curInput = {};
			for (int buttonIndex = 0; buttonIndex < KB_BUTTONCOUNT; ++buttonIndex)
			{
				curInput->keyboard.buttons[buttonIndex].isDown =
					lastInput->keyboard.buttons[buttonIndex].isDown;
				curInput->keyboard.buttons[buttonIndex].wasDown =
					lastInput->keyboard.buttons[buttonIndex].isDown;
			}
			curInput->mouse.x = lastInput->mouse.x;
			curInput->mouse.y = lastInput->mouse.y;
			curInput->mouse.relX = lastInput->mouse.relX;
			curInput->mouse.relY = lastInput->mouse.relY;
			curInput->mouse.left.isDown = lastInput->mouse.left.isDown;
			curInput->mouse.right.isDown = lastInput->mouse.right.isDown;
			curInput->mouse.right.isDown = lastInput->mouse.right.isDown;
			curInput->mouse.left.wasDown = lastInput->mouse.left.isDown;
			curInput->mouse.right.wasDown = lastInput->mouse.right.isDown;
			curInput->mouse.right.wasDown = lastInput->mouse.right.isDown;
		}

		{	//TIMED_BLOCK("messages")
			processPendingMessages(clientWidth, clientHeight, *lastInput, *curInput);
			for (uint32 i = 0; i < SCENE_COUNT; ++i)
			{
				if (buttonDownEvent(*curInput, (kb_button)(KB_1 + i)))
				{
					currentScene = i;
				}
			}
		}

		{	//TIMED_BLOCK("update and render")
			updateScene(scenes[currentScene], *curInput, secondsElapsed);
			renderScene(renderer, scenes[currentScene], clientWidth, clientHeight);
		}

		{	//TIMED_BLOCK("rest")
			SwapBuffers(windowDC);
			std::swap(lastInput, curInput);

			LARGE_INTEGER currentTime;
			QueryPerformanceCounter(&currentTime);
			secondsElapsed = ((float)(currentTime.QuadPart - lastTime.QuadPart) / (float)perfFreq);
			lastTime = currentTime;
			float fps = 1.f / secondsElapsed;

			char titleBuffer[50];
			sprintf(titleBuffer, "SSR --- FPS: %f, %fms", fps, secondsElapsed * 1000.f);
			SetWindowTextA(windowHandle, titleBuffer);
		}
		globalTimer.printTimedBlocks();
	}

	for (uint32 i = 0; i < SCENE_COUNT; ++i)
		cleanupScene(scenes[i]);

	cleanupRenderer(renderer);
}

uint64 getFileWriteTime(const char* filename)
{
	ULARGE_INTEGER create, access, write;
	HANDLE fileHandle = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	write.QuadPart = 0;
	if (fileHandle != INVALID_HANDLE_VALUE) {
		GetFileTime(fileHandle, LPFILETIME(&create), LPFILETIME(&access), LPFILETIME(&write));
	}
	CloseHandle(fileHandle);
	return (uint64)write.QuadPart;
}

input_file readFile(const char* filename)
{
	input_file result = { 0 };
	result.filename = filename;
	HANDLE fileHandle = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	if (fileHandle != INVALID_HANDLE_VALUE)
	{
		LARGE_INTEGER fileSize;
		if (GetFileSizeEx(fileHandle, &fileSize))
		{
			uint32 fileSize32 = (uint32)(fileSize.QuadPart);
			result.contents = VirtualAlloc(0, fileSize32, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
			if (result.contents)
			{
				DWORD bytesRead;
				if (ReadFile(fileHandle, result.contents, fileSize32, &bytesRead, 0) &&
					(fileSize32 == bytesRead))
				{
					result.size = fileSize32;
				}
				else
				{
					freeFile(result);
					result.contents = nullptr;
				}
			}
			else
			{
			}
		}
		else
		{
		}

		CloseHandle(fileHandle);
	}
	else
	{
	}

	return result;
}

void freeFile(input_file file)
{
	if (file.contents)
	{
		VirtualFree(file.contents, 0, MEM_RELEASE);
	}
}

timer::timer()
{
	LARGE_INTEGER perfFreqResult;
	QueryPerformanceFrequency(&perfFreqResult);
	perfFreq = (double)perfFreqResult.QuadPart;

	blocks = (timed_block*)malloc(sizeof(timed_block) * 20);
}

timer::~timer()
{
	free(blocks);
}

void timer::add(timed_block* block)
{
	blocks[numberOfTimedBlocks++] = *block;
}

void timer::printTimedBlocks()
{
	for (uint32 i = 0; i < numberOfTimedBlocks; ++i)
	{
		double duration = ((double)(blocks[i].end - blocks[i].start) / perfFreq) * 1000.0;
		std::cout << blocks[i].function << " - " << blocks[i].name << " (" << blocks[i].line << "): " << duration << "ms" << std::endl;
	}
	numberOfTimedBlocks = 0;
}