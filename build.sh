#!/bin/bash

echo "========================================"
echo "Скрипт сборки и запуска"
echo "========================================"

echo
echo "Обновление из GitHub..."
git pull origin main 2>/dev/null || echo "Git пропущен (нет репозитория или интернета)"

if [ ! -f "lib/sqlite3/sqlite3.c" ]; then
    echo
    echo "Установка SQLite3..."
    
    SQLITE_YEAR="2025"
    SQLITE_VER="3490000"
    SQLITE_ZIP="sqlite-amalgamation-${SQLITE_VER}.zip"
    SQLITE_URL="https://www.sqlite.org/${SQLITE_YEAR}/${SQLITE_ZIP}"
    TARGET_DIR="lib/sqlite3"
    
    mkdir -p "${TARGET_DIR}"
    
    if command -v curl &> /dev/null; then
        curl -o "${SQLITE_ZIP}" "${SQLITE_URL}" 2>/dev/null
    elif command -v wget &> /dev/null; then
        wget -O "${SQLITE_ZIP}" "${SQLITE_URL}" 2>/dev/null
    else
        echo "Ошибка: curl или wget не найдены."
        exit 1
    fi
    
    if [ ! -f "${SQLITE_ZIP}" ]; then
        echo "Попытка альтернативной версии..."
        SQLITE_URL="https://www.sqlite.org/2024/sqlite-amalgamation-3450000.zip"
        if command -v curl &> /dev/null; then 
            curl -o "${SQLITE_ZIP}" "${SQLITE_URL}" 2>/dev/null
        else 
            wget -O "${SQLITE_ZIP}" "${SQLITE_URL}" 2>/dev/null
        fi
    fi
    
    if [ -f "${SQLITE_ZIP}" ]; then
        unzip -j "${SQLITE_ZIP}" "*/sqlite3.c" "*/sqlite3.h" -d "${TARGET_DIR}" > /dev/null 2>&1
        rm "${SQLITE_ZIP}"
        echo "SQLite3 установлен"
    else
        echo "Не удалось загрузить SQLite."
        exit 1
    fi
fi

echo
echo "Сборка C++ компонентов..."
mkdir -p build
cd build
cmake ..
make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 2)
cd ..

echo
echo "Установка Python зависимостей..."
pip3 install -q -r web_client/requirements.txt 2>/dev/null || \
python3 -m pip install -q -r web_client/requirements.txt 2>/dev/null || \
echo "Внимание: не удалось установить пакеты Python. Установите вручную: pip3 install flask requests"
echo
echo "Остановка старых процессов..."
pkill -f "build/simulator" 2>/dev/null || true
pkill -f "build/server" 2>/dev/null || true
pkill -f "web_client/app.py" 2>/dev/null || true
sleep 1

echo
echo "========================================"
echo "Запуск компонентов..."
echo "========================================"

cd build

echo "Запуск симулятора..."
./simulator > /dev/null 2>&1 &
SIMULATOR_PID=$!
echo "  PID симулятора: $SIMULATOR_PID"
sleep 1

echo "Запуск HTTP сервера (порт 8080)..."
./server > /dev/null 2>&1 &
SERVER_PID=$!
echo "  PID сервера: $SERVER_PID"
sleep 2

cd ..

echo "Запуск веб-клиента (порт 5010)..."
python3 web_client/app.py > /dev/null 2>&1 &
WEB_PID=$!
echo "  PID веб-клиента: $WEB_PID"
sleep 2

echo
echo "========================================"
echo "Все компоненты запущены"
echo "========================================"
echo
echo "Откройте в браузере:"
echo "  http://localhost:5010"
echo
echo "Для остановки:"
echo "  kill $SIMULATOR_PID $SERVER_PID $WEB_PID"
echo
echo "========================================"
echo
