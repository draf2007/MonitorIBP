#!/usr/bin/env bash
# ============================================================================
#  Полный релиз MonitorIBP: установщик Windows + .deb для Linux
#  Запускать в консоли «MSYS2 MINGW64» (или двойным кликом по make_release.bat)
#  Результат в dist\:
#     MonitorIBP-<версия>-setup.exe    — Windows (make_dist.sh + Inno Setup)
#     monitoribp_<версия>_amd64.deb    — Linux (make_deb.sh в WSL Debian 12)
#     SHA256SUMS-<версия>.txt          — контрольные суммы обоих файлов
# ============================================================================
set -euo pipefail

PROJECT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
PRO="$PROJECT_DIR/MonitorIBP.pro"
BUILD_DIR="$HOME/build-MonitorIBP-release"     # вне проекта: его можно смело стирать
WSL_DISTRO="${WSL_DISTRO:-Debian12-build}"      # WSL-машина для сборки .deb

fail() { echo; echo "ОШИБКА: $*"; exit 1; }

# ---------- 1. Версия ----------
CURRENT=$(sed -n 's/^VERSION *= *//p' "$PRO" | tr -d '\r ')
[[ -n "$CURRENT" ]] || fail "в MonitorIBP.pro не найдена строка VERSION"
echo "Текущая версия: $CURRENT"
read -rp "Новая версия (Enter — оставить $CURRENT): " NEW
NEW="${NEW:-$CURRENT}"
[[ "$NEW" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]] || fail "версия должна быть вида 1.2.3, а введено «$NEW»"
if [[ "$NEW" != "$CURRENT" ]]; then
    # меняем только цифры, конец строки (\r\n или \n) остаётся как был
    sed -i "s/^\(VERSION *= *\)[0-9.]*/\1$NEW/" "$PRO"
    echo "== VERSION в MonitorIBP.pro: $CURRENT → $NEW"
fi
VERSION="$NEW"

# ---------- 2. Windows: чистая Release-сборка ----------
echo "== Windows: сборка Release в $BUILD_DIR"
QMAKE=$(command -v qmake6 || command -v qmake-qt6 || command -v qmake || true)
MAKE=$(command -v mingw32-make || command -v make || true)
[[ -n "$QMAKE" && -n "$MAKE" ]] || fail "не найден qmake6 или mingw32-make — запускайте в консоли MSYS2 MINGW64"
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"
"$QMAKE" "$PRO" CONFIG+=release CONFIG-=debug CONFIG-=debug_and_release CONFIG-=qml_debug
"$MAKE" -j"$(nproc)"
EXE=$(find "$BUILD_DIR" -name MonitorIBP.exe -print -quit)
[[ -n "$EXE" ]] || fail "сборка прошла, но MonitorIBP.exe не найден"

# ---------- 3. Windows: папка дистрибутива и установщик ----------
bash "$PROJECT_DIR/deploy/make_dist.sh" "$EXE"
SETUP="$PROJECT_DIR/dist/MonitorIBP-$VERSION-setup.exe"
[[ -f "$SETUP" ]] || fail "не собран $SETUP (установлен ли Inno Setup 7?)"

# ---------- 4. Linux: make_deb.sh внутри WSL ----------
echo "== Linux: сборка .deb в WSL ($WSL_DISTRO)"
command -v wsl.exe >/dev/null || fail "WSL не установлен"
WIN_PROJECT=$(cygpath -w "$PROJECT_DIR")
# MSYS2_ARG_CONV_EXCL: не давать MSYS2 переделывать /tmp/... в C:/msys64/tmp/...
# tr -d '\r': если скрипт сохранён в Windows с CRLF, bash в Linux его не поймёт
MSYS2_ARG_CONV_EXCL='*' wsl.exe -d "$WSL_DISTRO" --cd "~" -e bash -lc '
    set -e
    P=$(wslpath -u "$1")
    tr -d "\r" < "$P/deploy/make_deb.sh" > /tmp/make_deb.sh
    PROJECT_DIR="$P" bash /tmp/make_deb.sh
' _ "$WIN_PROJECT" || fail "сборка .deb в WSL не удалась (вывод выше)"
DEB="$PROJECT_DIR/dist/monitoribp_${VERSION}_amd64.deb"
[[ -f "$DEB" ]] || fail "не собран $DEB"

# ---------- 5. Контрольные суммы ----------
cd "$PROJECT_DIR/dist"
sha256sum "$(basename "$SETUP")" "$(basename "$DEB")" > "SHA256SUMS-$VERSION.txt"

echo
echo "================ Релиз $VERSION готов ================"
ls -lh "$(basename "$SETUP")" "$(basename "$DEB")" "SHA256SUMS-$VERSION.txt"
echo "Папка: $(cygpath -w "$PROJECT_DIR/dist")"
