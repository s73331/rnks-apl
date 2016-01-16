/*  returns: 
             0 success
             1 error when opening the file at path, mode "r"
            -1 error when closing the file
             2 error when fseek to SEEK_END
             3 error when ftell(path)
             4 error when fseek to SEEK_SET
             5 error when allocating memory for destination
             6 error when reading
        When the absolute value is greater than 1 and the return value is negative, then the file wasn't closed properly.

    The input value of destination does not matter.
*/
int  readfile(char* path, char** destination);

/*  returns:
             0 success
             1 error when opening file at path, mode "w"
            -1 error when closing file
             2 error when writing to file
            -2 error when writing to file and closing the file
*/
int writefile(char* path, char* string);