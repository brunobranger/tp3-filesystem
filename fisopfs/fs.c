#include "fs.h"
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
// Estos 3 de aca abajo los agregue para que S_IFDIR no salga undefined
#include <fcntl.h>
#include <linux/stat.h>
#include <sys/stat.h>
#include <wine/msvcrt/sys/stat.h>

// ============================================================================
// INICIALIZACIÓN Y LIMPIEZA
// ============================================================================

int
fs_init(filesystem_t *fs)
{
	if (!fs)
		return -EINVAL;

	memset(fs, 0, sizeof(filesystem_t));
	
	// Your code here: Crear directorio raíz
	// - Inicializar path y name
	// - Configurar type, uid, gid, mode
	// - Establecer nlink, parent_idx
	// - Inicializar timestamps (atime, mtime, ctime)
	// - Configurar entry_count, used_space, initialized

	// Crear directorio raíz
	fs_entry_t *root = &fs->entries[0];
	strcpy(root->path, "/");
	strcpy(root->name, "/");
	root->type = ENTRY_DIR;
	root->uid = getuid();
	root->gid = getgid();
	root->mode = S_IFDIR | 0755;
	root->size = 0;
	root->nlink = 2;
	root->parent_idx = -1;
	
	time_t now = time(NULL);
	root->atime = now;
	root->mtime = now;
	root->ctime = now;
	
	fs->entry_count = 1;
	fs->used_space = 0;
	fs->initialized = true;
	
	return 0;
	
	return 0;
}

void
fs_destroy(filesystem_t *fs)
{
	if (!fs || !fs->initialized)
		return;
	
	// Your code here: Liberar memoria de todos los archivos
	// - Recorrer todas las entradas
	// - Si es archivo y tiene data, hacer free(data)
	// - Marcar filesystem como no inicializado

	// Liberar memoria de todos los archivos
	for (int i = 0; i < fs->entry_count; i++) {
		if (fs->entries[i].type == ENTRY_FILE && fs->entries[i].data != NULL) {
			free(fs->entries[i].data);
			fs->entries[i].data = NULL;
		}
	}
	
	fs->initialized = false;
}

// ============================================================================
// UTILIDADES PARA PATHS
// ============================================================================

bool
fs_is_root(const char *path)
{
	// Your code here: Verificar si path es "/"
	return path && strcmp(path, "/") == 0;
}

int
fs_path_depth(const char *path)
{
	// Your code here: Contar el número de '/' en el path
	// Retornar 0 para la raíz
	if (!path || fs_is_root(path))
		return 0;
	
	int depth = 0;
	for (const char *p = path; *p; p++) {
		if (*p == '/')
			depth++;
	}
	return depth;
}

void
fs_split_path(const char *path, char *parent, char *name)
{
	if (!path || !parent || !name) {
		if (parent) parent[0] = '\0';
		if (name) name[0] = '\0';
		return;
	}
	
	// Your code here: Separar path en directorio padre y nombre
	// Ejemplos:
	//   "/file.txt" -> parent="/", name="file.txt"
	//   "/dir/file.txt" -> parent="/dir", name="file.txt"
	//   "/" -> parent="/", name=""
	// Hint: usar strrchr() para encontrar la última '/'

	// Caso especial: raíz
	if (strcmp(path, "/") == 0) {
		strcpy(parent, "/");
		strcpy(name, "");
		return;
	}
	
	// Buscar la última barra
	const char *last_slash = strrchr(path, '/');
	
	if (last_slash == NULL) {
		// No hay barra (no debería pasar en paths bien formados)
		strcpy(parent, "/");
		strcpy(name, path);
		return;
	}
	
	// Copiar el nombre (después de la última barra)
	strcpy(name, last_slash + 1);
	
	// Copiar el parent
	if (last_slash == path) {
		// Caso: "/archivo.txt" -> parent = "/"
		strcpy(parent, "/");
	} else {
		// Caso: "/dir/archivo.txt" -> parent = "/dir"
		size_t len = last_slash - path;
		strncpy(parent, path, len);
		parent[len] = '\0';
	}
}	

// ============================================================================
// BÚSQUEDA DE ENTRADAS
// ============================================================================

int
fs_find_entry(filesystem_t *fs, const char *path)
{
	if (!fs || !fs->initialized)
		return -EINVAL;
	
	// Caso especial: raíz
	if (fs_is_root(path))
		return 0;
	
	// Your code here: Buscar entrada por path
	// - Recorrer fs->entries hasta fs->entry_count
	// - Comparar entry.path con path usando strcmp()
	// - Retornar índice si se encuentra
	// - Retornar -ENOENT si no existe
	
	// Buscar entrada por path
	for (int i = 0; i < fs->entry_count; i++) {
		if (strcmp(fs->entries[i].path, path) == 0) {
			return i;
		}
	}
	
	return -ENOENT;
}

