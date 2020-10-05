#include "adxlsimview.h"
#include "ui_adxlsimview.h"

using namespace cv;

ADXLSimView::ADXLSimView(QWidget *parent) :
    SimulatorTab(parent),
    ui(new Ui::ADXLSimView)
{
    ui->setupUi(this);

    data = new TimeSeries();
    namesToPath = new QMap<QListWidgetItem *, MotionPath *>();
    paths = new QList<MotionPath *>();
    deltaTVD = 0;
    rateMultiplier = 1.0;
    hasInit = false;

    connect(ui->xScrollBar, SIGNAL(valueChanged(int)), this, SLOT(horzScrollBarChanged(int)));
    connect(ui->customPlot->xAxis, SIGNAL(rangeChanged(QCPRange)), this, SLOT(xAxisChanged(QCPRange)));
    connect(ui->customPlot, SIGNAL(mouseDoubleClick(QMouseEvent *)), this, SLOT(on_customPlot_doubleClick(QMouseEvent *)));

    connect(ui->spinbox_acttime, SIGNAL(valueChanged(QString)), this, SLOT(ADXL_spinbox_valueChanged(QString)));
    connect(ui->spinbox_actthresh, SIGNAL(valueChanged(QString)), this, SLOT(ADXL_spinbox_valueChanged(QString)));
    connect(ui->spinbox_inacttime, SIGNAL(valueChanged(QString)), this, SLOT(ADXL_spinbox_valueChanged(QString)));
    connect(ui->spinbox_inactthresh, SIGNAL(valueChanged(QString)), this, SLOT(ADXL_spinbox_valueChanged(QString)));
    connect(ui->spinbox_downsample, SIGNAL(valueChanged(QString)), this, SLOT(ADXL_spinbox_valueChanged(QString)));

    ui->playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    ui->forwardButton->setIcon(style()->standardIcon(QStyle::SP_MediaSkipForward));
    ui->backButton->setIcon(style()->standardIcon(QStyle::SP_MediaSkipBackward));

}
void ADXLSimView::init(){
    ui->trackBar->setTickInterval(1);
    ui->trackBar->setMinimum(0);
    ui->trackBar->setMaximum(ui->vidWidget->getTotalFrames() - 1);

    qreal firstTime = data->timeColumnData()->first();
    qreal lastTime = data->timeColumnData()->last();

    dataLength = lastTime - firstTime;

    ui->xScrollBar->setRange(qFloor(firstTime*100.0), qCeil(lastTime*100.0));
    simStart = firstTime;
    simEnd = lastTime;
    ui->label_simStart->setText(OpenCVVideoPlayer::formatTime(((simStart-deltaTVD)/rateMultiplier)));
    ui->label_simEnd->setText(OpenCVVideoPlayer::formatTime(((simEnd-deltaTVD)/rateMultiplier)));

    ui->customPlot->setInteraction(QCP::iRangeZoom, true);
    ui->customPlot->axisRect(0)->setRangeZoom(Qt::Horizontal);

    ui->customPlot->clearGraphs();
    ui->customPlot->clearItems();
    int numGraphs = QCPPlotTimeSeries::plotData(ui->customPlot, data);

    ui->customPlot->addGraph();
    ui->customPlot->graph(numGraphs)->setPen(QPen(QColor(127, 127, 127)));
    ui->customPlot->graph(numGraphs)->setName("Active (Sim)");
    ui->customPlot->graph(numGraphs)->setBrush(QBrush(QColor(127, 127, 127, 63)));

    ui->customPlot->legend->setVisible(true);

    ui->customPlot->addGraph();
    ui->customPlot->graph(numGraphs+1)->setName("Video Motion");
    ui->customPlot->graph(numGraphs+1)->setBrush(QBrush(QColor(0, 184, 218, 255)));

    plotTrackBar = new QCPItemStraightLine(ui->customPlot);
    plotTrackBar->point1->setCoords(deltaTVD, 0);
    plotTrackBar->point2->setCoords(deltaTVD, 1);
    plotTrackBar->setPen(QPen(QColor(255, 0, 0), 3));

    ui->customPlot->xAxis->setRange(firstTime, firstTime + (ui->vidWidget->getDurationMs()/1000.0));
    ui->customPlot->yAxis->rescale();

    clearGraph(); // Also generates new graphics

    ui->customPlot->replot();

    // Set sample rate label. Also catch zero-div.
    ui->label_samplerateactual->setText((dataLength>0)?QString::number(data->numRows()/(dataLength), 'f', 2):"0");

    pathCoverage = new QHash<MotionPath *, double>();
    hasInit = true;
}

