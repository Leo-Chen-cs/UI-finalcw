#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "seekFrame.h"
#include "utils.h"
#include <QDebug>
#include <QMediaPlayer>
#include <QTime>
#include <QMediaPlaylist>
#include <QFileDialog>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QShortcut>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setMouseTracking(true);
    this->setWindowFlags(Qt::FramelessWindowHint);

    initSystem();
    ui->normal_widget->setStyleSheet("background-color:black;");
    mediaPlayer = new QMediaPlayer;
    mediaPlayer->setVideoOutput(ui->normal_widget);

    connect(mediaPlayer, &QMediaPlayer::durationChanged, this, &MainWindow::onDurationChanged);

    playList = new QMediaPlaylist();
    mediaPlayer->setPlaylist(playList);
    initWdigets();
    connect2Player();
}


MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::showOutControlWidget()
{

    mAnimation_ControlWidget->setDuration(800);

    int w = ui->widget_controller->width();
    int h = ui->widget_controller->height();
    int x = 0;
    int y = ui->centralwidget->height() - ui->widget_controller->height();

    if (ui->widget_controller->isHidden())
    {
        ui->widget_controller->show();
        mAnimation_ControlWidget->setStartValue(ui->widget_controller->geometry());
    }
    else
    {
        mAnimation_ControlWidget->setStartValue(ui->widget_controller->geometry());
    }

    mAnimation_ControlWidget->setEndValue(QRect(x, y, w, h));
    mAnimation_ControlWidget->setEasingCurve(QEasingCurve::Linear); //设置动画效果

    mAnimation_ControlWidget->start();

}

void MainWindow::hideControlWidget()
{
    mAnimation_ControlWidget->setTargetObject(ui->widget_controller);

    mAnimation_ControlWidget->setDuration(300);

    int w = ui->widget_controller->width();
    int h = ui->widget_controller->height();
    int x = 0;
    int y = ui->centralwidget->height() + h;

    mAnimation_ControlWidget->setStartValue(ui->widget_controller->geometry());
    mAnimation_ControlWidget->setEndValue(QRect(x, y, w, h));
    mAnimation_ControlWidget->setEasingCurve(QEasingCurve::Linear); //设置动画效果

    mAnimation_ControlWidget->start();
}

void MainWindow::playClicked(){
    if(!loadedVideo){
        qDebug()<<"尚未加载视频";
        return;
    }
    // 处理点击了播放-暂停按钮的处理
    qDebug()<<"'current satate:"<<m_playerState;
    switch (m_playerState) {
    case QMediaPlayer::StoppedState:
    case QMediaPlayer::PausedState:
        play();
        break;
    case QMediaPlayer::PlayingState:
        pause();
        break;
    }
    qDebug()<<"'after satate:"<<m_playerState;
}


