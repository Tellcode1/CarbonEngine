/*
    you: uSe bOoST pOG opTiOnSSS!!! iT"s sO sMinPle!!!!
    me: no.
*/

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

#define false 0
#define true 1
typedef unsigned char bool;

#define SHADER_COMPILER "glslangValidator -V "
void compile_shader(const char *buffer) {
    char temp[512];
    memset(temp, 0, 512 * sizeof(char));

    strcat(temp, SHADER_COMPILER);
    strcat(temp, buffer);
    printf("Compiling shader with command: \"%s\"\n", temp);

    system(temp);
}

void compile_all_shaders(const char *compile_info_path) {
    FILE *file = fopen(compile_info_path, "r");
    assert(file);

    char buffer[512];

    while (fgets(buffer, sizeof(buffer), file)) {
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
        }

        compile_shader(buffer);
    }

    fclose(file);
}

enum err_t {
    ERR_NO_ERROR = 0,
    ERR_FILE_NOT_FOUND = -1,
    ERR_INVALID_ARGUEMENTS = -2
};

void assert_shader_is_valid(const char *shader) {
    FILE *shader_file = fopen(shader, "r");
    if (!shader_file) {
        printf("Shader file \"%s\" not found. Things you should check:\nDid you pass in some other arguement after --append?\nIs the path to the shader correct?\n", shader);
        exit(ERR_FILE_NOT_FOUND);
    }
    fclose(shader_file);
}

int str_exists_in_file(FILE *fp, const char *str) {
	char temp[512];
	
	while(fgets(temp, 512, fp) != NULL) {
		if((strstr(temp, str)) != NULL) {
			return true;
		}
	}

   	return false;
}

void append_shader_to_cache(const char *compile_info_path, const char *shader, const char *stage, const char *output) {
    assert_shader_is_valid(shader);

    FILE *file = fopen(compile_info_path, "a+");
    assert(file);

    if (str_exists_in_file(file, shader)) {
        printf("Shader already exists in cache. Doing nothing.\n");
        fclose(file);
        exit(0);
    }

    fprintf(file, " %s -S %s -o %s\n", shader, stage, output);

    fclose(file);
}

// bool was_shader_modified(const char *shader) {
//     struct stat attrs;
//     if(stat(shader, &attrs) == 0)
//     {
//         time_t mod_time = attrs.st_mtime;
//         printf("%li \n", mod_time);
//     }
//     return 1;
// }

void printcache(const char *compile_info_path) {
    FILE *file = fopen(compile_info_path, "r");

    char ch;
    while ((ch = fgetc(file)) != EOF)
        putchar(ch);

    fclose(file);
}
void clear_cache() {
    FILE *file = fopen("shaders.cache", "w");
    fclose(file);
}

#define HELP_STR \
"Carbon Shader Manager version 0.1\n\
    (By default, the first arguement, Required) >> specify compile info file to use\n\
    --help   || -h || --h || no arguements >> print this\n\
    --append || -a || --a >> append shader to cache. This command has the following syntax : ./shader_manager --append <Shader Path> <Shader Stage> <Output> < must be in that order <Extra compile options (implement)>\n\
    --remove || -r || --r >> remove shader from cache. The arguement after this must be the path to the shader\n\
    --print  || -p || --p >> print all shader paths in cache\n\
    --compile|| -c || --c >> compile the shaders that have been changed since last ran\n\
    --force  || -f || --f >> force compile all the shaders in cache\n\
    --clear  || -c || --c >> clear the cache\n\
done."

#define ARG_EQUAL(str) (strcmp(argv[i], str) == 0)

int main(int argc, char *argv[]) {
    if (argc == 1) {
        printf("%s\n", HELP_STR);
        return 0;
    }
    const char *compile_info_path = argv[1];
    for (int i = 2; i < argc; i++)
    {
        if (ARG_EQUAL("-h") || ARG_EQUAL("--help") || ARG_EQUAL("--h")) {
            printf("%s\n", HELP_STR);
            return 0;
        }
        else if (ARG_EQUAL("--append") || ARG_EQUAL("-a") || ARG_EQUAL("--a")) {
            i++;

            const char *output = "";
            const char *stage = "";
            for (int j = i+1; j < argc; j++) {
                if ((strcmp(argv[j], "-o") == 0)) {
                    if ((j + 1) >= argc) {
                        printf("Expected output file. Got nothing\n");
                        return ERR_INVALID_ARGUEMENTS;
                    }
                    output = argv[j + 1];
                }
                else if (strcmp(argv[j], "-S") == 0) {
                    if ((j + 1) >= argc) {
                        printf("Expected shader stage. Got nothing\n");
                        return ERR_INVALID_ARGUEMENTS;
                    }
                    stage = argv[j + 1];
                }
            }
            if (strcmp(output, "") == 0) {
                printf("No output directory?\n");
                return ERR_INVALID_ARGUEMENTS;
            }
            if (strcmp(stage, "") == 0) {
                printf("No shader stage?\n");
                return ERR_INVALID_ARGUEMENTS;
            }

            append_shader_to_cache(compile_info_path, argv[i], stage, output);
            return 0;
        }
        else if (ARG_EQUAL("--print") || ARG_EQUAL("-p") || ARG_EQUAL("--p")) {
            printcache(compile_info_path);
            return 0;
        }
        else if (ARG_EQUAL("--compile") || ARG_EQUAL("-c") || ARG_EQUAL("--c")) {
            compile_all_shaders(compile_info_path);
            return 0;
        }
        else if (ARG_EQUAL("--clear") || ARG_EQUAL("-cl") || ARG_EQUAL("--cl")) {
            printf("Are you sure about that? [y/n]: ");
            if (getc(stdin) == 'y') {
                clear_cache();
            }
            return 0;
        }
    }
}