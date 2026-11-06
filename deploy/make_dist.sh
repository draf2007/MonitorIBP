#!/usr/bin/env bash
# Сборка папки дистрибутива MonitorIBP для Windows.
# Запускать в консоли «MSYS2 MINGW64»:
#     ./deploy/make_dist.sh                  — возьмёт самый свежий release/MonitorIBP.exe из build/
#     ./deploy/make_dist.sh путь/к/MonitorIBP.exe
# Результат: dist/MonitorIBP рядом с проектом.
set -euo pipefail

PROJECT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
DIST_DIR="$PROJECT_DIR/dist/MonitorIBP"

# ---------- 1. Находим Release-exe ----------
EXE="${1:-}"
if [[ -z "$EXE" ]]; then
    EXE=$(find "$PROJECT_DIR/build" -path '*release/MonitorIBP.exe' -printf '%T@ %p\n' 2>/dev/null \
          | sort -n | tail -1 | cut -d' ' -f2-)
fi
if [[ -z "$EXE" || ! -f "$EXE" ]]; then
    echo "Не найден MonitorIBP.exe: соберите проект в Release или укажите путь к exe."
    exit 1
fi
echo "== Берём: $EXE"

# ---------- 2. Чистая папка, exe без отладочных символов ----------
rm -rf "$DIST_DIR"
mkdir -p "$DIST_DIR"
cp "$EXE" "$DIST_DIR/"
strip "$DIST_DIR/MonitorIBP.exe"

# ---------- 3. Библиотеки и плагины Qt ----------
echo "== windeployqt6"
windeployqt6 --release --no-translations --no-system-d3d-compiler --no-opengl-sw \
    --skip-plugin-types qmltooling,generic,networkinformation,tls,styles \
    --exclude-plugins qgif,qjpeg,qico \
    --qmldir "$PROJECT_DIR/resources/qml" \
    "$DIST_DIR/MonitorIBP.exe"

# ---------- 4. Лишние QML-модули (нам нужны только QtQuick, Layouts, Shapes, Effects) ----------
echo "== Убираем лишние QML-модули"
for module in QtQuick/Controls QtQuick/Dialogs QtQuick/Templates QtQuick/NativeStyle \
              QtQuick/Particles QtQuick/LocalStorage QtQuick/VectorImage QtQuick/tooling \
              QtQuick/Shapes/DesignHelpers QtQml/XmlListModel; do
    rm -rf "$DIST_DIR/qml/$module"
done

# ---------- 5. DLL самого MSYS2 (ICU, zstd, freetype, libstdc++ …) ----------
# Два прохода: у только что скопированных DLL бывают свои зависимости.
# pipefail здесь выключаем: grep без совпадений (всё уже скопировано) возвращает 1,
# и скрипт молча остановился бы на этом месте
echo "== Зависимости MSYS2"
cd "$DIST_DIR"
set +o pipefail
for pass in 1 2; do
    find . \( -name '*.exe' -o -name '*.dll' \) -print0 \
        | xargs -0 -n1 ldd 2>/dev/null \
        | grep -i '/mingw64/' | awk '{print $3}' | sort -u \
        | while read -r dll; do
              [[ -e "$(basename "$dll")" ]] || cp "$dll" .
          done
done
set -o pipefail

# ---------- 6. qt.conf: где Qt искать плагины и QML ----------
# Qt из MSYS2 по умолчанию ищет их в <префикс>/share/qt6/...; qt.conf указывает на нашу папку
cat > "$DIST_DIR/qt.conf" <<'CONF'
[Paths]
Prefix = .
Plugins = .
QmlImports = qml
CONF

# ---------- 7. Проверка и итог ----------
if ldd MonitorIBP.exe | grep -qi 'not found'; then
    echo "ВНИМАНИЕ: не найдены библиотеки:"
    ldd MonitorIBP.exe | grep -i 'not found'
    exit 1
fi
echo "== Готово: $DIST_DIR"
echo "   файлов: $(find . -type f | wc -l), размер: $(du -sh . | cut -f1)"

# ---------- 8. Установщик Inno Setup ----------
# Ищем ISCC.exe: в PATH или в стандартных папках Inno Setup 7
ISCC="$(command -v ISCC.exe 2>/dev/null || true)"
for candidate in "/c/Program Files/Inno Setup 7/ISCC.exe" \
                 "/c/Program Files (x86)/Inno Setup 7/ISCC.exe" \
                 "$(cygpath -u "${LOCALAPPDATA:-}" 2>/dev/null)/Programs/Inno Setup 7/ISCC.exe"; do
    [[ -z "$ISCC" && -f "$candidate" ]] && ISCC="$candidate"
done
if [[ -z "$ISCC" ]]; then
    echo "== Inno Setup 7 не найден — установщик не собран (папка dist готова)"
    exit 0
fi
VERSION=$(grep -E '^\s*VERSION\s*=' "$PROJECT_DIR/MonitorIBP.pro" | head -1 | sed 's/.*=\s*//' | tr -d '[:space:]')
echo "== Установщик (версия $VERSION)"
"$ISCC" -Q "-DAppVersion=$VERSION" "$(cygpath -w "$PROJECT_DIR/deploy/MonitorIBP.iss")"
echo "== Готово: $PROJECT_DIR/dist/MonitorIBP-$VERSION-setup.exe"
