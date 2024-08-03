#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include <assert.h>
#include <time.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>

#ifndef WIN32
    #include <unistd.h>
#endif

#ifdef WIN32
    #define stat _stat
#endif

typedef unsigned char bool;

void append_shaders_to_cache(const char *shader) {
    time_t now = time(0);
    FILE *file = fopen("shaders.cache", "a");
    assert(file);

    fprintf(file, " %s %li\n", shader, now);

    fclose(file);
}

bool was_shader_modified(const char *shader) {
    struct stat attrs;
    if(stat(shader, &attrs) == 0)
    {
        time_t mod_time = attrs.st_mtime;
        printf("%li \n", mod_time);
    }
}

void printcache() {
    FILE *file = fopen("shaders.cache", "r");

    char ch;
    while ((ch = fgetc(file)) != EOF)
        putchar(ch);

    fclose(file);
}

typedef enum {
    ARG_INT,
    ARG_FLOAT,
    ARG_STRING
} arg_tp;

void process_args(int argc, ...) {
    va_list args;
    va_start(args, argc);

    for (int i = 0; i < argc; i++) {
        arg_tp type = va_arg(args, arg_tp);
        switch (type) {
            case ARG_INT: {
                int int_val = va_arg(args, int);
                printf("Integer: %d\n", int_val);
                break;
            }
            case ARG_FLOAT: {
                float float_val = va_arg(args, double);
                printf("Float: %f\n", float_val);
                break;
            }
            case ARG_STRING: {
                char *str_val = va_arg(args, char*);
                
                if (strcmp(str_val, "clear") == 0) {
                    FILE *file = fopen("shaders.cache", "w");
                    fclose(file);
                    printf("Cleared.\n");
                    return;
                }
                if (strcmp(str_val, "print") == 0) {
                    printcache();
                    return;
                }

                break;
            }
            default:
                printf("Unknown type\n");
                break;
        }
    }

    va_end(args);
}

int main(int argc, char *argv[]) {
    for (int i = 0; i < argc; i++)
    {
        // for each argv, check if it matches an arg defined in this program or any of its alternates,
        // if true, like set a flag or something
        // just take it as append for now
    }

    process_args(
        2,
        ARG_STRING, argv[0],
        ARG_FLOAT, 69.420f
    );

    append_shaders_to_cache(argv[1]);
    was_shader_modified(argv[1]);
}