#include<stdio.h>
int main() {
	int a,b,c;
	FILE *f = fopen("neos3.mtx", "r");
	fscanf(f, "%d%d%d",&a,&b,&c);
	fclose(f);
	printf("%d %d %d\n", a,b,c);
}
