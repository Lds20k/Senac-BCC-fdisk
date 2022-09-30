#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

#define ARG_SIZE 100
#define BOOT 0x80


typedef struct buffer_s{
    char* value;
    uint64_t length;
} buffer_t;


typedef struct partition_s{
    char* device;
    uint8_t* status;
    uint8_t* address_partition_start;
    uint8_t* type;
    uint8_t* address_last_partition;
    uint32_t* lba_partition_start;
    uint32_t* number_sectors;
} partition_t;

typedef struct partition_lenght_s{
    uint32_t device;
    uint32_t status;
    uint32_t type;
    uint32_t lba_partition_start;
    uint32_t number_sectors;
} partition_lenght_t;


partition_t create_partition_t(uint8_t* partition_entry, char* device){
    partition_t partition = (partition_t){ 0 };
    uint8_t* aux = partition_entry;

    partition.device = device;
    partition.status = aux++;
    partition.address_partition_start = aux;
    aux += 3;
    partition.type = aux++;
    partition.address_last_partition = aux;
    aux += 3;
    partition.lba_partition_start = aux;
    aux += 4;
    partition.number_sectors = aux;

    return partition;
}


partition_lenght_t create_partition_lenght_t(partition_t* partition){
    partition_lenght_t lenght = (partition_lenght_t){ 0 };
    char lba_partition_start[10] = "";
    char number_sectors[10] = "";

    sprintf(lba_partition_start, "%d", *partition->lba_partition_start);
    sprintf(number_sectors, "%d", *partition->number_sectors);

    lenght.lba_partition_start = strlen(lba_partition_start) + 2;
    lenght.number_sectors = strlen(number_sectors) + 1;
    lenght.device = strlen(partition->device);
    lenght.status = 5;
    lenght.type = 3;

    return lenght;
}


char* get_partition_type_name(uint8_t type){
    switch (type)
    {
    case 0x82:
        return "Linux swap / Solaris\0";
    case 0x83:
        return "Linux\0";
    }
}


void print_partition_t(partition_t* partition, partition_lenght_t lenght){
    uint32_t partition_end = *partition->number_sectors + *partition->lba_partition_start - 1;
    uint64_t size_b = ((uint64_t)*partition->number_sectors) * 512;
    double_t size_kb = size_b / 1024.f;
    double_t size_mb = size_kb / 1024.f;
    double_t size_gb = size_mb / 1024.f;
    char* partition_type = get_partition_type_name(*partition->type);

    printf("%-*s", lenght.device, partition->device);
    printf("%*s", lenght.status, (*partition->status == BOOT) ? "*" : "");
    printf("%*d", lenght.lba_partition_start, *partition->lba_partition_start);
    printf("%*d", lenght.number_sectors, partition_end);
    printf("%*d", lenght.number_sectors, *partition->number_sectors);
    printf("%4s %gG", "", size_gb);
    printf("%*x", lenght.type, *partition->type);
    printf(" %s", partition_type);
    printf("\n");
}


void print_title(partition_lenght_t lenght){

    printf("%-*s", lenght.device, "Device");
    printf("%*s", lenght.status, "Boot");
    printf("%*s", lenght.lba_partition_start, "Start");
    printf("%*s", lenght.number_sectors, "End");
    printf("%*s", lenght.number_sectors, "Sectors");
    printf("%*s", 7, "Size");
    printf("%*s", lenght.type, "Id");
    printf(" %s", "Type");
    printf("\n");
}


buffer_t* open_file(const char* filename, const char* mode){
    FILE* file = fopen(filename, mode);
    buffer_t* filebuffer = malloc(sizeof *filebuffer);
    *filebuffer = (buffer_t){ 0 };

    if(file == NULL){
        printf("[err: %d] %s\n", errno, strerror(errno));
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    filebuffer->length = ftell(file);
    fseek(file, 0, SEEK_SET);
    filebuffer->value = malloc(filebuffer->length * sizeof *filebuffer->value);

    if(filebuffer->value == NULL){
        printf("[err: %d] %s\n", errno, strerror(errno));
        return NULL;
    }

    fread(filebuffer->value, 1, filebuffer->length, file);
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
    

    buffer_t* mbr_buffer = open_file(filename, "rb");
    if(mbr_buffer == NULL) return 1;
    
    if((uint8_t)mbr_buffer->value[mbr_buffer->length - 1] != 0xAA || mbr_buffer->value[mbr_buffer->length - 2] != 0x55){
        printf("Invalid bootsector\b");
        return 1;
    }

    uint32_t* disk_identifier = (uint32_t*) &mbr_buffer->value[440];
    partition_t partition[4];
    partition[0] = create_partition_t(&mbr_buffer->value[446], filename);
    partition[1] = create_partition_t(&mbr_buffer->value[462], filename);
    partition[2] = create_partition_t(&mbr_buffer->value[478], filename);
    partition[3] = create_partition_t(&mbr_buffer->value[494], filename);
    partition_lenght_t lenght = create_partition_lenght_t(&partition[1]);

    printf("Disk identifier: %#x\n\n", *disk_identifier);
    print_title(lenght);

    for(int i = 0; i < 4; i++){
        if(*partition[i].lba_partition_start != 0)
            print_partition_t(&partition[i], lenght);
    }

    for(int i = 0; i < argc - 1; i++) free(aux[i]);
    free(aux);
    free(mbr_buffer->value);
    free(mbr_buffer);

    return 0;
}