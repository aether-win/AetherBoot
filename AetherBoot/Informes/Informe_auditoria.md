# Informe de auditoría de referencias a Ventoy

Generado por `AetherBoot/Scripts/auditoria_referencias.py`, que recorre todos los archivos de texto del proyecto (incluidos los UTF-16 de Windows) y los nombres de las imágenes, buscando `Ventoy`, `Ventoy2Disk`, `VentoyPlugson`, `VentoyVlnk`, `ventoy.net` y variantes. Se excluye el código de terceros incluido tal cual (GRUB base, EDK2, iPXE, BusyBox, etc.), cuyo contenido no forma parte de ninguna interfaz.

## Resumen

| Categoría | Significado | Original | AetherBoot |
|---|---|---|---|
| A | Visual o comercial visible para el usuario: sustituir | 501 | **0** |
| B | Documental o enlace innecesario: eliminar o reemplazar | 100 | **0** |
| C | Técnica o legal imprescindible: conservar | 13548 | 13699 |

**No se eliminaron todas las referencias.** Quedan 13699 de categoría C, que son necesarias para el funcionamiento, la compatibilidad o el cumplimiento de la licencia. Abajo se explica cada motivo. La lista completa (archivo, línea, referencia, categoría, motivo y contexto) está en `auditoria_referencias.csv`.

## Referencias A y B tratadas, por archivo

