/******************************************************************************
 * aetherfat.c — Herramienta para leer y modificar la imagen FAT de la
 * partición de arranque (VTOYEFI) sin necesidad de montarla.
 *
 * Uso:
 *   aetherfat <imagen> ls     [ruta]          lista recursivamente
 *   aetherfat <imagen> get    <ruta> <local>  extrae un archivo
 *   aetherfat <imagen> dump   <carpeta>       extrae todo el contenido
 *   aetherfat <imagen> put    <local> <ruta>  copia (o reemplaza) un archivo
 *   aetherfat <imagen> rm     <ruta>          borra un archivo
 *   aetherfat <imagen> mkdir  <ruta>          crea una carpeta
 *   aetherfat <imagen> free                   espacio libre (bytes)
 *
 * Usa fat_io_lib (incluida en el código de Ventoy).
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3 of the
 * License, or (at your option) any later version.
 ******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#include "fat_filelib.h"

/* fat_io_lib in Ventoy logs through this hook */
void ventoy_syslog_printf(const char *fmt, ...) { (void)fmt; }

static int g_fd = -1;

static int img_read(uint32 sector, uint8 *buffer, uint32 count)
{
    ssize_t len = (ssize_t)count * 512;
    return pread(g_fd, buffer, len, (off_t)sector * 512) == len;
}

static int img_write(uint32 sector, uint8 *buffer, uint32 count)
{
    ssize_t len = (ssize_t)count * 512;
    return pwrite(g_fd, buffer, len, (off_t)sector * 512) == len;
}

static int list_dir(const char *path, int dump, const char *outdir)
{
    FL_DIR dir;
    fl_dirent ent;
    char child[1300];
    char local[2600];

    if (!fl_opendir(path, &dir))
    {
        fprintf(stderr, "no se puede abrir %s\n", path);
        return 1;
    }

    while (fl_readdir(&dir, &ent) == 0)
    {
        if (strcmp(ent.filename, ".") == 0 || strcmp(ent.filename, "..") == 0)
        {
            continue;
        }
        snprintf(child, sizeof(child), "%s%s%s", path, (path[strlen(path) - 1] == '/') ? "" : "/", ent.filename);

        if (dump)
        {
            snprintf(local, sizeof(local), "%s%s", outdir, child);
            if (ent.is_dir)
            {
                mkdir(local, 0755);
            }
            else
            {
                void *f = fl_fopen(child, "rb");
                FILE *o = fopen(local, "wb");
                char buf[65536];
                int n;
                if (!f || !o)
                {
                    fprintf(stderr, "error al extraer %s\n", child);
                    if (f) fl_fclose(f);
                    if (o) fclose(o);
                    fl_closedir(&dir);
                    return 1;
                }
                while ((n = fl_fread(buf, 1, sizeof(buf), f)) > 0)
                {
                    fwrite(buf, 1, n, o);
                }
                fl_fclose(f);
                fclose(o);
            }
        }
        else
        {
            printf("%s %10u %s\n", ent.is_dir ? "d" : "-", ent.size, child);
        }
    }
    fl_closedir(&dir);

    /* recursion after closing: fat_io_lib directory handles are limited */
    if (!fl_opendir(path, &dir))
    {
        return 1;
    }
    {
        /* propio de cada nivel de recursión (no static) */
        char (*subs)[1024] = malloc(512 * 1024);
        int nsub = 0, i, rc2 = 0;
        if (!subs)
        {
            fl_closedir(&dir);
            return 1;
        }
        while (fl_readdir(&dir, &ent) == 0)
        {
            if (ent.is_dir && strcmp(ent.filename, ".") && strcmp(ent.filename, "..") && nsub < 512)
            {
                snprintf(subs[nsub++], 1024, "%s%s%s", path, (path[strlen(path) - 1] == '/') ? "" : "/", ent.filename);
            }
        }
        fl_closedir(&dir);
        for (i = 0; i < nsub && rc2 == 0; i++)
        {
            rc2 = list_dir(subs[i], dump, outdir);
        }
        free(subs);
        if (rc2)
        {
            return 1;
        }
    }
    return 0;
}

