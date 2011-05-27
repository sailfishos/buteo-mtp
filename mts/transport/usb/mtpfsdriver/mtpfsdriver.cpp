#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include "mtpfsdriver.h"
#include "mtp1descriptors.h"

#include <QDebug>

static char err_string[50] = "No errors";
static enum verbosity_level verbose = VERBOSE_OFF;

static void debug(const char* str)
{
    if(VERBOSE_ON == verbose)
    {
        printf("\n%s\n",str);
    }
}

MTPFSDriver::MTPFSDriver(enum version mtpversion, enum verbosity_level verbosity)
    :  control_fd(-1), in_fd(-1), out_fd(-1), interrupt_fd(-1), outThread(NULL)
{
    mtpfs_setup(mtpversion, verbosity);

    ctrlThread = new ControlReaderThread(control_fd, this);

    QObject::connect(ctrlThread, SIGNAL(startIO()), this, SLOT(startIO()));
    QObject::connect(ctrlThread, SIGNAL(stopIO()), this, SLOT(stopIO()));

    ctrlThread->start();
}

int MTPFSDriver::mtpfs_setup(enum version mtpversion, enum verbosity_level verbosity)
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

void MTPFSDriver::mtpfs_close()
{
    close(control_fd);
    close(in_fd);
    close(out_fd);
    close(interrupt_fd);
    debug("closed mtp function");
}

const char* MTPFSDriver::mtpfs_last_error()
{
    return err_string;
}

int MTPFSDriver::mtpfs_get_control_fd()
{
    return control_fd;
}

int MTPFSDriver::mtpfs_get_in_fd()
{
    return in_fd;
}

int MTPFSDriver::mtpfs_get_out_fd()
{
    return out_fd;
}

int MTPFSDriver::mtpfs_get_interrupt_fd()
{
    return interrupt_fd;
}

void MTPFSDriver::mtpfs_send_reset( int fd )
{
    int err = ioctl(fd, FUNCTIONFS_CLEAR_HALT);
    if (err < 0)
        perror("mtpfs_send_reset");
}

void MTPFSDriver::startIO()
{
    qDebug() << "Starting IO";

    in_fd = open(in_file, O_WRONLY);
    if(-1 == in_fd)
    {
        snprintf(err_string, sizeof err_string, \
                "Couldn't open IN endpoint file %s",in_file);
        debug(err_string);
    }
    emit inFdChanged(in_fd);

    out_fd = open(out_file, O_RDONLY);
    if(-1 == out_fd)
    {
        snprintf(err_string, sizeof err_string, \
               "Couldn't open IN endpoint file %s",out_file);
        debug(err_string);
    }
    if(outThread != NULL)
        delete outThread;
    outThread = new OutReaderThread(out_fd, this);
    QObject::connect(outThread, SIGNAL(dataRead(char*,int)),
        this, SIGNAL(dataRead(char*,int)));
    outThread->start();
    emit outFdChanged(out_fd);

    interrupt_fd = open(interrupt_file, O_WRONLY);
    if(-1 == interrupt_fd)
    {
        snprintf(err_string, sizeof err_string, \
                "Couldn't open IN endpoint file %s",interrupt_file);
        debug(err_string);
    }
    emit intrFdChanged(interrupt_fd);
}

void MTPFSDriver::stopIO()
{
    qDebug() << "Stopping IO";
    // FIXME: this probably won't exit properly?
    delete outThread;
    outThread = NULL;
    if(out_fd != -1) {
        close(out_fd);
        out_fd = -1;
        emit outFdChanged(out_fd);
    }
    if(in_fd != -1) {
        close(in_fd);
        in_fd = -1;
        emit inFdChanged(in_fd);
    }
    if(interrupt_fd != -1) {
        close(interrupt_fd);
        interrupt_fd = -1;
        emit intrFdChanged(interrupt_fd);
    }
}
