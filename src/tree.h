#ifndef FILETREE_H
    #define FILETREE_H

    #include <stdbool.h>

    #include "utils/resizable_array.h"

typedef struct {
    char **paths;
    size_t nmemb;
    size_t capacity;
} file_tree_t;

bool file_tree_init(file_tree_t *tree, const char *initial_path);

#endif /* !FILETREE_H */
