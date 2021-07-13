#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#include "fileindex.h"
#include "linebuffer.h"
#include "dialog.h"
#include "database.h"

#define READLEN 1000
#define INFOLEN 100
#define LINEMAX 100

FileIndex *fie = NULL;
int smtpZustand = 0;
char buffer[READLEN];
char smtpdatabase_path[] = "users.dat";

char absender[INFOLEN];
char current_user[INFOLEN];
char rcpt_to[INFOLEN];
char data[1000];

int first_write = 1;

DialogRec smtpDialog[] = { 
		/* command, 		param,	state,	nextstate,	validator */
		{ "helo", 			"", 	0, 		1, 			validate_hasParam }, 
		{ "mail from:",		"", 	1, 		2,			validate_hasParam }, 
		{ "rcpt to:", 		"", 	2, 		3,			validate_hasParam }, 
		{ "data", 			"", 	3,		4,			validate_noparam }, 
		{ "quit", 			"", 	5, 		5,			validate_noparam },
		{ "" }
};


DBRecord *getUserMailbox(char user[]) {
	DBRecord *userstat = calloc(1, sizeof(DBRecord));
	int found;

	strcpy(userstat->key, rcpt_to);
	strcpy(userstat->cat, "smtp");
	
	found = db_search(smtpdatabase_path, 0, userstat);
	if(found == -1) {
		free(userstat);
		return NULL;
	}

	strcpy(userstat->key, userstat->value);
	strcpy(userstat->cat, "mailbox");
	found = db_search(smtpdatabase_path, 0, userstat);
	
	printf("KEY: %s", userstat->key);
	
	return userstat;
}

void sendMessage(char code[], char msg[], int outfd) {
	int schreiben;
	int len = strlen(code) + strlen(msg) + 1;
	char fullMsg[len];
	memset(fullMsg, '\0', len);
	strcat(fullMsg, code);
	strcat(fullMsg, " ");
	strcat(fullMsg, msg);
	strcat(fullMsg, "\r\n");
	
	if( (schreiben = write(outfd, fullMsg, strlen(fullMsg))) < 0) {
		perror("Konnte nicht auf outfd schreiben");
		exit(3);
	}
}

int initSmtpFileIndex() {
	char sperrName[strlen(getUserMailbox(rcpt_to)->value) + strlen("_lock")];
	
	int BUFFERSIZE = 100;
	char leseBuffer[BUFFERSIZE];
	int prozessid = getpid();
	int prozessFile; 
	int oeffnen;
	int lesen;
	int schreiben;
	
	strcpy(sperrName, getUserMailbox(rcpt_to)->value);
	strcat(sperrName, "_lock");

	/* Potenzielle Sperrdatei oeffnen */
	printf("Sperrname: %s\n", sperrName);
	oeffnen = open(sperrName, O_RDWR | O_CREAT, 0644);
	if(oeffnen < 0)  {
		perror("Fehler beim Oeffnen der Sperrdatei");
		exit(1);
	}
	
	/* Versuchen zu lesen */
	lesen = read(oeffnen, leseBuffer, BUFFERSIZE);
	if(lesen < 0) {
		perror("Fehler beim Lesen der Sperrdatei");
		exit(2);
	} 

	if(lesen == 0) {
		/* Die Sperrdatei ist leer und wurde neu angelegt -> Prozess-Id reinschreiben */
		memset(leseBuffer, '\0', BUFFERSIZE);		
		sprintf(leseBuffer, "%d", prozessid);
		schreiben = write(oeffnen, leseBuffer, strlen(leseBuffer));
		if(schreiben < 0) {
			perror("Schreiben fehlgeschlagen");
			exit(3);
		}
		printf("Mailbox: %s\n", getUserMailbox(rcpt_to)->value);
		fie = fi_new(getUserMailbox(rcpt_to)->value, "From ");
		return 0;
	} else {
		prozessFile = atoi(leseBuffer);
		if(prozessFile == prozessid || kill(prozessFile, 0) < 0) {
			if(prozessFile != prozessid) {
				/* Neue Prozessnummer schreiben */
				lseek(oeffnen, 0, SEEK_SET);
				memset(leseBuffer, '\0', BUFFERSIZE);		
				sprintf(leseBuffer, "%d", prozessid);
				schreiben = write(oeffnen, leseBuffer, strlen(leseBuffer));
				if(schreiben < 0) {
					perror("Schreiben fehlgeschlagen");
					exit(3);
				}
			}
			printf("Lock-Datei wurde eventuell ueberschrieben: %d\n", prozessFile);
			fie = fi_new(getUserMailbox(rcpt_to)->value, "From ");
			return 0;
		} else {
			return 1;
		}
	}
	close(oeffnen);	
}


