#include "MailBox_common.h"

typedef char message_t[MESSAGE_LENGTH];

struct messageWithCode_t{
	int status;
	message_t message;
};

struct user_t {
	int id;
	char name[HOST_NAME_LENGTH];
};

struct messageInsArgs_t
{
	user_t user;
	int messageNumber;
	message_t message;
};

struct messageRetArgs_t
{
	user_t user;
	int messageNumber;
};

struct messageListRet_t
{
	int messageCount;
	message_t messages[MAIL_BOX_SIZE];
	int messageNumbers[MAIL_BOX_SIZE];
};

struct messageDelArgs_t
{
	user_t user;
	int messageNumber;
};


program MAIL_BOX_PRG
{
	version MAIL_BOX_VERS
	{
		int START(user_t) = 1;
		int quit(user_t) = 2;
		int INSERT_MESSAGE(messageInsArgs_t) = 3;
		messageWithCode_t RETRIEVE_MESSAGE(messageRetArgs_t) = 4;
		messageListRet_t LIST_ALL_MESSAGES(user_t) = 5;
		int DELETE_MESSAGE(messageDelArgs_t) = 6;
	} = 1;
} = 0xFF1277;
