/*
 * This file is part of buteo-mtp package
 *
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
 *
 * Contact: Sateesh Kavuri <sateesh.kavuri@nokia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#include <signal.h>
#include <iostream>
#include <QCoreApplication>
#include "mts_gfs.h"

static Mts *m = 0;

void signalHandler(int /*signum*/)
{
    mts_stop(m);
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    system("source /tmp/session_bus_address.user");
    m = mts_init();
    if (!m) {
        return 1;
    }

    signal(SIGINT, &signalHandler);
    signal(SIGALRM, &signalHandler);

    mts_run(m);

    mts_free(m);

    printf("\nEnded normally\n");

    return 0;
}
