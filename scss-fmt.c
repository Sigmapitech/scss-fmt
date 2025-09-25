#include <dirent.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define INITIAL_SIZE (1UL * 32 * 1024)

typedef struct {
    char *buff;
    size_t nmemb;
    size_t capacity;
} resizable_array_t;

static char BUILD_PATH[PATH_MAX];

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

static
bool read_file_into_buffer(
    const char *path, resizable_array_t *file, off_t filesize)
{
    FILE *fp;
    size_t n;

    if (!sized_struct_ensure_capacity(file, filesize, sizeof(char))) {
        fprintf(stderr, "OOM while preparing buffer for %s\n", path);
        return false;
    }

    fp = fopen(path, "rb");
    if (!fp) {
        perror("fopen");
        return false;
    }

    n = fread(file->buff, sizeof(char), filesize, fp);
    fclose(fp);

    if (n != (size_t)filesize) {
        fprintf(stderr, "Short read on %s\n", path);
        return false;
    }

    file->nmemb = n;
    return true;
}

static
bool write_to_file(const char *path, resizable_array_t *file)
{
    size_t n;
    FILE *fp = fopen(path, "wb");

    if (fp == NULL)
        return false;
    n = fwrite(file->buff, sizeof(char), file->nmemb, fp);
    fclose(fp);
    return n == file->nmemb;
}

static
void apply_scss_formatter(resizable_array_t *buff)
{
    // noop
}

static
void compute_print_diff(const char *path, const resizable_array_t *file)
{
    printf("diff: %s\n", path);
    printf("%.*s", (int)(file->nmemb * sizeof(char)), file->buff);
}

static
bool has_scss_ext(const char *name)
{
    const char *dot = strrchr(name, '.');

    return dot && strcmp(dot, ".scss") == 0;
}

void process_file(const char *path, resizable_array_t *file, off_t filesize)
{
    if (!has_scss_ext(path))
        return;
    if (!read_file_into_buffer(path, file, filesize)) {
        fprintf(stderr, "Failed to read %s\n", path);
        return;
    }

    // both for now
    apply_scss_formatter(file);
    compute_print_diff(path, file);

    if (!write_to_file(path, file)) {
        fprintf(stderr, "Failed to write %s\n", path);
    }
}

static
void traverse(const char *base, resizable_array_t *file)
{
    struct dirent *entry;
    size_t base_len = strlen(base);
    DIR *dir = opendir(base);
    struct stat st;

    if (dir == NULL) {
        perror("opendir");
        return;
    }

    strncpy(BUILD_PATH, base, PATH_MAX - 1);
    BUILD_PATH[PATH_MAX - 1] = '\0';

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0
            || strcmp(entry->d_name, "..") == 0)
            continue;

        snprintf(BUILD_PATH + base_len, PATH_MAX - base_len,
            "/%s", entry->d_name);

        if (stat(BUILD_PATH, &st) == -1) {
            perror("stat");
            continue;
        }

        if (S_ISDIR(st.st_mode))
            traverse(BUILD_PATH, file);
        else
            process_file(BUILD_PATH, file, st.st_size);
        BUILD_PATH[base_len] = '\0';
    }

    closedir(dir);
}

int main(int argc, char *argv[])
{
    const char *start = (argc > 1) ? argv[1] : ".";
    resizable_array_t buffer = {0};

    traverse(start, &buffer);

    free(buffer.buff);
    return 0;
}
