#ifndef TAPETUBEWIDGET_H
#define TAPETUBEWIDGET_H

#include <QWidget>
#include <QSlider>
#include "labelledslider.h"

class TapetubeWidget : public QWidget
{
  Q_OBJECT
public:
  explicit TapetubeWidget(QString portname, QWidget *parent = 0);
  
signals:
  void tapetubeUpdated(QString portname, QString control, float value);
  void passFilterUpdated(QString portname, QString control, float value);
public slots:
  void tapetubeDriveUpdated(QString name, float value);
  void tapetubeBlendUpdated(QString name, float value);
private:
  QHBoxLayout *layout;
  QString portname;
  int fxId;
  LabelledSlider *driveSlider;
  LabelledSlider *blendSlider;

  
};

#endif // TAPETUBEWIDGET_H