ADXLSimView::~ADXLSimView()
{
    delete ui;
}

void ADXLSimView::clearGraph(){
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

    // Stat start point
    plotStatStart = new QCPItemStraightLine(ui->customPlot);
    plotStatStart->point1->setCoords(0, 0);
    plotStatStart->point2->setCoords(0, 1);
    plotStatStart->setPen(QPen(QBrush(QColor(64, 64, 64)), 1, Qt::DashLine));
    plotStatStart->setSelectable(false);

    // Stat end point
    plotStatEnd = new QCPItemStraightLine(ui->customPlot);
    plotStatEnd->point1->setCoords(ui->vidWidget->getDurationMs()/1000.0, 0);
    plotStatEnd->point2->setCoords(ui->vidWidget->getDurationMs()/1000.0, 1);
    plotStatEnd->setPen(QPen(QBrush(QColor(64, 64, 64)), 1, Qt::DashLine));
    plotStatEnd->setSelectable(false);

    // Text for statistics start point
    plotStatStartText = new QCPItemText(ui->customPlot);
    plotStatStartText->setPositionAlignment(Qt::AlignBottom|Qt::AlignHCenter);
    plotStatStartText->position->setTypeX(QCPItemPosition::ptPlotCoords);
    plotStatStartText->position->setTypeY(QCPItemPosition::ptAxisRectRatio);
    plotStatStartText->position->setParentAnchorX(plotStatStart->point1);
    plotStatStartText->position->setCoords(0, 0);
    plotStatStartText->setClipToAxisRect(false);
    plotStatStartText->setText("Stat Start");
    plotStatStartText->setSelectable(false);

    // Text for statistics end point
    plotStatEndText = new QCPItemText(ui->customPlot);
    plotStatEndText->setPositionAlignment(Qt::AlignBottom|Qt::AlignHCenter);
    plotStatEndText->position->setTypeX(QCPItemPosition::ptPlotCoords);
    plotStatEndText->position->setTypeY(QCPItemPosition::ptAxisRectRatio);
    plotStatEndText->position->setParentAnchorX(plotStatEnd->point1);
    plotStatEndText->position->setCoords(0, 0);
    plotStatEndText->setClipToAxisRect(false);
    plotStatEndText->setText("Stat End");
    plotStatEndText->setSelectable(false);

    plotStatStart->point1->setCoords(simStart, 0);
    plotStatStart->point2->setCoords(simStart, 1);
    plotStatEnd->point1->setCoords(simEnd, 0);
    plotStatEnd->point2->setCoords(simEnd, 1);

}

double ADXLSimView::frameToDataTime(int frame){
    return ((frame*ui->vidWidget->getFrameInterval()/1000.0)*rateMultiplier)+deltaTVD;
}

int ADXLSimView::dataTimeToFrame(double dataTime){
    return int((dataTime - deltaTVD)/rateMultiplier*1000.0/ui->vidWidget->getFrameInterval());
}

