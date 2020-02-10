#include <stdio.h>
#include <stdlib.h>

/**************************************************************************//**

  \brief  Read file contents into allocated memory

  \return pointer to string containing file contents

******************************************************************************/
char * file_read(const char * filename, const char * mode)
{
    FILE * fp = fopen(filename, mode);
    if (fp == NULL)
    {
        fprintf(stderr, "Could not open file %s\n", filename);
        /* No need to close the file before returning since it was never opened */
        return NULL;
    }

    /* Get total file size */
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    rewind(fp);

    /* Allocate enough memory for entire file */
    char * buf = malloc(file_size);
    if (buf == NULL)
    {
        fprintf(stderr, "Memory allocation failure\n");
        fclose(fp);
        return NULL;
    }

    /* Read the entire file into a buffer */
    long read_size = fread(buf, 1, file_size, fp);
    if (read_size != file_size)
    {
        fprintf(stderr, "Read %ld of %ld total bytes in file %s\n", read_size, file_size, filename);
        fclose(fp);
        return NULL;
    }

    fclose(fp);
    return buf;
}
