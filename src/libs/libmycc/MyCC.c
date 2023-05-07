#include <MyCC.h>

MyCC_PDU_t *my_request_balance(balance_t *bal)
{
	MyCC_PDU_t *myCC = NULL;
	
	myCC = calloc(1, sizeof(MyCC_PDU_t));

	if(myCC == NULL) return NULL;

	myCC->present = MyCC_PDU_PR_myRequest;
	
	myCC->choice.myRequest.header.cdrServerId = bal->cdr_server_id;
	myCC->choice.myRequest.header.version = MyCCVersion;
	myCC->choice.myRequest.header.transactionId = bal->tid;
	
	myCC->choice.myRequest.header.timestamp = bal->ts;

	myCC->choice.myRequest.header.hostname.size = strlen(bal->host);
	myCC->choice.myRequest.header.hostname.buf = bal->host;
		
	myCC->choice.myRequest.payload.present = MyRequestPayload_PR_balance;	
	myCC->choice.myRequest.payload.choice.balance.callingNumber.e164.size = strlen(bal->clg);
	myCC->choice.myRequest.payload.choice.balance.callingNumber.e164.buf = bal->clg;
	
	return myCC;
}

MyCC_PDU_t *my_response_balance(balance_t *bal)
{
	MyCC_PDU_t *myCC = NULL;
	
	myCC = calloc(1, sizeof(MyCC_PDU_t));

	if(myCC == NULL) return NULL;

	myCC->present = MyCC_PDU_PR_myResponse;
	
	myCC->choice.myResponse.header.cdrServerId = bal->cdr_server_id;
	myCC->choice.myResponse.header.version = MyCCVersion;
	myCC->choice.myResponse.header.transactionId = bal->tid;
	
	myCC->choice.myResponse.header.timestamp = bal->ts;

	myCC->choice.myResponse.header.hostname.size = strlen(bal->host);
	myCC->choice.myResponse.header.hostname.buf = bal->host;
		
	myCC->choice.myResponse.payload.present = MyResponsePayload_PR_balance;
	myCC->choice.myResponse.payload.choice.balance.amount.size = strlen(bal->amount);
	myCC->choice.myResponse.payload.choice.balance.amount.buf = bal->amount;	
	
	return myCC;
}

MyCC_PDU_t *my_request_maxsec(maxsec_t *max)
{
	MyCC_PDU_t *myCC = NULL;
	myCC = calloc(1, sizeof(MyCC_PDU_t));

	if(myCC == NULL) return NULL;

	myCC->present = MyCC_PDU_PR_myRequest;
	
	myCC->choice.myRequest.header.cdrServerId = max->cdr_server_id;
	myCC->choice.myRequest.header.version = MyCCVersion;
	myCC->choice.myRequest.header.transactionId = max->tid;
	
	myCC->choice.myRequest.header.timestamp = max->ts;

	myCC->choice.myRequest.header.hostname.size = strlen(max->host);
	myCC->choice.myRequest.header.hostname.buf = max->host;
		
	myCC->choice.myRequest.payload.present = MyRequestPayload_PR_maxsec;	
	myCC->choice.myRequest.payload.choice.maxsec.callingNumber.e164.size = strlen(max->clg);
	myCC->choice.myRequest.payload.choice.maxsec.callingNumber.e164.buf = max->clg;
	myCC->choice.myRequest.payload.choice.maxsec.calledNumber.e164.size = strlen(max->cld);
	myCC->choice.myRequest.payload.choice.maxsec.calledNumber.e164.buf = max->cld;
	myCC->choice.myRequest.payload.choice.maxsec.callId.callId.size = strlen(max->call_uid);
	myCC->choice.myRequest.payload.choice.maxsec.callId.callId.buf = max->call_uid;	
	
	return myCC;
}

