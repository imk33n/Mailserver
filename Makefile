mailserver: 
	gcc -o leonsmailserver -g -Wall -pedantic -pthread mailserver_leon.c smtp.c pop3.c fileindex.c linebuffer.c dialog.c database.c

configtool:
	gcc -o configtool -g -Wall -pedantic configtool.c 

run:
	./leonsmailserver 

debug:
	nemiver ./leonsmailserver

clean:
	rm leonsmailserver 


