#include "persistence.h"
#include "fs.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <stddef.h>

bool
persistence_file_exists(const char *filepath)
{	
	// Your code here: Verificar si el archivo existe
	// Hint: usar stat() y verificar el retorno

	if (!filepath)
		return false;
	
	struct stat st;
	return stat(filepath, &st) == 0;
	
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
	FILE *f = fopen(filepath, "rb");
	if (!f)
		return false;
	
	char magic[5] = { 0 };
	size_t read = fread(magic, 1, 4, f);
	fclose(f);
	
	return read == 4 && strcmp(magic, MAGIC_NUMBER) == 0;
	
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

	FILE *file = fopen(filename, "wb");
	if (!file) {
		fprintf(stderr, "[error] persistence_save: cannot open %s: %s\n", filename, strerror(errno));
		return -errno;
	}

	// Escribir magic number
	if (fwrite(MAGIC_NUMBER, 1, 4, file) != 4) {
		fclose(file);
		return -EIO;
	}

	// Escribir versión
	int version = FS_VERSION;
	if (fwrite(&version, sizeof(int), 1, file) != 1) {
		fclose(file);
		return -EIO;
	}

	// Escribir entry_count
	if (fwrite(&fs->entry_count, sizeof(int), 1, file) != 1) {
		fclose(file);
		return -EIO;
	}

	// Escribir used_space
	if (fwrite(&fs->used_space, sizeof(size_t), 1, file) != 1) {
		fclose(file);
		return -EIO;
	}

	// Escribir cada entrada
	for (int i = 0; i < fs->entry_count; i++) {
		const fs_entry_t *entry = &fs->entries[i];

		// Escribir metadatos (todo excepto el puntero data)
		if (fwrite(entry, offsetof(fs_entry_t, data), 1, file) != 1) {
			fclose(file);
			return -EIO;
		}

		// Si es archivo y tiene contenido, escribir los datos
		if (entry->type == ENTRY_FILE && entry->data != NULL && entry->size > 0) {
			if (fwrite(entry->data, entry->size, 1, file) != 1) {
				fclose(file);
				return -EIO;
			}
		}
	}

	fclose(file);
	printf("[info] Filesystem saved to %s (%d entries)\n", filename, fs->entry_count);
	return 0;
	
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

	FILE *file = fopen(filename, "rb");
	if (!file) {
		fprintf(stderr, "[error] persistence_load: cannot open %s: %s\n", filename, strerror(errno));
		return -errno;
	}

	// Leer y verificar magic number
	char magic[5] = { 0 };
	if (fread(magic, 1, 4, file) != 4 || strcmp(magic, MAGIC_NUMBER) != 0) {
		fclose(file);
		fprintf(stderr, "[error] Invalid magic number\n");
		return -EINVAL;
	}

	// Leer versión
	int version;
	if (fread(&version, sizeof(int), 1, file) != 1) {
		fclose(file);
		return -EIO;
	}

	if (version != FS_VERSION) {
		fclose(file);
		fprintf(stderr, "[error] Unsupported version: %d\n", version);
		return -EINVAL;
	}

	// Leer entry_count
	int entry_count;
	if (fread(&entry_count, sizeof(int), 1, file) != 1) {
		fclose(file);
		return -EIO;
	}

	if (entry_count > MAX_FILES) {
		fclose(file);
		fprintf(stderr, "[error] Too many entries: %d\n", entry_count);
		return -ENOSPC;
	}

	// Leer used_space
	size_t used_space;
	if (fread(&used_space, sizeof(size_t), 1, file) != 1) {
		fclose(file);
		return -EIO;
	}

	// Inicializar filesystem
	memset(fs, 0, sizeof(filesystem_t));
	fs->entry_count = 0;
	fs->used_space = used_space;
	fs->initialized = true;

	// Leer cada entrada
	for (int i = 0; i < entry_count; i++) {
		fs_entry_t *entry = &fs->entries[i];
		
		// Leer metadatos
		if (fread(entry, offsetof(fs_entry_t, data), 1, file) != 1) {
			// Error: liberar memoria ya alocada
			for (int j = 0; j < i; j++) {
				if (fs->entries[j].type == ENTRY_FILE && fs->entries[j].data) {
					free(fs->entries[j].data);
				}
			}
			fclose(file);
			return -EIO;
		}

		// Si es archivo y tiene contenido, leer los datos
		if (entry->type == ENTRY_FILE && entry->size > 0) {
			entry->data = malloc(entry->size);
			if (!entry->data) {
				// Error de memoria: liberar lo ya alocado
				for (int j = 0; j < i; j++) {
					if (fs->entries[j].type == ENTRY_FILE && fs->entries[j].data) {
						free(fs->entries[j].data);
					}
				}
				fclose(file);
				return -ENOMEM;
			}
			
			if (fread(entry->data, entry->size, 1, file) != 1) {
				// Error al leer: liberar todo
				free(entry->data);
				for (int j = 0; j < i; j++) {
					if (fs->entries[j].type == ENTRY_FILE && fs->entries[j].data) {
						free(fs->entries[j].data);
					}
				}
				fclose(file);
				return -EIO;
			}
		} else {
			entry->data = NULL;
		}

		fs->entry_count++;
	}

	fclose(file);
	printf("[info] Filesystem loaded from %s (%d entries, %zu bytes used)\n", filename, fs->entry_count, fs->used_space);
	
	return 0;


	
	return -EIO;
}