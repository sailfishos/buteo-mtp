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
#include <QCommandLineParser>
#include <QDir>
#include <QObject>
#include <QEventLoop>
#include <QStringList>
#include <QTimer>
#include <QLoggingCategory>
#include <MGConfItem>
#include "mts.h"

using namespace meegomtp1dot0;

void signalHandler(int signum, siginfo_t *info, void *context)
{
    Q_UNUSED(signum);
    Q_UNUSED(info);
    Q_UNUSED(context);

    // Is this async safe?
    Mts::destroyInstance();
    _exit(0);
}

static void setupSymLinkPolicy()
{
    MGConfItem confItem("/desktop/sailfish/buteo-mtp/symlink_policy");
    QString symLinkPolicy(confItem.value().toString());

    /* Note that we override value that might be existing in env,
     * i.e. environment is used only as a vehicle for conveying the
     * dconf setting data from main binary over to plugin that gets
     * loaded at later stage.
     */
    qputenv("BUTEO_MTP_SYMLINK_POLICY", symLinkPolicy.toUtf8());
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QCommandLineParser parser;
    parser.setApplicationDescription("standalone buteo-mtp daemon");
    parser.addHelpOption();
    parser.process(app);

    /* Get symlink policy from dconf */
    setupSymLinkPolicy();

    struct sigaction action;

    action.sa_sigaction = signalHandler;
    action.sa_flags     = SA_SIGINFO | SA_RESTART;
    sigemptyset(&action.sa_mask);

    if (sigaction(SIGINT, &action, NULL) < 0)
        return EXIT_FAILURE;
    if (sigaction(SIGALRM, &action, NULL) < 0)
        return EXIT_FAILURE;
    if (sigaction(SIGUSR1, &action, NULL) < 0)
        return EXIT_FAILURE;

    QObject::connect(&app, SIGNAL(aboutToQuit()), Mts::getInstance(), SLOT(destroyInstance()));

    bool ok = Mts::getInstance()->activate();
    if ( ok ) {
        QLoggingCategory lcMtp("buteo.mtp");
        lcMtp.setEnabled(QtDebugMsg, Mts::getInstance()->debugLogsEnabled());
        app.exec();
    } else {
        Mts::destroyInstance();
    }
    return EXIT_SUCCESS;
}
