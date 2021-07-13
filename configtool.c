#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "database.h"
#include <string.h>

int db_list(const char *path, int outfd, int (*filter)(DBRecord *rec, const void *data), const void *data) {
	int oeffnen;
	int lesen;
	int i;
	char buffer_key[DB_KEYLEN];
	char buffer_cat[DB_CATLEN]; /*Meow*/
	char buffer_value[DB_VALLEN];
	
	oeffnen = open(path, O_RDONLY);	
	if(oeffnen < 0) {
		perror("Datei konnte nicht geoeffnet werden");
		exit(1);
	}
	
	if(filter != NULL && data != NULL) {
		 /* Es gibt Filter :( */
	} else {
		/* Es gibt keinen Filter :) */
		while(1) {
			/* KEY lesen */
			lesen = read(oeffnen, buffer_key, DB_KEYLEN);
			if(lesen < 0) {
				perror("Beim Lesen ist etwasschief gelaufen :(");
				exit(2);
			}
			if(lesen == 0) {
				break;
			}
			for(i=strlen(buffer_key);i<DB_KEYLEN;i++) {
				buffer_key[i] = ' ';
			}
			write(outfd, buffer_key, DB_KEYLEN);
		
			/* CAT lesen */
			lesen = read(oeffnen, buffer_cat, DB_CATLEN);
			if(lesen < 0) {
				perror("Beim Lesen ist etwasschief gelaufen :(");
				exit(2);
			}
			if(lesen == 0) {
				break;
			}
			for(i=strlen(buffer_cat);i<DB_CATLEN;i++) {
				buffer_cat[i] = ' ';
			}
			write(outfd, "|", 1);
			write(outfd, buffer_cat, DB_KEYLEN);
			
			/* VALUE lesen */
			lesen = read(oeffnen, buffer_value, DB_VALLEN);
			if(lesen < 0) {
				perror("Beim Lesen ist etwasschief gelaufen :(");
				exit(2);
			}
			if(lesen == 0) {
				break;
			}
			for(i=strlen(buffer_value);i<DB_VALLEN;i++) {
				if(i+1 == DB_VALLEN) {
					buffer_value[i] = '\n';
				} else {
					buffer_value[i] = ' ';
				}
			}
			write(outfd, "|", 1);
			write(outfd, buffer_value, DB_KEYLEN);
			write(outfd, "\n", 1);
		}
	}
	close(oeffnen);
	close(lesen);
	return 0;
}

int db_search(const char *filepath, int start, DBRecord *record) {
	int oeffnen;
	int lesen;
	int index = -1; 
	int setzen = 0;
	int gesetzt = 0;
	int counter = 1;
	
	char buffer_key[DB_KEYLEN];
	char buffer_cat[DB_CATLEN];
	char buffer_value[DB_VALLEN];
	
	oeffnen = open(filepath, O_RDONLY);	
	if(oeffnen < 0) {
		perror("Datei konnte nicht geoeffnet werden");
		exit(1);
	}
	
	setzen = lseek(oeffnen, start * sizeof(DBRecord), SEEK_SET);
	if(setzen < 0) {
		perror("Fehler beim Setzen (lseek)");
		exit(4);
	}
	
	while(1) {
		lesen = read(oeffnen, buffer_key, DB_KEYLEN);
		if(lesen < 0) {
			perror("Beim Lesen ist etwas schief gelaufen :(");
			exit(2);
			return -42;
		}
		lesen = read(oeffnen, buffer_cat, DB_CATLEN);
		if(lesen < 0) {
			perror("Beim Lesen ist etwas schief gelaufen :(");
			exit(2);
			return -42;
		}
		lesen = read(oeffnen, buffer_value, DB_VALLEN);
		if(lesen < 0) {
			perror("Beim Lesen ist etwas schief gelaufen :(");
			exit(2);
			return -42;
		}
		
		/* Herausfinden bei welchem Record wir sind */
		sscanf(buffer_key, "%d", &index);

		if(strlen(buffer_key) > 0) {
			if(strlen(record->key) > 0 && strlen(record->cat) > 0 && strcmp(buffer_key, record->key) == 0 && strcmp(buffer_cat, record->cat) == 0) {
				gesetzt = 1;
			} else if(strlen(record->key) > 0 && strlen(record->cat) == 0 && strcmp(buffer_key, record->key) == 0) {
				gesetzt = 1;
			} else if(strlen(record->cat) > 0 && strlen(record->key) == 0 && strcmp(buffer_cat, record->cat) == 0) {
				gesetzt = 1;
			} else if(strlen(record->key) == 0 && strlen(record->cat) == 0) {
				gesetzt = 1;
			}
			
			if(gesetzt == 1) {
				sscanf(buffer_key, "%s", &record->key);
				sscanf(buffer_cat, "%s", &record->cat);
				sscanf(buffer_value, "%s", &record->value);
			}
			
		}
		
		if(lesen == 0 && gesetzt == 1) {
			index = counter;
			break;
		} else if(lesen == 0 && gesetzt == 0) {
			index = -1;
			break;
		} else if(lesen != 0 && gesetzt == 1) {
			index = counter;
			break;
		}
		counter = counter + 1;
	}
	close(oeffnen);
	close(lesen);
	return index;
}

