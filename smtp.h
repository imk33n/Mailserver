#ifndef SMTP_H
#define SMTP_H

void sendMessage(char code[], char msg[], int outfd);
int process_smtp(int infd, int outfd);

#endif
