//
// This file is part of the Marble Virtual Globe.
//
// This program is free software licensed under the GNU LGPL. You can
// find a copy of this license in LICENSE.txt in the top directory of
// the source code.
//
// Copyright 2008 Torsten Rahn <tackat@kde.org>
// Copyright 2009 Jens-Michael Hoffmann <jensmh@gmx.de>
// Copyright 2011,2012 Bernahrd Beschow <bbeschow@cs.tu-berlin.de>
//


// Own
#include "LayerManager.h"

// Local dir
#include "MarbleDebug.h"
#include "AbstractDataPlugin.h"
#include "AbstractDataPluginItem.h"
#include "AbstractFloatItem.h"
#include "GeoPainter.h"
#include "MarbleModel.h"
#include "PluginManager.h"
#include "RenderPlugin.h"
#include "LayerInterface.h"
#include "RenderState.h"

namespace Marble
{

class Q_DECL_HIDDEN LayerManager::Private
{
 public:
    Private( const MarbleModel* model, LayerManager *parent );
    ~Private();

    void updateVisibility( bool visible, const QString &nameId );

    void addPlugins();

    LayerManager *const q;

    QList<RenderPlugin *> m_renderPlugins;
    QList<AbstractFloatItem *> m_floatItems;
    QList<AbstractDataPlugin *> m_dataPlugins;
    QList<LayerInterface *> m_internalLayers;
    const MarbleModel* m_model;

    bool m_showBackground;

