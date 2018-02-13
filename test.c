
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void main()
{
	printf("start strsep\n");

	char str[1024];
	sprintf(str, "som/chat/lakjdfs/121231");

	char * token;
	int i = 0;

	  // char *t = malloc(30);
	  char *t = &buf[0];
	  char *s = t;
	  // strcpy(s, str);

	printf("parse string : %s\n", buf);
	while ((token = strsep(&s, "/"))) {
		printf("get %d : %s, s %p/%s\n", i, token, s, s);
		++i;
	}
	printf("after, parse string : %s\n", buf);
}

