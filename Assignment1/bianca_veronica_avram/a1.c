#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

int print_dir_content(char *path, bool namefilter, char *startswith, bool permex)
{
    DIR *dir = opendir(path);
    if(dir == NULL)
    {
        return -1;
    }
    printf("SUCCESS");
    struct dirent *entry;
    struct stat inode;
    while((entry = readdir(dir)) != NULL){
        if(strcmp(entry->d_name,".") != 0 && strcmp(entry->d_name,"..")){
            char *name = (char *)malloc(512);
            strncpy(name,path,strlen(path));
            strcat(name,"/");
            strcat(name,entry->d_name);
            lstat(name, &inode);
            free(name);
            if(namefilter == true){
                int position = entry->d_name - strstr(entry->d_name,startswith);
                if(position == 0){
                    if(permex == true){
                        if(inode.st_mode & S_IXUSR){
                            printf("\n%s/%s", path, entry->d_name);
                        }
                    }
                    else
                    {
                        printf("\n%s/%s", path, entry->d_name);
                    }
                }
            }
            else
            {
                if(permex == true){
                    if(inode.st_mode & S_IXUSR){
                        printf("\n%s/%s", path, entry->d_name);
                    }
                }
                else
                {
                    printf("\n%s/%s", path, entry->d_name);
                }
            }
            
        }
    }
    closedir(dir);
    return 0;
}

int print_dir_content_rec(char *path, bool namefilter, char *startswith, bool permex)
{
    DIR *dir = opendir(path);
    if(dir == NULL)
    {
        return -1;
    }
    struct dirent *entry;
    struct stat inode;
    
    while((entry = readdir(dir)) != NULL){
        char *name = (char *)malloc(512);
        strncpy(name, path, strlen(path));
        name[strlen(path)] = '\0';
        strcat(name, "/");
        strcat(name, entry->d_name);
        lstat(entry->d_name, &inode);
        if(strcmp(entry->d_name,".") != 0 && strcmp(entry->d_name,".."))
        {
            if(namefilter == true){
                int position = entry->d_name - strstr(entry->d_name,startswith);
                if(position == 0){
                    if(permex == true){
                        if(inode.st_mode & S_IXUSR){
                            printf("\n%s/%s", path, entry->d_name);
                        }
                    }
                    else{
                        printf("\n%s/%s", path, entry->d_name);
                    }
                }
                else{
                    if(permex == true){
                        if(inode.st_mode & S_IXUSR){
                            printf("\n%s/%s", path, entry->d_name);
                        }
                    }
                }
            }
            else{
                printf("\n%s/%s", path, entry->d_name);
            }
            if(S_ISDIR(inode.st_mode) && name[strlen(name) - 1] != '.'){
                print_dir_content_rec(name,namefilter,startswith,permex);
            }
        }
        free(name);
    }
    closedir(dir);
    return 0;
}

int parse(char *path){
    int fd;
    if((fd = open(path, O_RDONLY)) == -1){
        printf("File could not be opened");
        return -1;
    }
    char magic;
    int header_size;
    int version;
    char no_of_sections;
    lseek(fd,-1,SEEK_END);
    if(read(fd,&magic,1) != 1){
        return -1;
    }
    if(magic != 'b'){
        printf("ERROR\nwrong magic");
        return 0;
    }
    lseek(fd,-3,SEEK_END);
    if(read(fd,&header_size,2) != 2){
        return -1;
    }
    lseek(fd,-header_size,SEEK_END);
    if(read(fd,&version,4) != 4){
        return -1;
    }
    if(version < 97 || version > 177){
        printf("ERROR\nwrong version");
        return 0;
    }
    if(read(fd,&no_of_sections,1) != 1){
        return -1;
    }
    if(no_of_sections < 4 || no_of_sections > 12){
        printf("ERROR\nwrong sect_nr");
        return 0;
    }
    for(int i = 0 ; i < no_of_sections ; i++){
        lseek(fd,8,SEEK_CUR);
        short type;
        if(read(fd,&type,2) != 2){
            return -1;
        }
        if(type != 14 && type != 68 && type != 87 && type != 90 && type != 77){
            printf("ERROR\nwrong sect_types");
            return -1;
        }
        lseek(fd,8,SEEK_CUR);
    }
    lseek(fd,-no_of_sections*18,SEEK_CUR);
    printf("SUCCESS\n");
    printf("version=%d\n",version);
    printf("nr_sections=%d\n",no_of_sections);
    for(int i = 0 ; i < no_of_sections ; i++){
        printf("section%d: ",i+1);
        char name[8];
        if(read(fd,&name,8) != 8){
            return -1;
        }
        short type;
        if(read(fd,&type,2) != 2){
            return -1;
        }
        int offset;
        if(read(fd,&offset,4) != 4){
            return -1;
        }
        int size;
        if(read(fd,&size,4) != 4){
            return -1;
        }
        printf("%s %d %d\n",name,type,size);
    }
    return 1;
}

