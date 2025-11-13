#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "fs.h"
#include "persistence.h"
#include <linux/limits.h> // Para que PATH_MAX no salga undeefined

#define DEFAULT_FILE_DISK "persistence_file.fisopfs"

// Filesystem global (vive en memoria durante la ejecución)
static filesystem_t g_fs;

// Path absoluto para archivo de persistencia
static char filedisk_path[2 * PATH_MAX];

// ============================================================================
// OPERACIONES FUSE
// ============================================================================

static int
fisopfs_getattr(const char *path, struct stat *st)
{
	printf("[debug] fisopfs_getattr - path: %s\n", path);
	
	// Your code here: Llamar a fs_getattr(&g_fs, path, st)
	// Manejar el código de error retornado
	
	return -ENOENT;
}

static int
fisopfs_readdir(const char *path,
                void *buffer,
                fuse_fill_dir_t filler,
                off_t offset,
                struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_readdir - path: %s\n", path);
	
	// Siempre incluir . y ..
	filler(buffer, ".", NULL, 0);
	filler(buffer, "..", NULL, 0);
	
	// Your code here: Listar directorio
	// 1. Declarar char **entries = NULL; int count = 0;
	// 2. Llamar a fs_read_dir(&g_fs, path, &entries, &count)
	// 3. Si hay error, retornar el código de error
	// 4. Para cada entrada (i = 0 hasta count):
	//    - Llamar a filler(buffer, entries[i], NULL, 0)
	//    - Liberar entries[i] con free()
	// 5. Liberar el array entries con free()
	// 6. Retornar 0
	
	return -ENOENT;
}

static int
fisopfs_read(const char *path,
             char *buffer,
             size_t size,
             off_t offset,
             struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_read - path: %s, offset: %lu, size: %lu\n",
	       path, offset, size);
	
	// Your code here: Llamar a fs_read_file(&g_fs, path, buffer, size, offset)
	// Retornar el número de bytes leídos o código de error
	
	return -ENOENT;
}

static int
fisopfs_write(const char *path,
              const char *buffer,
              size_t size,
              off_t offset,
              struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_write - path: %s, offset: %lu, size: %lu\n",
	       path, offset, size);
	
	// Your code here: Llamar a fs_write_file(&g_fs, path, buffer, size, offset)
	// Retornar el número de bytes escritos o código de error
	
	return -ENOENT;
}

static int
fisopfs_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_create - path: %s, mode: %o\n", path, mode);
	
	// Your code here: Llamar a fs_create_file(&g_fs, path, S_IFREG | mode)
	// Retornar 0 si éxito o código de error
	
	return -ENOSPC;
}

static int
fisopfs_mkdir(const char *path, mode_t mode)
{
	printf("[debug] fisopfs_mkdir - path: %s, mode: %o\n", path, mode);
	
	// Your code here: Llamar a fs_create_dir(&g_fs, path, S_IFDIR | mode)
	// Retornar 0 si éxito o código de error
	
	return -ENOSPC;
}

static int
fisopfs_unlink(const char *path)
{
	printf("[debug] fisopfs_unlink - path: %s\n", path);
	
	// Your code here: Llamar a fs_unlink_file(&g_fs, path)
	// Retornar 0 si éxito o código de error
	
	return -ENOENT;
}

static int
fisopfs_rmdir(const char *path)
{
	printf("[debug] fisopfs_rmdir - path: %s\n", path);
	
	// Your code here: Llamar a fs_remove_dir(&g_fs, path)
	// Retornar 0 si éxito o código de error
	
	return -ENOENT;
}

static int
fisopfs_truncate(const char *path, off_t size)
{
	printf("[debug] fisopfs_truncate - path: %s, size: %ld\n", path, size);
	
	// Your code here: Llamar a fs_truncate_file(&g_fs, path, size)
	// Retornar 0 si éxito o código de error
	
	return -ENOENT;
}

static int
fisopfs_utimens(const char *path, const struct timespec ts[2])
{
	printf("[debug] fisopfs_utimens - path: %s\n", path);
	
	// Your code here: Llamar a fs_update_times(&g_fs, path, ts)
	// Retornar 0 si éxito o código de error
	
	return 0;
}

// ============================================================================
// OPERACIONES QUE SE MANEJAN DIRECTAMENTE EN FISOPFS.C
// ============================================================================

static int
fisopfs_open(const char *path, struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_open - path: %s\n", path);
	
	// Your code here: Verificar que el archivo existe
	// 1. Llamar a fs_find_entry(&g_fs, path)
	// 2. Si retorna < 0, retornar -ENOENT
	// 3. Si existe, retornar 0
	
	// Verificar que el archivo existe
	int idx = fs_find_entry(&g_fs, path);
	if (idx < 0) {
		return -ENOENT;
	}
	
	return 0;
}

