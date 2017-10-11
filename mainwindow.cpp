#include "mainwindow.h"
#include "ui_mainwindow.h"
using namespace std;
QFile igcFile("flightlog.igc");
#define NMEA_BUFFER  1024

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this); 
    printf("Searching for gps module\n");

    //QMainWindow::showFullScreen();

    QObject::connect(this, SIGNAL(emitcenterMap(QPointF)),this, SLOT(centerMap(QPointF)));
    QObject::connect(this, SIGNAL(emitupdateValues()),this, SLOT(updateValues()));
    QObject::connect(this, SIGNAL(emitupdateGpsFix(char*)),this, SLOT(updateGpsFix(char*)));

    m_IsSerialThreadRunning = false;

    m_GpsItems.latitude     = 0;
    m_GpsItems.longitude    = 0;
    m_GpsItems.oldlatitude  = 0;
    m_GpsItems.oldlongitude = 0;
    m_GpsItems.accuracy     = 0;
    m_GpsItems.altitude     = 0;
    m_GpsItems.accuracy     = 0;  
    m_GpsItems.bearing      = 0;
    m_GpsItems.gps_fix      = 0;
    m_GpsItems.sat_inview   = 0;    
    m_GpsItems.speed        = 0;

    m_gpsConnected = false;
    createIgcFile = false;  

    qApp->installEventFilter(this);

    ui->lbl_gpsspeed->setStyleSheet("color: white;background-color: rgba(255,51,51, 175);");
    ui->lbl_gpsaltitude->setStyleSheet("color: white;background-color: rgba(255,51,51, 175);");
    ui->lbl_gpsbearing->setStyleSheet("color: white;background-color: rgba(255,51,51, 175);");
    ui->lbl_gpsaccuracy->setStyleSheet("color: white;background-color: rgba(255,51,51, 175);");
    ui->lbl_gpstime->setStyleSheet("color: white;background-color: rgba(255,51,51, 175);");
    ui->pushButton_exit->setStyleSheet("color: white;background-color: rgba(255,51,51, 175);");
    ui->pushButton_zoomp->setStyleSheet("color: white;background-color: rgba(16,78,139, 175);");
    ui->pushButton_zoomm->setStyleSheet("color: white;background-color: rgba(16,78,139, 175);");
    ui->lbl_gpsfix->setStyleSheet("color:black;");
    ui->lbl_maptile->setStyleSheet("color: black;");

    layout = ui->gridLayout;

    //Setup the MapGraphics scene and view
    scene = new MapGraphicsScene(this);
    view = new MapGraphicsView(scene,this);
    layout->addWidget(view);

    objCircle = new CircleObject(8,true,QColor(255,51,51,255));
    scene->addObject(objCircle);

    createMap(OSMTileSource::GrayTiles);

    view->setZoomLevel(16);

    QPointF pos = QPointF(32.546586,39.842953);
    centerMap(pos);

    if (igcFile.exists())
    {
       igcFile.remove();
    }

    sprintf(s_port,"/dev/rfcomm0");

    if(!m_gpsConnected)
    {
       m_ConnectionThreadInstance.reset(new boost::thread(boost::bind(&MainWindow::checkConnectionThread, this)));
    }  
}

MainWindow::~MainWindow()
{
    if(igcFile.isOpen())
    {
        igcFile.close();
    }

    if (m_SerialThreadInstance.get() != 0)
    {
        if (!m_SerialThreadInstance->try_join_for(boost::chrono::milliseconds(1000)))
        {
            // interrupt the thread
            m_SerialThreadInstance->interrupt();
        }
    }

    disConnetSerialPort();

    delete ui;
}

void MainWindow::createMap(OSMTileSource::OSMTileType tileSource)
{
    //Setup some tile sources
    QSharedPointer<OSMTileSource> osmTiles(new OSMTileSource(tileSource), &QObject::deleteLater);
    QSharedPointer<CompositeTileSource> composite(new CompositeTileSource(), &QObject::deleteLater);
    composite->addSourceBottom(osmTiles);
    view->setTileSource(composite);   
    currentMap = osmTiles->name();
    ui->lbl_maptile->setText(currentMap);
    ::usleep(SLEEP_PERIOD * 100);
}

