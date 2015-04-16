//Client.c
//Steven Miller
//slm115
//eecs338
//as6



#include "MailBox.h"
#include "MailBox_common.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int processInput(char *input, CLIENT *clnt, user_t *user);
char * parseErrorCode(int code);

int main(int argc, char * argv[]) {
	if (argc < 2) {
		printf ("usage: %s <server_hostname>\n", argv[0]);
		return 1;
	}
	char * hostname = argv[1];
	char localhost[HOST_NAME_LENGTH];
	if(gethostname(localhost, 20)!=0){
		perror("Error getting local hostname!");
		return 1;
	}
	printf("I am a client running on %s\n", localhost);
	printf("Connecting to %s...\n", hostname);
	
	CLIENT *clnt = clnt_create (hostname, MAIL_BOX_PRG, MAIL_BOX_VERS, "udp");
	if (clnt == NULL) {
		clnt_pcreateerror (hostname);
		return 1;
	}
	user_t user;
	strncpy(user.name, localhost, HOST_NAME_LENGTH);
	//user.name = localhost;
	printf("Sending initial Start() command...\n");
	int * res = start_1(&user, clnt);
	if(*res==ERROR_TOO_MANY_USERS){
		perror("Could not connect to server: too many users.");
		return 1;
	}
	user.id = *res;
	printf("Recieved user id %d from the server.\n", user.id);
	
	printf("\nReady. Enter a command, or \'help\' for a list of commands.\n");
	
	
	char input[256];

	do{
		printf(">");
		fgets(input, 256, stdin);
	} while(processInput(input, clnt, &user));
	
	return 0;
}

int processInput(char *input, CLIENT *clnt, user_t *user){
	//first things first, we ned to remove the newline character from the end of the input string by setting it to a NULL character
	int inputlen = strlen(input);
	if(inputlen>=1) input[inputlen-1] = '\0';//trim off the newline... the saftey check isn't really needed, but whatever
	
	char * command = strtok(input, " ");
	int len = strlen(command) + 1; //keeps track of how far thrugh the string we've come so far
	if(strcmp(command, "insert") == 0){
		char *arg1 = strtok(NULL, " ");
		len+=strlen(arg1) + 1;
		int msgNum = atoi(arg1);
		printf("Sending insert command with messageNumber=%d, and message=%s\n", msgNum, input+len);
		//set up the args struct
		messageInsArgs_t args;
		args.user = *user;
		args.messageNumber = msgNum;
		strcpy(args.message, input + len); //some pointer magic to cut off the begining of the string
		printf("Response: %s\n", parseErrorCode(*insert_message_1(&args, clnt)));
	} else if(strcmp(command, "retrieve") == 0){
		char *arg1 = strtok(NULL, " ");
		int msgNum = atoi(arg1);
		printf("Sending retrieve command with messageNumber=%d\n", msgNum);
		//set up the args struct
		messageRetArgs_t args;
		args.user = *user;
		args.messageNumber = msgNum;
		messageWithCode_t * result = retrieve_message_1(&args, clnt);
		printf("Response: %s\n", parseErrorCode(result->status));
		if(result->status == ERROR_OK){
			printf("\tMessage: %s\n", result->message);
		}
	} else if(strcmp(command, "delete") == 0){
		char *arg1 = strtok(NULL, " ");
		int msgNum = atoi(arg1);
		printf("Sending delete command with messageNumber=%d\n", msgNum);
		//set up the args struct
		messageDelArgs_t args;
		args.user = *user;
		args.messageNumber = msgNum;
		printf("Response: %s\n", parseErrorCode(*delete_message_1(&args, clnt)));
	} else if(strcmp(command, "list") == 0){
		printf("Sending list_all command\n");
		messageListRet_t * retval = list_all_messages_1(user, clnt);
		printf("Response:\n");
		int i;
		for(i = 0; i<retval->messageCount; i++){
			printf("\t%d: \"%s\"\n", retval->messageNumbers[i], retval->messages[i]);
		}
	} else if(strcmp(command, "quit") == 0){
		printf("Sending quit command\n");
		printf("Response: %s\n", parseErrorCode(*quit_1(user, clnt)));
		return FALSE;
	} else if(strcmp(command, "help") == 0){
		printf("Command list:\n");
		printf("\tinsert <message-number> <message>\n");
		printf("\tretrieve <message-number>\n");
		printf("\tdelete <message-number>\n");
		printf("\tlist\n");
		printf("\tquit\n\n");
	} else {
		printf("Invaid command; enter \'help\' for a list of commands.\n");
	}
	return TRUE;
}

char * parseErrorCode(int code){
	switch(code){
		case ERROR_OK: return "Success";
		case ERROR_PERMISSION: return "Error: Permission Denied; attepmted to access a message that belongs to another user";
		case ERROR_INVALID_MESSAGE_NUMBER: return "Error: Invalid Message Number";
		case ERROR_MESSAGE_NOT_FOUND: return "Error: Message not found";
		case ERROR_USER_ID_NOT_FOUND: return "Error: User ID not found";
		case ERROR_USER_NAME_ID_NO_MATCH: return "Error: the supplied user name and ID did not match";
		case ERROR_TOO_MANY_USERS: return "Error: server is full; cannot accept any more users at this time";
		default: return "Unknown response code";
	}
}


