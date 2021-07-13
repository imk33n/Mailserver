#include "database.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h> 
#include <fcntl.h>
#include "dialog.h"
#include <errno.h>
#include <string.h>
#include <sys/types.h>
int catValue(const char *filepath, const DBRecord *record){
	/*returns index of DBRecord with matching key & cat */ 
    int read_fd,re,start = 0; 
    char keyBuffer[DB_KEYLEN], catBuffer[DB_CATLEN]; 
    read_fd = open(filepath,O_RDONLY); 
    
    if(read_fd < 0){
        perror("Error while Reading"); 
        return -42; 
    }
    while(1){
        if(record->key == NULL || record->cat == NULL){
            break; 
        }
        re = read(read_fd,keyBuffer,DB_KEYLEN);
        if(re == 0){
            return -1; 
        }
        if(strcmp(record->key,keyBuffer)==0){
            re = read(read_fd,catBuffer,DB_CATLEN); 
            if(strcmp(record->cat,catBuffer)==0){
                break;
            }
        }
        start += 1; 
        lseek(read_fd,(start)*sizeof(DBRecord),SEEK_SET);
    }
    
    close(read_fd); 
    return start;
}

int db_search(const char *filepath, int start, DBRecord *record){
    int read_fd, re; 
    int idx = sizeof(DBRecord) * start;
    char keyBuffer[DB_KEYLEN], catBuffer[DB_CATLEN]; 
    read_fd = open(filepath,O_RDONLY); 
    
    if(read_fd < 0){
        perror("Error while Reading"); 
        return -42; 
    }
    idx = lseek(read_fd,idx,SEEK_SET); 
    if(idx < 0){
        perror("Error while setting Index"); 
        return -42; 
    }
    while(1){
        if(record->key == NULL || record->cat == NULL){
            break; 
        }
        re = read(read_fd,keyBuffer,DB_KEYLEN);
        if(re == 0){
            return -1; 
        }
        if(strcmp(record->key,keyBuffer)==0){
            break; 
        }
        re = read(read_fd,catBuffer,DB_CATLEN); 
        if(strcmp(record->cat,catBuffer)==0){
            break; 
        }
        start += 1; 
        idx = lseek(read_fd,(start)*sizeof(DBRecord),SEEK_SET);
    }

    close(read_fd); 
    return start;
}

int db_get(const char *filepath, int index, DBRecord *result){
    int read_fd,re; 
    int idx = index * sizeof(DBRecord); 
    char keyBuffer[DB_KEYLEN], catBuffer[DB_CATLEN], valBuffer[DB_VALLEN]; 
    read_fd = open(filepath,O_RDONLY); 
    
    if(read_fd < 0){
        return -1; 
    }
    idx = lseek(read_fd,idx,SEEK_SET); 
    re = read(read_fd,keyBuffer,DB_KEYLEN);
    if(re == 0){
        perror("Error while Reading Key"); 
        return -1; 
    }
    re = read(read_fd,catBuffer,DB_CATLEN); 
    if(re == 0){
        perror("Error while Reading Cat");
        return -1; 
    }
    re = read(read_fd,valBuffer,DB_VALLEN); 
    if(re == 0){
        perror("Error while Reading Value");
        return -1; 
    }
    strcpy(result->key,keyBuffer); 
    strcpy(result->cat, catBuffer); 
    strcpy(result->value,valBuffer); 
    return 0;  
}

int db_size(const char *filepath){
    int read_fd, re,i; 
    char keyBuffer[DB_KEYLEN], catBuffer[DB_CATLEN],valBuffer[DB_VALLEN]; 
    char *buffer[3];
    int counter = 0,bufferSize []= {DB_KEYLEN,DB_CATLEN,DB_VALLEN}; 
    
    buffer[0] = keyBuffer; 
    buffer[1] = catBuffer; 
    buffer[2] = valBuffer; 
    /*vorher geschriebene Datei oeffnen*/
    read_fd = open(filepath,O_RDONLY); 
    
    if(read_fd < 0){
        perror("error while writing"); 
        exit(3); 
    }
   /* w = write(outfd,*/
   while(1){
      
       /*Buffersize variiert fuer key, cat und Inhalt*/
       for(i = 0; i < 3; i++){
           re = read(read_fd,buffer[i],bufferSize[i]); 
           if(re == 0){
               return counter; 
            }else if(re < 0){
                perror("Error while reading"); 
                break; 
            }
            
        }
        counter ++; 
   }
   close(read_fd);  
   
   return -1; 
}
int db_put(const char *filepath, int index, const DBRecord *record){
    int read_fd,wr; 
    int idx = index * sizeof(DBRecord); 
    /*1. pruefen ob record ans ende gehört - wenn j amit Append öffenen*/
    if(idx > 0 && idx < db_size(filepath)){
        read_fd = open(filepath,O_RDWR );
    }else{
        read_fd = open(filepath,O_RDWR|O_APPEND ); 
    }
    if(read_fd < 0){
        perror("Error while Reading"); 
        return -1; 
    }
    if(idx >= 0){
        idx = lseek(read_fd,idx,SEEK_SET); 
    }
    wr = write(read_fd,record,sizeof(DBRecord)); 
    
    if(wr < 0){
        perror("Error while Writing"); 
        return -1; 
    }
    close(read_fd); 
    return 0; 
}

