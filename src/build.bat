@echo off
if not defined DevEnvDir (
	call vcvarsall x64
)

setlocal EnableDelayedExpansion

set OneFile=/Fe"D3D12 Learn" /Fd"D3D12 Learn"
set CommonCompFlags=/std:c++20 -fp:fast -nologo -MT -EHsc -Od -Oi -WX- -W4 -GR- -Gm- -GS -FC -Zi -D_MBCS -wd4100 -wd4189 -wd4201 -wd4238 -wd4244 -wd4267
set CommonLinkFlags=-opt:ref -incremental:no /SUBSYSTEM:console

set CppFiles="..\src\main.cpp"
rem set CppFiles= 
rem for /R %%f in (*.cpp) do (
rem 	set CppFiles=!CppFiles! "%%f"
rem )

dxc.exe "..\shaders\mesh.vert.hlsl" -O2 -E main -T vs_6_6 -enable-16bit-types -Fo "..\build\mesh.vert.cso"
dxc.exe "..\shaders\mesh_back_cull.geom.hlsl" -O2 -E main -T gs_6_6 -enable-16bit-types -Fo "..\build\mesh_back_cull.geom.cso"
dxc.exe "..\shaders\mesh.frag.hlsl" -O2 -E main -T ps_6_6 -Fo "..\build\mesh.frag.cso"

dxc.exe "..\shaders\indirect_cull_frust.comp.hlsl" -O2 -E main -T cs_6_6 -Fo "..\build\indirect_cull_frust.comp.cso"
dxc.exe "..\shaders\indirect_cull_occl.comp.hlsl" -O2 -E main -T cs_6_6 -Fo "..\build\indirect_cull_occl.comp.cso"
dxc.exe "..\shaders\depth_reduce.comp.hlsl" -O2 -E main -T cs_6_6 -Fo "..\build\depth_reduce.comp.cso"

if not exist ..\build\ mkdir ..\build\
pushd ..\build\
cl %CommonCompFlags% user32.lib kernel32.lib d3d12.lib dxgi.lib dxguid.lib d3dcompiler.lib dxcompiler.lib %CppFiles% %OneFile% /link %CommonLinkFlags%
popd
