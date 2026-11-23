#!/usr/bin/env python3
"""Traduce al español los mensajes visibles de los .cfg de GRUB (AetherBoot).
Solo reemplaza cadenas literales exactas; no toca variables, rutas ni lógica."""
import re, sys, os

G = sys.argv[1]  # carpeta INSTALL/grub

MAPA = {
 "grub.cfg": [
  ('echo "press Enter to continue ......"', 'echo "Presione Enter para continuar ......"'),
  ('echo "ventoy_grub.cfg NOT exist."', 'echo "No existe ventoy_grub.cfg."'),
  ('echo "NetBSD not supported"', 'echo "NetBSD no es compatible"'),
  ('echo "Unknown unix type"', 'echo "Tipo de Unix desconocido"'),
  ('echo "No bootfile found for UEFI!"', 'echo "¡No se encontró un archivo de arranque UEFI!"'),
  ('echo "Maybe the image does not support $VTOY_EFI_ARCH UEFI"', 'echo "Puede que la imagen no admita UEFI $VTOY_EFI_ARCH"'),
  ("echo 'Loading ISO file to memory ...'", "echo 'Cargando la imagen ISO en memoria ...'"),
  ('echo "Loading ISO file to memory ..."', 'echo "Cargando la imagen ISO en memoria ..."'),
  ('echo -e "\\nThis is 32bit Windows and does NOT support x86_64 UEFI firmware.\\n"', 'echo -e "\\nEste Windows es de 32 bits y NO es compatible con firmware UEFI x86_64.\\n"'),
  ('==================== VENTOY WIMBOOT ==================', '================== AETHERBOOT WIMBOOT ================'),
  ('set ventoy_loading_tip="Loading files ......"', 'set ventoy_loading_tip="Cargando archivos ......"'),
  ('echo -e "\\n The name of the iso file could NOT contain space or non-ascii characters. \\n"', 'echo -e "\\n El nombre de la imagen ISO NO puede contener espacios ni caracteres no ASCII. \\n"'),
  ('echo -e "\\n This is NOT a bootable WIM file. \\n"', 'echo -e "\\n Este NO es un archivo WIM arrancable. \\n"'),
  ('echo -e "\\n### VLNK FILE NOT FOUND ###\\n### VLNK 文件不存在 ###\\n"', 'echo -e "\\n### NO SE ENCONTRÓ EL ARCHIVO VLNK ###\\n"'),
  ('echo "Please put the right ventoy_vhdboot.img file to the 1st partition"', 'echo "Copie el archivo ventoy_vhdboot.img correcto en la primera partición"'),
  ('echo -e "!!! WARNING !!!\\n"', 'echo -e "!!! ADVERTENCIA !!!\\n"'),
  ('echo -e "\\nPartition1 ($vtoy_iso_fs) is NOT ntfs, the VHD(x) file may not boot normally \\n"', 'echo -e "\\nLa partición 1 ($vtoy_iso_fs) NO es NTFS; el archivo VHD(x) podría no arrancar correctamente \\n"'),
  ('echo "The OS in the vdisk was created in UEFI mode, but current is Legacy BIOS mode."', 'echo "El sistema del disco virtual se creó en modo UEFI, pero el equipo está en modo BIOS heredado."'),
  ('echo "The OS in the vdisk was created in Legacy BIOS mode, but current is UEFI mode."', 'echo "El sistema del disco virtual se creó en modo BIOS heredado, pero el equipo está en modo UEFI."'),
  ('echo "Unsupported vtoy type $vtoytype"', 'echo "Tipo vtoy no compatible: $vtoytype"'),
  ('echo -e "\\n This IMG file is only supported in UEFI mode. \\n"', 'echo -e "\\n Este archivo IMG solo es compatible en modo UEFI. \\n"'),
  ('echo -e "\\nPlease extend the img file size before boot it. \\n"', 'echo -e "\\nAmplíe el tamaño del archivo IMG antes de arrancarlo. \\n"'),
  ('echo -e "\\n ventoy_openwrt.xz not found. Please refer https://www.ventoy.net/en/doc_openwrt.html.\\n"', 'echo -e "\\n No se encontró ventoy_openwrt.xz. Consulte la documentación de AetherBoot (sección OpenWrt).\\n"'),
  ('echo -e "\\n IMG file need processed. Please refer https://www.ventoy.net/en/doc_openwrt.html.\\n"', 'echo -e "\\n Este archivo IMG debe procesarse antes. Consulte la documentación de AetherBoot (sección OpenWrt).\\n"'),
  ('echo -e "\\n press ENTER to exit (请按 回车 键返回) ..."', 'echo -e "\\n Presione ENTER para salir ..."'),
  ('echo -e "\\n This IMG file is NOT supported now. \\n"', 'echo -e "\\n Este archivo IMG NO es compatible por ahora. \\n"'),
  ('echo "Loading img file to memory ..."', 'echo "Cargando el archivo IMG en memoria ..."'),
  ('set VTOY_MEM_DISK_STR="[Memdisk]"', 'set VTOY_MEM_DISK_STR="[Memdisk]"'),
  ('set VTOY_ISO_RAW_STR="Compatible Mode"', 'set VTOY_ISO_RAW_STR="Modo compatible"'),
  ('set VTOY_GRUB2_MODE_STR="GRUB2 Mode"', 'set VTOY_GRUB2_MODE_STR="Modo GRUB2"'),
  ('set VTOY_WIMBOOT_MODE_STR="WIMBOOT Mode"', 'set VTOY_WIMBOOT_MODE_STR="Modo WIMBOOT"'),
  ('echo -e "\\n\\nPath case does not match! ventoy directory and ventoy.json MUST be all lowercase!"', 'echo -e "\\n\\n¡Las mayúsculas no coinciden! La carpeta ventoy y el archivo ventoy.json DEBEN estar en minúsculas."'),
  ('echo -e "\\n\\npress ENTER to continue (请按回车键继续) ..."', 'echo -e "\\n\\nPresione ENTER para continuar ..."'),
  ('echo -e "\\n Encoding type for ventoy.json is not supported, please convert to UTF-8.\\n"', 'echo -e "\\n La codificación de ventoy.json no es compatible; conviértalo a UTF-8.\\n"'),
  ('echo -e "\\n Syntax error detected in ventoy.json, please check! \\n"', 'echo -e "\\n Se detectó un error de sintaxis en ventoy.json. ¡Revíselo! Ninguna opción se aplicará. \\n"'),
  ('echo -e "\\n press ENTER to continue (请按 回车 键继续) ..."', 'echo -e "\\n Presione ENTER para continuar ..."'),
  ('echo -e "\\n You need to put $vtTFile in the 1st partition which hold the ISO files.\\n"', 'echo -e "\\n El archivo $vtTFile debe ir en la primera partición (la de las imágenes ISO), dentro de la carpeta ventoy.\\n"'),
  ('NO_ISO_MENU="No ISO or supported IMG files found, $VTOY_NO_ISO_TIP"', 'NO_ISO_MENU="No se encontraron imágenes ISO o IMG compatibles, $VTOY_NO_ISO_TIP"'),
  ('NO_ISO_MENU="No ISO or supported IMG files found, please check VTOY_DEFAULT_SEARCH_ROOT"', 'NO_ISO_MENU="No se encontraron imágenes ISO o IMG compatibles; revise VTOY_DEFAULT_SEARCH_ROOT"'),
  ('NO_ISO_MENU="No ISO or supported IMG files found"', 'NO_ISO_MENU="No se encontraron imágenes ISO o IMG compatibles"'),
  ('menuentry "$NO_ISO_MENU (Press enter to reboot ...)"', 'menuentry "$NO_ISO_MENU (Presione Enter para reiniciar ...)"'),
  ('echo -e "\\n    Rebooting ... "', 'echo -e "\\n    Reiniciando ... "'),
 ],
 "checksum.cfg": [("echo 'Return ...'", "echo 'Volviendo ...'")],
 "localboot.cfg": [
  ('echo "Windows NOT found ..."', 'echo "NO se encontró Windows ..."'),
  ('echo "Grub4dos NOT found ..."', 'echo "NO se encontró Grub4dos ..."'),
  ('echo "BOOTIA32.EFI NOT found ..."', 'echo "NO se encontró BOOTIA32.EFI ..."'),
  ('echo "xorboot NOT found ..."', 'echo "NO se encontró xorboot ..."'),
  ('echo "BOOTAA64.EFI NOT found ..."', 'echo "NO se encontró BOOTAA64.EFI ..."'),
  ('echo "BOOTX64.EFI NOT found ..."', 'echo "NO se encontró BOOTX64.EFI ..."'),
  ('echo "Return ..."', 'echo "Volviendo ..."'),
 ],
 "power.cfg": [
  ("echo -e '\\n\\nSystem is rebooting ... \\n'", "echo -e '\\n\\nReiniciando el sistema ... \\n'"),
  ("echo -e '\\n\\nSystem is halting ... \\n'", "echo -e '\\n\\nApagando el sistema ... \\n'"),
 ],
 "hwinfo.cfg": [
  ('echo "Platform                ', 'echo "Plataforma              '),
  ('echo "UEFI Version            ', 'echo "Versión UEFI            '),
  ('echo "PI   Version            ', 'echo "Versión PI              '),
  ('echo "Secure Boot             Enabled"', 'echo "Arranque seguro         Activado"'),
  ('echo "Secure Boot             Disabled"', 'echo "Arranque seguro         Desactivado"'),
  ('echo "CPU Model               ', 'echo "Modelo de CPU           '),
  ('echo "Physical RAM            ', 'echo "Memoria RAM física      '),
  ('echo "Manufacturer            ', 'echo "Fabricante              '),
  ('echo "Product Name            ', 'echo "Producto                '),
  ('echo "Version                 ', 'echo "Versión                 '),
  ('echo "Board Manufacturer      ', 'echo "Fabricante de placa     '),
  ('echo "Board Name              ', 'echo "Placa base              '),
  ('echo "Board Version           ', 'echo "Versión de placa        '),
  ('echo "BIOS Manufacturer       ', 'echo "Fabricante de BIOS      '),
  ('echo "BIOS Version            ', 'echo "Versión de BIOS         '),
  ('echo "BIOS Date               ', 'echo "Fecha de BIOS           '),
  ('echo "BIOS ROM Size           ', 'echo "Tamaño de ROM BIOS      '),
 ],
}

CJK = re.compile(r'[㐀-鿿＀-￯]')
total = 0
for fname, pares in MAPA.items():
    p = os.path.join(G, fname)
    s = open(p, encoding="utf-8", newline="").read()
    for a, b in pares:
        n = s.count(a)
        if n == 0:
            print(f"AVISO: no encontrado en {fname}: {a[:60]}")
        s = s.replace(a, b)
        total += n
    # quitar líneas echo que solo contienen el duplicado en chino
    lineas = s.split("\n")
    nuevas = []
    for ln in lineas:
        st = ln.strip()
        if st.startswith("echo") and CJK.search(ln) and not re.search(r'[A-Za-z]{4,}', re.sub(r'^\s*echo( -e| -en)?', '', ln).replace("\\n", "")):
            total += 1
            continue
        nuevas.append(ln)
    s = "\n".join(nuevas)
    open(p, "w", encoding="utf-8", newline="").write(s)
print("reemplazos:", total)
