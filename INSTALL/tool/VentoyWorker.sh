#!/bin/sh

. ./tool/ventoy_lib.sh

print_usage() {

    echo 'Uso:  Aether2Disk.sh ORDEN [ OPCIÓN ] /dev/sdX'
    echo '  ORDEN:'
    echo '   -i  instala AetherBoot en sdX (falla si el disco ya lo tiene)'
    echo '   -I  fuerza la instalación de AetherBoot en sdX (lo tenga o no)'
    echo '   -u  actualiza AetherBoot en sdX'
    echo '   -l  muestra la información de AetherBoot en sdX'
    echo ''
    echo '  OPCIÓN: (opcional)'
    echo '   -r MiB      reserva espacio (MiB) al final del disco (solo al instalar)'
    echo '   -s/-S       activa/desactiva Secure Boot (por defecto activado)'
    echo '   -g          usa particiones GPT; por defecto MBR (solo al instalar)'
    echo '   -L          etiqueta de la 1.ª partición exFAT (por defecto Aether)'
    echo '   -n          intenta una instalación no destructiva (solo al instalar)'
    echo ''
}


SECUREBOOT="YES"
VTNEW_LABEL='Aether'
RESERVE_SIZE_MB=0
while [ -n "$1" ]; do
    if [ "$1" = "-i" ]; then
        MODE="install"
    elif [ "$1" = "-I" ]; then
        MODE="install"
        FORCE="Y"
    elif [ "$1" = "-n" ]; then
        NONDESTRUCTIVE="Y"
    elif [ "$1" = "-u" ]; then
        MODE="update"
    elif [ "$1" = "-l" ]; then
        MODE="list"
    elif [ "$1" = "-s" ]; then
        SECUREBOOT="YES"
    elif [ "$1" = "-S" ]; then
        SECUREBOOT="NO"
    elif [ "$1" = "-g" ]; then
        VTGPT="YES"
    elif [ "$1" = "-L" ]; then
        shift
        VTNEW_LABEL=$1
    elif [ "$1" = "-r" ]; then
        RESERVE_SPACE="YES"
        shift
        RESERVE_SIZE_MB=$1
    elif [ "$1" = "-V" ] || [ "$1" = "--version" ]; then
        exit 0
    elif [ "$1" = "-h" ] || [ "$1" = "--help" ]; then
        print_usage
        exit 0
    else
        if ! [ -b "$1" ]; then
            vterr "$1 NO es un dispositivo válido"
            print_usage
            exit 1
        fi
        DISK=$1
        # Resolve symlinks now, will be needed to look up information about the device in
        # the /sys/ filesystem, for example /sys/class/block/${DISK#/dev/}/start
        # The main use case is supporting /dev/disk/by-id/ symlinks instead of raw devices
        if [ -L "$DISK" ]; then
            DISK=$(readlink -e -n "$DISK")
        fi
    fi

    shift
done

if [ -z "$MODE" ]; then
    print_usage
    exit 1
fi

if ! [ -b "$DISK" ]; then
    vterr "El disco $DISK no existe"
    exit 1
fi

