/*
 * AetherFUSboot: funciones propias del derivado de Rufus para el ecosistema AetherBoot
 *
 *   - AetherApplyWindowStyle(): barra de título azul noche, borde cian y esquinas
 *     redondeadas (Windows 11), igual que Aether2Disk, AetherPlugson y AetherVInk.
 *   - AetherQuickCapacityTest(): prueba rápida de capacidad real antes de escribir
 *     (detecta memorias USB falsas que anuncian más capacidad de la que tienen).
 *   - AetherVerifyImageWrite(): vuelve a leer la unidad después de escribir una
 *     imagen en modo DD y la compara con el archivo original.
 *
 * Basado en Rufus: The Reliable USB Formatting Utility
 * Copyright © 2011-2025 Pete Batard <pete@akeo.ie>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifdef _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "rufus.h"
#include "missing.h"
#include "resource.h"
#include "msapi_utf8.h"
#include "localization.h"

extern DWORD ErrorStatus;

/* -------------------------------------------------------------------------- */
/* Estilo de ventana                                                          */
/* -------------------------------------------------------------------------- */

typedef HRESULT (WINAPI *PF_AetherDwmSetWindowAttribute)(HWND, DWORD, LPCVOID, DWORD);

void AetherApplyWindowStyle(HWND hWnd)
{
	static PF_AetherDwmSetWindowAttribute pfSetAttr = NULL;
	static BOOL loaded = FALSE;
	DWORD corner = 2;                       /* DWMWCP_ROUND */
	COLORREF caption = RGB(6, 14, 36);      /* azul profundo (#060E24), igual que la cabecera */
	COLORREF text = RGB(255, 255, 255);     /* blanco */
	COLORREF border = RGB(34, 211, 238);    /* cian (#22D3EE) */

	if (!loaded) {
		HMODULE hDwm = LoadLibraryW(L"dwmapi.dll");
		loaded = TRUE;
		if (hDwm != NULL)
			pfSetAttr = (PF_AetherDwmSetWindowAttribute)(void*)GetProcAddress(hDwm, "DwmSetWindowAttribute");
	}

	/* En Windows 10 o anterior estos atributos no existen y la llamada no hace nada */
	if ((pfSetAttr != NULL) && (hWnd != NULL)) {
		pfSetAttr(hWnd, 33 /* DWMWA_WINDOW_CORNER_PREFERENCE */, &corner, sizeof(corner));
		pfSetAttr(hWnd, 34 /* DWMWA_BORDER_COLOR */, &border, sizeof(border));
		pfSetAttr(hWnd, 35 /* DWMWA_CAPTION_COLOR */, &caption, sizeof(caption));
		pfSetAttr(hWnd, 36 /* DWMWA_TEXT_COLOR */, &text, sizeof(text));
	}
}

/* -------------------------------------------------------------------------- */
/* Prueba rápida de capacidad real                                            */
/* -------------------------------------------------------------------------- */

#define AETHER_CAP_POINTS   32
#define AETHER_CAP_BLOCK    (64 * 1024)
#define AETHER_CAP_ALIGN    (1024 * 1024)

static uint64_t AetherRand(uint64_t* s)
{
	/* xorshift64* */
	uint64_t x = *s;
	x ^= x >> 12;
	x ^= x << 25;
	x ^= x >> 27;
	*s = x;
	return x * 2685821657736338717ULL;
}

static void AetherFillBlock(uint8_t* buf, uint64_t seed, uint64_t offset, int index)
{
	size_t i;
	uint64_t state = seed ^ (offset * 0x9E3779B97F4A7C15ULL) ^ (uint64_t)(index + 1);
	uint64_t* p = (uint64_t*)buf;

	if (state == 0)
		state = 0xA37E5B00751DEULL;
	for (i = 0; i < AETHER_CAP_BLOCK / sizeof(uint64_t); i++)
		p[i] = AetherRand(&state);
	/* Cabecera reconocible en cada sector de 512 bytes */
	for (i = 0; i < AETHER_CAP_BLOCK; i += 512) {
		memcpy(buf + i, "AETHRCAP", 8);
		*(uint64_t*)(buf + i + 8) = offset + i;
		*(uint64_t*)(buf + i + 16) = seed;
	}
}

static BOOL AetherSeek(HANDLE h, uint64_t offset)
{
	LARGE_INTEGER pos, new_pos;
	pos.QuadPart = (LONGLONG)offset;
	new_pos.QuadPart = 0;
	return SetFilePointerEx(h, pos, &new_pos, FILE_BEGIN) && (new_pos.QuadPart == pos.QuadPart);
}

