#!/bin/bash
# ============================================================================
#  prueba_arranque.sh — prueba real de arranque de AetherBoot en QEMU.
#
#  1. Instala AetherBoot en un disco virtual con el instalador de Linux
#     (Ventoy2Disk.sh / VentoyWorker.sh del repositorio) y el paquete
#     compilado (partición de arranque AetherBoot).
#  2. Comprueba las etiquetas: partición de datos "Aether", arranque "VTOYEFI".
#  3. Crea una ISO de prueba mínima (núcleo Linux + busybox) que escribe
#     AETHERBOOT_PRUEBA_OK por el puerto serie y se apaga.
#  4. Arranca el disco en QEMU en modo BIOS y en modo UEFI y verifica que se
#     ve el menú y que la ISO arranca (prueba_qemu.py).
#
#  Uso (necesita root, p. ej. en GitHub Actions):
#    sudo bash prueba_arranque.sh <aetherboot-X-windows.zip> <carpeta_resultados>
#
#  Paquetes necesarios (Ubuntu): qemu-system-x86 ovmf busybox-static
#    grub-pc-bin grub-efi-amd64-bin grub-common xorriso mtools exfatprogs
# ============================================================================
set -euo pipefail

ZIP="$(realpath "$1")"
OUT="$(realpath -m "$2")"
RAIZ="$(cd "$(dirname "$0")/../.." && pwd)"
TMP="$(mktemp -d)"
LOOP=""

limpiar() {
    set +e
    mountpoint -q "$TMP/p1" && umount "$TMP/p1"
    [ -n "$LOOP" ] && losetup -d "$LOOP"
    rm -rf "$TMP"
}
trap limpiar EXIT

mkdir -p "$OUT"

echo "==> Preparando instalador de Linux con el paquete compilado"
unzip -q "$ZIP" -d "$TMP/pkg"
PKG="$(find "$TMP/pkg" -mindepth 1 -maxdepth 1 -type d | head -n1)"
cp -a "$RAIZ/INSTALL/tool" "$PKG/tool"
cp "$RAIZ/INSTALL/Ventoy2Disk.sh" "$PKG/"
test -f "$PKG/boot/boot.img"
test -f "$PKG/boot/core.img.xz"
test -f "$PKG/ventoy/ventoy.disk.img.xz"

echo "==> Disco virtual de 3 GB"
truncate -s 3G "$TMP/disk.img"
LOOP="$(losetup --show -fP "$TMP/disk.img")"
echo "    $LOOP"

echo "==> Instalando AetherBoot (MBR)"
( cd "$PKG" && printf 'y\ny\n' | bash ./Ventoy2Disk.sh -I "$LOOP" ) | tee "$OUT/instalacion.log"
partprobe "$LOOP" 2>/dev/null || true
sleep 2

L1="$(blkid -s LABEL -o value "${LOOP}p1" || true)"
L2="$(blkid -s LABEL -o value "${LOOP}p2" || true)"
echo "    Etiqueta partición 1: '$L1'   partición 2: '$L2'"
if [ "$L1" != "Aether" ]; then echo "ERROR: la partición de datos no se llama Aether"; exit 1; fi
if [ "$L2" != "VTOYEFI" ]; then echo "ERROR: la partición de arranque no se llama VTOYEFI"; exit 1; fi

echo "==> Creando ISO de prueba"
mkdir -p "$TMP/iso/boot/grub" "$TMP/initrd/bin" "$TMP/initrd/dev" "$TMP/initrd/proc" "$TMP/initrd/sys"
NUCLEO="$(ls -1 /boot/vmlinuz-* | sort -V | tail -n1)"
cp "$NUCLEO" "$TMP/iso/boot/vmlinuz"
cp "$(command -v busybox)" "$TMP/initrd/bin/busybox"
file "$TMP/initrd/bin/busybox" | grep -q "statically linked" || { echo "ERROR: busybox no es estático"; exit 1; }
cat > "$TMP/initrd/init" <<'EOF'
#!/bin/busybox sh
/bin/busybox mount -t devtmpfs devtmpfs /dev 2>/dev/null
/bin/busybox mount -t proc proc /proc 2>/dev/null
for t in /dev/ttyS0 /dev/console; do
    echo "AETHERBOOT_PRUEBA_OK" > $t 2>/dev/null
done
/bin/busybox sleep 2
/bin/busybox poweroff -f
EOF
chmod +x "$TMP/initrd/init"
( cd "$TMP/initrd" && find . | cpio -o -H newc --quiet | gzip -9 ) > "$TMP/iso/boot/initrd.img"
cat > "$TMP/iso/boot/grub/grub.cfg" <<'EOF'
set timeout=1
set default=0
menuentry "Prueba AetherBoot" {
    linux /boot/vmlinuz console=tty0 console=ttyS0,115200 panic=10
    initrd /boot/initrd.img
}
EOF
grub-mkrescue -o "$TMP/aether_prueba.iso" "$TMP/iso" -- -volid AETHER_PRUEBA >/dev/null 2>&1
ls -l "$TMP/aether_prueba.iso"

echo "==> Copiando la ISO a la partición Aether"
mkdir -p "$TMP/p1"
mount "${LOOP}p1" "$TMP/p1"
cp "$TMP/aether_prueba.iso" "$TMP/p1/"
sync
umount "$TMP/p1"
losetup -d "$LOOP"; LOOP=""

OVMF_CODE=""; OVMF_VARS=""
for c in /usr/share/OVMF/OVMF_CODE_4M.fd /usr/share/OVMF/OVMF_CODE.fd; do [ -f "$c" ] && { OVMF_CODE="$c"; break; }; done
for v in /usr/share/OVMF/OVMF_VARS_4M.fd /usr/share/OVMF/OVMF_VARS.fd; do [ -f "$v" ] && { OVMF_VARS="$v"; break; }; done

RC=0
echo "==> Arranque en modo BIOS (Legacy)"
python3 "$RAIZ/AetherBoot/Tests/prueba_qemu.py" --modo bios --disco "$TMP/disk.img" --salida "$OUT" || RC=1

echo "==> Arranque en modo UEFI"
python3 "$RAIZ/AetherBoot/Tests/prueba_qemu.py" --modo uefi --disco "$TMP/disk.img" --salida "$OUT" \
        --ovmf-code "$OVMF_CODE" --ovmf-vars "$OVMF_VARS" || RC=1

rm -f "$OUT"/*.sock "$OUT"/OVMF_VARS-copia.fd
chmod -R a+r "$OUT"
if [ $RC -eq 0 ]; then
    echo "RESULTADO: prueba de arranque CORRECTA (BIOS y UEFI)"
else
    echo "RESULTADO: prueba de arranque con FALLOS (ver capturas y registros)"
fi
exit $RC
