#!/bin/bash

# ============================================
# Script: updateGitHub.sh
# Pull -> pauza pentru modificari -> commit selectiv -> push
# Cu salvare credentiale in fisier local
# ============================================

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CONFIG_FILE="${SCRIPT_DIR}/.github_credentials.conf"

echo -e "${YELLOW}=== Actualizare depozit GitHub ===${NC}"

# ============================================================
# INCARCARE CONFIGURATIE (daca exista)
# ============================================================
USE_SAVED="n"
if [ -f "$CONFIG_FILE" ]; then
    echo -e "${CYAN}Am gasit credentiale salvate in:${NC} $CONFIG_FILE"
    echo "  - REPO_SLUG  = $REPO_SLUG"
    echo "  - GIT_USER   = $GIT_USER"
    echo "  - GIT_EMAIL  = $GIT_EMAIL"
    echo "  - AUTH_USER  = $AUTH_USER"
    echo "  - AUTH_PASS  = ********"
    read -p "Folosesti aceste credentiale salvate? (Y/n): " USE_SAVED
    if [[ "$USE_SAVED" =~ ^[Nn]$ ]]; then
        USE_SAVED="n"
    else
        USE_SAVED="y"
        # shellcheck disable=SC1090
        source "$CONFIG_FILE"
    fi
fi

# ============================================================
# DACA NU FOLOSIM CELE SALVATE -> CERE DE LA TASTATURA
# ============================================================
if [ "$USE_SAVED" != "y" ]; then

    # --- Calea repo-ului local ---
    read -p "Introdu calea catre repository-ul local [default: ${SCRIPT_DIR}]: " REPO_PATH
    if [ -z "$REPO_PATH" ]; then
        REPO_PATH="$SCRIPT_DIR"
    fi
    REPO_PATH="${REPO_PATH/#\~/$HOME}"

    if [ ! -d "$REPO_PATH" ]; then
        echo -e "${RED}Eroare: Directorul '$REPO_PATH' nu exista!${NC}"
        exit 1
    fi

    # --- owner/repo ---
    read -p "Introdu owner/repo GitHub (ex: marinpopa/AsyncOTAManager): " REPO_SLUG
    if [ -z "$REPO_SLUG" ]; then
        echo -e "${RED}Eroare: owner/repo nu poate fi gol!${NC}"
        exit 1
    fi

    # --- user + email ---
    read -p "Introdu username GitHub: " GIT_USER
    read -p "Introdu email GitHub: " GIT_EMAIL

    # --- credentiale ---
    read -p "Introdu username-ul pentru autentificare GitHub: " AUTH_USER
    read -s -p "Introdu parola / token-ul GitHub: " AUTH_PASS
    echo

    # --- Salvare in fisier? ---
    read -p "Salvezi aceste date pentru viitor? (Y/n): " SAVE_CRED
    if [[ ! "$SAVE_CRED" =~ ^[Nn]$ ]]; then
        cat > "$CONFIG_FILE" <<EOF
# Fisier de configurare generat automat de updateGitHub.sh
# ATENTIE: contine credentiale! Nu-l urca pe GitHub!
REPO_PATH="$REPO_PATH"
REPO_SLUG="$REPO_SLUG"
GIT_USER="$GIT_USER"
GIT_EMAIL="$GIT_EMAIL"
AUTH_USER="$AUTH_USER"
AUTH_PASS="$AUTH_PASS"
EOF
        chmod 600 "$CONFIG_FILE"
        echo -e "${GREEN}Credentiale salvate in: $CONFIG_FILE (chmod 600)${NC}"
    fi
fi

# Daca REPO_PATH lipseste (cand am folosit credentiale salvate fara el)
if [ -z "$REPO_PATH" ]; then
    REPO_PATH="$SCRIPT_DIR"
fi
if [ ! -d "$REPO_PATH" ]; then
    echo -e "${RED}Eroare: Directorul '$REPO_PATH' nu exista!${NC}"
    exit 1
fi

cd "$REPO_PATH"
echo -e "${GREEN}Director curent: $(pwd)${NC}"

REMOTE_URL="https://github.com/${REPO_SLUG}.git"
CRED_URL="https://${AUTH_USER}:${AUTH_PASS}@github.com/${REPO_SLUG}.git"

# --- Init + remote ---
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

# --- Fetch + pull ---
echo -e "${YELLOW}[6/9] Fetch + Pull origin main (merge)${NC}"

git -c credential.helper= \
    -c "url.https://${AUTH_USER}:${AUTH_PASS}@github.com/.insteadOf=https://github.com/" \
    fetch origin main

