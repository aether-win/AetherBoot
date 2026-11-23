#!/bin/bash
# ============================================================================
#  restaurar.sh — devuelve el código fuente de AetherBoot al estado original
#  de Ventoy 1.1.18.
#
#  Qué hace:
#    1. Aplica al revés el parche aetherboot-vs-original.patch (git apply -R).
#    2. Elimina la carpeta AetherBoot/ y README.md nuevo (añadidos por AetherBoot).
#    3. Si se indica el zip original, compara el resultado archivo por archivo.
#
#  Uso:
#    bash AetherBoot/Backup/restaurar.sh [ruta/al/Ventoy-1_1_18.zip original]
#
#  Se ejecuta desde la raíz del código fuente. Pide confirmación antes de cambiar nada.
# ============================================================================
set -euo pipefail

RAIZ="$(cd "$(dirname "$0")/../.." && pwd)"
PARCHE="$RAIZ/AetherBoot/Backup/aetherboot-vs-original.patch"
ORIGINAL_ZIP="${1:-}"

command -v git >/dev/null || { echo "Se necesita git (solo para aplicar el parche)."; exit 1; }
[ -f "$PARCHE" ] || { echo "No se encuentra $PARCHE"; exit 1; }

echo "Se va a restaurar el código original de Ventoy 1.1.18 en:"
echo "  $RAIZ"
echo "Se perderán todos los cambios de AetherBoot en esa carpeta."
read -r -p "¿Continuar? (escriba SI): " r
[ "$r" = "SI" ] || { echo "Cancelado."; exit 0; }

cd "$RAIZ"
echo "==> Comprobando que el parche se puede revertir..."
git apply -R --check --binary --whitespace=nowarn "$PARCHE"

TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
cp "$PARCHE" "$TMP/parche"
cp "$0" "$TMP/restaurar.sh"

echo "==> Revirtiendo cambios..."
git apply -R --binary --whitespace=nowarn "$TMP/parche"
rm -rf "$RAIZ/AetherBoot"

if [ -n "$ORIGINAL_ZIP" ]; then
    echo "==> Comparando con $ORIGINAL_ZIP ..."
    mkdir -p "$TMP/orig"
    unzip -q "$ORIGINAL_ZIP" -d "$TMP/orig"
    ORIG_DIR="$(find "$TMP/orig" -mindepth 1 -maxdepth 1 -type d | head -n1)"
    if diff -rq "$ORIG_DIR" "$RAIZ" -x .git >/dev/null; then
        echo "    Idéntico al original."
    else
        echo "    Hay diferencias:"; diff -rq "$ORIG_DIR" "$RAIZ" -x .git | head -20
        exit 1
    fi
fi
echo "Listo. El código está como en Ventoy 1.1.18."
