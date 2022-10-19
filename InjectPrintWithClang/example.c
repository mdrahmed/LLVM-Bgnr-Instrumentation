#include <stdio.h>
int main(int argc, const char** argv) {
    int no;
    scanf("%i", &no);
    if(no> 10){
        printf("%i\n", no + 2);
    }
    else{
            printf("Num is less than 10");
    }
    return 0;
}

