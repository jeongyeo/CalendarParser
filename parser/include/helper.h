#ifndef HELPER_H
#define HELPER_H

// frees char** array
void freeArray(char **file, int length);
// creates a property object
Property *createProperty(char* string);
char *strdup(const char *s);
bool searchList(char *list[], char *string, int length);
bool comparePropertyName(const void* first, const void* second);
#endif 

