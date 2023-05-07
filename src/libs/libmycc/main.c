#include <stdio.h>
#include <time.h>
#include <sys/types.h>

#include <MyCC.h>

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <fcntl.h>


void main(void)
{
	char buffer[512];
	char *write_buf;
	size_t write_buf_size;
	
	MyCC_PDU_t *myCC = NULL;
	MyCC_PDU_t *myCC_Dec = NULL;
	
/*	balance_t bal;
	
	bal.ts = time(NULL);
	bal.tid = 1;
	bal.cdr_server_id = 3;
	strcpy(bal.host,"SMS1");
	strcpy(bal.clg,"359996666199");
	strcpy(bal.amount,"6.6456");
	
	//myCC = my_request_balance(&bal);
	myCC = my_request_balance(&bal);
*/

	maxsec_t max;
	
	max.ts = time(NULL);
	max.tid = 1;
	max.cdr_server_id = 3;
	strcpy(max.host,"SMS1");
	strcpy(max.clg,"359996666199");
	strcpy(max.cld,"35924119998");
	strcpy(max.call_uid,"34f81b3c-7b3f-11e7-8537-69df9bdae73a");
	
	max.maxsec = 3662;

/*	empty_t stat;
	
	stat.ts = time(NULL);
	stat.tid = 1;
	stat.cdr_server_id = 3;
	strcpy(stat.host,"SMS1");
	stat.type = MyResponsePayload_PR_status;
*/

/*	term_t tr;
	
	tr.ts = time(NULL);
	tr.tid = 1;
	tr.cdr_server_id = 3;
	strcpy(tr.host,"SMS1");
	strcpy(tr.clg,"359996666199");
	strcpy(tr.cld,"35924119998");
	strcpy(tr.call_uid,"34f81b3c-7b3f-11e7-8537-69df9bdae73a");
	tr.billsec = 366;
*/	
	myCC = my_response_maxsec(&max);
	
	if(myCC == NULL) exit(1);
	
	memset(buffer,0,sizeof(buffer));

	/* Encoding */	
	encode_MyCC_PDU(myCC,buffer,sizeof(buffer));

	write_buf_size = recognize_buffer_size(buffer,sizeof(buffer));
		
	//asn_DEF_MyCC_PDU.free_struct(&asn_DEF_MyCC_PDU, myCC, 0);
	//asn_DEF_MyCC_PDU.free_struct(&asn_DEF_MyCC_PDU, myCC_Dec, 0);

	write_buf = calloc(1,write_buf_size);
	memcpy(write_buf,buffer,write_buf_size);
	
	free_MyCC_PDU(myCC);
	
	/* Decoding */
	myCC_Dec = calloc(1,sizeof(MyCC_PDU_t));
	decode_MyCC_PDU(myCC_Dec,write_buf,write_buf_size);
	
	free(write_buf);

    if(myCC_Dec != NULL) {
		fprintf(stderr, "\nDEBUG: %ld , %ld \n",myCC_Dec->choice.myResponse.header.timestamp,myCC_Dec->choice.myResponse.payload.choice.maxsec.maxsec);
    } else {
		fprintf(stderr, "\nMyCC_Dec is NULL: \n");
    }

	free_MyCC_PDU(myCC_Dec);
}
