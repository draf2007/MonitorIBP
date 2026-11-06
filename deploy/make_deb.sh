#!/bin/bash
# ============================================================================
#  Сборка .deb «с Qt внутри» для Debian 12/13 и Astra Linux 1.8
#  Запускать на Debian 12 (сборочная машина), из любой папки:
#      ~/src/MonitorIBP/deploy/make_deb.sh
#  Результат: ~/src/MonitorIBP/dist/monitoribp_<версия>_amd64.deb
# ============================================================================
set -euo pipefail

# ---------- Настройки ----------
QT_DIR="${QT_DIR:-$HOME/Qt/6.8.3/gcc_64}"          # Qt, с которым собираем
PROJECT_DIR="${PROJECT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"   # корень проекта (папка с .pro)
BUILD_DIR="$HOME/build-MonitorIBP-deb"            # отдельная Release-сборка
INSTALL_DIR="/opt/MonitorIBP"                     # куда ставится программа
APP="MonitorIBP"
PKG="monitoribp"                                  # имя пакета: только строчные

VERSION=$(sed -n 's/^VERSION *= *//p' "$PROJECT_DIR/$APP.pro" | tr -d '\r ')
STAGE="$BUILD_DIR/pkg"                            # «образ» файловой системы пакета
APPDIR="$STAGE$INSTALL_DIR"

echo "== $APP $VERSION, Qt: $QT_DIR"
[ -x "$QT_DIR/bin/qmake" ] || { echo "Нет $QT_DIR/bin/qmake — проверьте QT_DIR"; exit 1; }
for tool in patchelf dpkg-deb python3; do
    command -v $tool >/dev/null || { echo "Нет $tool: sudo apt install patchelf dpkg-dev python3"; exit 1; }
done

# ---------- 1. Release-сборка ----------
echo "== 1. Сборка"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"
"$QT_DIR/bin/qmake" "$PROJECT_DIR/$APP.pro" CONFIG+=release CONFIG-=debug CONFIG-=qml_debug
make -j"$(nproc)"

# ---------- 2. Каркас пакета ----------
echo "== 2. Каркас"
rm -rf "$STAGE"
mkdir -p "$APPDIR/lib" "$APPDIR/plugins" "$APPDIR/qml" \
         "$STAGE/DEBIAN" "$STAGE/usr/bin" "$STAGE/usr/share/applications" \
         "$STAGE/usr/share/icons/hicolor/scalable/apps"
install -m 755 "$BUILD_DIR/$APP" "$APPDIR/$APP"