int extract(char *path, int section, int line){
    int fd = open(path,O_RDONLY);
    if(fd == -1){
        return -1;
    }
    lseek(fd,-3,SEEK_END);
    short header_size;
    if(read(fd,&header_size,2) != 2){
        return -2;
    }
    lseek(fd,-header_size,SEEK_END);
    lseek(fd,4,SEEK_CUR);
    char no_of_sections;
    if(read(fd,&no_of_sections,1) != 1){
        return -2;
    }
    int sec = (int)no_of_sections;
    if(section > sec){
        return 2;
    }
    //now go to the wanted section header and get size and offset
    int size;
    int offset;
    lseek(fd,(section - 1)*18,SEEK_CUR);
    lseek(fd,10,SEEK_CUR);
    if(read(fd,&offset,4) != 4){
        return -2;
    }
    if(read(fd,&size,4) != 4){
        return -2;
    }
    lseek(fd,offset,SEEK_SET);
    printf("SUCCESS\n");
    char *section_content = (char *)malloc(size);
    read(fd,section_content,size);
    section_content[size] = '\0';
    int i = strlen(section_content) - 1;
    while(line > 0 && i >= 0){
        if(line == 1 && section_content[i] != 0x0A){
            printf("%c",section_content[i]);
        }
        if(section_content[i] == 0x0A)
        {
            line--;
        }
        i--;
    }

    free(section_content);
    close(fd);
    return 1;
}

int isgood(char *path){
    //checks if the file is SF and has no section with size greater than 1192
    int fd;
    if((fd = open(path, O_RDONLY)) == -1){
        return -1;
    }
    char magic;
    short header_size;
    int version;
    char no_of_sections;
    lseek(fd,-1,SEEK_END);
    if(read(fd,&magic,1) != 1){
        return -1;
    }
    if(magic != 'b'){
        return 0;
    }
    lseek(fd,-3,SEEK_END);
    if(read(fd,&header_size,2) != 2){
        return -1;
    }
    lseek(fd,-header_size,SEEK_END);
    if(read(fd,&version,4) != 4){
        return -1;
    }
    if(version < 97 || version > 177){
        return 0;
    }
    if(read(fd,&no_of_sections,1) != 1){
        return -1;
    }
    if((int)no_of_sections < 4 || (int)no_of_sections > 12){
        return 0;
    }
    for(int i = 0 ; i < (int)no_of_sections ; i++){
        lseek(fd,8,SEEK_CUR);
        short type;
        if(read(fd,&type,2) != 2){
            return -1;
        }
        if(type != 14 && type != 68 && type != 87 && type != 90 && type != 77){
            return 0;
        }
        lseek(fd,4,SEEK_CUR);
        int size;
        if(read(fd,&size,4) != 4){
            return -1;
        }
        if(size > 1192){
            return 0;
        }
    }
    close(fd);
    return 1;
}

int findall(char *path){
    DIR *dir = opendir(path);
    if(dir == NULL)
    {
        return -1;
    }
    struct dirent *entry;
    struct stat inode;
    while((entry = readdir(dir)) != NULL){
        char *name = (char *)malloc(512);
        strncpy(name, path, strlen(path));
        name[strlen(path)] = '\0';
        strcat(name, "/");
        strcat(name, entry->d_name);
        lstat(entry->d_name, &inode);
        if(strcmp(entry->d_name,".") != 0 && strcmp(entry->d_name,".."))
        {
            if(isgood(name) == true){
                 printf("\n%s/%s", path, entry->d_name);
            }
            if(S_ISDIR(inode.st_mode) && name[strlen(name) - 1] != '.'){
                findall(name);
            }
        }
        free(name);
    }
    free(entry);
    closedir(dir);
    return 0;
}

