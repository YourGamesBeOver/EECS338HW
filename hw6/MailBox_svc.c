/*
 * Please do not edit this file.
 * It was generated using rpcgen.
 */

#include "MailBox.h"
#include <stdio.h>
#include <stdlib.h>
#include <rpc/pmap_clnt.h>
#include <string.h>
#include <memory.h>
#include <sys/socket.h>
#include <netinet/in.h>

#ifndef SIG_PF
#define SIG_PF void(*)(int)
#endif

static void
mail_box_prg_1(struct svc_req *rqstp, register SVCXPRT *transp)
{
	union {
		user_t start_1_arg;
		user_t quit_1_arg;
		messageInsArgs_t insert_message_1_arg;
		messageRetArgs_t retrieve_message_1_arg;
		user_t list_all_messages_1_arg;
		messageDelArgs_t delete_message_1_arg;
	} argument;
	char *result;
	xdrproc_t _xdr_argument, _xdr_result;
	char *(*local)(char *, struct svc_req *);

	switch (rqstp->rq_proc) {
	case NULLPROC:
		(void) svc_sendreply (transp, (xdrproc_t) xdr_void, (char *)NULL);
		return;

	case START:
		_xdr_argument = (xdrproc_t) xdr_user_t;
		_xdr_result = (xdrproc_t) xdr_int;
		local = (char *(*)(char *, struct svc_req *)) start_1_svc;
		break;

	case quit:
		_xdr_argument = (xdrproc_t) xdr_user_t;
		_xdr_result = (xdrproc_t) xdr_int;
		local = (char *(*)(char *, struct svc_req *)) quit_1_svc;
		break;

	case INSERT_MESSAGE:
		_xdr_argument = (xdrproc_t) xdr_messageInsArgs_t;
		_xdr_result = (xdrproc_t) xdr_int;
		local = (char *(*)(char *, struct svc_req *)) insert_message_1_svc;
		break;

	case RETRIEVE_MESSAGE:
		_xdr_argument = (xdrproc_t) xdr_messageRetArgs_t;
		_xdr_result = (xdrproc_t) xdr_messageWithCode_t;
		local = (char *(*)(char *, struct svc_req *)) retrieve_message_1_svc;
		break;

	case LIST_ALL_MESSAGES:
		_xdr_argument = (xdrproc_t) xdr_user_t;
		_xdr_result = (xdrproc_t) xdr_messageListRet_t;
		local = (char *(*)(char *, struct svc_req *)) list_all_messages_1_svc;
		break;

	case DELETE_MESSAGE:
		_xdr_argument = (xdrproc_t) xdr_messageDelArgs_t;
		_xdr_result = (xdrproc_t) xdr_int;
		local = (char *(*)(char *, struct svc_req *)) delete_message_1_svc;
		break;

	default:
		svcerr_noproc (transp);
		return;
	}
	memset ((char *)&argument, 0, sizeof (argument));
	if (!svc_getargs (transp, (xdrproc_t) _xdr_argument, (caddr_t) &argument)) {
		svcerr_decode (transp);
		return;
	}
	result = (*local)((char *)&argument, rqstp);
	if (result != NULL && !svc_sendreply(transp, (xdrproc_t) _xdr_result, result)) {
		svcerr_systemerr (transp);
	}
	if (!svc_freeargs (transp, (xdrproc_t) _xdr_argument, (caddr_t) &argument)) {
		fprintf (stderr, "%s", "unable to free arguments");
		exit (1);
	}
	return;
}

int
main (int argc, char **argv)
{
	register SVCXPRT *transp;

	pmap_unset (MAIL_BOX_PRG, MAIL_BOX_VERS);

	transp = svcudp_create(RPC_ANYSOCK);
	if (transp == NULL) {
		fprintf (stderr, "%s", "cannot create udp service.");
		exit(1);
	}
	if (!svc_register(transp, MAIL_BOX_PRG, MAIL_BOX_VERS, mail_box_prg_1, IPPROTO_UDP)) {
		fprintf (stderr, "%s", "unable to register (MAIL_BOX_PRG, MAIL_BOX_VERS, udp).");
		exit(1);
	}

	transp = svctcp_create(RPC_ANYSOCK, 0, 0);
	if (transp == NULL) {
		fprintf (stderr, "%s", "cannot create tcp service.");
		exit(1);
	}
	if (!svc_register(transp, MAIL_BOX_PRG, MAIL_BOX_VERS, mail_box_prg_1, IPPROTO_TCP)) {
		fprintf (stderr, "%s", "unable to register (MAIL_BOX_PRG, MAIL_BOX_VERS, tcp).");
		exit(1);
	}

	svc_run ();
	fprintf (stderr, "%s", "svc_run returned");
	exit (1);
	/* NOTREACHED */
}
