#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <sys/types.h>
#include <unistd.h>

#include "fileindex.h"
#include "linebuffer.h"
#include "dialog.h"
#include "database.h"
#include <string.h>

#define READLEN 100

int zustand = 0;
FileIndex *fi = NULL;
char buffer[READLEN];
char database_path[] = "users.dat";
char current_user[100]; 

DialogRec dialog[] = { 
		/* command, 		param,	state,	nextstate,	validator */
		{ "user", 			"", 	0, 		1, 			validate_hasParam }, 
		{ "pass",			"", 	1, 		2,			validate_hasParam }, 
		{ "list", 			"", 	2, 		2,			validate_digitempty }, 
		{ "noop", 			"", 	2,		2,			validate_noparam }, 
		{ "stat", 			"", 	2, 		2,			validate_noparam },
		{ "retr",			"",		2,		2,			validate_digit },
		{ "dele", 			"",		2,		0,			validate_hasParam },
		{ "rset", 			"",		2,		0,			validate_noparam },  
		{ "quit", 			"",		2,		0,			validate_noparam }, 
		{ "" }
};

void sendErrorMessage(char errorMsg[], int outfd) {
	int schreiben;
	char error[] = "-ERR ";
	int len = strlen(error) + strlen(errorMsg);
	char fullError[len];
	
	strcpy(fullError, error);
	strcat(fullError, errorMsg);
	strcat(fullError, "\r\n");
	
	//printf("%s\n", fullError);
	
	if( (schreiben = write(outfd, fullError, strlen(fullError)) < 0) ) {
		perror("Konnte nicht auf outfd schreiben");
		exit(3);
	}
	
}
void sendSuccessMessage(char successMsg[], int outfd) {
	int schreiben;
	char success[] = "+OK ";
	int len = strlen(success) + strlen(successMsg);
	char fullSuccess[len];
	strcpy(fullSuccess, success);
	strcat(fullSuccess, successMsg);
	strcat(fullSuccess, "\r\n");
	
	//printf("%s\n", fullSuccess);
	
	if( (schreiben = write(outfd, fullSuccess, strlen(fullSuccess)) < 0) ) {
		perror("Konnte nicht auf outfd schreiben");
		exit(3);
	}
}

DBRecord *getMailbox() {
	DBRecord *userstat = calloc(1, sizeof(DBRecord));
	int found;
	
	/* strcpy(userstat->key, current_user); */
	strcpy(userstat->key, current_user);
	strcpy(userstat->cat, "mailbox");
	
	found = db_search(database_path, 0, userstat);
	return userstat;
}

int initFileIndex() {
	char sperrName[strlen(getMailbox()->value) + strlen("_lock")];
	int BUFFERSIZE = 100;
	char leseBuffer[BUFFERSIZE];
	int prozessid = getpid();
	
	int prozessFile; 
	int oeffnen;
	int lesen;
	int schreiben;
	strcpy(sperrName, getMailbox()->value);
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
		printf("Mailbox: %s\n", getMailbox());
		if(fi == NULL) {
			fi = fi_new(getMailbox()->value, "From ");
		}
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
			if(fi == NULL) {
				fi = fi_new(getMailbox()->value, "From ");
			}
			return 0;
		} else {
			return 1;
		}
	}
	close(oeffnen);	
}

int handleUser(DialogRec *d) {
	DBRecord *user = calloc(1, sizeof(DBRecord));
	int found;
	int len = strlen(d->param);
	int belegt = 0;
	
	d->param[len-1] = '\0';
	d->param[len-2] = '\0';
	strcpy(user->key, d->param);
	found = db_search(database_path, 0, user);	
	free(user);
	
	belegt = initFileIndex();
	
	if(found != -1 && belegt == 0) {
		return 1;
	} else {
		return 0;
	}
}

int handlePassword(DialogRec *d) {
	DBRecord *userpass = calloc(1, sizeof(DBRecord));
	int found;
	int len = strlen(d->param);
	printf("%s", current_user);
	strcpy(userpass->key, current_user);
	strcpy(userpass->cat, "password");
	
	d->param[len-1] = '\0';
	d->param[len-2] = '\0';
	found = db_search(database_path, 0, userpass);	
	
	if(found > 0 && strcmp(userpass->value, d->param) == 0) {
		return 1;
	} else {
		return 0;
	}
}

void handleStat(DialogRec *d, int outfd) {
	DBRecord *userstat = getMailbox();
	
	/* FileIndex updaten */
	//fi_dispose(fi);
	initFileIndex();
	
	if(userstat != NULL) {
		/* fi = fi_new(userstat->value, "From "); */
		/* Message aufbauen */
		char anz[100];
		char len[100];
		char message[strlen(anz) + strlen(len) + 1];
		
		snprintf(anz, 100, "%d", fi->nEntries);
		snprintf(len, 100, "%d", fi->totalSize);
		
		strcpy(message, anz);
		strcat(message, " ");
		strcat(message, len);
		sendSuccessMessage(message, outfd);
		free(userstat);
	} else {
		sendErrorMessage("Mailbox not found.", outfd);
	}
	/*free(fi);*/
}

