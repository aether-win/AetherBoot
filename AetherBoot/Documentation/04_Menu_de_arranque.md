# 4. Uso del menú de arranque

Al arrancar desde el USB aparece el menú de AetherBoot. Tiene el logotipo en la cabecera, la versión
y la plataforma (UEFI o BIOS) debajo, la lista de imágenes en un panel central y los atajos en la
barra inferior.

## Teclas

| Tecla | Función |
|---|---|
| ↑ ↓ / Enter | Moverse y arrancar la imagen seleccionada |
| **L** | Cambiar el idioma del menú (por defecto, español) |
| **F1** | Ayuda |
| **F2** | Explorar y arrancar archivos de otros discos del equipo |
| **F3** | Cambiar entre vista de lista y vista de árbol |
| **F4** | Arrancar el Windows/Linux instalado en el disco del equipo |
| **F5** | Utilidades: información del hardware, comprobar `ventoy.json`, selección de tema, resolución… |
| **F6** | Menú GRUB2 personalizado (`/ventoy/ventoy_grub.cfg`) |
| **F7** | Cambiar entre modo gráfico y modo texto |
| **m** | Calcular las sumas md5/sha1/sha256/sha512 de la imagen seleccionada |
| **d / w / r / i** | Arrancar en modo Memdisk / WIMBOOT / GRUB2 / compatible |

Si junto a la imagen hay un archivo `imagen.iso.sha256` (o `.sha1`, `.md5`, `.sha512`), el menú de
sumas (**m**) compara el resultado automáticamente.

## Comprobar las imágenes desde el equipo

Antes de arrancar conviene verificar que las imágenes no se dañaron al copiarlas:

- **Windows:** `powershell -ExecutionPolicy Bypass -File VerificarISOs.ps1`
- **Linux:** `sh VerificarISOs.sh /media/usuario/Ventoy`

Ambos buscan las sumas junto a cada imagen (`.sha256`, `SHA256SUMS`, etc.) e indican **OK**,
**NO COINCIDE** o **SIN REFERENCIA**. Con `-Generar` (Windows) o `-g` (Linux) crean el `.sha256` de las
imágenes que no lo tienen.

## Temas

- **aetherboot** (por defecto): el tema oficial, ya integrado en el USB.
- **moderno** (opcional): en `plugin/ventoy/theme_moderno` del paquete. Cópielo a `/ventoy/` en el USB
  y actívelo con AetherPlugson → Tema, o con el ejemplo `plugin/ventoy/ventoy_ejemplo_es.json`.

Con más de un tema configurado puede cambiar de uno a otro en **F5 → Selección de tema**.

## Qué hay detrás del tema

El menú lo dibuja GRUB2. Admite imágenes PNG con transparencia (el panel "de cristal" es real: se
mezcla con el fondo), pero **no** admite animaciones, desenfoque en vivo ni fuentes vectoriales. Por
eso el tema usa imágenes pre-renderizadas y posiciones relativas, que se adaptan a cualquier
resolución (por defecto 1024×768).
