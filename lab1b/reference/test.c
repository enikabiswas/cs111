#include <stdio.h>
#include <unistd.h>
#include "zlib.h"
#include <stdlib.h>
#include <string.h>

#define BUFSIZE 1024

int comp_size(char* compd) {
	int size = -1;
	char checksize[BUFSIZE+10];
	size = sprintf(checksize, "%s", compd);
	return size;
}


char* def(char* to_deflate) {
	char copy[BUFSIZE] = "";
	strcpy(copy, to_deflate);
	char *deflated = malloc(sizeof(char) * BUFSIZE);

	z_stream client_to_server;
	client_to_server.zalloc = Z_NULL;
	client_to_server.zfree = Z_NULL;
	client_to_server.opaque = Z_NULL;


	//printf("Bytes before compression: %d\n", strlen(to_deflate));
	client_to_server.avail_in = (uInt) strlen(copy) + 1; //to include null byte
	client_to_server.next_in = (Bytef *) copy;
	client_to_server.avail_out = (uInt) BUFSIZE;
	client_to_server.next_out = (Bytef *) deflated;
	deflateInit(&client_to_server, Z_DEFAULT_COMPRESSION);	
	do {
		deflate(&client_to_server, Z_SYNC_FLUSH);
	} while(client_to_server.avail_in > 0);
	
	//write(sockfd, compression_buf, compression_buf_size - client_to_server.avail_out);

	//deflate(&client_to_server, Z_FINISH);
	deflateEnd(&client_to_server);

	return deflated;
}

char* inf(char* to_inflate) {
	char copy[BUFSIZE] = "";
	strcpy(copy, to_inflate);
	z_stream server_to_client;
	char *inflated = malloc(sizeof(char) * BUFSIZE);
	//	char checksize[BUFSIZE+10];
	int numbytes = comp_size(copy);//sprintf(checksize, "%s", to_inflate);
	//	printf("Bytes after compression: %d\n", numbytes);
	server_to_client.zalloc = Z_NULL;
	server_to_client.zfree = Z_NULL;
	server_to_client.opaque = Z_NULL;

	server_to_client.avail_in = (uInt) numbytes + 1;
	server_to_client.next_in = (Bytef *) copy;
	server_to_client.avail_out = (uInt) BUFSIZE;
	server_to_client.next_out = (Bytef *) inflated;
	inflateInit(&server_to_client);

	do {
		inflate(&server_to_client, Z_SYNC_FLUSH);
	} while(server_to_client.avail_in > 0);
	

	//inflate(&server_to_client, Z_NO_FLUSH);
	inflateEnd(&server_to_client);
	//printf("inflated??? %s\n", inflated);
	return inflated;
}


int main(void) {
	char a[50] = "w";
	char d[50] = "e";
	char e[50] = "s";
	char f[50] = "b";
	char g[50] = "l";
	char h[50] = "u";
	char i[50] = "d";
	char j[50] = "s";
	char* b;

	printf("a\n");
	b = def(a);
	printf("%s\n", b);
	char* c = inf(b);
	printf("%s\n", c);
	free(b);
	free(c);
	printf("\n----------------\n");
	printf("d\n");
	b = def(d);
	printf("%s\n", b);
	c = inf(b);
	printf("%s\n", c);
	free(b);
	free(c);
	printf("\n----------------\n");
	printf("e\n");
	b = def(e);
	printf("%s\n", b);
	c = inf(b);
	printf("%s\n", c);
	free(b);
	free(c);

	printf("\n----------------\n");
	printf("f\n");
	b = def(f);
	printf("%s\n", b);
	c = inf(b);
	printf("%s\n", c);
	free(b);
	free(c);

	printf("\n----------------\n");
	printf("g\n");
	b = def(g);
	printf("%s\n", b);
	c = inf(b);
	printf("%s\n", c);
	free(b);
	free(c);

	printf("\n----------------\n");
	printf("h\n");
	b = def(h);
	printf("%s\n", b);
	c = inf(b);
	printf("%s\n", c);
	free(b);
	free(c);

	printf("\n----------------\n");
	printf("i\n");
	b = def(i);
	printf("%s\n", b);
	c = inf(b);
	printf("%s\n", c);
	free(b);
	free(c);

	printf("\n----------------\n");
	printf("j\n");
	b = def(j);
	printf("%s\n", b);
	c = inf(b);
	printf("%s\n", c);
	free(b);
	free(c);
	/*

	char a[50] = "we's is bluds mane";
	char *b;
	char *c;

	b = def(a);                                                   
        printf("%s\n", b);                                                                     
        c = inf(b);                                                                                                                        
        printf("%s\n", c); 

	free(b);
	free(c);

	printf("\n----------------\n");

	char d[50] = "awww shit bitch";
	strcpy(a, d);
	printf("%s\n", a);

	b = def(a);
        printf("%s\n", b);
        c = inf(b);
        printf("%s\n", c);

	free(b);
	free(c);

        printf("\n----------------\n");

        char e[50] = "fuck no baby";
        strcpy(a, e);
        printf("%s\n", a);

        b = def(a);
        printf("%s\n", b);
        c = inf(b);
        printf("%s\n", c);

	free(b);
	free(c);

	char wtf[10] = "hello b";
	char hey[20];
	int size = sprintf(hey, "%s", wtf);
	write(0, wtf, size);
	*/	
	char yuh[20] = "h";
	char* oman = def(yuh);

	char* siri = inf(oman);
	printf("inf: %s\n", siri);
	char shitfuck[20] = "hello hello hello";
	char yadig[5] = "bitch";
	strcpy(shitfuck, yadig);
	printf("%s\n", shitfuck);

	return 0;

}
