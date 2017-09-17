#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <modbus/modbus.h>

int main(int argc, char * argv[])
{
	int i, n, rc;
	modbus_t *mb;
	uint16_t tab_reg[256];
	char * ipad;
	int portn;
	int regad;
	int regcn;

	if (argc < 5)
	{
		printf("Usage: %s <IP> <PORT> <REGISTER> <COUNT>\n", argv[0]);
		return -1;
	}
	ipad = argv[1];
	portn = atoi(argv[2]);
	regad = (int) strtol(argv[3], NULL, 16);
	regcn = atoi(argv[4]);

	printf("Connect to address %s port %d\n", ipad, portn);

	/* allocate and initialize structure to communicate with a Modbus TCP/IPv4 server */
	mb = modbus_new_tcp(ipad, portn);
	if (NULL == mb)
	{
		fprintf(stderr, "modbus_new_tcp failed\n");
		return -1;
	}
	/* mb->debug = 1; */
	
	/* Establish connection to Modbus server, network or bus */
	rc = modbus_connect(mb);
	if (rc) 
	{
		fprintf(stderr, "Connection failed: %s\n", modbus_strerror(errno));
		modbus_free(mb);
		return -1;
	}
	
	n = 10;
	while (n--)
	{
		/* Read 4 registers from the address 0 */
		printf("Read %d registers at 0x%x\n", regcn, regad);
		rc = modbus_read_registers(mb, regad, regcn, tab_reg);
		if (rc == -1) 
		{
			fprintf(stderr, "Read registers failed: %s\n", modbus_strerror(errno));
			modbus_free(mb);
			return -1;
		}
		
		printf("Read %d registers\n", rc);
		for (i=0; i<rc; i++)
		{
			printf("\tRegister %d : %d\n", i, tab_reg[i]);
		}
		sleep(3);
	}

	/* Close the connection established with the backend set in the context */
	modbus_close(mb);
	
	/* Free allocated structure */
	modbus_free(mb);

	return 0;
}

