@echo off
@set SDL_STDIO_REDIRECT=0
@set CUR_DIR=%~dp0

cd %CUR_DIR%\..

:RunQemu
start cmd /c "chcp 65001 && make qemudbg"