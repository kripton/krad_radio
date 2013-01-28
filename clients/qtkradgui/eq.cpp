#include "eq.h"

EqWidget::EqWidget(QWidget *parent) :
  QWidget(parent)
{
  eqLayout = new QHBoxLayout ();
  this->setLayout(eqLayout);
}

void EqWidget::addBand(float freq)
{

  int bandId = freqs.length();
  freqs.append(freq);
  EqSlider *eqSlider = new EqSlider(bandId, this);
  eqLayout->addWidget(eqSlider);
  connect(eqSlider, SIGNAL(valueChanged(int,int)), this, SIGNAL(bandDbChanged(int,int)));
  emit bandAdded(bandId, freq);
}
