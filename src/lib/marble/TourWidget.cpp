//
// This file is part of the Marble Virtual Globe.
//
// This program is free software licensed under the GNU LGPL. You can
// find a copy of this license in LICENSE.txt in the top directory of
// the source code.
//
// Copyright 2013 Mihail Ivchenko <ematirov@gmail>
//

// Self
#include "TourWidget.h"

// Qt
#include <QFileDialog>
#include <QDir>
#include <QModelIndex>

// Marble
#include "GeoDataDocument.h"
#include "GeoDataTour.h"
#include "GeoDataTreeModel.h"
#include "GeoDataTypes.h"
#include "MarbleModel.h"
#include "MarblePlacemarkModel.h"
#include "ParsingRunnerManager.h"
#include "MarbleWidget.h"

using namespace Marble;
// Ui
#include "ui_TourWidget.h"

namespace Marble
{

class TourWidgetPrivate
{

public:
    TourWidgetPrivate( TourWidget *parent );

public Q_SLOTS:
    void openFile();
    void mapCenterOn( QModelIndex index );

private:
    GeoDataTour* findTour( GeoDataFeature* feature ) const;

public:
    TourWidget *q;
    Ui::TourWidget  m_tourUi;
    MarbleWidget *m_widget;
    GeoDataTreeModel *m_model;
};

TourWidgetPrivate::TourWidgetPrivate( TourWidget *parent )
    :q( parent )
{
    m_tourUi.setupUi( parent );
    m_model = new GeoDataTreeModel();
    m_tourUi.m_listView->setModel( m_model );
    m_tourUi.m_listView->setModelColumn(1);
    QObject::connect( m_tourUi.m_openButton, SIGNAL( clicked() ), q, SLOT( openFile() ) );
    QObject::connect( m_tourUi.m_listView, SIGNAL( activated( QModelIndex ) ), q, SLOT( mapCenterOn( QModelIndex ) ) );
}

                  TourWidget::TourWidget( QWidget *parent, Qt::WindowFlags f )
                      : QWidget( parent, f ),
                        d( new TourWidgetPrivate( this ) )
{
    layout()->setMargin( 0 );
}

TourWidget::~TourWidget()
{
    delete d;
}

void TourWidget::setMarbleWidget( MarbleWidget *widget )
{
    d->m_widget = widget;
}

void TourWidgetPrivate::openFile()
{
    QString filename = QFileDialog::getOpenFileName( q, q->tr( "Open Tour" ), QDir::homePath(), q->tr( "Tour (*.kml)" ) );
    m_widget->model()->addGeoDataFile( filename );
    ParsingRunnerManager manager( m_widget->model()->pluginManager() );
    GeoDataDocument* document = manager.openFile( filename );
    if ( document ) {
        GeoDataTour *tour = findTour( document );
        m_model->addDocument( document );
        m_tourUi.m_listView->setRootIndex( m_model->index( tour->playlist() ) );
    }
}

GeoDataTour *TourWidgetPrivate::findTour( GeoDataFeature *feature ) const
{
    if ( feature && feature->nodeType() == GeoDataTypes::GeoDataTourType ) {
        return static_cast<GeoDataTour*>( feature );
    }

    GeoDataContainer *container = dynamic_cast<GeoDataContainer*>( feature );
    if ( container ) {
        QVector<GeoDataFeature*>::Iterator end = container->end();
        QVector<GeoDataFeature*>::Iterator iter = container->begin();
        for( ; iter != end; ++iter ) {
            GeoDataTour *tour = findTour( *iter );
            if ( tour ) {
                return tour;
            }
        }
    }
    return 0;
}

void TourWidgetPrivate::mapCenterOn(QModelIndex index)
{
    QVariant coordinatesVariant = m_model->data( index, MarblePlacemarkModel::CoordinateRole);
    if ( !coordinatesVariant.isNull() ) {
        GeoDataCoordinates coordinates = coordinatesVariant.value<GeoDataCoordinates>();
        m_widget->centerOn( coordinates );
    }
}

}

#include "TourWidget.moc"