void MainWindow::centerMap(QPointF pos)
{    
    view->centerOn(pos);
    objCircle->setPos(pos);

    if(m_GpsItems.bearing != m_GpsItems.oldbearing)
    {
        setHeading(360 - m_GpsItems.bearing);
    }
}

void MainWindow::setHeading(qreal angle)
{
    view->rotate(-1 * (360 - m_GpsItems.oldbearing));
    view->rotate(angle);
    m_GpsItems.oldbearing = m_GpsItems.bearing;
}

void MainWindow::setMarker(QPointF center)
{
    objCircle->setPos(center);
}


bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonDblClick)
    {
        QString strobj = QString(obj->objectName());
        if(strobj.isEmpty())
        {           
            ui->lbl_gpsfix->setStyleSheet("color:black;");
            ui->lbl_maptile->setStyleSheet("color: black;");

            //QMainWindow::showFullScreen();
            if(currentMap.contains("Gray"))
            {
              createMap(OSMTileSource::OsmTiles);
            }
            else if(currentMap.contains("Osm"))
            {
              createMap(OSMTileSource::TerrainTiles);
            }
            else if(currentMap.contains("Terrain"))
            {
              createMap(OSMTileSource::GoogleTiles);
            }
            else if(currentMap.contains("Google"))
            {
              createMap(OSMTileSource::SatTiles);
              ui->lbl_gpsfix->setStyleSheet("color: white;");
              ui->lbl_maptile->setStyleSheet("color: white;");
            }
            else if(currentMap.contains("Sat"))
            {
              createMap(OSMTileSource::GrayTiles);
            }
        }
    }
    // pass the event on to the parent class
    return QWidget::eventFilter(obj, event);
}

void MainWindow::printValues()
{
    QTextStream out(stdout);

    out << "Retrieved values :" << endl;
    out << QString(" - Latitude     = ") << QString::number( m_GpsItems.latitude, 'f', 6 ) << " " << m_GpsItems.latDirection << endl;
    out << QString(" - Longitude    = ") << QString::number( m_GpsItems.longitude, 'f', 6 ) << " " << m_GpsItems.lonDirection << endl;
    out << QString(" - Altitude     = ") << QString::number( m_GpsItems.altitude, 'f', 0 ) << " m" << endl;
    out << QString(" - Speed        = ") << QString::number( m_GpsItems.speed, 'f', 2 ) << " km/h" << endl;
    out << QString(" - Bearing      = ") << QString::number( m_GpsItems.bearing, 'd', 0 ) << " deg" << endl;
    out << QString(" - Accuracy     = ") << QString::number( m_GpsItems.accuracy, 'f', 2 ) << " m" << endl;
    out << QString(" - Sats in view = ") << QString::number( m_GpsItems.sat_inview, 'd', 0 ) << endl;
    out << QString(" - Gps fix      = ") << QString::number( m_GpsItems.gps_fix, 'd', 0 ) << " ( 0 = Invalid 1 = GPS fix 2 = DGPS fix )" << endl;

    char s[20]; /* strlen("2009-08-10 18:17:54") + 1 */
    strftime(s, 20, "%d-%m-%y %H:%M:%S", &m_GpsItems.localtime);
    out << QString(" - Time         = ") << s << endl;

}

void MainWindow::updateGpsFix(char* port)
{
    ui->lbl_gpsfix->setText("Searching gps: " +  QString(port));
}

void MainWindow::updateValues()
{
    QChar ch(0x00B0); //replace NN with the unicode charecter code for the degree symbol
    QString degreeSymbol=" ";
    degreeSymbol.append(ch);

    ui->lbl_gpsspeed->setText(QString::number( m_GpsItems.speed, 'f', 1) + QString(" km/h"));
    ui->lbl_gpsaltitude->setText(QString::number( m_GpsItems.altitude, 'f', 0 ) + QString(" m"));
    ui->lbl_gpsbearing->setText(QString::number( m_GpsItems.bearing, 'd', 0 ) + degreeSymbol);
    ui->lbl_gpsaccuracy->setText(QString::number( m_GpsItems.accuracy, 'f', 2 ) + QString(" m"));
    QString gpsFix =  m_GpsItems.gps_fix ? "GPS fixed" : "Waiting for GPS fix";
    ui->lbl_gpsfix->setText(gpsFix + QString(" Sats : ") + QString::number( m_GpsItems.sat_inview, 'd', 0 ));
    char s[9]; /* strlen("2009-08-10 18:17:54") + 1 */
    strftime(s, 9, "%H:%M:%S", &m_GpsItems.localtime);
    ui->lbl_gpstime->setText(QString(s));
    printValues();
}

