#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#pragma pack(1)

typedef struct{
    uint8_t jmp[3];
    char name[8];
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t number_of_fats;
    uint16_t root_dir_entries;
    uint16_t sectors_count;
    uint8_t media_descriptor;
    uint16_t sectors_per_fat;
    uint16_t sectors_per_track;
    uint16_t heads_count;
    uint8_t trash[26];
    char type[5];
}boot;

typedef struct{
    char filename[8];
    char extension[3];
    uint8_t attributes;
    uint16_t reserved;
    uint16_t creation_time;
    uint16_t creation_date;
    uint16_t last_access_date;
    uint16_t trash;
    uint16_t last_write_time;
    uint16_t last_write_date;
    uint16_t source_cluster;
    uint32_t file_size;
}fileInfo;

int readFolder(FILE* disk, boot bpb, fileInfo folder, char cur_fold_name[100]){
    int files_count = 2;
    int root_offset = (bpb.reserved_sectors + bpb.sectors_per_fat * bpb.number_of_fats) * bpb.bytes_per_sector;
    int first_cluster_offset = root_offset + bpb.root_dir_entries * 32;
    
    int fat_offset = bpb.reserved_sectors * bpb.bytes_per_sector;
    mkdir(cur_fold_name, S_IRWXU | S_IRWXG | S_IRWXO);

    int current_chain_len = 0;
    uint16_t clusters_chain[2000];
    for(int i = 0; i < 2000; i++){
        clusters_chain[i] = 0;
    }
        //создание цепочки кластеров
    fseek(disk, fat_offset, SEEK_SET);
    int i = 0;
    clusters_chain[i] = folder.source_cluster;
    current_chain_len++;
        while(1){
            uint16_t tmp;
            int clust_offset = clusters_chain[i]*2;
            fseek(disk, fat_offset + clust_offset, SEEK_SET);
            fread(&tmp, 2, 1, disk);
            if(tmp >= 0xFFF8){
                break;
            }
            clusters_chain[++i] = tmp;
            current_chain_len++;
        }

    int current_clast_number = 0;


    for(int h = 0; h < current_chain_len; h++){
    int folder_cluster_offset = first_cluster_offset + (clusters_chain[current_clast_number] - 2)*bpb.sectors_per_cluster*bpb.bytes_per_sector;
    files_count = 2;
    //
    while(files_count < bpb.sectors_per_cluster*bpb.bytes_per_sector/32){
        fileInfo file;
        fseek(disk, folder_cluster_offset + 32*files_count, SEEK_SET);
        fread(&file, sizeof(folder), 1, disk);
        rewind(disk);

        if(file.filename[0] == -27){
            files_count++;
            continue;
        }
        if(file.attributes == 16){
            char name[9];
            for(int i = 0; i < 9; i++){
            name[i] = '\0';
            }
            for(int j = 0; j < 8; j++){
                if(file.filename[j] == ' '){ break; }
                name[j] = file.filename[j];
            }
            char newPath[100];
            strcpy(newPath, cur_fold_name);
            strcat(newPath, "/");
            strcat(newPath, name);
            readFolder(disk, bpb, file, newPath);
            files_count++;
            continue;
        }

        if(file.file_size < 1){
            break;
        }

        int current_chain_len = 0;

        uint16_t clusters_chain[2000];
        for(int i = 0; i < 2000; i++){
            clusters_chain[i] = 0;
        }
        //создание цепочки кластеров
        fseek(disk, fat_offset, SEEK_SET);
        int i = 0;
        clusters_chain[i] = file.source_cluster;
        current_chain_len++;
        while(1){
            uint16_t tmp;
            int clust_offset = clusters_chain[i]*2;
            fseek(disk, fat_offset + clust_offset, SEEK_SET);
            fread(&tmp, 2, 1, disk);
            if(tmp >= 0xFFF8){
                break;
            }
            clusters_chain[++i] = tmp;
            current_chain_len++;
        }

        char name[13];
        for(int i = 0; i < 13; i++){
            name[i] = '\0';
        }
        int j = 0;
        for(j; j < 8; j++){
            if(file.filename[j] == ' '){ break; }
            name[j] = file.filename[j];
        }
        if(strcmp(file.extension, "   ")){
        	name[j] = '.';
        	j++;
        }
        int k = 0;
        for(j; j < 12; j++){
            if(file.extension[k] == ' '){ break; }
            name[j] = file.extension[k];
            k++;
        }
        char fold[100];
        strcpy(fold, cur_fold_name);
        strcat(fold, "/");
        strcat(fold, name);

        FILE* output = fopen(fold, "wb");
        if(!output){
            printf("Error! Try to add 'files' folder in current folder\n");
            fclose(output);
            return -1;
        }
        
        int act_clust = -1;
        int active_cluster_offset;
        for(int l = 0; l < file.file_size; l++){
            if(l % (bpb.bytes_per_sector * bpb.sectors_per_cluster) == 0){
                act_clust++;
                active_cluster_offset = first_cluster_offset + (clusters_chain[act_clust] - 2) * bpb.bytes_per_sector * bpb.sectors_per_cluster;
                fseek(disk, active_cluster_offset, SEEK_SET);
            }
            uint8_t tmp;
            fread(&tmp, 1, 1, disk);
            fwrite(&tmp, 1, 1, output);
        }
        fclose(output);
        files_count++;
    }
    current_clast_number++;
    }
}