void MainWindow::connect2Player(){
    //关闭, 最大化, 最小化
    connect(ui->close,&QPushButton::clicked,this,[=](){ this->close();});
    connect(ui->minimize,&QPushButton::clicked,this,[=](){ this->setWindowState(Qt::WindowMinimized);});
    connect(ui->maximize,&QPushButton::clicked,this,[=](){ this->setWindowState(Qt::WindowMaximized);});
    // ui:设置打开和关闭侧边栏
    connect(ui->openFileList,&QPushButton::clicked,this,[=](){

        bool playListVisable = ui->sideBlock->isVisible();
        if(playListVisable){
            ui->sideBlock->setVisible(false);
            this->resize(this->width()-ui->sideBlock->width(),this->height());
            ui->openFileList->setStyleSheet(
                        "image: url(:new/image/off1.png);"
                        "border-radius:0px; "
            );
        }
        else{
            ui->sideBlock->setVisible(true);
            this->resize(this->width()+ui->sideBlock->width(),this->height());
            ui->openFileList->setStyleSheet(
                        "image: url(:new/image/on1.png);"
                        "border-radius:0px; "
            );
        }
    });

    connect(ui->play_button,SIGNAL(clicked()),this,SLOT(playClicked()));
    connect(ui->pause_botton,SIGNAL(clicked()),this,SLOT(playClicked()));
    connect(mediaPlayer,SIGNAL(positionChanged(qint64)),this,SLOT(positionChange(qint64)));
    connect(ui->video_slider,SIGNAL(sig_valueChanged_v(qint64)),this,SLOT(seek(qint64)));
    connect(ui->volume_slider,SIGNAL(valueChanged(int)),SLOT(changeVolume(int)));
    connect(ui->volume_button,SIGNAL(clicked()),this,SLOT(changeMute()));
    connect(ui->repeat,SIGNAL(clicked()),this,SLOT(changePlayOrder()));
    connect(ui->addFile,SIGNAL(clicked()),this,SLOT(openFileButtonClicked()));


    connect(ui->playList,&QListWidget::itemDoubleClicked, this, [=](){

        qDebug()<<"双击啦!";
        /*获取当前双击的item*/
        QListWidgetItem * item = ui->playList->item(ui->playList->currentIndex().row());

        mediaPlayer->pause();
//        initVideo(item->data(Qt::UserRole).toString(),true,false);
        initializeVideo(item->data(Qt::UserRole).toString());
        normalPlay();
    });
    connect(ui->playList, &QListWidget::itemClicked, this, [=](){
        qDebug()<<"当前点击的index"<<ui->playList->currentIndex().row();
        QListWidgetItem * item = ui->playList->item(ui->playList->currentIndex().row());
        qDebug()<<"当前点击的index的text"<<item->data(Qt::UserRole).toString();

    });
    connect(ui->forward_seconds,&QPushButton::clicked,this,[=](){ skipForwardOrBackward(true);});
    connect(ui->backward_seconds,&QPushButton::clicked,this,[=](){ skipForwardOrBackward(false);});
    connect(ui->next_video,&QPushButton::clicked,this,[=](){ changeVideo(true); });
    connect(ui->previous_video,&QPushButton::clicked,this,[=](){ changeVideo(false); });


    //快捷键
    QShortcut *shortCutForPasueAndPlay = new QShortcut(Qt::Key_Space, this );
    QShortcut *shortCutForSkipForward = new QShortcut(Qt::Key_Right, this );
    QShortcut *shortCutForSkipBackward = new QShortcut(Qt::Key_Left, this );
    QShortcut *shortCutForMute = new QShortcut(Qt::CTRL+Qt::Key_M, this );
    connect(shortCutForMute,&QShortcut::activated,this,[=](){ changeMute();});
    connect(shortCutForSkipForward,&QShortcut::activated,this,[=](){ skipForwardOrBackward(true);});
    connect(shortCutForSkipBackward,&QShortcut::activated,this,[=](){ skipForwardOrBackward(false);});
    connect(shortCutForPasueAndPlay,&QShortcut::activated,this,[=](){playClicked();});

}
void MainWindow::positionChange(qint64 progress) // * progress 是当前的毫秒数
{
    // qDebug()<<"MainWindow::positionChange:  progress"<<progress;
    // qDebug()<<"MainWindow::positionChange:  total duration"<<mediaPlayer->duration();
    // 计算当前进度
    // 计算当前的秒数
    // int curr = int(double(progress)/double(currVideoSeekFrame->formatContext->duration/1000)*100) ;
    // // qDebug()<<"curr"<<curr;
    if (!ui->video_slider->isSliderDown())
        qDebug()<<"max:"<<ui->video_slider->maximum();
        ui->video_slider->setValue(progress);

    currentPosition = progress;
    updateDurationInfo(progress/1000);  //progress/1000 = 秒数
}


