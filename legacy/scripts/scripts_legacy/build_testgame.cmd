:logln
echo "[!] :: " %1
goto:eof

:logerr
echo "[!ERR] :: " %1
goto:eof

call :logln "Build started."
call :logln "Using MSBuild.exe for building..."
	
if not exist ../RSE.sln (
	call :logln "Generating Visual Studio files..."
	call "erase_sln.cmd"
	call "generate_vs_sol.cmd"
	call :logln "Visual Studio build file generation finished."
)

call :logln "Calling MSBuild..."
msbuild ../RSE.sln
if %ERRORLEVEL% NEQ 0 (
	call :logerr "Build failed! Aborting..."
	exit
) else (
	call :logln "Build finished."
	call :logln "Executing program DEBUG..."
	cd ../TestGame/Debug
	TestGame.exe
	set exit_code=%errorlevel%
	call :logln "Program existed with code: " ^& %errorlevel%
)
