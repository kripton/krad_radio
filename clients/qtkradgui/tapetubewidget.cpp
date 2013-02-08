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

  connect(driveSlider, SIGNAL(valueChanged(QString,float)), this, SLOT(tapetubeDriveUpdated(QString, float)));
  connect(blendSlider, SIGNAL(valueChanged(QString,float)), this, SLOT(tapetubeBlendUpdated(QString, float)));


}


void TapetubeWidget::tapetubeDriveUpdated(QString name, float value)
{
  emit tapetubeUpdated(portname, tr("drive"), value);
}

void TapetubeWidget::tapetubeBlendUpdated(QString name, float value)
{
  emit tapetubeUpdated(portname, tr("blend"), value);
}
