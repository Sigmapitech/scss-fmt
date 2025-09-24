#include <stdio.h>

#include "tree.h"

int main(int argc, char *argv[])
{
    file_tree_t tree = {0};

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <directory>\n", argv[0]);
        return 1;
    }
    if (!file_tree_init(&tree, argv[1])) {
        fprintf(stderr, "Error exploring directory '%s'\n", argv[1]);
        return 1;
    }
    for (size_t i = 0; i < tree.nmemb; i++) {
        printf("%s\n", tree.paths[i]);
    }
    free((void *)tree.paths);

    return 0;
}
