#include <ctype.h>
#include <dirent.h>
#include <errno.h>
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

    if (!sized_struct_ensure_capacity(file, filesize + 1, sizeof(char))) {
        fprintf(stderr, "OOM while preparing buffer for %s\n", path);
        return false;
    }

    fp = fopen(path, "rb");
    if (!fp) {
        perror("fopen");
        return false;
    }

    errno = 0;
    n = fread(file->buff, sizeof(char), filesize, fp);
    if (errno != 0)
        return false;

    file->buff[n] = '\0';

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
size_t scss_format_write(const char *in, char *out)
{
    size_t written = 0;
    size_t indent_level = 0;

    for (; *in != '\0';) {
        if (*in == '\n') {
            if (out != NULL) *out++ = *in++;
            written++;
            for (; isspace(*in); in++);
            for (size_t i = 0; i < indent_level * 2; i++) {
                if (out != NULL) *out++ = ' ';
                written++;
            }
        } else {
            if (*in == '{') indent_level++;
            if (*in == '}') indent_level--;
            if (out != NULL) *out++ = *in;
            written++;
            in++;
        }
    }

    return written;
}

static
ssize_t apply_scss_formatter(resizable_array_t *file)
{
    size_t final_size = scss_format_write(file->buff, NULL);

    printf("size: %zu\n", final_size);

    if (!sized_struct_ensure_capacity(
            file, file->nmemb + final_size + 1, sizeof(char))) {
        fprintf(stderr, "OOM\n");
        return -1;
    }

    scss_format_write(file->buff, file->buff + file->nmemb + 1);

    file->capacity += final_size;
    return final_size;
}

static
void compute_print_diff(
    const char *path,
    const resizable_array_t *file, const resizable_array_t *vout)
{
    printf("[%s] ", path);
    printf("%zu -> %zu, %+zd bytes\n", file->nmemb, vout->nmemb,
        (ssize_t)(vout->nmemb - file->nmemb));
}

static
bool has_scss_ext(const char *name)
{
    const char *dot = strrchr(name, '.');

    return dot && strcmp(dot, ".scss") == 0;
}

void process_file(const char *path, resizable_array_t *file, off_t filesize)
{
    resizable_array_t vout;

    printf("=> %s\n", path);
    if (!has_scss_ext(path))
        return;
    if (!read_file_into_buffer(path, file, filesize)) {
        fprintf(stderr, "Failed to read %s\n", path);
        return;
    }

    vout.nmemb = apply_scss_formatter(file);
    vout.buff = file->buff + file->nmemb + 1;
    vout.capacity = -1; // should not be used anyway

    compute_print_diff(path, file, &vout);

    printf("%.*s", (int)vout.nmemb, vout.buff);
    if (!write_to_file(path, &vout))
        fprintf(stderr, "Failed to write %s\n", path);
}

static
void traverse(const char *base, resizable_array_t *file)
{
    static char build_path[PATH_MAX];

    struct dirent *entry;
    size_t base_len = strlen(base);
    DIR *dir = opendir(base);
    struct stat st;

    if (dir == NULL) {
        perror("opendir");
        return;
    }

    if (base != build_path) {
        strncpy(build_path, base, PATH_MAX - 1);
        build_path[PATH_MAX - 1] = '\0';
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0
            || strcmp(entry->d_name, "..") == 0)
            continue;

        snprintf(build_path + base_len, PATH_MAX - base_len,
            "/%s", entry->d_name);

        if (stat(build_path, &st) == -1) {
            perror("stat");
            continue;
        }

        if (S_ISDIR(st.st_mode))
            traverse(build_path, file);
        else
            process_file(build_path, file, st.st_size);
        build_path[base_len] = '\0';
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
