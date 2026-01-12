IF EXIST "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" (
	call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
) ELSE (
	call "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common\7\Tools\VsDevCmd.bat"
)

cmake -G "Visual Studio 17 2022" -S . -B cmake-build-vs2022 -DCMAKE_TOOLCHAIN_FILE="..\vcpkg\scripts\buildsystems\vcpkg.cmake"