if git rev-parse --verify origin/main >/dev/null 2>&1; then
    git -c credential.helper= \
        -c "url.https://${AUTH_USER}:${AUTH_PASS}@github.com/.insteadOf=https://github.com/" \
        pull origin main --no-rebase --allow-unrelated-histories || {
            echo -e "${YELLOW}Pull a esuat (posibil conflicte). Continuam...${NC}"
        }
else
    echo -e "${YELLOW}Nu exista branch remote 'main'. Sar peste pull.${NC}"
fi

# ============================================================
# PAUZA: asteapta sa faci modificarile manual
# ============================================================
echo ""
echo -e "${CYAN}==========================================================${NC}"
echo -e "${CYAN}  Depozitul a fost actualizat (pull).${NC}"
echo -e "${CYAN}  Acum fa modificarile dorite in fisiere.${NC}"
echo -e "${CYAN}  Cand ai terminat, apasa ENTER.${NC}"
echo -e "${CYAN}==========================================================${NC}"
read -p "Apasa ENTER dupa ce ai terminat modificarile... " _dummy

# --- Detectie fisiere modificate ---
echo ""
echo -e "${YELLOW}[7/9] Fisiere modificate detectate:${NC}"

CHANGED_FILES=$(git status --porcelain | awk '{print $2}')

if [ -z "$CHANGED_FILES" ]; then
    echo -e "${YELLOW}Nu s-a detectat nicio modificare. Nimic de commit.${NC}"
    git status
    echo -e "${GREEN}=== GATA (fara modificari). ===${NC}"
    exit 0
fi

i=1
declare -a FILE_ARRAY
while IFS= read -r f; do
    FILE_ARRAY[$i]="$f"
    echo "  [$i] $f"
    i=$((i+1))
done <<< "$CHANGED_FILES"

TOTAL=$((i-1))

echo ""
echo -e "${CYAN}Ce fisiere vrei sa commit-ezi?${NC}"
echo "  - Introdu numerele separate prin spatiu (ex: 1 3 4)"
echo "  - Sau 'all' pentru toate"
echo "  - Sau ENTER gol pentru a anula"
read -p "Selectie: " SELECTION

if [ -z "$SELECTION" ]; then
    echo -e "${YELLOW}Anulat. Nu s-a facut niciun commit.${NC}"
    exit 0
fi

FILES_TO_ADD=()
if [ "$SELECTION" = "all" ] || [ "$SELECTION" = "ALL" ]; then
    for ((k=1; k<=TOTAL; k++)); do
        FILES_TO_ADD+=("${FILE_ARRAY[$k]}")
    done
else
    for num in $SELECTION; do
        if [[ "$num" =~ ^[0-9]+$ ]] && [ "$num" -ge 1 ] && [ "$num" -le "$TOTAL" ]; then
            FILES_TO_ADD+=("${FILE_ARRAY[$num]}")
        else
            echo -e "${RED}Numar invalid ignorat: $num${NC}"
        fi
    done
fi

if [ ${#FILES_TO_ADD[@]} -eq 0 ]; then
    echo -e "${RED}Nicio selectie valida. Anulat.${NC}"
    exit 0
fi

echo -e "${GREEN}Fisiere selectate:${NC}"
for f in "${FILES_TO_ADD[@]}"; do
    echo "  - $f"
done

# --- Mesaj commit ---
echo ""
read -p "Mesaj commit [default: \"Update files\"]: " COMMIT_MSG
if [ -z "$COMMIT_MSG" ]; then
    COMMIT_MSG="Update files"
fi

# --- add selectiv + commit + push ---
echo -e "${YELLOW}[8/9] git add (selectiv) + commit${NC}"
git add -- "${FILES_TO_ADD[@]}"
git status
git commit -m "$COMMIT_MSG"

echo -e "${YELLOW}[9/9] Push origin main${NC}"
git push "$CRED_URL" main

# --- Tag optional ---
echo ""
read -p "Vrei sa creezi tag v1.0.5? (y/N): " MAKE_TAG
if [[ "$MAKE_TAG" =~ ^[Yy]$ ]]; then
    git tag v1.0.5 2>/dev/null || echo -e "${YELLOW}Tag v1.0.5 exista deja.${NC}"
    git push "$CRED_URL" v1.0.5
fi

# --- cleanup remote ---
echo -e "${YELLOW}Resetare remote la URL fara credentiale...${NC}"
git remote set-url origin "$REMOTE_URL"

echo -e "${GREEN}=== GATA! Modificari publicate cu succes. ===${NC}"
