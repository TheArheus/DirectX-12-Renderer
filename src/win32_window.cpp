
window::window_class window::WindowClass;
LARGE_INTEGER window::TimerFrequency;

window::window(unsigned int _Width, unsigned int _Height, const char* _Name)
	: Width(_Width), Height(_Height), Name(_Name)
{
	WindowClass.IsRunning = true;
	WindowClass.WindowNames.push_back(_Name);

	RECT AdjustRect = {};
	AdjustRect.left = 100;
	AdjustRect.top = 100;
	AdjustRect.right = AdjustRect.left + Width;
	AdjustRect.bottom = AdjustRect.top + Height;

	AdjustWindowRect(&AdjustRect, WS_OVERLAPPEDWINDOW & (~WS_THICKFRAME), 0);

	Handle = CreateWindow(WindowClass.Name, Name, WS_OVERLAPPEDWINDOW & (~WS_THICKFRAME), CW_USEDEFAULT, CW_USEDEFAULT, AdjustRect.right - AdjustRect.left, AdjustRect.bottom - AdjustRect.top, 0, 0, WindowClass.Inst, this);
	ShowWindow(Handle, SW_SHOWNORMAL);

	QueryPerformanceFrequency(&TimerFrequency);
}

LRESULT window::InitWindowProc(HWND hWindow, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch(Message)
	{
		case WM_NCCREATE:
		{
			CREATESTRUCT* DataOnCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
			window* NewWindow = reinterpret_cast<window*>(DataOnCreate->lpCreateParams);
			SetWindowLongPtr(hWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&window::WindowProc));
			SetWindowLongPtr(hWindow, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(NewWindow));
			return NewWindow->DispatchMessages(hWindow, Message, wParam, lParam);
		} break;
	}

	return DefWindowProc(hWindow, Message, wParam, lParam);
}

LRESULT window::WindowProc(HWND hWindow, UINT Message, WPARAM wParam, LPARAM lParam)
{
	window* Window = reinterpret_cast<window*>(GetWindowLongPtr(hWindow, GWLP_USERDATA));
	return Window->DispatchMessages(hWindow, Message, wParam, lParam);
}

LRESULT window::DispatchMessages(HWND hWindow, UINT Message, WPARAM wParam, LPARAM lParam)
{
	if(strcmp(Name, WindowClass.WindowNames[0].c_str()) == 0)
	{
		switch(Message)
		{
#if 0
			// TODO: Consider to do in the future
			// correct window resizing
			case WM_ENTERSIZEMOVE:
			{
				IsGfxPaused = true;
				return 0;
			} break;
			case WM_EXITSIZEMOVE:
			{
				IsGfxPaused = false;
				if (Gfx)
				{
					Gfx->RecreateSwapchain(Width, Height);
				}
				return 0;
			} break;
			case WM_SIZE:
			{
				Width  = LOWORD(lParam);
				Height = HIWORD(lParam);

				return 0;
			} break;
#endif
			case WM_CLOSE:
			{
				PostQuitMessage(0);
				return 0;
			} break;
		}
	}

	return DefWindowProc(hWindow, Message, wParam, lParam);
}

std::optional<int> window::ProcessMessages()
{
	MSG Message = {};
	while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
	{
		if(Message.message == WM_QUIT) 
		{
			WindowClass.IsRunning = false;
			return int(Message.wParam);
		}
		TranslateMessage(&Message);
		DispatchMessage(&Message);
	}

	return {};
}

void window::SetTitle(std::string& Title)
{
	SetWindowTextA(Handle, (std::string(Name) + " - " + Title).c_str());
}

void window::InitGraphics()
{
	Gfx = std::make_unique<renderer_backend>(Handle, Width, Height);
}

// Returns time in milliseconds
double window::GetTimestamp()
{
	LARGE_INTEGER Time;
	QueryPerformanceCounter(&Time);

	return double(Time.QuadPart) / double(TimerFrequency.QuadPart) * 1000.0;
}

