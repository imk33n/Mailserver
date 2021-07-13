#include "database.h"
#include "pop3.h"
#include "smtp.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h> 

#define PORT 9999
#define MAX 10

DBRecord fbPopHost = { "host", "pop3", "localhost" };
DBRecord fbPopPort = { "port", "pop3", "8110" };


int main(void) {
	int pop3socket;
	int pop3Client;
	int smtpsocket;
	int smtpClient;
	int biggestSocket;
	
	int len;
	int binden;
	int zuhoeren;
	int schliessen;
	int enable=1;
	int pid;
	int pop3_status;
	int server_running = 1;
	int thread, threadCounter=0;
	
	/* Infos ueber die Kommunikationspartner :) */
	struct sockaddr_in pop3server;
	struct sockaddr_in smtpServer;
	struct sockaddr_in pop3client;
	struct sockaddr_in smtpclient;
	
	fd_set socketSet;
	pthread_t tid[MAX];

	char database_path[] = "users.dat";

	void *smtpThread(void *args) {
	
		int smtpClient = *((int *) args); 
		int server_running = 1;
		int zustand;
		sendMessage("220", "localhost", smtpClient);
		while(server_running == 1) {
			switch( (zustand = process_smtp(smtpClient, smtpClient)) ) {
				case -1:
					server_running = 0;
					break;
			}
		}
		close(smtpClient);
		return 0;
	}
	/* POP3-Socket erstellen */
	pop3socket = socket(AF_INET, SOCK_STREAM, 0);
	if(pop3socket < 0) {
		perror("Fehler beim Erstellen des Sockets!");
	}

	/* IP wiederverwenden */
	if(setsockopt(pop3socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
		perror("Fehler beim Wiederverwenden der IP");
	}

	printf("****************************************************************\n");
	printf("****************** Leons Mailserver :-) ************************\n");
	printf("****************************************************************\n");

	/* IP und Port festlegen mithilfe von Bind */
	
		/* Hole die Sachen aus der Datenbank */
	DBRecord *pop3Host = calloc(1, sizeof(DBRecord));
	strcpy(pop3Host->key, "host");
	strcpy(pop3Host->cat, "pop3");
	db_search(database_path, 0, pop3Host);
	
	DBRecord *pop3Port = calloc(1, sizeof(DBRecord));
	strcpy(pop3Port->key, "port");
	strcpy(pop3Port->cat, "pop3");
	db_search(database_path, 0, pop3Port);
		
	printf("=> Starte POP3-Server mit %s@%s\n", pop3Host->value, pop3Port->value);
	
	memset(&pop3server, 0, sizeof(struct sockaddr_in)); 
	pop3server.sin_family = AF_INET;
	
	inet_aton(pop3Host->value, pop3server.sin_addr.s_addr);
	pop3server.sin_port = htons(atoi(pop3Port->value));
	
	free(pop3Host);
	free(pop3Port);
	
	binden = bind(pop3socket, (struct sockaddr*)&pop3server, sizeof(pop3server));
	if(binden < 0) {
		perror("Binden fehlgeschlaen");
		exit(9);
	}
	
	/* SMTP-Socket erstellen */
	
	smtpsocket = socket( AF_INET, SOCK_STREAM, 0 );
	
	/* IP wiederverwenden */
	if(setsockopt(smtpsocket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
		perror("Fehler beim Wiederverwenden der IP");
	}
	
	/* Kram holen */
	
	DBRecord *smtpHost = calloc(1, sizeof(DBRecord));
	strcpy(smtpHost->key, "host");
	strcpy(smtpHost->cat, "smtp");
	db_search(database_path, 0, smtpHost);
	
	DBRecord *smtpPort = calloc(1, sizeof(DBRecord));
	strcpy(smtpPort->key, "port");
	strcpy(smtpPort->cat, "smtp");
	db_search(database_path, 0, smtpPort);
	
	printf("=> Starte SMTP-Server mit %s@%s\n", smtpHost->value, smtpPort->value);
	
	memset( &smtpServer, 0, sizeof (smtpServer));
	smtpServer.sin_family = AF_INET; 
	
	inet_aton(smtpHost->value, smtpServer.sin_addr.s_addr);
	smtpServer.sin_port = htons(atoi(smtpPort->value));
	/*
	smtpServer.sin_addr.s_addr = htonl(INADDR_ANY);
	smtpServer.sin_port = htons(9998);
	*/

	free(smtpHost);
	free(smtpPort);
	
	binden = bind(smtpsocket, (struct sockaddr*)&smtpServer, sizeof(smtpServer));

	/* Warteschlange einrichten */
	zuhoeren = listen(pop3socket, 5);
	if(zuhoeren == -1) {
		perror("Fehler beim Listen");
	}
	
	zuhoeren = listen(smtpsocket, 5);
	
	FD_ZERO(&socketSet);
	
	/* Verbindung(en) akzeptieren */
	for(;;) {
		
		FD_SET(pop3socket, &socketSet);
        FD_SET(smtpsocket, &socketSet);
		
		len = sizeof(struct sockaddr);
		
		if(pop3socket>smtpsocket) {
			biggestSocket = pop3socket;
		} else {
			biggestSocket = smtpsocket;
		}
		
		select(biggestSocket + 1, &socketSet, NULL, NULL, NULL);
		
		if(FD_ISSET(pop3socket, &socketSet)) {
	
			pop3Client = accept(pop3socket, (struct sockaddr*)&pop3client, &len);
		
			if(pop3Client < 0) {
				perror("Fehler beim Accepten der Verbindung");
			}
					
			if(	(pid = fork()) < 0) {
				perror("Fork fehlgeschlagen");
				exit(8);
			} else if(pid == 0) {
				close(pop3socket);
				printf("Prozess: %d\n", getpid());
				/* Begruessen */
				sendSuccessMessage("", pop3Client);
				while(server_running == 1) {
					switch( (pop3_status = (process_pop3(pop3Client, pop3Client))) ) {
						printf("Status: %d\n", pop3_status);
						case -1: 
							server_running = 0;
							break;
					}
				}
				if(server_running == 0) {
					printf("Connection beenden\n");
					schliessen = close(pop3Client);
					if(schliessen < 0) {
						perror("Fehler beim Schließen des Sockets");
						exit(9);
					}				
					exit(0);
				}
			} else {
				wait(NULL);
				close(pop3Client);
			}
		} else if(FD_ISSET(smtpsocket, &socketSet)) {
			smtpClient = accept(smtpsocket, (struct sockaddr*)&smtpclient, &len);
			if(threadCounter < MAX) {
				thread = pthread_create(&tid[threadCounter], NULL, smtpThread, &smtpClient);
				if(thread < 0) {
					perror("Thread konnte nicht gestartet werden");
					exit(10);
				}
				threadCounter = threadCounter+1;
			}
		}
		/* Vaterprozess muss den neuen Socket noch schließen */
	}
	return 0;
}
