all:
	make rpcgen
	make server
	make client

rpcgen:
	rpcgen MailBox.x

server:
	gcc -o Server MailBox_svc.c MailBox_xdr.c Server.c -lnsl
	
client:
	gcc -o Client Client.c MailBox_clnt.c MailBox_xdr.c -lnsl
	
fullserver:
	make rpcgen
	make server

fullclient:
	make rpcgen
	make client
	
clean:
	rm -f Server
	rm -f Client
	