// ============================================================================
// OPERACIONES DE ARCHIVOS
// ============================================================================

int
fs_create_file(filesystem_t *fs, const char *path, mode_t mode)
{
	if (!fs || !fs->initialized || !path)
        return -EINVAL;

    if (fs->entry_count >= MAX_FILES)
        return -ENOSPC;

    if (fs_find_entry(fs, path) >= 0)
        return -EEXIST;

    char parent[MAX_PATH_LEN];
    char name[MAX_NAME_LEN];
    fs_split_path(path, parent, name);

    if (strlen(name) == 0)
        return -EINVAL; 

    int parent_idx = fs_find_entry(fs, parent);
    if (parent_idx < 0)
        return -ENOENT;

    fs_entry_t *p = &fs->entries[parent_idx];
    if (p->type != ENTRY_DIR)
        return -ENOTDIR;

    int idx = fs->entry_count;
    fs_entry_t *entry = &fs->entries[idx];
    memset(entry, 0, sizeof(fs_entry_t));

    strcpy(entry->path, path);
    strcpy(entry->name, name);

    entry->type = ENTRY_FILE;
    entry->uid  = getuid();
    entry->gid  = getgid();
    entry->mode = S_IFREG | (mode & 0777);

    entry->size = 0;
    entry->nlink = 1;
    entry->parent_idx = parent_idx;
    entry->data = NULL;

    time_t now = time(NULL);
    entry->atime = now;
    entry->mtime = now;
    entry->ctime = now;

    fs->entry_count++;

    return 0;
}

int
fs_read_file(filesystem_t *fs,
             const char *path,
             char *buffer,
             size_t size,
             off_t offset)
{
	if (!fs || !fs->initialized || !buffer)
        return -EINVAL;

    int idx = fs_find_entry(fs, path);
    if (idx < 0)
        return -ENOENT;

    fs_entry_t *entry = &fs->entries[idx];

    if (entry->type != ENTRY_FILE)
        return -EISDIR;

    entry->atime = time(NULL);

    if (!entry->data || entry->size == 0 || offset >= entry->size)
        return 0;

    size_t available = entry->size - offset;

    size_t to_read = (size < available) ? size : available;

    memcpy(buffer, entry->data + offset, to_read);

    return (int)to_read;
}

int
fs_write_file(filesystem_t *fs,
              const char *path,
              const char *buffer,
              size_t size,
              off_t offset)
{
	if (!fs || !fs->initialized || !buffer)
		return -EINVAL;
	
	int idx = fs_find_entry(fs, path);
	if (idx < 0)
		return -ENOENT;
	
	fs_entry_t *entry = &fs->entries[idx];
	
	if (entry->type != ENTRY_FILE)
		return -EISDIR;
	
	// Your code here: Escribir archivo
	// 1. Calcular new_size = offset + size
	// 2. Verificar que new_size no exceda MAX_FILE_SIZE
	// 3. Si new_size > entry->size:
	//    a. Verificar espacio disponible (fs->used_space + diferencia <= MAX_FS_SIZE)
	//    b. Realocar memoria con realloc()
	//    c. Si offset > entry->size, llenar hueco con memset(..., 0, ...)
	//    d. Actualizar fs->used_space
	//    e. Actualizar entry->size
	// 4. Copiar datos con memcpy(entry->data + offset, buffer, size)
	// 5. Actualizar entry->atime y entry->mtime
	// 6. Retornar número de bytes escritos
	
	return 0;
}

int
fs_truncate_file(filesystem_t *fs, const char *path, off_t size)
{
	if (!fs || !fs->initialized)
		return -EINVAL;
	
	int idx = fs_find_entry(fs, path);
	if (idx < 0)
		return -ENOENT;
	
	// Your code here: Truncar archivo
	// 1. Verificar que sea un archivo
	// 2. Verificar que size no exceda MAX_FILE_SIZE
	// 3. Si size == entry->size, retornar 0
	// 4. Si size > entry->size, verificar espacio disponible
	// 5. Realocar memoria con realloc()
	// 6. Si agrandamos, llenar nueva parte con zeros
	// 7. Actualizar fs->used_space (sumar o restar según corresponda)
	// 8. Actualizar entry->size, entry->mtime, entry->ctime
	
	return 0;
}

int
fs_unlink_file(filesystem_t *fs, const char *path)
{
	if (!fs || !fs->initialized)
		return -EINVAL;
	
	int idx = fs_find_entry(fs, path);
	if (idx < 0)
		return -ENOENT;
	
	// Your code here: Borrar archivo
	// 1. Verificar que sea un archivo (no directorio)
	// 2. Liberar entry->data si no es NULL
	// 3. Actualizar fs->used_space (restar entry->size)
	// 4. Mover última entrada a este hueco (si no es la última)
	// 5. Actualizar parent_idx de entradas que apuntaban a la última
	// 6. Decrementar fs->entry_count
	
	return 0;
}

