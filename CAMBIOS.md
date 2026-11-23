# Ventoy 1.1.18: mejoras

Cambios sobre el código fuente oficial de Ventoy 1.1.18. Cada mejora está en su propio commit,
y el parche completo viene en `ventoy-1.1.18-mejoras.patch`.

## 1. Seguridad y errores

### 1.1 Servidores web locales vulnerables a DNS rebinding y CSRF (crítico)
**Archivos:** `Plugson/src/Web/ventoy_http.c`, `LinuxGUI/Ventoy2Disk/Web/ventoy_http.c`,
nuevo `ventoy_http_guard.h` (en ambas carpetas `Web/`).

La interfaz web de Linux (`VentoyWeb.sh`, puerto 24680) corre como **root** y puede instalar
Ventoy en cualquier disco, o sea, borrarlo. Plugson (puerto 24681) edita `ventoy.json`. Ninguno de
los dos revisaba los encabezados `Host` ni `Origin`, y el método `sysinfo` entrega el token de
sesión sin pedir autenticación. Una página web maliciosa abierta en el mismo equipo podía usar
*DNS rebinding* para leer el token y ordenar una instalación sobre un disco, o mandar peticiones
POST "a ciegas" para cambiar la configuración.

**Corrección:**
- Solo se aceptan peticiones con `Host` igual a `127.0.0.1`, `localhost`, `[::1]` o la IP
  configurada, siempre con el puerto correcto.
- Si llega un `Origin`, tiene que coincidir con esa misma dirección. Si no, se responde 403.
- `/vtoy/json` solo acepta POST.
- El cuerpo de la petición se lee con límite de tamaño (16 MB), comprobando `malloc()` y
  `mg_read()`. Antes, un `Content-Length` falso podía hacer que el servidor se cayera o escribiera
  en `buffer[-1]`.

**Probado:** compilé el servidor de Linux aquí y le hice estas peticiones:

| Petición | Resultado |
|---|---|
| Normal desde la propia interfaz | 200, funciona |
| `Host: evil.example` (rebinding) | **403 bad host** |
| `Origin: http://evil.example` (CSRF) | **403 bad origin** |
| `Origin: null` | **403** |
| `Content-Length: 99999999999` | se rechaza, el servidor sigue funcionando |

### 1.2 XSS por el nombre del dispositivo USB (WebUI de Linux)
**Archivos:** `LinuxGUI/Ventoy2Disk/Core/ventoy_disk.c`, `LinuxGUI/WebUI/index.html`, `LinuxGUI/WebUI/static/js/vtoy.js`

El fabricante y el modelo del USB (los dicta el propio dispositivo) se metían sin escapar en el
JSON y en el HTML de la página que corre como root. Una memoria con un "modelo" manipulado podía
romper el JSON o inyectar JavaScript. Ahora se limpian al leerlos (`ventoy_sanitize_disk_string`)
y se escapan en la interfaz (`vtoy_html_escape`).

### 1.3 Cadenas sin terminador nulo en el módulo GRUB de Ventoy
**Archivos:** `ventoy_json.c`, `ventoy_browser.c`, `ventoy_cmd.c`, `ventoy_linux.c` (en `GRUB2/MOD_SRC/.../ventoy/`)

`grub_strncpy(dst, src, sizeof(dst))` no añade `\0` cuando el texto ocupa todo el búfer. Esto
pasaba con nombres de archivo largos (modo navegador F2), valores de `ventoy.json` y rutas `.vlnk`.
Ahora se copia `tamaño - 1` y siempre se termina la cadena. También cubrí el caso de búfer de
tamaño 1, en el que `grub_strncpy(…, 0)` copiaría sin límite. El prefijo de los initrd de isolinux
quedó acotado.

### 1.4 Archivos .vlnk
**Archivos:** `Vlnk/src/main_linux.c`, `ventoy_cmd.c`

Una `.vlnk` es un archivo que cualquiera puede crear (el CRC no es una protección). La ruta que
contiene se concatenaba con `strcat` sin comprobar su longitud. Ahora se valida que esté terminada
en `\0` y que quepa. Comprobé que los `.vlnk` legítimos siguen funcionando, porque los generadores
oficiales ponen a cero toda la estructura.

### 1.5 Instalador de Windows
**Archivo:** `Ventoy2Disk/Ventoy2Disk/WinDialog.c` (UTF-16; se conservó la codificación)

