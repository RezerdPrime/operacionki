@echo off
setlocal enabledelayedexpansion

echo ==========================================
echo GitHub Update Script for C++/CMake Project
echo ==========================================

set "REPO_PATH=C:\Users\Lenovo\Desktop\CodeShit\operacionki"
set "BRANCH=main"
set "COMMIT_MESSAGE=Update project files"

cd /d "%REPO_PATH%"

echo.
echo 1. Status checking...
git status

echo.
echo 2. Acceptance of changes...
git add .

echo.
echo 3. Commiting...
git commit -m "%COMMIT_MESSAGE%"

echo.
echo 4. Pull origin...
git pull origin %BRANCH%

echo.
echo 5. Push origin...
git push origin %BRANCH%

echo.
echo ========================================
echo Fin.
echo ========================================

pause