    RenderState m_renderState;
    bool m_showRuntimeTrace;
};

LayerManager::Private::Private( const MarbleModel* model, LayerManager *parent )
    : q( parent ),
      m_renderPlugins(),
      m_model( model ),
      m_showBackground( true ),
      m_showRuntimeTrace( false )
{
}

LayerManager::Private::~Private()
{
    qDeleteAll( m_renderPlugins );
}

void LayerManager::Private::updateVisibility( bool visible, const QString &nameId )
{
    emit q->visibilityChanged( nameId, visible );
}


LayerManager::LayerManager( const MarbleModel* model, QObject *parent )
    : QObject( parent ),
      d( new Private( model, this ) )
{
    d->addPlugins();
    connect( model->pluginManager(), SIGNAL(renderPluginsChanged()), this, SLOT(addPlugins()) );
}

LayerManager::~LayerManager()
{
    delete d;
}

bool LayerManager::showBackground() const
{
    return d->m_showBackground;
}

QList<RenderPlugin *> LayerManager::renderPlugins() const
{
    return d->m_renderPlugins;
}

QList<AbstractFloatItem *> LayerManager::floatItems() const
{
    return d->m_floatItems;
}

QList<AbstractDataPlugin *> LayerManager::dataPlugins() const
{
    return d->m_dataPlugins;
}

QList<AbstractDataPluginItem *> LayerManager::whichItemAt( const QPoint& curpos ) const
{
    QList<AbstractDataPluginItem *> itemList;

    foreach( auto *plugin, d->m_dataPlugins ) {
        itemList.append( plugin->whichItemAt( curpos ) );
    }
    return itemList;
}

void LayerManager::renderLayers( GeoPainter *painter, ViewportParams *viewport )
{
    d->m_renderState = RenderState( "Marble" );
    const QTime totalTime = QTime::currentTime();

    QStringList renderPositions;

    if ( d->m_showBackground ) {
        renderPositions << "STARS" << "BEHIND_TARGET";
    }

    renderPositions << "SURFACE" << "HOVERS_ABOVE_SURFACE" << "ATMOSPHERE"
                    << "ORBIT" << "ALWAYS_ON_TOP" << "FLOAT_ITEM" << "USER_TOOLS";

    QStringList traceList;
    foreach( const auto& renderPosition, renderPositions ) {
        QList<LayerInterface*> layers;

        // collect all RenderPlugins of current renderPosition
        foreach( auto *renderPlugin, d->m_renderPlugins ) {
            if ( renderPlugin && renderPlugin->renderPosition().contains( renderPosition ) ) {
                if ( renderPlugin->enabled() && renderPlugin->visible() ) {
                    if ( !renderPlugin->isInitialized() ) {
                        renderPlugin->initialize();
                        emit renderPluginInitialized( renderPlugin );
                    }
                    layers.push_back( renderPlugin );
                }
            }
        }

        // collect all internal LayerInterfaces of current renderPosition
        foreach( auto *layer, d->m_internalLayers ) {
            if ( layer && layer->renderPosition().contains( renderPosition ) ) {
                layers.push_back( layer );
            }
        }

        // sort them according to their zValue()s
        qSort( layers.begin(), layers.end(), [] ( const LayerInterface * const one, const LayerInterface * const two ) -> bool {
            Q_ASSERT( one && two );
            return one->zValue() < two->zValue();
        } );

        // render the layers of the current renderPosition
        QTime timer;
        foreach( auto *layer, layers ) {
            timer.start();
            layer->render( painter, viewport, renderPosition, 0 );
            d->m_renderState.addChild( layer->renderState() );
            traceList.append( QString("%2 ms %3").arg( timer.elapsed(),3 ).arg( layer->runtimeTrace() ) );
        }
    }

    if ( d->m_showRuntimeTrace ) {
        const int totalElapsed = totalTime.elapsed();
        const int fps = 1000.0/totalElapsed;
        traceList.append( QString( "Total: %1 ms (%2 fps)" ).arg( totalElapsed, 3 ).arg( fps ) );

        painter->save();
        painter->setBackgroundMode( Qt::OpaqueMode );
        painter->setBackground( Qt::gray );
        painter->setFont( QFont( QStringLiteral("Sans Serif"), 10, QFont::Bold ) );

        int i=0;
        int const top = 150;
        int const lineHeight = painter->fontMetrics().height();
        foreach ( const auto &text, traceList ) {
            painter->setPen( Qt::black );
            painter->drawText( QPoint(10,top+1+lineHeight*i), text );
            painter->setPen( Qt::white );
            painter->drawText( QPoint(9,top+lineHeight*i), text );
            ++i;
        }
        painter->restore();
    }
}

void LayerManager::Private::addPlugins()
{
    foreach ( const auto *factory, m_model->pluginManager()->renderPlugins() ) {
        bool alreadyCreated = false;
        foreach( const auto* existing, m_renderPlugins ) {
            if ( existing->nameId() == factory->nameId() ) {
                alreadyCreated = true;
                break;
            }
        }

        if ( alreadyCreated ) {
            continue;
        }

        RenderPlugin *const renderPlugin = factory->newInstance( m_model );
        Q_ASSERT( renderPlugin && "Plugin returned null when requesting a new instance." );
        m_renderPlugins.append( renderPlugin );

        QObject::connect( renderPlugin, SIGNAL(settingsChanged(QString)),
                 q, SIGNAL(pluginSettingsChanged()) );
        QObject::connect( renderPlugin, SIGNAL(repaintNeeded(QRegion)),
                 q, SIGNAL(repaintNeeded(QRegion)) );
        QObject::connect( renderPlugin, SIGNAL(visibilityChanged(bool,QString)),
                 q, SLOT(updateVisibility(bool,QString)) );

        // get float items ...
        AbstractFloatItem * const floatItem =
            qobject_cast<AbstractFloatItem *>( renderPlugin );
        if ( floatItem )
            m_floatItems.append( floatItem );

        // ... and data plugins
        AbstractDataPlugin * const dataPlugin =
            qobject_cast<AbstractDataPlugin *>( renderPlugin );
        if( dataPlugin )
            m_dataPlugins.append( dataPlugin );
    }
}

void LayerManager::setShowBackground( bool show )
{
    d->m_showBackground = show;
}

void LayerManager::setShowRuntimeTrace( bool show )
{
    d->m_showRuntimeTrace = show;
}

void LayerManager::addLayer(LayerInterface *layer)
{
    d->m_internalLayers.push_back(layer);
}

void LayerManager::removeLayer(LayerInterface *layer)
{
    d->m_internalLayers.removeAll(layer);
}

QList<LayerInterface *> LayerManager::internalLayers() const
{
    return d->m_internalLayers;
}

RenderState LayerManager::renderState() const
{
    return d->m_renderState;
}

}

#include "moc_LayerManager.cpp"
