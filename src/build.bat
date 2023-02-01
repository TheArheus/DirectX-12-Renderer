@echo off
if not defined DevEnvDir (
	call vcvarsall x64
)

setlocal EnableDelayedExpansion

set OneFile=/Fe"D3D12 Learn" /Fd"D3D12 Learn"
set CommonCompFlags=/std:c++20 -nologo -MTd -EHsc -Od -WX- -W4 -GR- -Gm- -GS -FC -Z7 -D_MBCS -wd4100 -wd4189 -wd4238
set CommonLinkFlags=-opt:ref -incremental:no /SUBSYSTEM:windows

set CppFiles=
for /R %%f in (*.cpp) do (
	set CppFiles=!CppFiles! "%%f"
)

dxc.exe "..\shaders\mesh.vert.hlsl" -E main -T vs_6_6 -Fo "..\build\mesh.vert.cso"
dxc.exe "..\shaders\mesh.frag.hlsl" -E main -T ps_6_6 -Fo "..\build\mesh.frag.cso"
if not exist ..\build\ mkdir ..\build\
pushd ..\build\
cl %CommonCompFlags% user32.lib kernel32.lib d3d12.lib dxgi.lib dxguid.lib d3dcompiler.lib dxcompiler.lib %CppFiles% %OneFile% /link %CommonLinkFlags%
popd
