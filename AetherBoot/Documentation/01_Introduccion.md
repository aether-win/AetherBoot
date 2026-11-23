# 1. Introducción a AetherBoot

AetherBoot es un gestor de arranque multisistema para memorias USB. Después de instalarlo una vez,
el USB queda dividido en dos partes:

| Partición | Etiqueta | Contenido |
|---|---|---|
| 1 (la grande) | `Aether` | Sus imágenes ISO/WIM/IMG/VHD(x)/EFI y la carpeta de configuración `ventoy/`. Se puede usar como un USB normal. |
| 2 (32 MB) | `VTOYEFI` | El menú de arranque AetherBoot (GRUB2, temas, idiomas). No hay que tocarla. |

> La etiqueta `Aether` de la partición de datos se puede cambiar libremente (clic derecho → Cambiar
> nombre); los USB preparados con Ventoy conservan la etiqueta `Ventoy` y funcionan igual. En cambio,
> la etiqueta `VTOYEFI`, la carpeta `ventoy/` y el archivo `ventoy.json` mantienen esos nombres a
> propósito: el motor de arranque los busca por nombre, y cambiarlos dejaría el USB sin arrancar.
> Son nombres técnicos, no la marca del producto.

## Cómo se usa, en tres pasos

1. **Instale AetherBoot en el USB** con Aether2Disk ([capítulo 2](02_Instalacion.md)).
2. **Copie las imágenes** a la primera partición, en cualquier carpeta.
3. **Arranque el equipo desde el USB** (menú de arranque de la BIOS/UEFI, normalmente F12, F11, F8 o
   Esc) y elija el sistema en el menú de AetherBoot.

AetherBoot detecta solo las imágenes que haya en el USB: no hay que configurar nada para añadir o
quitar sistemas.

## Las aplicaciones del ecosistema

- **AetherBoot:** el menú que aparece al arrancar desde el USB, en español, con tema propio.
- **Aether2Disk:** instala, actualiza o limpia AetherBoot en el USB. En Windows es `Aether2Disk.exe`;
  en Linux, `Aether2Disk.sh` (consola) o `AetherWeb.sh` (interfaz web).
- **AetherPlugson:** editor visual de las opciones avanzadas (`/ventoy/ventoy.json`).
- **AetherVInk:** crea enlaces `.vlnk` para arrancar imágenes guardadas en otro disco.
- **VerificarISOs:** comprueba las sumas de verificación de todas las imágenes del USB
  (`VerificarISOs.ps1` en Windows, `VerificarISOs.sh` en Linux).

## Formatos admitidos

ISO, WIM, IMG, VHD/VHDX, EFI y VTOY (discos virtuales de Linux). Son los mismos que admite el motor
original; AetherBoot no añade ni quita ninguno.
