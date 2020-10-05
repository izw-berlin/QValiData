#include "syncview.h"
#include "ui_syncview.h"

using namespace cv;

SyncView::SyncView(QFrame *parent) :
    QFrame(parent),
    ui(new Ui::SyncView)
{
    ui->setupUi(this);

    data = new TimeSeries();
    deltaTVD = 0;
    rateMultiplier = 1.0;

    connect(ui->xScrollBar, SIGNAL(valueChanged(int)), this, SLOT(horzScrollBarChanged(int)));
    connect(ui->customPlot->xAxis, SIGNAL(rangeChanged(QCPRange)), this, SLOT(xAxisChanged(QCPRange)));

    ui->playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    ui->forwardButton->setIcon(style()->standardIcon(QStyle::SP_MediaSkipForward));
    ui->backButton->setIcon(style()->standardIcon(QStyle::SP_MediaSkipBackward));
}

void SyncView::init(){
    ui->trackBar->setTickInterval(1);
    ui->trackBar->setMinimum(0);
    ui->trackBar->setMaximum(ui->vidWidget->getTotalFrames() - 1);

    qreal firstTime = data->timeColumnData()->first();
    qreal lastTime = data->timeColumnData()->last();

    dataLength = lastTime - firstTime;
    ui->xScrollBar->setRange(qFloor(firstTime*100.0), qCeil(lastTime*100.0));

    ui->customPlot->setInteraction(QCP::iRangeZoom, true);
    ui->customPlot->setInteraction(QCP::iRangeDrag, true);
    ui->customPlot->axisRect(0)->setRangeZoom(Qt::Horizontal);
    ui->customPlot->axisRect(0)->setRangeDrag(Qt::Horizontal);

    ui->customPlot->clearGraphs();
    ui->customPlot->clearItems();
    QCPPlotTimeSeries::plotData(ui->customPlot, data);

    ui->customPlot->legend->setVisible(true);

    ui->customPlot->xAxis->setRange(firstTime, firstTime + (ui->vidWidget->getDurationMs()/1000.0));
    ui->customPlot->yAxis->rescale();

    // Track bar
    plotTrackBar = new QCPItemStraightLine(ui->customPlot);
    plotTrackBar->point1->setCoords(0, 0);
    plotTrackBar->point2->setCoords(0, 1);
    plotTrackBar->setPen(QPen(QColor(255, 0, 0), 3));
    plotTrackBar->setSelectedPen(QPen(QColor(255, 127, 127), 5));
    plotTrackBar->setSelectable(true);

    // Video start point
    plotVidStart = new QCPItemStraightLine(ui->customPlot);
    plotVidStart->point1->setCoords(0, 0);
    plotVidStart->point2->setCoords(0, 1);
    plotVidStart->setPen(QPen(QBrush(QColor(64, 64, 64)), 1, Qt::DashLine));
    plotVidStart->setSelectable(false);

    // Video end point
    plotVidEnd = new QCPItemStraightLine(ui->customPlot);
    plotVidEnd->point1->setCoords(ui->vidWidget->getDurationMs()/1000.0, 0);
    plotVidEnd->point2->setCoords(ui->vidWidget->getDurationMs()/1000.0, 1);
    plotVidEnd->setPen(QPen(QBrush(QColor(32, 32, 32)), 3, Qt::SolidLine));
    plotVidEnd->setSelectedPen(QPen(QColor(127, 127, 127), 5));
    plotVidEnd->setSelectable(true);

    // Text for video start point
    plotVidStartText = new QCPItemText(ui->customPlot);
    plotVidStartText->setPositionAlignment(Qt::AlignBottom|Qt::AlignHCenter);
    plotVidStartText->position->setTypeX(QCPItemPosition::ptPlotCoords);
    plotVidStartText->position->setTypeY(QCPItemPosition::ptAxisRectRatio);
    plotVidStartText->position->setParentAnchorX(plotVidStart->point1);
    plotVidStartText->position->setCoords(0, 0);
    plotVidStartText->setClipToAxisRect(false);
    plotVidStartText->setText("Video Start");
    plotVidStartText->setSelectable(false);

    // Text for video end point
    plotVidEndText = new QCPItemText(ui->customPlot);
    plotVidEndText->setPositionAlignment(Qt::AlignBottom|Qt::AlignHCenter);
    plotVidEndText->position->setTypeX(QCPItemPosition::ptPlotCoords);
    plotVidEndText->position->setTypeY(QCPItemPosition::ptAxisRectRatio);
    plotVidEndText->position->setParentAnchorX(plotVidEnd->point1);
    plotVidEndText->position->setCoords(0, 0);
    plotVidEndText->setClipToAxisRect(false);
    plotVidEndText->setText("Video End");
    plotVidEndText->setSelectable(false);


    ui->customPlot->setInteraction(QCP::iSelectItems, true);

    ui->customPlot->replot();
}

SyncView::~SyncView()
{
    delete ui;
}

void SyncView::on_vidWidget_positionChanged(int position){
    ui->durationLabel->setText(OpenCVVideoPlayer::formatTime(ui->vidWidget->getDurationMs()/1000.0));
    ui->timeLabel->setText(OpenCVVideoPlayer::formatTime(ui->vidWidget->getTimeMs()/1000.0));
    if(!ui->trackBar->isSliderDown())
        ui->trackBar->setValue(position);

    // If you play the video, the graph will follow along.
    if(ui->vidWidget->isPlaying()){
        ui->customPlot->xAxis->setRange(plotTrackBar->point1->coords().x(), ui->customPlot->xAxis->range().size(), Qt::AlignCenter);
    }
    syncChange();
}


