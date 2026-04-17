#include "index.h"
#include "pes.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

// Needed for your setup
int object_write(ObjectType type, const void *data, size_t len, ObjectID *id_out);

// ─── Load index ─────────────────────────────────────────────
int index_load(Index *index) {
    if (!index) return -1;

    index->count = 0;

    FILE *f = fopen(".pes/index", "r");
    if (!f) return 0;

    while (index->count < MAX_INDEX_ENTRIES) {
        IndexEntry temp;
        char hash_hex[65];
        char path_buf[256];

        int ret = fscanf(f, "%o %64s %u %255s",
                         &temp.mode,
                         hash_hex,
                         &temp.size,
                         path_buf);

        if (ret != 4) break;

        hex_to_hash(hash_hex, &temp.hash);

        strncpy(temp.path, path_buf, sizeof(temp.path) - 1);
        temp.path[sizeof(temp.path) - 1] = '\0';

        index->entries[index->count++] = temp;
    }

    fclose(f);
    return 0;
}

// ─── Save index ─────────────────────────────────────────────
int index_save(const Index *index) {
    if (!index) return -1;

    FILE *f = fopen(".pes/index.tmp", "w");
    if (!f) return -1;

    for (int i = 0; i < index->count; i++) {
        const IndexEntry *e = &index->entries[i];

        char hash_hex[65];
        hash_to_hex(&e->hash, hash_hex);

        fprintf(f, "%o %s %u %s\n",
                e->mode,
                hash_hex,
                e->size,
                e->path);
    }

    fclose(f);
    rename(".pes/index.tmp", ".pes/index");
    return 0;
}

// ─── Add file ─────────────────────────────────────────────
int index_add(Index *index, const char *path) {
    if (!index || !path) return -1;

    struct stat st;
    if (stat(path, &st) != 0) return -1;

    FILE *f = fopen(path, "rb");
    if (!f) return -1;

    void *data = malloc(st.st_size);
    if (!data) {
        fclose(f);
        return -1;
    }

    size_t read_bytes = fread(data, 1, st.st_size, f);
    fclose(f);

    if (read_bytes != (size_t)st.st_size) {
        free(data);
        return -1;
    }

    ObjectID hash;
    if (object_write(OBJ_BLOB, data, st.st_size, &hash) != 0) {
        free(data);
        return -1;
    }

    free(data);

    // update existing
    for (int i = 0; i < index->count; i++) {
        if (strcmp(index->entries[i].path, path) == 0) {
            index->entries[i].hash = hash;
            index->entries[i].size = st.st_size;
            return 0;
        }
    }

    if (index->count >= MAX_INDEX_ENTRIES) return -1;

    IndexEntry *e = &index->entries[index->count++];

    e->mode = 0100644; // FIXED mode
    e->hash = hash;
    e->size = st.st_size;

    strncpy(e->path, path, sizeof(e->path) - 1);
    e->path[sizeof(e->path) - 1] = '\0';

    return 0;
}

// ─── Status ─────────────────────────────────────────────
int index_status(const Index *unused) {
    (void)unused; // ignore whatever pes.c passes

    FILE *f = fopen(".pes/index", "r");

    printf("Staged changes:\n");

    if (!f) {
        printf("  (nothing to show)\n");
        return 0;
    }

    char line[512];
    int empty = 1;

    while (fgets(line, sizeof(line), f)) {
        char mode[16], hash[65], size[32], path[256];

        if (sscanf(line, "%15s %64s %31s %255s", mode, hash, size, path) == 4) {
            printf("  staged:     %s\n", path);
            empty = 0;
        }
    }

    fclose(f);

    if (empty) {
        printf("  (nothing to show)\n");
    }

    return 0;
}
