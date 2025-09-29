@echo off


REM ZIP形式で圧縮するフォルダ(またはファイル)パスを指定

set targetPath=PhysX\PhysX-5.5.1\bin\x64


REM 解凍(展開)先フォルダを指定

set zipFilePath=PhysX\PhysX-5.5.1\bin\zip\Debug.zip
set zipFilePath2=PhysX\PhysX-5.5.1\bin\zip\Release.zip

rem 実行するPowerShellのコマンドレットを組み立て
set psCommand0=powershell -command chcp 65001
set psCommand=powershell -NoProfile -ExecutionPolicy Unrestricted Expand-Archive -Path %zipFilePath% -DestinationPath %targetPath% -Force
set psCommand2=powershell -NoProfile -ExecutionPolicy Unrestricted Expand-Archive -Path %zipFilePath2% -DestinationPath %targetPath% -Force

makedir "PhysX\PhysX-5.5.1\bin\x64"
rem PowerShellのコマンドレットを実行
%psCommand0%
%psCommand%
%psCommand2%

rd /s /q "PhysX\PhysX-5.5.1\bin\zip"
rem 実行結果を確認
if %errorlevel% == 0 (
    echo 正常終了しました。戻り値：%errorlevel%
) else (
    echo 異常終了しました。戻り値：%errorlevel%
)

exit 