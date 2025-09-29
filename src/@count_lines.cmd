:: 全ソースの行数を数える
@echo off
@setlocal enabledelayedexpansion
chcp 65001 > nul

cd /d "%~dp0"
echo 【ソースコード行数】

pushd "%~dp0src"

set total=0
for /f "usebackq" %%i in (`dir /b /ad %dir%`) do (
pushd %%i
	@echo | set /p="%%i = "
	call :line_count
	set /a total+=total_lines
popd
)

popd

echo ------------------------------------------------------------
echo total %total% lines
echo.

pause

goto end

::---------------------------------------------------------------
:: 行数をカウント
::---------------------------------------------------------------
:line_count
:: 行数
set total_lines=0

for /r %%i in (*.c *.h *.cpp *.hpp *.cxx *.hxx *.inl) do (
	set lines=
	for /f %%a in ('type %%i ^| find /c /v ""') do set /a lines=%%a
	set /a total_lines+=lines
)
echo %total_lines% lines

:end











