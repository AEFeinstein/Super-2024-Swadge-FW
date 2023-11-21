#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <stdint.h>

char *pathcat(const char *str1, char *str2);

#define MAX_FILES 4096

struct fileEntry
{
    char * filename;
    uint8_t * data;
    int offset;
    int len;
} entries[MAX_FILES];
int nr_file = 0;

char * filelist[MAX_FILES];

int stringcmp( const void * a, const void * b )
{
    return strcmp( *(char**)a, *(char**)b );
}

int main( int argc, char ** argv )
{
    if( argc != 4 )
    {
        fprintf( stderr, "Error: Usage: cnfs_gen folder/ image.c image.h\n" );
        return -5;
    }
    char tfolder[PATH_MAX];
    snprintf( tfolder, PATH_MAX, "%s", argv[1] );
    struct dirent *dp;
    DIR *dir = opendir(tfolder);
    if( !dir )
    {
        fprintf( stderr, "Error: Can't open %s\n", argv[1] );
        return -5;
    }
    int offset = 0;
    int numfiles_in = 0;
    while (dp=readdir(dir)) // if dp is null, there's no more content to read
    {
        filelist[numfiles_in++] = strdup( dp->d_name );
    }
    closedir(dir); // close the handle (pointer)

    qsort( filelist, numfiles_in, sizeof( char * ), stringcmp );

    int fno;
    for( fno = 0; fno < numfiles_in; fno++ )
    {
        char * fname_in = filelist[fno];
        char fname[PATH_MAX];
        snprintf( fname, PATH_MAX, "%s/%s", argv[1], fname_in );

        FILE * f = fopen( fname, "rb" );
        if( !f )
        {
            fprintf( stderr, "Error: Can't open file %s\n", fname );
            free( fname );
            continue;
        }
        fseek( f, 0, SEEK_END );
        int len = ftell( f );
        fseek( f, 0, SEEK_SET );

        struct fileEntry * fe = entries + nr_file;
        fe->data = malloc( len );
        fe->len = len;
        int r = fread( fe->data, 1, len, f );
        fe->filename = fname_in;
        if( r != fe->len )
        {
            fprintf( stderr, "Error: File %s truncated (expected %d bytes, got %d)\n", fname, fe->len, r );
        }
        else
        {
            fe->offset = offset;
            printf( "%s %d %d\n", fname_in, len, offset );
            offset += fe->len;
            nr_file++;
        }
        fclose( f );
    }

    int i;

    FILE * f = fopen( argv[3], "w" );

    fprintf( f, "#ifndef CNFS_IMAGE_HEADER_H\n" );
    fprintf( f, "#define CNFS_IMAGE_HEADER_H\n" );
    fprintf( f, "#include <stdint.h>\n" );
    fprintf( f, "#define NR_FILES %d\n", nr_file );
    fprintf( f, "struct cnfsFileEntry {\n" );
    fprintf( f, "    const char * name;\n" );
    fprintf( f, "    uint32_t     len;\n" );
    fprintf( f, "    uint32_t     offset;\n" );
    fprintf( f, "};\n" );
    fprintf( f, "extern const struct cnfsFileEntry cnfs_files[NR_FILES];\n" );
    fprintf( f, "extern const uint8_t cnfs_data[%d];\n", offset );
    fprintf( f, "#endif\n" );
    fclose( f );

    f = fopen( argv[2], "w" );
    fprintf( f, "#include <stdint.h>\n" );
    fprintf( f, "#define NR_FILES %d\n", nr_file );
    fprintf( f, "#ifndef CNFS_IMAGE_HEADER_H\n" );
    fprintf( f, "struct cnfsFileEntry {\n" );
    fprintf( f, "    const char * name;\n" );
    fprintf( f, "    uint32_t     len;\n" );
    fprintf( f, "    uint32_t     offset;\n" );
    fprintf( f, "};\n" );
    fprintf( f, "#endif\n" );
    fprintf( f, "const struct cnfsFileEntry cnfs_files[NR_FILES] = {\n" );
    for( i = 0; i < nr_file; i++ )
    {
        struct fileEntry * fe = entries + i;
        fprintf( f, "    { \"%s\", %d, %d },\n", fe->filename, fe->len, fe->offset ); 
    }
    fprintf( f, "};\n" );
    fprintf( f, "const uint8_t cnfs_data[%d] = {\n\t", offset );
    int ki = 0;
    for( i = 0; i < nr_file; i++ )
    {
        struct fileEntry * fe = entries + i;
        //fprintf( f, "    // %s\n", fe->filename );
        int k;
        for( k = 0; k < fe->len; k++ )
        {
            fprintf( f, "%d%s", fe->data[k], (k == fe->len-1 ||  ( ( ki & 0xf ) == 0xf )) ? ",\n\t" : ", " );
            ki++;
        }
    }
    fprintf( f, "\n};\n" );
    fclose( f );
    return 0;
}


