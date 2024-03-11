#ifndef __ngap__
#define __ngap__

#include "enb.h"
#include "ue.h"

int procedure_NG_Setup(eNB * enb);
int procedure_Registration_Request(eNB * enb, UE * ue);
int procedure_Deregistration_Request(eNB * enb, UE * ue, int switch_off);

#endif