/////igc file///////////

void MainWindow::updateIGC()
{
    if(!createIgcFile)
    {
        createIgcHeader();
        createIgcFile = true;
    }

    igcFile.open(QIODevice::Append | QIODevice::Text);

    if(!igcFile.isOpen()){
       qDebug() << "- Error, unable to open" << "outputFilename" << "for output";
    }

    QTextStream out(&igcFile);
    QString str;

    //B,110135,5206343N,00006198W,A,00587,00558
    strftime(currentTime, 20, "%H%M%S", &m_GpsItems.localtime);

    if(strcmp(oldTime, currentTime) != 0)
    {
        QString record  = "B";

        record.append(QString::fromUtf8(currentTime).simplified());
        record.append(m_GpsItems.igclatitude);
        record.append(m_GpsItems.igclongitude);
        record.append("A");
        str.sprintf("%05d",(int)m_GpsItems.altitude);
        record.append(str);
        record.append(str);
        out << record << endl;
        strcpy(oldTime,currentTime);
    }

    igcFile.close();
}

void MainWindow::createIgcHeader()
{
    igcFile.open(QIODevice::Append | QIODevice::Text);

    if(!igcFile.isOpen()){
       qDebug() << "- Error, unable to open" << "outputFilename" << "for output";
    }

    QTextStream out(&igcFile);

    char s[20];
    strftime(s, 20, "%d%m%y", &m_GpsItems.localtime);

    QString header  = "AXXX XcMap v1.0\n";
    header.append("HFDTE");
    header.append(QString("%1\n").arg(s));
    header.append("DTM100GPSDATUM: WGS-1984");
    out << header << endl;
    igcFile.close();
}

void MainWindow::on_pushButton_exit_clicked()
{
    exit(0);
}

///// Gps Device /////


bool MainWindow::connectGpsPort()
{
    //try usb ports ttyACM
    for( int port = 0; port < 10; port++ )
    {
        sprintf(s_port,"/dev/ttyACM%d", port );
        emitupdateGpsFix(s_port);

        if(checkGpsPort())
        {
            if(connetSerialPort(s_port))
            {
                m_gpsConnected = true;
                return true;
            }
        }
        ::usleep(SLEEP_PERIOD * 50);
    }

    return false;
}

bool MainWindow::checkGpsPort()
{
    QRegExp gpsCheck("GP", Qt::CaseInsensitive);
    int tryPort = 0;

    if(connetSerialPort(s_port))
    {
        while(tryPort < 5)
        {
            char buffer[NMEA_BUFFER];

            if(getData(buffer, sizeof(buffer)) != -1)
            {
                QString str = QString::fromUtf8(buffer).simplified();
                if(!str.isEmpty() )
                {
                    if(str.contains(gpsCheck))
                    {
                        printf("Gps Module found on: %s\n\n",s_port);
                        return true;
                    }
                }
            }

            tryPort++;
            ::usleep(SLEEP_PERIOD * 10);
        }
        tryPort = 0;
        disConnetSerialPort();
    }
    return false;
}

void MainWindow::changeEvent( QEvent* e )
{
    if( e->type() == QEvent::WindowStateChange )
    {

    }
}

std::string MainWindow::exec(const char* cmd)
{
    char buffer[128];
    std::string result = "";
    FILE* pipe = popen(cmd, "r");
    if (!pipe) throw std::runtime_error("popen() failed!");
    try {
        while (!feof(pipe)) {
            if (fgets(buffer, 128, pipe) != NULL)
                result += buffer;
        }
    } catch (...) {
        pclose(pipe);
        throw;
    }
    pclose(pipe);
    return result;
}

