@echo off
setlocal EnableExtensions

set "COMMAND=C:\VulkanSDK\1.3.250.1\Bin\glslc.exe %%~fF -o %CD%/../app/src/main/assets/shaders/%%~nxF.spv"

for %%F in (*) do (
	if not exist "%%F\"  (
		if /i not "%%~xF"==".bat" (
			echo Processing "%%~fF"
			%COMMAND% 
		)
	)
)

pause

endlocal
