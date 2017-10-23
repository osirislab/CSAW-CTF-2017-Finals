#include <util.h>

int get_unsigned_input(FILE *socket, unsigned int *data)
{
    // TODO check scanf output better..
    char buf[10];
    int scanf_ret;
    char *fgets_ret;

    fgets_ret = fgets(buf, 10, socket);
    if (fgets_ret == NULL) {
        return -1;
    }
    scanf_ret = sscanf(buf, "%u", data);
    fflush(socket); 

    return scanf_ret;
}
