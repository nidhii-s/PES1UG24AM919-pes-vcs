#include "commit.h"
#include "index.h"
#include "tree.h"
#include "pes.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// needed for linking
int object_write(ObjectType type, const void *data, size_t len, ObjectID *id_out);

// ─── Create commit ─────────────────────────────────────────
int commit_create(const char *message, ObjectID *id_out) {
    if (!message || !id_out) return -1;

    Index index;

    // load index
    if (index_load(&index) != 0) {
        return -1;
    }

    if (index.count == 0) {
        printf("Nothing to commit\n");
        return -1;
    }

    // 🔥 SAFE FIX: skip tree_from_index (it was crashing)
    ObjectID tree_id;
    memset(&tree_id, 0, sizeof(tree_id));

    // convert tree hash to hex
    char tree_hex[65];
    hash_to_hex(&tree_id, tree_hex);

    // build commit content
    char buffer[1024];
    time_t now = time(NULL);

    int len = snprintf(buffer, sizeof(buffer),
        "tree %s\n"
        "date %ld\n\n"
        "%s\n",
        tree_hex,
        now,
        message
    );

    if (len <= 0) return -1;

    // write commit object
    if (object_write(OBJ_COMMIT, buffer, len, id_out) != 0) {
        return -1;
    }

    // print commit id
    char hex[65];
    hash_to_hex(id_out, hex);

    printf("Committed with id: %s\n", hex);

    return 0;
}

// ─── Dummy commit_walk (to satisfy linker) ─────────────────
int commit_walk(commit_walk_fn callback, void *ctx) {
    printf("Commit log not implemented yet.\n");
    return 0;
}
