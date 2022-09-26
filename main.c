#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#define ARG_SIZE 100

typedef struct FILE_BUFFER_T{
    char* buffer;
    uint64_t length;
} filebuffer_t;



filebuffer_t* open_file(const char* filename, const char* mode){
    FILE* file = fopen(filename, mode);
    filebuffer_t* filebuffer = malloc(sizeof *filebuffer);
    *filebuffer = (filebuffer_t){ 0 };

    if(file == NULL){
        printf("[err: %d] %s\n", errno, strerror(errno));
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    filebuffer->length = ftell(file);
    fseek(file, 0, SEEK_SET);
    filebuffer->buffer = malloc(filebuffer->length * sizeof *filebuffer->buffer);

    if(filebuffer->buffer == NULL){
        printf("[err: %d] %s\n", errno, strerror(errno));
        return NULL;
    }

    fread(filebuffer->buffer, 1, filebuffer->length, file);
    fclose(file);

    return filebuffer;
}

int main(int argc, char const *argv[])
{
    size_t bytes = (argc - 1) * sizeof(char*);
    char** aux = malloc((argc - 1) * sizeof(char*));
    memset(aux, 0, bytes);

    char* filename = NULL;
    for(int i = 1; i < argc; i++){
        int aux_pos = i - 1;

        aux[aux_pos] = malloc(ARG_SIZE * sizeof(char));
        strcpy(aux[aux_pos], argv[i]);

        char* flag = strtok(aux[aux_pos], " ");
        char* value = strtok(NULL, " ");

        if(strcmp(flag, "-l") == 0) filename = value;
    }
    

    filebuffer_t* mbr_buffer = open_file(filename, "rb");
    if(mbr_buffer == NULL) return 1;

    for(int i = 0; i < argc - 1; i++) free(aux[i]);
    free(aux);
    free(mbr_buffer->buffer);
    free(mbr_buffer);

    return 0;
}