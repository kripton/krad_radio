#ifndef EQ_H
#define EQ_H

#include <QWidget>
#include "eqslider.h"

class EqWidget : public QWidget
{
  Q_OBJECT
public:
  explicit EqWidget(QWidget *parent = 0);

signals:
  void bandAdded(int bandId, float freq);
  void bandDbChanged(int bandId, int freq);
public slots:
  void addBand(float freq);
  //void eqUpdated()
private:

  QHBoxLayout *eqLayout;
  QString portname;
  int fxId;
  QList<float> freqs;
};

#endif // EQ_H
