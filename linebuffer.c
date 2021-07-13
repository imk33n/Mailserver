#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "linebuffer.h"

LineBuffer *buf_new(int descriptor, const char *linesep) {
	LineBuffer *neu = calloc(1, sizeof(LineBuffer));
	neu->descriptor = descriptor;
	neu->linesep = linesep;
	neu->lineseplen = strlen(linesep);
	neu->bytesread = 0;
	neu->here = 0;
	return neu;
}

void buf_dispose(LineBuffer *b) {
	close(b->descriptor);
	free(b);
}

void bufferStatus(LineBuffer *buf) {
	printf("=> [Buffer] bytesread %d, here: %d, end: %d, buf_where(): %d\n", buf->bytesread, buf->here, buf->end, buf_where(buf));
}

int buf_readline(LineBuffer *b, char *line, int linemax){
	int lesen;
	int sepPos;
	int oset=0;
	if(b->here >= b->end) {
		lesen = read(b->descriptor, b->buffer, LINEBUFFERSIZE);
		
		if(lesen == 0){
			return -1;
		}
		if(lesen < 0){
			exit(2);
		}
		b->here = 0;
		b->end = lesen;
		
		b->bytesread = b->bytesread + strlen(b->buffer);

	}
	
	sepPos = strstr(b->buffer + b->here, b->linesep) - b->buffer;
	
	if(sepPos >= 0){
		strncpy(line, b->buffer + b->here, sepPos - b->here);
		line[sepPos - b->here] = 0;
	} else {
		strncpy(line, b->buffer + b->here, b->end - b->here);
		line[b->end - b->here] = 0;
		oset = strlen(line);
		
		lesen = read(b->descriptor, b->buffer, LINEBUFFERSIZE);
		
		if(lesen == 0){
			return -1;
		}
		if(lesen < 0){
			exit(2);
		}
		b->here = 0;
		b->end = lesen;
		
		b->bytesread = b->bytesread + lesen;
		
		sepPos = strstr(b->buffer + b->here, b->linesep) - b->buffer;

		strncpy(line+oset, b->buffer + b->here, sepPos);
		line[sepPos - b->here + oset] = 0;
	}
	
	line[linemax-1] = 0;
	
	b->here = sepPos + b->lineseplen;
	
	return buf_where(b) - 1;
}

int buf_where(LineBuffer *b){
	return b->bytesread - b->end + b->here;
}

int buf_seek(LineBuffer *b, int seekpos){
	int set;
	
	set = lseek(b->descriptor, seekpos, SEEK_SET);
	if(set < 1) {
		perror("Fehler beim Setzen");
		exit(3);
	}
	b->bytesread = seekpos;
	b->here = 0;
	b->end = 0;
	
	return set;
}


/*
int main(void) {
	int datei;
	int readLine = 0;
	LineBuffer *lbuffer = NULL;
	char *befuellMich = NULL;
	
	datei = open("joendhard.mbox", O_RDONLY, 0644);
	if(datei<0) {
		perror("Oeffnen fehlgeschlagen!");
		exit(1);
	}
	
	lbuffer = buf_new(datei, "\n");
	befuellMich = calloc(1, sizeof(char *) * 10);
	
	bufferStatus(lbuffer);
	while(readLine = buf_readline(lbuffer, befuellMich, 100) > 0) {
		printf("Befuellt: %s Pos: %d\n", befuellMich, lbuffer->here);
		memset(befuellMich, 0, 80);
	}
	buf_dispose(lbuffer);
	return 0;
}
*/