void MainWindow::updateDurationInfo(qint64 currentInfo){
    /*用于更新当显示的秒数*/
    qDebug()<<"currentInfo"<<currentInfo;
    qDebug()<<"second_duration"<<durationS;

    QString tStr;
    if (currentInfo || durationS) {
        QTime currentTime((currentInfo / 3600) % 60, (currentInfo / 60) % 60,
            currentInfo % 60, (currentInfo * 1000) % 1000);
        QTime totalTime((durationS / 3600) % 60, (durationS/ 60) % 60,
           durationS % 60, (durationS * 1000) % 1000);
        QString format = "mm:ss";
        if (durationS > 3600)
            format = "hh:mm:ss";
        tStr = currentTime.toString(format) + " / " + totalTime.toString(format);
    }
    qDebug()<<"m_labelDuration"<<tStr;
    ui->current_time->setText(tStr);
    // 顺序播放/循环播放
    if(currentInfo==(qint64)durationS){
        if(isRepeat){
            initializeVideo(currentVideoPath);
            normalPlay();
        }
        else{
            changeVideo(true); // 下一个视频
        }
    }
}

void MainWindow::initVideoInfo(QString fileName){
    // 记录当前播放的路径
    currentVideoPath = fileName;
    durationMs = (qint64)(currVideoSeekFrame->formatContext->duration/1000);
    durationS = (qint64)(currVideoSeekFrame->formatContext->duration/1000000);
    qDebug() << "####Total Duration:" << duration  <<",durationS:"<<durationS<<",durationMs:"<<durationMs; // Duration is in milliseconds
}

void MainWindow::onDurationChanged(qint64 duration) {
    // qDebug() << "####Total Duration:" << duration  << "milliseconds"; // Duration is in milliseconds
    // durationMs = (qint64)(duration);
    // durationS = (qint64)(duration/1000);

    // ui->video_slider->setMaximum((int)durationMs);
    // qDebug() << "####Total Duration:" << duration  <<",durationS:"<<durationS<<",durationMs:"<<durationMs; // Duration is in milliseconds
}


void MainWindow::seek(qint64 time){
    if(loadedVideo){
        /*正放的时候跳转到某个位置*/
        mediaPlayer->setPosition(time);
    }

}
void MainWindow::changeVolume(int volume){
    mediaPlayer->setVolume(volume);
}

void MainWindow::changeMute(){
    mediaPlayer->setMuted(m_playerMuted);
    m_playerMuted = !m_playerMuted;
}

void MainWindow::jump(int second){
    if(!loadedVideo){
        qDebug()<<"尚未加载视频";
        return;
    }
    mediaPlayer->setPosition(currentPosition+second*1000);
}

