#include "../jsonRW.hpp"
#include <stdio.h>

#define BUFFER_LEN 1000

int main(int argc, char *argv[])
{
    char buffer[BUFFER_LEN];
    int err = JWRITE_OK;

    jWrite jw(buffer, BUFFER_LEN); // Create jWrite instance to use application buffer
    jw.open(JW_OBJECT, JW_PRETTY); // open root node as object
    jw.add("key", "value"); // writes "key":"value"
    jw.add("int", 1);          // writes "int":1
    jw.add("Double", 3.56);     //writes "Double":3.56
    jw.add("Bool", true);       // writes "Bool":true
    jw.add("NotBool", 1);       // writes "NotBool":1 (This is an integer)
    jw.add("anArray", JsonNodeType::JS_ARRAY);       // start "anArray": [...]
    jw.arr_int(0);                 // add a few integers to the array
    jw.arr_int(1);
    jw.arr_int(2);

    jw.add(JsonNodeType::JS_NULL);      // Writes null
    jw.add(JsonNodeType::JS_ARRAY);     // Writes [
    jw.end();   // Close array ]
    jw.add(JsonNodeType::JS_OBJECT);    // Writes {
    jw.end();   // Close object }

    jw.end(); // end the array

    jw.add("ObjectObject", JsonNodeType::JS_OBJECT);    // Writes "ObjectObject": {
    jw.add("ObjectNull", JsonNodeType::JS_NULL);        // Writes "ObjectNull": null
    jw.add("ObjectArray", JsonNodeType::JS_ARRAY);      // Writes "ObjectArray": [
    jw.end();   // Close array
    jw.end();   // Close object

    err = jw.close(); // close root object - done

    printf("JSON: \n\r%s \n\r", buffer);
    return 0;
}
