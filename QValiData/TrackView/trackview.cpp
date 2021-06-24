#include "trackview.h"
#include "ui_trackview.h"
#include <QFile>
#include <QDir>
#include <QDebug>
#include <QTime>
#include <QMessageBox>

using namespace cv;
TrackView::TrackView(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::TrackView)
{
    ui->setupUi(this);
    namesToPath = new QMap<QListWidgetItem *, MotionPath *>();
    paths = new QList<MotionPath *>();
    hasInit = false;

    connect(ui->vidWidget, SIGNAL(positionChanged(int)), this, SLOT(on_player_positionChanged(int)));
    connect(ui->vidWidget, SIGNAL(playStateChanged(bool)), this, SLOT(on_playState_changed(bool)));
    connect(ui->vidWidget, SIGNAL(pathsUpdated()), this, SLOT(updatePathList()));
    connect(ui->vidWidget, SIGNAL(pathSelected(MotionPath *)), this, SLOT(selectPath(MotionPath *)));

    connect(ui->xScrollBar, SIGNAL(valueChanged(int)), this, SLOT(horzScrollBarChanged(int)));
    connect(ui->customPlot->xAxis, SIGNAL(rangeChanged(QCPRange)), this, SLOT(xAxisChanged(QCPRange)));

    connect(ui->customPlot, SIGNAL(mouseDoubleClick(QMouseEvent *)), this, SLOT(on_customPlot_doubleClick(QMouseEvent *)));

    // Standard icons packaged with Qt (doc.qt.io/qt-5/qstyle.html)
    ui->playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    ui->forwardButton->setIcon(style()->standardIcon(QStyle::SP_MediaSkipForward));
    ui->backButton->setIcon(style()->standardIcon(QStyle::SP_MediaSkipBackward));

    data = new TimeSeries();
    deltaTVD = 0;
    rateMultiplier = 1.0;

    addingManualAnnotation = false;
    manualAdd1 = -1;
    manualAdd2 = -1;
}

void TrackView::init(){

    ui->trackBar->setTickInterval(1);
    ui->trackBar->setRange(0, ui->vidWidget->getTotalFrames() - 1);


    qreal firstTime = data->timeColumnData()->first();
    qreal lastTime = data->timeColumnData()->last();

    dataLength = lastTime - firstTime;

    ui->xScrollBar->setRange(qFloor(firstTime*100.0), qCeil(lastTime*100.0));

    ui->customPlot->setInteraction(QCP::iRangeZoom, true);
    ui->customPlot->axisRect(0)->setRangeZoom(Qt::Horizontal);

    ui->customPlot->clearGraphs();
    ui->customPlot->clearItems();
    int numGraphs = QCPPlotTimeSeries::plotData(ui->customPlot, data);

    ui->customPlot->addGraph();
    ui->customPlot->graph(numGraphs)->setName("Video Motion");
    ui->customPlot->graph(numGraphs)->setBrush(QBrush(QColor(0, 184, 218, 255)));

    ui->customPlot->legend->setVisible(true);

    ui->customPlot->xAxis->setRange(firstTime, firstTime + (ui->vidWidget->getDurationMs()/1000.0));
    ui->customPlot->yAxis->rescale();

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
    plotVidEnd->setPen(QPen(QBrush(QColor(64, 64, 64)), 1, Qt::DashLine));
    plotVidEnd->setSelectable(false);

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

    plotTrackBar = new QCPItemStraightLine(ui->customPlot);
    plotTrackBar->point1->setCoords(0, 0);
    plotTrackBar->point2->setCoords(0, 1);
    plotTrackBar->setPen(QPen(QColor(255, 0, 0), 3));

    ui->customPlot->replot();

    hasInit = true;
    ui->vidWidget->init();
}

TrackView::~TrackView()
{
    delete ui;
}

