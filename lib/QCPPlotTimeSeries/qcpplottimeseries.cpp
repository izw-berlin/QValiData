#include "qcpplottimeseries.h"

/**
 * @brief plotData Plots data on provided QCustomPlot widget
 * @param plot The QCustomPlot widget
 * @param ts The TimeSeries from which to derive data
 * @return The number of graphs plotted
 */
int QCPPlotTimeSeries::plotData(QCustomPlot *plot, TimeSeries *ts){
    int currentGraph = 0; // Keep track of graph index
    for(int col=0; col<ts->numColumns(); ++col){
        if (col != ts->getTimeColumn()){
            plot->addGraph();
            plot->graph(currentGraph)->setData(ts->timeColumnData()->toVector(), ts->getColumn(col)->toVector());
            plot->graph(currentGraph)->setPen(getPenStyle(currentGraph));
            plot->graph(currentGraph)->setName(ts->columnName(col));
            ++currentGraph;
        }
    }
    return currentGraph;
}

/**
 * @brief getPenStyle Helper function for generating unique pen colors and styles
 * @param i A number that uniquely identifies the current pen style
 * @return The pen style generated
 */
QPen QCPPlotTimeSeries::getPenStyle(int i){
    Qt::PenStyle strokeStyle = Qt::PenStyle(((i/colors.size()) % NUM_QPEN_STYLES) + 1);
    QColor theColor = colors.at(i%colors.size());
    return QPen(theColor, 1, strokeStyle);
}
