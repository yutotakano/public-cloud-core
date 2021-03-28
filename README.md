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