#include "intrinsics.h"
#include "mesh.cpp"
#include "renderer_directx12.cpp"
#include "win32_window.cpp"


int WinMain(HINSTANCE CurrInst, HINSTANCE PrevInst, PSTR Cmd, int Show)
{	
	window DirectWindow(1240, 720, "3D Renderer");
	DirectWindow.InitGraphics();

	mesh Mesh;
	Mesh.Load("..\\assets\\kitten.obj");

	DirectWindow.Gfx->OnInit(Mesh);

	while(DirectWindow.IsRunning())
	{
		double TimeBeg = window::GetTimestamp();

		auto Result = DirectWindow.ProcessMessages();
		if(Result) return *Result;

		if(!DirectWindow.IsGfxPaused)
		{
			DirectWindow.Gfx->BeginRender();
			DirectWindow.Gfx->DrawIndexed(Mesh);
			DirectWindow.Gfx->EndRender();
		}

		double TimeEnd = window::GetTimestamp();

		std::string Title = "Frame " + std::to_string(TimeEnd - TimeBeg) + "ms";
		DirectWindow.SetTitle(Title);
	}

	return 0;
}

int main(int argc, char* argv[])
{
	return WinMain(0, 0, argv[0], SW_SHOWNORMAL);
}