bool MainWindow::connetSerialPort( const char* port)
{
    struct termios options;

    // linux
    fd = open(port, O_RDWR | O_NOCTTY | O_NONBLOCK);

    /* get the current options */
    tcgetattr(fd, &options);

    /* Set Baud Rate */
    cfsetospeed (&options, (speed_t)B115200);
    cfsetispeed (&options, (speed_t)B115200);

    /* Setting other Port Stuff */
    options.c_cflag     &=  ~PARENB;            // Make 8n1
    options.c_cflag     &=  ~CSTOPB;
    options.c_cflag     &=  ~CSIZE;
    options.c_cflag     |=  CS8;

    options.c_cflag     &=  ~CRTSCTS;           // no flow control
    options.c_cc[VMIN]   =  1;                  // read doesn't block
    options.c_cc[VTIME]  =  1;                  // 0.1 seconds read timeout
    options.c_cflag     |=  CREAD | CLOCAL;     // turn on READ & ignore ctrl lines

    /* Make raw */
    cfmakeraw(&options);

    /* Flush Port, then applies attributes */
    tcflush( fd, TCIFLUSH );
    if ( tcsetattr ( fd, TCSANOW, &options ) != 0)
    {
       //std::cout << "Error " << errno << " from tcsetattr" << std::endl;
       return false;
    }

    return true;
}

void MainWindow::disConnetSerialPort()
{
    // If the port is actually open, close it
    if (fd != -1) {
        ::close(fd);
    }
}

int MainWindow::getData(char* data, int size)
{
    /* Allocate memory for read buffer */
    char buf [size];
    memset (&buf, '\0', sizeof buf);

    if (fd != -1)
    {
        int n = read( fd, &buf , sizeof buf );
        if (n > 0)
        {
            strncpy(data, buf, size);
            data[size -1] = '\0';
            return n;
        }
    }

    return -1;
}

void MainWindow::checkConnectionThread()
{
    while(!m_gpsConnected)
    {
        connectGpsPort();

        if(m_gpsConnected)
        {
            m_IsSerialThreadRunning = true;
            m_SerialThreadInstance.reset(new boost::thread(boost::bind(&MainWindow::checkSerialThread, this)));
        }

        ::usleep(SLEEP_PERIOD * 50);
    }
}

void MainWindow::checkSerialThread()
{
    while (m_IsSerialThreadRunning)
    {
        char buffer[NMEA_BUFFER];
        memset (&buffer, '\0', sizeof buffer);

        if(getData(buffer, sizeof(buffer)) != -1)
        {
            QString str = QString::fromUtf8(buffer);//.simplified();

            if(!str.isEmpty())
            {
                parseNMEA(str);
                //printf("%s\n",str.toStdString().c_str());
            }
        }
        ::usleep(SLEEP_PERIOD * 250);
    }
}

