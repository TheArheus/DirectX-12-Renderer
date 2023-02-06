#include "intrinsics.h"
#include "mesh.cpp"
#include "renderer_directx12.cpp"
#include "win32_window.cpp"


int WinMain(HINSTANCE CurrInst, HINSTANCE PrevInst, PSTR Cmd, int Show)
{	
	window DirectWindow(1240, 720, "3D Renderer");
	d3d_app Graphics(DirectWindow.Handle, DirectWindow.Width, DirectWindow.Height);

	mesh Mesh;
	Mesh.Load("..\\assets\\kitten.obj");

	float val = 3.14159273;
	float exp = val - floorf(3.14159273);
	u8 res = 127 + floorf(3.14159273);

	Graphics.OnInit(Mesh);

	while(DirectWindow.IsRunning())
	{
		auto Result = DirectWindow.ProcessMessages();
		if(Result) return *Result;

		Graphics.BeginRender();
		Graphics.DrawIndexed(Mesh);
		Graphics.EndRender();
	}

	return 0;
}

int main(int argc, char* argv[])
{
	return WinMain(0, 0, argv[0], SW_SHOWNORMAL);
}

