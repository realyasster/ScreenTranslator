#!/usr/bin/env bash
# Local Windows cross-build (mingw-w64 + Qt5 Windows).
# Bu script saatler sürebilir (Qt Windows binary ~1GB indirir).
# Daha hızlı alternatif: GitHub Actions'dan .exe indir
# (Actions → Build → en son başarılı run → Artifacts → screen-translator-windows.zip).
set -e

cd "$(dirname "$0")/.."

BUILD_DIR="build-windows"
DEPS_DIR="$(pwd)/deps-windows"
QT_VERSION="5.15.2"
QT_INSTALLER="qt-windows-x86_64-${QT_VERSION}.exe"

echo "==> Cross-build environment"
echo "    Toolchain : mingw-w64 (x86_64-w64-mingw32-g++)"
echo "    Qt version: ${QT_VERSION}"
echo "    Deps dir  : ${DEPS_DIR}"
echo
echo "Önkoşullar:"
echo "  sudo apt install -y mingw-w64-x86-64-dev g++-mingw-w64-x86-64 \\"
echo "    nsis cmake git"
echo
echo "Qt5 Windows binary indir (~1GB):"
echo "  wget -q https://download.qt.io/archive/qt/5.15/${QT_VERSION}/${QT_INSTALLER}"
echo "  wget -q https://download.qt.io/archive/qt/5.15/${QT_VERSION}/qt-5.15.2-windows-x86_64.zip"
echo
echo "Bağımlılıkların Windows DLL'leri:"
echo "  Tesseract: https://github.com/UB-Mannheim/tesseract/releases (tesseract-ocr-w64-setup-5.x.exe)"
echo "  veya vcpkg / MSYS2 üzerinden: libtesseract, leptonica, hunspell DLL'leri"
echo
echo "Sonra:"
echo "  export QTDIR=/path/to/qt-5.15.2-windows-x86_64"
echo "  export TESSDATA_PREFIX=...\\assets\\tessdata"
echo "  ./share/ci/win-cross.sh build"

if [ "$1" != "build" ]; then
    exit 0
fi

if [ -z "${QTDIR:-}" ]; then
    echo "HATA: QTDIR tanımlanmamış"
    exit 1
fi

rm -rf "${BUILD_DIR}"
mkdir -p "${BUILD_DIR}" "${DEPS_DIR}/include" "${DEPS_DIR}/lib"

cat > "${BUILD_DIR}/qt.conf" <<EOF
[Paths]
Prefix = ${QTDIR}
EOF

${QTDIR}/bin/qmake.exe \
    -spec win32-g++ \
    "QMAKE_CXX=x86_64-w64-mingw32-g++" \
    "QMAKE_CC=x86_64-w64-mingw32-gcc" \
    "QMAKE_LINK=x86_64-w64-mingw32-g++" \
    "QMAKE_RC=x86_64-w64-mingw32-windres" \
    "INCLUDEPATH+=${DEPS_DIR}/include" \
    "LIBS+=-L${DEPS_DIR}/lib" \
    -o "${BUILD_DIR}/Makefile" \
    ../screen-translator.pro

cd "${BUILD_DIR}"
make -j$(nproc)

echo
echo "==> Tamamlandı: ${BUILD_DIR}/screen-translator.exe"
echo "Dağıtım için windeploy kullan:"
echo "  ${QTDIR}/bin/windeployqt --qmldir ../share/qml screen-translator.exe"