#!/bin/bash

echo "=========================================="
echo "Github Update Script for C++/Cmake Project"
echo "=========================================="

REPO_PATH="/Desktop/operacionki"
BRANCH="main"
COMMIT_MESSAGE="Update project files"

echo "$(git config --global user.name)"

cd "$REPO_PATH"

echo ""
echo "1. Status checking..."
git status

echo ""
echo "2. Acceptance of changes..."
git add .

echo ""
echo "3. Commiting..."
git commit -m "$COMMIT_MESSAGE"

echo ""
echo "4. Pull origin..."
git pull origin "$BRANCH"

echo ""
echo "5. Push origin..."
git push origin "$BRANCH"

echo ""
echo "============================="
echo "Fin."
echo "============================="
