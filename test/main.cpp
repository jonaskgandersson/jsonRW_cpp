#include "../jsonRW.hpp"
#include <stdio.h>

#define BUFFER_LEN 1000

using namespace jonaskgandersson;
//-------------------------------------------------
//// Do a query and print the results
void testQuery(Json &oJson, char *pJson, const char *query)
{
    struct ReadElement jElement;
    oJson.getElement(query, jElement);
    printf("Query: \"%s\"\n", query);
    //printf( "return: %d= %s\n", jElement.error, oJson.jReadErrorToString(jElement.error) );
    //printf( " dataType = %s\n", oJson.jReadTypeToString(jElement.dataType) );
    printf(" elements = %d\n", jElement.elements);
    printf(" bytelen  = %d\n", jElement.bytelen);
    printf(" value    = %*.*s\n\n", jElement.bytelen, jElement.bytelen, (const char *)jElement.pValue);
}

int main(int argc, char *argv[])
{
    // Wrtite test
    char buffer[BUFFER_LEN];
    int err = JWRITE_OK;

    Json jw(buffer, BUFFER_LEN);             // Create jWrite instance to use application buffer
    jw.open(NodeType::JS_OBJECT, JW_PRETTY); // open root node as object
    jw.add("key", "value");                  // writes "key":"value"
    jw.add("int", 1);                        // writes "int":1
    jw.add("Double", 3.56);                  //writes "Double":3.56
    jw.add("Bool", true);                    // writes "Bool":true
    jw.add("NotBool", 1);                    // writes "NotBool":1 (This is an integer)
    jw.add("anArray", NodeType::JS_ARRAY);   // start "anArray": [...]
    jw.add(0);                               // add a few integers to the array
    jw.add(1);
    jw.add(2);

    jw.add(NodeType::JS_NULL);   // Writes null
    jw.add(NodeType::JS_ARRAY);  // Writes [
    jw.add("StringInArray");     // Writes "StringInArray"
    jw.add(3.14159265);          // Writes 3.14159265
    jw.add(false);               // Writes false
    jw.add((bool)1);             // Writes true
    jw.end();                    // Close array ]
    jw.add(NodeType::JS_OBJECT); // Writes {
    jw.end();                    // Close object }

    jw.end(); // end the array

    jw.add("ObjectObject", NodeType::JS_OBJECT); // Writes "ObjectObject": {
    jw.add("ObjectNull", NodeType::JS_NULL);     // Writes "ObjectNull": null
    jw.add("ObjectArray", NodeType::JS_ARRAY);   // Writes "ObjectArray": [
    jw.end();                                    // Close array
    jw.end();                                    // Close object

    err = jw.close(); // close root object - done

    if (!err)
    {
        printf("JSON: \n\r%s \n\r", buffer);
    }
    else
    {
        printf("JSON Error: %s @pos: %d\n\r", jw.errorToString(err), jw.errorPos());
    }

    // Read test
    double json_pi;
    testQuery(jw, buffer, "{'key'");

    // Get an element from an raw json string
    ReadElement rElement;
    Json::getElement(buffer, "{'Bool'", &rElement);
    printf(" Bool    = %*.*s\n\n", rElement.bytelen, rElement.bytelen, (const char *)rElement.pValue);

    // Get an element from json object
    jw.getElement("{'anArray'[4[1", rElement);
    printf(" value    = %*.*s\n\n", rElement.bytelen, rElement.bytelen, (const char *)rElement.pValue);

    // Using quaryParams as index
    int index[] = {4};
    jw.getElement("{'anArray'[*[0", index, rElement);
    printf(" value    = %*.*s\n\n", rElement.bytelen, rElement.bytelen, (const char *)rElement.pValue);

    if (jw.getValue("{'anArray'[4[1", json_pi) == ReadError::JS_OK)
    {
        printf("Json Pi: %f\r\n", json_pi);
    }
    else
    {
        printf("Json Pi: Unknown\r\n");
    }

    // Get integer value from raw json
    int json_int;
    if (Json::getValue(buffer, "{'int'", NULL, &json_int) == ReadError::JS_OK)
    {
        printf("Json int: %d\r\n", json_int);
    }
    else
    {
        printf("Json int: Unknown\r\n");
    }

    // Get integer value from json object
    if (jw.getValue("{'int'", json_int) == ReadError::JS_OK)
    {
        printf("Json int: %d\r\n", json_int);
    }
    else
    {
        printf("Json int: Unknown\r\n");
    }

    // Get long integer value from raw json
    long json_long;
    if (Json::getValue(buffer, "{'int'", NULL, &json_long) == ReadError::JS_OK)
    {
        printf("Json long: %ld\r\n", json_long);
    }
    else
    {
        printf("Json long: Unknown\r\n");
    }

    // Get long integer value from json object
    if (jw.getValue("{'int'", json_long) == ReadError::JS_OK)
    {
        printf("Json long: %ld\r\n", json_long);
    }
    else
    {
        printf("Json long: Unknown\r\n");
    }

    // Get string from raw json
    char json_string[100];
    if( Json::jRead_string( buffer, "{'anArray'[4[0", NULL, json_string, 100) == ReadError::JS_OK)
    {
        printf("Json string: %s\r\n", json_string);
    }
    else
    {
        printf("Json string: Unknown\r\n");
    }

    return 0;
}
