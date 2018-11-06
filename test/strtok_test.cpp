#include <iostream> 
#include <string.h>

using namespace std; 


int main() 
{ 
	char s[] = "6->1->2->3->4->5"; 
	char *p; 
	const char *delim = "->"; 
	p = strtok(s, delim); 

	while(p) { 
		cout << p << endl; 
		p = strtok(NULL, delim); 
	} 
	return 0; 
}

