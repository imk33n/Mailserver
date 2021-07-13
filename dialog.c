#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dialog.h"

int validate_hasParam(DialogRec *d) {
	if(strcmp(d->param, "") != 0 && strlen(d->param) > 0) {
		return 1;
	} else {
		printf("hasparam error\n");
		return 0;
	}
}

int validate_digitempty(DialogRec *d) {
	int len = strlen(d->param);
	int result = 0;
	char copy[strlen(d->param)];
	strcpy(copy, d->param);
	/* Leer ! */
	if(d->param[0] == '\n') {
		d->param[0] = '\0';
		d->param[1] = '\0';
		return 1;
	}
	if(strlen(d->param) == 0) {
		return 1;
	}

	if(strlen(d->param) > 0) {
		d->param[len-1] = '\0';
		d->param[len-2] = '\0';
		len = atoi(d->param);
		strcpy(d->param, copy);
		return 1;
	}
	return 0;
}

int validate_noparam(DialogRec *d) {
	d->param[0] = '\0';
	printf("%s", d->param);
	if(strlen(d->param) == 0) {
		return 1;
	} else {
		printf("noparam error\n");
		return 0;
	}
}

int validate_digit(DialogRec *d) {
	return 1;
	if(strlen(d->param) > 0 && isdigit(d->param)) {
		return 1;
	} else {
		printf("digit error");
		return 0;
	}
}


DialogRec *findDialogRec(char *command, DialogRec dialogspec[]) {
	DialogRec *result = NULL;
	int i = 0;
	
	while(strcmp(dialogspec[i].command, "") != 0) {
		if(strstr(command, dialogspec[i].command) != NULL) {
			result = &dialogspec[i];
			break;
		}
		i=i+1;
	} 
	return result;
}

ProlResult processLine(char line[LINEMAX], int state, DialogRec dialogspec[]) {
	ProlResult result;
	DialogRec *found = findDialogRec(line, dialogspec);
	/* char infoText[GRMSGMAX] = "Leider konnte kein entsprechendes Dialogrec gefunden werden :/"; */
	if(found == NULL) {
		result.failed = 1;
		result.dialogrec = NULL;
	} else if(found != NULL && found->state != state) {
		result.failed = 1;
		result.dialogrec = found;
	} else {
		result.failed = 0;
		strcpy(found->param, line+strlen(found->command)+1);
		if(found->validator != NULL) {
			found->is_valid = found->validator(found);
		} else {
			found->is_valid = 1;
		}
		result.dialogrec = found;
	}
	
	memset(line, 0, strlen(line));
	return result;
}

/*
int main(void) {
	DialogRec dialog[] = { 
		/* command, 		param,	state,	nextstate,	validator 
		{ "BEGIN", 			"", 	0, 		1, 			 }, 
		{ "kuehlschrank",	"", 	1, 		1,			 }, 
		{ "fernseher", 		"", 	1, 		1,			 }, 
		{ "toaster", 		"", 	1,		1}, 
		{ "END", 			"", 	1, 		2,			 }, 
		{ "" }
	};
	
	DialogRec *find = findDialogRec("toaster", dialog);
	printf("DialogRec: %s\n", find->command);
	
	ProlResult result = processLine("BEGIN off", 0, dialog);
	find = result.dialogrec;
	printf("DialogRec: %s, Param: %s\n", find->command, find->param);
	
	return 0;
}
*/
