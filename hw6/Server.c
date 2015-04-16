//Server.c
//Steven Miller
//slm115
//eecs338
//as6



#include <rpc/rpc.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "MailBox.h"
#include "MailBox_common.h"

typedef struct MESSAGE_LIST_ENTRY_STRUCT
{
	message_t messageContents;
	int user;
} messageListEntry_t;

//array of all connected users
user_t * connectedUsers[MAX_USERS];
messageListEntry_t * messageList[MAIL_BOX_SIZE];
int messageCount = 0;

int getNextUserId(){
	int i;
	for(i = 0; i < MAX_USERS; i++){
		if(connectedUsers[i] == NULL){
			return i;
		}
	}
	return ERROR;
}

//the user ID that is sent as part of the user_t argument is ignored;
//the server assigns the client a user id and returns it
//further requests should use this user id
int * start_1_svc(user_t * user, struct svc_req *rqstp){
	//if(connectedUsers == NULL) connectedUsers = calloc(MAX_USERS, sizeof(void *));
	printf("%lld: New user! Name=%s\n", (long long)time(NULL), user->name);
	static int userId = 0;
	userId = getNextUserId();
	if(userId == ERROR){//too many users already connected
		printf("%lld: \tCould not allocate user id to new user %s, too many users connected!\n", (long long)time(NULL), user->name);
		userId = ERROR_TOO_MANY_USERS;
	} else {
		user_t * newUser = malloc(sizeof(user_t));
		newUser->id = userId;
		strncpy(newUser->name, user->name, HOST_NAME_LENGTH);
		//newUser->name = user->name;
		connectedUsers[userId] = newUser;
		printf("%lld: \tUser %s(%d) successfully connectd!\n", (long long)time(NULL), user->name, userId);
	}
	return &userId;
}

int * quit_1_svc(user_t * user, struct svc_req *rqstp){
	static int ret = 0;
	printf("%lld: User %s(%d) has requested to quit.\n", (long long)time(NULL), user->name, user->id);
	if(connectedUsers[user->id] == NULL){
		printf("%lld: User %s(%d) tried to quit, but user id wasn't found!\n", (long long)time(NULL), user->name, user->id);
		ret = ERROR_USER_ID_NOT_FOUND;
	}else if(strcmp(connectedUsers[user->id]->name, user->name) != 0){
		printf("%lld: User %s(%d) tried to quit, but user name doesn't match (connected user name=%s)\n", (long long)time(NULL), user->name, user->id, connectedUsers[user->id]->name);
		ret = ERROR_USER_NAME_ID_NO_MATCH;
	} else {
		free(connectedUsers[user->id]);
		connectedUsers[user->id] = NULL;
		printf("%lld: User %s(%d) has successfully quit.\n", (long long)time(NULL), user->name, user->id);
		ret = ERROR_OK;
	}
	return &ret;
	
}

int * insert_message_1_svc(messageInsArgs_t *args, struct svc_req *rqstp) {
	static int  result;
	int messageNumber = args->messageNumber;
	if(messageNumber < 0 || messageNumber >= MAIL_BOX_SIZE){
		printf("%lld: User %s(%d) attempted to insert a message with an invalid index (%d).\n", (long long)time(NULL), args->user.name, args->user.id, args->messageNumber);
		result = ERROR_INVALID_MESSAGE_NUMBER;
	} else if(messageList[args->messageNumber] == NULL){
		printf("%lld: User %s(%d) inserted a message at unused index %d with content \"%s\"\n", 
			(long long)time(NULL), args->user.name, args->user.id, args->messageNumber, args->message);
		messageList[messageNumber] = malloc(sizeof(messageListEntry_t));
		strcpy(messageList[messageNumber]->messageContents, args->message);
		messageList[messageNumber]->user = args->user.id;
		messageCount++;
		result = ERROR_OK;
	}else{
		if(messageList[args->messageNumber]->user != args->user.id){
			printf("%lld: User %s(%d) tried to overwrite the message at index %d, but it belongs to user %d.", 
				(long long)time(NULL), args->user.name, args->user.id, args->messageNumber, messageList[args->messageNumber]->user);
			result = ERROR_PERMISSION;
		} else {
			printf("%lld: User %s(%d) overwrote the message at index %d with content \"%s\"\n", 
				(long long)time(NULL), args->user.name, args->user.id, args->messageNumber, args->message);
			free(messageList[messageNumber]);
			messageList[messageNumber] = malloc(sizeof(messageListEntry_t));
			strcpy(messageList[messageNumber]->messageContents, args->message);
			messageList[messageNumber]->user = args->user.id;
			result = ERROR_OK;
		}
	}
	return &result;
}