MyCC_PDU_t *my_response_maxsec(maxsec_t *max)
{
	MyCC_PDU_t *myCC = NULL;
	
	myCC = calloc(1, sizeof(MyCC_PDU_t));

	if(myCC == NULL) return NULL;

	myCC->present = MyCC_PDU_PR_myResponse;
	
	myCC->choice.myResponse.header.cdrServerId = max->cdr_server_id;
	myCC->choice.myResponse.header.version = MyCCVersion;
	myCC->choice.myResponse.header.transactionId = max->tid;
	
	myCC->choice.myResponse.header.timestamp = max->ts;

	myCC->choice.myResponse.header.hostname.size = strlen(max->host);
	myCC->choice.myResponse.header.hostname.buf = max->host;
		
	myCC->choice.myResponse.payload.present = MyResponsePayload_PR_maxsec;
	myCC->choice.myResponse.payload.choice.maxsec.maxsec = max->maxsec;
	
	return myCC;
}

MyCC_PDU_t *my_request_empty(empty_t *emp)
{
	MyCC_PDU_t *myCC = NULL;
	myCC = calloc(1, sizeof(MyCC_PDU_t));

	if(myCC == NULL) return NULL;

	myCC->present = MyCC_PDU_PR_myRequest;
	
	myCC->choice.myRequest.header.cdrServerId = emp->cdr_server_id;
	myCC->choice.myRequest.header.version = MyCCVersion;
	myCC->choice.myRequest.header.transactionId = emp->tid;
	
	myCC->choice.myRequest.header.timestamp = emp->ts;

	myCC->choice.myRequest.header.hostname.size = strlen(emp->host);
	myCC->choice.myRequest.header.hostname.buf = emp->host;
		
	myCC->choice.myRequest.payload.present = emp->type;
	
	return myCC;
}

MyCC_PDU_t *my_response_empty(empty_t *emp)
{
	MyCC_PDU_t *myCC = NULL;
	
	myCC = calloc(1, sizeof(MyCC_PDU_t));

	if(myCC == NULL) return NULL;

	myCC->present = MyCC_PDU_PR_myResponse;
	
	myCC->choice.myResponse.header.cdrServerId = emp->cdr_server_id;
	myCC->choice.myResponse.header.version = MyCCVersion;
	myCC->choice.myResponse.header.transactionId = emp->tid;
	
	myCC->choice.myResponse.header.timestamp = emp->ts;

	myCC->choice.myResponse.header.hostname.size = strlen(emp->host);
	myCC->choice.myResponse.header.hostname.buf = emp->host;
		
	myCC->choice.myResponse.payload.present = emp->type;
	
	return myCC;
}

MyCC_PDU_t *my_request_term(term_t *tr)
{
	MyCC_PDU_t *myCC = NULL;
	myCC = calloc(1, sizeof(MyCC_PDU_t));

	if(myCC == NULL) return NULL;

	myCC->present = MyCC_PDU_PR_myRequest;
	
	myCC->choice.myRequest.header.cdrServerId = tr->cdr_server_id;
	myCC->choice.myRequest.header.version = MyCCVersion;
	myCC->choice.myRequest.header.transactionId = tr->tid;
	
	myCC->choice.myRequest.header.timestamp = tr->ts;

	myCC->choice.myRequest.header.hostname.size = strlen(tr->host);
	myCC->choice.myRequest.header.hostname.buf = tr->host;
		
	myCC->choice.myRequest.payload.present = MyRequestPayload_PR_term;	
	myCC->choice.myRequest.payload.choice.term.callingNumber.e164.size = strlen(tr->clg);
	myCC->choice.myRequest.payload.choice.term.callingNumber.e164.buf = tr->clg;
	myCC->choice.myRequest.payload.choice.term.calledNumber.e164.size = strlen(tr->cld);
	myCC->choice.myRequest.payload.choice.term.calledNumber.e164.buf = tr->cld;
	myCC->choice.myRequest.payload.choice.term.callId.callId.size = strlen(tr->call_uid);
	myCC->choice.myRequest.payload.choice.term.callId.callId.buf = tr->call_uid;
	myCC->choice.myRequest.payload.choice.term.billsec.billsec = tr->billsec;
	
	return myCC;
}

