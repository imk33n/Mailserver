#ifndef POP3_H
#define POP3_H

void sendSuccessMessage(char successMsg[], int outfd);
void sendErrorMessage(char errorMsg[], int outfd);
int process_pop3(int infd, int outfd);

#endif
