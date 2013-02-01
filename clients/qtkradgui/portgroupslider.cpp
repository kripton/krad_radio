#include "portgroupslider.h"

PortgroupSlider::PortgroupSlider(QString label, QWidget *parent) :
  LabelledSlider(label, Qt::Vertical, 100, 0, parent)
{

  portname = label;
  buttonAdd = 1;
  QHBoxLayout *bg = new QHBoxLayout(this);

  toggleFx = new QCheckBox(tr("Effects"));

  connect(toggleFx, SIGNAL(stateChanged(int)), this, SLOT(fxToggled(int)));
  bg->addWidget(toggleFx);

  layout->addLayout(bg);

}


void PortgroupSlider::addEffects()
{
  emit addEffects(portname);
}

void PortgroupSlider::removeEffects()
{
  emit removeEffects(portname);
}

void PortgroupSlider::fxToggled(int state)
{
  if (state == Qt::Checked) {
    addEffects();
  } else if (state == Qt::Unchecked) {
    removeEffects();
  }
}



void PortgroupSlider::addXmms2Controls()
{

  playAction = new QAction(style()->standardIcon(QStyle::SP_MediaPlay), tr("Play"), this);
  playAction->setShortcut(tr("Ctrl+P"));
  //playAction->setDisabled(true);
  pauseAction = new QAction(style()->standardIcon(QStyle::SP_MediaPause), tr("Pause"), this);
  pauseAction->setShortcut(tr("Ctrl+A"));
  //pauseAction->setDisabled(true);
  stopAction = new QAction(style()->standardIcon(QStyle::SP_MediaStop), tr("Stop"), this);
  stopAction->setShortcut(tr("Ctrl+S"));
  //stopAction->setDisabled(true);
  nextAction = new QAction(style()->standardIcon(QStyle::SP_MediaSkipForward), tr("Next"), this);
  nextAction->setShortcut(tr("Ctrl+N"));
  previousAction = new QAction(style()->standardIcon(QStyle::SP_MediaSkipBackward), tr("Previous"), this);
  previousAction->setShortcut(tr("Ctrl+R"));

  connect(playAction, SIGNAL(triggered()), this, SLOT(xmms2Play()));
  connect(pauseAction, SIGNAL(triggered()), this, SLOT(xmms2Pause()));
  connect(stopAction, SIGNAL(triggered()), this, SLOT(xmms2Stop()));
  connect(nextAction, SIGNAL(triggered()), this, SLOT(xmms2Next()));
  connect(previousAction, SIGNAL(triggered()), this, SLOT(xmms2Prev()));

  QToolBar *bar = new QToolBar;

  bar->addAction(previousAction);
  bar->addAction(playAction);
  bar->addAction(pauseAction);
  bar->addAction(stopAction);
  bar->addAction(nextAction);

  layout->insertWidget(3, bar);
   /*

      QHBoxLayout *playbackLayout = new QHBoxLayout;
      playbackLayout->addWidget(bar);
      playbackLayout->addStretch();
      playbackLayout->addWidget(volumeLabel);
      playbackLayout->addWidget(volumeSlider);

      QVBoxLayout *mainLayout = new QVBoxLayout;
      mainLayout->addWidget(musicTable);
      mainLayout->addLayout(seekerLayout);
      mainLayout->addLayout(playbackLayout);

      QWidget *widget = new QWidget;
      widget->setLayout(mainLayout);

      setCentralWidget(widget);
      setWindowTitle("Phonon Music Player");

  /*
  QHBoxLayout *hbox = new QHBoxLayout();
  QPushButton *play = new QPushButton(tr("Play"));
  QPushButton *prev = new QPushButton(tr("Prev"));
  QPushButton *next = new QPushButton(tr("Next"));
  QPushButton *pause = new QPushButton(tr("Pause"));
  hbox->addWidget(prev);
  hbox->addWidget(play);
  hbox->addWidget(pause);
  hbox->addWidget(next);
  layout->insertLayout(3,hbox);
  connect(play, SIGNAL(clicked()), this, SLOT(xmms2Play()));
  connect(pause, SIGNAL(clicked()), this, SLOT(xmms2Pause()));
  connect(next, SIGNAL(clicked()), this, SLOT(xmms2Next()));
  connect(prev, SIGNAL(clicked()), this, SLOT(xmms2Prev()));
*/
}

void PortgroupSlider::xmms2Play()
{
  emit xmms2Play(portname);
}

void PortgroupSlider::xmms2Pause()
{
  emit xmms2Pause(portname);
}

void PortgroupSlider::xmms2Stop()
{
  emit xmms2Stop(portname);
}

void PortgroupSlider::xmms2Next()
{
  emit xmms2Next(portname);
}

void PortgroupSlider::xmms2Prev()
{
  emit xmms2Prev(portname);
}


