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

#include <unistd.h>
#include <signal.h>
#include <iostream>
#include <QCoreApplication>
#include <QDir>
#include <QObject>
#include <QEventLoop>
#include <QStringList>
#include <QTimer>
#include <Logger.h>
#include "mts.h"

using namespace meegomtp1dot0;

void signalHandler(int signum)
{
    if(signum == SIGUSR1)
        return;

    // Is this async safe?
    Mts::destroyInstance();
    _exit(0);
}

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);

    signal(SIGINT, &signalHandler);
    signal(SIGALRM, &signalHandler);
    signal(SIGUSR1, &signalHandler);

    QObject::connect(&app,SIGNAL(aboutToQuit()),Mts::getInstance(),SLOT(destroyInstance()));

    bool ok = Mts::getInstance()->activate();
    if( ok )
    {
        if (app.arguments().count() > 1 && app.arguments().at(1) == "-d")
        {
            Buteo::Logger::createInstance(QDir::homePath() + "/mtp.log", false, 0);
            if (!Mts::getInstance()->debugLogsEnabled())
            {
                Mts::getInstance()->toggleDebugLogs();
            }

            Buteo::Logger::instance()->enable();
        }
        app.exec();
    }
    else
    {
        Mts::destroyInstance();
    }
    return 0;
}