# ---------- 3. Плагины Qt ----------
echo "== 3. Плагины"
copy_plugins() {   # $1 — папка плагинов, остальные — имена файлов (пусто = все)
    local dir="$1"; shift
    [ -d "$QT_DIR/plugins/$dir" ] || return 0
    mkdir -p "$APPDIR/plugins/$dir"
    if [ $# -eq 0 ]; then
        cp -a "$QT_DIR/plugins/$dir/"*.so "$APPDIR/plugins/$dir/"
    else
        for f in "$@"; do
            if [ -f "$QT_DIR/plugins/$dir/$f" ]; then cp -a "$QT_DIR/plugins/$dir/$f" "$APPDIR/plugins/$dir/"; fi
        done
    fi
}
copy_plugins platforms libqxcb.so libqwayland-egl.so libqwayland-generic.so libqoffscreen.so
copy_plugins xcbglintegrations
copy_plugins wayland-shell-integration
copy_plugins wayland-decoration-client
copy_plugins wayland-graphics-integration-client
copy_plugins platforminputcontexts
copy_plugins platformthemes libqxdgdesktopportal.so
copy_plugins imageformats libqsvg.so libqico.so libqjpeg.so libqgif.so
copy_plugins iconengines

# ---------- 4. Модули QML (какие нужны — определяет qmlimportscanner) ----------
echo "== 4. Модули QML"
SCANNER="$QT_DIR/libexec/qmlimportscanner"
[ -x "$SCANNER" ] || SCANNER="$QT_DIR/bin/qmlimportscanner"
"$SCANNER" -rootPath "$PROJECT_DIR/resources/qml" -importPath "$QT_DIR/qml" > "$BUILD_DIR/qmlimports.json"
python3 - "$BUILD_DIR/qmlimports.json" "$QT_DIR/qml" "$APPDIR/qml" <<'EOF'
import json, os, shutil, sys
imports, src_root, dst_root = json.load(open(sys.argv[1])), sys.argv[2], sys.argv[3]
for item in imports:
    path = item.get("path")
    if item.get("type") != "module" or not path or not path.startswith(src_root):
        continue
    rel = os.path.relpath(path, src_root)
    dst = os.path.join(dst_root, rel)
    if os.path.isdir(dst):
        continue
    # копируем только сам модуль, без вложенных модулей (они придут отдельной строкой)
    os.makedirs(dst, exist_ok=True)
    for name in os.listdir(path):
        s = os.path.join(path, name)
        if os.path.isfile(s) and not name.endswith(".debug"):
            shutil.copy2(s, dst)
    print("   ", rel)
EOF

# ---------- 5. Библиотеки: всё из Qt + то, чего может не быть в системе ----------
echo "== 5. Библиотеки"
# Не из Qt, но везём с собой: net-snmp, OpenSSL и xcb-утилиты (их часто нет «из коробки»)
EXTRA_LIBS="libnetsnmp.so.40 libssl.so.3 libcrypto.so.3 libxcb-cursor.so.0 libxcb-icccm.so.4 \
libxcb-image.so.0 libxcb-keysyms.so.1 libxcb-render-util.so.0 libxcb-util.so.1 libxkbcommon-x11.so.0"

collect_libs() {   # пройти по всем ELF в пакете и докопировать их зависимости
    local changed=1
    while [ $changed -eq 1 ]; do
        changed=0
        while IFS= read -r elf; do
            while IFS= read -r line; do
                name=$(echo "$line" | awk '{print $1}')
                path=$(echo "$line" | awk '{print $3}')
                [ -f "$path" ] || continue
                [ -e "$APPDIR/lib/$name" ] && continue
                case "$path" in
                    "$QT_DIR"/lib/*) ;;                                  # библиотека Qt — берём
                    *) echo " $EXTRA_LIBS " | grep -q " $name " || continue ;;
                esac
                cp -L "$path" "$APPDIR/lib/$name"
                echo "    $name"
                changed=1
            done < <(LD_LIBRARY_PATH="$QT_DIR/lib" ldd "$elf" 2>/dev/null | grep '=>')
        done < <(find "$APPDIR" -type f \( -name "$APP" -o -name '*.so*' \))
    done
}
collect_libs

# ---------- 6. RPATH: где искать библиотеки ----------
# Путь абсолютный, не $ORIGIN: из-за setcap (порт 162) программа запускается в
# «защищённом» режиме, а в нём glibc игнорирует $ORIGIN
echo "== 6. RPATH"
find "$APPDIR" -type f \( -name "$APP" -o -name '*.so*' \) | while read -r elf; do
    patchelf --set-rpath "$INSTALL_DIR/lib" "$elf" 2>/dev/null || true
done
find "$APPDIR" -type f \( -name "$APP" -o -name '*.so*' \) -exec strip --strip-unneeded {} + 2>/dev/null || true

# ---------- 7. qt.conf, ярлык, значок ----------
echo "== 7. qt.conf, ярлык, значок"
cat > "$APPDIR/qt.conf" <<EOF
[Paths]
Prefix=$INSTALL_DIR
Libraries=lib
Plugins=plugins
QmlImports=qml
EOF

ln -s "$INSTALL_DIR/$APP" "$STAGE/usr/bin/$APP"
install -m 644 "$PROJECT_DIR/resources/icons/app.svg" \
        "$STAGE/usr/share/icons/hicolor/scalable/apps/$APP.svg"

cat > "$STAGE/usr/share/applications/$APP.desktop" <<EOF
[Desktop Entry]
Type=Application
Name=Мониторинг ИБП
Name[en]=UPS Monitor
Comment=Мониторинг ИБП по COM-порту и SNMP
Exec=$INSTALL_DIR/$APP
Icon=$APP
Terminal=false
Categories=System;Monitor;Utility;
StartupWMClass=$APP
EOF

# ---------- 8. Служебные файлы пакета ----------
echo "== 8. DEBIAN/"
SIZE_KB=$(du -sk "$STAGE" --exclude=DEBIAN | cut -f1)
cat > "$STAGE/DEBIAN/control" <<EOF
Package: $PKG
Version: $VERSION
Architecture: amd64
Maintainer: Ruselt
Installed-Size: $SIZE_KB
Depends: libc6 (>= 2.36), libgl1, libegl1, libfontconfig1, libfreetype6, libx11-6, libx11-xcb1, libxcb1, libxcb-shape0, libxkbcommon0, libdbus-1-3, libcap2-bin
Section: utils
Priority: optional
Description: Мониторинг ИБП
 Наблюдение за ИБП по COM-порту (Megatec) и SNMP (UPS-MIB),
 приём SNMP-трапов, журналы событий и измерений.
 Qt 6 входит в пакет (/opt/MonitorIBP).
EOF

cat > "$STAGE/DEBIAN/postinst" <<EOF
#!/bin/sh
set -e
# Разрешить слушать UDP-порт 162 (SNMP-трапы) без прав root
setcap cap_net_bind_service=+ep $INSTALL_DIR/$APP || true
command -v update-desktop-database >/dev/null && update-desktop-database -q /usr/share/applications || true
command -v gtk-update-icon-cache >/dev/null && gtk-update-icon-cache -q /usr/share/icons/hicolor || true
exit 0
EOF
chmod 755 "$STAGE/DEBIAN/postinst"

# ---------- 9. Сборка пакета ----------
echo "== 9. dpkg-deb"
mkdir -p "$PROJECT_DIR/dist"
OUT="$PROJECT_DIR/dist/${PKG}_${VERSION}_amd64.deb"
dpkg-deb --build --root-owner-group "$STAGE" "$OUT"
ls -lh "$OUT"
echo "Готово. Установка: sudo apt install $OUT"
