#include "timeseries.h"
#include <QtMath>
#include <QDebug>
#include <QPoint>

TimeSeries::TimeSeries()
{
    data = new QList<QPair<QString, QList<qreal>>>();
    timeColumn = -1;
}

// Get a time series from CSV data
bool TimeSeries::fromCSV(QFile *csv, int timeColumn){
    if(!csv->isOpen()){
        bool openResult = csv->open(QFile::ReadOnly); // Try to open the file if not already open
        if(!openResult){
            return false;
        }
    }
    // Empty file check
    if(csv->atEnd()){
        return false;
    }

    QList<QString> header = QString(csv->readLine().trimmed()).split(REGEX_COMMASEP, QString::KeepEmptyParts);
    QList<QList<qreal>* > data;
    // Make the columns
    for(int i=0; i<header.size(); i++){
        data.append(new QList<qreal>());
    }
    while(!csv->atEnd()){
        QList<QString> line = QString(csv->readLine().trimmed()).split(REGEX_COMMASEP, QString::KeepEmptyParts);
        for(int i=0; i<header.size(); i++){
            // Try to fill in valid numerical data, otherwise, give it a zero.
            if(line.size() > i){
                // toDouble is intrinsically safe i.e. it always returns a valid double (0 in case of failure)
                data.at(i)->append(line.at(i).toDouble());
            }
            else{
                data.at(i)->append(0);
            }
        }
    }
    for(int i=0; i<header.size(); i++){
        addColumn(header.at(i), *(data.at(i)));
    }
    setTimeColumn(timeColumn);
    return true;
}

void TimeSeries::addColumn(QString header, QList<qreal> data){
    addColumn(QPair<QString, QList<qreal>>(header, data));
}
void TimeSeries::addColumn(QPair<QString, QList<qreal>> column){
    data->append(column);
}

void TimeSeries::setTimeColumn(int col){
    timeColumn = col;
}

int TimeSeries::getTimeColumn(){
    return timeColumn;
}

int TimeSeries::numColumns(){
    return data->size();
}

int TimeSeries::numDataColumns(){ //Returns number of data (non-time) columns
    return data->size() - 1;
}

QString TimeSeries::columnName(int column){
    return data->at(column).first;
}

const QList<qreal>* TimeSeries::getColumn(const QString &colname){
    for(int i=0; i<data->size(); i++){
        if(data->at(i).first.compare(colname) == 0){
            return &data->at(i).second;
        }
    }
    return nullptr;
}

const QList<qreal>* TimeSeries::getColumn(int column){
    return &data->at(column).second;
}

const QList<qreal>* TimeSeries::timeColumnData(){
    return getColumn(timeColumn);
}

int TimeSeries::numRows(){ //Returns length of time series, as the number of rows of the current time column.
    return data->at(timeColumn).second.size();
}

//Makes a list comprised of points representing a single row of the time series.
QList<QPair<QString, QPointF>> TimeSeries::rowAt(int i){
    qreal timeAt = data->at(timeColumn).second.at(i); //Gets the time at row i.
    QList<QPair<QString, QPointF>> out = QList<QPair<QString, QPointF>>();

    for(int col=0; col<numDataColumns() + 1; col++){ //Cycle through all the columns
        if(col != timeColumn){
            out.append(QPair<QString, QPointF>(data->at(col).first,
                                                 QPointF(timeAt, data->at(col).second.at(i))));
        }
    }
    return out;
}

QList<qreal> TimeSeries::rowData(int i){
    QList<qreal> out = QList<qreal>();
    for(int col=0; col<numColumns(); col++){
        out.append(data->at(col).second.at(i));
    }
    return out;
}

QList<QPair<QString, QPointF>> TimeSeries::linearInterpolate(qreal t, int indexStart, int indexEnd){
    QList<QPair<QString, QPointF>> out = QList<QPair<QString, QPointF>>();

    int indexBefore = indexOfLEQ(t, indexStart, indexEnd);
    QList<QPair<QString, QPointF>> rowBefore = rowAt(indexBefore);

    if(indexBefore == (numRows() - 1))
        out = rowBefore;
    else{
        int indexAfter = indexBefore + 1;
        QList<QPair<QString, QPointF>> rowAfter = rowAt(indexAfter);
        //Time difference between the two points
        qreal dTimePoints = data->at(timeColumn).second.at(indexAfter) - data->at(timeColumn).second.at(indexBefore);
        //Time difference between first point and t
        qreal dT = t - data->at(timeColumn).second.at(indexBefore);
        for(int i=0; i<rowBefore.size(); i++){ //Iterate through all data columns
            QPointF pointBefore = rowBefore.at(i).second;
            QPointF pointAfter = rowAfter.at(i).second;
            //Append a point with the column label and new interpolated coordinates
            out.append(QPair<QString, QPointF>(rowBefore.at(i).first, QPointF(t, pointBefore.y() + (pointAfter.y() - pointBefore.y()) / dTimePoints * dT)));
        }
    }
    return out;
}

int TimeSeries::indexOfLEQ(qreal value, int indexStart, int indexEnd){
    int _start = indexStart;
    int _end = indexEnd;
    int _mid;
    const QList<qreal> *column = &(data->at(timeColumn).second);

    if(column->at(_start) > value){
        return -1;
    }

    while(_start != _end){
        if(_start == _end)
            return _start;
        _mid = qCeil((_start + _end) / 2.0);
        if(column->at(_mid) > value)
            _end = _mid - 1;
        else
            _start = _mid;
    }
    return _start;
}