void MainWindow::addVideoItem(QString fileName){
    int mediaTypeTemp = mediaType(fileName);

    QString shortFilename = getFileName(fileName);
    qDebug()<<"加入!";

    QWidget *widget_3 = new QWidget(ui->playList);
    widget_3->setObjectName(QString::fromUtf8("widget_3"));
    widget_3->setMaximumSize(QSize(16777215, 1651461));
    QHBoxLayout *horizontalLayout_4 = new QHBoxLayout(widget_3);
    horizontalLayout_4->setObjectName(QString::fromUtf8("horizontalLayout_4"));
    QLabel *label = new QLabel(widget_3);
    label->setText(shortFilename);
    label->setStyleSheet("color: rgb(246,246,246);"
                         "font-family:'Microsoft YaHei';"
                         "font:10pt;");

    label->setObjectName(QString::fromUtf8("label"));


    QPushButton *pushButton_3 = new QPushButton(widget_3);
    pushButton_3->setObjectName(QString::fromUtf8("pushButton_3"));
    pushButton_3->setMinimumSize(QSize(30, 30));
    pushButton_3->setMaximumSize(QSize(30, 30));
    pushButton_3->setStyleSheet(QString::fromUtf8("QPushButton{ \n"
            "image: url(:new/image/info_1.png);\n"
            "border-radius:0px; \n"
            "padding:0px;\n"
            "}  \n"
            "QPushButton:hover{ \n"
            "image: url(:new/image/info_2.png);\n"
            "border-radius:0px; \n"
            "padding:0px;\n"
            "} \n"
            "\n"
            "QPushButton:checked{ \n"
            "image: url(:new/image/info_1.png);\n"
            "} \n"
            "\n"
            ""));

    horizontalLayout_4->addWidget(pushButton_3);

    horizontalLayout_4->addWidget(label);

    QSpacerItem * horizontalSpacer_4 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    horizontalLayout_4->addItem(horizontalSpacer_4);

    QPushButton *pushButton_2 = new QPushButton(widget_3);
    pushButton_2->setObjectName(QString::fromUtf8("pushButton_2"));
    pushButton_2->setMinimumSize(QSize(30, 30));
    pushButton_2->setMaximumSize(QSize(30, 30));
    pushButton_2->setStyleSheet(QString::fromUtf8("QPushButton{ \n"
        "image: url(:new/image/delete.png);\n"
        "border-radius:0px; \n"
        "padding:0px;\n"
        "}  \n"
        "QPushButton:hover{ \n"
        "image: url(:new/image/delete_hover.png);\n"
        "border-radius:0px; \n"
        "padding:0px;\n"
        "} \n"
        "\n"
        "QPushButton:checked{ \n"
        "image: url(:new/image/delete.png);\n"
        "} \n"
        "\n"
        ""));

    horizontalLayout_4->addWidget(pushButton_2);

    QListWidgetItem *Qitem=new QListWidgetItem(ui->playList,0);
    Qitem->setSizeHint(QSize(120,40));
    ui->playList->setItemWidget(Qitem,widget_3);
    Qitem->setData(Qt::UserRole,fileName);
    // 在创建的时间就连接好这个删除键
    connect(pushButton_2,&QPushButton::clicked,this,[=](){
        ui->playList->removeItemWidget(Qitem);
        int removeIndex = -1;
        for (int i = 0;i<playListLocal->size();i++) {
            if((*playListLocal)[i]==Qitem->data(Qt::UserRole).toString()){
                removeIndex = i;
                qDebug()<<Qitem->data(Qt::UserRole).toString()<<currentVideoPath;
                if((Qitem->data(Qt::UserRole).toString()==currentVideoPath)&&m_playerState == QMediaPlayer::PlayingState){
                    mediaPlayer->stop();

                }
            }
        }
        if(removeIndex!=-1){
            playListLocal->removeAt(removeIndex);
            writePlayList(*playListLocal);
        }

        delete Qitem;
    });
    // 读取视频的信息
    QString info = getVideoInfo(fileName);
    // 连接信息键和信息弹窗
    connect(pushButton_3,&QPushButton::clicked,[=](){
        QMessageBox msgBox;
        msgBox.setStyleSheet(
                    "QMessageBox"
        "{"
           " border:none;"
            "background-color: rgb(57,60,62);"
            "border-radius: 15px;"
            "    color: white;"  // 设置白色字体
        "}"
                  "  QPushButton{ "
                "    image: url(:new/image/ok.png);"
               "     border-radius:0px; "
              "      padding:1px;"
              "      }  "
            "QLabel {"
            "    color: white;"  // 设置 QLabel 中的文本颜色
            "}"
        );


        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.button(QMessageBox::Ok)->setText("");
        msgBox.button(QMessageBox::Ok)->setMinimumSize(QSize(25, 25));
        msgBox.setIcon(QMessageBox::NoIcon);
        QIcon * windowIcon = new QIcon(":new/image/info.png");
        msgBox.setWindowIcon(*windowIcon);
        msgBox.setWindowTitle("属性");

        // 设置窗口标题，通常这里可以为空来去掉标题栏
        msgBox.setWindowTitle("");
        // 去掉标题栏
        msgBox.setWindowFlag(Qt::FramelessWindowHint);

        msgBox.setText(info);
        msgBox.exec();
    });
}
void MainWindow::openFileButtonClicked(){
    QString filename;
    QT_TRY{
        filename = QFileDialog::getOpenFileName();
        if (filename == ""){
            return;
        }
        if(!isValidVideoFile(filename)){
            QMessageBox::warning(this, tr("Error"),
                                           tr("打开的文件不是音视频格式")
                                           );
            return;
        }
        // 判断在不在当前的list中
        bool inList = false;
        int numVideos = ui->playList->count();
        for(int i = 0;i<numVideos;i++){
            QString i_address = ui->playList->item(i)->data(Qt::UserRole).toString();
            if(filename==i_address){
                QMessageBox::information(this, tr("info"),
                                               tr("打开的文件已在播放列表")
                                               );
                inList = true;
                break;
            }
        }



        mediaPlayer->pause();
        if(!inList){
            addVideoItem(filename);
            playListLocal->append(filename); // 把文件名写入到本地
        }

//        initVideo(filename,true,false);
        qDebug()<<"openFileButtonClicked1";
        initializeVideo(filename);
        qDebug()<<"openFileButtonClicked2";
        normalPlay();
        if(writePlayList(*playListLocal)){
            qDebug()<<"成功写入";
        }
        else {
            qDebug()<<"发生异常";
        }
    }
    QT_CATCH(QMediaPlayer::Error e)
    {
        qDebug()<<"出错了";
        return;
    }
}

