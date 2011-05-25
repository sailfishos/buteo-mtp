#ifndef MTPFSDRIVER_H
#define MTPFSDRIVER_H

#include <QObject>

#include "readerthread.h"

/*! \brief Verbosity level
 */
enum verbosity_level
{
    VERBOSE_OFF,
    VERBOSE_ON
};

/*! \brief MTP versions
 */
enum version
{
    MTP1,
    MTP2
};

/*! \brief MTP USB packet types
 *
 * CONTROL : setup data exchanged over control file ep0
 * DATA : Data IN sent via IN file ep1
 * DATA : Data OUT received via OUT file ep2
 * EVENT : Interrupts sent over INT file ep3
 *
 */
enum packet_type
{
    CONTROL,
    DATA,
    EVENT
};

class MTPFSDriver : public QObject {
    Q_OBJECT
public:
    MTPFSDriver();
    ~MTPFSDriver();
/* \brief Call this to setup the mtp functionfs driver
 *
 * @param mtpversion setup MTP1 or MTP2
 * @param verbosity set VERBOSE_ON if the driver should print debug statements,
 * VERBOSE_OFF otherwise
 * @return -1 if setup failed, 0 otherwise
 *
 */
    int setup(enum version mtpversion, enum verbosity_level verbosity);

/* \brief Call this for cleanup once done
 */
    void closedev();

/* \brief Get handle to control ep file
 *
 * @return the control ep file fd
 * or-1 if the file isn't open
 *
 */
    int mtpfs_get_control_fd();
/* \brief Get handle to control ep file
 *
 * @return the IN ep file fd
 * or-1 if the file isn't open
 *
 */
    int mtpfs_get_in_fd();
/* \brief Get handle to control ep file
 *
 * @return the OUT ep file fd
 * or-1 if the file isn't open
 *
 */
    int mtpfs_get_out_fd();
/* \brief Get handle to control ep file
 *
 * @return the INT ep file fd
 * or-1 if the file isn't open
 *
 */
    int mtpfs_get_interrupt_fd();

/* \brief Call this method to send data/events/setup packets
 * over the corresponding ep file
 *
 * @param fd should be the handle of control/OUT/INT ep files
 * @param packet_type should be CONTROL/DATA/EVENT
 * @param data points to buffer
 * @param len buffer length
 * @param set is_last_packet to 1 if data shouldn't be buffered
 * and shouldbe sent immdediately, 0 if you don't care
 * @return -1 for failure, 0 for success
 *
 */
    int mtpfs_send(int fd, const char* data, unsigned int len,
                   enum packet_type type, unsigned short is_last_packet);

/* \brief Call this to flush any buffered data
 */
    void mtpfs_flush();

/* \brief description of the last error
 *
 * @return pointer to error string
 *
 */
    const char* mtpfs_last_error();

    void mtpfs_send_reset( int fd );

signals:
    void linkEnabled();
    void linkDisabled();

    void outFdChanged(int fd);
    void inFdChanged(int fd);
    void intrFdChanged(int fd);

    void dataRead(char *buffer, int size);

public slots:
    void startIO();
    void stopIO();

private:
    ControlReaderThread *ctrlThread;
    OutReaderThread *outThread;

    int control_fd;
    int in_fd;
    int out_fd;
    int interrupt_fd;
};

#endif
