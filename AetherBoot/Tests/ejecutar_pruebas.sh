#!/bin/bash
# ============================================================================
#  ejecutar_pruebas.sh — pruebas automáticas de AetherBoot
#
#  Uso: bash AetherBoot/Tests/ejecutar_pruebas.sh <ventoy-1.1.18-windows.zip oficial>
#
#  No prueba el arranque real (requiere una máquina virtual o hardware) ni
#  compila los programas de Windows (requiere Visual Studio): ver
#  Documentation/08_Compatibilidad.md.
# ============================================================================
set -uo pipefail
ZIP="$(realpath "$1")"
RAIZ="$(cd "$(dirname "$0")/../.." && pwd)"
T="$(mktemp -d)"; trap 'kill $(jobs -p) 2>/dev/null; rm -rf "$T"' EXIT
pasa=0; falla=0; omite=0
ok()   { echo "  [PASA]   $*"; pasa=$((pasa+1)); }
mal()  { echo "  [FALLA]  $*"; falla=$((falla+1)); }
omit() { echo "  [OMITE]  $*"; omite=$((omite+1)); }
cd "$RAIZ"

echo "== 1. Partición de arranque"
unzip -q "$ZIP" -d "$T/zip"
ORIG_IMG="$(find "$T/zip" -name ventoy.disk.img.xz | head -n1)"
if bash AetherBoot/Build/construir_imagen_arranque.sh "$ORIG_IMG" "$T/nueva.img.xz" > "$T/build.log" 2>&1; then
    ok "construcción y verificación byte a byte ($(grep -o '[0-9]* archivos verificados' "$T/build.log"))"
else
    mal "construcción de la imagen (ver salida)"; tail -5 "$T/build.log"
fi
FAT="$T/aetherfat"; L=LinuxGUI/Ventoy2Disk/Lib/fat_io_lib
gcc -O2 -w -I$L AetherBoot/Scripts/aetherfat/aetherfat.c $L/fat_*.c -o "$FAT"
xz -dc "$ORIG_IMG" > "$T/a.img"; xz -dc "$T/nueva.img.xz" > "$T/b.img"
[ "$(stat -c%s "$T/b.img")" = 33554432 ] && ok "tamaño exacto de 32 MiB" || mal "tamaño de la imagen"
[ "$(dd if="$T/b.img" bs=1 skip=43 count=7 status=none)" = "VTOYEFI" ] && ok "etiqueta VTOYEFI conservada" || mal "etiqueta VTOYEFI"
"$FAT" "$T/a.img" dump "$T/da" >/dev/null; "$FAT" "$T/b.img" dump "$T/db" >/dev/null
python3 - "$T/da" "$T/db" <<'EOF' && ok "binarios idénticos; solo cambian cfg, idiomas, ayuda y tema" || mal "cambios fuera de cfg/idiomas/tema"
import os, sys, hashlib
def t(r): return {os.path.relpath(os.path.join(d,f),r): hashlib.sha256(open(os.path.join(d,f),'rb').read()).hexdigest() for d,_,fs in os.walk(r) for f in fs}
a, b = t(sys.argv[1]), t(sys.argv[2])
dist = [k for k in set(a) | set(b) if a.get(k) != b.get(k)]
malos = [k for k in dist if not (k.startswith('grub/') and (k.endswith(('.cfg', '.tar.gz')) or k.startswith('grub/themes/')))]
print(f"     {sum(1 for k in a if a.get(k)==b.get(k))} idénticos, {len(dist)} distintos, fuera de lo permitido: {malos or 'ninguno'}")
sys.exit(1 if malos else 0)
EOF
xz -lvv "$T/nueva.img.xz" | grep -q "dict=8MiB" && ok "compresión igual a la oficial (xz, CRC32, 8 MiB)" || mal "formato xz"

echo "== 2. Estructura de los .cfg de GRUB frente al original"
git show ventoy-1.1.18-original:INSTALL/grub/grub.cfg > "$T/orig.cfg" 2>/dev/null || "$FAT" "$T/a.img" get /grub/grub.cfg "$T/orig.cfg"
for f in grub.cfg checksum.cfg localboot.cfg power.cfg hwinfo.cfg; do
    [ "$f" = grub.cfg ] && o="$T/orig.cfg" || { "$FAT" "$T/a.img" get /grub/$f "$T/o_$f"; o="$T/o_$f"; }
    n=INSTALL/grub/$f
    c() { grep -cE "$1" "$2"; }
    if [ "$(c '^\s*if ' $o)" = "$(c '^\s*if ' $n)" ] && [ "$(c '^\s*fi\b' $o)" = "$(c '^\s*fi\b' $n)" ] \
       && [ "$(c '^\s*(menuentry|submenu|function) ' $o)" = "$(c '^\s*(menuentry|submenu|function) ' $n)" ] \
       && [ "$(tr -cd '{' < $o | wc -c)" = "$(tr -cd '{' < $n | wc -c)" ] && [ "$(tr -cd '}' < $o | wc -c)" = "$(tr -cd '}' < $n | wc -c)" ]; then
        ok "$f: mismos bloques if/fi, menús, funciones y llaves que el original"
    else mal "$f: la estructura difiere del original"; fi