int main(int argc, char **argv){

    if(argc >= 2){
        if(strcmp(argv[1], "variant") == 0){
            printf("68947\n");
            return 0;
        }
        if(strcmp(argv[1], "list") == 0){
            bool rec = false;
            bool permex = false;
            bool namefilter = false;
            char *namestartswith = (char *)malloc(512);
            char *path = (char *)malloc(512);
            for(int i = 2 ; i < argc ; i++){
                if(strcmp(argv[i], "recursive") == 0){
                    rec = true;
                }
                else
                {
                    if(strcmp(argv[i],"has_perm_execute") == 0){
                        permex = true;
                    }
                    else
                    {
                        if(strncmp(argv[i],"name_starts_with=",17) == 0)
                        {
                            namefilter = true;
                            strncpy(namestartswith,argv[i] + 17,strlen(argv[i]) - 17);
                            namestartswith[strlen(argv[i]) - 17] = '\0';
                        }
                        else
                        {
                            if(strncmp(argv[i],"path=",5) == 0){
                                strncpy(path,argv[i]+5, strlen(argv[i]) - 5);
                                path[strlen(argv[i]) - 5] = '\0';
                            }
                        }
                    }
                }
            }
            if(rec == false){
                if(print_dir_content(path,namefilter,namestartswith,permex) == -1){
                    printf("ERROR\ninvalid directory path");
                }
            }
            else{
                DIR *dir = opendir(path);
                if(dir != NULL)
                {
                    printf("SUCCESS");
                    if(print_dir_content_rec(path,namefilter,namestartswith,permex) == -1){
                        printf("ERROR\ninvalid directory path");
                    }
                } 
                else
                {
                    printf("ERROR\ninvalid directory path");
                }
                closedir(dir);
            }
            free(path);
            free(namestartswith);
            return 0;
        }
        if(strcmp(argv[1],"parse") == 0){
            char *path = (char *)malloc(512);
            strncpy(path,argv[2] + 5, strlen(argv[2]) - 5);
            path[strlen(argv[2]) - 5] = '\0';
            parse(path);
            free(path);
            return 0;
        }
        if(strcmp(argv[1],"extract") == 0){
            char *path = (char *)malloc(2048);
            strncpy(path,argv[2] + 5, strlen(argv[2]) - 5);
            path[strlen(argv[2]) - 5] = '\0';
            char *sect_nr = (char *)malloc(256);
            strncpy(sect_nr, argv[3] + 8, strlen(argv[3]) - 7);
            sect_nr[strlen(argv[3]) - 7] = '\0';
            int section = atoi(sect_nr);
            free(sect_nr);
            char *line_nr = (char *)malloc(256);
            strncpy(line_nr, argv[4] + 5, strlen(argv[4]) - 4);
            line_nr[strlen(argv[4]) - 4] = '\0';
            int line = atoi(line_nr);
            free(line_nr);
            switch(extract(path,section,line)){
                case 1:
                    
                break;
                case -1:
                    printf("invalid file");
                break;
                case 2:
                    printf("invalid section");
                break;
                case 3:
                    printf("invalid line");
                break;
                default:
                    printf("Error at reading from file");
            }
            free(path);
            return 0;
        }
        if(strcmp(argv[1],"findall") == 0){
            char *path = (char *)malloc(512);
            strncpy(path,argv[2] + 5, strlen(argv[2]) - 5);
            path[strlen(argv[2]) - 5] = '\0';
            DIR *dir = opendir(path);
                if(dir != NULL)
                {
                    printf("SUCCESS");
                    if(findall(path) == -1){
                        printf("ERROR\ninvalid directory path");
                    }
                } 
                else
                {
                    printf("ERROR\ninvalid directory path");
                }
            free(path);
            closedir(dir);
            return 0;
        }
    }
    else{
        printf("Not enough arguments");
        exit(2);
    }
    return 0;
}