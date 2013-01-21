#include "broadcastthread.h"

BroadcastThread::BroadcastThread(KradStation *kradStation, QObject *parent) :
  QThread(parent)
{
  this->kradStation = kradStation;
}

void BroadcastThread::run()
{
  kradStation->waitForBroadcasts();
}