int handleList(DialogRec *d, int outfd) {
	DBRecord *userstat = getMailbox();
	FileIndexEntry *copy = NULL;
	char buffer[1024] = "";
	char fullList[1024] = "";
	int nr;
	int abzieher = 0;

	/* FileIndex updaten */
	//fi_dispose(fi);
	initFileIndex();

	d->param[strlen(d->param)-1] = '\0';
	if(userstat != NULL) {
		/* fi = fi_new(userstat->value, "From "); */
		copy = fi->entries;
		
		/* Wenn keine Nummer angegeben wurde */
		if(strcmp(d->param, "") == 0) {
			
			if(fi == NULL || fi->entries == NULL) {
				sendErrorMessage("No entries found!", outfd);
				return 0;
			} else {
				sprintf(fullList, "%d messages:\r\n", fi->nEntries);
				while(fi->entries != NULL) {
					if(fi->entries->del_flag != 1) {
						/* Nummer zusammenbauen */
						sprintf(buffer, "%d", fi->entries->nr);
						strcat(fullList, buffer);
						strcat(fullList, " ");
						
						/* Size zusammen bauen */
						sprintf(buffer, "%d", fi->entries->size - fi->entries->lines);
						strcat(fullList, buffer);
						strcat(fullList, "\r\n");
						
						memset(buffer, '\0', 1024);
					}
					fi->entries = fi->entries->next;
				}
				strcat(fullList, ".");
				sendSuccessMessage(fullList, outfd);
				
				fi->entries = copy;
				/*fi_dispose(fi);*/
				free(userstat);
			}
		} else {
			/* TODO CR entfernen */
			d->param[strlen(d->param)-1] = '\0';
			nr = atoi(d->param);
			copy = fi_find(fi, nr);
			if(copy != NULL) {
				sprintf(fullList, "%d %d", copy->nr, copy->size);
				sendSuccessMessage(fullList, outfd);
			} else {
				sendErrorMessage("Konnte Eintrag nicht finden", outfd);
				return 0;
			}
			/* fi->entries = copy; */
			/*fi_dispose(fi);*/
			free(userstat);
		}
	}
	return 1;
	free(userstat);
}

int handleRetr(DialogRec *d, int outfd) {
	DBRecord *userstat = getMailbox();
	FileIndexEntry *fie = NULL;
	int lesen;
	int oeffnen;
	int setzen;
	int nr;
	
	/* FileIndex updaten */
	initFileIndex();
	
	d->param[strlen(d->param)-1] = '\0';
	nr = atoi(d->param);

	if(fi != NULL && fi->entries != NULL) {
		fie = fi_find(fi, nr);
		if(fie == NULL) {
			sendErrorMessage("Konnte Eintrag nicht finden", outfd);
			return 0;
		}
		
		oeffnen = open(userstat->value, O_RDONLY, 0644);
		if(oeffnen < 0) {
			perror("Datei konnte nicht geoeffnet werden!");
			sendErrorMessage("Mailbox nicht gefunden", outfd);
			return 0;
		}
		setzen = lseek(oeffnen, fie->seekpos, SEEK_SET);
		if(setzen < 0) {
			perror("Konnte nicht setzen");
			sendErrorMessage("Mailbox fehlerhaft", outfd);
			return 0;
		}
		
		/*
		char message[fie->size + 6 + strlen(" ") + strlen("octets\r\n")];
		memset(message, '\0', fie->size + strlen(" ") + strlen("octets"));
		*/
		
		char message[15000];
		memset(message, '\0', 15000);
		
		
		sprintf(message, "%d octets\r\n", fie->size);
		char retrBuffer[fie->size];
		
		memset(retrBuffer, '\0', fie->size);
		
		lesen = read(oeffnen, retrBuffer, fie->size);
		if(lesen < 0) {
			perror("Fehler beim Lesen einer Mailbox");
			sendErrorMessage("Mailbox fehlerhaft", outfd);
			return 0;
		}
		strcat(message, retrBuffer);
		strcat(message, "\r\n.");
		sendSuccessMessage(message, outfd);
	} else {
		sendErrorMessage("Konnte Eintrag nicht finden", outfd);
	}
	free(userstat);
	return 1;
}

void handleDele(DialogRec *d, int outfd) {
	int nr = -1;
	FileIndexEntry *fie = NULL;
	
	/* FileIndex updaten */
	//fi_dispose(fi);
	initFileIndex();
	
	d->param[strlen(d->param)-1] = '\0';
	nr = atoi(d->param);
	fie = fi_find(fi, nr);
	if(fie != NULL) {
		fie->del_flag = 1;
		fi->nEntries = fi->nEntries-1;
		fi->totalSize=fi->totalSize-fie->size;
		sendSuccessMessage("Zum Loeschen vorgemerkt", outfd);
	} else {
		sendErrorMessage("Eintrag konnte nicht gefunden werden", outfd);
	}
}

