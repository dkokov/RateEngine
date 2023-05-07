/*
 * Generated by asn1c-0.9.28 (http://lionet.info/asn1c)
 * From ASN.1 module "MyCC"
 * 	found in "../MyCC.asn1"
 */

#ifndef	_MyCC_PDU_H_
#define	_MyCC_PDU_H_


#include <asn_application.h>

/* Including external dependencies */
#include "MyRequest.h"
#include "MyResponse.h"
#include <constr_CHOICE.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Dependencies */
typedef enum MyCC_PDU_PR {
	MyCC_PDU_PR_NOTHING,	/* No components present */
	MyCC_PDU_PR_myRequest,
	MyCC_PDU_PR_myResponse
} MyCC_PDU_PR;

/* MyCC-PDU */
typedef struct MyCC_PDU {
	MyCC_PDU_PR present;
	union MyCC_PDU_u {
		MyRequest_t	 myRequest;
		MyResponse_t	 myResponse;
	} choice;
	
	/* Context for parsing across buffer boundaries */
	asn_struct_ctx_t _asn_ctx;
} MyCC_PDU_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_MyCC_PDU;

#ifdef __cplusplus
}
#endif

#endif	/* _MyCC_PDU_H_ */
#include <asn_internal.h>
