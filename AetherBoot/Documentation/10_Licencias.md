# 10. Licencias y componentes de terceros

## AetherBoot

AetherBoot es una distribución personalizada **basada en la tecnología Ventoy**, creada por
**longpanda** (https://github.com/ventoy/Ventoy). Ventoy se publica bajo la **GNU General Public
License versión 3 o posterior** y, por tanto, AetherBoot también:

- El texto completo de la licencia está en [`COPYING`](../../COPYING).
- Quien distribuya AetherBoot (binarios o paquetes) debe ofrecer también su **código fuente completo**,
  incluidas las modificaciones. Este repositorio lo cumple.
- Se conservan todos los avisos de copyright del código original.
- Los cambios de AetherBoot están documentados en el historial de git y en
  [`historial/`](historial).

## Componentes de terceros

El motor incluye componentes de terceros con licencias propias, que se conservan sin cambios en
[`License/`](../../License): GRUB2 (GPLv3), EDK2 (BSD), iPXE (GPLv2), wimboot (GPLv2), BusyBox (GPLv2),
exFAT/fuse, 7-Zip, ImDisk, LZ4, LZO, zlib, zstd, liblzma, smallz4, lunzip, fat_io_lib, civetweb
(libhttp), Tiny Core, Porteus Kiosk y otros. `BLOB_List.md` detalla el origen de cada binario incluido.

## Identidad gráfica

Los logotipos e iconos de AetherBoot, Aether2Disk, AetherPlugson y AetherVInk son recursos propios del
proyecto AetherBoot, proporcionados por su autor (ver `AetherBoot/Branding/Originales`). Los temas
de menú `aetherboot` y `moderno`, y sus iconos, se generan con los scripts de `AetherBoot/Scripts`.

## Nombres técnicos conservados

Por compatibilidad, AetherBoot mantiene nombres internos de Ventoy: la etiqueta de partición `Ventoy`,
la partición `VTOYEFI`, la carpeta `ventoy/`, el archivo `ventoy.json` y los nombres de archivos
internos del motor. No son la marca del producto, sino identificadores que el motor y los sistemas
operativos usan para encontrar el USB. El informe de auditoría (`AetherBoot/Informes`) los enumera
uno por uno con el motivo de su conservación.

## Garantía

AetherBoot se ofrece **sin ninguna garantía**, en los términos de la GPLv3.
