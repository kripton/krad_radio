#include "filterwidget.h"

FilterWidget::FilterWidget(QString portname, QWidget *parent) :
  QWidget(parent)
{
  this->setToolTip(tr("%1 Filters").arg(portname));
  this->portname = portname;

  this->layout = new QVBoxLayout();
  hipassSlider = new LabelledSlider(tr("hipass"), Qt::Horizontal, 20000, 20);
  lopassSlider = new LabelledSlider(tr("lopass"), Qt::Horizontal, 20000, 20);

  layout->addWidget(hipassSlider);
  layout->addWidget(lopassSlider);
  setLayout(layout);
  connect(hipassSlider, SIGNAL(valueChanged(QString,float)), this, SLOT(hipassUpdated(QString,float)));
  connect(lopassSlider, SIGNAL(valueChanged(QString,float)), this, SLOT(lopassUpdated(QString,float)));
  hipassSlider->setValue(20000);
  lopassSlider->setValue(20);
}


void FilterWidget::lopassUpdated(QString name, float value)
{
  emit loPassFilterUpdated(portname, tr("hz"), (float) value);
}

void FilterWidget::hipassUpdated(QString name, float value)
{
  emit hiPassFilterUpdated(portname, tr("hz"), (float) value);
}


