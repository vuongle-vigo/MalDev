#include "ApiResolve.h"
#include "Base64.h"
#include "CRT.h"

int main() {
    
    const char* sHello = "HelloWorld";
    char* b64en = Base64Encode(sHello, 10);

    int length; 
    char* b64de = Base64Decode(b64en, length);



    return 0;
}