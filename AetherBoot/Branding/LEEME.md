# Identidad gráfica de AetherBoot

Todos los archivos tienen **fondo transparente real** (canal alfa). Ninguno lleva fondo blanco ni negro.

| Carpeta | Contenido |
|---|---|
| `Logo/` | Logotipos maestros en alta resolución, recortados al contenido |
| `Logo/web/` | Logotipos escalados (320 a 1920 px de ancho) para interfaces y documentación |
| `Icons/ico/` | Iconos `.ico` de Windows con 10 tamaños (16 a 256 px) para los ejecutables y accesos directos |
| `Icons/png/` | Iconos PNG de 16 a 1024 px (alta densidad) |
| `Grub/` | Versiones para el menú de arranque GRUB2 (PNG RGBA de 8 bits sin entrelazado) |

## Archivos maestros

- `AetherBoot_logo.png`: logotipo principal, con el lema en español "GESTOR DE ARRANQUE MULTISISTEMA".
- `AetherBoot_logo_original_en.png`: el mismo logotipo con el lema original en inglés, como respaldo.
- `Aether2Disk_icono.png` y `Aether2Disk_logo_horizontal.png`
- `AetherPlugson_icono.png`
- `AetherVInk_icono.png`

## Qué se hizo a los originales

- **AetherBoot:** tenía una sombra negra pixelada alrededor. Se convirtió en transparencia sin tocar
  el cuerpo del logotipo.
- **Los demás:** se eliminó una neblina blanca semitransparente que se veía como "fondo" sobre
  colores oscuros. El brillo azul pegado al dibujo se conserva.
- Se recortó el espacio vacío sobrante. No se cambiaron colores, formas ni proporciones.

## Recomendaciones de uso

- Los textos blancos ("Aether") están pensados para **fondos oscuros**. Sobre blanco se pierden; es
  parte del diseño original.
- Por debajo de 32 px el texto de los iconos deja de leerse. Windows usa esos tamaños en la barra de
  tareas y en la esquina de las ventanas.
