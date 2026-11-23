#!/bin/sh
# Lanzador de AetherBoot: ejecuta VentoyVlnk.sh (se conserva ese nombre por compatibilidad).
cd "$(dirname "$0")" || exit 1
exec bash ./VentoyVlnk.sh "$@"
