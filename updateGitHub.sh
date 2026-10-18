 #!/bin/bash

# Oprește execuția dacă apar erori
set -e

echo "========================================="
echo "   🚀 GitHub Repository Update Manager   "
echo "========================================="

# 1. Solicitare detalii repository
read -p "Introdu calea repoului (ex: marinpopa/AsyncOTAManager): " REPO_PATH
if [ -z "$REPO_PATH" ]; then
    echo "❌ Eroare: Calea repository-ului nu poate fi goală!"
    exit 1
fi

# 2. Solicitare versiune nouă
read -p "Introdu noua versiune (ex: 1.0.5): " VERSION
if [ -z "$VERSION" ]; then
    echo "❌ Eroare: Versiunea nu poate fi goală!"
    exit 1
fi

# 3. Solicitare Personal Access Token (pentru securitate)
read -sp "Introdu GitHub Personal Access Token (PAT): " TOKEN
echo ""
if [ -z "$TOKEN" ]; then
    echo "❌ Eroare: Token-ul nu poate fi gol!"
    exit 1
fi

# Extragere utilizator din path (tot ce e înainte de primul /)
USER_NAME=$(echo "$REPO_PATH" | cut -d'/' -f1)

echo -e "\n🔄 Se configurează conexiunea Git..."
# Inițializare repo dacă nu există deja
if [ ! -d ".git" ]; then
    git init
    git branch -m main
else
    # Curățare remote existent pentru a evita conflictele
    git remote remove origin 2>/dev/null || true
fi

# Adăugare remote securizat cu Token inclus direct
git remote add origin "https://${USER_NAME}:${TOKEN}@://github.com{REPO_PATH}.git"

# Sincronizare inițială cu serverul (păstrează modificările locale prioritare)
echo "📥 Se sincronizează cu istoricul de pe GitHub..."
git pull origin main --allow-unrelated-histories || true

# 4. Actualizare automată în library.properties
FILE="library.properties"
if [ -f "$FILE" ]; then
    echo "📝 Se actualizează versiunea în $FILE la $VERSION..."
    # Înlocuiește linia care începe cu version= cu noua valoare
    sed -i "s/^version=.*/version=${VERSION}/" "$FILE"
else
    echo "⚠️ Avertisment: Nu s-a găsit fișierul $FILE. Se trece peste modificarea internă."
fi

# 5. Procesul de trimitere pe GitHub (Push)
echo "📤 Se pregătesc fișierele pentru trimitere..."
git add .

# Verificăm dacă sunt modificări de trimis
if git diff-index --quiet HEAD --; then
    echo "ℹ️ Nu sunt modificări noi în fișiere."
else
    git commit -m "Update source files and bump version to ${VERSION}"
    echo "🚀 Se încarcă codul pe GitHub..."
    git push origin main
fi

# 6. Creare și trimitere Tag (Versiune)
echo "🏷️ Se creează tag-ul v${VERSION}..."
# Șterge tag-ul local/remote dacă exista deja pentru a evita erori la re-run
git tag -d "v${VERSION}" 2>/dev/null || true
git push origin :refs/tags/v${VERSION} 2>/dev/null || true

# Generare tag nou
git tag "v${VERSION}"
git push origin "v${VERSION}"

echo "========================================="
echo "🎉 Toate operațiunile s-au terminat cu succes!"
echo "✨ Versiunea v${VERSION} este acum live pe GitHub!"
echo "========================================="

