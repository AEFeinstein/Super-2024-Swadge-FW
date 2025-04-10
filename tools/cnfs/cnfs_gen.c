#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <stdint.h>

int stringcmp(const void* a, const void* b);
char* filenameToEnumName(const char* filename);

#define MAX_FILES 8192
#define CNFS_PATH_MAX  4096

/**
 * @brief Wrapper for strcmp() to be used by qsort()
 * 
 * @param a A string to compare
 * @param b Another string to compare
 * @return 0 if the strings are equal, non-zero otherwise
 */
int stringcmp(const void* a, const void* b)
{
    return strcmp(*(const char* const*)a, *(const char* const*)b);
}

/**
 * @brief TODO
 * 
 * @param filename 
 * @return char* 
 */
char* filenameToEnumName(const char* filename)
{
    char* enumName = calloc(1, strlen(filename) * 2);
    strcpy(enumName, filename);

    int chIdx = 0;
    while(enumName[chIdx])
    {
        if( (0 == chIdx) && ('0' <= enumName[chIdx + 1] && enumName[chIdx + 1] <= '9'))
        {
            // Can't start with a number, Insert an underscore
            memmove(&enumName[chIdx + 1], &enumName[chIdx], strlen(enumName) - chIdx);
            enumName[chIdx] = '_';
        }
        else if('a' <= enumName[chIdx] && enumName[chIdx] <= 'z')
        {
            // lowercase to uppercase
            enumName[chIdx] += ('A' - 'a');

            // If the next character is uppercase or a number
            if (('A' <= enumName[chIdx + 1] && enumName[chIdx + 1] <= 'Z') ||
                ('0' <= enumName[chIdx + 1] && enumName[chIdx + 1] <= '9'))
            {
                // Insert an underscore
                memmove(&enumName[chIdx + 2], &enumName[chIdx + 1], strlen(enumName) - chIdx + 1);
                enumName[chIdx + 1] = '_';
            }
        }
        else if(('A' <= enumName[chIdx] && enumName[chIdx] <= 'Z') ||
                ('0' <= enumName[chIdx] && enumName[chIdx] <= '9'))
        {
            // No change to uppercase, numbers
        }
        else
        {
            // Replace everything else with underscores
            enumName[chIdx] = '_';
        }

        chIdx++;
    }

    return enumName;
}

/**
 * @brief TODO
 * 
 * @param argc 
 * @param argv 
 * @return int 
 */
