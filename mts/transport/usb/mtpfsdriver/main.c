#include "mtpfsdriver.h"

int main(int argc, char** argv)
{
    mtpfs_setup(MTP1, VERBOSE_ON);
    while(1){}
    return 0;
}
