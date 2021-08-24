# libCK

*libCK* is the client library used to access *CoreKubeDB* from the client application. *libCK* implements the protocol used by *CoreKubeDB*. 

## How to compile libCK

To compile and install *libCK*, use the followings commands:
```bash
cd libck/
make
sudo make install
```

A successful compilation generates a shared library named *libck.so*.

## How to use libCK

To include libCK into the client source code use:
```c
#include <libck.h>
```

These are the functions provided by the *libCK* API:
```c
enum ITEM_TYPE
{
	IMSI=1,
	MSIN,
	TMSI,
	ENB_UE_S1AP_ID,
	UE_TEID,
	SPGW_IP,
	ENB_IP,
	PDN_IP,
	UE_NAS_SEQUENCE_NUMBER,
	EPC_NAS_SEQUENCE_NUMBER,
	UE_NAS_SEQUENCE_NUMBER_NO_INC,
	KEY,
	OPC,
	RAND,
	RAND_UPDATE,
	MME_UE_S1AP_ID,
	EPC_TEID,
	AUTH_RES,
	ENC_KEY,
	INT_KEY,
	KASME_1,
	KASME_2
};
typedef uint8_t ITEM_TYPE;

int db_connect(char * ip_addr, int port);
void db_disconnect(int sock);
int push_items(uint8_t * buffer, ITEM_TYPE id, uint8_t * id_value, int num_items, ...);
int pull_items(uint8_t * buffer, int push_len, int num_items, ...);
void send_request(int sock, uint8_t * buffer, int buffer_len);
int recv_response(int sock, uint8_t * buffer, int buffer_len);
```

### libCK examples

For all the examples, the CoreKubeDB IP address is *192.168.1.25*.

**Connect (default port)**
```c
int db_sock;
db_sock = db_connect("192.168.1.25", 0);
```

**Connect (other port)**
```c
int db_sock;
int db_port = 1234;
db_sock = db_connect("192.168.1.25", db_port);
```

**Disconnect**
```c
db_disconnect(db_sock);
```

**Update UE TEID, ENB IP, and UE NAS Sequence Number fields in the DB and request UE KEY, OPC KEY, and RAND.**
```c
uint8_t buf[1024];
uint8_t recv_buf[1024];
uint8_t teid[4];
uint8_t enb_ip[4];
uint8_t nas_seq_num;
char imsi[] = "208930000000001";
int n;

n = push_items(buf, IMSI, (uint8_t *)imsi, 3, 
	UE_TEID, teid, 
	ENB_IP, enb_ip, 
	UE_NAS_SEQUENCE_NUMBER, &nas_seq_num);
n = pull_items(buf, n, 3, KEY, OPC, RAND);

send_request(sock, buf, n);

n = recv_response(sock, buf, n);
```