void TrackView::on_player_positionChanged(int position){
    ui->durationLabel->setText(OpenCVVideoPlayer::formatTime(ui->vidWidget->getDurationMs()/1000.0));
    ui->timeLabel->setText(OpenCVVideoPlayer::formatTime(ui->vidWidget->getTimeMs()/1000.0));
    if(!ui->trackBar->isSliderDown())
        ui->trackBar->setValue(position);

    qreal plotTrackBarNewPosition = (rateMultiplier*(ui->vidWidget->getTimeMs()/1000.0)) + deltaTVD;
    plotTrackBar->point1->setCoords(plotTrackBarNewPosition, plotTrackBar->point1->coords().y());
    plotTrackBar->point2->setCoords(plotTrackBarNewPosition, plotTrackBar->point2->coords().y());

    // If you play the video, the graph will follow along.
    if(ui->vidWidget->isPlaying()){
        ui->customPlot->xAxis->setRange(plotTrackBar->point1->coords().x(), ui->customPlot->xAxis->range().size(), Qt::AlignCenter);
    }

    ui->customPlot->replot();
}


void TrackView::on_trackBar_sliderMoved(int position)
{
    ui->vidWidget->seek(position);
}

void TrackView::on_trackBar_sliderPressed()
{
    ui->vidWidget->pause();
}

void TrackView::on_trackBar_sliderReleased()
{
}

void TrackView::attachTimeSeries(TimeSeries *ts){
    this->data = ts;
}

void TrackView::on_playButton_clicked()
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

void TrackView::on_forwardButton_clicked()
{
    //Seek but keep playhead within bounds.
    ui->vidWidget->jog(10);
}

void TrackView::on_backButton_clicked()
{
    //Ditto as above.
    ui->vidWidget->jog(-10);
}

void TrackView::on_playState_changed(bool state){
    if(state){
        ui->playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPause));
    }
    else {
        ui->playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    }
}

void TrackView::xAxisChanged(QCPRange range){

    ui->xScrollBar->setValue(qRound(range.center()*100.0));
    ui->xScrollBar->setPageStep(qRound(range.size()*100.0));
    ui->zoomBar->setValue(qRound((dataLength/range.size()-1)/2));
}

void TrackView::horzScrollBarChanged(int value){
    // if user is dragging plot, we don't want to replot twice
    if(qAbs(ui->customPlot->xAxis->range().center()-(value/100.0)) > 0.005){
        ui->customPlot->xAxis->setRange(value/100.0, ui->customPlot->xAxis->range().size(), Qt::AlignCenter);
        ui->customPlot->replot();
    }
}

void TrackView::on_zoomBar_sliderMoved(int position)
{
    ui->customPlot->xAxis->setRange(ui->customPlot->xAxis->range().center(), dataLength/((position*2)+1), Qt::AlignCenter);
    ui->customPlot->replot();
}


void TrackView::keyPressEvent(QKeyEvent *e){
    qDebug() << "Key Event";
    switch(e->key()){
    case Qt::Key_Left:
        ui->vidWidget->reverseFrame();
        ui->vidWidget->frameUpdate();
        e->accept();
        break;

    case Qt::Key_Right:
        ui->vidWidget->advanceFrame();
        ui->vidWidget->frameUpdate();
        e->accept();
        break;

    case Qt::Key_Space:
        on_playButton_clicked();
        e->accept();
        break;
    }
}

void TrackView::updatePathList(){
    qDebug() << "Path names updated";
    namesToPath->clear();
    ui->clipList->clear();
    for(MotionPath *p : *paths){
        namesToPath->insert(new QListWidgetItem(QString("%1 -> %2 (%3 frames)")
                                           .arg(OpenCVVideoPlayer::formatTime(qreal(p->start)*ui->vidWidget->getFrameInterval()/1000.0))
                                           .arg(OpenCVVideoPlayer::formatTime(qreal(p->end)*ui->vidWidget->getFrameInterval()/1000.0))
                                           .arg(p->end - p->start + 1)), p);
    }

    for(QListWidgetItem *name : namesToPath->keys()){
        ui->clipList->addItem(name);
    }
    if (hasInit)
        plotMotionTracks();

    if (paths->size() > 0){
        ui->removeButton->setEnabled(true);
        ui->trimButton->setEnabled(true);
    }
    else{
        ui->removeButton->setEnabled(false);
        ui->trimButton->setEnabled(false);
    }
}

