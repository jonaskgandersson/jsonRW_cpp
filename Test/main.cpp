#include "../jsonRW.hpp"
#include <stdio.h>

#define BUFFER_LEN 1000

int main( int argc, char* argv[])
{
    char buffer[BUFFER_LEN];
    int err = JWRITE_OK;

    jWrite jw( buffer, BUFFER_LEN );        // Create jWrite instance to use application buffer 
    jw.open( JW_OBJECT, JW_PRETTY );    // open root node as object 
    jw.obj_string( "key", "value" );    // writes "key":"value" 
    jw.obj_int( "int", 1 );             // writes "int":1 
    jw.obj_array( "anArray");           // start "anArray": [...] 
    jw.arr_int( 0 );                 // add a few integers to the array 
    jw.arr_int( 1 ); 
    jw.arr_int( 2 ); 
    jw.end();                           // end the array 
    err= jw.close();                    // close root object - done

    printf("JSON: \n\r%s \n\r", buffer);
    return 0;
}
