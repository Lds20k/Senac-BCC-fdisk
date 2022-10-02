#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

#define SECTOR_SIZE 512
#define DISK_INFO_SIZE 72
#define DISK_INFO (SECTOR_SIZE - DISK_INFO_SIZE)
#define ARG_SIZE 100
#define BOOT 0x80
#define BOOT_SIGNATURE 70


typedef struct buffer_s{
    uint8_t* value;
    uint64_t length;
} buffer_t;


typedef struct partition_s{
    char* device;
    uint8_t* status;
    uint8_t* chs_start;
    uint8_t* type;
    uint8_t* chs_end;
    uint32_t* lba_start;
    uint32_t* sectors;
} partition_t;


typedef struct partition_lenght_s{
    uint32_t device;
    uint32_t boot;
    uint32_t start;
    uint32_t end;
    uint32_t sectors;
    uint32_t size;
    uint32_t id;
} partition_lenght_t;


typedef enum information_measurement_unit_e{
    B,
    KiB,
    MiB,
    GiB
} unit_t;


partition_t create_partition_t(uint8_t* partition_entry, char* device){
    partition_t partition = (partition_t){ 0 };
    uint8_t* aux = partition_entry;

    partition.device = device;
    partition.status = aux++;
    partition.chs_start = aux;
    aux += 3;
    partition.type = aux++;
    partition.chs_end = aux;
    aux += 3;
    partition.lba_start = (uint32_t*) aux;
    aux += 4;
    partition.sectors = (uint32_t*) aux;

    return partition;
}


static inline uint32_t calculate_end_partition_t(partition_t* partition){
    return *partition->sectors + *partition->lba_start - 1;
}


uint64_t convert_sector_bytes(uint64_t sectors){
    return sectors * SECTOR_SIZE;
}


double_t convert_sector(uint64_t sectors, unit_t unit){
    if(unit == B)
        return convert_sector_bytes(sectors);
    
    return convert_sector(sectors, --unit) / 1024.f;
}


uint8_t get_unit(uint64_t bytes){
    if(trunc(bytes / pow(1024, 3)) != 0) return GiB;
    if(trunc(bytes / pow(1024, 2)) != 0) return MiB;
    if(trunc(bytes / 1024) != 0) return KiB;
    return B;
}


double_t convert_unit(uint64_t bytes, unit_t unit){
    switch (unit)
    {
    case GiB:
        return bytes / pow(1024, 3);
    case MiB:
        return bytes / pow(1024, 2);
    case KiB:
        return bytes / 1024;
    default:
        return bytes;
    }
}


char* get_unit_name(unit_t unit){
    switch (unit)
    {
    case GiB:
        return "GiB";
    case MiB:
        return "MiB";
    case KiB:
        return "KiB";
    default:
        return "B";
    }
}


partition_lenght_t create_partition_lenght_t(partition_t* partition){
    partition_lenght_t lenght = {
        .device = strlen(partition->device),
        .boot = 4,
        .start = 5,
        .end = 3,
        .sectors = 7,
        .size = 3,
        .id = 2
    };
    
    for (size_t i = 0; i < 4; i++)
    {
        // Start
        char start_string[11] = "";
        sprintf(start_string, "%d", *partition[i].lba_start);
        uint32_t start_lenght = strlen(start_string);
        if(start_lenght > lenght.start)
            lenght.start = start_lenght;
        
        // End
        uint32_t end = calculate_end_partition_t(&partition[i]);
        char end_string[11] = "";
        sprintf(end_string, "%d", end);
        uint32_t end_lenght = strlen(end_string);
        if(end_lenght > lenght.end)
            lenght.end = end_lenght;
        
        // Sectors
        char sectors_string[11] = "";
        sprintf(sectors_string, "%d", *partition[i].sectors);
        uint32_t sectors_lenght = strlen(sectors_string);
        if(sectors_lenght > lenght.sectors)
            lenght.sectors = sectors_lenght;
        
        // Size
        double_t size = round(convert_sector(*partition[i].sectors, GiB) * 10) / 10;
        char size_string[28] = "";
        sprintf(size_string, "%g", size);
        uint32_t size_lenght = strlen(size_string);
        if(size_lenght > lenght.size)
            lenght.size = size_lenght;
    }
    return lenght;
}


char* get_partition_type_name(uint8_t type){
    switch (type)
    {
    case 0x82:
        return "Linux swap / Solaris\0";
    case 0x83:
        return "Linux\0";
    default:
        return "Not mapped";
    }
}


void print_partition_t(partition_t* partition, partition_lenght_t lenght){
    char* boot =  *partition->status == BOOT ? "*" : "";
    uint32_t partition_end = calculate_end_partition_t(partition);
    double_t size = round(convert_sector(*partition->sectors, GiB) * 10) / 10;
    char* partition_type = get_partition_type_name(*partition->type);

    printf("%-*s ", lenght.device,   partition->device);
    printf("%*s ",  lenght.boot,     boot);
    printf("%*d ",  lenght.start,    *partition->lba_start);
    printf("%*d ",  lenght.end,      partition_end);
    printf("%*d ",  lenght.sectors,  *partition->sectors);
    printf("%*gG ", lenght.size,     size);
    printf("%*x ",  lenght.id,       *partition->type);
    printf("%s",  partition_type);
    printf("\n");
}


