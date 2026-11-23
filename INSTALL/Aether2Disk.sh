#!/bin/sh
# Lanzador de AetherBoot: ejecuta Ventoy2Disk.sh (se conserva ese nombre por compatibilidad).
cd "$(dirname "$0")" || exit 1
exec sh ./Ventoy2Disk.sh "$@"