void MainWindow::parseNMEA(QString value)
{
    QRegExp ggaRegExp("(GP|GN|GL)GGA", Qt::CaseInsensitive);
    QRegExp gllRegExp("(GP|GN|GL)GLL", Qt::CaseInsensitive);
    QRegExp rmcRegExp("(GP|GN|GL)RMC", Qt::CaseInsensitive);
    QRegExp vtgRegExp("(GP|GN|GL)VTG", Qt::CaseInsensitive);
    QRegExp gsvRegExp("(GP|GN|GL)GSV", Qt::CaseInsensitive);
    QRegExp gsaRegExp("(GP|GN|GL)GSA", Qt::CaseInsensitive);

    QRegExp latRegExp("(\\d{2})(\\d{2}\\.\\d+)", Qt::CaseInsensitive);
    QRegExp lonRegExp("(\\d{3})(\\d{2}\\.\\d+)", Qt::CaseInsensitive);

    //// all contains your input ////
    QStringList lines = value.split(QRegExp("\\$"));
    QString str;

    foreach(QString line, lines)
    {
        QStringList fields = line.split(",");
        //printf("%s\n",line.simplified().toStdString().c_str());

        if(fields[0].contains(ggaRegExp))
        {
            if(fields[2].contains(latRegExp))
            {
               int deg = latRegExp.cap(1).toInt();
               double min = latRegExp.cap(2).toDouble();
               m_GpsItems.latitude = (double)deg+min/60.0;
               m_GpsItems.latDirection = fields[3];

               str.sprintf("%02d%02.3f",deg,min);
               m_GpsItems.igclatitude = str.remove(QRegExp("[^a-zA-Z\\d\\s]")) + m_GpsItems.latDirection;
            }

            if(fields[3] == "S")
               m_GpsItems.latitude *= -1;

            if(fields[4].contains(lonRegExp))
            {
               int deg = lonRegExp.cap(1).toInt();
               double min = lonRegExp.cap(2).toDouble();
               m_GpsItems.longitude = (double)deg+min/60.0;
               m_GpsItems.lonDirection = fields[5];

               str.sprintf("%03d%02.3f", deg,min);
               m_GpsItems.igclongitude = str.remove(QRegExp("[^a-zA-Z\\d\\s]")) + m_GpsItems.lonDirection;
            }

            if(fields[5] == "W")
               m_GpsItems.longitude *= -1;

            m_GpsItems.gps_fix = fields[6].toInt();
            m_GpsItems.sat_inview = fields[7].toInt();
            m_GpsItems.accuracy = fields[8].toDouble();
            m_GpsItems.altitude = fields[9].toDouble();
        }       
        else if(fields[0].contains(rmcRegExp))
        {
            if(fields[2].contains(latRegExp))
            {
               int deg = latRegExp.cap(1).toInt();
               double min = latRegExp.cap(2).toDouble();
               m_GpsItems.latitude = (double)deg+min/60.0;
               m_GpsItems.latDirection = fields[3];
            }

            if(fields[3] == "S")
               m_GpsItems.latitude *= -1;

            if(fields[4].contains(lonRegExp))
            {
               int deg = lonRegExp.cap(1).toInt();
               double min = lonRegExp.cap(2).toDouble();
               m_GpsItems.longitude = (double)deg+min/60.0;
               m_GpsItems.lonDirection = fields[5];
            }

            if(fields[5] == "W")
               m_GpsItems.longitude *= -1;

            QString time = fields[1];
            nmea_time_parse((char*)time.toStdString().c_str(), &m_GpsItems.utctime);

            time = fields[9];
            nmea_date_parse((char*)time.toStdString().c_str(), &m_GpsItems.utctime);

            time_t t = timegm(&m_GpsItems.utctime);
            m_GpsItems.localtime = *localtime(&t);

            m_GpsItems.speed = fields[7].toDouble() * toKph;
            m_GpsItems.bearing = fields[8].toDouble();
        }
        else if(fields[0].contains(gllRegExp))
        {
           if(fields[1].contains(latRegExp))
           {
              int deg = latRegExp.cap(1).toInt();
              double min = latRegExp.cap(2).toDouble();
              m_GpsItems.latitude = (double)deg+min/60.0;
              m_GpsItems.latDirection = fields[2];
           }

           if(fields[2] == "S")
              m_GpsItems.latitude *= -1;

           if(fields[3].contains(lonRegExp))
           {
              int deg = lonRegExp.cap(1).toInt();
              double min = lonRegExp.cap(2).toDouble();
              m_GpsItems.longitude = (double)deg+min/60.0;
              m_GpsItems.lonDirection = fields[4];
           }

           if(fields[4] == "W")
              m_GpsItems.longitude *= -1;
        }
        else if(line.contains(vtgRegExp))
        {
            //m_GpsItems.speed = fields[5].toDouble();
        }
        else if(line.contains(gsvRegExp))
        {
            m_GpsItems.sat_inview = fields[3].toInt();
        }
        else if(line.contains(gsaRegExp))
        {
            m_GpsItems.accuracy = fields[16].toDouble();
        }

        if(m_GpsItems.latitude != 0 && m_GpsItems.longitude != 0)
        {
            updateIGC();
            m_GpsItems.oldlatitude = m_GpsItems.latitude;
            m_GpsItems.oldlongitude = m_GpsItems.longitude;

            QPointF pos = QPointF(m_GpsItems.longitude,m_GpsItems.latitude);
            emitcenterMap(pos);
        }

        updateValues();
    }
}


void MainWindow::on_pushButton_zoomp_clicked()
{
    view->setZoomLevel(view->zoomLevel() + 1);
}

void MainWindow::on_pushButton_zoomm_clicked()
{
    view->setZoomLevel(view->zoomLevel() - 1);
}
