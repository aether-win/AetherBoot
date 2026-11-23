# AetherFUSboot

AetherFUSboot es el creador de memorias USB de arranque del ecosistema
**AetherBoot**. Es un derivado de **Rufus 4.16** de Pete Batard
(<https://rufus.ie>, <https://github.com/pbatard/rufus>) y se distribuye bajo la
misma licencia, **GNU GPL versión 3 o posterior** (ver `LICENSE.txt`).

AetherFUSboot no es la versión oficial de Rufus ni está respaldado por su autor.
Se conservan todos los avisos de copyright y las atribuciones de Rufus y de los
componentes que incluye (ms-sys, FreeDOS, libcdio, wimlib, syslinux, GRUB, etc.).

## Qué cambia respecto a Rufus 4.16

| Área | Rufus 4.16 | AetherFUSboot |
|---|---|---|
| Nombre e icono | Rufus | AetherFUSboot, icono propio del proyecto AetherBoot |
| Idioma | Según Windows | Español por defecto (se puede cambiar desde el menú de idioma) |
| Etiqueta de la USB | Etiqueta actual de la unidad o su tamaño ("8GB") | **AetherFUS** (en FAT32 se guarda como `AETHERFUS`) |
| Etiqueta con ISO | Etiqueta de la ISO | Igual para Linux y otras ISO (algunas distribuciones la necesitan para arrancar). Para ISO de Windows se usa **AetherFUS** |
| Aspecto | Claro o según Windows | Tema oscuro azul AetherBoot por defecto, barra de título azul noche y borde cian en Windows 11 (el tema claro sigue disponible) |
| Prueba de capacidad | Solo dentro de la búsqueda completa de bloques dañados (lenta, opcional) | **Prueba rápida automática** antes de escribir: 32 bloques de 64 KB repartidos por toda la unidad. Si la USB parece falsa o dañada, avisa y pregunta si continuar |
| Imágenes en modo DD | Se escriben sin comprobar | **Verificación automática**: se vuelve a leer la unidad y se compara byte a byte con la imagen (imágenes sin comprimir) |
| Actualizaciones | Consulta el servidor de Rufus y ofrece descargar Rufus | No consulta versiones de Rufus. Sigue actualizando la lista de revocación de Secure Boot (DBX) y la descarga de ISO de Windows si se permite la conexión |
| Aviso de "versión no oficial" | Se muestra en compilaciones de terceros | No se muestra: AetherFUSboot tiene nombre propio y no se hace pasar por Rufus |
| Configuración | `HKCU\Software\Akeo Consulting\Rufus` o `rufus.ini` | `HKCU\Software\AetherBoot\AetherFUSboot` o `aetherfusboot.ini` (no se mezcla con un Rufus instalado) |

Lo que **no** cambia: los métodos de escritura y de arranque, el soporte de
Windows To Go, la omisión de requisitos de Windows 11, la persistencia de Linux,
UEFI:NTFS, las comprobaciones de Secure Boot y la compatibilidad con archivos
VTSI de AetherBoot/Ventoy.

## Recursos que se siguen descargando de servidores de Rufus

Algunas funciones descargan componentes técnicos que Rufus publica en
`https://rufus.ie/files` (versiones de GRUB/Syslinux que no vienen incluidas, el
script Fido para descargar ISO de Windows, la lista DBX). Se mantienen porque
son necesarios para que esas funciones sigan funcionando igual que en Rufus.

## Archivos modificados

- `src/rufus.h`: nombre de la aplicación, empresa del registro, etiqueta por defecto.
- `src/rufus.c`: idioma español por defecto, etiqueta AetherFUS, archivo `aetherfusboot.ini`, estilo de ventana, sin aviso de versión no oficial.
- `src/format.c`: prueba rápida de capacidad, verificación tras escribir en modo DD, etiqueta de respaldo.
- `src/aether.c` (nuevo): estilo de ventana, prueba de capacidad y verificación.
- `src/net.c`: sin consulta de versiones de Rufus.
- `src/darkmode.c`, `src/ui.h`: tema oscuro y colores AetherBoot.
- `src/license.h`, `src/rufus.rc`: textos de "Acerca de", recursos de versión e icono.
- `res/loc/rufus.loc`: nombre del programa en los textos, mensajes nuevos (MSG_390–MSG_393) y ajustes del español.
- `res/aetherfusboot.ico` (nuevo): icono.

## Compilar

Visual Studio 2022 o 2026 con las herramientas de C++ para escritorio:

```
msbuild rufus.sln /p:Configuration=Release /p:Platform=x64
```

El ejecutable queda en `x64\Release\rufus.exe`; la compilación automática de
GitHub (`.github/workflows/aetherboot.yml`, trabajo *aetherfusboot*) lo renombra a
`AetherFUSboot.exe`.

## Estado de las pruebas

El código se compila en GitHub Actions. Las funciones nuevas (prueba de
capacidad y verificación DD) todavía no se han probado con memorias USB reales.
