# 2. Instalación en USB

> ⚠️ **La instalación borra TODO el contenido del USB elegido.** Haga copia de sus datos antes. Desde
> AetherBoot, Aether2Disk muestra en cada confirmación **el nombre y el tamaño del disco** que se va a
> borrar. Compruébelo antes de aceptar.

## En Windows

1. Descomprima el paquete `aetherboot-<versión>-windows.zip` en una carpeta.
2. Conecte el USB y ejecute **`Aether2Disk.exe`**. Windows pedirá permisos de administrador.
3. Elija el USB en la lista **Dispositivo**.
4. (Opcional) En **Opción** puede elegir:
   - **Estilo de partición:** MBR (por defecto) o GPT. GPT es obligatorio en discos de más de 2 TB.
   - **Compatibilidad con Secure Boot:** activada por defecto.
   - **Configuración de partición:** reservar espacio al final del disco o cambiar el sistema de
     archivos (exFAT por defecto, también NTFS o FAT32).
5. Pulse **Instalar** y confirme las dos advertencias.

Para equipos ARM o para usar la versión de 64 bits, vea `FOR_X64_ARM.txt` (carpeta `altexe`).

### Comprobaciones automáticas de Aether2Disk

Antes de borrar el dispositivo, Aether2Disk revisa:

- **Disco de Windows:** si el disco elegido contiene Windows, la operación se bloquea.
- **Protección contra escritura:** si la memoria o el adaptador está bloqueado (interruptor *LOCK*),
  se avisa y no se continúa.
- **Disco que no es USB** (SATA, NVMe…, visible con *Mostrar todos los dispositivos*): se pide una
  confirmación extra porque podría ser un disco interno.

Durante la instalación se hace una **prueba rápida de capacidad real**: se escriben y se vuelven a
leer 32 bloques de 64 KB repartidos por todo el disco, incluido el final. Si alguno no vuelve igual,
es muy probable que la memoria sea falsa (anuncia más capacidad de la que tiene) o esté dañada, y
Aether2Disk pregunta si quiere continuar (la respuesta predeterminada es **No**). La prueba tarda
unos segundos en una memoria normal.

Al terminar, `log.txt` incluye un **resumen en español**: operación, dispositivo, resultado,
duración, resultado de la prueba de capacidad y velocidad de escritura medida. Si algo falla,
Aether2Disk ofrece abrir el registro.

### Restaurar la USB a memoria normal

**Opción → Restaurar USB (quitar AetherBoot)** borra AetherBoot y deja la memoria con una sola
partición formateada y con la etiqueta `USB`: FAT32 si mide 32 GB o menos (máxima compatibilidad
con televisores, radios y consolas) y exFAT si es más grande. En discos de más de 2 TB queda una
tabla GPT vacía que hay que formatear desde el Explorador de Windows.

## En Linux

```sh
tar xzf aetherboot-<versión>-linux.tar.gz
cd aetherboot-<versión>
sudo sh Aether2Disk.sh -i /dev/sdX        # instalar (X = su USB; el disco completo, no sdX1)
sudo sh Aether2Disk.sh -i -g /dev/sdX     # instalar con GPT
sudo bash AetherWeb.sh                    # interfaz web en http://127.0.0.1:24680
```

Antes de borrar nada, el script comprueba que el disco no esté montado ni en uso como swap, LVM,
LUKS o RAID. Si lo está, se detiene.

## Después de instalar

- Copie las imágenes a la partición **Aether** (la grande). Puede organizarlas en carpetas.
- Para comprobar que se copiaron bien, ejecute `VerificarISOs` (ver [capítulo 4](04_Menu_de_arranque.md)).
- El USB se puede seguir usando para guardar otros archivos.

## Instalación no destructiva (Windows)

**Opción → Instalación no destructiva** intenta instalar AetherBoot **sin borrar** los datos del USB,
reduciendo la primera partición. Solo funciona si el disco cumple ciertas condiciones (entre otras,
espacio libre suficiente al final de la primera partición). Si no se cumplen, Aether2Disk avisa y
cancela la operación; `log.txt` indica qué condición falló. Aun así, haga copia de seguridad antes.
