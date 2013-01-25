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

signals:
    void finished();

private:
    KradStation *kradStation;

};

#endif // BROADCASTTHREAD_H