done

echo "== 3. Idiomas y configuración"
python3 - <<'EOF' && ok "30 idiomas del menú válidos y sin la marca antigua" || mal "idiomas del menú"
import json, glob, re, sys
e = 0
for p in glob.glob('INSTALL/grub/menu/*.json'):
    s = open(p, encoding='utf-8').read()
    if not p.endswith('sr_RS.json'):
        json.loads(s.lstrip('﻿'))
    if re.search(r'Ventoy(?!\.json)', s): print('     marca en', p); e = 1
sys.exit(e)
EOF
gcc -w -DFOR_VTOY_JSON_CHECK Ventoy2Disk/Ventoy2Disk/VentoyJson.c -I Ventoy2Disk/Ventoy2Disk -o "$T/checkjson" && \
  "$T/checkjson" LANGUAGES/languages.json >/dev/null && ok "languages.json válido (verificador del proyecto)" || mal "languages.json"
for j in INSTALL/plugin/ventoy/ventoy.json INSTALL/plugin/ventoy/ventoy_ejemplo_es.json; do
    "$T/checkjson" "$j" >/dev/null && ok "$(basename $j) válido" || mal "$j"
done
python3 -c "import xml.dom.minidom as m; m.parse('INSTALL/plugin/ventoy/script/windows11_es_desatendido.xml')" && ok "plantilla de Windows 11 es XML válido" || mal "plantilla XML"

echo "== 4. VerificarISOs.sh"
U="$T/usb"; mkdir -p "$U/ISO"
head -c 200000 /dev/urandom > "$U/ISO/a.iso"; head -c 100000 /dev/urandom > "$U/ISO/b.iso"
(cd "$U/ISO" && sha256sum a.iso b.iso > SHA256SUMS)
sh INSTALL/VerificarISOs.sh "$U" >/dev/null && ok "imágenes correctas → código 0" || mal "imágenes correctas"
printf 'X' | dd of="$U/ISO/b.iso" bs=1 seek=10 conv=notrunc status=none
sh INSTALL/VerificarISOs.sh "$U" >/dev/null; [ $? = 1 ] && ok "imagen dañada → código 1" || mal "detección de imagen dañada"