| Archivo | A+B en el original | Acción |
|---|---|---|
| `LANGUAGES/languages.json` | 378 | Textos visibles sustituidos por AetherBoot / español; enlaces eliminados |
| `README.md` | 76 | Sustituido por un README de AetherBoot; el original se conserva en `AetherBoot/Documentation/original/` |
| `INSTALL/tool/VentoyWorker.sh` | 21 | Textos visibles sustituidos por AetherBoot / español; enlaces eliminados |
| `INSTALL/grub/grub.cfg` | 11 | Textos visibles sustituidos por AetherBoot / español; enlaces eliminados |
| `Ventoy2Disk/Ventoy2Disk/Ventoy2Disk.rc` | 8 | Textos visibles sustituidos por AetherBoot / español; enlaces eliminados |
| `Plugson/www/plugson_control.html` | 7 | Textos visibles sustituidos por AetherBoot / español; enlaces eliminados |
| `Plugson/www/index.html` | 6 | Textos visibles sustituidos por AetherBoot / español; enlaces eliminados |
| `Plugson/www/plugson_donation.html` | 4 | Textos visibles sustituidos por AetherBoot / español; enlaces eliminados |
| `INSTALL/README` | 4 | Textos visibles sustituidos por AetherBoot / español; enlaces eliminados |
| `INSTALL/tool/VentoyGTK.glade` | 4 | Eliminado o sustituido (ya no contiene referencias A/B) |
| `Plugson/www/plugson_theme.html` | 3 | Textos visibles sustituidos por AetherBoot / español; enlaces eliminados |
| `Plugson/www/plugson_image_list.html` | 3 | Textos visibles sustituidos por AetherBoot / español; enlaces eliminados |
| `Plugson/www/plugson_main.html` | 3 | Textos visibles sustituidos por AetherBoot / español; enlaces eliminados |
| `LiveCD/VTOY/ventoy/ventoy.sh` | 3 | Textos visibles sustituidos por AetherBoot / español; enlaces eliminados |
| `INSTALL/VentoyPlugson.sh` | 3 | Textos visibles sustituidos por AetherBoot / español; enlaces eliminados |
| `LiveCDGUI/ISO/EFI/boot/grub.cfg` | 2 | Eliminado o sustituido (ya no contiene referencias A/B) |
| `LiveCD/ISO/EFI/boot/grub.cfg` | 2 | Textos visibles sustituidos por AetherBoot / español; enlaces eliminados |
| `INSTALL/grub/menu/it_IT.json` | 2 | Textos visibles sustituidos por AetherBoot / español; enlaces eliminados |
| `INSTALL/grub/menu/ar_AR.json` | 2 | Textos visibles sustituidos por AetherBoot / español; enlaces eliminados |
| `INSTALL/grub/menu/hr_HR.json` | 2 | Textos visibles sustituidos por AetherBoot / español; enlaces eliminados |
| `INSTALL/grub/menu/pt_BR.json` | 2 | Textos visibles sustituidos por AetherBoot / español; enlaces eliminados |
| `INSTALL/grub/menu/es_ES.json` | 2 | Textos visibles sustituidos por AetherBoot / español; enlaces eliminados |
| `INSTALL/grub/menu/cs_CZ.json` | 2 | Textos visibles sustituidos por AetherBoot / español; enlaces eliminados |
| `INSTALL/grub/menu/en_US.json` | 2 | Textos visibles sustituidos por AetherBoot / español; enlaces eliminados |
| `INSTALL/grub/menu/sr_RS.json` | 2 | Textos visibles sustituidos por AetherBoot / español; enlaces eliminados |
| `INSTALL/grub/menu/fr_FR.json` | 2 | Textos visibles sustituidos por AetherBoot / español; enlaces eliminados |
| `INSTALL/grub/menu/pl_PL.json` | 2 | Textos visibles sustituidos por AetherBoot / español; enlaces eliminados |
| `INSTALL/grub/menu/ru_RU.json` | 2 | Textos visibles sustituidos por AetherBoot / español; enlaces eliminados |
| `INSTALL/grub/menu/ta_IN.json` | 2 | Textos visibles sustituidos por AetherBoot / español; enlaces eliminados |
| `INSTALL/grub/menu/fa_IR.json` | 2 | Textos visibles sustituidos por AetherBoot / español; enlaces eliminados |
| `INSTALL/grub/menu/ko_KR.json` | 2 | Textos visibles sustituidos por AetherBoot / español; enlaces eliminados |
| `INSTALL/grub/menu/hu_HU.json` | 2 | Textos visibles sustituidos por AetherBoot / español; enlaces eliminados |
| `INSTALL/grub/menu/vi_VN.json` | 2 | Textos visibles sustituidos por AetherBoot / español; enlaces eliminados |
| `INSTALL/grub/menu/el_GR.json` | 2 | Textos visibles sustituidos por AetherBoot / español; enlaces eliminados |
| `INSTALL/grub/menu/tr_TR.json` | 2 | Textos visibles sustituidos por AetherBoot / español; enlaces eliminados |
| `INSTALL/grub/menu/pt_PT.json` | 2 | Textos visibles sustituidos por AetherBoot / español; enlaces eliminados |
| `INSTALL/grub/menu/lg_UG.json` | 2 | Textos visibles sustituidos por AetherBoot / español; enlaces eliminados |
| `INSTALL/grub/menu/ka_GE.json` | 2 | Textos visibles sustituidos por AetherBoot / español; enlaces eliminados |
| `INSTALL/grub/menu/sl_si.json` | 2 | Textos visibles sustituidos por AetherBoot / español; enlaces eliminados |
| `INSTALL/grub/menu/de_DE.json` | 2 | Textos visibles sustituidos por AetherBoot / español; enlaces eliminados |
| `INSTALL/grub/menu/id_ID.json` | 2 | Textos visibles sustituidos por AetherBoot / español; enlaces eliminados |
| `INSTALL/grub/menu/uk_UA.json` | 2 | Textos visibles sustituidos por AetherBoot / español; enlaces eliminados |
| `INSTALL/grub/menu/ja_JP.json` | 2 | Textos visibles sustituidos por AetherBoot / español; enlaces eliminados |
| `LiveCDGUI/initrd.sh` | 1 | Textos visibles sustituidos por AetherBoot / español; enlaces eliminados |
| `Plugson/vs/VentoyPlugson/VentoyPlugson/VentoyPlugson.rc` | 1 | Eliminado o sustituido (ya no contiene referencias A/B) |
| `LinuxGUI/WebUI/index.html` | 1 | Textos visibles sustituidos por AetherBoot / español; enlaces eliminados |
| `INSTALL/FOR_X64_ARM.txt` | 1 | Eliminado o sustituido (ya no contiene referencias A/B) |
| `INSTALL/VentoyWeb.sh` | 1 | Textos visibles sustituidos por AetherBoot / español; enlaces eliminados |
| `INSTALL/Ventoy2Disk.sh` | 1 | Textos visibles sustituidos por AetherBoot / español; enlaces eliminados |
| `INSTALL/grub/menu/bn_BN.json` | 1 | Textos visibles sustituidos por AetherBoot / español; enlaces eliminados |
| `INSTALL/grub/help/tr_TR.txt` | 1 | Eliminado o sustituido (ya no contiene referencias A/B) |
| `Ventoy2Disk/Ventoy2Disk/Res/ventoy.ico` | 1 | Eliminado o sustituido (ya no contiene referencias A/B) |
| `Plugson/www/static/img/VentoyLogo.png` | 1 | Eliminado o sustituido (ya no contiene referencias A/B) |
| `LinuxGUI/WebUI/static/img/VentoyLogo.png` | 1 | Eliminado o sustituido (ya no contiene referencias A/B) |

## Referencias conservadas (C), por motivo

