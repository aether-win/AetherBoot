# 8. Compatibilidad e informe de compatibilidad

## Qué cambió AetherBoot y qué conservó

AetherBoot **no modifica el motor de arranque compilado**. En la partición de arranque `VTOYEFI` solo
se sustituyeron archivos de configuración y de apariencia de GRUB. Este es el resultado de la
comparación archivo por archivo con la imagen original de Ventoy 1.1.18:

| Elemento | Estado |
|---|---|
| Binarios EFI firmados (`bootx64.efi`, `grubx64_real.efi`, `mmx64.efi`, ia32, aa64, mips) | **Idénticos** byte a byte |
| GRUB para BIOS (`core.img`, `boot.img`) y sus módulos | **Idénticos** |
| `ventoy.cpio`, controladores EFI, wimboot, memdisk, iPXE, vtoyjump | **Idénticos** |
| Certificado `ENROLL_THIS_KEY_IN_MOKMANAGER.cer` | **Idéntico** |
| Etiqueta de partición `VTOYEFI` y estructura de particiones | **Sin cambios** |
| `grub/*.cfg` | Cambiados: textos en español, marca y tema por defecto |
| `grub/menu.tar.gz`, `grub/help.tar.gz`, `grub/menulang.cfg` | Cambiados: idiomas y ayuda |
| `grub/themes/` | Tema `ventoy` sustituido por el tema `aetherboot` |

En total, 960 archivos idénticos (897 de ellos binarios) y solo 8 archivos de configuración
modificados, más el tema.

## Consecuencias

- **Secure Boot:** funciona igual que en el original, porque los binarios firmados no cambian. GRUB
  no verifica la firma de los archivos `.cfg` ni de los temas.
- **UEFI x64 / IA32 / ARM64 / MIPS y BIOS heredado:** los mismos que el original.
- **Formatos y modos de arranque:** los mismos (ISO, WIM, IMG, VHD(x), EFI, VTOY; modos normal,
  WIMBOOT, GRUB2, Memdisk y compatible).
- **`ventoy.json`:** el mismo formato; las configuraciones existentes siguen funcionando.
- **Actualización desde Ventoy y vuelta a Ventoy:** posibles en ambos sentidos con **Actualizar**.
- **Comprobación de dispositivo del motor:** el GRUB original comprueba la estructura del USB
  (particiones, etiqueta `VTOYEFI`, presencia de ciertos archivos). AetherBoot respeta todos esos
  puntos, así que **no** aparece el aviso "[Unofficial …]".

## Textos que conservan la marca original

Estos textos están **dentro de binarios compilados y firmados** (GRUB, shim, `ventoy.cpio`). Cambiarlos
exigiría recompilar y volver a firmar, lo que rompería Secure Boot. Solo aparecen en situaciones de
error o diagnóstico:

- "This is NOT a standard Ventoy device…": solo si el USB no se preparó correctamente.
- "[Unofficial Ventoy]": solo en el mismo caso.
- "Ventoy scanning files, please wait…": un instante, con muchísimos archivos.
- "Ventoy Secure Policy": en F5 → información de Secure Boot.

La etiqueta de versión que el GRUB original dibuja en la pantalla gráfica se coloca fuera de la
pantalla (variables `VTLE_*`). En su lugar, el tema AetherBoot muestra "AetherBoot <versión>
<plataforma>".

## Resoluciones probadas en el diseño

El tema usa posiciones relativas y se comprobó, por cálculo, que ningún elemento se solapa en
800×600, 1024×768 (por defecto), 1280×1024 y 1920×1080.

## Pruebas: lo que se ejecutó y lo que no

| Prueba | Resultado |
|---|---|
| Construcción de la partición de arranque y verificación byte a byte de lo escrito | ✅ Ejecutada: 70 archivos verificados |
| Comparación de binarios críticos contra el original | ✅ Ejecutada: 897 idénticos, 0 cambios fuera de cfg/temas/idiomas |
| Formato de compresión igual al original (xz, CRC32, diccionario de 8 MiB) | ✅ Ejecutada |
| AetherPlugson (servidor Linux en modo simulación + Chrome): las 15 páginas | ✅ Ejecutada: todo en español, sin errores de JavaScript |
| Aether2Disk para Linux (servidor web + Chrome) | ✅ Ejecutada: sin errores de JavaScript |
| Protección Host/Origin de los servidores locales | ✅ Ejecutada |
| `VerificarISOs.sh` con imágenes reales y una dañada | ✅ Ejecutada |
| Sintaxis de los scripts de shell modificados | ✅ Ejecutada |
| **Arranque real en UEFI/BIOS (VM o hardware)** | ❌ **No ejecutada**: el entorno de desarrollo no tiene QEMU ni hardware. Debe probarse antes de distribuir. |
| **Compilación de Aether2Disk.exe, AetherPlugson.exe y AetherVInk.exe** | ❌ **No ejecutada**: requiere Visual Studio (Windows). Se incluye un flujo de GitHub Actions. |
| Sintaxis de `grub.cfg` con `grub-script-check` | ❌ No disponible; revisado línea por línea a mano |

Lista de pruebas recomendadas antes de publicar, en una máquina virtual (QEMU/VirtualBox con OVMF)
y en un equipo real:

1. Instalación MBR y GPT; actualización desde Ventoy 1.1.18.
2. Arranque UEFI con Secure Boot activado y desactivado; arranque BIOS heredado.
3. Menú: se ve el tema AetherBoot, la versión dinámica y los atajos en español.
4. F1–F7, L, m; cambio de tema en F5.
5. Arranque de una ISO de Windows 11, una de WinPE y una de Linux (Ubuntu).
6. Resoluciones 1024×768 y 1920×1080 (opción `gfxmode` del tema).
