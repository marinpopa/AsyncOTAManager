#!/bin/bash

# ============================================
# Script: update_github_repo.sh
# Actualizeaza un depozit GitHub
# ============================================

set -e  # opreste scriptul la prima eroare

# --- Culori pentru output ---
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${YELLOW}=== Actualizare depozit GitHub ===${NC}"

# --- 1. Cere calea catre repository (repo-ul local) ---
# Daca nu se introduce nimic, foloseste directorul in care se afla scriptul
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

read -p "Introdu calea catre repository-ul local [default: ${SCRIPT_DIR}]: " REPO_PATH

if [ -z "$REPO_PATH" ]; then
    REPO_PATH="$SCRIPT_DIR"
fi

# Extinde ~ in $HOME
REPO_PATH="${REPO_PATH/#\~/$HOME}"

if [ ! -d "$REPO_PATH" ]; then
    echo -e "${RED}Eroare: Directorul '$REPO_PATH' nu exista!${NC}"
    exit 1
fi

cd "$REPO_PATH"
echo -e "${GREEN}Director curent: $(pwd)${NC}"

# --- 2. Cere URL-ul remote (owner/repo) ---
read -p "Introdu owner/repo GitHub (ex: marinpopa/AsyncOTAManager): " REPO_SLUG
if [ -z "$REPO_SLUG" ]; then
    echo -e "${RED}Eroare: owner/repo nu poate fi gol!${NC}"
    exit 1
fi
REMOTE_URL="https://github.com/${REPO_SLUG}.git"

# --- 3. Cere username si email pentru git config ---
read -p "Introdu username GitHub: " GIT_USER
read -p "Introdu email GitHub: " GIT_EMAIL

# --- 4. Cere credentiale (username + parola/token) ---
read -p "Introdu username-ul pentru autentificare GitHub: " AUTH_USER
read -s -p "Introdu parola / token-ul GitHub: " AUTH_PASS
echo

# --- 5. Construieste URL cu credentiale pentru push/pull ---
CRED_URL="https://${AUTH_USER}:${AUTH_PASS}@github.com/${REPO_SLUG}.git"

# --- 6. Init si configurare remote ---
echo -e "${YELLOW}[1/9] git init${NC}"
git init

echo -e "${YELLOW}[2/9] Eliminare remote origin existent${NC}"
git remote remove origin 2>/dev/null || true

echo -e "${YELLOW}[3/9] Adaugare remote origin: ${REMOTE_URL}${NC}"
git remote add origin "$REMOTE_URL"

echo -e "${YELLOW}[4/9] Redenumire branch in 'main'${NC}"
git branch -m main

echo -e "${YELLOW}[5/9] Configurare user git${NC}"
git config --global user.email "$GIT_EMAIL"
git config --global user.name "$GIT_USER"

# --- 7. Pull cu credentiale ---
echo -e "${YELLOW}[6/9] Pull origin main${NC}"
git -c credential.helper= \
    -c "url.https://${AUTH_USER}:${AUTH_PASS}@github.com/.insteadOf=https://github.com/" \
    pull origin main --allow-unrelated-histories || {
        echo -e "${YELLOW}Pull a esuat sau nu exista branch remote. Continuam...${NC}"
    }

# --- 8. Add / status / commit / push ---
echo -e "${YELLOW}[7/9] git add . && status && commit${NC}"
git add .
git status
git commit -m "Update ota_html_gz.h with new web interface" || {
    echo -e "${YELLOW}Nimic de commit (poate fi deja actualizat).${NC}"
}

echo -e "${YELLOW}[8/9] Push origin main${NC}"
git push "$CRED_URL" main

# --- 9. Commit + tag pentru library.properties ---
echo -e "${YELLOW}[9/9] Actualizare library.properties + tag v1.0.5${NC}"
git add library.properties
git commit -m "Bump version to 1.0.5" || {
    echo -e "${YELLOW}Nimic de commit pentru library.properties.${NC}"
}
git push "$CRED_URL" main

git tag v1.0.5 2>/dev/null || echo -e "${YELLOW}Tag v1.0.5 exista deja.${NC}"
git push "$CRED_URL" v1.0.5

# --- Cleanup credentiale din remote ---
echo -e "${YELLOW}Resetare remote la URL fara credentiale...${NC}"
git remote set-url origin "$REMOTE_URL"

echo -e "${GREEN}=== GATA! Depozit actualizat cu succes. ===${NC}"
