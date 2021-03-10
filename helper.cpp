#include "helpers/strtools.cpp"

int hex_dump(unsigned char *data, int len) {
register int i;
register int j;
	for(i = 0; i < len; i += 16) {
		printf("\n");
		for(j = 0; j < 16; j++) {
			if(i+j < len) printf("%02X ", data[i+j]);
			else printf(" ");
		}
		printf(" | ");

		for(j = 0; j < 16; j++) {
			unsigned char character = (i+j < len) ? data[i+j] : ' ';
			if((character <= 0x20) || (character >= 0x7e)) character = '.';
			printf("%c", character);
		}
	}
	printf("\n");
	return 0;
}