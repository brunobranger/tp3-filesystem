#ifndef FS_H
#define FS_H

#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <stdbool.h>

// Límites del filesystem
#define MAX_FILES 128
#define MAX_DIRS 32
#define MAX_FILENAME 256
#define MAX_PATH 512
#define MAX_FILE_SIZE (10 * 1024 * 1024)  // 10MB por archivo
#define MAX_FS_SIZE (50 * 1024 * 1024)     // 50MB total
#define MAX_DIR_DEPTH 8

// Tipos de entrada
typedef enum {
	ENTRY_FILE,
	ENTRY_DIR
} entry_type_t;

// Estructura para metadatos de archivo
typedef struct {
	char name[MAX_FILENAME];
	entry_type_t type;
	uid_t uid;
	gid_t gid;
	mode_t mode;
	size_t size;
	time_t atime;  // access time
	time_t mtime;  // modification time
	time_t ctime;  // change time
	int nlink;     // número de hard links
	
	// Para archivos
	char *data;
	
	// Para directorios
	int parent_idx;  // índice del directorio padre (-1 para root)
} fs_entry_t;

// Estructura principal del filesystem
typedef struct {
	fs_entry_t entries[MAX_FILES];
	int entry_count;
	size_t used_space;
	bool initialized;
} filesystem_t;

// Inicialización y limpieza
int fs_init(filesystem_t *fs);
void fs_destroy(filesystem_t *fs);

// ============================================================================
// OPERACIONES OBLIGATORIAS
// ============================================================================

// Operaciones sobre archivos
int fs_create_file(filesystem_t *fs, const char *path, mode_t mode);
int fs_read_file(filesystem_t *fs, const char *path, char *buffer, size_t size, off_t offset);
int fs_write_file(filesystem_t *fs, const char *path, const char *buffer, size_t size, off_t offset);
int fs_truncate_file(filesystem_t *fs, const char *path, off_t size);
int fs_unlink_file(filesystem_t *fs, const char *path);

// Operaciones sobre directorios
int fs_create_dir(filesystem_t *fs, const char *path, mode_t mode);
int fs_remove_dir(filesystem_t *fs, const char *path);
int fs_read_dir(filesystem_t *fs, const char *path, char ***entries, int *count);

// Operaciones generales
int fs_getattr(filesystem_t *fs, const char *path, struct stat *st);
int fs_update_times(filesystem_t *fs, const char *path, const struct timespec tv[2]);

// ============================================================================
// OPERACIONES OPCIONALES (para challenges)
// ============================================================================

// Descomentar si implementan los challenges
// int fs_create_symlink(filesystem_t *fs, const char *target, const char *linkpath);
// int fs_create_hardlink(filesystem_t *fs, const char *target, const char *linkpath);
// int fs_chmod(filesystem_t *fs, const char *path, mode_t mode);
// int fs_chown(filesystem_t *fs, const char *path, uid_t uid, gid_t gid);
// int fs_rename(filesystem_t *fs, const char *oldpath, const char *newpath);

// ============================================================================
// UTILIDADES INTERNAS
// ============================================================================

int fs_find_entry(filesystem_t *fs, const char *path);
int fs_path_depth(const char *path);
void fs_split_path(const char *path, char *parent, char *name);
bool fs_is_root(const char *path);

#endif  // FS_H