int db_get(const char *filepath, int index, DBRecord *result) {
	int oeffnen;
	int lesen;
	int currIndex;
	int ergebnis = -1;
	
	char buffer_key[DB_KEYLEN];
	char buffer_cat[DB_CATLEN];
	char buffer_value[DB_VALLEN];
	
	oeffnen = open(filepath, O_RDONLY);	
	if(oeffnen < 0) {
		perror("Datei konnte nicht geoeffnet werden");
		exit(1);
	}
	
	while(1) {
		lesen = read(oeffnen, buffer_key, DB_KEYLEN);
		if(lesen < 0) {
			perror("Beim Lesen ist etwass chief gelaufen :(");
			exit(2);
		}
		lesen = read(oeffnen, buffer_cat, DB_CATLEN);
		if(lesen < 0) {
			perror("Beim Lesen ist etwas schief gelaufen :(");
			exit(2);
		}
		lesen = read(oeffnen, buffer_value, DB_VALLEN);
		if(lesen < 0) {
			perror("Beim Lesen ist etwas schief gelaufen :(");
			exit(2);
		}
		
		sscanf(buffer_key, "%d", &currIndex);
		
		if(index == currIndex) {
			strcpy(result->key, buffer_key);
			strcpy(result->cat, buffer_cat);
			strcpy(result->value, buffer_value);
			ergebnis = 0;
			break;
		}
		if(lesen == 0) {
			break;
		}
	}
	close(oeffnen);
	close(lesen);
	return ergebnis;
}

int db_put(const char *filepath, int index, const DBRecord *record) {
	int oeffnen;
	int lesen;
	int schreiben;
	int setzen;
	int currIndex;
	int found = -1;
	int ergebnis = 0; 
	
	char buffer_key[DB_KEYLEN];
	char buffer_cat[DB_CATLEN];
	char buffer_value[DB_VALLEN];
	
	oeffnen = open(filepath, O_RDWR);
	if(oeffnen < 0) {
		perror("Datei konnte nicht geoeffnet werden");
		exit(1);
	}

	if(index < 0) {
		found = 0;
	}
		
		while(1) {
			lesen = read(oeffnen, buffer_key, DB_KEYLEN);
			if(lesen < 0) {
				perror("Beim Lesen ist etwas schief gelaufen :(");
				exit(2);
			}
			lesen = read(oeffnen, buffer_cat, DB_CATLEN);
			if(lesen < 0) {
				perror("Beim Lesen ist etwas schief gelaufen :(");
				exit(2);
			}
			lesen = read(oeffnen, buffer_value, DB_VALLEN);
			if(lesen < 0) {
				perror("Beim Lesen ist etwas schief gelaufen :(");
				exit(2);
			}
			
			sscanf(buffer_key, "%d", &currIndex);
			
			if(lesen == 0) {
				break;
			}
			
			/* checken ob der hier der ist den wir suchen */
			if(currIndex == index) {
				/* Vorm schreiben  eins zurueck! */
				if(currIndex > 0) {
					setzen = lseek(oeffnen, (currIndex-1) * sizeof(DBRecord) ,SEEK_SET);
					if(setzen < 0) {
						perror("Beim Setzen ist etwas schief gelaufen");
						exit(3);
					}
				}
				schreiben = write(oeffnen, record, sizeof(DBRecord));
				if(schreiben < 0) {
					perror("Beim Schreiben ist etwas schief gelaufen");
					exit(2);
					ergebnis = -1;
					break;
				}
			}
		}
	
	/* Noch nicht enthalten -> anhängen */
	if(index > currIndex || found == 0) {
		schreiben = write(oeffnen, record, sizeof(DBRecord));
		if(schreiben < 0) {
			perror("Beim Schreiben ist etwas schief gelaufen");
			exit(2);
		}
	}
	close(oeffnen);
	close(lesen);
	close(schreiben);
	return ergebnis;
}

