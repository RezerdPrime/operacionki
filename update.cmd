@echo off
setlocal enabledelayedexpansion

echo ========================================
echo GitHub Update Script for C++/CMake Project
echo ========================================

rem Настройте эти переменные под ваш проект
set "REPO_PATH=C:\Users\Lenovo\Desktop\CodeShit\operacionki"  rem Путь к репозиторию (текущая директория)
set "BRANCH=main"  rem Ветка по умолчанию
set "COMMIT_MESSAGE=Update project files"

rem Проверка наличия Git
where git >nul 2>nul
if errorlevel 1 (
    echo ERROR: Git не установлен или не добавлен в PATH
    pause
    exit /b 1
)

rem Переход в директорию репозитория
cd /d "%REPO_PATH%"

echo.
echo 1. Проверка статуса репозитория...
git status

echo.
echo 2. Добавление всех изменений...
git add .

echo.
echo 3. Коммит изменений...
git commit -m "%COMMIT_MESSAGE%"

echo.
echo 4. Получение последних изменений с сервера...
git pull origin %BRANCH%

echo.
echo 5. Отправка изменений на сервер...
git push origin %BRANCH%

echo.
echo ========================================
echo Обновление завершено успешно!
echo ========================================

rem Пауза для просмотра результатов
pause