int main(int argc, char** argv)
{
    // Make sure enough arguments are supplied
    if (argc != 4)
    {
        fprintf(stderr, "Error: Usage: cnfs_gen folder/ image.c image.h\n");
        return -5;
    }

    // Open the input directory
    struct dirent* dp;
    DIR* dir = opendir(argv[1]);
    if (!dir)
    {
        fprintf(stderr, "Error: Can't open %s\n", argv[1]);
        return -5;
    }

    // Traverse the input directory and save the file names in an array
    char* filelist[MAX_FILES] = {0};
    int numfiles_in = 0;
    while ((dp = readdir(dir))) // if dp is null, there's no more content to read
    {
        if (strcmp(dp->d_name, ".") != 0 && strcmp(dp->d_name, "..") != 0)
        {
            filelist[numfiles_in++] = strdup(dp->d_name);
        }
    }
    // close the handle (pointer)
    closedir(dir);

    // Sort all input files
    qsort(filelist, numfiles_in, sizeof(char*), stringcmp);

    // A list of all the data required for an input file
    struct fileEntry
    {
        char* filename;
        uint8_t* data;
        int offset;
        int len;
    } entries[MAX_FILES];

    // A count of input files
    int nr_file = 0;

    // The offset for the output file
    int offset      = 0;

    // For each input file
    for (int fno = 0; fno < numfiles_in; fno++)
    {
        // Build a path to the input file
        char* fname_in = filelist[fno];
        char fname[CNFS_PATH_MAX];
        snprintf(fname, CNFS_PATH_MAX, "%s/%s", argv[1], fname_in);

        // Open the input file
        FILE* f = fopen(fname, "rb");
        if (!f)
        {
            fprintf(stderr, "Error: Can't open file %s\n", fname);
            free(fname_in);
            continue;
        }

        // Read the file length
        fseek(f, 0, SEEK_END);
        int len = ftell(f);
        fseek(f, 0, SEEK_SET);

        // Save the input file into entries[]
        struct fileEntry* fe = &entries[nr_file];
        fe->data             = malloc(len);
        fe->len              = len;
        int readLen          = fread(fe->data, 1, len, f);
        fe->filename         = fname_in;
        if (readLen != fe->len)
        {
            fprintf(stderr, "Error: File %s truncated (expected %d bytes, got %d)\n", fname, fe->len, readLen);
        }
        else
        {
            // Save the offset for this file
            fe->offset = offset;
            // Move the global offset
            offset += fe->len;
            // Increment the file IDX
            nr_file++;
        }

        // Close the input file
        fclose(f);
    }

    // Open the output file header
    FILE* f = fopen(argv[3], "w");
    if (!f)
    {
        fprintf(stderr, "Error: cannot open %s\n", argv[3]);
        return -19;
    }

    // Write the output header file
    fprintf(f, "#pragma once\n");
    fprintf(f, "\n");
    fprintf(f, "#include <stdint.h>\n");
    fprintf(f, "\n");
    fprintf(f, "typedef struct\n");
    fprintf(f, "{\n");
    // fprintf(f, "    const char* name;\n");
    fprintf(f, "    uint32_t len;\n");
    fprintf(f, "    uint32_t offset;\n");
    fprintf(f, "} cnfsFileEntry;\n");
    fprintf(f, "\n");
    fprintf(f, "typedef enum\n");
    fprintf(f, "{\n");
    for (int i = 0; i < nr_file; i++)
    {
        struct fileEntry* fe = &entries[i];
        char* enumName = filenameToEnumName(fe->filename);
        fprintf(f, "    %s = %d,\n", enumName, i);
        free(enumName);
    }
    fprintf(f, "    %s = %d,\n", "CNFS_NUM_FILES", nr_file);
    fprintf(f, "} cnfsFileIdx_t;\n");
    fprintf(f, "\n");
    fprintf(f, "const uint8_t* getCnfsImage(void);\n");
    fprintf(f, "int32_t getCnfsSize(void);\n");
    fprintf(f, "const cnfsFileEntry* getCnfsFiles(void);\n");
    fprintf(f, "int32_t getCnfsNumFiles(void);\n");
    fclose(f);

    int directorySize = 0;

    // Get the name of the header without the path
    char* hdrNoPath = strrchr(argv[3], '/');
    if (NULL == hdrNoPath)
    {
        // Slash not found, use name as-is
        hdrNoPath = argv[3];
    }
    else
    {
        // Advance past the slash
        hdrNoPath++;
    }

    // Open the output C file
    f = fopen(argv[2], "w");
    if (!f)
    {
        fprintf(stderr, "Error: cannot open %s\n", argv[2]);
        return -20;
    }

    // Write the cnfs_files[] array, which is a table of file names, lengths, and offsets
    fprintf(f, "#include <stdint.h>\n");
    fprintf(f, "#include \"%s\"\n", hdrNoPath);
    fprintf(f, "\n");
    fprintf(f, "#define NR_FILES %d\n", nr_file);
    fprintf(f, "\n");
    fprintf(f, "const cnfsFileEntry cnfs_files[NR_FILES] = {\n");
    for (int i = 0; i < nr_file; i++)
    {
        struct fileEntry* fe = entries + i;
        // fprintf(f, "    { \"%s\", %d, %d },\n", fe->filename, fe->len, fe->offset);
        fprintf(f, "    { .len = %d, .offset = %d },\n", fe->len, fe->offset);
        directorySize += (((strlen(fe->filename) + 1) + 3) & (~3)) + 12;
    }
    fprintf(f, "};\n");
    fprintf(f, "\n");

    // Write the input file data to the output C file
    fprintf(f, "const uint8_t cnfs_data[%d] = {\n\t", offset);
    int ki = 0;
    for (int i = 0; i < nr_file; i++)
    {
        struct fileEntry* fe = entries + i;
        // fprintf( f, "    // %s\n", fe->filename );
        for (int k = 0; k < fe->len; k++)
        {
            fprintf(f, "0x%02X%s", fe->data[k], (k == fe->len - 1 || ((ki & 0xf) == 0xf)) ? ",\n\t" : ", ");
            ki++;
        }
    }
    fprintf(f, "\n};\n");
    fprintf(f, "\n");

    // Write some helper functions
    fprintf(f, "const uint8_t* getCnfsImage(void)\n");
    fprintf(f, "{\n");
    fprintf(f, "    return cnfs_data;\n");
    fprintf(f, "}\n");
    fprintf(f, "\n");
    fprintf(f, "int32_t getCnfsSize(void)\n");
    fprintf(f, "{\n");
    fprintf(f, "    return sizeof(cnfs_data);\n");
    fprintf(f, "}\n");
    fprintf(f, "\n");
    fprintf(f, "const cnfsFileEntry* getCnfsFiles(void)\n");
    fprintf(f, "{\n");
    fprintf(f, "    return cnfs_files;\n");
    fprintf(f, "}\n");
    fprintf(f, "\n");
    fprintf(f, "int32_t getCnfsNumFiles(void)\n");
    fprintf(f, "{\n");
    fprintf(f, "    return NR_FILES;\n");
    fprintf(f, "}\n");
    fclose(f);

    // Debug print
    printf("Image size: %d bytes\n", offset);
    printf("Directory size: %d bytes\n", directorySize);

    // Free everything
    for (int idx = 0; idx < numfiles_in; idx++)
    {
        free(entries[idx].data);
        free(filelist[idx]);
    }

    return 0;
}