void SyncView::on_trackBar_sliderMoved(int position)
{
    ui->vidWidget->seek(position);
}

void SyncView::on_trackBar_sliderPressed()
{
    ui->vidWidget->pause();
}

void SyncView::on_trackBar_sliderReleased()
{

}

void SyncView::on_playButton_clicked()
{
    if(ui->vidWidget->isPlaying()){
        ui->vidWidget->pause();
    }
    else{ //If player is paused or stop, resume or start over.
        if(ui->vidWidget->atEnd())
            ui->vidWidget->seek(0);
        ui->vidWidget->play();
    }
}

void SyncView::on_forwardButton_clicked()
{
    //Seek but keep playhead within bounds.
    ui->vidWidget->jog(10);
}

void SyncView::on_backButton_clicked()
{
    //Ditto as above.
    ui->vidWidget->jog(-10);
}

void SyncView::on_vidWidget_playStateChanged(bool state){
    if(state){
        ui->playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPause));
    }
    else {
        ui->playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    }
}

void SyncView::keyPressEvent(QKeyEvent *e){
    switch(e->key()){
    case Qt::Key_Left:
        ui->vidWidget->jog(-10);
        e->accept();
        break;

    case Qt::Key_Right:
        ui->vidWidget->jog(10);
        e->accept();
        break;

    case Qt::Key_Space:
        on_playButton_clicked();
        e->accept();
        break;
    }
}

void SyncView::on_customPlot_mouseMove(QMouseEvent *e){
    if(e->buttons() & Qt::LeftButton){
        if(ui->customPlot->selectedItems().size() != 0){
            double x,y;
            ui->customPlot->graph(0)->pixelsToCoords(e->x(), e->y(), x, y);
            if(plotTrackBar->selected()){
                deltaTVD = x - (rateMultiplier*(ui->vidWidget->getTimeMs()/1000.0));
                syncChange();
            }
            if(plotVidEnd->selected()){
                rateMultiplier = (x - deltaTVD)/(ui->vidWidget->getDurationMs()/1000.0);
                syncChange();
            }
        }
    }
}

void SyncView::xAxisChanged(QCPRange range){

    ui->xScrollBar->setValue(qRound(range.center()*100.0));
    ui->xScrollBar->setPageStep(qRound(range.size()*100.0));
    ui->zoomBar->setValue(qRound((dataLength/range.size()-1)/2));
}

void SyncView::horzScrollBarChanged(int value){
    // if user is dragging plot, we don't want to replot twice
    if(qAbs(ui->customPlot->xAxis->range().center()-(value/100.0)) > 0.005){
        ui->customPlot->xAxis->setRange(value/100.0, ui->customPlot->xAxis->range().size(), Qt::AlignCenter);
        ui->customPlot->replot();
    }
}

void SyncView::on_zoomBar_sliderMoved(int position)
{
    ui->customPlot->xAxis->setRange(ui->customPlot->xAxis->range().center(), dataLength/((position*2)+1), Qt::AlignCenter);
    ui->customPlot->replot();
}

void SyncView::on_lineEdit_editingFinished()
{
    qDebug() << "Editing Finished";
}

void SyncView::on_button_syncReset_clicked()
{
    deltaTVD = 0;
    syncChange();
}

void SyncView::on_button_rateReset_clicked()
{
    rateMultiplier = 1.0;
    syncChange();
}

void SyncView::syncChange(){
    qreal plotTrackBarNewPosition = (rateMultiplier*(ui->vidWidget->getTimeMs()/1000.0)) + deltaTVD;
    plotTrackBar->point1->setCoords(plotTrackBarNewPosition, plotTrackBar->point1->coords().y());
    plotTrackBar->point2->setCoords(plotTrackBarNewPosition, plotTrackBar->point2->coords().y());

    plotVidStart->point1->setCoords(deltaTVD, 0);
    plotVidStart->point2->setCoords(deltaTVD, 1);
    plotVidEnd->point1->setCoords(deltaTVD + (rateMultiplier*(ui->vidWidget->getDurationMs()/1000.0)), 0);
    plotVidEnd->point2->setCoords(deltaTVD + (rateMultiplier*(ui->vidWidget->getDurationMs()/1000.0)), 1);

    ui->label_syncInfo->setText(OpenCVVideoPlayer::formatTime(deltaTVD));
    ui->label_rateInfo->setText(QString::number(rateMultiplier, 'g', 3));
    ui->customPlot->replot();
    emit syncChanged(deltaTVD, rateMultiplier);
}

void SyncView::on_customPlot_selectionChangedByUser(){
    ui->customPlot->setInteraction(QCP::iRangeDrag, (ui->customPlot->selectedItems().size() == 0));
}

void SyncView::attachCap(VideoCapture *cap){
    ui->vidWidget->attachCap(cap);
}

void SyncView::attachTimeSeries(TimeSeries *ts){
    this->data = ts;
}

void SyncView::syncCap(){
    ui->vidWidget->syncCap();
}

void SyncView::updateSync(double start, double rate){
    deltaTVD = start;
    rateMultiplier = rate;
    syncChange();
}
