# XcMap
http://wiki.openstreetmap.org/wiki/Tiles

Tile map renderer Usb Gps Nmea parser
All source should compiled with qt4 (qmake -qt=4 && make)
Below you may find the map tile urls which i used.

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
    
    gps parsed values are:
    
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
