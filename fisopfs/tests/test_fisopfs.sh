#!/bin/bash

# Pruebas para fisopfs
# Ejecutar desde el directorio raíz: ./tests/test_fisopfs.sh

set +e  # Permitir que siga aunque haya errores

# Colores para la salida
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # Sin color

MOUNT_POINT="./test_mount"
FS_BIN="./fisopfs"
TEST_DISK="test_filesystem.fisopfs"

TESTS_PASSED=0
TESTS_FAILED=0

echo "DEBUG: Script iniciado"
echo "DEBUG: FS_BIN=$FS_BIN"
echo "DEBUG: PWD=$(pwd)"
echo "DEBUG: UID=$(id -u)"

# Función para imprimir mensajes
print_test() {
    echo -e "${BLUE}[TEST]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[✓]${NC} $1"
    ((TESTS_PASSED++))
}

print_error() {
    echo -e "${RED}[✗]${NC} $1"
    ((TESTS_FAILED++))
}

print_info() {
    echo -e "${YELLOW}[INFO]${NC} $1"
}

# Función de limpieza mejorada
cleanup() {
    echo ""
    print_info "Limpiando..."
    
    # Verificar si estamos en el contenedor/docker
    if [ "$(id -u)" = "0" ]; then
        print_info "Ejecutando como root en contenedor"
    fi
    
    # Desmontar si está montado - con múltiples métodos
    if mountpoint -q "$MOUNT_POINT" 2>/dev/null; then
        print_info "Desmontando $MOUNT_POINT"
        fusermount -u "$MOUNT_POINT" 2>/dev/null || 
        umount "$MOUNT_POINT" 2>/dev/null || 
        umount -l "$MOUNT_POINT" 2>/dev/null || true
        sleep 1
    fi
    
    # Matar proceso fisopfs si quedó corriendo
    print_info "Buscando procesos fisopfs..."
    pkill -f "fisopfs.*$MOUNT_POINT" 2>/dev/null || true
    sleep 1
    
    # Forzar kill si es necesario
    if pgrep -f "fisopfs.*$MOUNT_POINT" > /dev/null; then
        print_info "Forzando terminación de fisopfs"
        pkill -9 -f "fisopfs.*$MOUNT_POINT" 2>/dev/null || true
        sleep 1
    fi
    
    # Limpiar directorios
    if [ -d "$MOUNT_POINT" ]; then
        print_info "Eliminando directorio mount: $MOUNT_POINT"
        rm -rf "$MOUNT_POINT"
    fi
    
    if [ -f "$TEST_DISK" ]; then
        print_info "Eliminando archivo disk: $TEST_DISK"
        rm -f "$TEST_DISK"
    fi
    
    print_info "Limpieza completada"
}

# Trap para limpiar al salir
trap cleanup EXIT INT TERM

# ============================================================================
# SETUP INICIAL
# ============================================================================

print_info "=== Iniciando suite de pruebas para fisopfs ==="
echo ""

# Verificar que el binario existe
if [ ! -f "$FS_BIN" ]; then
    print_error "No se encontró el binario fisopfs. Ejecutar 'make' primero."
    echo "DEBUG: Buscando en: $(pwd)/$FS_BIN"
    ls -la ./fisopfs 2>/dev/null || echo "DEBUG: No existe ./fisopfs"
    exit 1
fi

# Verificar que el binario es ejecutable
if [ ! -x "$FS_BIN" ]; then
    print_error "El binario fisopfs no es ejecutable"
    chmod +x "$FS_BIN"
fi

# Limpiar primero
cleanup

# Crear directorio de montaje
mkdir -p "$MOUNT_POINT"
print_info "Directorio de montaje creado: $MOUNT_POINT"

# Verificar permisos
if [ ! -w "$MOUNT_POINT" ]; then
    print_error "Sin permisos de escritura en $MOUNT_POINT"
    exit 1
fi

# ============================================================================
# PRUEBA BÁSICA DE MONTAJE
# ============================================================================

print_test "Montando filesystem..."
echo "DEBUG: Comando: $FS_BIN $MOUNT_POINT --filedisk $TEST_DISK -f"

# Montar filesystem en segundo plano
$FS_BIN "$MOUNT_POINT" --filedisk "$TEST_DISK" -f &
FS_PID=$!
print_info "PID del filesystem: $FS_PID"

