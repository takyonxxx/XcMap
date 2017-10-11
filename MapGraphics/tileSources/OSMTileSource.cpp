#include "OSMTileSource.h"

#include "guts/MapGraphicsNetwork.h"

#include <cmath>
#include <QPainter>
#include <QStringBuilder>
#include <QtDebug>
#include <QNetworkReply>

const qreal PI = 3.14159265358979323846;
const qreal deg2rad = PI / 180.0;
const qreal rad2deg = 180.0 / PI;

OSMTileSource::OSMTileSource(OSMTileType tileType) :
    MapTileSource(), _tileType(tileType)
{
    this->setCacheMode(MapTileSource::DiskAndMemCaching);
}

OSMTileSource::~OSMTileSource()
{
    qDebug() << this << this->name() << "Destructing";
}

QPointF OSMTileSource::ll2qgs(const QPointF &ll, quint8 zoomLevel) const
{
    const qreal tilesOnOneEdge = pow(2.0,zoomLevel);
    const quint16 tileSize = this->tileSize();
    qreal x = (ll.x()+180) * (tilesOnOneEdge*tileSize)/360; // coord to pixel!
    qreal y = (1-(log(tan(PI/4+(ll.y()*deg2rad)/2)) /PI)) /2  * (tilesOnOneEdge*tileSize);

    return QPoint(int(x), int(y));
}

QPointF OSMTileSource::qgs2ll(const QPointF &qgs, quint8 zoomLevel) const
{
    const qreal tilesOnOneEdge = pow(2.0,zoomLevel);
    const quint16 tileSize = this->tileSize();
    qreal longitude = (qgs.x()*(360/(tilesOnOneEdge*tileSize)))-180;
    qreal latitude = rad2deg*(atan(sinh((1-qgs.y()*(2/(tilesOnOneEdge*tileSize)))*PI)));

    return QPointF(longitude, latitude);
}

quint64 OSMTileSource::tilesOnZoomLevel(quint8 zoomLevel) const
{
    return pow(4.0,zoomLevel);
}

quint16 OSMTileSource::tileSize() const
{
    return 256;
}

quint8 OSMTileSource::minZoomLevel(QPointF ll)
{
    Q_UNUSED(ll)
    return 0;
}

quint8 OSMTileSource::maxZoomLevel(QPointF ll)
{
    Q_UNUSED(ll)
    return 18;
}

QString OSMTileSource::name() const
{
    switch(_tileType)
    {
    case OsmTiles:
        return "OsmTiles";
        break;

    case GoogleTiles:
        return "GoogleTiles";
        break;

    case TerrainTiles:
        return "TerrainTiles";
        break;

    case GrayTiles:
        return "GrayTiles";
        break;

    case SatTiles:
        return "SatTiles";
        break;

    default:
        return "UnknownTiles";
        break;
    }
}

QString OSMTileSource::tileFileExtension() const
{
    if (_tileType == OsmTiles || _tileType == TerrainTiles)
        return "png";
    else
        return "jpg";
}