int process_smtp(int infd, int outfd) {
	ProlResult validated;
	DialogRec *related;
	int lesen;
	int schreiben;
	int handle;
	int len;
	int mbox;
	int i = 1;
	int j = 0;
	int k = 0;
	int allowed = 0;
	int datastream  = 0;
	int seekpos;
	int first_write = 1;
	
	if(smtpZustand != 4) {		
		if( (lesen = read(infd, buffer, READLEN) < 0) ) {
			perror("POP3: Fehler beim Lesen!"); 
			exit(2);
		}
	
		for(k=0;k<strlen(buffer);k++) {
			buffer[k] = tolower(buffer[k]);
		}
		validated = processLine(buffer, smtpZustand , smtpDialog);
		related = validated.dialogrec;
	} else {
		datastream = 1;
	}

	if((validated.failed == 1 || related->is_valid == 0) && datastream == 0) {
		sendMessage("500", "", outfd);
		return smtpZustand;
	} else {
		switch(smtpZustand ) {
			case 0:
				if( (strcmp(related->command, "helo") == 0) ) {
						len = strlen(related->param);
						related->param[len-1] = '\0';
						related->param[len-2] = '\0';
							
						sendMessage("250", "Ok", outfd);
		
						smtpZustand  = smtpZustand +1;
						return smtpZustand ;
				}
				break;
			case 1:
				if( (strcmp(related->command, "mail from:") == 0) ) {		
						char copy[strlen(related->param)];
						memset(copy, '\0', strlen(related->param));
						memset(absender, '\0', INFOLEN);
						
						/* Absender ermitteln */
						i=0;
						j=0;
												
						memset(copy, '\0', strlen(related->param));
						while(related->param[i] != '\0') {
							if(related->param[i] != '>') {
								copy[j] = related->param[i];
							}
							i=i+1;
							j=j+1;
						}
						strcpy(absender, copy);
						
						printf("Erkannter Absender:%s\n", absender);
						
						sendMessage("250", "Ok", outfd);
						smtpZustand  = smtpZustand +1;
						return smtpZustand ;
				}
				break;
			case 2:
				if( (strcmp(related->command, "rcpt to:") == 0) ) {					
						//Empfaenger extrahieren
						char copy[strlen(related->param)-2];
						memset(copy, '\0', strlen(related->param)-2);
						
						i=0;
						j=0;
						while(related->param[i] != '\0') {
							if(related->param[i] != '>') {
								copy[j]=related->param[i];
							}
							i=i+1;
							j=j+1;
						}
						strcpy(rcpt_to, copy);
						
						printf("Erkannter Empfaenger:%s\n", rcpt_to);
						//Check ob der User existiert
						
						if(getUserMailbox(rcpt_to) != NULL) {
							sendMessage("250", "Ok", outfd);
							smtpZustand = smtpZustand +1;
						} else {
							sendMessage("550", "", outfd);
						}
				
						return smtpZustand;
				}
				break;
			case 3:
				if( (strcmp(related->command, "data") == 0) ) {
						sendMessage("354", "Ok", outfd);
	
						smtpZustand = smtpZustand +1;						
						return smtpZustand;
				}
				break;
				
			case 4:					
				if(smtpZustand == 4) {
					char *line = calloc(LINEMAX, sizeof(char));	
					LineBuffer *buf = buf_new(infd, "\r\n");	
					
					memset(line, '\0', sizeof(*line) * LINEMAX);
					while( (seekpos = buf_readline(buf, line, LINEMAX)) ) {
						
						//printf("Gelesen: [%s]\n", line);
						
						mbox = open(getUserMailbox(rcpt_to)->value, O_WRONLY | O_APPEND);
						if(seekpos == -1) {
							break;
						}
						
						if(strcmp(line, ".") == 0) {
							close(mbox);
							smtpZustand = smtpZustand+1;
							sendMessage("250", "Ok", outfd);
							break;
						} else {
							if(first_write == 1) {
								time_t t;
								char entry[1024];
								memset(entry, '\0', 1024);
								time(&t);
								sprintf(entry, "\nFrom %s %s\n%s\n", absender, ctime(&t), line);
								printf("Schreiben: [%s]\n", entry);
								schreiben = write(mbox, entry, strlen(entry));
								first_write = 0;
							} else {
								schreiben = write(mbox, line, strlen(line));
								schreiben = write(mbox, "\n", 1);
							}
						}
						memset(line, '\0', sizeof(*line) * LINEMAX);
					}
				}
				return smtpZustand;
				break;
			case 5:
				if( (strcmp(related->command, "quit") == 0) ) {
						sendMessage("221", "Bye", outfd);
						return -1;
				}
				break;
				
			default:
				sendMessage("500", "", outfd);
				return smtpZustand;
				break;
		}
	}
}
