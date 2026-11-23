# 11. Compilación

## Qué hay que compilar y qué no

| Pieza | Cómo se genera | ¿Requiere compilar? |
|---|---|---|
| Menú de arranque (partición `VTOYEFI`) | `AetherBoot/Build/construir_imagen_arranque.sh` a partir de la imagen oficial | No: solo sustituye configuración, idiomas y tema |
| Interfaz de AetherPlugson (`plugson.tar.xz`) | `AetherBoot/Build/empaquetar_windows.sh` | No: es HTML/CSS/JS |
| Textos de Aether2Disk (`languages.json`) | Se copia tal cual | No |
| **Aether2Disk.exe, AetherPlugson.exe, AetherVInk.exe** | Visual Studio (C, Win32) | **Sí**, en Windows |
| Herramientas de Linux (servidor web, Plugson, vlnk) y GRUB | Sistema de compilación oficial (Docker) | Sí, en Linux |

## Opción A: GitHub Actions (recomendada)

1. Suba este código a un repositorio de GitHub.
2. En **Actions → AetherBoot → Run workflow**, ejecute el flujo `.github/workflows/aetherboot.yml`.
3. Al terminar, descargue el artefacto **aetherboot-windows**: contiene
   `aetherboot-1.1.18-windows.zip` con los tres programas ya compilados y renombrados.

El flujo descarga el paquete oficial de Ventoy 1.1.18 y **verifica su SHA-256** antes de usarlo.

> Los proyectos se crearon con Visual Studio 2019 (v142). El flujo compila con v143 (Visual Studio
> 2022), que es compatible. La plataforma ARM de 32 bits es opcional: si el SDK del servidor ya no la
> incluye, el paquete sale sin `Aether2Disk_ARM.exe`.

## Opción B: Visual Studio en Windows

1. Instale Visual Studio 2022 con "Desarrollo para el escritorio con C++" (incluya las herramientas
   ARM64 si las necesita).
2. Abra y compile en **Release**:
   - `Ventoy2Disk\Ventoy2Disk.sln` (Win32, x64, ARM64) → Aether2Disk
   - `Plugson\vs\VentoyPlugson\VentoyPlugson.sln` (Win32, x64) → AetherPlugson
   - `Vlnk\vs\VentoyVlnk\VentoyVlnk.sln` (Win32) → AetherVInk
3. Copie los `.exe` generados (con sus nombres originales) a una carpeta, por ejemplo `exe\`.
4. En Linux o WSL:

   ```sh
   bash AetherBoot/Build/construir_imagen_arranque.sh ventoy-1.1.18/ventoy/ventoy.disk.img.xz ventoy.disk.img.xz
   bash AetherBoot/Build/empaquetar_windows.sh ventoy-1.1.18-windows.zip ventoy.disk.img.xz \
        aetherboot-1.1.18-windows.zip exe
   ```

Los nombres de los proyectos y de los `.exe` intermedios siguen siendo los originales para no romper
las referencias entre proyectos. El empaquetador los renombra a `Aether2Disk.exe`,
`AetherPlugson.exe` y `AetherVInk.exe`.

## Paquete de Linux completo

El repositorio conserva el flujo oficial `.github/workflows/ci.yml`, que compila todo el proyecto con
Docker (`docker compose up`) y usa los archivos de `INSTALL/`, ya adaptados a AetherBoot. Así se
generan también las herramientas de Linux con las correcciones de seguridad.

## Scripts de AetherBoot

| Script | Función |
|---|---|
| `Build/construir_imagen_arranque.sh` | Partición de arranque AetherBoot, verificada byte a byte |
| `Build/empaquetar_windows.sh` | Paquete de Windows (final o de prueba) |
| `Scripts/aetherfat/aetherfat.c` | Lee y escribe la imagen FAT sin montarla |
| `Scripts/generar_tema_aetherboot.py` | Genera el tema del menú a partir del logotipo |
| `Scripts/simular_menu.py` | Vista previa aproximada del menú |
| `Scripts/traducir_grub.py` | Traducción de los mensajes de GRUB |
| `Scripts/plugson_es/` | Extracción y aplicación de la traducción de AetherPlugson |
| `Scripts/auditoria_referencias.py` | Auditoría de referencias (A/B/C) |
| `Scripts/limpiar_logo.py`, `quitar_sombra_negra.py`, `lema_es.py`, `derivados.py` | Preparación de los logotipos e iconos |

## Prueba de arranque automática

El trabajo **prueba-arranque** de `.github/workflows/aetherboot.yml` comprueba en cada compilación
que el paquete realmente arranca:

1. Instala AetherBoot en un disco virtual de 3 GB con el instalador de Linux del repositorio.
2. Comprueba que la partición de datos se llama `Aether` y la de arranque `VTOYEFI`.
3. Crea una ISO de prueba mínima (núcleo Linux + busybox) que escribe `AETHERBOOT_PRUEBA_OK` por el
   puerto serie.
4. Arranca el disco en QEMU en modo **BIOS** y en modo **UEFI**, captura la pantalla del menú y
   elige la ISO.

La prueba pasa solo si el menú se ve con el tema de AetherBoot y la ISO arranca en los dos modos.
Las capturas y registros quedan en el artefacto `prueba-arranque-capturas`.
Scripts: `AetherBoot/Tests/prueba_arranque.sh` y `AetherBoot/Tests/prueba_qemu.py`.

## AetherFUSboot

El trabajo **aetherfusboot** compila `AetherFUSboot/rufus.sln` para x64, x86 y ARM64 y publica el
artefacto `aetherfusboot-windows`. Detalles en `AetherFUSboot/LEEME_AetherFUSboot.md`.