// ============================================================================
// OPERACIONES DE DIRECTORIOS
// ============================================================================

int
fs_create_dir(filesystem_t *fs, const char *path, mode_t mode)
{
	if (!fs || !fs->initialized || !path)
		return -EINVAL;

	// sin lugar
	if (fs->entry_count >= MAX_FILES)
		return -ENOSPC;

	if (fs_find_entry(fs, path) >= 0)
		return -EEXIST;

	char parent[MAX_PATH];
	char name[MAX_FILENAME];
	fs_split_path(path, parent, name);

	//no permitir crear la raiz ni nombres vacios
	if (strlen(name) == 0)
		return -EINVAL;

	//el padre debe existir
	int parent_idx = fs_find_entry(fs, parent);
	if (parent_idx < 0)
		return -ENOENT;

	fs_entry_t *p = &fs->entries[parent_idx];
	if (p->type != ENTRY_DIR)
		return -ENOTDIR;

	int idx = fs->entry_count;
	fs_entry_t *entry = &fs->entries[idx];
	memset(entry, 0, sizeof(fs_entry_t));

	strncpy(entry->path, path, MAX_PATH - 1);
	entry->path[MAX_PATH - 1] = '\0';
	strncpy(entry->name, name, MAX_FILENAME - 1);
	entry->name[MAX_FILENAME - 1] = '\0';

	entry->type = ENTRY_DIR;
	entry->uid  = getuid();
	entry->gid  = getgid();
	entry->mode = S_IFDIR | (mode & 0777);

	entry->size = 0;
	entry->nlink = 2;    
	entry->parent_idx = parent_idx;
	entry->data = NULL;          //revisar

	time_t now = time(NULL);
	entry->atime = now;
	entry->mtime = now;
	entry->ctime = now;

	p->nlink++;

	fs->entry_count++;

	return 0;
}

int
fs_remove_dir(filesystem_t *fs, const char *path)
{
	if (!fs || !fs->initialized)
		return -EINVAL;
	
	int idx = fs_find_entry(fs, path);
	if (idx < 0)
		return -ENOENT;
	
	// Your code here: Borrar directorio
	// 1. Verificar que sea un directorio
	// 2. Verificar que esté vacío (ninguna entrada con parent_idx == idx)
	//    Si no está vacío, retornar -ENOTEMPTY
	// 3. Decrementar nlink del padre
	// 4. Mover última entrada a este hueco
	// 5. Actualizar parent_idx de entradas que apuntaban a la última
	// 6. Decrementar fs->entry_count
	
	return 0;
}

int
fs_read_dir(filesystem_t *fs, const char *path, char ***entries, int *count)
{
	if (!fs || !fs->initialized || !entries || !count)
		return -EINVAL;
	
	int idx = fs_find_entry(fs, path);
	if (idx < 0)
		return -ENOENT;
	
	// Your code here: Listar directorio
	// 1. Verificar que sea un directorio
	// 2. Contar cuántas entradas tienen parent_idx == idx
	// 3. Alocar array de strings con malloc(num_entries * sizeof(char*))
	// 4. Para cada entrada hija, hacer strdup(entry->name) y agregarlo al array
	// 5. Manejar errores de memoria (liberar todo si falla)
	// 6. Asignar *entries = array y *count = num_entries
	// 7. Retornar 0
	
	*count = 0;
	*entries = NULL;
	
	return 0;
}

// ============================================================================
// OPERACIONES GENERALES
// ============================================================================

int
fs_getattr(filesystem_t *fs, const char *path, struct stat *st)
{
	if (!fs || !fs->initialized || !st)
		return -EINVAL;
	
	int idx = fs_find_entry(fs, path);
	if (idx < 0)
		return -ENOENT;
	
	fs_entry_t *entry = &fs->entries[idx];
	
	// Your code here: Llenar struct stat
	// Copiar: uid, gid, mode, size, atime, mtime, ctime, nlink
	memset(st, 0, sizeof(struct stat));
	
	return 0;
}

int
fs_update_times(filesystem_t *fs, const char *path, const struct timespec tv[2])
{
	if (!fs || !fs->initialized)
		return -EINVAL;
	
	int idx = fs_find_entry(fs, path);
	if (idx < 0)
		return -ENOENT;
	
	// Your code here: Actualizar timestamps
	// tv[0] = access time, tv[1] = modification time
	// Si tv es NULL, usar time(NULL) para ambos
	// Siempre actualizar ctime = time(NULL)
	
	return 0;
}