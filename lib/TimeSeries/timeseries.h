#ifndef TIMESERIES_H
#define TIMESERIES_H

#include <QList>
#include <QPair>
#include <QFile>

// Comma separation regex
#define REGEX_COMMASEP QRegExp("\\s*,\\s*")

class TimeSeries
{
public:
    TimeSeries();
    bool fromCSV(QFile *csv, int timeColumn);
    void addColumn(QPair<QString, QList<qreal>> column);
    void addColumn(QString header, QList<qreal> data);
    void setTimeColumn(int col);
    int getTimeColumn();
    int numDataColumns();
    int numRows();
    int numColumns();
    QString columnName(int column);
    const QList<qreal> *timeColumnData();
    const QList<qreal> *getColumn(int column);
    const QList<qreal> *getColumn(const QString &colname);
    QList<QPair<QString, QPointF>> rowAt(int i);
    QList<qreal> rowData(int i);
    QList<QPair<QString, QPointF>> linearInterpolate(qreal t, int indexStart, int indexEnd);
    int indexOfLEQ(qreal value, int indexStart, int indexEnd);
private:
    int timeColumn;
    QList<QPair<QString, QList<qreal>>> *data;
};

#endif // TIMESERIES_H