void TrackView::selectPath(MotionPath * path){
    if(path != nullptr){
        ui->clipList->setCurrentItem(namesToPath->key(path));
    }
}

void TrackView::on_clipList_itemClicked(QListWidgetItem *item){
    ui->vidWidget->seekToPath(namesToPath->value(item));
    ui->customPlot->xAxis->setRange(plotTrackBar->point1->coords().x(), ui->customPlot->xAxis->range().size(), Qt::AlignCenter);
    ui->customPlot->replot();
}

void TrackView::attachCap(VideoCapture *cap){
    ui->vidWidget->attachCap(cap);
}

void TrackView::syncCap(){
    ui->vidWidget->syncCap();
}

void TrackView::attachPath(QList<MotionPath *> *paths){
    this->paths = paths;
    ui->vidWidget->attachtoPathList(paths);
}

/**
 * @brief TrackView::syncPath Refresh path display with our own path list
 */
void TrackView::syncPath(){
    updatePathList();
}

void TrackView::on_trimButton_clicked()
{
    int maxLengthToDelete = ui->trimFrames->value();
    QMessageBox::StandardButton result = QMessageBox::question(this, "Confirm", QString("Delete all tracks with %1 or fewer frames? This cannot be undone.").arg(maxLengthToDelete));
    if(result == QMessageBox::Yes){
        if(paths != nullptr){
            int i = 0;
            while(i < paths->size()){
                MotionPath *currentPath = paths->at(i);
                if((currentPath->end - currentPath->start) <= maxLengthToDelete){
                    paths->removeAt(i);
                }
                else{
                    ++i;
                }
            }
        }
    }
    updatePathList();
}

void TrackView::plotMotionTracks(){
    clearGraph();
    for(MotionPath *m: *paths){
        QCPItemRect *rect = new QCPItemRect(ui->customPlot);

        qreal plotMidPoint = ui->customPlot->yAxis->coordToPixel(0);
        qreal rectTop = ui->customPlot->yAxis->pixelToCoord(plotMidPoint-10);
        qreal rectBottom = ui->customPlot->yAxis->pixelToCoord(plotMidPoint+10);

        rect->topLeft->setCoords((rateMultiplier*(m->start*ui->vidWidget->getFrameInterval()/1000.0)) + deltaTVD, rectTop);
        rect->bottomRight->setCoords((rateMultiplier*(m->end*ui->vidWidget->getFrameInterval()/1000.0)) + deltaTVD, rectBottom);
        rect->setBrush(QColor(0, 184, 218, 255));
    }
    ui->customPlot->replot();
}

void TrackView::clearGraph(){
    qreal trackBarPosition = plotTrackBar->point1->coords().x();
    ui->customPlot->clearItems();
    plotTrackBar = new QCPItemStraightLine(ui->customPlot);
    plotTrackBar->point1->setCoords(trackBarPosition, 0);
    plotTrackBar->point2->setCoords(trackBarPosition, 1);
    plotTrackBar->setPen(QPen(QColor(255, 0, 0), 3));

    // Video start point
    plotVidStart = new QCPItemStraightLine(ui->customPlot);
    plotVidStart->setPen(QPen(QBrush(QColor(64, 64, 64)), 1, Qt::DashLine));
    plotVidStart->setSelectable(false);

    // Video end point
    plotVidEnd = new QCPItemStraightLine(ui->customPlot);
    plotVidEnd->setPen(QPen(QBrush(QColor(64, 64, 64)), 1, Qt::DashLine));
    plotVidEnd->setSelectable(false);

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

    plotVidStart->point1->setCoords(deltaTVD, 0);
    plotVidStart->point2->setCoords(deltaTVD, 1);
    plotVidEnd->point1->setCoords(deltaTVD + (rateMultiplier*(ui->vidWidget->getDurationMs()/1000.0)), 0);
    plotVidEnd->point2->setCoords(deltaTVD + (rateMultiplier*(ui->vidWidget->getDurationMs()/1000.0)), 1);
}

