@echo off


REM PhysX DLLのパス設定
set PhysX_DLLs="PhysX\PhysX-5.5.1\bin\x64"

REM x64/Debug と x64/Release フォルダを作成
mkdir "%~dp0\x64\Debug"
mkdir "%~dp0\x64\Release"

REM 実行ファイルと同じディレクトリにDLLをコピー（DebugまたはReleaseを選択）
copy "%PhysX_DLLs%\Debug\*.dll" "%~dp0\x64\Debug\"
copy "%PhysX_DLLs%\Release\*.dll" "%~dp0\x64\Release\"

REM Visual Studio で .sln ファイルを開く
start /min "" "BasicEngine.sln"
exit