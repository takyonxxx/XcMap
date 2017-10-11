#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtGui/qwidget.h>
#include <QMainWindow>
#include <QDebug>
#include <QTextStream>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QFormLayout>

#include <QSharedPointer>
#include <QtDebug>
#include <QThread>
#include <QImage>


#include "MapGraphicsView.h"
#include "MapGraphicsScene.h"
#include "tileSources/GridTileSource.h"
#include "tileSources/OSMTileSource.h"
#include "tileSources/CompositeTileSource.h"
#include "guts/CompositeTileSourceConfigurationWidget.h"
#include "CircleObject.h"
#include "PolygonObject.h"

#include <boost/thread.hpp>

#include <math.h>
#include <time.h>
#include <termios.h> /* POSIX terminal control definitions */
#include <errno.h> /* Error number definitions */
#include <fcntl.h> /* File control definitions */
#include<iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <string.h>

#define NMEA_TIME_FORMAT	"%H%M%S"
#define NMEA_TIME_FORMAT_LEN	6

#define NMEA_DATE_FORMAT	"%d%m%y"
#define NMEA_DATE_FORMAT_LEN	6

#define toMps   0.514444444444 // knots -> m/s
#define toMph   1.150779;      // knots -> mph
#define toKph   1.852          // knots -> kph
#define earthRadiusKm 6371.0

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:

    static const unsigned int SLEEP_PERIOD = 1000;
    static const unsigned int SLEEP_COUNTER= 1;

    // This function converts decimal degrees to radians
    double deg2rad(double deg) {
      return (deg * M_PI / 180);
    }

    //  This function converts radians to decimal degrees
    double rad2deg(double rad) {
      return (rad * 180 / M_PI);
    }

    double distanceEarth(double lat1d, double lon1d, double lat2d, double lon2d) {
      double lat1r, lon1r, lat2r, lon2r, u, v;
      lat1r = deg2rad(lat1d);
      lon1r = deg2rad(lon1d);
      lat2r = deg2rad(lat2d);
      lon2r = deg2rad(lon2d);
      u = sin((lat2r - lat1r)/2);
      v = sin((lon2r - lon1r)/2);
      return 2.0 * earthRadiusKm * asin(sqrt(u * u + cos(lat1r) * cos(lat2r) * v * v));
    }

    double constrainAngle(double x){
        x = fmod(x,360);
        if (x < 0)
            x += 360;
        return x;
    }

    double CalculateHeading(double lat1, double long1, double lat2, double long2)
    {
        double a = lat1 * M_PI / 180;
        double b = long1 * M_PI / 180;
        double c = lat2 * M_PI / 180;
        double d = long2 * M_PI / 180;

        if (cos(c) * sin(d - b) == 0)
            if (c > a)
                return 0;
            else
                return 180;
        else
        {
            double angle = atan2(cos(c) * sin(d - b), sin(c) * cos(a) - sin(a) * cos(c) * cos(d - b));
            return constrainAngle(angle * 180 / M_PI + 360);
        }
    }

     int nmea_time_parse(char *s, struct tm *time)
     {
        char *rv;

        memset(time, 0, sizeof (struct tm));

        if (s == NULL || *s == '\0') {
            return -1;
        }

        rv = strptime(s, NMEA_TIME_FORMAT, time);
        if (NULL == rv || (int) (rv - s) != NMEA_TIME_FORMAT_LEN) {
            return -1;
        }

        return 0;
     }

     int nmea_date_parse(char *s, struct tm *time)
     {
        char *rv;

        // Assume it has been already cleared
        // memset(time, 0, sizeof (struct tm));

        if (s == NULL || *s == '\0') {
            return -1;
        }

        rv = strptime(s, NMEA_DATE_FORMAT, time);
        if (NULL == rv || (int) (rv - s) != NMEA_DATE_FORMAT_LEN) {
            return -1;
        }

        return 0;
     }

     time_t timegm( struct tm *tm ) {
      time_t t = mktime( tm );
      return t + localtime( &t )->tm_gmtoff;
     }


    typedef struct GpsItems
       {
           double latitude;
           double oldlatitude;
           QString igclatitude;
           QString latDirection;
           double longitude;
           double oldlongitude;
           QString igclongitude;
           QString lonDirection;
           double altitude;
           struct tm utctime;
           struct tm localtime;
           double accuracy;           
           double speed;
           double bearing;
           double oldbearing;
           int gps_fix;          
           int sat_inview;

       }GpsItems;

       GpsItems m_GpsItems;      

   MapGraphicsView *view ;
   MapGraphicsScene *scene;

   CircleObject *objCircle;
   QString currentMap;
   QGridLayout *layout;

   int fd; //rfcomm0
   char log_buffer[1024];
   char s_port[32];
   bool m_gpsConnected;
   bool createIgcFile;

   char oldTime[20];
   char currentTime[20];

   boost::atomic_bool m_IsSerialThreadRunning;
   boost::shared_ptr<boost::thread> m_SerialThreadInstance;   
   boost::shared_ptr<boost::thread> m_ConnectionThreadInstance;

   void createMap(OSMTileSource::OSMTileType tileSource);

   void parseNMEA(QString value);
   void changeEvent( QEvent* e );

   bool connetSerialPort( const char* port);
   void disConnetSerialPort();
  // bool bindRFComm(const char* dest);
  // int sdp_search_spp(sdp_session_t *sdp, uint8_t *channel);
   int getData(char* data,int size);

   void checkSerialThread();
   void checkConnectionThread();
   bool connectGpsPort();
   bool checkGpsPort();

   void createIgcHeader();
   void updateIGC();
   void setHeading(qreal angle);
   void setMarker(QPointF center);

protected:
   bool eventFilter(QObject *obj, QEvent *event);

private slots:   
   void updateValues();
   void updateGpsFix(char* port);
   void centerMap(QPointF pos);
   void on_pushButton_exit_clicked();   
   void on_pushButton_zoomp_clicked();
   void on_pushButton_zoomm_clicked();

signals:
   void emitcenterMap(QPointF);
   void emitupdateValues();
   void emitupdateGpsFix(char* port);

public:
   void printValues();
   std::string exec(const char* cmd);

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
