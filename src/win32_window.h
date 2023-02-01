
#ifndef WIN32_WINDOWS_H_
#include <windows.h>
#include <optional>
#include <string>
#include <vector>


class window
{
private:
	struct window_class
	{
		window_class()
			: Inst(GetModuleHandle(nullptr))
		{
			WNDCLASS Class = {};
			Class.lpszClassName = Name;
			Class.hInstance = Inst;
			Class.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
			Class.lpfnWndProc = InitWindowProc;

			RegisterClass(&Class);
		}
		~window_class()
		{
			UnregisterClass(Name, Inst);
		}

		HINSTANCE Inst;
		bool IsRunning = true;
		const char* Name = "Renderer Engine";

		std::vector<std::string> WindowNames;
	};

public:
	window(unsigned int Width, unsigned int Height, const char* Name);
	window(window&& rhs) = default;
	window& operator=(window&& rhs) = default;
	~window()
	{
		DestroyWindow(Handle);
		WindowClass.WindowNames.erase(std::remove(WindowClass.WindowNames.begin(), WindowClass.WindowNames.end(), Name), WindowClass.WindowNames.end());
		if(WindowClass.WindowNames.size() == 0)
		{
			WindowClass.IsRunning = false;
		}
	}
	static std::optional<int> ProcessMessages();

	void SetTitle(std::string& Title);
	bool IsRunning(){return WindowClass.IsRunning;}

	HWND Handle;
	const char* Name;
	unsigned int Width;
	unsigned int Height;

private:
	window(const window& rhs) = delete;
	window& operator=(const window& rhs) = delete;

	static LRESULT InitWindowProc(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam);
	static LRESULT WindowProc(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam);
	LRESULT DispatchMessages(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam);

	static window_class WindowClass;
};

#define WIN32_WINDOWS_H_
#endif
