#include <stdio.h>
#include <direct.h>
#include <string.h>
#include <stdlib.h>

int main(){
   char* buffer;

   FILE *fd = fopen("test.log", "a+");

   // Get the current working directory:
   if ( (buffer = _getcwd( NULL, 0 )) == NULL )
      perror( "_getcwd error" );
   else
   {
      
      fwrite(buffer, strlen(buffer), strlen(buffer), fd);
      free(buffer);
      fclose(fd);
      //printf( "%s \nLength: %zu\n", buffer, strlen(buffer) );
   }

    return 0;
}