MyCC_PDU_t *my_request_rate(rate_t *rt)
{
	MyCC_PDU_t *myCC = NULL;
	
	myCC = calloc(1, sizeof(MyCC_PDU_t));

	if(myCC == NULL) return NULL;

	myCC->present = MyCC_PDU_PR_myRequest;
	
	myCC->choice.myRequest.header.cdrServerId = rt->cdr_server_id;
	myCC->choice.myRequest.header.version = MyCCVersion;
	myCC->choice.myRequest.header.transactionId = rt->tid;
	
	myCC->choice.myRequest.header.timestamp = rt->ts;

	myCC->choice.myRequest.header.hostname.size = strlen(rt->host);
	myCC->choice.myRequest.header.hostname.buf = rt->host;
		
	myCC->choice.myRequest.payload.present = MyRequestPayload_PR_rate;	
	myCC->choice.myRequest.payload.choice.rate.callingNumber.e164.size = strlen(rt->clg);
	myCC->choice.myRequest.payload.choice.rate.callingNumber.e164.buf = rt->clg;
	myCC->choice.myRequest.payload.choice.rate.calledNumber.e164.size = strlen(rt->cld);
	myCC->choice.myRequest.payload.choice.rate.calledNumber.e164.buf = rt->cld;	
	
	return myCC;
}

MyCC_PDU_t *my_response_rate(rate_t *rt)
{
	MyCC_PDU_t *myCC = NULL;
	
	myCC = calloc(1, sizeof(MyCC_PDU_t));

	if(myCC == NULL) return NULL;

	myCC->present = MyCC_PDU_PR_myResponse;
	
	myCC->choice.myResponse.header.cdrServerId = rt->cdr_server_id;
	myCC->choice.myResponse.header.version = MyCCVersion;
	myCC->choice.myResponse.header.transactionId = rt->tid;
	
	myCC->choice.myResponse.header.timestamp = rt->ts;

	myCC->choice.myResponse.header.hostname.size = strlen(rt->host);
	myCC->choice.myResponse.header.hostname.buf = rt->host;
		
	myCC->choice.myResponse.payload.present = MyResponsePayload_PR_rate;
	myCC->choice.myResponse.payload.choice.rate.cprice.size = strlen(rt->cprice);
	myCC->choice.myResponse.payload.choice.rate.cprice.buf = rt->cprice;
	return myCC;
}

void free_MyCC_PDU(MyCC_PDU_t *myCC)
{
	if(myCC != NULL) free(myCC); //asn_DEF_MyCC_PDU.free_struct(&asn_DEF_MyCC_PDU, myCC, 0);
}

int recognize_buffer_size(char *buf,size_t max)
{
	int i;
	size_t num = 0;
	
	for(i=0;i<max;i++) {
		if(buf[i] == 0) break;
		
		num++;
	}

	return num;
}

int encode_MyCC_PDU(MyCC_PDU_t *myCC,char *buffer,size_t buffer_size,FILE *fp)
{
	asn_enc_rval_t ec;
	
	ec = der_encode_to_buffer(&asn_DEF_MyCC_PDU, myCC,buffer, buffer_size);
		
	if(ec.encoded == -1) {
		fprintf(stderr, "Could not encode MyCC-PDU (at %s)\n",ec.failed_type ? ec.failed_type->name : "unknown");
		return 1;
	} else {
		fprintf(stderr, "Created BER encoded MyCC-PDU\n");
	}
	
	//asn_fprint(stdout, &asn_DEF_MyCC_PDU, myCC);
	if(fp != NULL) xer_fprint(fp, &asn_DEF_MyCC_PDU, myCC_Dec);
	
	return 0;
}

int decode_MyCC_PDU(MyCC_PDU_t *myCC_Dec,char *buf,size_t buf_size,FILE *fp)
{
	asn_dec_rval_t rval;

	rval = ber_decode(0, &asn_DEF_MyCC_PDU, (void **)&myCC_Dec,buf,buf_size);
	if(rval.code != RC_OK) {
		fprintf(stderr, "Brocken \n");
		return 1;
	}
	
	if(fp != NULL) xer_fprint(fp, &asn_DEF_MyCC_PDU, myCC_Dec);
	
	return 0;
}