/*
 * Escribe 32 bloques distintos de 64 KB repartidos por todo el disco (incluido
 * el final) y después los lee todos. Una memoria con capacidad falsa "envuelve"
 * las direcciones altas sobre las bajas, así que algunos bloques no vuelven
 * iguales. Solo se llama cuando el usuario ya aceptó borrar la unidad.
 *
 * Devuelve el número de bloques incorrectos (0 = correcto) o -1 si la prueba no
 * se pudo hacer. En *write_mbps se devuelve la velocidad de escritura medida.
 */
int AetherQuickCapacityTest(HANDLE hDrive, uint64_t disk_size, DWORD sector_size, double* write_mbps)
{
	int i, bad = 0;
	DWORD size;
	BOOL r;
	uint64_t seed, span, offsets[AETHER_CAP_POINTS];
	uint8_t *wbuf = NULL, *rbuf = NULL;
	LARGE_INTEGER freq, t0, t1;

	if (write_mbps != NULL)
		*write_mbps = 0.0;
	if ((sector_size == 0) || (AETHER_CAP_BLOCK % sector_size != 0) || (disk_size < 64 * MB)) {
		uprintf("Prueba de capacidad omitida (sector %lu, tamaño %llu)", sector_size, disk_size);
		return -1;
	}

	span = ((disk_size - AETHER_CAP_BLOCK) & ~((uint64_t)AETHER_CAP_ALIGN - 1)) - 2 * (uint64_t)AETHER_CAP_ALIGN;
	for (i = 0; i < AETHER_CAP_POINTS; i++) {
		offsets[i] = 2 * (uint64_t)AETHER_CAP_ALIGN + (span / (AETHER_CAP_POINTS - 1)) * i;
		offsets[i] &= ~((uint64_t)AETHER_CAP_ALIGN - 1);
	}
	offsets[AETHER_CAP_POINTS - 1] = (disk_size - AETHER_CAP_BLOCK) & ~((uint64_t)AETHER_CAP_ALIGN - 1);

	wbuf = (uint8_t*)_mm_malloc(AETHER_CAP_BLOCK, 4096);
	rbuf = (uint8_t*)_mm_malloc(AETHER_CAP_BLOCK, 4096);
	if ((wbuf == NULL) || (rbuf == NULL)) {
		bad = -1;
		goto out;
	}

	seed = GetTickCount64() ^ ((uint64_t)GetCurrentProcessId() << 32) ^ (uint64_t)(uintptr_t)wbuf;
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&t0);
	uprintf("Prueba rápida de capacidad real: %d bloques de %d KB", AETHER_CAP_POINTS, AETHER_CAP_BLOCK / 1024);

	for (i = 0; i < AETHER_CAP_POINTS; i++) {
		if (IS_ERROR(ErrorStatus) && (SCODE_CODE(ErrorStatus) == ERROR_CANCELLED)) {
			bad = -1;
			goto out;
		}
		AetherFillBlock(wbuf, seed, offsets[i], i);
		if (!AetherSeek(hDrive, offsets[i])) {
			uprintf("  No se pudo posicionar en %llu: %s", offsets[i], WindowsErrorString());
			bad++;
			continue;
		}
		r = WriteFile(hDrive, wbuf, AETHER_CAP_BLOCK, &size, NULL);
		if (!r || (size != AETHER_CAP_BLOCK)) {
			uprintf("  Error de escritura en %llu: %s", offsets[i], WindowsErrorString());
			bad++;
		}
	}
	FlushFileBuffers(hDrive);
	QueryPerformanceCounter(&t1);

	for (i = 0; i < AETHER_CAP_POINTS; i++) {
		AetherFillBlock(wbuf, seed, offsets[i], i);
		memset(rbuf, 0, AETHER_CAP_BLOCK);
		if (!AetherSeek(hDrive, offsets[i])) {
			bad++;
			continue;
		}
		r = ReadFile(hDrive, rbuf, AETHER_CAP_BLOCK, &size, NULL);
		if (!r || (size != AETHER_CAP_BLOCK)) {
			uprintf("  Error de lectura en %llu: %s", offsets[i], WindowsErrorString());
			bad++;
			continue;
		}
		if (memcmp(wbuf, rbuf, AETHER_CAP_BLOCK) != 0) {
			uprintf("  El bloque %d (%llu MB) NO coincide", i, offsets[i] / MB);
			bad++;
		}
	}

	if ((write_mbps != NULL) && (freq.QuadPart > 0) && (t1.QuadPart > t0.QuadPart)) {
		double secs = (double)(t1.QuadPart - t0.QuadPart) / (double)freq.QuadPart;
		*write_mbps = ((double)AETHER_CAP_POINTS * AETHER_CAP_BLOCK / (double)MB) / secs;
	}
	uprintf("Prueba de capacidad: %d de %d bloques incorrectos%s", bad, AETHER_CAP_POINTS,
		(bad == 0) ? " (correcta)" : " (posible USB falsa o dañada)");

