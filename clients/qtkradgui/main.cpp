#include <QApplication>
#include "mainwindow.h"
#include "resizableframe.h"

int main(int argc, char *argv[])
{
  QApplication a(argc, argv);
  MainWindow w;
  w.show();
//  ResizableFrame *f1=new ResizableFrame();
//  f1->setGeometry(50,50,100,100);

//  QGraphicsScene scene(0,0,800,600);

//  QGraphicsProxyWidget * c1=scene.addWidget(f1);
//  f1->setProxy(c1);

//  QGraphicsView view(&scene);
//  view.show();

  return a.exec();
}
