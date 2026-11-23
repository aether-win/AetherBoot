/******************************************************************************
 * AetherChecks.c
 *
 * AetherBoot: comprobaciones de seguridad para Aether2Disk.
 *   - Comprobaciones antes de instalar (disco del sistema, disco no USB,
 *     protección contra escritura).
 *   - Prueba rápida de capacidad real (detecta USB falsas o dañadas).
 *   - Resumen legible de la operación al final de log.txt.
 *
 * Basado en Ventoy2Disk (Copyright (c) 2020-2025, longpanda <admin@ventoy.net>).
 * Este archivo forma parte de AetherBoot y se distribuye bajo la misma licencia
 * GPLv3 que el proyecto original.
 *
 * Nota: los mensajes que van a log.txt se escriben sin tildes porque la
 * función Log() es de 8 bits; los mensajes de la interfaz sí llevan tildes.
 ******************************************************************************/

#include <Windows.h>
#include <winioctl.h>
#include <stdio.h>
#include <shellapi.h>
#include <commctrl.h>
#include "resource.h"
#include "Language.h"
#include "Ventoy2Disk.h"

extern HWND g_DialogHwnd;

/* Resultado de la última prueba de capacidad (para el resumen). */
static int    g_AetherCapTested = 0;
static int    g_AetherCapBad = 0;
static int    g_AetherCapTotal = 0;
static double g_AetherCapWriteMBps = 0.0;
static double g_AetherCapReadMBps = 0.0;
static int    g_AetherCapUserAbort = 0;

static ULONGLONG g_AetherOpStart = 0;

#define AETHER_CAP_POINTS      32
#define AETHER_CAP_BLOCK       (64 * 1024)
#define AETHER_CAP_ALIGN       (1024 * 1024)

/* -------------------------------------------------------------------------- */
/* Utilidades                                                                 */
/* -------------------------------------------------------------------------- */

static const WCHAR *AetherBusName(STORAGE_BUS_TYPE Bus)
{
    switch (Bus)
    {
        case BusTypeUsb:     return L"USB";
        case BusTypeSata:    return L"SATA";
        case BusTypeAta:     return L"ATA";
        case BusTypeNvme:    return L"NVMe";
        case BusTypeScsi:    return L"SCSI";
        case BusTypeSas:     return L"SAS";
        case BusTypeRAID:    return L"RAID";
        case BusTypeSd:      return L"Tarjeta SD";
        case BusTypeMmc:     return L"MMC";
        case BusTypeVirtual: return L"Virtual";
        case BusTypeFileBackedVirtual: return L"Disco virtual (VHD)";
        case BusType1394:    return L"FireWire";
        default:             return L"desconocido";
    }
}

static BOOL AetherDiskHoldsWindows(int PhyDrive)
{
    int Letter;
    UINT Len;
    CHAR WinDir[MAX_PATH] = { 0 };

    /* Disco donde está instalado Windows */
    Len = GetWindowsDirectoryA(WinDir, sizeof(WinDir));
    if (Len >= 2 && WinDir[1] == ':')
    {
        Letter = WinDir[0];
        if (GetPhyDriveByLogicalDrive(Letter, NULL) == PhyDrive)
        {
            Log("[Aether] PhyDrive%d contiene la unidad de Windows %C:", PhyDrive, Letter);
            return TRUE;
        }
    }

    /* Disco donde está el sistema de arranque actual (por si Windows usa otra letra) */
    Len = GetSystemDirectoryA(WinDir, sizeof(WinDir));
    if (Len >= 2 && WinDir[1] == ':')
    {
        Letter = WinDir[0];
        if (GetPhyDriveByLogicalDrive(Letter, NULL) == PhyDrive)
        {
            Log("[Aether] PhyDrive%d contiene el directorio del sistema %C:", PhyDrive, Letter);
            return TRUE;
        }
    }

    return FALSE;
}