void ADXLSimView::on_buttonApply_clicked()
{

    plotMotionTracks();

    double threshAct = ui->spinbox_actthresh->value();
    double threshInact = ui->spinbox_inactthresh->value();
    int timeAct = ui->spinbox_acttime->value();
    int timeInact = ui->spinbox_inacttime->value();
    // Quick check to make sure it's not zero
    double samplerate = (dataLength > 0)?(data->numRows() / dataLength):0;

    adxl = new ADXLSim2(threshAct, threshInact, timeAct, timeInact);

    bool state_prev = false;
    int firstActive = 0;
    int lastActive = 0;
    int totalActiveRegions = 0;
    int totalActiveSamples = 0;
    int samplesPerChunk = 1;

    int currentIndex = 0;
    int totalSamples = data->numRows();
    int totalSamplesStat = 0; // Total number of samples within statistics window

    int downSample = ui->spinbox_downsample->value();
    int samplesPerChunkDownSampled = 0;

    int correctSamples = 0;
    int falsePositives = 0;
    int falseNegatives = 0;

    int correctEvents = 0;
    int falsePositiveEvents = 0;
    bool annotationThisWakeup = false;

    int frameStatStart = dataTimeToFrame(simStart);
    int frameStatEnd = dataTimeToFrame(simEnd);

    int wakeupDownSample = int(ceil(samplerate/6.0)); // Calculate downsample rate in wakeup mode when inactive (6 Hz)
    bool wakeupMode = false;

    int currentDownSample = downSample;

    activeRegionList.clear();

    if(ui->checkBox_adxlWakeup->isChecked()){
        timeAct = 1;
        wakeupMode = true;
    }

    bool activebit = false;

    pathCoverage->clear();
    if(ui->groupBox_eventCoverage->isChecked()){
        for(MotionPath *p: *paths){
            if(p->end > frameStatStart && p->start <= frameStatEnd)
                pathCoverage->insert(p, 0);
        }
    }

    while(currentIndex + samplesPerChunk < totalSamples){
        if(wakeupMode){
            if(!activebit){
                currentDownSample = wakeupDownSample;
            }
            else{
                currentDownSample = downSample;
            }
        }
        for(int chunkSamples = 0; chunkSamples < samplesPerChunk; ++chunkSamples){
            int sampleIndex = currentIndex + chunkSamples;
            // Only step the simulation every n samples. All other samples are interpolated, assuming the most recent states.
            if((sampleIndex % currentDownSample) == 0)
            {
                samplesPerChunkDownSampled ++;
                if (data->getColumn("X") == nullptr || data->getColumn("Y") == nullptr || data->getColumn("Z") == nullptr){
                    QMessageBox::warning(this, "", "ADXL Simulator requires data with columns \"X\", \"Y\", and \"Z\"");
                    return;
                }
                adxl->next(data->getColumn("X")->at(sampleIndex),
                           data->getColumn("Y")->at(sampleIndex),
                           data->getColumn("Z")->at(sampleIndex));
                if (data->timeColumnData()->at(sampleIndex) >= simStart && data->timeColumnData()->at(sampleIndex) <= simEnd){
                    totalSamplesStat ++;
                }
            }
        }

        activebit = adxl->isActive();

        bool activeAnnotation = false; // Active bit from annotation
        qreal currentTime = data->timeColumnData()->at(currentIndex);
        int currentVideoFrame = dataTimeToFrame(currentTime);

        if (currentTime >= simStart && currentTime <= simEnd){
            // Find out if there's an annotation at the present location
            if(ui->groupBox_eventCoverage->isChecked()){
                for(MotionPath *p: pathCoverage->keys()){

                    if(p->start <= currentVideoFrame && p->end > currentVideoFrame){
                        activeAnnotation = true;
                        if(activebit){
                            pathCoverage->insert(p, pathCoverage->value(p)+((1.0/samplerate)/(ui->vidWidget->getFrameInterval()/1000.0))); // Increment this path's count by the number of frames covered by one sample

                        }
                    }
                }
            }
            if(activebit){
                totalActiveSamples += samplesPerChunkDownSampled;
            }
            if(ui->groupBox_eventCoverage->isChecked()){
                if(activebit){
                    if(activeAnnotation){
                        correctSamples ++;
                        annotationThisWakeup = true;
                    }
                    else{
                        falsePositives ++;
                    }
                }
                else{
                    if(activeAnnotation){
                        falseNegatives ++;
                    }
                    else{
                        correctSamples ++;
                    }

                }
            }
        }
        samplesPerChunkDownSampled = 0;
        // Detect transitions from active -> inactive and vice versa

        if(state_prev != activebit){
            state_prev = activebit;

            // Transition from active to inactive
            if(!activebit){
                if (currentTime >= simStart && currentTime <= simEnd && ui->groupBox_eventCoverage->isChecked()){
                    double activeRegionLength = (samplerate > 0)?((currentIndex - firstActive)/samplerate):0;
                    if(!annotationThisWakeup){
                        falsePositiveEvents ++;
                        activeRegionList.append(QPair<double, bool>(activeRegionLength, false));
                    }else{
                        correctEvents ++;
                        activeRegionList.append(QPair<double, bool>(activeRegionLength, true));
                    }
                }
                lastActive = currentIndex;
            }

            // Transition from inactive to active
            else if(activebit){
                firstActive = currentIndex;
                annotationThisWakeup = false;
                if (currentTime >= simStart && currentTime <= simEnd){
                    ++totalActiveRegions;
                }

                QCPItemRect *rect = new QCPItemRect(ui->customPlot);
                //Set rectangles to go off screen
                rect->topLeft->setCoords(data->timeColumnData()->at(lastActive), ui->customPlot->yAxis->range().upper+1);
                rect->bottomRight->setCoords(currentTime, ui->customPlot->yAxis->range().lower-1);
                rect->setBrush(QBrush(QColor(127, 127, 127, 200)));
            }
        }
        currentIndex += samplesPerChunk;
    }

    // Do some additional calculations, and display the results

    double totalEnergy = (ui->spinbox_standbyenergy->value()*(totalSamplesStat-totalActiveSamples) +
                          ui->spinbox_wakeupenergy->value()*totalActiveRegions +
                          ui->spinbox_energypersample->value()*totalActiveSamples)/1000.0;

    double storagekB = ui->spinbox_bitspersample->value()*totalActiveSamples/1000.0;

    ui->label_powerconsumed->setText(QString::number(totalEnergy, 'f', 2));
    ui->label_totalstorage->setText(QString::number(storagekB, 'f', 2));

    ui->label_samples->setText(QString::number(totalSamplesStat));
    ui->label_activesamples->setText(QString::number(totalActiveSamples));
    ui->label_sampleratesim->setText(QString::number(samplerate/downSample, 'f', 2));
    ui->label_wakeups->setText(QString::number(totalActiveRegions));
    double percentActive = (currentIndex > 0)?double(totalActiveSamples)/totalSamplesStat*100:0;
    ui->label_activepercent->setText(QString::number(percentActive, 'f', 1)); //Display percentage rounded to 1 decimal

    ui->customPlot->replot();

    if(ui->groupBox_eventCoverage->isChecked()){
        int coveredEvents = 0;
        for(MotionPath *p: pathCoverage->keys()){
            if(pathCoverage->value(p) >= 0.5*(p->end-p->start)){
                coveredEvents++;
            }
        }
        ui->label_correctsamples->setText(QString::number(correctSamples));
        ui->label_falsepsamples->setText(QString::number(falsePositives));
        ui->label_falsensamples->setText(QString::number(falseNegatives));
        ui->label_correctevents->setText(QString::number(coveredEvents));
        ui->label_falsepevents->setText(QString::number(falsePositiveEvents));
        ui->label_falsenevents->setText(QString::number(pathCoverage->size()-coveredEvents));
        ui->label_annotationCount->setText(QString::number(pathCoverage->size()));
    }
}


