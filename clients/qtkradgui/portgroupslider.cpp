#include "portgroupslider.h"

PortgroupSlider::PortgroupSlider(QString label, QWidget *parent) :
  LabelledSlider(label, Qt::Vertical, 100, 0, parent)
{

  portname = label;
  QHBoxLayout *bg = new QHBoxLayout(this);

  QPushButton *addEfxButton = new QPushButton(tr("+ Efx"));
  QPushButton *rmEfxButton = new QPushButton(tr("- Efx"));
  bg->addWidget(addEfxButton);
  bg->addWidget(rmEfxButton);
  layout->addLayout(bg);

  connect(addEfxButton, SIGNAL(clicked()), this, SLOT(addEffects()));
  connect(rmEfxButton, SIGNAL(clicked()), this, SLOT(removeEffects()));
}


void PortgroupSlider::addEffects()
{
  emit addEffects(portname);
}

void PortgroupSlider::removeEffects()
{
  emit removeEffects(portname);
}
