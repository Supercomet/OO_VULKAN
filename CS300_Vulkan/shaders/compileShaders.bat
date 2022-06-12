@echo off
SETLOCAL EnableDelayedExpansion


for %%i in (*.vert  *.frag) do (
	rem this is full accuracy
	rem forfiles /M "%%~i.spv" /C "cmd /c set time=@ftime set date=@fdate 
	rem echo !time!
	rem echo !date!
	rem echo "%%~ti"
	for /f "tokens=1-6 delims=:0/,. " %%A in ("%%~ti") do (
	set /A "Day=%%A"
	set /A "Month=%%B"
	set /A "Year=%%C"
	set /A "Hour=%%D"
	set /A "Min=%%E"
	)
	rem echo day !Day!
	rem echo month !Month!
	rem echo year !Year!	
	rem echo hours !Hour!
	rem echo min !Min!
	set /A compile=0
	for %%j in ("%%~i.spv") do (
	 IF exist "%%~j" (
				rem echo %%~tj				
				for /f "tokens=1-6 delims=:0/,. " %%A in ("%%~tj") do (
				set /A "jDay=%%A"
				set /A "jMonth=%%B"
				set /A "jYear=%%C"
				set /A "jHour=%%D"
				set /A "jMin=%%E"
				)
				rem echo jday !jDay!
				rem echo jmonth !jMonth!
				rem echo jyear !jYear!	
				rem echo jhours !jHour!
				rem echo jmin !jMin!
				
				IF !Year! LSS !jYear! (set /A compile=0) else (
					IF !Month! LSS !jMonth! (set /A compile=0) else (
						IF !Day! LSS !jDay! (set /A compile=0) else (
							IF !Hour! LSS !jHour! (set /A compile=0) else (
								IF !Min! LSS !jMin! (set /A compile=0) else (set /A compile=1)
							)
						)
					)
				)
				
		) else (echo does not exist!)
		
	set numberstring=                      %%~i
	if !compile! EQU 1 ( 	
	"%VULKAN_SDK%\Bin\glslc.exe" --target-env=vulkan1.2 "%%~i" -o "%%~i.spv"
	if !ERRORLEVEL! NEQ 0 (echo ["%%~i" COMPILATION ERROR] 
		echo.) else ( echo !numberstring:~-28! compiled )	
	)else (echo !numberstring:~-28! ^| unchanged no compilation)
	
	)
	
	rem echo.
 )
 echo.
 echo Finished
 echo.


rem forfiles /s /m *.vert /c "cmd /c %VULKAN_SDK%/Bin/glslangValidator.exe -V @path -o @fname.vert.spv"
rem forfiles /s /m *.frag /c "cmd /c %VULKAN_SDK%/Bin/glslangValidator.exe -V @path -o @fname.frag.spv"
rem pause