# Esperar a que se monte
sleep 3

# Verificar que se montó correctamente
if mountpoint -q "$MOUNT_POINT"; then
    print_success "Filesystem montado correctamente"
    
    # Prueba básica
    print_test "Prueba básica - listar directorio"
    if ls -la "$MOUNT_POINT" > /dev/null 2>&1; then
        print_success "Listado de directorio exitoso"
    else
        print_error "Falló listar directorio"
    fi
    
else
    print_error "No se pudo montar el filesystem"
    
    # Verificar si el proceso sigue corriendo
    if kill -0 $FS_PID 2>/dev/null; then
        print_info "Proceso fisopfs aún corriendo (PID: $FS_PID)"
    else
        print_error "Proceso fisopfs terminó"
    fi
    
    # Mostrar logs si es posible
    dmesg | tail -10 | grep -i fuse || true
    exit 1
fi

# ============================================================================
# PRUEBAS BÁSICAS DE DIRECTORIOS (UNITARIAS)
# ============================================================================

print_test "1. Verificar pseudo-directorios . y .."
OUTPUT=$(ls -a "$MOUNT_POINT")
if echo "$OUTPUT" | grep -q "^\.$" && echo "$OUTPUT" | grep -q "^\.\.$"; then
    print_success "Pseudo-directorios . y .. presentes"
else
    print_error "Faltan pseudo-directorios . o .."
fi

print_test "2. Crear directorio"
if mkdir "$MOUNT_POINT/test_dir" 2>/dev/null; then
    print_success "Directorio creado"
else
    print_error "Fallo al crear directorio"
fi

print_test "3. Verificar que el directorio existe"
if [ -d "$MOUNT_POINT/test_dir" ]; then
    print_success "Directorio verificado"
else
    print_error "Directorio no existe"
fi

# ============================================================================
# PRUEBAS DE ARCHIVOS
# ============================================================================

print_test "4. Crear archivo vacío con touch"
if touch "$MOUNT_POINT/empty_file.txt" 2>/dev/null; then
    print_success "Archivo vacío creado"
else
    print_error "Fallo al crear archivo vacío"
fi

print_test "5. Escribir en archivo (redirección >)"
if echo "Hola fisopfs" > "$MOUNT_POINT/hello.txt" 2>/dev/null; then
    print_success "Escritura con > exitosa"
else
    print_error "Fallo al escribir con >"
fi

print_test "6. Leer contenido de archivo"
CONTENT=$(cat "$MOUNT_POINT/hello.txt" 2>/dev/null)
if [ "$CONTENT" = "Hola fisopfs" ]; then
    print_success "Lectura correcta: '$CONTENT'"
else
    print_error "Contenido incorrecto: '$CONTENT'"
fi

print_test "7. Append a archivo (>>)"
if echo "Segunda línea" >> "$MOUNT_POINT/hello.txt" 2>/dev/null; then
    LINES=$(wc -l < "$MOUNT_POINT/hello.txt")
    if [ "$LINES" -eq 2 ]; then
        print_success "Append exitoso (2 líneas)"
    else
        print_error "Append falló (líneas: $LINES)"
    fi
else
    print_error "Fallo al hacer append"
fi

print_test "8. Obtener estadísticas con stat"
if stat "$MOUNT_POINT/hello.txt" > /dev/null 2>&1; then
    SIZE=$(stat -c %s "$MOUNT_POINT/hello.txt")
    print_success "Stat exitoso (tamaño: $SIZE bytes)"
else
    print_error "Fallo al ejecutar stat"
fi

print_test "9. Verificar tamaño de archivo"
EXPECTED_SIZE=28  # "Hola fisopfs\nSegunda línea\n" - 28
ACTUAL_SIZE=$(stat -c %s "$MOUNT_POINT/hello.txt")
if [ "$ACTUAL_SIZE" -eq "$EXPECTED_SIZE" ]; then
    print_success "Tamaño correcto: $ACTUAL_SIZE bytes"
else
    print_error "Tamaño incorrecto: esperado $EXPECTED_SIZE, obtenido $ACTUAL_SIZE"
fi

