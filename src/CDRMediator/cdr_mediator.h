#ifndef CDR_MEDIATOR_H
#define CDR_MEDIATOR_H

#if PROC_THREAD_FLAG
void *CDRMediatorEngine(void *dt);
#else 
void CDRMediatorEngine(void);
#endif

#endif