Cambié `wsprintf`/`lstrcpy` sin límite por `swprintf_s`/`lstrcpyn` con el tamaño del búfer.

### 1.6 Script de Linux: discos con LVM, LUKS o RAID
**Archivo:** `INSTALL/tool/VentoyWorker.sh`

El script solo miraba `/proc/mounts`. Un disco con LVM, LUKS (por ejemplo, el sistema cifrado) o
RAID por software no aparece ahí con su nombre, así que podía acabar borrado. Ahora revisa
`/sys/class/block/*/holders` y se niega a continuar si el disco está en uso.

## 2. Interfaz del instalador

- **Windows:** las dos confirmaciones de "se borrarán todos los datos" (instalar y limpiar)
  ahora dicen **qué disco** se va a borrar, por ejemplo `>> Dispositivo: E: [32GB] SanDisk Cruzer`.
  Además, el botón por defecto es "No" en todas.
- **Español:** corregí errores en las traducciones ("Los archivo ISO", "a finalizado",
  "anomalía remoto", espacios sobrantes, mayúsculas de más) y traduje el mensaje de 4Kn que faltaba
  en español de España. Pasa el verificador del proyecto (`LANGUAGES/check.sh`).

## 3. Menú de arranque

- **Tema nuevo "moderno"** (`INSTALL/plugin/ventoy/theme_moderno/`): fondo oscuro, acento verde
  azulado, panel redondeado, barra de tiempo, buen contraste en las teclas de ayuda y **iconos
  por tipo** (ISO, WIM, EFI, IMG, VHD, carpeta, herramientas, apagado, sumas de verificación…).
  Todo el arte es original y se genera con `tools/make_theme.py`. Ver `preview_tema_moderno.png`
  (es una simulación: la fuente real de GRUB se ve algo distinta).
- Arreglé la entrada SHA-512 del menú de sumas, a la que le faltaba un espacio antes de `{`.

## 4. Funciones nuevas

- **`VerificarISOs.sh` (Linux) y `VerificarISOs.ps1` (Windows):** comprueban de una sola vez
  todas las imágenes de la USB contra su `.sha256`, `.sha1`, `.md5` o `.sha512`, o contra listas
  `SHA256SUMS`/`CHECKSUM` (formatos GNU, BSD o solo el hash). Indican OK, NO COINCIDE o SIN
  REFERENCIA, devuelven código 1 si hay imágenes dañadas, y con `-g`/`-Generar` crean el `.sha256`
  de las que no lo tienen. Ese `.sha256` también lo usa el menú de sumas de Ventoy para comparar
  automáticamente al arrancar. Se incluyen en los paquetes (`ventoy_pack.sh`).
- **Plantilla `windows11_es_desatendido.xml`:** Windows 10/11 en español sin requisitos de TPM,
  Secure Boot, RAM ni CPU, sin internet ni cuenta Microsoft, con cuenta local "Usuario". A
  propósito **no** elige ni formatea el disco.
- **`ventoy_ejemplo_es.json`:** ejemplo que activa el tema nuevo (con el clásico como alternativa
  en F5), el menú en español, alias, consejos y la plantilla de Windows.

## Cómo compilar

- **Partes Linux, GRUB y LiveCD:** el proyecto trae `.github/workflows/ci.yml`. Si sube este código
  a un repositorio suyo en GitHub, *Actions* lo compila con Docker y genera los zip de Windows y
  Linux.
- **`Ventoy2Disk.exe` y `VentoyPlugson.exe`:** esos ejecutables **no** se compilan en el CI; el
  autor los compila con Visual Studio (`Ventoy2Disk/Ventoy2Disk.sln`, `Plugson/vs/`). Para que los
  cambios de Windows (1.1, 1.5 y las confirmaciones con el nombre del disco) lleguen al `.exe`, hay
  que compilarlos en Windows con Visual Studio y copiarlos a `INSTALL/` antes de empaquetar.

## Qué no pude probar aquí

- El módulo GRUB y el `.exe` de Windows: aquí no hay compilador de Windows ni el árbol completo de
  GRUB. Revisé esos cambios a mano; son pequeños y localizados.
- `VerificarISOs.ps1` y el tema en arranque real. Sí probé `VerificarISOs.sh` con imágenes,
  formatos y una imagen dañada.
