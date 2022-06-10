rem %VULKAN_SDK%/Bin/glslangValidator.exe -V shader.vert
rem %VULKAN_SDK%/Bin/glslangValidator.exe -V shader.frag
for %%i in (*.vert *.frag) do (
 "%VULKAN_SDK%\Bin\glslc.exe" --target-env=vulkan1.2 "%%~i" -o "%%~i.spv" 
 )

rem forfiles /s /m *.vert /c "cmd /c %VULKAN_SDK%/Bin/glslangValidator.exe -V @path -o @fname.vert.spv"
rem forfiles /s /m *.frag /c "cmd /c %VULKAN_SDK%/Bin/glslangValidator.exe -V @path -o @fname.frag.spv"
pause