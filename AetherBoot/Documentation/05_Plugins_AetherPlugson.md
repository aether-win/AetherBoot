# 5. Plugins con AetherPlugson

Las opciones avanzadas de AetherBoot se guardan en **`/ventoy/ventoy.json`**, en la primera partición
del USB. AetherPlugson es el editor visual de ese archivo.

## Abrir AetherPlugson

- **Windows:** ejecute `AetherPlugson.exe`, elija el USB y pulse **Iniciar**. Se abre el navegador con
  la interfaz.
- **Linux:** `sudo bash AetherPlugson.sh /dev/sdX` y abra `http://127.0.0.1:24681`.

La interfaz solo atiende peticiones del propio equipo: rechaza páginas web de otros orígenes (ver la
nota de seguridad al final).

## Secciones

| Sección | Qué configura |
|---|---|
| Información del dispositivo | Datos del USB y de AetherBoot instalado |
| Control global | Idioma, carpeta de búsqueda, tiempo de espera, vista de lista/árbol, filtros… |
| Tema | Archivo `theme.txt`, resolución, modo gráfico o texto, fuentes |
| Alias de menú | Nombre que se muestra en lugar del nombre de archivo |
| Consejos de menú | Texto de ayuda al seleccionar una imagen |
| Clases de menú | Icono de cada imagen o carpeta |
| Instalación automática | Plantillas de instalación desatendida (p. ej. `windows11_es_desatendido.xml`) |
| Persistencia | Archivos `.dat` de persistencia para distribuciones Linux |
| Inyección de archivos | Archivos que se añaden a la imagen al arrancar |
| Reemplazo de config. | Reemplazar archivos de configuración dentro de la ISO |
| Contraseñas | Contraseña para el menú, para tipos de archivo o para imágenes concretas |
| Lista de imágenes | Lista blanca o lista negra de imágenes |
| Memdisk automático | Imágenes que se arrancan siempre en modo Memdisk |
| Controladores DUD | Discos de controladores para instaladores Linux |

Cada pestaña (`control`, `control_legacy`, `control_uefi`…) permite valores distintos según el
modo de arranque.

## Guardado y validación

- AetherPlugson comprueba las rutas y valores al escribirlos, y marca en rojo los que no existen.
- **Vista previa** (arriba a la derecha) muestra el `ventoy.json` resultante antes de que lo use.
- Al abrirse, guarda una copia del archivo anterior en `/ventoy/ventoy_backup.json`.
- Si `ventoy.json` tiene un error de sintaxis, AetherPlugson avisa y no lo sobrescribe.

## Ejemplo listo para usar

`plugin/ventoy/ventoy_ejemplo_es.json` activa el tema, el menú en español, un alias, consejos y la
plantilla de Windows 11 en español. Cópielo a `/ventoy/ventoy.json` y ajuste las rutas.

## Nota de seguridad

Los servidores locales de AetherPlugson y Aether2Disk (Linux) solo aceptan peticiones cuyo `Host` y
`Origin` sean el propio equipo (`127.0.0.1`, `localhost` o la IP configurada). Así, una página web
maliciosa no puede modificar la configuración ni ordenar una instalación (protección contra *DNS
rebinding* y CSRF).
