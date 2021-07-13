#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "fileindex.h"
#include "linebuffer.h"
#include <string.h>

void addEntry(FileIndex *index, FileIndexEntry *entry) {
	FileIndexEntry *copy = index->entries;
	
	if(index->entries == NULL) {
		index->entries = entry;
	} else {
		while(index->entries->next != NULL) {
			index->entries = index->entries->next;
		}
		index->entries->next = entry;
		index->entries = copy;
	}
}

FileIndex *fi_new(const char *filepath, const char *separator) {
	
	FileIndex *result = calloc(1, sizeof(FileIndex));
	FileIndexEntry *newEntry = NULL;
	LineBuffer *buf = NULL;
	
	int LINEMAX = 100;
	int found_mode = 0;
	int seekpos=0;
	int seekpos_before_end;
	int lines  = 0;
	int nr = 1;
	int oeffnen;
	
	oeffnen = open(filepath, O_RDONLY, 0644);
	if(oeffnen < 0) {
		perror("Mailbox wurde nicht gefunden");
		exit(1);
	}
	
	result->filepath = filepath;
	
	buf = buf_new(oeffnen, "\n");
	char *line = calloc(LINEMAX, sizeof(char *));
	memset(line, '\0', sizeof(*line) * LINEMAX);
	FileIndexEntry *entry = NULL;
	while( (seekpos = buf_readline(buf, line, LINEMAX)) ) {
		
		if(seekpos > -1) {
			
			//if(strstr(line, separator) != NULL && *line == *separator) {
				
			if(!strncmp(line, separator, strlen(separator))) {
				
				if(newEntry != NULL) {
					newEntry->size = (seekpos-strlen(line)) - newEntry->seekpos;
					
					newEntry->lines = lines;
					lines = 0;
					
					addEntry(result, newEntry);
					result->nEntries = result->nEntries+1;
					result->totalSize = result->totalSize + newEntry->size;
				}
		
				newEntry = calloc(1, sizeof(FileIndexEntry));
				newEntry->nr = nr;
				nr=nr+1;
				
				newEntry->seekpos = seekpos - strlen(line);
				//newEntry->seekpos = buf->bytesread - buf->end + buf->here - strlen(line);
			} else {
				lines=lines+1;
			}
			
		} else {	
			
			if(newEntry != NULL) {
				newEntry->size = (seekpos_before_end - newEntry->seekpos);
		
				newEntry->lines = lines;
				newEntry->nr = nr-1;
				lines = 0;
				
				addEntry(result, newEntry);
				result->nEntries = result->nEntries+1;
				result->totalSize = result->totalSize + newEntry->size;
			}	
			break;
			
		}
		seekpos_before_end = seekpos;
		memset(line, '\0', LINEMAX);
	}
	free(buf);
	return result;
}

void fi_dispose(FileIndex *fi) {
	FileIndexEntry *copy = fi->entries;
	FileIndexEntry *current = NULL;
	FileIndexEntry *buffer = NULL;

	while(copy != NULL)  {
		buffer = copy->next;
		current = copy;
		free(current);
		copy = buffer;
	}
	free(fi);
}

FileIndexEntry *fi_find(FileIndex *fi, int n) {
	FileIndexEntry *result = NULL;
	FileIndexEntry *copy = fi->entries;
	
	while(fi->entries->next != NULL) {
		if(fi->entries->nr == n) {
			result = fi->entries;
			break;
		}
		fi->entries = fi->entries->next;
	}
	if(fi->entries->nr == n) {
		result = fi->entries;
	}
	
	fi->entries = copy;
	return result;
}


int fi_compactify(FileIndex *fi) {
		FileIndexEntry *freeBuffer = NULL;
		FileIndex *newfi = NULL;
		const char *fpath = fi->filepath;
		int oeffnen;
		int mailboxRead;
		int mailbox;
		int setzen;
		int schreiben;
		int entf;
		int ren;
		
		
		oeffnen = open("mailbox_compactify", O_CREAT|O_TRUNC|O_WRONLY, 0644);
		if(oeffnen < 0) {
			perror("FileIndex.c: Fehler beim Oeffnen der Datei");
			exit(2);
		}
		
		mailbox = open(fi->filepath, O_RDONLY, 0644);
		if(mailbox < 0) {
			perror("FileIndex.c: Fehler beim Oeffnen der Mailbox");
			exit(2);
		} 
		
		while(fi->entries != NULL) {
			if(fi->entries->del_flag != 1) {	
				
				char readBuffer[fi->entries->size];
				memset(readBuffer, '\0', fi->entries->size);
				freeBuffer = fi->entries;
				
				setzen = lseek(mailbox, fi->entries->seekpos, SEEK_SET);
				if(setzen < 0) {
					perror("FileIndex.c: Fehler beim Setzen");
					exit(4);
				}
				mailboxRead = read(mailbox, readBuffer, fi->entries->size);
				if(mailboxRead < 0) {
					perror("FileIndex.c: Fehler beim Lesen");
					exit(3);
				}
				schreiben = write(oeffnen, readBuffer, fi->entries->size);
				if(schreiben < 0) {
					perror("FileIndex.c: Fehler beim Schreiben");
				}
			} else {
				freeBuffer = fi->entries;
			}
			fi->entries = fi->entries->next;
			free(freeBuffer);
		}

		close(oeffnen);
		close(mailbox);
			
		entf = remove(fi->filepath);
		if(entf < 0) {
			perror("Fehler beim Loeschen");
			exit(6);
		}
		ren = rename("mailbox_compactify", fi->filepath);
		if(ren < 0) {
			perror("Fehler beim Umbenennen");
			exit(7);
		}
		free(fi);
		newfi = fi_new(fpath, "From "); 
		fi = newfi;
		return 0;
}

void printFileIndex(FileIndex *fi) {
	FileIndexEntry *copy = fi->entries;
	
	printf("[FileIndex] nEntries: %d, totalSize: %d\n", fi->nEntries, fi->totalSize);
	while(fi->entries->next != NULL) {
		printf("... [FileIndexEntry] nr: %d, seekpos: %d, lines: %d, size: %d, del_flag: %d\n", fi->entries->nr, fi->entries->seekpos, fi->entries->lines, fi->entries->size, fi->entries->del_flag);
		fi->entries = fi->entries->next;
	}
	printf("... [FileIndexEntry] nr: %d, seekpos: %d, lines: %d, size: %d, del_flag: %d\n", fi->entries->nr, fi->entries->seekpos, fi->entries->lines, fi->entries->size, fi->entries->del_flag);
	fi->entries = copy;
}

void printFileIndexEntry(FileIndexEntry *fie) {
	if(fie != NULL) {
		printf("[FileIndexEntry] nr: %d, seekpos: %d, lines: %d, size: %d, del_flag: %d\n", fie->nr, fie->seekpos, fie->lines, fie->size, fie->del_flag);
	}
}

/*
int main(void) {
	FileIndex *test = fi_new("mailbox/joendhard.mbox", "From ");
	
	printFileIndex(test);
	
	FileIndexEntry *testEntry = fi_find(test, 1);

	testEntry->del_flag = 1;
	fi_compactify(test);
	
	//printFileIndex(test);
	printFileIndexEntry(testEntry);
	
	fi_dispose(test);
}
*/



