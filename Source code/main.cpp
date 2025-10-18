#include "ClassScheduleApp.h"
#include <QApplication>

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);
    ClassScheduleApp w;
    w.show();
    return a.exec();
}