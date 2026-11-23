<p align="center">
  <img src="AetherBoot/Branding/Logo/web/AetherBoot_logo_960.png" alt="AetherBoot" width="640">
</p>

# AetherBoot: gestor de arranque multisistema

AetherBoot convierte una memoria USB en un disco de arranque para muchos sistemas a la vez. Se instala
una sola vez y después basta con **copiar los archivos ISO, WIM, IMG, VHD(x) o EFI** al USB: al
arrancar el equipo desde la memoria aparece el menú de AetherBoot con todas las imágenes para elegir.
No hace falta volver a formatear cada vez.

| Aplicación | Para qué sirve |
|---|---|
| **AetherBoot** | Menú de arranque del USB (UEFI y BIOS heredado) |
| **Aether2Disk** | Instala, actualiza o limpia AetherBoot en una memoria USB (Windows y Linux) |
| **AetherPlugson** | Configura los plugins y opciones avanzadas del menú (`ventoy.json`) |
| **AetherVInk** | Crea y analiza enlaces `.vlnk` a imágenes guardadas en otros discos |

## Documentación

Toda la documentación está en español, en [`AetherBoot/Documentation`](AetherBoot/Documentation):

1. [Introducción](AetherBoot/Documentation/01_Introduccion.md)
2. [Instalación en USB](AetherBoot/Documentation/02_Instalacion.md)
3. [Actualización](AetherBoot/Documentation/03_Actualizacion.md)
4. [Uso del menú de arranque](AetherBoot/Documentation/04_Menu_de_arranque.md)
5. [Plugins con AetherPlugson](AetherBoot/Documentation/05_Plugins_AetherPlugson.md)
6. [AetherVInk](AetherBoot/Documentation/06_AetherVInk.md)
7. [Solución de problemas](AetherBoot/Documentation/07_Solucion_de_problemas.md)
8. [Compatibilidad](AetherBoot/Documentation/08_Compatibilidad.md)
9. [Copia de seguridad y restauración](AetherBoot/Documentation/09_Restauracion.md)
10. [Licencias y componentes de terceros](AetherBoot/Documentation/10_Licencias.md)
11. [Compilación](AetherBoot/Documentation/11_Compilacion.md)

## Procedencia y licencia

AetherBoot es una **distribución personalizada basada en la tecnología Ventoy** (© longpanda), con su
propia identidad visual, interfaz en español y mejoras de seguridad. No es un gestor de arranque
escrito desde cero: conserva el motor original y su compatibilidad. Se distribuye bajo la
**GNU GPL versión 3 o posterior** (ver [`COPYING`](COPYING)); los componentes de terceros conservan sus
licencias (ver [`License/`](License)). El README original del proyecto se conserva en
[`AetherBoot/Documentation/original`](AetherBoot/Documentation/original).
