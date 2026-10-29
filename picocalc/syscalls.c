/* syscalls.c — newlib system calls the Pico SDK does not provide.
 *
 * ScummVM's DefaultSaveFileManager deletes a save with remove(), which
 * newlib turns into _unlink(). It goes to the card through upstream's
 * FatFs layer, as every other file operation does (design.md §10).
 */

#include <errno.h>

#include "frank_quest_fs.h"

int _unlink(const char *path);

int _unlink(const char *path) {
    if (cabal_remove(path) == CABAL_FS_OK) {
        return 0;
    }
    errno = ENOENT;
    return -1;
}
