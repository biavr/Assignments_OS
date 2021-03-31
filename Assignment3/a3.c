#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

char *resp_pipe = "RESP_PIPE_68947";
char *req_pipe = "REQ_PIPE_68947";

unsigned int dim = 0;
char *shmap = NULL;
int curr;
char *mappedfile = NULL;
int shmem = -1;
int filesize = 0;

int main(int argc, char *argv[]){

    if(mkfifo(resp_pipe,0660) < 0){
        printf("ERROR\ncannot create the response pipe");
        exit(1);
    }

    int fdread = open(req_pipe, O_RDONLY);
    if(fdread == -1){
        printf("ERROR\ncannot open the request pipe");
        exit(2);
    }

    int fdwrite = open(resp_pipe, O_WRONLY);
    if(fdwrite == -1){
        printf("cannot open pipe for writing");
        exit(2);
    }
    char len = 7;
    unsigned int num;
    write(fdwrite,&len,1);
    if(write(fdwrite,"CONNECT",len) == -1){
        printf("cannot write into pipe");
        exit(3);
    }

    printf("SUCCESS\n");
    char buf;
    while(read(fdread,&buf,sizeof(char)) == 1){
        int size = (int)buf;
        char *msg = (char *)malloc(size*sizeof(char));
        for(int i = 0 ; i < size ; i++){
            read(fdread,&msg[i],1);
        }
        msg[size] = '\0';
        //at this point I have the request message
        if(strncmp(msg,"PING",4) == 0){
            len = 4;
            write(fdwrite,&len,1);
            write(fdwrite,"PING",size);
            write(fdwrite,&len,1);
            write(fdwrite,"PONG",strlen("PONG"));
            num = 68947;
            write(fdwrite,&num,sizeof(unsigned int));
        }

        if(strncmp(msg,"CREATE_SHM",size) == 0){
            unsigned int dimension;
            read(fdread,&dimension,sizeof(unsigned int));
            dim = dimension;
            //create shared memory area of size dimension
            shmem = shm_open("/aZ6dSe",O_CREAT | O_RDWR,0664);
            if(shmem != -1){
                ftruncate(shmem,dimension);
                shmap = (char *)mmap(NULL, dimension, PROT_READ | PROT_WRITE, MAP_SHARED, shmem, 0);
                if(shmap == MAP_FAILED){
                    len = 10;
                    write(fdwrite,&len,1);
                    write(fdwrite,"CREATE_SHM",size);
                    len = 5;
                    write(fdwrite,&len,1);
                    write(fdwrite,"ERROR",5);
                }
                else{
                    close(shmem);
                    len = 10;
                    write(fdwrite,&len,1);
                    write(fdwrite,"CREATE_SHM",len);
                    len = 7;
                    write(fdwrite,&len,1);
                    write(fdwrite,"SUCCESS",len);
                }
            }
            else{
                len = 10;
                write(fdwrite,&len,1);
                write(fdwrite,"CREATE_SHM",size);
                len = 5;
                write(fdwrite,&len,1);
                write(fdwrite,"ERROR",5);
            }
        }

        if(strncmp(msg,"WRITE_TO_SHM",12) == 0){
            unsigned int offset;
            unsigned int value;
            read(fdread,&offset,sizeof(unsigned int));
            read(fdread,&value,sizeof(unsigned int));
            if(0 <= offset && offset <= dim && offset + sizeof(unsigned int) <= dim){
                //this is the method I found for being able to put an unsigned int into a string
                unsigned char bytes[4];
                bytes[0] = (value >> 24);// & 0xFF;
                bytes[1] = (value >> 16);// & 0xFF;
                bytes[2] = (value >> 8);// & 0xFF;
                bytes[3] = value;// & 0xFF;

                shmap[offset] = bytes[3];
                shmap[offset+1] = bytes[2];
                shmap[offset+2] = bytes[1];
                shmap[offset+3] = bytes[0];
                
                len = 12;
                write(fdwrite,&len,1);
                write(fdwrite,"WRITE_TO_SHM",len);
                len = 7;
                write(fdwrite,&len,1);
                write(fdwrite,"SUCCESS",len);
            }
            else{
                len = 12;
                write(fdwrite,&len,1);
                write(fdwrite,"WRITE_TO_SHM",len);
                len = 5;
                write(fdwrite,&len,1);
                write(fdwrite,"ERROR",len);
            }
        }

        if(strncmp(msg,"MAP_FILE",8) == 0){
            char buf2;
            read(fdread,&buf2,1);
            char *filename = (char *)malloc((int)buf2*sizeof(char));
            for(int i = 0 ; i < (int)buf2 ; i++){
                read(fdread,&filename[i],1);
            }
            filename[(int)buf2] = '\0';
            int fd = open(filename,O_RDONLY);
            if(fd == -1){
                len = 8;
                write(fdwrite,&len,1);                
                write(fdwrite,"MAP_FILE",strlen("MAP_FILE"));
                len = 5;
                write(fdwrite,&len,1);   
                write(fdwrite,"ERROR",strlen("ERROR"));
            }
            else{
                filesize = lseek(fd,0,SEEK_END);
                lseek(fd,0,SEEK_SET);
                mappedfile = (char *)mmap(NULL,filesize,PROT_READ,MAP_PRIVATE,fd,0);
                len = 8;
                write(fdwrite,&len,1);                
                write(fdwrite,"MAP_FILE",strlen("MAP_FILE"));
                len = 7;
                write(fdwrite,&len,1);   
                write(fdwrite,"SUCCESS",strlen("SUCCESS"));
            }            
        }

        if(strncmp(msg,"READ_FROM_FILE_OFFSET",21) == 0){
            unsigned int offset;
            unsigned int bytes;
            read(fdread,&offset,sizeof(unsigned int));
            read(fdread,&bytes,sizeof(unsigned int));
            if(mappedfile != NULL && shmap != NULL && offset + bytes < filesize){
                for(int i = 0 ; i < bytes ; i++){
                    shmap[i] = mappedfile[offset+i];
                }
                len = 21;
                write(fdwrite,&len,1);                
                write(fdwrite,"READ_FROM_FILE_OFFSET",21);
                len = 7;
                write(fdwrite,&len,1);   
                write(fdwrite,"SUCCESS",strlen("SUCCESS"));
            }
            else{
                len = 21;
                write(fdwrite,&len,1);                
                write(fdwrite,"READ_FROM_FILE_OFFSET",21);
                len = 5;
                write(fdwrite,&len,1);   
                write(fdwrite,"ERROR",strlen("ERROR"));
            }
        }

        if(strncmp(msg,"READ_FROM_FILE_SECTION",22) == 0){
            unsigned int section;
            unsigned int offset;
            unsigned int bytes;
            read(fdread,&section,sizeof(unsigned int));
            read(fdread,&offset,sizeof(unsigned int));
            read(fdread,&bytes,sizeof(unsigned int));
            //char *buf4 = (char *)malloc(bytes*sizeof(char));
            short header_size = (mappedfile[filesize-2] << 8) | (0x00ff & mappedfile[filesize-3]);
            int no_of_sections = mappedfile[filesize-header_size+4];
            if(section >= 1 && section <= no_of_sections){
                //filesize - 3 - 18 * section -> is where the header of the section begins
                int beg = filesize - 3 - 18 * (no_of_sections - section + 1);//section;
                //jump over name and type
                beg += 10;
                //get section offset
                char str[4];
                str[3] = mappedfile[beg + 3];
                str[2] = mappedfile[beg + 2];
                str[1] = mappedfile[beg + 1];
                str[0] = mappedfile[beg + 0];
                str[4] = '\0';
                int sect_off = *((int *)str);
                beg += 4;
                //get section size
                str[3] = mappedfile[beg + 3];
                str[2] = mappedfile[beg + 2];
                str[1] = mappedfile[beg + 1];
                str[0] = mappedfile[beg + 0];
                str[4] = '\0';
                int sect_size = *((int *)str);
                if(sect_off + sect_size < sect_off + offset + bytes){
                    len = 22;
                    write(fdwrite,&len,1);                
                    write(fdwrite,"READ_FROM_FILE_SECTION",22);
                    len = 5;
                    write(fdwrite,&len,1);   
                    write(fdwrite,"ERROR",strlen("ERROR"));
                }
                else{
                    for(int i = 0 ; i < bytes ; i++){
                        shmap[i] = mappedfile[sect_off + offset + i];
                    }
                    len = 22;
                    write(fdwrite,&len,1);                
                    write(fdwrite,"READ_FROM_FILE_SECTION",22);
                    len = 7;
                    write(fdwrite,&len,1);   
                    write(fdwrite,"SUCCESS",strlen("SUCCESS"));
                }
            }
            else{
                len = 22;
                write(fdwrite,&len,1);                
                write(fdwrite,"READ_FROM_FILE_SECTION",22);
                len = 5;
                write(fdwrite,&len,1);   
                write(fdwrite,"ERROR",strlen("ERROR"));
            }
        }

        if(strncmp(msg,"READ_FROM_LOGICAL_SPACE_OFFSET",30) == 0){
            unsigned int offset;
            unsigned int bytes;
            read(fdread,&offset,sizeof(unsigned int));
            read(fdread,&bytes,sizeof(unsigned int));

            //first find the number of 3072 blocks each section needs
            short header_size = (mappedfile[filesize-2] << 8) | (0x00ff & mappedfile[filesize-3]);
            int no_of_sections = mappedfile[filesize-header_size+4];
            int *sect_sizes = (int *)malloc(no_of_sections * sizeof(int));
            char str[4];
            int beg = -1;

            for(int i = 0; i < no_of_sections; i++){
                beg = filesize - 3 - 18 * (no_of_sections - i) + 14;
                //get section size
                str[3] = mappedfile[beg + 3];
                str[2] = mappedfile[beg + 2];
                str[1] = mappedfile[beg + 1];
                str[0] = mappedfile[beg + 0];
                str[4] = '\0';
                sect_sizes[i] = *((int *)str);
                //now determine the number of blocks each section needs
                sect_sizes[i] /= 3072;
                sect_sizes[i]++;
            }
            
            //find in which block the required offset is; simple division
            int block = offset / 3072 + 1;
            //find to which section does that block belong to
            int section = 0;
            int offset_in_section = -1;
            int i = 0;
            int sum = 0;
            int ok = 0;
            //these are helping variables I used
            while(i < no_of_sections){
                if(sum + sect_sizes[i] < block){
                    sum += sect_sizes[i];
                    i++;
                }
                else{
                    ok = 1;
                    section = i+1;
                    offset_in_section = offset - sum * 3072;
                    break;
                }
            }
            //now I have the section and the offset in section that mathces the logical offset
            //now the problem reduces ti the previous requirement
            if(ok == 1){
                int beg = filesize - 3 - 18 * (no_of_sections - section + 1);//section;
                //jump over name and type
                beg += 10;
                //get section offset
                str[3] = mappedfile[beg + 3];
                str[2] = mappedfile[beg + 2];
                str[1] = mappedfile[beg + 1];
                str[0] = mappedfile[beg + 0];
                str[4] = '\0';
                int sect_off = *((int *)str);
                beg += 4;
                //get section size
                str[3] = mappedfile[beg + 3];
                str[2] = mappedfile[beg + 2];
                str[1] = mappedfile[beg + 1];
                str[0] = mappedfile[beg + 0];
                str[4] = '\0';
                int sect_size = *((int *)str);
                if(sect_off + sect_size < sect_off + offset_in_section + bytes){
                    len = 30;
                    write(fdwrite,&len,1);                
                    write(fdwrite,"READ_FROM_LOGICAL_SPACE_OFFSET",30);
                    len = 5;
                    write(fdwrite,&len,1);   
                    write(fdwrite,"ERROR",strlen("ERROR"));
                }
                else{
                    for(int i = 0 ; i < bytes ; i++){
                        shmap[i] = mappedfile[sect_off + offset_in_section + i];
                    }
                    len = 30;
                    write(fdwrite,&len,1);                
                    write(fdwrite,"READ_FROM_LOGICAL_SPACE_OFFSET",30);
                    len = 7;
                    write(fdwrite,&len,1);   
                    write(fdwrite,"SUCCESS",strlen("SUCCESS"));
                }
            }
            else{
                len = 30;
                write(fdwrite,&len,1);                
                write(fdwrite,"READ_FROM_LOGICAL_SPACE_OFFSET",30);
                len = 5;
                write(fdwrite,&len,1);   
                write(fdwrite,"ERROR",strlen("ERROR"));
            }
            free(sect_sizes);
        }

        if(strncmp(msg,"EXIT",strlen("EXIT")) == 0){
            //disconnect, close pipes and shared memory, unmap file
            if(mappedfile != NULL){
                munmap(mappedfile,filesize);
            }
            if(shmap != NULL){
                munmap(shmap,dim);
            }
            if(shmem != -1){
                shm_unlink("/aZ6dSe");
            }
            close(fdread);
            close(fdwrite);
            free(msg);
            return 0;
        }
        
    }
}