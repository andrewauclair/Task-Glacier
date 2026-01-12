IF EXIST "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" (
	call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
) ELSE (
	call "C:\Program Files\Microsoft Visual Studio\18\Professional\Common\7\Tools\VsDevCmd.bat"
)

cmake -G "Visual Studio 18 2026" -S . -B cmake-build-vs2026 -DCMAKE_TOOLCHAIN_FILE="..\vcpkg\scripts\buildsystems\vcpkg.cmake" -DCMAKE_POLICY_VERSION_MINIMUM=3.5