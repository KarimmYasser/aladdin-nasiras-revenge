@echo off
setlocal enabledelayedexpansion

REM ==========================================
REM CRASH BANDICOOT PHYSICS TEST RUNNER
REM ==========================================

cd /D "%~dp0"
set APP_EXE=..\bin\GAME_APPLICATION.exe
set DEFAULT_CONFIG=..\config\app.jsonc
set BACKUP_CONFIG=..\config\app.jsonc.bak

echo ==========================================
echo STARTING PHYSICS TESTS
echo ==========================================

REM Backup current config if it exists
if exist "%DEFAULT_CONFIG%" (
    copy /y "%DEFAULT_CONFIG%" "%BACKUP_CONFIG%" >nul
)

set tests[1]=test1-gravity.jsonc
set tests[2]=test2-restitution.jsonc
set tests[3]=test3-friction.jsonc
set tests[4]=test4-shapes.jsonc
set tests[5]=test5-bodytypes.jsonc
set tests[6]=test6-trigger.jsonc

for /l %%i in (1,1,6) do (
    echo.
    echo ==========================================
    echo RUNNING TEST %%i: !tests[%%i]!
    echo ==========================================

    REM Overwrite default config with test so the game loads it automatically
    copy /y "..\config\physics-test\!tests[%%i]!" "%DEFAULT_CONFIG%" >nul

    echo Press any key to launch the test.
    echo (CLOSE THE GAME WINDOW to proceed to the next test!)
    pause >nul

    REM Run the game
    start "" /wait "%APP_EXE%"
)

echo.
echo ==========================================
echo ALL TESTS COMPLETED. RESTORING CONFIG...
echo ==========================================
if exist "%BACKUP_CONFIG%" (
    copy /y "%BACKUP_CONFIG%" "%DEFAULT_CONFIG%" >nul
    del "%BACKUP_CONFIG%"
)

echo Done! Press any key to exit.
pause >nul
