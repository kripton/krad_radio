#include "broadcastthread.h"

#include <QDebug>

BroadcastThread::BroadcastThread(KradStation *kradStation, QObject *parent) :
    QObject(parent)
{
    this->kradStation = kradStation;
    mystopprocessing = false;
}

void BroadcastThread::process()
{
    while (!mystopprocessing) {
        qDebug() << "THREAD:" << mystopprocessing;
        kradStation->waitForBroadcasts();
    }
}

void BroadcastThread::stopprocessing()
{
    mystopprocessing = true;
}