int main(){
    char image_path[255];
    printf("Enter path to image: ");
    scanf("%s", &image_path);
    FILE* disk = fopen(image_path, "rb");
    if(!disk){
        printf("Error with opening image!\n");
        return -1;
    }

    boot bpb;
    fileInfo file;

    fread(&bpb, sizeof(bpb), 1, disk);
    rewind(disk);

    if(strcmp(bpb.type, "FAT16")){
        printf("Current image is not FAT16 Image\n");
        return -1;
    }

    int files_count = 0;
    int root_offset = (bpb.reserved_sectors + bpb.sectors_per_fat * bpb.number_of_fats) * bpb.bytes_per_sector;
    int first_cluster_offset = root_offset + bpb.root_dir_entries * 32;
    int fat_offset = bpb.reserved_sectors * bpb.bytes_per_sector;



    while(files_count < bpb.root_dir_entries){
        fseek(disk, root_offset + 32*files_count, SEEK_SET);
        fread(&file, sizeof(file), 1, disk);
        rewind(disk);

        if(file.filename[0] == -27){
            files_count++;
            continue;
        }
        if(file.attributes == 16){
            char name[9];
            for(int i = 0; i < 9; i++){
            name[i] = '\0';
            }
            for(int j = 0; j < 8; j++){
                if(file.filename[j] == ' '){ break; }
                name[j] = file.filename[j];
            }

            char path[100] = "files/";
            strcat(path, name); 
            readFolder(disk, bpb, file, path);
            files_count++;
            continue;
        }
        if(file.file_size < 1){
            break;
        }
        int current_chain_len = 0;

        uint16_t clusters_chain[2000];
        for(int i = 0; i < 2000; i++){
            clusters_chain[i] = 0;
        }
        //создание цепочки кластеров
        fseek(disk, fat_offset, SEEK_SET);
        int i = 0;
        clusters_chain[i] = file.source_cluster;
        current_chain_len++;
        while(1){
            uint16_t tmp;
            int clust_offset = clusters_chain[i] * 2;
            fseek(disk, fat_offset + clust_offset, SEEK_SET);
            fread(&tmp, 2, 1, disk);
            if(tmp >= 0xFFF8){
                break;
            }
            clusters_chain[++i] = tmp;
            current_chain_len++;
        }

        //создание имени
        char name[13];
        for(int i = 0; i < 13; i++){
            name[i] = '\0';
        }
        int j = 0;
        for(j; j < 8; j++){
            if(file.filename[j] == ' '){ break; }
            name[j] = file.filename[j];
        }
        if(strcmp(file.extension, "   ")){
        	name[j] = '.';
        	j++;
        }
        int k = 0;
        for(j; j < 12; j++){
            if(file.extension[k] == ' '){ break; }
            name[j] = file.extension[k];
            k++;
        }
        char folder[] = "files/";
        strcat(folder, name);

        FILE* output = fopen(folder, "wb");
        if(!output){
            printf("Error! Try to add 'files' folder in current folder\n");
            fclose(output);
            return -1;
        }

        int act_clust = -1;
        int active_cluster_offset;
        for(int l = 0; l < file.file_size; l++){
            if(l % (bpb.bytes_per_sector * bpb.sectors_per_cluster) == 0){
                act_clust++;
                active_cluster_offset = first_cluster_offset + (clusters_chain[act_clust] - 2) * bpb.bytes_per_sector * bpb.sectors_per_cluster;
                fseek(disk, active_cluster_offset, SEEK_SET);
            }
            uint8_t tmp;
            fread(&tmp, 1, 1, disk);
            fwrite(&tmp, 1, 1, output);
        }
        fclose(output);
        files_count++;
    }


    fclose(disk);
    return 0;
}
