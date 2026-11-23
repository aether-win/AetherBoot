# 7. Solución de problemas

| Síntoma | Causa probable | Qué hacer |
|---|---|---|
| El USB no aparece en el menú de arranque del equipo | Secure Boot o modo de arranque | Pruebe a desactivar Secure Boot o a cambiar entre UEFI y Legacy en la BIOS. |
| Aparece "Verification failed: (0x1A) Security Violation" | Secure Boot activado y clave sin registrar | Elija **Enroll key from disk** → `VTOYEFI` → `ENROLL_THIS_KEY_IN_MOKMANAGER.cer`, confirme y reinicie. Solo hay que hacerlo una vez por equipo. |
| "No se encontraron imágenes ISO o IMG compatibles" | Las imágenes no están en la primera partición, o están ocultas por un filtro | Cópielas a la partición de datos (`Aether`, o `Ventoy` en USB antiguos). Revise **Control global** → carpeta de búsqueda y filtros. |
| El menú se ve en blanco o el cursor va muy lento | Tarjeta gráfica o firmware poco compatible | Pulse **F7** (modo texto) o configure `display_mode: CLI` en Tema. |
| "Se detectó un error de sintaxis en ventoy.json" | JSON mal escrito | Corríjalo con AetherPlugson o restaure `/ventoy/ventoy_backup.json`. |
| Una imagen no arranca | Imagen dañada o modo no adecuado | Ejecute `VerificarISOs`. Pruebe los modos **w** (WIMBOOT), **r** (GRUB2) o **i** (compatible). |
| Windows 11 dice "Este equipo no cumple los requisitos" | TPM / Secure Boot / RAM | Use la plantilla `windows11_es_desatendido.xml` o la opción `VTOY_WIN11_BYPASS_CHECK` en Control global. |
| Aether2Disk no muestra el USB | Disco no reconocido como USB extraíble | **Opción → Mostrar todos los dispositivos** (con cuidado: verá también discos internos). |
| En Linux: "está en uso por dm-0 (LVM/LUKS/RAID)" | Ese disco tiene volúmenes activos | Compruebe que eligió el disco correcto. Si es el USB, desactive los volúmenes (`vgchange -an`, `cryptsetup close`). |

## Registros

- Windows: `log.txt` junto a `Aether2Disk.exe`, y `VentoyPlugson.log` / `VentoyVlnk.log` para las
  otras herramientas (nombres de archivo técnicos conservados).
- Linux: `log.txt` en la carpeta del paquete.
- Menú de arranque: **F5 → Utilidades → Información del hardware** y **Comprobar configuración**.
