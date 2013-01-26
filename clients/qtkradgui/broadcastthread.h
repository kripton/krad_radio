#ifndef BROADCASTTHREAD_H
#define BROADCASTTHREAD_H

#include "kradstation.h"

class BroadcastThread : public QObject
{
    Q_OBJECT
public:
    explicit BroadcastThread(QObject *parent = 0);
    explicit BroadcastThread(KradStation *kradStation, QObject *parent = 0);

public slots:
    void process();
    void stopprocessing();

signals:
    void finished();

private:
    KradStation *kradStation;
    bool mystopprocessing;

};

#endif // BROADCASTTHREAD_H
