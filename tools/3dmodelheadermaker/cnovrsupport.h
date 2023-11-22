#ifndef _CNOVR_SUPPORT_H
#define _CNOVR_SUPPORT_H

char * CNOVRFileToString( const char * fname, int * length );
char ** CNOVRSplitStrings( const char * line, char * split, char * white, int merge_fields, int * elementcount );

#endif


