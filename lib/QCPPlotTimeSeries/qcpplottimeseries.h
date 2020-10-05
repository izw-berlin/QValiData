#ifndef QCPPLOTTIMESERIES_H
#define QCPPLOTTIMESERIES_H
#include <qcustomplot.h>
#include <timeseries.h>
#include <QList>
#include <QColor>
#include <QPen>

// Number of QPen stroke styles. By default:
/*
  1 = solid line
  2 = dash line
  3 = dotted line
  4 = dash-dot line
  5 = dash-dot-dot line
  */
#define NUM_QPEN_STYLES 5

namespace QCPPlotTimeSeries{
    const QList<QColor> colors = {QColor(255, 0, 0),     //  Red
                                  QColor(0, 255, 0),     //  Green
                                  QColor(0, 0, 255),     //  Blue
                                  QColor(255, 127, 0),    //  Orange
                                  QColor(0, 255, 255),    //  Cyan
                                  QColor(255, 0, 255)};   //  Magenta

    int plotData(QCustomPlot *plot, TimeSeries *ts);
    QPen getPenStyle(int i);
}


#endif // QCPPLOTTIMESERIES_H
