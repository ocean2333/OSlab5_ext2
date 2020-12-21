#include<stdio.h>
#include<string.h>
#include"ext2.c"
int main(){
    char s[10][10] = {"a","./c/b","/a/b","a/b"};
    char p[50];
    for(int i=0;i<4;i++){
        find_name(s[i],p);
        printf("%s\n",p);
    }  
    return 0;
}