out:
	safe_mm_free(wbuf);
	safe_mm_free(rbuf);
	/* Volver al principio, como espera el resto del proceso */
	AetherSeek(hDrive, 0);
	return bad;
}

/* -------------------------------------------------------------------------- */
/* Verificación de imágenes escritas en modo DD                               */
/* -------------------------------------------------------------------------- */

/*
 * Lee 'image_size' bytes de la imagen y de la unidad y los compara.
 * Devuelve TRUE si coinciden. Si no, establece ErrorStatus.
 */
BOOL AetherVerifyImageWrite(HANDLE hDrive, const char* image_file, uint64_t image_size, DWORD sector_size, DWORD buf_size)
{
	BOOL ret = FALSE, r;
	HANDLE hImage = INVALID_HANDLE_VALUE;
	uint8_t *ibuf = NULL, *dbuf = NULL;
	uint64_t done = 0, first_bad = UINT64_MAX;
	DWORD to_read, to_read_drive, got_i, got_d;

	if ((sector_size == 0) || (buf_size == 0) || (buf_size % sector_size != 0) || (image_file == NULL))
		return TRUE;	/* nada que verificar */

	hImage = CreateFileU(image_file, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
		FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if (hImage == INVALID_HANDLE_VALUE) {
		uprintf("Verificación: no se pudo abrir la imagen: %s", WindowsErrorString());
		ErrorStatus = RUFUS_ERROR(ERROR_OPEN_FAILED);
		return FALSE;
	}
	ibuf = (uint8_t*)_mm_malloc(buf_size, sector_size);
	dbuf = (uint8_t*)_mm_malloc(buf_size, sector_size);
	if ((ibuf == NULL) || (dbuf == NULL)) {
		ErrorStatus = RUFUS_ERROR(ERROR_NOT_ENOUGH_MEMORY);
		goto out;
	}
	if (!AetherSeek(hDrive, 0)) {
		uprintf("Verificación: no se pudo volver al inicio de la unidad: %s", WindowsErrorString());
		ErrorStatus = RUFUS_ERROR(ERROR_SEEK);
		goto out;
	}

	uprintf("Verificando lo escrito (%llu bytes):", image_size);
	UpdateProgressWithInfoInit(NULL, FALSE);
	uprint_progress(0, 0);
	while (done < image_size) {
		if (IS_ERROR(ErrorStatus) && (SCODE_CODE(ErrorStatus) == ERROR_CANCELLED))
			goto out;
		UpdateProgressWithInfo(OP_FORMAT, MSG_390, done, image_size);
		uprint_progress(done, image_size);

		to_read = (DWORD)MIN((uint64_t)buf_size, image_size - done);
		to_read_drive = ((to_read + sector_size - 1) / sector_size) * sector_size;

		r = ReadFile(hImage, ibuf, to_read, &got_i, NULL);
		if (!r || (got_i != to_read)) {
			uprintf("\r\nVerificación: error al leer la imagen: %s", WindowsErrorString());
			ErrorStatus = RUFUS_ERROR(ERROR_READ_FAULT);
			goto out;
		}
		r = ReadFile(hDrive, dbuf, to_read_drive, &got_d, NULL);
		if (!r || (got_d != to_read_drive)) {
			uprintf("\r\nVerificación: error al leer la unidad en %llu: %s", done, WindowsErrorString());
			ErrorStatus = RUFUS_ERROR(ERROR_READ_FAULT);
			goto out;
		}
		if (memcmp(ibuf, dbuf, to_read) != 0) {
			DWORD k;
			for (k = 0; (k < to_read) && (ibuf[k] == dbuf[k]); k++);
			first_bad = done + k;
			break;
		}
		done += to_read;
	}
	uprintfs("\r\n");

	if (first_bad != UINT64_MAX) {
		uprintf("Verificación FALLIDA: los datos de la unidad no coinciden con la imagen a partir del byte %llu (%llu MB).",
			first_bad, first_bad / MB);
		uprintf("La memoria puede estar dañada o ser falsa. Prueba con otra unidad.");
		ErrorStatus = RUFUS_ERROR(ERROR_CRC);
		goto out;
	}
	uprintf("Verificación correcta: la unidad contiene exactamente la imagen.");
	ret = TRUE;

out:
	safe_mm_free(ibuf);
	safe_mm_free(dbuf);
	if (hImage != INVALID_HANDLE_VALUE)
		CloseHandle(hImage);
	return ret;
}
