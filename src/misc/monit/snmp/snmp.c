/*
 * gcc -g -Wall -o test_snmp snmp.c -I. -I/usr/include -lnetsnmp
 * 
 * */

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>

int main(void) {
	struct snmp_session session, *ss;
	struct snmp_pdu *pdu;
	struct snmp_pdu *response;
	
	oid anOID[MAX_OID_LEN];
	size_t anOID_len = MAX_OID_LEN;
	
	struct variable_list *vars;
	int status;
	
	init_snmp("snmpapp");
	
	snmp_sess_init( &session );
	session.peername = "hpvp.bulsat.com";
	
	session.version = SNMP_VERSION_1;
	
	session.community = "public";
	//session.community_len = strlen(session.community);
	session.community_len = strlen(session.community);
	
	ss = snmp_open(&session);
	
	if (!ss) {
		snmp_perror("ack");
		snmp_log(LOG_ERR, "something horrible happened!!!\n");
		exit(2);
	}
	
	pdu = snmp_pdu_create(SNMP_MSG_GET);
	
	read_objid(".1.3.6.1.2.1.1.1.0", anOID, &anOID_len);
	
	snmp_add_null_var(pdu, anOID, anOID_len);
	
	status = snmp_synch_response(ss, pdu, &response);
	if (status == STAT_SUCCESS && response->errstat == SNMP_ERR_NOERROR) {
		for(vars = response->variables; vars; vars = vars->next_variable)
			print_variable(vars->name, vars->name_length, vars);
		
		for(vars = response->variables; vars; vars = vars->next_variable) {
			int count=1;
			if (vars->type == ASN_OCTET_STR) {
				char *sp = malloc(1 + vars->val_len);
				memcpy(sp, vars->val.string, vars->val_len);
				sp[vars->val_len] = '\0';
				printf("value #%d is a string: %s\n", count++, sp);
				free(sp);
			} else printf("value #%d is NOT a string! Ack!\n", count++);
		}
	} else {
		if (status == STAT_SUCCESS) fprintf(stderr, "Error in packet\nReason: %s\n",snmp_errstring(response->errstat));
		else snmp_sess_perror("snmpget", ss);
	}
	
	if(response) snmp_free_pdu(response);
	snmp_close(ss);   
	
	return 0;
}
