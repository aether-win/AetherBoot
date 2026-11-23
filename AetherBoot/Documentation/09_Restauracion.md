# 9. Copia de seguridad y restauración

## Del código fuente

En `AetherBoot/Backup/` se guarda todo lo necesario para volver al original:

| Archivo | Para qué sirve |
|---|---|
| `aetherboot-vs-original.patch` | Todos los cambios de AetherBoot sobre Ventoy 1.1.18 (menos la carpeta nueva `AetherBoot/`), en formato `git diff --binary` |
| `restaurar.sh` | Revierte el parche, borra `AetherBoot/` y, si se le da el zip original, comprueba que el resultado sea idéntico |
| `SHA256SUMS_originales.txt` | Sumas SHA-256 de los paquetes originales con los que se trabajó |
| `ventoy-1.1.18-windows.zip` | Paquete original de Ventoy para Windows (para devolver un USB a Ventoy) |

```sh
bash AetherBoot/Backup/restaurar.sh /ruta/a/Ventoy-1_1_18.zip
```

El parche también sirve en sentido contrario: aplicado sobre el código original de Ventoy 1.1.18
(`git apply --binary aetherboot-vs-original.patch`) y copiando la carpeta `AetherBoot/`, reproduce
AetherBoot.

Si trabaja con el repositorio git del proyecto, el estado original está marcado con la etiqueta
`ventoy-1.1.18-original`, y cada fase de AetherBoot es un commit separado
(`git log --oneline`, `git diff ventoy-1.1.18-original -- ruta`).

## De un USB

- **Volver a Ventoy:** ejecute el `Ventoy2Disk.exe` original (incluido en
  `AetherBoot/Backup/ventoy-1.1.18-windows.zip`) y pulse **Actualizar**. Solo se reescribe la partición
  de arranque; las imágenes no se tocan.
- **Restaurar la configuración:** AetherPlugson guarda la versión anterior de `ventoy.json` en
  `/ventoy/ventoy_backup.json`. Para restaurarla, cópiela sobre `/ventoy/ventoy.json`.
- **Volver a la configuración por defecto:** borre `/ventoy/ventoy.json`.

## De la partición de arranque

`AetherBoot/Build/construir_imagen_arranque.sh` genera la partición de arranque AetherBoot a partir de
la imagen original, de forma reproducible. Si algo falla, la imagen original
(`ventoy/ventoy.disk.img.xz` del paquete de Ventoy) sigue intacta y se puede volver a instalar con
**Actualizar**.
