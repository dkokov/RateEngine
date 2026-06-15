#ifndef PCARD_H
#define PCARD_H

typedef enum pcard_type {
	debit_card  = 1,
	credit_card = 2
} pcard_type_t;

typedef enum pcard_status {
	pcard_deactive = 0,
	pcard_active   = 1,
	pcard_block    = 2
} pcard_status_t;

void pcard_manager(db_t *dbp,racc_t *rtp);
void pcard_free(pcard_t *card);

#endif