//protected
void OSMTileSource::fetchTile(quint32 x, quint32 y, quint8 z)
{
    MapGraphicsNetwork * network = MapGraphicsNetwork::getInstance();

    QString host;
    QString url;

    /*OpenCycleMap
    https://{s}.tile.thunderforest.com/cycle/{z}/{x}/{y}.png?apikey=8ec9ff1a1a6649afa34575020ee19300
    Transport
    https://{s}.tile.thunderforest.com/transport/{z}/{x}/{y}.png?apikey=8ec9ff1a1a6649afa34575020ee19300
    Landscape
    https://{s}.tile.thunderforest.com/landscape/{z}/{x}/{y}.png?apikey=8ec9ff1a1a6649afa34575020ee19300
    Outdoors
    https://{s}.tile.thunderforest.com/outdoors/{z}/{x}/{y}.png?apikey=8ec9ff1a1a6649afa34575020ee19300
    Transport Dark
    https://{s}.tile.thunderforest.com/transport-dark/{z}/{x}/{y}.png?apikey=8ec9ff1a1a6649afa34575020ee19300
    Spinal Map
    https://{s}.tile.thunderforest.com/spinal-map/{z}/{x}/{y}.png?apikey=8ec9ff1a1a6649afa34575020ee19300
    Pioneer
    https://{s}.tile.thunderforest.com/pioneer/{z}/{x}/{y}.png?apikey=8ec9ff1a1a6649afa34575020ee19300
    Mobile Atlas
    https://{s}.tile.thunderforest.com/mobile-atlas/{z}/{x}/{y}.png?apikey=8ec9ff1a1a6649afa34575020ee19300
    Neighbourhood
    https://{s}.tile.thunderforest.com/neighbourhood/{z}/{x}/{y}.png?apikey=8ec9ff1a1a6649afa34575020ee19300*/

    //Figure out which server to request from based on our desired tile type
    if (_tileType == OsmTiles)
    {
        host = "http://tile.openstreetmap.org";
        url = "/%1/%2/%3.png";
    }
    else if (_tileType == GoogleTiles)
    {
        host = "https://mts1.google.com/vt";
        url = "/x=%2&y=%3&z=%1";
    }
    else if (_tileType == TerrainTiles)
    {
        host = "https://tile.thunderforest.com/outdoors";
        url = "/%1/%2/%3.png?apikey=8ec9ff1a1a6649afa34575020ee19300";
        //host = "https://a.tile.opentopomap.org";
        //url = "/%1/%2/%3.png";
        //host = "http://tile.mtbmap.cz/mtbmap_tiles";
        //url = "/%1/%2/%3.png";
    }
    else if (_tileType == GrayTiles)
    {
        host = "http://a.tiles.wmflabs.org/bw-mapnik";
        url = "/%1/%2/%3.png";
    }
    else if (_tileType == SatTiles)
    {
        host = "https://mts1.google.com/vt/lyrs=y&";
        url = "x=%2&y=%3&z=%1";
    }

    //Use the unique cacheID to see if this tile has already been requested
    const QString cacheID = this->createCacheID(x,y,z);
    if (_pendingRequests.contains(cacheID))
        return;
    _pendingRequests.insert(cacheID);

    //Build the request
    const QString fetchURL = url.arg(QString::number(z),
                                     QString::number(x),
                                     QString::number(y));
    QNetworkRequest request(QUrl(host + fetchURL));

    //Send the request and setupd a signal to ensure we're notified when it finishes
    QNetworkReply * reply = network->get(request);
    _pendingReplies.insert(reply,cacheID);

    connect(reply,
            SIGNAL(finished()),
            this,
            SLOT(handleNetworkRequestFinished()));
}

//private slot
void OSMTileSource::handleNetworkRequestFinished()
{
    QObject * sender = QObject::sender();
    QNetworkReply * reply = qobject_cast<QNetworkReply *>(sender);

    if (reply == 0)
    {
        qWarning() << "QNetworkReply cast failure";
        return;
    }

    /*
      We can do this here and use reply later in the function because the reply
      won't be deleted until execution returns to the event loop.
    */
    reply->deleteLater();

    if (!_pendingReplies.contains(reply))
    {
        qWarning() << "Unknown QNetworkReply";
        return;
    }

    //get the cacheID
    const QString cacheID = _pendingReplies.take(reply);
    _pendingRequests.remove(cacheID);

    //If there was a network error, ignore the reply
    if (reply->error() != QNetworkReply::NoError)
    {
        qDebug() << "Network Error:" << reply->errorString();
        return;
    }

    //Convert the cacheID back into x,y,z tile coordinates
    quint32 x,y,z;
    if (!MapTileSource::cacheID2xyz(cacheID,&x,&y,&z))
    {
        qWarning() << "Failed to convert cacheID" << cacheID << "back to xyz";
        return;
    }

    QByteArray bytes = reply->readAll();
    QImage * image = new QImage();

    if (!image->loadFromData(bytes))
    {
        delete image;
        qWarning() << "Failed to make QImage from network bytes";
        return;
    }

    //Figure out how long the tile should be cached
    QDateTime expireTime;
    if (reply->hasRawHeader("Cache-Control"))
    {
        //We support the max-age directive only for now
        const QByteArray cacheControl = reply->rawHeader("Cache-Control");
        QRegExp maxAgeFinder("max-age=(\\d+)");
        if (maxAgeFinder.indexIn(cacheControl) != -1)
        {
            bool ok = false;
            const qint64 delta = maxAgeFinder.cap(1).toULongLong(&ok);

            if (ok)
                expireTime = QDateTime::currentDateTimeUtc().addSecs(delta);
        }
    }

    //Notify client of tile retrieval
    this->prepareNewlyReceivedTile(x,y,z, image, expireTime);
}
