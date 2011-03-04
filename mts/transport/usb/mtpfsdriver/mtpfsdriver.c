#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include "mtpfsdriver.h"
#include "mtp1descriptors.h"

static int control_fd = -1;
static int in_fd = -1;
static int out_fd = -1;
static int interrupt_fd = -1;
static char err_string[50] = "No errors";
static enum verbosity_level verbose = VERBOSE_OFF;

static void debug(const char* str)
{
    if(VERBOSE_ON == verbose)
    {
        printf("\n%s\n",str);
    }
}

int mtpfs_setup(enum version mtpversion, enum verbosity_level verbosity)
{
    int success = -1;

    if(MTP2 == mtpversion)
    {
        snprintf(err_string, sizeof err_string,
                "Only MTP1 support as of now");
        debug(err_string);
    }
    else
    {
        verbose = verbosity;
        control_fd = open(control_file, O_RDWR);
        if(-1 == control_fd)
        {
            snprintf(err_string, sizeof err_string,
                    "Couldn't open control endpoint file %s",control_file);
            debug(err_string);
        }
        else
        {
            if(-1 == write(control_fd, &mtp1descriptors, sizeof mtp1descriptors))
            {
                snprintf(err_string, sizeof err_string, \
                        "Couldn't write descriptors to \
                        control endpoint file %s",control_file);
                debug(err_string);
            }
            else
            {
                if(-1 == write(control_fd, &mtp1strings, sizeof(mtp1strings)))
                {
                    snprintf(err_string, sizeof err_string, \
                            "Couldn't write strings to control \
                            endpoint file %s",control_file);
                    debug(err_string);
                }
                else
                {
                    success = 0;
                    debug("mtp function set up");
                }
            }
        }
    }
    return success;
}

void mtpfs_close()
{
    close(control_fd);
    close(in_fd);
    close(out_fd);
    close(interrupt_fd);
    debug("closed mtp function");
}

const char* mtpfs_last_error()
{
    return err_string;
}

int mtpfs_get_control_fd()
{
    return control_fd;
}

int mtpfs_get_in_fd()
{
    if(-1 == in_fd)
    {
        in_fd = open(in_file, O_WRONLY);
        if(-1 == in_fd)
        {
            snprintf(err_string, sizeof err_string, \
                    "Couldn't open IN endpoint file %s",in_file);
            debug(err_string);
        }
    }
    return in_fd;
}

int mtpfs_get_out_fd()
{
    if(-1 == out_fd)
    {
        out_fd = open(out_file, O_RDONLY);
        if(-1 == out_fd)
        {
            snprintf(err_string, sizeof err_string, \
                "Couldn't open IN endpoint file %s",out_file);
            debug(err_string);
        }
    }
    return out_fd;
}

int mtpfs_get_interrupt_fd()
{
    if(-1 == interrupt_fd)
    {
        interrupt_fd = open(interrupt_file, O_WRONLY);
        if(-1 == interrupt_fd)
        {
            snprintf(err_string, sizeof err_string, \
                    "Couldn't open IN endpoint file %s",interrupt_file);
            debug(err_string);
        }
    }
    return interrupt_fd;
}
