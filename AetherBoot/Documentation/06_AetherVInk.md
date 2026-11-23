# 6. AetherVInk

AetherVInk crea **enlaces `.vlnk`**: archivos pequeños que apuntan a una imagen guardada en **otro
disco** del equipo (por ejemplo, una ISO de 6 GB en el disco duro). Al copiar el `.vlnk` al USB, la
imagen aparece en el menú de AetherBoot y arranca desde su ubicación real, sin copiarla al USB.

## Crear un enlace (Windows)

1. Ejecute `AetherVInk.exe`.
2. Pulse **Crear** y elija la imagen (`.iso`, `.img`, `.wim`, `.vhd`, `.vhdx`, `.efi`, `.dat` o `.vtoy`).
3. Indique dónde guardar el enlace. El nombre termina en `.vlnk.iso`, `.vlnk.img`, etc.
4. Copie el `.vlnk.*` al USB.

**Analizar** muestra a qué archivo apunta un `.vlnk` y si ese archivo sigue existiendo.

## Línea de órdenes

```
AetherVInk.exe -i C:\ISO\grande.iso -o D:\enlaces\grande.vlnk.iso
```

`-q` desactiva el registro. En Linux, la misma función está en `VentoyVlnk.sh` del paquete Linux.

## Limitaciones

- En Windows, la imagen debe estar en una partición NTFS, exFAT, FAT, FAT32 o UDF (es lo que comprueba AetherVInk).
- El enlace guarda la firma del disco y la posición de la partición: si reformatea o clona ese disco,
  hay que volver a crear el enlace.
- Desde AetherBoot, el menú comprueba que la ruta del enlace sea válida. Un `.vlnk` dañado o
  manipulado se rechaza en lugar de arrancar.
