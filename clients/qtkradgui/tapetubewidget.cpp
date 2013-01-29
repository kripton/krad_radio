#include "tapetubewidget.h"

TapetubeWidget::TapetubeWidget(QString portname, QWidget *parent) :
  QWidget(parent)
{
  this->setToolTip(tr("%1 Tapetube").arg(portname));
  this->portname = portname;
  layout = new QHBoxLayout(this);
  driveSlider = new LabelledSlider(tr("drive"), Qt::Vertical, 10, 0);
  blendSlider = new LabelledSlider(tr("blend"), Qt::Vertical, 10, -10);

  layout->addWidget(driveSlider);
  layout->addWidget(blendSlider);

  connect(driveSlider, SIGNAL(valueChanged(QString,int)), this, SLOT(tapetubeDriveUpdated(QString, int)));
  connect(blendSlider, SIGNAL(valueChanged(QString,int)), this, SLOT(tapetubeBlendUpdated(QString, int)));

}


void TapetubeWidget::tapetubeDriveUpdated(QString name, int value)
{
  emit tapetubeUpdated(portname, tr("drive"), value);
}

void TapetubeWidget::tapetubeBlendUpdated(QString name, int value)
{
  emit tapetubeUpdated(portname, tr("blend"), value);
}
