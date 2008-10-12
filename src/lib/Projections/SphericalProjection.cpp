//
// This file is part of the Marble Desktop Globe.
//
// This program is free software licensed under the GNU LGPL. You can
// find a copy of this license in LICENSE.txt in the top directory of
// the source code.
//
// Copyright 2007-2008 Inge Wallin  <ingwa@kde.org>
//

// Local
#include "SphericalProjection.h"

#include <QtCore/QDebug>

// Marble
#include "SphericalProjectionHelper.h"
#include "ViewportParams.h"
#include "GeoDataPoint.h"

namespace Marble
{

static SphericalProjectionHelper  theHelper;

class SphericalProjectionPrivate
{
 public:
    SphericalProjectionPrivate()
    {
    }

    ~SphericalProjectionPrivate()
    {
    }
};

SphericalProjection::SphericalProjection()
    : AbstractProjection(), 
      d( new SphericalProjectionPrivate() )
{
    m_maxLat  = 90.0 * DEG2RAD;
    m_minLat  = -90.0 * DEG2RAD;
    m_traversableMaxLat = true;
    m_repeatX = false;
}

SphericalProjection::~SphericalProjection()
{
    delete d;
}


AbstractProjectionHelper *SphericalProjection::helper()
{
    return &theHelper;
}


bool SphericalProjection::screenCoordinates( const qreal lon, const qreal lat,
                                             const ViewportParams *viewport,
                                             int& x, int& y )
{
    Quaternion  p( lon, lat );
    p.rotateAroundAxis( viewport->planetAxis().inverse() );
 
    x = (int)( viewport->width()  / 2 + (qreal)( viewport->radius() ) * p.v[Q_X] );
    y = (int)( viewport->height() / 2 - (qreal)( viewport->radius() ) * p.v[Q_Y] );
 
    return p.v[Q_Z] > 0;
}

bool SphericalProjection::screenCoordinates( const GeoDataCoordinates &coordinates, 
                                             const ViewportParams *viewport,
                                             int &x, int &y, bool &globeHidesPoint )
{
    qreal       absoluteAltitude = coordinates.altitude() + EARTH_RADIUS;
    Quaternion  qpos             = coordinates.quaternion();

    qpos.rotateAroundAxis( *( viewport->planetAxisMatrix() ) );

    qreal      pixelAltitude = ( ( viewport->radius() ) 
                                  / EARTH_RADIUS * absoluteAltitude );
    if ( coordinates.altitude() < 10000 ) {
        // Skip placemarks at the other side of the earth.
        if ( qpos.v[Q_Z] < 0 ) {
            globeHidesPoint = true;
            return false;
        }
    }
    else {
        qreal  earthCenteredX = pixelAltitude * qpos.v[Q_X];
        qreal  earthCenteredY = pixelAltitude * qpos.v[Q_Y];
        qreal  radius         = viewport->radius();

        // Don't draw high placemarks (e.g. satellites) that aren't visible.
        if ( qpos.v[Q_Z] < 0
             && ( ( earthCenteredX * earthCenteredX
                    + earthCenteredY * earthCenteredY )
                  < radius * radius ) ) {
            globeHidesPoint = true;
            return false;
        }
    }

    // Let (x, y) be the position on the screen of the placemark..
    x = (int)(viewport->width()  / 2 + pixelAltitude * qpos.v[Q_X]);
    y = (int)(viewport->height() / 2 - pixelAltitude * qpos.v[Q_Y]);

    // Skip placemarks that are outside the screen area
    if ( x < 0 || x >= viewport->width() || y < 0 || y >= viewport->height() ) {
        globeHidesPoint = false;
        return false;
    }

    globeHidesPoint = false;
    return true;
}

bool SphericalProjection::screenCoordinates( const GeoDataCoordinates &coordinates,
                                             const ViewportParams *viewport,
                                             int *x, int &y,
                                             int &pointRepeatNum,
                                             bool &globeHidesPoint )
{
    qreal       absoluteAltitude = coordinates.altitude() + EARTH_RADIUS;
    Quaternion  qpos             = coordinates.quaternion();

    qpos.rotateAroundAxis( *( viewport->planetAxisMatrix() ) );

    qreal      pixelAltitude = ( ( viewport->radius() ) 
                                  / EARTH_RADIUS * absoluteAltitude );
    if ( coordinates.altitude() < 10000 ) {
        // Skip placemarks at the other side of the earth.
        if ( qpos.v[Q_Z] < 0 ) {
            globeHidesPoint = true;
            return false;
        }
    }
    else {
        qreal  earthCenteredX = pixelAltitude * qpos.v[Q_X];
        qreal  earthCenteredY = pixelAltitude * qpos.v[Q_Y];
        qreal  radius         = viewport->radius();

        // Don't draw high placemarks (e.g. satellites) that aren't visible, 
        // because they are "behind" the earth
        if ( qpos.v[Q_Z] < 0
             && ( ( earthCenteredX * earthCenteredX
                    + earthCenteredY * earthCenteredY )
                  < radius * radius ) ) {
            globeHidesPoint = true;
            return false;
        }
    }

    // Let (x, y) be the position on the screen of the placemark..
    *x = (int)(viewport->width()  / 2 + pixelAltitude * qpos.v[Q_X]);
    y = (int)(viewport->height() / 2 - pixelAltitude * qpos.v[Q_Y]);

    // Skip placemarks that are outside the screen area
    if ( *x < 0 || *x >= viewport->width() 
         || y < 0 || y >= viewport->height() )
    {
        globeHidesPoint = false;
        return false;
    }

    // This projection doesn't have any repetitions, 
    // so the number of screen points referring to the geopoint is one.
    pointRepeatNum = 1;
    globeHidesPoint = false;
    return true;
}


bool SphericalProjection::screenCoordinates( const GeoDataLineString &lineString, 
                                    const ViewportParams *viewport,
                                    QVector<QPolygonF *> &polygons )
{
    int x, y;
    bool globeHidesPoint;

    int previousX, previousY;
    bool previousGlobeHidesPoint;
    bool previousIsVisible;
    GeoDataCoordinates previousCoords;

    QPolygonF  *polygon = new QPolygonF;
    
    GeoDataLineString::ConstIterator itCoords;
    GeoDataLineString::ConstIterator itEnd = lineString.constEnd();

    bool isVisible = false;

    for ( itCoords = lineString.constBegin(); itCoords != itEnd; ++itCoords )
    {
        isVisible = screenCoordinates( **itCoords, viewport, x, y, globeHidesPoint );

        // Initializing variables that store the values of the previous iteration
        if ( itCoords == lineString.constBegin() ) {
            previousGlobeHidesPoint = globeHidesPoint;
            previousIsVisible = isVisible;
            previousCoords = **itCoords;
            previousX = x;
            previousY = y;
        }

        // TODO: on flat maps we need to take the date line into account right here.

        // Adding interpolated nodes if the current or previous point is visible
        if ( isVisible || previousIsVisible ) {

            if ( globeHidesPoint || previousGlobeHidesPoint ) {
                // Add interpolated "horizon" nodes 

                // Assign the first or last horizon point to the current or
                // previous point, so that we still get correct results for 
                // the case where we need to geoproject the line segment.
            }

        }

        if ( lineString.tessellate() ) {
            // let the line segment follow the spherical surface
            // if the distance between the previous point and the current point 
            // on screen is too big

            qreal precision = 70.0;

            // We take the manhattan length as a distance approximation
            // that can be too big by a factor of sqrt(2)
            qreal distance =   fabs(x - previousX) + fabs(y - previousY);

            // FIXME: This is a work around: remove as soon as we handle horizon crossing
            if ( globeHidesPoint || previousGlobeHidesPoint ) {
                distance = 350;
            }

            qreal suggestedCount = (int)( distance / precision );

            const qreal safeDistance = -distance / 2.0;

            // Interpolate additional nodes if the current or previous nodes are visible
            // or if the line segment that connects them might cross the viewport.
            // The latter can pretty safely excluded for most projections if both points 
            // are located on the same side relative to the viewport boundaries and if they are 
            // located more than half the line segment distance away from the viewport.

            if (    isVisible || previousIsVisible
                 || !( x < safeDistance && previousX < safeDistance )
                 || !( y < safeDistance && previousY < safeDistance )
                 || !( x + safeDistance > viewport->width() 
                       && previousX + safeDistance > viewport->width() )
                 || !( y + safeDistance > viewport->height()
                       && previousY + safeDistance > viewport->height() )
            ){
                if ( distance > precision ) {
                    qDebug() << "Distance: " << distance;
                    *polygon << tessellateLineSegment( previousCoords, **itCoords, 
                                                      suggestedCount, viewport, lineString.tessellationFlags() ); 
                }
            }
        }

        if ( !globeHidesPoint ) {
            polygon->append( QPointF( x, y ) );
        }
        else {
            if ( !previousGlobeHidesPoint ) {
                polygons.append( polygon );
                polygon = new QPolygonF;
            }
        }

        previousGlobeHidesPoint = globeHidesPoint;
        previousIsVisible = isVisible;
        previousCoords = **itCoords;
        previousX = x;
        previousY = y;
    }

    if ( polygon->size() > 1 ){
        polygons.append( polygon );
    }
    else {
        delete polygon;
    }

    return polygons.isEmpty();
}

bool SphericalProjection::screenCoordinates( const GeoDataLinearRing &linearRing, 
                                    const ViewportParams *viewport,
                                    QVector<QPolygonF *> &polygons )
{
    int x, y;
    bool globeHidesPoint;

    int previousX, previousY;
    bool previousGlobeHidesPoint;
    bool previousIsVisible;
    GeoDataCoordinates previousCoords;

    QPolygonF  *polygon = new QPolygonF;
    
    GeoDataLineString::ConstIterator itCoords;
    GeoDataLineString::ConstIterator itEnd = linearRing.constEnd();

    bool isVisible = false;

    for ( itCoords = linearRing.constBegin(); itCoords != itEnd; ++itCoords )
    {
        isVisible = screenCoordinates( **itCoords, viewport, x, y, globeHidesPoint );

        // Initializing variables that store the values of the previous iteration
        if ( itCoords == linearRing.constBegin() ) {
            previousGlobeHidesPoint = globeHidesPoint;
            previousIsVisible = isVisible;
            previousCoords = **itCoords;
            previousX = x;
            previousY = y;
        }

        // TODO: on flat maps we need to take the date line into account right here.

        // Adding interpolated nodes if the current or previous point is visible
        if ( isVisible || previousIsVisible ) {

            if ( globeHidesPoint || previousGlobeHidesPoint ) {
                // Add interpolated "horizon" nodes 

                // Assign the first or last horizon point to the current or
                // previous point, so that we still get correct results for 
                // the case where we need to geoproject the line segment.
            }

        }

        if ( linearRing.tessellate() ) {
            // let the line segment follow the spherical surface
            // if the distance between the previous point and the current point 
            // on screen is too big

            qreal precision = 70.0;

            // We take the manhattan length as a distance approximation
            // that can be too big by a factor of sqrt(2)
            qreal distance =   fabs(x - previousX) + fabs(y - previousY);

            // FIXME: This is a work around: remove as soon as we handle horizon crossing
            if ( globeHidesPoint || previousGlobeHidesPoint ) {
                distance = 350;
            }

            qreal suggestedCount = (int)( distance / precision );

            const qreal safeDistance = -distance / 2.0;

            // Interpolate additional nodes if the current or previous nodes are visible
            // or if the line segment that connects them might cross the viewport.
            // The latter can pretty safely excluded for most projections if both points 
            // are located on the same side relative to the viewport boundaries and if they are 
            // located more than half the line segment distance away from the viewport.

            if (    isVisible || previousIsVisible
                 || !( x < safeDistance && previousX < safeDistance )
                 || !( y < safeDistance && previousY < safeDistance )
                 || !( x + safeDistance > viewport->width() 
                       && previousX + safeDistance > viewport->width() )
                 || !( y + safeDistance > viewport->height()
                       && previousY + safeDistance > viewport->height() )
            ){
                if ( distance > precision ) {
                    qDebug() << "Distance: " << distance;
                    *polygon << tessellateLineSegment( previousCoords, **itCoords, 
                                                      suggestedCount, viewport, linearRing.tessellationFlags() ); 
                }
            }
        }

        if ( !globeHidesPoint ) {
            polygon->append( QPointF( x, y ) );
        }
        else {
            if ( !previousGlobeHidesPoint ) {
                polygons.append( polygon );
                polygon = new QPolygonF;
            }
        }

        previousGlobeHidesPoint = globeHidesPoint;
        previousIsVisible = isVisible;
        previousCoords = **itCoords;
        previousX = x;
        previousY = y;
    }

    if ( polygon->size() > 1 ){
        polygons.append( polygon );
    }
    else {
        delete polygon;
    }

    return polygons.isEmpty();
}


bool SphericalProjection::geoCoordinates( int x, int y,
                                          const ViewportParams *viewport,
                                          qreal& lon, qreal& lat,
                                          GeoDataCoordinates::Unit unit )
{
    const qreal  inverseRadius = 1.0 / (qreal)(viewport->radius());
    bool          noerr         = false;

    qreal radius = (qreal)( viewport->radius() );
    qreal centerX = (qreal)( x - viewport->width() / 2 );
    qreal centerY = (qreal)( y - viewport->height() / 2 );

    if ( radius * radius > centerX * centerX + centerY * centerY ) {
        qreal qx = inverseRadius * +centerX;
        qreal qy = inverseRadius * -centerY;
        qreal qr = 1.0 - qy * qy;

        qreal qr2z = qr - qx * qx;
        qreal qz   = ( qr2z > 0.0 ) ? sqrt( qr2z ) : 0.0;

        Quaternion  qpos( 0.0, qx, qy, qz );
        qpos.rotateAroundAxis( viewport->planetAxis() );
        qpos.getSpherical( lon, lat );

        noerr = true;
    }

    if ( unit == GeoDataCoordinates::Degree ) {
        lon *= RAD2DEG;
        lat *= RAD2DEG;
    }

    return noerr;
}

GeoDataLatLonAltBox SphericalProjection::latLonAltBox( const QRect& screenRect,
                                                       const ViewportParams *viewport )
{
    // For the case where the whole viewport gets covered there is a 
    // pretty dirty and generic detection algorithm:
    GeoDataLatLonAltBox latLonAltBox = AbstractProjection::latLonAltBox( screenRect, viewport );

    // If the whole globe is visible we can easily calculate
    // analytically the lon-/lat- range.
    qreal pitch = GeoDataPoint::normalizeLat( viewport->planetAxis().pitch() );

    if ( 2 * viewport->radius() + 1 <= viewport->height()
         &&  2 * viewport->radius() + 1 <= viewport->width() )
    { 
        // Unless the planetaxis is in the screen plane the allowed longitude range
        // covers full -180 deg to +180 deg:

        if ( pitch > 0.0 && pitch < +M_PI ) {
            latLonAltBox.setWest(  -M_PI );
            latLonAltBox.setEast(  +M_PI );
            latLonAltBox.setNorth( +fabs( M_PI / 2.0 - fabs( pitch ) ) );
            latLonAltBox.setSouth( -M_PI / 2.0 );
        }
        if ( pitch < +0.0 || pitch < -M_PI ) {
            latLonAltBox.setWest(  -M_PI );
            latLonAltBox.setEast(  +M_PI );
            latLonAltBox.setNorth( +M_PI / 2.0 );
            latLonAltBox.setSouth( -fabs( M_PI / 2.0 - fabs( pitch ) ) );
        }

        // Last but not least we deal with the rare case where the
        // globe is fully visible and pitch = 0.0 or pitch = -M_PI or
        // pitch = +M_PI
        if ( pitch == 0.0 || pitch == -M_PI || pitch == +M_PI ) {
            qreal yaw = viewport->planetAxis().yaw();
            latLonAltBox.setWest( GeoDataPoint::normalizeLon( yaw - M_PI / 2.0 ) );
            latLonAltBox.setEast( GeoDataPoint::normalizeLon( yaw + M_PI / 2.0 ) );
        }
    }

    // Now we check whether maxLat (e.g. the north pole) gets displayed
    // inside the viewport to get more accurate values for east and west.

    // We need a point on the screen at maxLat that definetely gets displayed:
    qreal averageLongitude = ( latLonAltBox.west() + latLonAltBox.east() ) / 2.0;

    GeoDataCoordinates maxLatPoint( averageLongitude, m_maxLat, 0.0, GeoDataCoordinates::Radian );
    GeoDataCoordinates minLatPoint( averageLongitude, m_minLat, 0.0, GeoDataCoordinates::Radian );

    int dummyX, dummyY; // not needed
    bool dummyVal;

    if ( screenCoordinates( maxLatPoint, viewport, dummyX, dummyY, dummyVal ) ) {
        latLonAltBox.setWest( -M_PI );
        latLonAltBox.setEast( +M_PI );
    }
    if ( screenCoordinates( minLatPoint, viewport, dummyX, dummyY, dummyVal ) ) {
        latLonAltBox.setWest( -M_PI );
        latLonAltBox.setEast( +M_PI );
    }

//    qDebug() << latLonAltBox.text( GeoDataCoordinates::Degree );

    return latLonAltBox;
}


bool SphericalProjection::mapCoversViewport( const ViewportParams *viewport ) const
{
    qint64  radius = viewport->radius();
    qint64  width  = viewport->width();
    qint64  height = viewport->height();

    // This first test is a quick one that will catch all really big
    // radii and prevent overflow in the real test.
    if ( radius > width + height )
        return true;

    // This is the real test.  The 4 is because we are really
    // comparing to width/2 and height/2.
    if ( 4 * radius * radius >= width * width + height * height )
        return true;

    return false;
}

}
