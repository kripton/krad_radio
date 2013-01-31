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

}

void PortgroupSlider::xmms2Play()
{
  emit xmms2Play(portname);
}

void PortgroupSlider::xmms2Pause()
{
  emit xmms2Pause(portname);
}

void PortgroupSlider::xmms2Next()
{
  emit xmms2Next(portname);
}

void PortgroupSlider::xmms2Prev()
{
  emit xmms2Prev(portname);
}


