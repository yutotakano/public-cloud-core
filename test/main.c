#include <stdio.h>
#include <libck.h>

void dump_mem(uint8_t * value, int len)
{
	int i;

	for(i = 0; i < len; i++) {
		if(i % 17 == 0)
			printf("\n");
		printf("%.2x ", value[i]);
	}
	printf("\n");
}

int main(int argc, char const *argv[])
{
	uint8_t buf[1024];
	uint8_t teid[4];
	uint8_t enb_ip[4];
	uint8_t nas_seq_num = 3;
	uint8_t mme_ue_id[4];
	char imsi[] = "208930000000001";
	int n;
	int sock;

	printf("Running test...\n");
	sock = db_connect("127.0.0.1", 0);

	/* Request data using the IMSI */

	teid[0] = 0;
	teid[1] = 0;
	teid[2] = 0;
	teid[3] = 1;
	enb_ip[0] = 192;
	enb_ip[1] = 168;
	enb_ip[2] = 1;
	enb_ip[3] = 1;

	n = push_items(buf, IMSI, (uint8_t *)imsi, 0);
	n = pull_items(buf, n, 2, KEY, OPC);
	printf("REQUEST:");
	dump_mem(buf, n);
	send_request(sock, buf, n);
	n = recv_response(sock, buf, 64);
	printf("RESPONSE:");
	dump_mem(buf, n);
	printf("\n\n");

	/* Request data using the MME_UE_S1AP_ID */

	mme_ue_id[0] = 0;
	mme_ue_id[1] = 0;
	mme_ue_id[2] = 0;
	mme_ue_id[3] = 1;

	n = push_items(buf, MME_UE_S1AP_ID, (uint8_t *)mme_ue_id, 0);
	n = pull_items(buf, n, 2, KEY, OPC);
	printf("REQUEST:");
	dump_mem(buf, n);
	send_request(sock, buf, n);
	n = recv_response(sock, buf, 64);
	printf("RESPONSE:");
	dump_mem(buf, n);
	printf("\n\n");

	/* Request data using the TMSI */

	n = push_items(buf, TMSI, (uint8_t *)mme_ue_id, 0);
	n = pull_items(buf, n, 2, KEY, OPC);
	printf("REQUEST:");
	dump_mem(buf, n);
	send_request(sock, buf, n);
	n = recv_response(sock, buf, 64);
	printf("RESPONSE:");
	dump_mem(buf, n);
	printf("\n\n");

	db_disconnect(sock);


	return 0;
}