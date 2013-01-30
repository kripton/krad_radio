#include "eq.h"

EqWidget::EqWidget(QString portname, QWidget *parent) :
  QWidget(parent)
{
  this->setToolTip(tr("%1 EQ").arg(portname));
  this->portname = portname;
  eqLayout = new QHBoxLayout ();
  this->setLayout(eqLayout);
 // connect(this, SIGNAL(destroyed()), this, SLOT(destroyed()));
}

void EqWidget::addBand(float freq)
{

  int bandId = freqs.length();
  freqs.append(freq);
  EqSlider *eqSlider = new EqSlider(bandId, freq, this);
  eqLayout->addWidget(eqSlider);
  connect(eqSlider, SIGNAL(valueChanged(int,int)), this, SLOT(bandDbChanged(int,int)));

  emit bandAdded(portname, bandId, freq);
}

void EqWidget::bandDbChanged(int bandId, int freq)
{
  emit bandDbChanged(portname, bandId, freq);
}