void TrackView::updateSync(double startTime, double rate){
    this->deltaTVD = startTime;
    this->rateMultiplier = rate;
    plotVidStart->point1->setCoords(deltaTVD, 0);
    plotVidStart->point2->setCoords(deltaTVD, 1);
    plotVidEnd->point1->setCoords(deltaTVD + (rateMultiplier*(ui->vidWidget->getDurationMs()/1000.0)), 0);
    plotVidEnd->point2->setCoords(deltaTVD + (rateMultiplier*(ui->vidWidget->getDurationMs()/1000.0)), 1);

}

void TrackView::on_customPlot_mousePress(QMouseEvent *e){
    double x,y;
    ui->customPlot->graph(0)->pixelsToCoords(e->x(), e->y(), x, y);
    ui->vidWidget->seek_ms(((x-deltaTVD)/rateMultiplier)*1000);
}

void TrackView::on_customPlot_mouseMove(QMouseEvent *e){
    if(e->buttons() & Qt::LeftButton){
        on_customPlot_mousePress(e);
    }
}

// Removes motion path at selected location
void TrackView::on_removeButton_clicked()
{
    if(paths != nullptr){
        if(ui->clipList->selectedItems().size() > 0){
            QMessageBox::StandardButton result = QMessageBox::question(this, "Confirm", QString("Delete selected track? This cannot be undone."));
            if(result == QMessageBox::Yes){
                paths->removeAt(paths->indexOf(namesToPath->value(ui->clipList->selectedItems().at(0))));
            }
        }
    }
    updatePathList();
}

void TrackView::on_button_addAnnot_clicked()
{

}

void TrackView::on_customPlot_doubleClick(QMouseEvent *e){
    if(ui->button_addAnnot->isChecked()){
        double x,y;
        ui->customPlot->graph(0)->pixelsToCoords(e->x(), e->y(), x, y);
        double timeClicked_formatted = ((x-deltaTVD)/rateMultiplier);
        int frameClicked = ((ui->vidWidget->getFrameInterval() >= 0))?int(timeClicked_formatted/(ui->vidWidget->getFrameInterval()/1000)):0;
        int maxFrame = ui->vidWidget->getTotalFrames();
        qDebug() << frameClicked;
        if(frameClicked >= 0 && frameClicked < maxFrame){
            qDebug() << "Add annot: " << frameClicked;
            if(manualAdd1 == -1){
                manualAdd1 = frameClicked;
                ui->button_addAnnot->setText("Double-Click for Point 2, Click to Cancel");
            }
            else
                manualAdd2 = frameClicked;

            if (manualAdd1 != -1 && manualAdd2 != -1){
                paths->append(new MotionPath(qMin(manualAdd1, manualAdd2), qMax(manualAdd1, manualAdd2)));
                ui->button_addAnnot->setText("Double-Click for Point 1, Click to Cancel");
                manualAdd1 = -1;
                manualAdd2 = -1;
                plotMotionTracks();
            }
        }
    }
}

void TrackView::on_button_addAnnot_toggled(bool checked)
{
    if(checked){
        addingManualAnnotation = true;
        ui->button_addAnnot->setText("Double-Click for Point 1, Click to Cancel");
        ui->button_addAnnot->update();
        manualAdd1 = -1;
        manualAdd2 = -1;
    }else{
        ui->button_addAnnot->setText("Manually Add Annotation...");
    }
}

void TrackView::on_checkBox_showPaths_toggled(bool checked)
{
    if(hasInit){
        ui->vidWidget->setShowPaths(checked);
        ui->vidWidget->frameUpdate();
    }
}