static int AetherIsWriteProtected(int PhyDrive)
{
    BOOL bRet;
    DWORD dwSize = 0;
    DWORD Err;
    HANDLE hDrive;
    CHAR Path[64];

    sprintf_s(Path, sizeof(Path), "\\\\.\\PhysicalDrive%d", PhyDrive);
    hDrive = CreateFileA(Path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if (hDrive == INVALID_HANDLE_VALUE)
    {
        Log("[Aether] No se pudo abrir %s para revisar proteccion (%u)", Path, GetLastError());
        return -1;
    }

    bRet = DeviceIoControl(hDrive, IOCTL_DISK_IS_WRITABLE, NULL, 0, NULL, 0, &dwSize, NULL);
    Err = GetLastError();
    CloseHandle(hDrive);

    if (bRet)
    {
        return 0;
    }

    Log("[Aether] IOCTL_DISK_IS_WRITABLE fallo, error %u", Err);
    return (Err == ERROR_WRITE_PROTECT) ? 1 : -1;
}

/* -------------------------------------------------------------------------- */
/* 1) Comprobaciones antes de instalar (hilo de la interfaz)                  */
/* -------------------------------------------------------------------------- */

BOOL AetherPreInstallChecks(HWND hWnd, PHY_DRIVE_INFO *pPhyDrive)
{
    int WP;
    WCHAR Msg[1024];

    if (!pPhyDrive)
    {
        return FALSE;
    }

    Log("[Aether] Comprobaciones previas PhyDrive%d bus:%d removable:%d size:%llu",
        pPhyDrive->PhyDrive, pPhyDrive->BusType, pPhyDrive->RemovableMedia,
        (ULONGLONG)pPhyDrive->SizeInBytes);

    /* a) Nunca tocar el disco donde está Windows. */
    if (AetherDiskHoldsWindows(pPhyDrive->PhyDrive))
    {
        MessageBoxW(hWnd,
            L"El dispositivo seleccionado es el disco donde está instalado Windows.\r\n\r\n"
            L"Aether2Disk no permite instalar AetherBoot en él porque borraría tu sistema.\r\n"
            L"Selecciona una memoria USB.",
            L"Operación bloqueada", MB_OK | MB_ICONERROR);
        return FALSE;
    }

    /* b) Protección contra escritura (interruptor físico o controlador bloqueado). */
    WP = AetherIsWriteProtected(pPhyDrive->PhyDrive);
    if (WP == 1)
    {
        MessageBoxW(hWnd,
            L"El dispositivo está protegido contra escritura.\r\n\r\n"
            L"Si la memoria o el adaptador de tarjeta tiene un interruptor de bloqueo (LOCK), "
            L"muévelo a la posición desbloqueada y vuelve a conectarlo.\r\n"
            L"Si no tiene interruptor, la memoria puede haber pasado a modo de solo lectura por "
            L"desgaste y ya no se puede escribir en ella.",
            L"Dispositivo protegido", MB_OK | MB_ICONERROR);
        return FALSE;
    }

    /* c) Disco que no está conectado por USB: posible disco interno. */
    if (pPhyDrive->BusType != BusTypeUsb)
    {
        swprintf_s(Msg, ARRAYSIZE(Msg),
            L"ATENCIÓN: este dispositivo no está conectado por USB (tipo de conexión: %s).\r\n\r\n"
            L"Podría ser un disco interno de tu computadora. Si continúas, TODO su contenido "
            L"se borrará.\r\n\r\n"
            L"¿Estás completamente seguro de que quieres usar este disco?",
            AetherBusName(pPhyDrive->BusType));

        if (MessageBoxW(hWnd, Msg, L"Disco no USB", MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) != IDYES)
        {
            Log("[Aether] Usuario cancelo: disco no USB");
            return FALSE;
        }
        Log("[Aether] Usuario confirmo disco no USB");
    }

    return TRUE;
}

/* -------------------------------------------------------------------------- */
/* 2) Prueba rápida de capacidad real (hilo de trabajo)                       */
/* -------------------------------------------------------------------------- */

static UINT64 AetherRand(UINT64 *s)
{
    /* xorshift64* */
    UINT64 x = *s;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    *s = x;
    return x * 2685821657736338717ULL;
}

static void AetherFillBlock(BYTE *Buf, UINT64 Seed, UINT64 Offset, int Index)
{
    DWORD i;
    UINT64 State = Seed ^ (Offset * 0x9E3779B97F4A7C15ULL) ^ (UINT64)(Index + 1);
    UINT64 *p = (UINT64 *)Buf;

    if (State == 0)
    {
        State = 0xA37E5B00751DEULL;
    }

    for (i = 0; i < AETHER_CAP_BLOCK / sizeof(UINT64); i++)
    {
        p[i] = AetherRand(&State);
    }

    /* Cabecera reconocible en cada sector de 512 bytes */
    for (i = 0; i < AETHER_CAP_BLOCK; i += 512)
    {
        memcpy(Buf + i, "AETHRCAP", 8);
        *(UINT64 *)(Buf + i + 8) = Offset + i;
        *(UINT64 *)(Buf + i + 16) = Seed;
    }
}

static BOOL AetherSeek(HANDLE hDrive, UINT64 Offset)
{
    LARGE_INTEGER Pos, NewPos;

    Pos.QuadPart = (LONGLONG)Offset;
    NewPos.QuadPart = 0;
    return SetFilePointerEx(hDrive, Pos, &NewPos, FILE_BEGIN) && NewPos.QuadPart == Pos.QuadPart;
}

/*
 * Escribe bloques de prueba distintos repartidos por todo el disco (incluido
 * el final) y después los lee todos. Una USB con capacidad falsa "envuelve"
 * las direcciones altas sobre las bajas, así que algunos bloques no vuelven
 * iguales. El disco ya fue confirmado para borrado completo por el usuario.
 *
 * Retorno: 0 = correcto, 1 = bloques incorrectos, -1 = no se pudo hacer la prueba.
 */
int AetherCapacityTest(HANDLE hDrive, UINT64 SizeInBytes)
{
    int i;
    int Bad = 0;
    DWORD dwSize;
    BOOL bRet;
    UINT64 Seed;
    UINT64 Span;
    UINT64 Offsets[AETHER_CAP_POINTS];
    BYTE *WBuf = NULL;
    BYTE *RBuf = NULL;
    LARGE_INTEGER Freq, T0, T1, T2;

    g_AetherCapTested = 0;
    g_AetherCapBad = 0;
    g_AetherCapTotal = 0;
    g_AetherCapWriteMBps = 0.0;
    g_AetherCapReadMBps = 0.0;

    if (SizeInBytes < (UINT64)64 * 1024 * 1024)
    {
        Log("[Aether] Prueba de capacidad omitida: disco demasiado pequeno");
        return -1;
    }

    /* Puntos de prueba: desde 2 MB hasta el último MB alineado del disco */
    Span = ((SizeInBytes - AETHER_CAP_BLOCK) & ~((UINT64)AETHER_CAP_ALIGN - 1)) - (2 * AETHER_CAP_ALIGN);
    for (i = 0; i < AETHER_CAP_POINTS; i++)
    {
        Offsets[i] = (2 * (UINT64)AETHER_CAP_ALIGN) + ((Span / (AETHER_CAP_POINTS - 1)) * i);
        Offsets[i] &= ~((UINT64)AETHER_CAP_ALIGN - 1);
    }
    Offsets[AETHER_CAP_POINTS - 1] = (SizeInBytes - AETHER_CAP_BLOCK) & ~((UINT64)AETHER_CAP_ALIGN - 1);

    WBuf = (BYTE *)VirtualAlloc(NULL, AETHER_CAP_BLOCK, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    RBuf = (BYTE *)VirtualAlloc(NULL, AETHER_CAP_BLOCK, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!WBuf || !RBuf)
    {
        Log("[Aether] Prueba de capacidad: sin memoria");
        if (WBuf) VirtualFree(WBuf, 0, MEM_RELEASE);
        if (RBuf) VirtualFree(RBuf, 0, MEM_RELEASE);
        return -1;
    }

    Seed = GetTickCount64() ^ ((UINT64)GetCurrentProcessId() << 32) ^ (UINT64)(ULONG_PTR)WBuf;
    QueryPerformanceFrequency(&Freq);
    QueryPerformanceCounter(&T0);

    Log("[Aether] Prueba de capacidad real: %d bloques de %u KB en %llu bytes",
        AETHER_CAP_POINTS, AETHER_CAP_BLOCK / 1024, (ULONGLONG)SizeInBytes);

    /* Fase 1: escribir todos los bloques */
    for (i = 0; i < AETHER_CAP_POINTS; i++)
    {
        AetherFillBlock(WBuf, Seed, Offsets[i], i);
        if (!AetherSeek(hDrive, Offsets[i]))
        {
            Log("[Aether] Seek de escritura fallo en %llu (%u)", (ULONGLONG)Offsets[i], GetLastError());
            Bad++;
            continue;
        }

        dwSize = 0;
        bRet = WriteFile(hDrive, WBuf, AETHER_CAP_BLOCK, &dwSize, NULL);
        if (!bRet || dwSize != AETHER_CAP_BLOCK)
        {
            Log("[Aether] Escritura fallo en %llu ret:%d size:%u err:%u", (ULONGLONG)Offsets[i], bRet, dwSize, GetLastError());
            Bad++;
        }
    }

    FlushFileBuffers(hDrive);
    QueryPerformanceCounter(&T1);

    /* Fase 2: leer y comparar todos los bloques */
    for (i = 0; i < AETHER_CAP_POINTS; i++)
    {
        AetherFillBlock(WBuf, Seed, Offsets[i], i);
        memset(RBuf, 0, AETHER_CAP_BLOCK);

        if (!AetherSeek(hDrive, Offsets[i]))
        {
            Log("[Aether] Seek de lectura fallo en %llu (%u)", (ULONGLONG)Offsets[i], GetLastError());
            Bad++;
            continue;
        }

        dwSize = 0;
        bRet = ReadFile(hDrive, RBuf, AETHER_CAP_BLOCK, &dwSize, NULL);
        if (!bRet || dwSize != AETHER_CAP_BLOCK)
        {
            Log("[Aether] Lectura fallo en %llu ret:%d size:%u err:%u", (ULONGLONG)Offsets[i], bRet, dwSize, GetLastError());
            Bad++;
            continue;
        }

        if (memcmp(WBuf, RBuf, AETHER_CAP_BLOCK) != 0)
        {
            Log("[Aether] Bloque %d en %llu (%llu MB) NO coincide", i, (ULONGLONG)Offsets[i], (ULONGLONG)(Offsets[i] >> 20));
            Bad++;
        }
    }

    QueryPerformanceCounter(&T2);

    if (Freq.QuadPart > 0)
    {
        double MB = (double)AETHER_CAP_POINTS * AETHER_CAP_BLOCK / (1024.0 * 1024.0);
        double tw = (double)(T1.QuadPart - T0.QuadPart) / (double)Freq.QuadPart;
        double tr = (double)(T2.QuadPart - T1.QuadPart) / (double)Freq.QuadPart;
        g_AetherCapWriteMBps = (tw > 0.0) ? MB / tw : 0.0;
        g_AetherCapReadMBps = (tr > 0.0) ? MB / tr : 0.0;
    }

    VirtualFree(WBuf, 0, MEM_RELEASE);
    VirtualFree(RBuf, 0, MEM_RELEASE);

    g_AetherCapTested = 1;
    g_AetherCapBad = Bad;
    g_AetherCapTotal = AETHER_CAP_POINTS;

    Log("[Aether] Prueba de capacidad: %d de %d bloques incorrectos. Escritura aprox %.2f MB/s, lectura aprox %.2f MB/s",
        Bad, AETHER_CAP_POINTS, g_AetherCapWriteMBps, g_AetherCapReadMBps);

    return (Bad > 0) ? 1 : 0;
}

/*
 * Llamada desde el hilo de instalación cuando la prueba falla.
 * Devuelve TRUE si el usuario decide continuar de todas formas.
 */
BOOL AetherAskContinueAfterCapacityFail(void)
{
    WCHAR Msg[1024];

    if (g_CLI_Mode)
    {
        Log("[Aether] Modo CLI: se detiene la instalacion por prueba de capacidad fallida");
        g_AetherCapUserAbort = 1;
        return FALSE;
    }

    swprintf_s(Msg, ARRAYSIZE(Msg),
        L"La prueba de capacidad real encontró %d de %d bloques que NO se guardaron correctamente.\r\n\r\n"
        L"Es muy probable que esta memoria USB sea falsa (anuncia más capacidad de la que tiene) "
        L"o que esté dañada. Si la usas, los archivos que copies podrían corromperse sin aviso.\r\n\r\n"
        L"Recomendación: no la uses para AetherBoot ni para guardar datos importantes. "
        L"Puedes confirmarlo con H2testw o ValiDrive.\r\n\r\n"
        L"¿Quieres instalar AetherBoot de todas formas?",
        g_AetherCapBad, g_AetherCapTotal);

    if (MessageBoxW(g_DialogHwnd, Msg, L"Posible USB falsa o dañada", MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) == IDYES)
    {
        Log("[Aether] Usuario decidio continuar pese a la prueba de capacidad fallida");
        return TRUE;
    }

    Log("[Aether] Usuario cancelo la instalacion por prueba de capacidad fallida");
    g_AetherCapUserAbort = 1;
    return FALSE;
}

BOOL AetherCapacityUserAborted(void)
{
    return g_AetherCapUserAbort ? TRUE : FALSE;
}

/* -------------------------------------------------------------------------- */
/* 3) Resumen de la operación en log.txt                                      */
/* -------------------------------------------------------------------------- */

void AetherOpBegin(void)
{
    g_AetherOpStart = GetTickCount64();
    g_AetherCapTested = 0;
    g_AetherCapBad = 0;
    g_AetherCapTotal = 0;
    g_AetherCapWriteMBps = 0.0;
    g_AetherCapReadMBps = 0.0;
    g_AetherCapUserAbort = 0;
}

void AetherOpSummary(const char *Operation, const PHY_DRIVE_INFO *pPhyDrive, int rc)
{
    ULONGLONG Secs = 0;

    if (g_AetherOpStart)
    {
        Secs = (GetTickCount64() - g_AetherOpStart) / 1000;
    }

    Log("");
    Log("==================== RESUMEN AETHERBOOT ====================");
    Log("Operacion   : %s", Operation);
    if (pPhyDrive)
    {
        Log("Dispositivo : PhyDrive%d  %s %s  (%d GB)  conexion:%s",
            pPhyDrive->PhyDrive, pPhyDrive->VendorId, pPhyDrive->ProductId,
            GetHumanReadableGBSize(pPhyDrive->SizeInBytes),
            pPhyDrive->BusType == BusTypeUsb ? "USB" : "NO USB");
    }
    Log("Resultado   : %s", rc == 0 ? "CORRECTO" : "FALLO");
    Log("Duracion    : %llu min %llu s", Secs / 60, Secs % 60);

    if (g_AetherCapTested)
    {
        Log("Capacidad   : %s (%d de %d bloques incorrectos)",
            g_AetherCapBad ? "SOSPECHOSA - posible USB falsa o danada" : "correcta",
            g_AetherCapBad, g_AetherCapTotal);
        Log("Velocidad   : escritura aprox %.2f MB/s, lectura aprox %.2f MB/s",
            g_AetherCapWriteMBps, g_AetherCapReadMBps);
        if (g_AetherCapWriteMBps > 0.0 && g_AetherCapWriteMBps < 1.0)
        {
            Log("Nota        : la escritura es muy lenta (menos de 1 MB/s); la causa es la memoria USB.");
        }
    }

    if (rc != 0)
    {
        if (g_AetherCapUserAbort)
        {
            Log("Causa       : cancelado por el usuario tras la prueba de capacidad.");
        }
        else
        {
            Log("Causa       : revisa las lineas anteriores que empiezan con 'Failed', 'failed' o 'error'.");
            Log("Sugerencia  : desconecta y vuelve a conectar la USB, cierra programas que la usen");
            Log("              (Explorador, antivirus) y vuelve a intentarlo. Si sigue fallando,");
            Log("              prueba con otro puerto u otra memoria.");
        }
    }
    Log("============================================================");
    Log("");
}

/* Pregunta si se quiere abrir log.txt después de un error. */
void AetherOfferOpenLog(HWND hWnd)
{
    CHAR Path[MAX_PATH] = { 0 };

    if (g_CLI_Mode)
    {
        return;
    }

    if (MessageBoxW(hWnd,
        L"¿Quieres abrir el registro (log.txt) para ver el detalle del error?\r\n"
        L"Al final del archivo hay un resumen en español.",
        L"Registro", MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES)
    {
        if (GetFullPathNameA(VENTOY_FILE_LOG, sizeof(Path), Path, NULL) > 0)
        {
            ShellExecuteA(hWnd, "open", "notepad.exe", Path, NULL, SW_SHOWNORMAL);
        }
    }
}