static long long free_bytes(void)
{
    /* FAT16: read the BPB ourselves and count free FAT entries */
    uint8 bpb[512], sec[512];
    uint32 spc, rsvd, nfats, rootents, totsec, fatsz, datasec, clusters, i, freec = 0;

    img_read(0, bpb, 1);
    spc      = bpb[13];
    rsvd     = bpb[14] | (bpb[15] << 8);
    nfats    = bpb[16];
    rootents = bpb[17] | (bpb[18] << 8);
    totsec   = bpb[19] | (bpb[20] << 8);
    if (totsec == 0)
    {
        totsec = bpb[32] | (bpb[33] << 8) | (bpb[34] << 16) | ((uint32)bpb[35] << 24);
    }
    fatsz    = bpb[22] | (bpb[23] << 8);
    if (fatsz == 0 || spc == 0)
    {
        return -1; /* not FAT12/16 */
    }
    datasec  = totsec - (rsvd + nfats * fatsz + (rootents * 32 + 511) / 512);
    clusters = datasec / spc;

    for (i = 2; i < clusters + 2; i++)
    {
        uint32 off = i * 2;
        if (off % 512 == 0 || i == 2)
        {
            img_read(rsvd + off / 512, sec, 1);
        }
        if (sec[off % 512] == 0 && sec[off % 512 + 1] == 0)
        {
            freec++;
        }
    }
    return (long long)freec * spc * 512;
}

int main(int argc, char **argv)
{
    int rc = 0;
    const char *cmd;

    if (argc < 3)
    {
        fprintf(stderr, "uso: %s <imagen> ls|get|dump|put|rm|mkdir|free ...\n", argv[0]);
        return 2;
    }

    cmd = argv[2];
    g_fd = open(argv[1], (strcmp(cmd, "put") == 0 || strcmp(cmd, "rm") == 0 || strcmp(cmd, "mkdir") == 0) ? O_RDWR : O_RDONLY);
    if (g_fd < 0)
    {
        perror(argv[1]);
        return 2;
    }

    fl_init();
    if (fl_attach_media(img_read, img_write) != FAT_INIT_OK)
    {
        fprintf(stderr, "no es una imagen FAT válida\n");
        return 2;
    }

    if (strcmp(cmd, "ls") == 0)
    {
        rc = list_dir(argc > 3 ? argv[3] : "/", 0, NULL);
    }
    else if (strcmp(cmd, "dump") == 0 && argc > 3)
    {
        mkdir(argv[3], 0755);
        rc = list_dir("/", 1, argv[3]);
    }
    else if (strcmp(cmd, "get") == 0 && argc > 4)
    {
        void *f = fl_fopen(argv[3], "rb");
        FILE *o = fopen(argv[4], "wb");
        char buf[65536];
        int n;
        if (!f || !o)
        {
            fprintf(stderr, "no se puede leer %s\n", argv[3]);
            return 1;
        }
        while ((n = fl_fread(buf, 1, sizeof(buf), f)) > 0)
        {
            fwrite(buf, 1, n, o);
        }
        fl_fclose(f);
        fclose(o);
    }
    else if (strcmp(cmd, "put") == 0 && argc > 4)
    {
        FILE *in = fopen(argv[3], "rb");
        void *f;
        char buf[65536];
        size_t n;
        if (!in)
        {
            perror(argv[3]);
            return 1;
        }
        fl_remove(argv[4]);
        f = fl_fopen(argv[4], "wb");
        if (!f)
        {
            fprintf(stderr, "no se puede crear %s (¿existe la carpeta?)\n", argv[4]);
            return 1;
        }
        while ((n = fread(buf, 1, sizeof(buf), in)) > 0)
        {
            if (fl_fwrite(buf, 1, (int)n, f) != (int)n)
            {
                fprintf(stderr, "sin espacio al escribir %s\n", argv[4]);
                rc = 1;
                break;
            }
        }
        fl_fclose(f);
        fclose(in);
    }
    else if (strcmp(cmd, "rm") == 0 && argc > 3)
    {
        rc = fl_remove(argv[3]) == 0 ? 0 : 1;
    }
    else if (strcmp(cmd, "mkdir") == 0 && argc > 3)
    {
        if (!fl_is_dir(argv[3]))
        {
            rc = fl_createdirectory(argv[3]) ? 0 : 1;
        }
    }
    else if (strcmp(cmd, "free") == 0)
    {
        printf("%lld\n", free_bytes());
    }
    else
    {
        fprintf(stderr, "orden no válida\n");
        rc = 2;
    }

    fl_shutdown();
    close(g_fd);
    return rc;
}
