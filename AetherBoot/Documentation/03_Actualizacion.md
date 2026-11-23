# 3. Actualización

La actualización **solo reescribe la partición de arranque** (`VTOYEFI`). Sus imágenes y archivos de la
primera partición no se tocan.

## Windows

1. Ejecute la versión nueva de `Aether2Disk.exe`.
2. Elija el USB. En **AetherBoot en el dispositivo** verá la versión instalada.
3. Pulse **Actualizar** y confirme.

## Linux

```sh
sudo sh Aether2Disk.sh -u /dev/sdX
```

## Actualizar desde Ventoy

Un USB preparado con Ventoy 1.x se puede actualizar a AetherBoot con **Actualizar**, sin perder las
imágenes. Su `ventoy.json` sigue funcionando igual: el formato de configuración es el mismo.

Para volver a Ventoy, actualice el USB con el instalador original de Ventoy (ver
[capítulo 9](09_Restauracion.md)).
