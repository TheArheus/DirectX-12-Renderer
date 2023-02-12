#include "intrinsics.h"
#include "mesh.cpp"
#include "renderer_directx12.cpp"
#include "win32_window.cpp"


int WinMain(HINSTANCE CurrInst, HINSTANCE PrevInst, PSTR Cmd, int Show)
{	
	window DirectWindow(1240, 720, "3D Renderer");
	DirectWindow.InitGraphics();

	mesh KittenMesh("..\\assets\\kitten.obj");
	mesh BunnyMesh("..\\assets\\stanford-bunny.obj");
	mesh Geometries({"..\\assets\\kitten.obj", "..\\assets\\stanford-bunny.obj"});

	memory_heap VertexHeap(MiB(10));
	memory_heap IndexHeap(MiB(10));

#if 0
	buffer VertexBuffer = VertexHeap.PushBuffer(DirectWindow.Gfx, KittenMesh.Vertices.data(), KittenMesh.Vertices.size() * sizeof(vertex));
	buffer IndexBuffer = IndexHeap.PushBuffer(DirectWindow.Gfx, KittenMesh.VertexIndices.data(), KittenMesh.VertexIndices.size() * sizeof(u32));
	DirectWindow.Gfx->PushIndexedVertexData(&VertexBuffer, &IndexBuffer, KittenMesh.Offsets);

	buffer VertexBuffer1 = VertexHeap.PushBuffer(DirectWindow.Gfx, BunnyMesh.Vertices.data(), BunnyMesh.Vertices.size() * sizeof(vertex));
	buffer IndexBuffer1 = IndexHeap.PushBuffer(DirectWindow.Gfx, BunnyMesh.VertexIndices.data(), BunnyMesh.VertexIndices.size() * sizeof(u32));
	DirectWindow.Gfx->PushIndexedVertexData(&VertexBuffer1, &IndexBuffer1, BunnyMesh.Offsets);
#else
	buffer VertexBuffer = VertexHeap.PushBuffer(DirectWindow.Gfx, Geometries.Vertices.data(), Geometries.Vertices.size() * sizeof(vertex));
	buffer IndexBuffer = IndexHeap.PushBuffer(DirectWindow.Gfx, Geometries.VertexIndices.data(), Geometries.VertexIndices.size() * sizeof(u32));
	DirectWindow.Gfx->PushIndexedVertexData(&VertexBuffer, &IndexBuffer, Geometries.Offsets);
#endif

	double TargetFrameRate = 1.0 / 60 * 1000.0; // Frames Per Milliseconds

	double TimeLast = window::GetTimestamp();
	double AvgCpuTime = 0.0;
	while(DirectWindow.IsRunning())
	{
		auto Result = DirectWindow.ProcessMessages();
		if(Result) return *Result;

		if(!DirectWindow.IsGfxPaused)
		{
			DirectWindow.Gfx->BeginRender();
			DirectWindow.Gfx->DrawIndexed();
			DirectWindow.Gfx->EndRender();
		}

		double TimeEnd = window::GetTimestamp();
		double TimeElapsed = (TimeEnd - TimeLast);
		if(TimeElapsed < TargetFrameRate)
		{
			while(TimeElapsed < TargetFrameRate)
			{
				double TimeToSleep = TargetFrameRate - TimeElapsed;
				if (TimeToSleep > 0) 
				{
					Sleep(TimeToSleep);
				}
				TimeEnd = window::GetTimestamp();
				TimeElapsed = TimeEnd - TimeLast;
			}
		}

		TimeLast = TimeEnd;
		AvgCpuTime = 0.75 * AvgCpuTime + TimeElapsed * 0.25;

		//std::string Title = "Frame " + std::to_string(1.0 / AvgCpuTime * 1000.0) + "fps";
		std::string Title = "Frame " + std::to_string(AvgCpuTime) + "ms";
		DirectWindow.SetTitle(Title);
	}

	return 0;
}

int main(int argc, char* argv[])
{
	return WinMain(0, 0, argv[0], SW_SHOWNORMAL);
}

