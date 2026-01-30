@echo off
setlocal enabledelayedexpansion

echo ========================================
echo Скрипт сборки и запуска
echo ========================================

echo.
echo Обновление из GitHub...
git pull origin main 2>nul || echo Git пропущен (нет репозитория или интернета)

if not exist lib\sqlite3\sqlite3.c (
    echo.
    echo Установка SQLite3...
    if not exist lib\sqlite3 mkdir lib\sqlite3
    
    powershell -Command "$progressPreference = 'SilentlyContinue'; Invoke-WebRequest -Uri 'https://www.sqlite.org/2024/sqlite-amalgamation-3450000.zip' -OutFile 'sqlite.zip'"
    
    if exist sqlite.zip (
        powershell -Command "$progressPreference = 'SilentlyContinue'; Expand-Archive -Path 'sqlite.zip' -DestinationPath 'sqlite_tmp' -Force"
        copy "sqlite_tmp\sqlite-amalgamation-3450000\sqlite3.c" "lib\sqlite3\" >nul
        copy "sqlite_tmp\sqlite-amalgamation-3450000\sqlite3.h" "lib\sqlite3\" >nul
        rmdir /s /q sqlite_tmp
        del sqlite.zip
        echo SQLite3 успешно загружен.
    ) else (
        echo Внимание: Не удалось загрузить SQLite3.
    )
)

echo.
echo Сборка C++ компонентов...
if not exist build mkdir build
cd build
cmake .. -G "MinGW Makefiles" || cmake ..
cmake --build .
cd ..

echo.
echo Установка Python зависимостей...
pip install -q -r web_client\requirements.txt >nul 2>&1 || python -m pip install -q -r web_client\requirements.txt >nul 2>&1 || echo Внимание: Не удалось установить пакеты Python. Установите вручную: pip install flask requests
echo.
echo Остановка старых процессов...
taskkill /F /IM simulator.exe /T >nul 2>&1
taskkill /F /IM server.exe /T >nul 2>&1
taskkill /F /FI "WINDOWTITLE eq Python*" /FI "COMMANDLINE eq *app.py*" /T >nul 2>&1
timeout /t 1 /nobreak >nul

echo.
echo ========================================
echo Запуск компонентов...
echo ========================================

echo Запуск симулятора...
start "Simulator" /min build\simulator.exe
timeout /t 1 /nobreak >nul

echo Запуск HTTP сервера (порт 8080)...
start "Server" /min build\server.exe
timeout /t 2 /nobreak >nul

echo Запуск веб-клиента (порт 5010)...
start "WebClient" /min python web_client\app.py
timeout /t 2 /nobreak >nul

echo.
echo ========================================
echo Все компоненты запущены
echo ========================================
echo.
echo Откройте в браузере:
echo   http://localhost:5010
echo.
echo ========================================
echo.

timeout /t 1 /nobreak >nul
start http://localhost:5010

pause