int db_update(const char *filepath, const DBRecord *new) {
	int oeffnen;
	int lesen;
	int schreiben;
	int setzen;
	int found = 0;
	int currIndex = 0;
	int ergebnis = -1;
	
	char buffer_key[DB_KEYLEN];
	char buffer_cat[DB_CATLEN];
	char buffer_value[DB_VALLEN];
	
	oeffnen = open(filepath, O_RDWR);
	if(oeffnen < 0) {
		perror("Datei konnte nicht geoeffnet werden");
		exit(1);
	}
	
	while(1) {
		lesen = read(oeffnen, buffer_key, DB_KEYLEN);
		if(lesen < 0) {
			perror("Beim Lesen ist etwas schief gelaufen :(");
			exit(2);
		}
		lesen = read(oeffnen, buffer_cat, DB_CATLEN);
		if(lesen < 0) {
			perror("Beim Lesen ist etwas schief gelaufen :(");
			exit(2);
		}
		lesen = read(oeffnen, buffer_value, DB_VALLEN);
		if(lesen < 0) {
			perror("Beim Lesen ist etwas schief gelaufen :(");
			exit(2);
		}
		
		sscanf(buffer_key, "%d", &currIndex);
		if(strcmp(buffer_cat, new->cat) == 0 && strcmp(buffer_key, new->key) == 0) {	
			sscanf(buffer_key, "%s", &new->key);
			if(currIndex > 0) {
				setzen = lseek(oeffnen, (currIndex-1) * sizeof(DBRecord), SEEK_SET);
				if(setzen < 0) {
					perror("Beim Setzen ist etwas schief gelaufen");
					exit(3);
				}
			}
			
			schreiben = write(oeffnen, new, sizeof(DBRecord));
			if(schreiben < 0) {
				perror("Beim Schreiben ist etwas schief gelaufen");
				exit(2);
			}
			found = 1;
			sscanf(buffer_key, "%d", &ergebnis);
			break;
		}
		
		if(lesen == 0) {
			break;
		}
	}
	
	if(found == 0) {
		currIndex = currIndex + 1;
		schreiben = write(oeffnen, new, sizeof(DBRecord));
		if(schreiben < 0) {
			perror("Beim Schreiben ist etwas schief gelaufen");
			exit(2);
		}
		ergebnis = currIndex;
	}
	
	close(lesen);
	close(schreiben);
	close(setzen);
	close(oeffnen);
	return ergebnis;
}

int db_del(const char *filepath, int index) {
	int oeffnen;
	int newDesc;
	int lesen;
	int schreiben;
	int ergebnis = 0;
	int currIndex = 0;
	
	DBRecord buffer;
	char buffer_key[DB_KEYLEN];
	char buffer_cat[DB_CATLEN];
	char buffer_value[DB_VALLEN];
	
	oeffnen = open(filepath, O_RDWR);
	if(oeffnen < 0) {
		perror("Datei konnte nicht geoeffnet werden");
		exit(1);
	}
	
	newDesc = open("COPYCAT", O_CREAT | O_RDWR,  0644);
	if(newDesc < 0) {
		perror("COPYCAT-Datei konnte nicht geoeffnet/erstellt werden");
		exit(1);
	}
	
	while(1) {
		lesen = read(oeffnen, buffer_key, DB_KEYLEN);
		if(lesen < 0) {
			perror("Beim Lesen ist etwas schief gelaufen :(");
			exit(2);
		}
		lesen = read(oeffnen, buffer_cat, DB_CATLEN);
		if(lesen < 0) {
			perror("Beim Lesen ist etwas schief gelaufen :(");
			exit(2);
		}
		lesen = read(oeffnen, buffer_value, DB_VALLEN);
		if(lesen < 0) {
			perror("Beim Lesen ist etwas schief gelaufen :(");
			exit(2);
		}
		
		if(lesen == 0) {
			break;
		}
		
		sscanf(buffer_key, "%d", &currIndex);
		
		if(currIndex != index) {
			strcpy(buffer.key, buffer_key);
			strcpy(buffer.cat, buffer_cat);
			strcpy(buffer.value, buffer_value);
			schreiben = write(newDesc, &buffer, sizeof(DBRecord));
			if(schreiben < 0) {
				perror("Schreiben in neue Datei fehlgeschlagen");
				exit(2);
			}
		}
	}
	close(oeffnen);
	close(lesen);
	close(schreiben);
	close(newDesc);
	return ergebnis;
}

int testWrite(DBRecord rec[], int anzahl) {
	int datei;
	int writeDatei;

	datei = open("users.dat", O_CREAT|O_WRONLY, 0644);
	
	writeDatei = write(datei, rec, sizeof(DBRecord) * anzahl);
	if(writeDatei < 0) {
		perror("Fehler beim Testschreiben");
		exit(2);
	}
	close(datei);
	return 0;
}

int printRecord(DBRecord *toPrint) {
	printf("[DBRecord] -> key: %s, cat: %s, value: %s\n", toPrint->key, toPrint->cat, toPrint->value);
	return 0;
}

int main(int argc,  char *argv[]) {
    DBRecord testRec[] = {{"joendhard", "password", "test123"}, {"joendhard@biffel.xy", "mailbox", "joendhard.mbox"}, {"joendhard", "mailbox", "joendhard.mbox"}, {"host", "pop3", "localhost"}, {"port", "pop3", "9999"}, {"host", "smtp", "localhost"}, {"port", "smtp", "9998"}};
    if(argc > 1){
        if(strcmp(argv[1],"test") == 0){
            testWrite(testRec, 7); 
        }
        if(strcmp(argv[1],"list") == 0){
            db_list("users.dat", 1, NULL, NULL); 
        }
        if(strcmp(argv[1],"add") == 0){
			DBRecord add;
			strcpy(add.key, argv[2]);
			strcpy(add.cat, argv[3]);
			strcpy(add.value, argv[4]);
           db_update("users.dat", &add);
        }
        
    }
    
}
