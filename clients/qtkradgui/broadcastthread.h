#ifndef BROADCASTTHREAD_H
#define BROADCASTTHREAD_H

#include <QThread>
#include "kradstation.h"

class BroadcastThread : public QThread
{
  Q_OBJECT
public:
  explicit BroadcastThread(QObject *parent = 0);
  explicit BroadcastThread(KradStation *kradStation, QObject *parent = 0);

  void run();
signals:
  
public slots:
  
private:
  KradStation *kradStation;

};

#endif // BROADCASTTHREAD_H
