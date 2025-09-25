#include <dirent.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define INITIAL_SIZE 64

typedef struct {
    char *buff;
    size_t nmemb;
    size_t capacity;
} resizable_array_t;


static
bool process_initial_alloc(resizable_array_t *arr, size_t objsize)
{
    arr->nmemb = 0;
    arr->capacity = INITIAL_SIZE;
    arr->buff = malloc(INITIAL_SIZE * objsize);
    return arr->buff != NULL;
}

static
bool sized_struct_ensure_capacity(
    resizable_array_t *arr, size_t requested, size_t objsize
)
{
    size_t endsize = INITIAL_SIZE;
    void *newp;

    if (arr->capacity == 0 && requested <= INITIAL_SIZE)
        return process_initial_alloc(arr, objsize);
    if ((arr->nmemb + requested) < arr->capacity)
        return true;
    for (; endsize < arr->nmemb + requested; endsize <<= 1);
    if (endsize < arr->capacity)
        return true;
    newp = realloc(arr->buff, objsize * endsize);
    if (newp == NULL)
        return false;
    arr->buff = newp;
    arr->capacity = endsize;
    return true;
}

void traverse(const char *base)
{
    static char build_path[PATH_MAX];

    struct dirent *entry;
    size_t base_len = strlen(base);
    DIR *dir = opendir(base);

    if (dir == NULL) {
        perror("opendir");
        return;
    }

    strncpy(build_path, base, PATH_MAX - 1);
    build_path[PATH_MAX - 1] = '\0';

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0
            || strcmp(entry->d_name, "..") == 0)
            continue;

        snprintf(build_path + base_len, PATH_MAX - base_len,
            "/%s", entry->d_name);

        struct stat st;
        if (stat(build_path, &st) == -1) {
            perror("stat");
            continue;
        }

        if (S_ISDIR(st.st_mode)) {
            printf("d : %s\n", build_path);
            traverse(build_path);
        } else {
            printf("f : %s\n", build_path);
        }

        build_path[base_len] = '\0';
    }

    closedir(dir);
}

int main(int argc, char *argv[])
{
    const char *start = (argc > 1) ? argv[1] : ".";

    traverse(start);
    return 0;
}