int db_update(const char *filepath, const DBRecord *new){
    int read_fd,wr; 
    int idx = 0; 
    idx = catValue(filepath,new);
    if(idx < 0){
        read_fd = open(filepath,O_RDWR|O_APPEND ); 
        wr = write(read_fd,new,sizeof(DBRecord)); 
        if(wr < 0){
            perror("Error while writing Record"); 
            return -1; 
        }
        close(read_fd); 
        return db_size(filepath); 
        
    }
    read_fd = open(filepath,O_RDWR );
    idx = idx * sizeof(DBRecord) + DB_KEYLEN + DB_CATLEN; 
    lseek(read_fd,idx,SEEK_SET);
    wr = write(read_fd,new->value,DB_VALLEN); 
    if(wr < 0){
            perror("Error while writing Record"); 
            return -1; 
        }
    
    return 0; 
    
}

int db_del(const char *filepath, int index){
    int read_fd, fd_write,wr,re, dbIndex = 0; 
    char *tmpFile = "temp";
    char DBBuffer[sizeof(DBRecord)]; 
    /*Index nicht vorhanden */
    if(index < 0 || index > db_size(filepath)){
        perror("No Record with Index"); 
        return -1; 
    }
    fd_write = open(tmpFile, O_WRONLY|O_TRUNC|O_CREAT, 0644); 
    read_fd = open(filepath, O_RDONLY); 
    
    while(1){
    
        if(dbIndex != index){
            re = read(read_fd,DBBuffer,sizeof(DBRecord)); 
            if(re == 0){
                break; 
            }else if(re < 0){
                perror("Error while reading"); 
                break; 
            }
            wr = write(fd_write,DBBuffer,sizeof(DBRecord)); 
            if( wr < 0){
                perror("Error while writing"); 
            }
        }else{
            lseek(read_fd,sizeof(DBRecord),SEEK_CUR); 
        }
        dbIndex += 1; 
    }
    close(read_fd); 
    close(fd_write); 
    rename(tmpFile,filepath); 
    return 0; 
}

void writeFailure(int wr){
     if( wr < 0){
        perror("Error while writing"); 
        exit(4); 
    }
}

/*0 = stdin, 1 = stdout, 2 = stder*/
int db_list(const char *path, int outfd, int (*filter)(DBRecord *rec, const void *data), const void *data){
    int read_fd, re,wr,j, valid = 1; 
    DBRecord *akt = malloc(sizeof(DBRecord)); 
    char keyBuffer[DB_KEYLEN], catBuffer[DB_CATLEN],valBuffer[DB_VALLEN]; 
    int counter = 0; 
    
    /*vorher geschriebene Datei oeffnen*/
    read_fd = open(path,O_RDONLY); 
    if(read_fd < 0){
        perror("error while writing"); 
        exit(3); 
    }
   /* w = write(outfd,*/
    while(1){
        re = read(read_fd,keyBuffer,DB_KEYLEN); 
        if(re == 0){
            return counter; 
        }else if(re < 0){
            perror("Error while reading"); 
            break;
        }
        re = read(read_fd,catBuffer,DB_CATLEN); 
        if(re == 0){
            return counter; 
        }else if(re < 0){
            perror("Error while reading"); 
            break;
        }
        re = read(read_fd,valBuffer,DB_VALLEN); 
        if(re == 0){
            return counter; 
        }else if(re < 0){
            perror("Error while reading"); 
            break;
        }
        strcpy(akt->key, keyBuffer); 
        strcpy(akt->cat, catBuffer); 
        strcpy(akt->value, valBuffer);
        
        if(filter != NULL){
            valid = filter(akt, data); 
        }
        if(valid){ 
            wr = write(outfd, keyBuffer, DB_KEYLEN); 
            writeFailure(wr); 
            for(j = 0; j < DB_KEYLEN-strlen(keyBuffer); j++){
                wr = write(outfd," ", 1); 
            }
            writeFailure(wr);
            wr = write(outfd,"|",1); 
            
            wr = write(outfd, catBuffer, DB_CATLEN); 
            writeFailure(wr); 
            for(j = 0; j < DB_CATLEN-strlen(catBuffer); j++){
                wr = write(outfd," ", 1); 
            }
            wr = write(outfd,"|",1);
            
            wr = write(outfd, valBuffer, DB_VALLEN); 
            writeFailure(wr); 
            wr = write(outfd,"\n",1);
        }
        counter ++; 
   }
   close(read_fd); 
   close(outfd); 
   return -1; 
}