# ============================================================================
# PRUEBAS DE DIRECTORIOS ANIDADOS
# ============================================================================

print_test "10. Crear subdirectorio"
if mkdir "$MOUNT_POINT/test_dir/subdir" 2>/dev/null; then
    print_success "Subdirectorio creado"
else
    print_error "Fallo al crear subdirectorio"
fi

print_test "11. Crear archivo en subdirectorio"
if echo "archivo anidado" > "$MOUNT_POINT/test_dir/subdir/nested.txt" 2>/dev/null; then
    print_success "Archivo en subdirectorio creado"
else
    print_error "Fallo al crear archivo en subdirectorio"
fi

print_test "12. Leer archivo de subdirectorio"
NESTED_CONTENT=$(cat "$MOUNT_POINT/test_dir/subdir/nested.txt" 2>/dev/null)
if [ "$NESTED_CONTENT" = "archivo anidado" ]; then
    print_success "Lectura de archivo anidado correcta"
else
    print_error "Contenido de archivo anidado incorrecto"
fi

# ============================================================================
# PRUEBAS DE BORRADO
# ============================================================================

print_test "13. Borrar archivo"
if rm "$MOUNT_POINT/empty_file.txt" 2>/dev/null; then
    if [ ! -f "$MOUNT_POINT/empty_file.txt" ]; then
        print_success "Archivo borrado correctamente"
    else
        print_error "Archivo aún existe después de rm"
    fi
else
    print_error "Fallo al borrar archivo"
fi

print_test "14. Intentar borrar directorio no vacío (debe fallar)"
if rmdir "$MOUNT_POINT/test_dir" 2>/dev/null; then
    print_error "Se borró un directorio no vacío (no debería)"
else
    print_success "Correctamente rechazó borrar directorio no vacío"
fi

print_test "15. Vaciar subdirectorio"
if rm "$MOUNT_POINT/test_dir/subdir/nested.txt" 2>/dev/null; then
    print_success "Archivo de subdirectorio borrado"
else
    print_error "Fallo al borrar archivo de subdirectorio"
fi

print_test "16. Borrar subdirectorio vacío"
if rmdir "$MOUNT_POINT/test_dir/subdir" 2>/dev/null; then
    print_success "Subdirectorio vacío borrado"
else
    print_error "Fallo al borrar subdirectorio vacío"
fi

print_test "17. Borrar directorio ahora vacío"
if rmdir "$MOUNT_POINT/test_dir" 2>/dev/null; then
    print_success "Directorio vacío borrado"
else
    print_error "Fallo al borrar directorio vacío"
fi

# ============================================================================
# PRUEBAS DE PERSISTENCIA
# ============================================================================

print_test "18. Crear datos para prueba de persistencia"
mkdir "$MOUNT_POINT/persist_dir" 2>/dev/null
echo "datos persistentes" > "$MOUNT_POINT/persist_dir/persist.txt" 2>/dev/null
print_success "Datos de prueba creados"

print_test "19. Desmontar filesystem"
fusermount -u "$MOUNT_POINT" || umount "$MOUNT_POINT"
sleep 1
wait $FS_PID 2>/dev/null || true
print_success "Filesystem desmontado"

print_test "20. Verificar archivo de persistencia"
if [ -f "$TEST_DISK" ]; then
    DISK_SIZE=$(stat -c %s "$TEST_DISK")
    print_success "Archivo de persistencia existe ($DISK_SIZE bytes)"
else
    print_error "Archivo de persistencia no existe"
fi

print_test "21. Remontar filesystem"
$FS_BIN "$MOUNT_POINT" --filedisk "$TEST_DISK" -f &
FS_PID=$!
sleep 2

if mountpoint -q "$MOUNT_POINT"; then
    print_success "Filesystem remontado"
else
    print_error "Fallo al remontar filesystem"
fi

print_test "22. Verificar persistencia de datos"
if [ -f "$MOUNT_POINT/persist_dir/persist.txt" ]; then
    PERSIST_CONTENT=$(cat "$MOUNT_POINT/persist_dir/persist.txt")
    if [ "$PERSIST_CONTENT" = "datos persistentes" ]; then
        print_success "Datos persistieron correctamente"
    else
        print_error "Contenido persistido incorrecto: '$PERSIST_CONTENT'"
    fi