void MainWindow::initializeVideo(QString fileName){
    /*用于初始化视频, 无论是正放还是倒放都要用到*/
    currMediaType = mediaType(fileName);

    // 若是音频文件, 则显示封面
    if(currMediaType==1){
//        QImage cover = getAttachedPic(fileName);
        QImage *notFoundImage = new QImage(":new/image/song.png");
        ui->reverse_widget->slotSetOneFrame(*notFoundImage);
       // ui->reverse_widget->slotSetOneFrame(cover);
    }
    // 初始化SeekFrame
    deleteSeekFrame();
    initSeekFrame(fileName);
    // 初始化文件信息
    initVideoInfo(fileName);
}

void MainWindow::highlightInFileList(){
    // 在播放列表中高亮currentVideoPath
    int numVideos = ui->playList->count();
    if(numVideos!=0){
        int currentVideoIndex = -1;
        for(int i = 0;i<numVideos;i++){
            QString i_address = ui->playList->item(i)->data(Qt::UserRole).toString();
            if(currentVideoPath==i_address){
                currentVideoIndex = i;
                break;
            }
        }
        if(currentVideoIndex!=-1){
            ui->playList->item(currentVideoIndex)->setSelected(true);
        }
    }
}

void MainWindow::showNormalWidget(){
    ui->normal_widget->setVisible(true);
    ui->reverse_widget->setVisible(false);
}
void MainWindow::showReverseWidget(){
    ui->normal_widget->setVisible(true);
    ui->reverse_widget->setVisible(false);
}
void MainWindow::normalPlay(){
    mediaPlayer->setMedia(QMediaContent(QUrl::fromLocalFile(currentVideoPath)));
    // 在媒体库中高亮
    highlightInFileList();
    playHistory->append(currentVideoPath);

    // 若是音频文件, 则显示封面
    if(currMediaType==1){
        showReverseWidget();
        QImage cover = getAttachedPic(currentVideoPath);
        ui->reverse_widget->slotSetOneFrame(cover);
        QImage *notFoundImage = new QImage(":new/image/song.png");

        ui->reverse_widget->slotSetOneFrame(*notFoundImage);

    }
    else{
        // 视频则播放
        showNormalWidget();
    }

    play();

    // 设置播放进度条最大值(秒数)
    qDebug()<<"MainWindow::normalPlay  "<<mediaPlayer->duration();
    qDebug()<<"MainWindow::normalPlay s "<<durationS;
    qDebug()<<"MainWindow::normalPlay ms "<<durationMs;

    ui->video_slider->setMaximum((int)durationMs);

    // 设置标记位
    if(!loadedVideo){
        loadedVideo = true;
    }

}
void MainWindow::initSystem(){
    loadedVideo = false; // 标记目前还没有加载视频

    playHistory = new QVector<QString>;
    playListLocal = readPlayList();
    if(playListLocal->size()!=0){
        for (int i=0;i<playListLocal->size();i++) {
            addVideoItem((*playListLocal)[i]);
        }
    }
    /*初始化seekFrame*/
    currVideoSeekFrame = NULL;

}



