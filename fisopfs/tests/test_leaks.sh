#!/bin/bash

MOUNT_POINT="./mnt_valgrind"
LOG_FILE="valgrind_report.txt"
FS_BIN="./fisopfs"
DISK_IMG="vg_disk.fisopfs"

print_success() { echo -e "\033[0;32m$1\033[0m"; }
print_error() { echo -e "\033[0;31m$1\033[0m"; }

rm -rf "$MOUNT_POINT" "$LOG_FILE" "$DISK_IMG"
mkdir -p "$MOUNT_POINT"

echo "=== INICIANDO PRUEBA DE MEMORIA (VALGRIND) ==="


echo "[INFO] Lanzando Valgrind..."
valgrind --leak-check=full \
         --show-leak-kinds=all \
         --track-origins=yes \
         --log-file=$LOG_FILE \
         $FS_BIN "$MOUNT_POINT" --filedisk "$DISK_IMG" -f &

FS_PID=$!
sleep 3 

#verificar si el proceso murio al inicio
if ! kill -0 $FS_PID 2>/dev/null; then
    print_error "[ERROR] El proceso murió al iniciar. Revisa $LOG_FILE"
    cat $LOG_FILE
    exit 1
fi

echo "[INFO] Creando y borrando archivos para estresar malloc/free..."
for i in {1..20}; do
    #crear un archivo 
    echo "contenido de prueba para valgrind $i" > "$MOUNT_POINT/file_$i.txt"
    
    echo "mas datos" >> "$MOUNT_POINT/file_$i.txt"
    
    #borrar (free de fs_entry_t y su data)
    rm "$MOUNT_POINT/file_$i.txt"
done


echo "[INFO] Desmontando..."
fusermount -u "$MOUNT_POINT"
sleep 2

wait $FS_PID 2>/dev/null

# analizar el reporte
echo ""
echo "=== RESULTADOS DE VALGRIND ==="
if [ -f "$LOG_FILE" ]; then

    grep -E "HEAP SUMMARY|ERROR SUMMARY|All heap blocks were freed" "$LOG_FILE"

    echo ""

    SUCCESS_LINE=$(grep "All heap blocks were freed -- no leaks are possible" "$LOG_FILE")
    
    if [ -n "$SUCCESS_LINE" ]; then
        print_success "[ÉXITO] No hay fugas de memoria definitivas."
    else

        LOST_INFO=$(grep "definitely lost:" "$LOG_FILE" || echo "definitely lost: (Línea no encontrada)")
        print_error "[FALLO] Se detectaron fugas de memoria (definitivamente perdidas).";
        echo "Línea de fallo: $LOST_INFO";
        echo "Revisa el archivo $LOG_FILE para detalles.";
    fi
else
    print_error "[ERROR] No se generó archivo de log."
fi

#limpieza final
rm -rf "$MOUNT_POINT" "$DISK_IMG"