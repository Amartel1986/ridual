#include <QtGui/QApplication>
#include "Main_window.h"

#include <QStyleFactory>

int main(int argc, char *argv[]) {

  QApplication a(argc, argv);

  Main_window w;
  w.show();
  
  return a.exec();
}