void MainWindow::changePlayingRatio(float rt){
    qDebug()<<"改变速率"<<rt;
    if(loadedVideo){
        mediaPlayer->setPlaybackRate(qreal(rt));
    }
}

void MainWindow::initWdigets(){
    /*此函数用于初始化组件*/
    // 设置图标
    QIcon * windowIcon = new QIcon(":new/image/player.png");
    this->setWindowIcon(*windowIcon);


    // 初始化播放按钮
    ui->pause_botton->setVisible(false); // 暂停按钮初始为不可见
    ui->sideBlock->setVisible(false); // 播放列表初始化为不可见



    pf_playback_rate = 1;    // 每次加刻度

    // 更新速度
    ui->speedBtn->setText(QString("倍速:%1").arg(pf_playback_rate));
}

void MainWindow::skipForwardOrBackward(bool mode)
{
        if(!loadedVideo){
            qDebug()<<"skipForwardOrBackward:尚未加载视频";
            return;
        }
        if(mode){
            // 前进
            int jumpSecond = 5;
            if(currentPosition+jumpSecond*1000>=durationMs){
                // 减10是为了不要跳到最后
                mediaPlayer->setPosition(durationMs-10);
                qDebug()<<"jump end";
            }
            else{
                jump(jumpSecond);
            }
        }
        else{
            // 后退
            int jumpSecond = -5;
            if(currentPosition+jumpSecond*1000<=0){
                mediaPlayer->setPosition(10);
                qDebug()<<"jump start";
            }
            else{
                jump(jumpSecond);
            }

        }
}
void MainWindow::pause(){
    m_playerState = QMediaPlayer::StoppedState;
    ui->play_button->setVisible(true);
    ui->pause_botton->setVisible(false);
    mediaPlayer->pause();
}
void MainWindow::play(){
    m_playerState = QMediaPlayer::PlayingState;
    ui->play_button->setVisible(false);
    ui->pause_botton->setVisible(true);
    qDebug()<<"开始播放";
    mediaPlayer->play();
    // ui->label->setText(QString("当前播放:%1").arg());
}