static int
fisopfs_flush(const char *path, struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_flush - path: %s\n", path);
	
	// Your code here: Persistir filesystem a disco
	// Llamar a persistence_save(filedisk_path, &g_fs)
	// Retornar el código de error o 0 si éxito
	
	return 0;
}

// ============================================================================
// INIT Y DESTROY
// ============================================================================

static void *
fisopfs_init(struct fuse_conn_info *conn)
{
	printf("[debug] fisopfs_init\n");
	
	// Your code here: Cargar filesystem desde disco
	// 1. Llamar a persistence_load(filedisk_path, &g_fs)
	// 2. Si retorna error, imprimir mensaje y llamar a exit(EXIT_FAILURE)
	// 3. Imprimir mensaje de éxito
	// 4. Retornar NULL

	// Cargar filesystem desde disco
	int res = persistence_load(filedisk_path, &g_fs);
	if (res < 0) {
		fprintf(stderr, "[error] init failed: %s\n", strerror(-res));
		exit(EXIT_FAILURE);
	}
	
	printf("[info] Filesystem initialized successfully\n");
	return NULL;
	
	return NULL;
}

static void
fisopfs_destroy(void *private_data)
{
	printf("[debug] fisopfs_destroy\n");
	
	// Your code here: Persistir y limpiar
	// 1. Llamar a persistence_save(filedisk_path, &g_fs)
	// 2. Si hay error, imprimir mensaje
	// 3. Llamar a fs_destroy(&g_fs) para liberar memoria
	// 4. Imprimir mensaje de éxito

	// Persistir filesystem a disco antes de salir
	int res = persistence_save(filedisk_path, &g_fs);
	if (res < 0) {
		fprintf(stderr, "[error] destroy save failed: %s\n", strerror(-res));
	}
	
	// Limpiar estructuras
	fs_destroy(&g_fs);
	
	printf("[info] Filesystem destroyed successfully\n");
}

// ============================================================================
// ESTRUCTURA DE OPERACIONES
// ============================================================================

static struct fuse_operations operations = {
	// Your code here: Asignar todas las funciones a sus campos
	// Ejemplo: .getattr = fisopfs_getattr,
	// Completar con: readdir, read, write, create, mkdir, unlink, rmdir,
	//                truncate, utimens, open, flush, init, destroy
	.getattr = fisopfs_getattr,
	.readdir = fisopfs_readdir,
	.read = fisopfs_read,
	.write = fisopfs_write,
	.create = fisopfs_create,
	.mkdir = fisopfs_mkdir,
	.unlink = fisopfs_unlink,
	.rmdir = fisopfs_rmdir,
	.truncate = fisopfs_truncate,
	.utimens = fisopfs_utimens,
	.open = fisopfs_open,
	.flush = fisopfs_flush,
	.init = fisopfs_init,
	.destroy = fisopfs_destroy,
};

// ============================================================================
// MAIN
// ============================================================================

int
main(int argc, char *argv[])
{
	char *filedisk_name = DEFAULT_FILE_DISK;
	
	// Parsear argumento --filedisk
	for (int i = 1; i < argc - 1; i++) {
		if (strcmp(argv[i], "--filedisk") == 0) {
			filedisk_name = argv[i + 1];
			
			// Remover el argumento para que FUSE no lo procese
			for (int j = i; j < argc - 1; j++) {
				argv[j] = argv[j + 2];
			}
			argc = argc - 2;
			break;
		}
	}
	
	// Your code here: Construir path absoluto para el archivo de persistencia
	// 1. Declarar char cwd[PATH_MAX];
	// 2. Llamar a getcwd(cwd, sizeof(cwd))
	// 3. Si filedisk_name empieza con '/', es absoluto: strcpy(filedisk_path, filedisk_name)
	// 4. Si no, es relativo: snprintf(filedisk_path, sizeof(filedisk_path), "%s/%s", cwd, filedisk_name)
	// 5. Imprimir mensaje con el path que se usará
	
	// Construir path absoluto para el archivo de persistencia
	char cwd[PATH_MAX];
	if (getcwd(cwd, sizeof(cwd)) == NULL) {
		fprintf(stderr, "[error] getcwd failed\n");
		return EXIT_FAILURE;
	}
	
	// Si el path es relativo, convertir a absoluto
	if (filedisk_name[0] == '/') {
		strcpy(filedisk_path, filedisk_name);
	} else {
		snprintf(filedisk_path, sizeof(filedisk_path), "%s/%s", cwd, filedisk_name);
	}
	
	printf("[info] Using persistence file: %s\n", filedisk_path);
	
	return fuse_main(argc, argv, &operations, NULL);
}