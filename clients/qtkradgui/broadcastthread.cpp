#include "broadcastthread.h"

BroadcastThread::BroadcastThread(KradStation *kradStation, QObject *parent) :
  QObject(parent)
{
  this->kradStation = kradStation;
}

void BroadcastThread::process()
{
  kradStation->waitForBroadcasts();
}