void MainWindow::changeVideo(bool nextOrPrevious){
    /*
     * 可能的情况
     * 1. 播放列表什么也没有
     *  1. 没有加载视频
     *  2. 有加载视频=> 跳转到开始
     * 2. 播放列表有东西
     *  1. 没有加载视频
     *  2. 正在播的东西不在播放列表中
     *  3. 正在播的东西在列表中
    */
    if(!loadedVideo){ // solution for 1.1 & 2.1
        qDebug()<<"changeVideo:尚未加载视频";
        return;
    }
    // 若当前播放列表没有视频, 但有加载 (solution for 1.2)
    int numVideos = ui->playList->count();
    if(numVideos==0){
        // 此时下一个视频和上一个视频都是自己, 直接跳到开头
        mediaPlayer->setPosition(10);
        if(m_playerState==QMediaPlayer::PausedState)play();
        return;
    }
    int currentVideoIndex = -1;
    QString nextPath = "";
    QString previousPath = "";
    for(int i = 0;i<numVideos;i++){
        QString i_address = ui->playList->item(i)->data(Qt::UserRole).toString();
        if(currentVideoPath==i_address){
            currentVideoIndex = i;
            break;
        }
    }
    if(currentVideoIndex==-1){
        // 说明视频不在播放列表,
        // previousPath取播放记录的上一个视频
        // nextPath取播放列表的第一个视频
        nextPath = ui->playList->item(0)->data(Qt::UserRole).toString();
        if(playHistory->count()-2<0){
            // 在此之前没有播放过视频, prev此时不知道选什么, 那就播放列表第一个了吧
            previousPath = nextPath;
        }
        else{
            previousPath = (*playHistory)[playHistory->count()-2];
        }
    }
    else{
        // 说明视频在播放列表
        int previousIndex = currentVideoIndex-1;
        int nextIndex = currentVideoIndex+1;
        if(previousIndex<0){
            // 处于第一个, previous 是最后一个
            previousIndex = numVideos-1;
        }
        if (nextIndex>=numVideos){
            // 说明是最后一个, next 是最第一个
            nextIndex = 0;
        }
        previousPath = ui->playList->item(previousIndex)->data(Qt::UserRole).toString();
        nextPath = ui->playList->item(nextIndex)->data(Qt::UserRole).toString();
    }

    qDebug()<<"next"<<nextPath;
    qDebug()<<"previous"<<previousPath;
    if(nextOrPrevious){
        // 下一个视频(默认点击 上/下_一个视频的时候, 选择正放)
//        initVideo(nextPath,true,false);
        initializeVideo(nextPath);
        normalPlay();
    }
    else{
        //前一个视频
//        initVideo(previousPath,true,false);
        initializeVideo(previousPath);
        normalPlay();
    }
}




void MainWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        this->m_drag = true;
        this->dragPos = event->pos();
        this->resizeDownPos = event->globalPos();
        this->mouseDownRect = this->rect();

        qDebug()<<"state:"<<m_playerState;
        if(m_playerState == QMediaPlayer::PlayingState){
            pause();

        }
        else if(m_playerState == QMediaPlayer::StoppedState){
            play();

        }
    }
}
void MainWindow::mouseMoveEvent(QMouseEvent * event)
{
    if (resizeRegion != Default)
    {
        handleResize();
        return;
    }
    if(m_move) {
        move(event->globalPos() - dragPos);
        return;
    }
    QPoint clientCursorPos = event->pos();
    QRect r = this->rect();
    QRect resizeInnerRect(resizeBorderWidth, resizeBorderWidth, r.width() - 2*resizeBorderWidth, r.height() - 2*resizeBorderWidth);
    if(r.contains(clientCursorPos) && !resizeInnerRect.contains(clientCursorPos)) { //调整窗体大小
        ResizeRegion resizeReg = getResizeRegion(clientCursorPos);
        setResizeCursor(resizeReg);
        if (m_drag && (event->buttons() & Qt::LeftButton)) {
            resizeRegion = resizeReg;
            handleResize();
        }
    }
    else {


        //移动窗体
        setCursor(Qt::ArrowCursor);
        if (m_drag && (event->buttons() & Qt::LeftButton)) {
            m_move = true;
            move(event->globalPos() - dragPos);
        }

    }
}
void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    m_drag = false;
    if(m_move) {
        m_move = false;
//        handleMove(event->globalPos()); //鼠标放开后若超出屏幕区域自动吸附于屏幕顶部/左侧/右侧
    }
    resizeRegion = Default;
    setCursor(Qt::ArrowCursor);
}
void MainWindow::setResizeCursor(ResizeRegion region)
{
    switch (region)
    {
    case North:
    case South:
        setCursor(Qt::SizeVerCursor);
        break;
    case East:
    case West:
        setCursor(Qt::SizeHorCursor);
        break;
    case NorthWest:
    case SouthEast:
        setCursor(Qt::SizeFDiagCursor);
        break;
    default:
        setCursor(Qt::SizeBDiagCursor);
        break;
    }
}
ResizeRegion MainWindow::getResizeRegion(QPoint clientPos)
{
    if (clientPos.y() <= resizeBorderWidth) {
        if (clientPos.x() <= resizeBorderWidth)
            return NorthWest;
        else if (clientPos.x() >= this->width() - resizeBorderWidth)
            return NorthEast;
        else
            return North;
    }
    else if (clientPos.y() >= this->height() - resizeBorderWidth) {
        if (clientPos.x() <= resizeBorderWidth)
            return SouthWest;
        else if (clientPos.x() >= this->width() - resizeBorderWidth)
            return SouthEast;
        else
            return South;
    }
    else {
        if (clientPos.x() <= resizeBorderWidth)
            return West;
        else
            return East;
    }
}

