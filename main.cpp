#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    //TEMPORARY
    // Bit odd?  My widget locale seems to have OmitGroupSeparator set, preventing 1,000s being shown with their comma
    // Does not help, has to be done in displayText()?
    QLocale locale(QLocale::system());
    locale.setNumberOptions(locale.numberOptions() & ~QLocale::OmitGroupSeparator);
    QLocale::setDefault(locale);

    MainWindow w;
    w.show();

    return a.exec();
}