if [ -e /sys/class/block/${DISK#/dev/}/start ]; then
    vterr  "$DISK es una partición; indique el disco completo."
    echo   "Por ejemplo:"
    vterr  "    sudo sh Aether2Disk.sh -i /dev/sdb1 <=== Incorrecto"
    vtinfo "    sudo sh Aether2Disk.sh -i /dev/sdb  <=== Correcto"
    echo ""
    exit 1
fi

if [ -n "$RESERVE_SPACE" -a "$MODE" = "install" ]; then
    if echo $RESERVE_SIZE_MB | grep -q '^[0-9][0-9]*$'; then
        vtdebug "User will reserve $RESERVE_SIZE_MB MiB disk space"
    else
        vterr "$RESERVE_SIZE_MB no es un espacio reservado válido"
        exit 1
    fi
fi

vtdebug "MODE=$MODE FORCE=$FORCE RESERVE_SPACE=$RESERVE_SPACE RESERVE_SIZE_MB=$RESERVE_SIZE_MB"

#check tools
if check_tool_work_ok; then
    vtdebug "check tool work ok"
else
    vterr "Algunas herramientas no funcionan en este sistema. Revise log.txt para más detalles."
    exit 1
fi

if [ "$MODE" = "list" ]; then
    version=$(get_disk_ventoy_version $DISK)
    if [ $? -eq 0 ]; then
        echo "Versión de AetherBoot en el disco: $version"

        vtPart1Type=$(dd if=$DISK bs=1 count=1 skip=450 status=none | hexdump -n1 -e  '1/1 "%02X"')
        if [ "$vtPart1Type" = "EE" ]; then
            echo "Estilo de partición   : GPT"
        else
            echo "Estilo de partición   : MBR"
        fi

        if check_disk_secure_boot $DISK; then
            echo "Secure Boot           : SÍ"
        else
            echo "Secure Boot           : NO"
        fi
    else
        echo "Versión de AetherBoot: no instalada"
    fi
    echo ""
    exit 0
fi

#check mountpoint
check_umount_disk "$DISK"

if grep "$DISK" /proc/mounts; then
    vterr "$DISK está montado; desmóntelo primero."
    exit 1
fi

#check swap partition
if swapon --help 2>&1 | grep -q '^ -s,'; then
    if swapon -s | grep -q "^${DISK}[0-9]"; then
        vterr "$DISK se usa como swap; desactívelo primero (swapoff)."
        exit 1
    fi
fi

#check device-mapper users (LVM / LUKS / software RAID). Their partitions are
#not listed in /proc/mounts under $DISK, e.g. an encrypted system root.
vtDiskName=${DISK#/dev/}
for vtholder in /sys/class/block/$vtDiskName/holders/* /sys/class/block/$vtDiskName/$vtDiskName*/holders/*; do
    if [ -e "$vtholder" ]; then
        vterr "$DISK está en uso por $(basename $vtholder) (LVM/LUKS/RAID); libérelo primero."
        exit 1
    fi
done

#check access
if dd if="$DISK" of=/dev/null bs=1 count=1 >/dev/null 2>&1; then
    vtdebug "root permission check ok ..."
else
    vterr "No se pudo acceder a $DISK; puede que necesite permisos de root."
    echo ''
    exit 1
fi


#check tmp_mnt directory
if [ -d ./tmp_mnt ]; then
    vtdebug "There is a tmp_mnt directory, now delete it."
    umount ./tmp_mnt >/dev/null 2>&1
    rm -rf ./tmp_mnt
    if [ -d ./tmp_mnt ]; then
        vterr "Ya existe la carpeta tmp_mnt; bórrela primero."
        exit 1
    fi
fi


if [ "$MODE" = "install" -a -z "$NONDESTRUCTIVE" ]; then
    vtdebug "install Ventoy ..."

    if [ -n "$VTGPT" ]; then
        if parted -v > /dev/null 2>&1; then
            PARTTOOL='parted'
        else
            vterr "No se encontró parted; AetherBoot no puede crear particiones sin él."
            vterr "Instale primero \"GNU parted\"."
            exit 1
        fi
    else
        if parted -v > /dev/null 2>&1; then
            PARTTOOL='parted'            
        elif command -v fdisk > /dev/null 2>&1; then
            PARTTOOL='fdisk'            
        else
            vterr "No se encontraron parted ni fdisk; AetherBoot no puede crear particiones."
            exit 1
        fi
    fi
    
    if [ "$PARTTOOL" = "parted" ]; then
        if parted -s $DISK p 2>&1 | grep -i -q 'sector size.*4096.*4096'; then
            vterr "Por ahora AetherBoot no es compatible con dispositivos 4K nativos."
            exit 1
        fi
    else
        if fdisk -l $DISK | grep -i -q 'sector size.*4096.*4096'; then
            vterr "Por ahora AetherBoot no es compatible con dispositivos 4K nativos."
            exit 1
        fi
    fi
    

    version=$(get_disk_ventoy_version $DISK)
    if [ $? -eq 0 ]; then
        if [ -z "$FORCE" ]; then
            vtwarn "$DISK ya contiene AetherBoot versión $version"
            vtwarn "Use la opción -u para una actualización segura."
            vtwarn "O, si de verdad quiere reinstalar AetherBoot en $DISK, use la opción -I."
            vtwarn ""
            exit 1
        fi
    fi

    disk_sector_num=$(cat /sys/block/${DISK#/dev/}/size)
    disk_size_gb=$(expr $disk_sector_num / 2097152)

    if [ $disk_sector_num -gt 4294967296 ] && [ -z "$VTGPT" ]; then
        vterr "$DISK supera los 2 TB; MBR no funcionará en él."
        exit 1
    fi

    if [ -n "$RESERVE_SPACE" ]; then
        sum_size_mb=$(expr $RESERVE_SIZE_MB + $VENTOY_PART_SIZE_MB)
        reserve_sector_num=$(expr $sum_size_mb \* 2048)

        if [ $disk_sector_num -le $reserve_sector_num ]; then
            vterr "No se pueden reservar $RESERVE_SIZE_MB MiB en $DISK"
            exit 1
        fi
    fi

    #Print disk info
    echo "Disco  : $DISK"
    parted -s $DISK p 2>&1 | grep Model
    echo "Tamaño : $disk_size_gb GiB"
    if [ -n "$VTGPT" ]; then
        echo "Estilo : GPT"
    else
        echo "Estilo : MBR"
    fi
    echo ''

    if [ -n "$RESERVE_SPACE" ]; then
        echo "Se reservarán $RESERVE_SIZE_MB MiB del disco "
    fi
    echo ''

    vtwarn "Atención:"
    vtwarn "Va a instalar AetherBoot en $DISK."
    vtwarn "¡¡¡Se perderán todos los datos del disco $DISK!!!"
    echo ""

    read -p '¿Continuar? (y/n) '  Answer
    if [ "$Answer" != "y" ]; then
        if [ "$Answer" != "Y" ]; then
            exit 0
        fi
    fi

    echo ""
    vtwarn "¡¡¡Se perderán todos los datos del disco $DISK!!!"
    read -p 'Segunda confirmación. ¿Continuar? (y/n) '  Answer
    if [ "$Answer" != "y" ]; then
        if [ "$Answer" != "Y" ]; then
            exit 0
        fi
    fi

    if [ $disk_sector_num -le $VENTOY_SECTOR_NUM ]; then
        vterr "No hay espacio suficiente en el disco $DISK"
        exit 1
    fi

    # check and umount
    check_umount_disk "$DISK"

    if ! dd if=/dev/zero of=$DISK bs=64 count=512 status=none conv=fsync; then
        vterr "No se pudo escribir en $DISK; compruebe si está en uso."
        exit 1
    fi

    if [ -n "$VTGPT" ]; then
        vtdebug "format_ventoy_disk_gpt $RESERVE_SIZE_MB $DISK $PARTTOOL ..."
        format_ventoy_disk_gpt $RESERVE_SIZE_MB $DISK $PARTTOOL
    else
        vtdebug "format_ventoy_disk_mbr $RESERVE_SIZE_MB $DISK $PARTTOOL ..."
        format_ventoy_disk_mbr $RESERVE_SIZE_MB $DISK $PARTTOOL
    fi

    # format part1

    # DiskSize > 32GB  Cluster Size use 128KB
    # DiskSize < 32GB  Cluster Size use 32KB
    if [ $disk_size_gb -gt 32 ]; then
        cluster_sectors=256
    else
        cluster_sectors=64
    fi

    PART1=$(get_disk_part_name $DISK 1)
    PART2=$(get_disk_part_name $DISK 2)

    #clean part2
    dd status=none conv=fsync if=/dev/zero of=$DISK bs=512 count=32 seek=$part2_start_sector

    #format part1
    wait_and_create_part ${PART1} ${PART2}    
    if [ -b ${PART1} ]; then
        vtinfo "Formateando la partición 1 ${PART1} ..."
        mkexfatfs -n "$VTNEW_LABEL" -s $cluster_sectors ${PART1}
        if [ $? -ne 0 ]; then
            echo "mkexfatfs falló; reintentando..."
            mkexfatfs -n "$VTNEW_LABEL" -s $cluster_sectors ${PART1}
            if [ $? -ne 0 ]; then
                echo "######### mkexfatfs falló; saliendo ########"
                exit 1
            fi
        else
            echo "mkexfatfs correcto"
        fi        
    else
        vterr "${PART1} NO existe"
    fi

    vtinfo "escribiendo datos en el disco ..."
    dd status=none conv=fsync if=./boot/boot.img of=$DISK bs=1 count=446

    if [ -n "$VTGPT" ]; then
        echo -en '\x22' | dd status=none of=$DISK conv=fsync bs=1 count=1 seek=92
        xzcat ./boot/core.img.xz | dd status=none conv=fsync of=$DISK bs=512 count=2014 seek=34
        echo -en '\x23' | dd of=$DISK conv=fsync bs=1 count=1 seek=17908 status=none
    else
        xzcat ./boot/core.img.xz | dd status=none conv=fsync of=$DISK bs=512 count=2047 seek=1
    fi
    
    # check and umount
    check_umount_disk "$DISK"

    xzcat ./ventoy/ventoy.disk.img.xz | dd status=none conv=fsync of=$DISK bs=512 count=$VENTOY_SECTOR_NUM seek=$part2_start_sector

    #test UUID
    testUUIDStr=$(vtoy_gen_uuid | hexdump -C)
    vtdebug "test uuid: $testUUIDStr"

    #disk uuid
    vtoy_gen_uuid | dd status=none conv=fsync of=${DISK} seek=384 bs=1 count=16

    #disk signature
    vtoy_gen_uuid | dd status=none conv=fsync of=${DISK} skip=12 seek=440 bs=1 count=4

    vtinfo "sincronizando datos ..."
    sync

    vtinfo "procesando la partición ESP ..."

    if [ "$SECUREBOOT" != "YES" ]; then 
        sleep 2
        check_umount_disk "$DISK"  
        vtoycli partresize -s $DISK $part2_start_sector
    fi

    echo ""
    vtinfo "AetherBoot se instaló correctamente en $DISK."
    echo ""

elif [ "$MODE" = "install" -a -n "$NONDESTRUCTIVE" ]; then
    vtdebug "non-destructive install Ventoy ..."

    version=$(get_disk_ventoy_version $DISK)
    if [ $? -eq 0 ]; then
        if [ -z "$FORCE" ]; then
            vtwarn "$DISK ya contiene AetherBoot versión $version."
            vtwarn "No puede ni necesita hacer una instalación no destructiva."
            vtwarn ""
            exit 1
        fi
    fi
    
    disk_sector_num=$(cat /sys/block/${DISK#/dev/}/size)
    disk_size_gb=$(expr $disk_sector_num / 2097152)

    if vtoycli partresize -t $DISK; then
        OldStyle="GPT"
    else
        OldStyle="MBR"
    fi

    #Print disk info
    echo "Disco  : $DISK"
    parted -s $DISK p 2>&1 | grep Model
    echo "Tamaño : $disk_size_gb GiB" 
    echo "Estilo : $OldStyle"    
    echo ''

    vtwarn "Atención:"
    vtwarn "AetherBoot intentará una instalación no destructiva en $DISK si es posible."
    echo ""

    read -p '¿Continuar? (y/n) '  Answer
    if [ "$Answer" != "y" ]; then
        if [ "$Answer" != "Y" ]; then
            exit 0
        fi
    fi

    if [ $disk_sector_num -le $VENTOY_SECTOR_NUM ]; then  
        vterr "No hay espacio suficiente en el disco $DISK"
        exit 1
    fi

    PART1=$(get_disk_part_name $DISK 1)  
    PART2=$(get_disk_part_name $DISK 2)  

    #Part1 size in MB aligned with 4KB
    PART1_SECTORS=$(cat /sys/class/block/${PART1#/dev/}/size)
    PART1_4K=$(expr $PART1_SECTORS / 8)
    PART1_MB=$(expr $PART1_4K / 256)
    PART1_NEW_MB=$(expr $PART1_MB - 32)

    echo "$PART1 tiene ${PART1_MB} MiB"

    #check partition layout
    echo "comprobando la distribución de particiones ..."
    vtoycli partresize -c $DISK
    vtRet=$?
    if [ $vtRet -eq 0 ]; then
        exit 1
    else
        # check and umount
        check_umount_disk "$DISK"
        sleep 1
        check_umount_disk "$DISK"
    
        if [ $vtRet -eq 1 ]; then
            echo "Hay espacio libre suficiente; iniciando la instalación..."
            part2_start_sector=$(expr $PART1_SECTORS + 2048)
        elif [ $vtRet -eq 2 ]; then
            echo "Primero hay que reducir la partición 1 ..."
            
            PART1_BLKID=$(blkid $PART1)
            blkid $PART1
            
            if echo $PART1_BLKID | grep -E -q -i 'TYPE=ntfs|TYPE=.ntfs'; then
                echo "La partición 1 contiene un sistema de archivos NTFS"
                
                which ntfsresize
                if [ $? -ne 0 ]; then
                    echo "###[ERROR] No se encontró ntfsresize. Instale el paquete ntfs-3g."
                    exit 1
                fi
                
                echo "ntfsfix -b -d $PART1 ..."
                ntfsfix -b -d $PART1
                
                echo "ntfsresize --size ${PART1_NEW_MB}Mi $PART1 ..."
                ntfsresize -f --size ${PART1_NEW_MB}Mi $PART1
                if [ $? -ne 0 ]; then
                    echo "###[ERROR] ntfsresize falló." 
                    exit 1
                fi
            elif echo $PART1_BLKID | grep -E -q -i 'TYPE=ext[2-4]|TYPE=.ext[2-4]'; then
                echo "La partición 1 contiene un sistema de archivos EXT"
                
                which resize2fs
                if [ $? -ne 0 ]; then
                    echo "###[ERROR] No se encontró resize2fs. Instale el paquete e2fsprogs."
                    exit 1
                fi
                
                echo "e2fsck -f $PART1 ..."
                e2fsck -f $PART1
                
                echo "resize2fs $PART1 ${PART1_NEW_MB}M ..."
                resize2fs $PART1 ${PART1_NEW_MB}M
                if [ $? -ne 0 ]; then
                    echo "###[ERROR] resize2fs falló." 
                    exit 1
                fi
            else
                echo "###[ERROR] Sistema de archivos no compatible en la partición 1."
                exit 1
            fi
            
            sync
            PART1_NEW_END_MB=$(expr $PART1_NEW_MB + 1)
            part2_start_sector=$(expr $PART1_NEW_END_MB \* 2048)
        fi
    fi

    vtinfo "escribiendo datos en el disco part2_start=$part2_start_sector ..."
    
    dd status=none conv=fsync if=./boot/boot.img of=$DISK bs=1 count=440
    
    if [ "$OldStyle" = "GPT" ]; then
        echo -en '\x22' | dd status=none of=$DISK conv=fsync bs=1 count=1 seek=92        
        xzcat ./boot/core.img.xz | dd status=none conv=fsync of=$DISK bs=512 count=2014 seek=34
        echo -en '\x23' | dd of=$DISK conv=fsync bs=1 count=1 seek=17908 status=none
    else
        xzcat ./boot/core.img.xz | dd status=none conv=fsync of=$DISK bs=512 count=2047 seek=1
    fi
    
    xzcat ./ventoy/ventoy.disk.img.xz | dd status=none conv=fsync of=$DISK bs=512 count=$VENTOY_SECTOR_NUM seek=$part2_start_sector
    
    #test UUID
    testUUIDStr=$(vtoy_gen_uuid | hexdump -C)
    vtdebug "test uuid: $testUUIDStr"
    
    #disk uuid
    vtoy_gen_uuid | dd status=none conv=fsync of=${DISK} seek=384 bs=1 count=16
    
    vtinfo "sincronizando datos ..."
    sync
    
    vtinfo "procesando la partición ESP ..."
    if [ "$SECUREBOOT" != "YES" ]; then
        vtoycli partresize -s $DISK $part2_start_sector
    fi

    vtinfo "actualizando la tabla de particiones $DISK $part2_start_sector ..."
    vtoycli partresize -p $DISK $part2_start_sector
    if [ $? -eq 0 ]; then
        sync
        echo ""
        vtinfo "La instalación no destructiva de AetherBoot en $DISK terminó correctamente."
        echo ""
    else
        echo ""
        vterr "Falló la instalación no destructiva de AetherBoot en $DISK."
        echo ""
    fi
    
else
    vtdebug "update Ventoy ..."
    
    oldver=$(get_disk_ventoy_version $DISK)
    if [ $? -ne 0 ]; then
        if is_disk_contains_ventoy $DISK; then
            oldver="Unknown"
        else
            vtwarn "$DISK no contiene AetherBoot o los datos están dañados"
            echo ""
            vtwarn "Use la opción -i si quiere instalar AetherBoot en $DISK"
            echo ""
            exit 1
        fi
    fi

    #reserve secure boot option
    if [ -z "$SECUREBOOT" ]; then
        if check_disk_secure_boot $DISK; then
            SECUREBOOT="YES"
        else
            SECUREBOOT="NO"
        fi
    fi

    curver=$(cat ./ventoy/version)

    vtinfo "La actualización es segura: los datos de la 1.ª partición (imágenes ISO y demás) no se modifican."
    echo ""

    read -p "Actualizar AetherBoot  $oldver ===> $curver   ¿Continuar? (y/n) "  Answer
    if [ "$Answer" != "y" ]; then
        if [ "$Answer" != "Y" ]; then
            exit 0
        fi
    fi

    PART2=$(get_disk_part_name $DISK 2)
    SHORT_PART2=${PART2#/dev/}
    part2_start=$(cat /sys/class/block/$SHORT_PART2/start)

    PART1_TYPE=$(dd if=$DISK bs=1 count=1 skip=450 status=none | hexdump -n1 -e  '1/1 "%02X"')

    #reserve disk uuid
    rm -f ./diskuuid.bin
    dd status=none conv=fsync if=${DISK} skip=384 bs=1 count=16 of=./diskuuid.bin

    dd status=none conv=fsync if=./boot/boot.img of=$DISK bs=1 count=440
    dd status=none conv=fsync if=./diskuuid.bin of=$DISK bs=1 count=16 seek=384
    rm -f ./diskuuid.bin

    #reserve data
    rm -f ./rsvdata.bin
    dd status=none conv=fsync if=${DISK} skip=2040 bs=512 count=8 of=./rsvdata.bin

    if [ "$PART1_TYPE" = "EE" ]; then
        vtdebug "This is GPT partition style ..."
        echo -en '\x22' | dd status=none of=$DISK conv=fsync bs=1 count=1 seek=92
        xzcat ./boot/core.img.xz | dd status=none conv=fsync of=$DISK bs=512 count=2014 seek=34
        echo -en '\x23' | dd of=$DISK conv=fsync bs=1 count=1 seek=17908 status=none
    else
        vtdebug "This is MBR partition style ..."

        PART1_ACTIVE=$(dd if=$DISK bs=1 count=1 skip=446 status=none | hexdump -n1 -e  '1/1 "%02X"')
        PART2_ACTIVE=$(dd if=$DISK bs=1 count=1 skip=462 status=none | hexdump -n1 -e  '1/1 "%02X"')

        vtdebug "PART1_ACTIVE=$PART1_ACTIVE  PART2_ACTIVE=$PART2_ACTIVE"
        if [ "$PART1_ACTIVE" = "00" ] && [ "$PART2_ACTIVE" = "80" ]; then
            vtdebug "change 1st partition active, 2nd partition inactive ..."
            echo -en '\x80' | dd of=$DISK conv=fsync bs=1 count=1 seek=446 status=none
            echo -en '\x00' | dd of=$DISK conv=fsync bs=1 count=1 seek=462 status=none
        fi
        xzcat ./boot/core.img.xz | dd status=none conv=fsync of=$DISK bs=512 count=2047 seek=1
    fi

    dd status=none conv=fsync if=./rsvdata.bin seek=2040 bs=512 count=8 of=${DISK}
    rm -f ./rsvdata.bin

    check_umount_disk "$DISK"
    
    xzcat ./ventoy/ventoy.disk.img.xz | dd status=none conv=fsync of=$DISK bs=512 count=$VENTOY_SECTOR_NUM seek=$part2_start
    sync

    vtinfo "procesando la partición ESP ..."
    if [ "$SECUREBOOT" != "YES" ]; then
        sleep 2
        check_umount_disk "$DISK"
        vtoycli partresize -s $DISK $part2_start
    fi

    
    if [ "$PART1_TYPE" = "EE" ]; then    
        vtinfo "actualizando los atributos de la partición ESP"
        vtoycli gpt -f $DISK
        sync
    fi


    echo ""
    vtinfo "AetherBoot se actualizó correctamente en $DISK."
    echo ""

fi