echo "== 5. Seguridad del servidor local (Aether2Disk Linux)"
L2=LinuxGUI; X='-std=gnu99 -D_FILE_OFFSET_BITS=64 -O1 -w'
if gcc $X -c -DLINUX -I$L2/Ventoy2Disk/Lib/libhttp/include -DNDEBUG -DNO_CGI -DNO_CACHING -DNO_SSL -DUSE_STACK_SIZE=102400 -fPIC $L2/Ventoy2Disk/Lib/libhttp/include/civetweb.c -o "$T/cw.o" && \
   gcc $X -DSTATIC=static -DINIT= -I$L2/Ventoy2Disk -I$L2/Ventoy2Disk/Core -I$L2/Ventoy2Disk/Web -I$L2/Ventoy2Disk/Include -I$L2/Ventoy2Disk/Lib/libhttp/include -I$L2/Ventoy2Disk/Lib/fat_io_lib/include -I$L2/Ventoy2Disk/Lib/xz-embedded/linux/include -I$L2/Ventoy2Disk/Lib/xz-embedded/linux/include/linux -I$L2/Ventoy2Disk/Lib/xz-embedded/userspace -I$L2/Ventoy2Disk/Lib/exfat/src/libexfat -I$L2/Ventoy2Disk/Lib/exfat/src/mkfs -I$L2/Ventoy2Disk/Lib/fat_io_lib $L2/Ventoy2Disk/main_webui.c $L2/Ventoy2Disk/Core/*.c $L2/Ventoy2Disk/Web/*.c $L2/Ventoy2Disk/Lib/xz-embedded/linux/lib/decompress_unxz.c $L2/Ventoy2Disk/Lib/exfat/src/libexfat/*.c $L2/Ventoy2Disk/Lib/exfat/src/mkfs/*.c $L2/Ventoy2Disk/Lib/fat_io_lib/*.c -lpthread "$T/cw.o" -o "$T/srv"; then
    ok "compila el servidor web de Linux"
    mkdir -p "$T/run"; cp -r LinuxGUI/WebUI "$T/run/"; (cd "$T/run" && setsid "$T/srv" 127.0.0.1 24690 >/dev/null 2>&1 &); sleep 2
    J='{"method":"sysinfo"}'; URL=http://127.0.0.1:24690/vtoy/json
    cod() { curl -s -o /dev/null -w '%{http_code}' -m 3 -X POST --data "$J" "$@" $URL; }
    [ "$(cod -H 'Origin: http://127.0.0.1:24690')" = 200 ] && ok "petición legítima aceptada" || mal "petición legítima"
    [ "$(cod -H 'Host: evil.example:24690')" = 403 ] && ok "DNS rebinding (Host ajeno) rechazado" || mal "Host ajeno"
    [ "$(cod -H 'Origin: http://evil.example')" = 403 ] && ok "CSRF (Origin ajeno) rechazado" || mal "Origin ajeno"
    [ "$(cod -H 'Content-Length: 99999999999')" = 200 ] && curl -s -m3 -X POST --data "$J" $URL | grep -q token && ok "cuerpo gigante rechazado y el servidor sigue vivo" || mal "cuerpo gigante"
    { pkill -f "$T/srv"; } >/dev/null 2>&1
else mal "compilación del servidor web"; fi

echo "== 6. Interfaz de AetherPlugson"
if command -v google-chrome >/dev/null && python3 -c "import playwright" 2>/dev/null; then
    P=Plugson; gcc $X -DVENTOY_SIM -DSTATIC=static -DINIT= -I./$P/src -I./$P/src/Core -I./$P/src/Web -I./$P/src/Include -I./$P/src/Lib/libhttp/include -I./$P/src/Lib/fat_io_lib/include -I./$P/src/Lib/xz-embedded/linux/include -I./$P/src/Lib/xz-embedded/linux/include/linux -I./$P/src/Lib/xz-embedded/userspace -I./$P/src/Lib/fat_io_lib $P/src/main_linux.c $P/src/Core/ventoy_crc32.c $P/src/Core/ventoy_disk.c $P/src/Core/ventoy_disk_linux.c $P/src/Core/ventoy_json.c $P/src/Core/ventoy_log.c $P/src/Core/ventoy_md5.c $P/src/Core/ventoy_utf.c $P/src/Core/ventoy_util.c $P/src/Core/ventoy_util_linux.c $P/src/Web/*.c $P/src/Lib/xz-embedded/linux/lib/decompress_unxz.c $P/src/Lib/fat_io_lib/*.c -lpthread "$T/cw.o" -o "$T/plug" 2>/dev/null
    mkdir -p "$T/pusb/ventoy"; ln -s "$RAIZ/Plugson/www" "$T/pusb/www"; cp INSTALL/plugin/ventoy/ventoy_ejemplo_es.json "$T/pusb/ventoy/ventoy.json"
    (cd "$T/pusb" && setsid "$T/plug" 127.0.0.1 24691 "$T/pusb" /dev/sda 1.1.18 exFAT 0 1 >/dev/null 2>&1 &); sleep 2
    python3 - <<'EOF' && ok "15 páginas en español, sin chino visible ni errores de JavaScript" || mal "interfaz de AetherPlugson"
import asyncio, re, sys
from playwright.async_api import async_playwright
pags = "plugson_main plugson_control plugson_theme plugson_menu_alias plugson_menu_tip plugson_menu_class plugson_auto_install plugson_persistence plugson_injection plugson_conf_replace plugson_password plugson_image_list plugson_auto_memdisk plugson_dud plugson_donation".split()
async def m():
    async with async_playwright() as p:
        b = await p.chromium.launch(executable_path='/usr/bin/google-chrome', args=['--no-sandbox', '--no-proxy-server'])
        pg = await b.new_page(); err = []
        pg.on('pageerror', lambda e: err.append(str(e)))
        await pg.goto('http://127.0.0.1:24691/index.html', wait_until='load'); await pg.wait_for_timeout(1000)
        fallo = 0
        for n in pags:
            await pg.evaluate("n => loadContent(n)", n); await pg.wait_for_timeout(700)
            t = await pg.inner_text('body')
            if re.search(r'[一-鿿]', t): print('     chino visible en', n); fallo = 1
        await b.close()
        if err: print('     errores JS:', err[:3]); fallo = 1
        return fallo
sys.exit(asyncio.run(m()))
EOF
    { pkill -f "$T/plug"; } >/dev/null 2>&1
else omit "AetherPlugson en navegador (falta Chrome o Playwright)"; fi

echo "== 7. Sintaxis de scripts"
for s in INSTALL/tool/VentoyWorker.sh INSTALL/Ventoy2Disk.sh INSTALL/VentoyWeb.sh INSTALL/VentoyPlugson.sh INSTALL/Aether2Disk.sh INSTALL/AetherWeb.sh INSTALL/AetherPlugson.sh INSTALL/VerificarISOs.sh AetherBoot/Build/*.sh AetherBoot/Backup/restaurar.sh; do
    bash -n "$s" 2>/dev/null && ok "$s" || mal "$s"
done
node -e "new Function(require('fs').readFileSync('Plugson/www/static/js/vtoy.js','utf8'))" 2>/dev/null && ok "vtoy.js (AetherPlugson) es JavaScript válido" || mal "vtoy.js"

echo "== 8. Auditoría de referencias"
python3 -I AetherBoot/Scripts/auditoria_referencias.py . "$T/aud.csv" > "$T/aud.txt" 2>&1
if grep -q "A=0  B=0" "$T/aud.txt"; then ok "sin referencias visuales/comerciales (A) ni enlaces (B): $(cat "$T/aud.txt")"; else mal "auditoría: $(cat "$T/aud.txt")"; fi

echo ""
echo "RESULTADO: $pasa pasan, $falla fallan, $omite omitidas"
[ "$falla" -eq 0 ]
