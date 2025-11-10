#ifndef PERSISTENCE_H
#define PERSISTENCE_H

#include "fs.h"

// Formato del archivo en disco:
// - Magic number (4 bytes): "FISO"
// - Version (4 bytes)
// - Entry count (4 bytes)
// - Used space (8 bytes)
// - Entries array (variable)
//   - Para cada entry:
//     - Metadatos (struct serializado)
//     - Data (si es archivo)

#define MAGIC_NUMBER "FISO"
#define FS_VERSION 1

// Serialización
int persistence_save(const char *filepath, const filesystem_t *fs);
int persistence_load(const char *filepath, filesystem_t *fs);

// Verificación
bool persistence_file_exists(const char *filepath);
bool persistence_is_valid(const char *filepath);

#endif  // PERSISTENCE_H