else
    print_error "Archivo persistido no existe"
fi

print_test "23. Verificar que hello.txt también persistió"
if [ -f "$MOUNT_POINT/hello.txt" ]; then
    print_success "Archivo hello.txt persistió"
else
    print_error "Archivo hello.txt no persistió"
fi

# ============================================================================
# PRUEBAS DE CONTENIDO BINARIO
# ============================================================================

print_test "24. Crear archivo binario"
dd if=/dev/urandom of="$MOUNT_POINT/binary.dat" bs=1024 count=1 2>/dev/null
if [ -f "$MOUNT_POINT/binary.dat" ]; then
    SIZE=$(stat -c %s "$MOUNT_POINT/binary.dat")
    if [ "$SIZE" -eq 1024 ]; then
        print_success "Archivo binario creado (1024 bytes)"
    else
        print_error "Tamaño de archivo binario incorrecto: $SIZE"
    fi
else
    print_error "Fallo al crear archivo binario"
fi

print_test "25. Leer archivo binario"
if dd if="$MOUNT_POINT/binary.dat" of="/tmp/binary_copy.dat" bs=1024 count=1 2>/dev/null; then
    COPY_SIZE=$(stat -c %s "/tmp/binary_copy.dat")
    if [ "$COPY_SIZE" -eq 1024 ]; then
        print_success "Archivo binario leído correctamente"
    else
        print_error "Tamaño de copia incorrecto: $COPY_SIZE"
    fi
else
    print_error "Fallo al leer archivo binario"
fi
rm -f /tmp/binary_copy.dat

# ============================================================================
# PRUEBAS DE LÍMITES
# ============================================================================

print_test "26. Crear múltiples archivos"
SUCCESS_COUNT=0
for i in {1..10}; do
    if echo "archivo $i" > "$MOUNT_POINT/file_$i.txt" 2>/dev/null; then
        ((SUCCESS_COUNT++))
    fi
done
if [ "$SUCCESS_COUNT" -eq 10 ]; then
    print_success "Creados 10 archivos exitosamente"
else
    print_error "Solo se crearon $SUCCESS_COUNT de 10 archivos"
fi

print_test "27. Listar todos los archivos"
FILE_COUNT=$(ls -1 "$MOUNT_POINT" | grep -v '^\.' | wc -l)
print_success "Archivos en filesystem: $FILE_COUNT"


# ============================================================================
# PRUEBAS DE ARCHIVOS VACÍOS
# ============================================================================

print_test "28. Crear y verificar archivo vacío"
touch "$MOUNT_POINT/empty_test.txt"
if [ -f "$MOUNT_POINT/empty_test.txt" ]; then
    SIZE=$(stat -c %s "$MOUNT_POINT/empty_test.txt")
    if [ "$SIZE" -eq 0 ]; then
        print_success "Archivo vacío creado correctamente (0 bytes)"
    else
        print_error "Archivo vacío tiene tamaño $SIZE, esperado 0"
    fi
else
    print_error "No se pudo crear archivo vacío"
fi

# ============================================================================
# PRUEBAS DE ESCRITURA EN POSICIÓN
# ============================================================================

print_test "29. Escritura en posición específica"
echo "1234567890" > "$MOUNT_POINT/position.txt"
printf "ABC" | dd of="$MOUNT_POINT/position.txt" bs=1 seek=4 conv=notrunc 2>/dev/null
CONTENT=$(cat "$MOUNT_POINT/position.txt")
if [ "$CONTENT" = "1234ABC890" ]; then
    print_success "Escritura en posición correcta: '$CONTENT'"
else
    print_error "Escritura en posición falló: '$CONTENT'"
fi

# ============================================================================
# RESUMEN
# ============================================================================

echo ""
echo "========================================"
echo "         RESUMEN DE PRUEBAS"
echo "========================================"
echo -e "${GREEN}OK:${NC} $TESTS_PASSED"
echo -e "${RED}ERROR:${NC} $TESTS_FAILED"
echo "========================================"

if [ $TESTS_FAILED -eq 0 ]; then
    echo -e "${GREEN} TODO OK ${NC}" 
    exit 0
else
    echo -e "${RED}Algunas pruebas fallaron${NC}"
    exit 1
fi