void MainWindow::handleResize()
{
    int xdiff = QCursor::pos().x() - resizeDownPos.x();
    int ydiff = QCursor::pos().y() - resizeDownPos.y();
    qDebug()<<"xdiff"<<xdiff;
    qDebug()<<"ydiff"<<ydiff;
    switch (resizeRegion)
    {
    case East:
    {
        resize(mouseDownRect.width()+xdiff, this->height());
        break;
    }
    case West:
    {
        resize(mouseDownRect.width()-xdiff, this->height());
        move(resizeDownPos.x()+xdiff, this->y());
        break;
    }
    case South:
    {
        resize(this->width(),mouseDownRect.height()+ydiff);
        break;
    }
    case North:
    {
        resize(this->width(),mouseDownRect.height()-ydiff);
        move(this->x(), resizeDownPos.y()+ydiff);
        break;
    }
    case SouthEast:
    {
        resize(mouseDownRect.width() + xdiff, mouseDownRect.height() + ydiff);
        break;
    }
    case NorthEast:
    {
        resize(mouseDownRect.width()+xdiff, mouseDownRect.height()-ydiff);
        move(this->x(), resizeDownPos.y()+ydiff);
        break;
    }
    case NorthWest:
    {
        resize(mouseDownRect.width()-xdiff, mouseDownRect.height()-ydiff);
        move(resizeDownPos.x()+xdiff, resizeDownPos.y()+ydiff);
        break;
    }
    case SouthWest:
    {
        resize(mouseDownRect.width()-xdiff, mouseDownRect.height()+ydiff);
        move(resizeDownPos.x()+xdiff, this->y());
        break;
    }
    }
}
void MainWindow::reverseShowRatio(qint64 pts){
    /*用于更新进度条*/
    ui->video_slider->setValue(int(pts));
}

void MainWindow::initSeekFrame(QString fileName){
    // 初始化seekFrame 类
    currVideoSeekFrame = new SeekFrame(fileName,1000,9999);
}

void MainWindow::deleteSeekFrame(){
    if(this->currVideoSeekFrame!=NULL){
        // 释放内存
        delete currVideoSeekFrame;
        currVideoSeekFrame = NULL;
    }
}
void MainWindow::changePlayOrder(){
    if(isRepeat){
        ui->repeat->setStyleSheet(
                   " QPushButton{ "
                   " image: url(:new/image/inorder.png);"
                   " border-radius:0px; "
                   " } "
                    );
        isRepeat=false;
    }
    else{
        ui->repeat->setStyleSheet(
                   " QPushButton{ "
                   " image: url(:new/image/repeat.png);"
                   " border-radius:0px; "
                   " } "
                    );
        isRepeat = true;
    }
}
void MainWindow::on_speedBtn_clicked()
{
    // 设置变速
    pf_playback_rate += PLAYBACK_RATE_SCALE;    // 每次加刻度
    if(pf_playback_rate > PLAYBACK_RATE_MAX)
    {
        pf_playback_rate = PLAYBACK_RATE_MIN;
    }
    pf_playback_rate_changed = 1;
    // 更新速度
    ui->speedBtn->setText(QString("倍速:%1").arg(pf_playback_rate));
    changePlayingRatio(pf_playback_rate);
}
