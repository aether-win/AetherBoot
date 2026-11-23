#!/bin/sh
# Lanzador de AetherBoot: ejecuta VentoyPlugson.sh (se conserva ese nombre por compatibilidad).
cd "$(dirname "$0")" || exit 1
exec bash ./VentoyPlugson.sh "$@"
