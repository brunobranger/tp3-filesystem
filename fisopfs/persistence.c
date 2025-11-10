#include "persistence.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

bool
persistence_file_exists(const char *filepath)
{
	if (!filepath)
		return false;
	
	// Your code here: Verificar si el archivo existe
	// Hint: usar stat() y verificar el retorno
	
	return false;
}

bool
persistence_is_valid(const char *filepath)
{
	if (!persistence_file_exists(filepath))
		return false;
	
	// Your code here: Verificar que el archivo tenga el magic number correcto
	// 1. Abrir archivo en modo lectura binaria "rb"
	// 2. Leer los primeros 4 bytes
	// 3. Comparar con MAGIC_NUMBER usando strcmp()
	// 4. Cerrar archivo
	// 5. Retornar true si coincide, false si no
	
	return false;
}

int
persistence_save(const char *filename, const filesystem_t *fs)
{
	if (!filename || !fs || !fs->initialized) {
		fprintf(stderr, "[error] persistence_save: invalid arguments\n");
		return -EINVAL;
	}

	// Your code here: Guardar filesystem en disco
	// 1. Abrir archivo en modo escritura binaria "wb"
	// 2. Escribir MAGIC_NUMBER (4 bytes)
	// 3. Escribir FS_VERSION (int)
	// 4. Escribir fs->entry_count (int)
	// 5. Escribir fs->used_space (size_t)
	// 6. Para cada entrada (i = 0 hasta entry_count):
	//    a. Escribir metadatos usando fwrite(entry, offsetof(fs_entry_t, data), 1, file)
	//    b. Si es archivo y tiene data:
	//       - Escribir entry->data usando fwrite(entry->data, entry->size, 1, file)
	// 7. Cerrar archivo
	// 8. Manejar errores en cada fwrite (retornar -EIO si falla)
	// 9. Retornar 0 si éxito
	
	return -EIO;
}

int
persistence_load(const char *filename, filesystem_t *fs)
{
	if (!filename || !fs) {
		fprintf(stderr, "[error] persistence_load: invalid arguments\n");
		return -EINVAL;
	}

	// Si el archivo no existe, inicializar filesystem nuevo
	if (!persistence_file_exists(filename)) {
		printf("[info] Persistence file not found, creating new filesystem\n");
		return fs_init(fs);
	}

	// Verificar que sea un archivo válido
	if (!persistence_is_valid(filename)) {
		fprintf(stderr, "[error] persistence_load: invalid file format\n");
		return -EINVAL;
	}

	// Your code here: Cargar filesystem desde disco
	// 1. Abrir archivo en modo lectura binaria "rb"
	// 2. Leer y verificar MAGIC_NUMBER (4 bytes)
	// 3. Leer versión (int) y verificar que sea FS_VERSION
	// 4. Leer entry_count (int) y verificar que sea <= MAX_FILES
	// 5. Leer used_space (size_t)
	// 6. Inicializar fs con memset a 0
	// 7. Configurar fs->entry_count = 0, fs->used_space = used_space, fs->initialized = true
	// 8. Para cada entrada (i = 0 hasta entry_count):
	//    a. Leer metadatos usando fread(&entry, offsetof(fs_entry_t, data), 1, file)
	//    b. Si es archivo y entry->size > 0:
	//       - Alocar memoria: entry->data = malloc(entry->size)
	//       - Leer datos: fread(entry->data, entry->size, 1, file)
	//    c. Si es directorio o archivo vacío: entry->data = NULL
	//    d. Copiar entry a fs->entries[i]
	//    e. Incrementar fs->entry_count
	// 9. Cerrar archivo
	// 10. Manejar errores:
	//     - Si falla lectura o malloc, liberar memoria ya alocada y retornar error
	//     - Usar un loop para liberar todas las entradas previas si hay error
	// 11. Retornar 0 si éxito
	
	return -EIO;
}