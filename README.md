# CoreKubeDB

*CoreKubeDB* is a high-performance database created for the CoreKube project. *CoreKubeDB* is meant to store all the LTE subscribers information required by a stateless EPC (static data such as IMSI, keys, etc. and temporary data such as TEID, IP address, NAS sequence number, etc.).

*CoreKubeDB* internal structure is a multithreaded server with one thread per client (CoreKube worker). The data is loaded into memory during the initialization to provide fast access to the data avoiding IO-disk operations. Internally, *CoreKubeDB* structures the data into two HashMaps to provide constant access time to every entry (subscriber entry). The reason for having two HashMaps is because the EPC workers may request information about one subscriber with the IMSI or the TMSI as the subscriber ID. Therefore, the first HashMap derivates the key from the IMSI, whereas the second uses the TMSI generating a 32-bit integer key.

*CoreKubeDB* does not need to deal with data inconsistencies because situations in which data from the same subscriber is requested from two different EPC workers cannot happen in the LTE EPC context. Each HashMap has been implemented as a fixed table with a singly linked list to avoid hash collisions (very similar to the Kernel HashMap implementation).

Nagle's algorithm has been disabled from the database socket to avoid message queueing by the kernel.

## How to compile CoreKube


Install dependencies:

```bash
sudo apt-get install -y libconfig-dev
```

Compile *CoreKubeDB*:

```bash
make
```

## Configuration

*CoreKubeDB* requires two files: the configuration file and the database data file.

The configuration file has to be named as **db.conf** and must be in the same folder than the CoreKube executable.
The following is an example of the configuration file:
```
# DATABASE CONFIGURATION
db = {
	hashmap_size = 1024;	/* Size of the hashmap table (Default: 1024) */
	users_db = "users_db"; /* Path to the db file */
}

# NETWORK CONFIGURATION
network = {
	db_ip_address = "127.0.0.1";
	db_port = 7788; /* Default: 7788 */
}
```

The users_db field refers to the database data file path. It has the following aspect:
```
# OP_Type:  Operator's code type, either OP or OPc  
208930000000001,3f3f473f2f3fd0943f3f3f3f097c6862,opc,e9be7fb89bb01978e67972ca8580079e
208930000000002,3f3f473f2f3fd0943f3f3f3f097c6862,opc,e9be7fb89bb01978e67972ca8580079e
208930000000003,3f3f473f2f3fd0943f3f3f3f097c6862,opc,e9be7fb89bb01978e67972ca8580079e
208930000000004,3f3f473f2f3fd0943f3f3f3f097c6862,opc,e9be7fb89bb01978e67972ca8580079e
208930000000005,3f3f473f2f3fd0943f3f3f3f097c6862,opc,e9be7fb89bb01978e67972ca8580079e
```

## Running CoreKubeDB

To run *CoreKubeDB*, use the following command:
```bash
./corekubeDB
```

## libCK

*libCK* is the client library used to access *CoreKubeDB* from the client application. *libCK* implements the protocol used by *CoreKubeDB*. 

### How to compile libCK

To compile and install *libCK*, use the followings commands:
```bash
cd libck/
make
sudo make install
```

A successful compilation generates a shared library named *libck.so*.

### How to use libCK

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
	KEY,
	OPC,
	RAND,
	RAND_UPDATE,
	MME_UE_S1AP_ID,
	EPC_TEID,
	AUTH_RES,
	ENC_KEY,
	INT_KEY
};
typedef uint8_t ITEM_TYPE;

int db_connect(char * ip_addr, int port);
void db_disconnect(int sock);
int push_items(uint8_t * buffer, ITEM_TYPE id, uint8_t * id_value, int num_items, ...);
int pull_items(uint8_t * buffer, int push_len, int num_items, ...);
void send_request(int sock, uint8_t * buffer, int buffer_len);
int recv_response(int sock, uint8_t * buffer, int buffer_len);
```

#### libCK examples

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

** Update UE TEID, ENB IP, and UE NAS Sequence Number fields in the DB and request UE KEY, OPC KEY, and RAND. **
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