messageWithCode_t * retrieve_message_1_svc(messageRetArgs_t *args, struct svc_req *rqstp){
	static messageWithCode_t result;
	
	int messageNumber = args->messageNumber;
	user_t user = args->user;
	
	if(messageNumber < 0 || messageNumber >= MAIL_BOX_SIZE){
		printf("%lld: User %s(%d) requested a message with an invalid index (%d).\n", (long long)time(NULL), user.name, user.id, messageNumber);
		result.status = ERROR_INVALID_MESSAGE_NUMBER;
		memset(result.message, 0, MESSAGE_LENGTH);
	} else if (messageList[messageNumber] == NULL){
		printf("%lld: User %s(%d) requested message %d, but it does not exist.\n", (long long)time(NULL), user.name, user.id, messageNumber);
		result.status = ERROR_MESSAGE_NOT_FOUND;
		memset(result.message, 0, MESSAGE_LENGTH);
	} else if (messageList[messageNumber]->user != user.id){
		printf("%lld: User %s(%d) requested message %d, but it belongs to user %d.\n", (long long)time(NULL), user.name, user.id, messageNumber, messageList[messageNumber]->user);
		result.status = ERROR_PERMISSION;
		memset(result.message, 0, MESSAGE_LENGTH);
	}else{
		printf("%lld: User %s(%d) has requested message %d.\n", (long long)time(NULL), user.name, user.id, messageNumber);
		strncpy(result.message, messageList[messageNumber]->messageContents, MESSAGE_LENGTH);
		result.status = ERROR_OK;
	}
	return &result;
}

messageListRet_t * list_all_messages_1_svc(user_t *user, struct svc_req *rqstp) {
	static messageListRet_t  result;
	
	printf("%lld: User %s(%d) has requested all messages.\n", (long long)time(NULL), user->name, user->id);
	
	int outi = 0;
	int i;
	for(i = 0; i<MAIL_BOX_SIZE; i++){
		if(messageList[i] != NULL && messageList[i]->user == user->id){
			strncpy(result.messages[outi], messageList[i]->messageContents, MESSAGE_LENGTH);
			result.messageNumbers[outi] = i;
			outi++;
		}
	}
	
	result.messageCount = outi;
	
	printf("%lld: \tSending user %s(%d) %d message(s).\n", (long long)time(NULL), user->name, user->id, result.messageCount);

	return &result;
}

int * delete_message_1_svc(messageDelArgs_t *args, struct svc_req *rqstp) {
	static int result;
	
	int messageNumber = args->messageNumber;
	user_t user = args->user;
	
	if(messageNumber < 0 || messageNumber >= MAIL_BOX_SIZE){
		printf("%lld: User %s(%d) tried to delete a message with an invalid index (%d).\n", (long long)time(NULL), user.name, user.id, messageNumber);
		result = ERROR_INVALID_MESSAGE_NUMBER;
	} else if (messageList[messageNumber] == NULL){
		printf("%lld: User %s(%d) tried to delete message %d, but it does not exist.\n", (long long)time(NULL), user.name, user.id, messageNumber);
		result = ERROR_MESSAGE_NOT_FOUND;
	} else if (messageList[messageNumber]->user != user.id){
		printf("%lld: User %s(%d) tried to delete message %d, but it belongs to user %d.\n", (long long)time(NULL), user.name, user.id, messageNumber, messageList[messageNumber]->user);
		result = ERROR_PERMISSION;
	}else{
		free(messageList[messageNumber]);
		messageList[messageNumber] = NULL;
		messageCount--;
		printf("%lld: User %s(%d) has deleted message %d.\n", (long long)time(NULL), user.name, user.id, messageNumber);
		result = 0;
	}
	
	return &result;
}