void ADXLSimView::on_vidWidget_positionChanged(int position){
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


void ADXLSimView::on_trackBar_sliderMoved(int position)
{
    ui->vidWidget->seek(position);
}

void ADXLSimView::on_trackBar_sliderPressed()
{
    ui->vidWidget->pause();
}

void ADXLSimView::on_trackBar_sliderReleased()
{

}

void ADXLSimView::on_playButton_clicked()
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

void ADXLSimView::on_forwardButton_clicked()
{
    //Seek but keep playhead within bounds.
    ui->vidWidget->jog(10);
}

void ADXLSimView::on_backButton_clicked()
{
    //Ditto as above.
    ui->vidWidget->jog(-10);
}

void ADXLSimView::on_vidWidget_playStateChanged(bool state){
    if(state){
        ui->playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPause));
    }
    else {
        ui->playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    }
}

void ADXLSimView::keyPressEvent(QKeyEvent *e){
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

void ADXLSimView::xAxisChanged(QCPRange range){

    ui->xScrollBar->setValue(qRound(range.center()*100.0));
    ui->xScrollBar->setPageStep(qRound(range.size()*100.0));
    ui->zoomBar->setValue(qRound((dataLength/range.size()-1)/2));
}

void ADXLSimView::horzScrollBarChanged(int value){
    // if user is dragging plot, we don't want to replot twice
    if(qAbs(ui->customPlot->xAxis->range().center()-(value/100.0)) > 0.005){
        ui->customPlot->xAxis->setRange(value/100.0, ui->customPlot->xAxis->range().size(), Qt::AlignCenter);
        ui->customPlot->replot();
    }
}

void ADXLSimView::on_zoomBar_sliderMoved(int position)
{
    ui->customPlot->xAxis->setRange(ui->customPlot->xAxis->range().center(), dataLength/((position*2)+1), Qt::AlignCenter);
    ui->customPlot->replot();
}



void ADXLSimView::attachCap(VideoCapture *cap){
    ui->vidWidget->attachCap(cap);
}

void ADXLSimView::attachTimeSeries(TimeSeries *ts){
    this->data = ts;
}

void ADXLSimView::syncCap(){
    ui->vidWidget->syncCap();
}

void ADXLSimView::attachPath(QList<MotionPath *> *paths){
    this->paths = paths;
    ui->vidWidget->attachPath(paths);
}

void ADXLSimView::syncPath(){
    updatePathList(paths);
}

void ADXLSimView::on_clipList_itemClicked(QListWidgetItem *item){
    ui->vidWidget->seek(namesToPath->value(item)->start);
}

void ADXLSimView::updatePathList(QList<MotionPath *> *paths){
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
}

void ADXLSimView::updateSync(double startTime, double rate){
    this->deltaTVD = startTime;
    this->rateMultiplier = rate;
    plotVidStart->point1->setCoords(deltaTVD, 0);
    plotVidStart->point2->setCoords(deltaTVD, 1);
    plotVidEnd->point1->setCoords(deltaTVD + (rateMultiplier*(ui->vidWidget->getDurationMs()/1000.0)), 0);
    plotVidEnd->point2->setCoords(deltaTVD + (rateMultiplier*(ui->vidWidget->getDurationMs()/1000.0)), 1);

}

void ADXLSimView::on_customPlot_mousePress(QMouseEvent *e){
    double x,y;
    ui->customPlot->graph(0)->pixelsToCoords(e->x(), e->y(), x, y);
    ui->vidWidget->seek_ms(((x-deltaTVD)/rateMultiplier)*1000);
}

void ADXLSimView::on_customPlot_mouseMove(QMouseEvent *e){
    if(e->buttons() & Qt::LeftButton){
        on_customPlot_mousePress(e);
    }
}

void ADXLSimView::on_magnify_toggled(bool checked)
{
    ui->vidWidget->setMagnify(checked);
    ui->vidWidget->frameUpdate();
}

void ADXLSimView::plotMotionTracks(){
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

void ADXLSimView::on_customPlot_doubleClick(QMouseEvent *e){
    double x,y;
    ui->customPlot->graph(0)->pixelsToCoords(e->x(), e->y(), x, y);
    double timeClicked_formatted = ((x-deltaTVD)/rateMultiplier);
    switch(e->button()){
    case Qt::LeftButton:
        simStart = x;
        ui->label_simStart->setText(OpenCVVideoPlayer::formatTime(timeClicked_formatted));
        plotStatStart->point1->setCoords(x, 0);
        plotStatStart->point2->setCoords(x, 1);
        ui->customPlot->replot();
        if (ui->checkBox_autoUpdate->isChecked()){
            on_buttonApply_clicked();
        }
        emit statChanged(simStart, simEnd);
        break;
    case Qt::RightButton:
        simEnd = x;
        ui->label_simEnd->setText(OpenCVVideoPlayer::formatTime(timeClicked_formatted));
        plotStatEnd->point1->setCoords(x, 0);
        plotStatEnd->point2->setCoords(x, 1);
        ui->customPlot->replot();
        if (ui->checkBox_autoUpdate->isChecked()){
            on_buttonApply_clicked();
        }
        emit statChanged(simStart, simEnd);
        break;
    default:
        break;
    }


    // Compute annotation active percentage within statistics region
    int frameStart = 0;
    int frameEnd = 0;

    if(ui->vidWidget->getFrameInterval() >= 0){
        frameStart = dataTimeToFrame(simStart);
        frameEnd = dataTimeToFrame(simEnd);
    }

    int totalLength = 0;
    // Count motion paths, but don't double-count
    for(int i=frameStart; i<frameEnd; ++i){
        for (MotionPath *p: *paths){
            if(p->start <= i && p->end > i){
                totalLength ++;
                break;
            }
        }
    }

    qDebug() << totalLength;
    ui->label_videoActive->setText(QString::number(double(100*totalLength)/(frameEnd-frameStart), 'f', 2));
}


void ADXLSimView::ADXL_spinbox_valueChanged(QString arg1)
{
    Q_UNUSED(arg1)
    if (ui->checkBox_autoUpdate->isChecked()){
        on_buttonApply_clicked();
    }
}

void ADXLSimView::on_button_exportcoverage_clicked()
{
    if (pathCoverage->empty()){
        QMessageBox::warning(this, "", "No coverage data to export. Click \"Apply\" to generate data or adjust simulation bounds to include annotations.");
        return;
    }

    QString saveFileName = QFileDialog::getSaveFileName(this, "Export coverage data...", "", "CSV (*.csv)");

    if(!saveFileName.isEmpty()){
        QFile coverageFile(saveFileName);
        coverageFile.open(QFile::WriteOnly);\
        // Write header
        coverageFile.write("Start,Length,Coverage Percent\n");
        for(MotionPath *p: (pathCoverage->keys())){
            double pathStartTime = frameToDataTime(p->start);
            double pathEndTime = frameToDataTime(p->end);
            double pathLength = pathEndTime-pathStartTime;
            // Cap value to 100 if it exceeds it.
            double coveragePercent = qMin(100.0, pathLength>0?(100*pathCoverage->value(p)/(p->end-p->start)):0);

            coverageFile.write(QString("%1,%2,%3\n")
                               .arg(pathStartTime)
                               .arg(pathLength)
                               .arg(coveragePercent).toLatin1());
        }
        coverageFile.close();
    }
}

/*
 * Enabling wakeup mode disables the active timer
 */
void ADXLSimView::on_checkBox_adxlWakeup_stateChanged(int arg1)
{
    Q_UNUSED(arg1)
    ui->spinbox_acttime->setEnabled(!ui->checkBox_adxlWakeup->isChecked());
}

void ADXLSimView::updateStat(double start, double end){
    simStart = start;
    simEnd = end;

    ui->label_simStart->setText(OpenCVVideoPlayer::formatTime(((simStart-deltaTVD)/rateMultiplier)));
    plotStatStart->point1->setCoords(simStart, 0);
    plotStatStart->point2->setCoords(simStart, 1);

    ui->label_simEnd->setText(OpenCVVideoPlayer::formatTime(((simEnd-deltaTVD)/rateMultiplier)));
    plotStatEnd->point1->setCoords(simEnd, 0);
    plotStatEnd->point2->setCoords(simEnd, 1);

    ui->customPlot->replot();
}

void ADXLSimView::on_button_exportactive_clicked()
{
    if (activeRegionList.isEmpty()){
        QMessageBox::warning(this, "", "No data to export. Click \"Apply\" to generate data or adjust simulation bounds to include annotations.");
        return;
    }

    QString saveFileName = QFileDialog::getSaveFileName(this, "Export coverage data...", "", "CSV (*.csv)");

    if(!saveFileName.isEmpty()){
        QFile eventFile(saveFileName);
        eventFile.open(QFile::WriteOnly);\
        // Write header
        eventFile.write("Length,Correct\n");
        for(QPair<double, bool> p:activeRegionList){
            eventFile.write(QString("%1,%2\n").arg(p.first).arg((p.second)?"TRUE":"FALSE").toLatin1());
        }
        eventFile.close();
    }
}