void handleRset(int outfd) {
	FileIndexEntry *copy = NULL;
	
	/* FileIndex updaten */
	fi_dispose(fi);
	fi = NULL;
	initFileIndex();
	
	/*
	copy = fi->entries;
	while(fi->entries != NULL) {
		fi->entries->del_flag = 0;
		fi->entries = fi->entries->next;
	}
	fi->entries = copy;
	*/
	sendSuccessMessage("Alles del_flags zurueckgesetzt", outfd);
}

void handleQuit(int outfd) {
	char sperrName[strlen(getMailbox()->value) + strlen("_lock")];
	zustand = 0;
	strcpy(current_user, "");
	sendSuccessMessage("User logged out.", outfd);
	fi_compactify(fi);
	fi_dispose(fi);
	/* Sperrdatei entfernen */

	strcpy(sperrName, getMailbox()->value);
	strcat(sperrName, "_lock");
	remove(sperrName);
}

int process_pop3(int infd, int outfd) {	
	printf("Prozess-ID: %d\n", getpid());
	ProlResult validated;
	DialogRec *related;
	int lesen;
	int handle;
	int i=0;
	
	if( (lesen = read(infd, buffer, READLEN) < 0) ) {
		perror("POP3: Fehler beim Lesen!"); 
		exit(2);
	}
	
	printf("Gelesen: %s\n", buffer);
	
	for(int i=0;i<strlen(buffer);i++)  {
		buffer[i] = tolower(buffer[i]);
	}
	i=0;
	
	validated = processLine(buffer, zustand, dialog);
	related = validated.dialogrec;
	
	if(validated.failed == 1 || related->is_valid == 0) {
		sendErrorMessage("Operation ungueltig", outfd);
	} else {
		switch(zustand) {
			case 0: /* User */
				if( (strcmp(related->command, "user") == 0) || (strcmp(related->command, "USER") == 0) ) {
					handle = handleUser(related);
					if(handle == 1) {
						strcpy(current_user, related->param);
						//current_user[strlen(current_user)] = '\n';
						sendSuccessMessage("", outfd);
						zustand = 1;
						break;
					} else {
						sendErrorMessage("Error", outfd);
						break;
					}
				}
			case 1: /* Passwort */
				if( (strcmp(related->command, "pass") == 0) ||  (strcmp(related->command, "PASS") == 0)) {
					current_user[strlen(current_user)] = '\0';
					handle = handlePassword(related);
					if(handle == 1) {
						sendSuccessMessage("User logged in.", outfd);
						zustand = 2;	
					} else {
						sendErrorMessage("Error", outfd);
					}
					printf("Zustand: %d\n", zustand);
					break;
				}
				sendErrorMessage("Fehler", outfd);
			case 2: /* Noop, list, stat, retr */
				if( (strcmp(related->command, "stat") == 0) || (strcmp(related->command, "STAT") == 0) ) {
					printf("<<stat");
					handleStat(related, outfd);
					break;
				}
				if( (strcmp(related->command, "list") == 0) || (strcmp(related->command, "LIST") == 0)) {
					handleList(related, outfd);
					break;
				}
				if( (strcmp(related->command, "retr") == 0) || (strcmp(related->command, "RETR") == 0)) {
					handleRetr(related, outfd);
					break;
				}
				if( (strcmp(related->command, "noop") == 0) || (strcmp(related->command, "NOOP") == 0)) {
					sendSuccessMessage("", outfd);
					break;
				}
				
				if( strcmp(related->command, "dele") == 0 || strcmp(related->command, "DELE") == 0) {
					handleDele(related, outfd);
					break;
				}
				
				if( strcmp(related->command, "rset") == 0 || strcmp(related->command, "RSET") == 0) {
					handleRset(outfd);
					break;
				}
				
				if( strcmp(related->command, "quit") == 0 || strcmp(related->command, "QUIT") == 0) {
					handleQuit(outfd);
					return -1;
					break;
					close(fi->filepath);
				}
				sendErrorMessage("Unknown or wrong command.", outfd);
		}
	}
	/* free(related); */
	return zustand;
}

/*
int main(void) {
	
	int datei = open("testuser", O_RDONLY, 0644);
	process_pop3(datei, stdout);
	close(datei);
	
	datei = open("testpw", O_RDONLY, 0644);
	process_pop3(datei, stdout);
	close(datei);
	
	datei = open("teststat", O_RDONLY, 0644);
	process_pop3(datei, stdout);
	close(datei);
	
	return 0;
}
*/
