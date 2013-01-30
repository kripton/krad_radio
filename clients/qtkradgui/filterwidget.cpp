#include "filterwidget.h"

FilterWidget::FilterWidget(QString portname, QWidget *parent) :
  QWidget(parent)
{
  this->setToolTip(tr("%1 Filters").arg(portname));
  this->portname = portname;
  layout = new QVBoxLayout(this);
  hipassSlider = new LabelledSlider(tr("hipass"), Qt::Horizontal, 20000, 20);
  lopassSlider = new LabelledSlider(tr("lopass"), Qt::Horizontal, 20000, 20);

  layout->addWidget(hipassSlider);
  layout->addWidget(lopassSlider);

  connect(hipassSlider, SIGNAL(valueChanged(QString,int)), this, SLOT(hipassUpdated(QString, int)));
  connect(lopassSlider, SIGNAL(valueChanged(QString,int)), this, SLOT(lopassUpdated(QString, int)));

}


void FilterWidget::lopassUpdated(QString name, int value)
{
  emit filterUpdated(portname, tr("lopass"), value);
}

void FilterWidget::hipassUpdated(QString name, int value)
{
  emit filterUpdated(portname, tr("hipass"), value);
}