void print_partition_title(partition_lenght_t lenght){
    printf("\033[1m"); // Bold Start
    printf("%-*s ", lenght.device, "Device");
    printf("%*s ",  lenght.boot, "Boot");
    printf("%*s ",  lenght.start, "Start");
    printf("%*s ",  lenght.end, "End");
    printf("%*s ",  lenght.sectors, "Sectors");
    printf(" %*s ", lenght.size, "Size");
    printf("%*s ",  lenght.id, "Id");
    printf("%s", "Type");
    printf("\033[m\n"); // Bold End
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
    filebuffer->value = malloc(DISK_INFO_SIZE);

    if(filebuffer->value == NULL){
        printf("[err: %d] %s\n", errno, strerror(errno));
        return NULL;
    }

    fseek(file, DISK_INFO, SEEK_SET);
    fread(filebuffer->value, 1, filebuffer->length, file);
    fclose(file);

    return filebuffer;
}


int main(int argc, char const *argv[])
{
    size_t bytes = (argc - 1) * sizeof(char*);
    char** aux;
    if((aux = malloc((argc - 1) * sizeof(char*))) == NULL){
        printf("Not enough memory");
        return -1;
    }
    memset(aux, 0, bytes);
    
    // Check args
    char* filename = NULL;
    bool flag_l = false;
    for(size_t i = 1; i < (size_t) argc; i++){
        size_t aux_pos = i - 1;

        if((aux[aux_pos] = malloc(ARG_SIZE * sizeof(char))) == NULL){
            printf("Not enough memory");
            for (size_t j = 0; j < aux_pos; j++)
            {
                free(aux[j]);
            }
            free(aux);
            return -1;
        }
        strcpy(aux[aux_pos], argv[i]);

        if(flag_l) filename = aux[aux_pos];
        flag_l = strcmp(aux[aux_pos], "-l") == 0;
    }

    // MBR open file
    buffer_t* mbr_buffer = open_file(filename, "rb");
    if(mbr_buffer == NULL) {
        printf("Not enough memory");
        for(size_t i = 0; i < (size_t) argc - 1; i++) free(aux[i]);
        free(aux);
        return 1;
    }
    
    // Check if is valid MBR
    if(
        (uint8_t) mbr_buffer->value[  BOOT_SIGNATURE  ] != 0x55 ||
        (uint8_t) mbr_buffer->value[BOOT_SIGNATURE + 1] != 0xAA
    ){
        printf("Invalid bootsector!\n");
        return 1;
    }

    // Create 4 partition structure

    // Filename copy
    bytes = strlen(filename) * sizeof(char) + 2;
    char* filename_aux;
    uint32_t disk_identifier = *(uint32_t*) mbr_buffer->value;
    partition_t partition[4];
    for(size_t i = 0; i < 4; i++){
        char buffer[sizeof i];
        sprintf(buffer, "%li", i + 1);

        filename_aux = malloc(bytes);
        memset(filename_aux, 0, bytes);
        strcpy(filename_aux, filename);
        filename_aux[bytes-2] = buffer[0];

        partition[i] = create_partition_t(&mbr_buffer->value[6 + 16 * i], filename_aux);
    }
    partition_lenght_t lenght = create_partition_lenght_t(partition);

    // Disk information
    uint8_t unit = get_unit(mbr_buffer->length);
    double_t size = convert_unit(mbr_buffer->length, unit);
    size = round(size * 100) / 100;
    char* unit_name = get_unit_name(unit);
    uint64_t sectors = mbr_buffer->length / SECTOR_SIZE;

    // Print headers
    printf("\033[1m"); // Bold Start
    printf("Disk %s: %g %s, %li bytes, %li sectors", filename, size, unit_name, mbr_buffer->length, sectors);
    printf("\033[m\n"); // Bold End
    printf("Units: sectors of 1 * 512 = 512 bytes\n");
    printf("Sector size (logical/physical): 512 bytes / 512 bytes\n");
    printf("I/O size (minimum/optimal): 512 bytes / 512 bytes\n");
    printf("Disk identifier: %#x\n\n", disk_identifier);
    print_partition_title(lenght);

    // Print partition data
    for(size_t i = 0; i < 4; i++){
        if(*partition[i].lba_start != 0)
            print_partition_t(&partition[i], lenght);
    }

    // Free all allocated spaces
    for(size_t i = 0; i < 4; i++) free(partition[i].device);
    for(size_t i = 0; i < (size_t) argc - 1; i++) free(aux[i]);
    free(aux);
    free(mbr_buffer->value);
    free(mbr_buffer);

    return 0;
}