| Motivo | Apariciones | Ejemplo |
|---|---|---|
| Identificador, nombre de archivo o clave interna | 7824 | `Ventoy2Disk/Ventoy2Disk/crc32.c`: `UINT32 VentoyCrc32(void *Buffer, UINT32 Length)` |
| Documentación técnica para desarrolladores (procedencia del código y de los binarios) | 2137 | `BLOB_List.md`: `<tr> <td>./IMG/cpio_arm64/ventoy/busybox/a64</td>  <td>build</td>    <` |
| Texto dentro de un binario de arranque compilado (GRUB/shim/cpio): cambiarlo exige recompilar y volver a firmar, lo que rompería Secure Boot | 1794 | `VtoyTool/vtoytool.c`: `* vtoytool.c  ---- ventoy os tool` |
| Archivo de compilación (nombres de ejecutables y rutas internas) | 598 | `LinuxGUI/Ventoy2Disk/QT/build/Makefile`: `# Makefile for building: Ventoy2Disk` |
| Documentación o scripts de AetherBoot: explican la procedencia y los nombres técnicos conservados | 388 | `AetherBoot/Documentation/06_AetherVInk.md`: ``-q` desactiva el registro. En Linux, la misma función está en `Ventoy` |
| Identificador, variable, función, ruta o nombre interno del motor | 336 | `docker-compose.yml`: `ventoy:` |
| Nombre de archivo, carpeta o formato que el motor busca (compatibilidad) | 174 | `README.md`: `\| **AetherPlugson** \| Configura los plugins y opciones avanzadas del m` |
| Mensaje de registro interno (log), no visible en la interfaz | 114 | `Ventoy2Disk/Ventoy2Disk/PhyDrive.c`: `Log("Open ventoy efi file %p ", file);` |
| Nombre de archivo técnico conservado por compatibilidad (los lanzadores Aether* lo llaman) | 85 | `Ventoy2Disk/Ventoy2Disk/Language.h`: `#define VENTOY_CFG_INI     TEXT(".\\Ventoy2Disk.ini")` |
| Aviso de copyright, licencia o atribución obligatoria | 79 | `README.md`: `AetherBoot es una **distribución personalizada basada en la tecnología` |
| Script de compilación oficial (mensajes para desarrolladores) | 52 | `.github/workflows/ci.yml`: `name: Ventoy CI` |
| Licencia de un componente (información legal obligatoria) | 40 | `License/license-rufus.txt`: `Ventoy uses some function from Rufus, and all the Ventoy's code also f` |
| Texto de la rama en chino, inactiva (las interfaces se fijan en español) | 34 | `Ventoy2Disk/Ventoy2Disk/resource.h`: `// 供 Ventoy2Disk.rc 使用` |
| Etiqueta de partición que usan el motor y los scripts para localizar el USB | 13 | `Ventoy2Disk/Ventoy2Disk/DiskService_vds.c`: `hr = IVdsVolumeMF3_FormatEx2(pVolume, pFs, 0, VdsPara->ClusterSize, L"` |
| Sección del archivo de configuración INI (compatibilidad con instalaciones previas) | 10 | `Ventoy2Disk/Ventoy2Disk/WinDialog.c`: `value = GetPrivateProfileInt(TEXT("Ventoy"), TEXT("PartStyle"), 0, VEN` |
| URL en comentario o código interno, no visible | 10 | `LiveCDGUI/livecd.sh`: `xorriso -as mkisofs  -allow-lowercase  --sort-weight 0 / --sort-weight` |
| Tabla de mensajes en inglés inactiva (se usa la tabla en español) | 8 | `Plugson/src/main_windows.c`: `L"Failed to create ventoy directory!",` |
| Variable, comando o ruta de GRUB usada por el motor | 2 | `INSTALL/grub/grub.cfg`: `set vtoy_path=(ventoydisk)/ventoy` |
| Configuración del repositorio | 1 | `.gitignore`: `AetherBoot/Backup/ventoy-1.1.18-windows.zip` |

## Casos técnicos destacados

- **Etiqueta de partición `Ventoy` y partición `VTOYEFI`:** el GRUB firmado comprueba la etiqueta `VTOYEFI` (si no coincide muestra "[Unofficial …]"), y los sistemas en vivo buscan la partición de datos por la etiqueta `Ventoy`.
- **Carpeta `ventoy/` y `ventoy.json`:** el motor busca exactamente esas rutas; cambiarlas desactivaría todos los plugins.
- **GUID `VENTOY_GUID`:** estructura binaria compartida entre GRUB, vtoyjump y los instaladores para pasar parámetros al sistema arrancado.
- **Textos dentro de GRUB, shim y `ventoy.cpio`:** son binarios firmados para Secure Boot; recompilarlos sin la clave original rompería el arranque seguro.
- **Etiqueta de versión inyectada por GRUB** (contiene "www.ventoy.net" dentro del GUID): no se puede quitar sin recompilar; se coloca fuera de la pantalla con las variables `VTLE_LFT/TOP/CLR` y el tema muestra la versión de AetherBoot.
- **Avisos de copyright y licencias:** obligatorios por la GPLv3.
