/***************************************************************************
    oracleplugin.cpp  Access Oracle Spatial Plugin
    -------------------
    begin                : Oracle Spatial Plugin
    copyright            : (C) Ivan Lucena
    email                : ivan.lucena@pmldnet.com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsoracle_plugin.h"
#include "qgsselectgeoraster_ui.h"

static const QString sName = QObject::tr( "Oracle Spatial GeoRaster" );
static const QString sDescription = QObject::tr( "Access Oracle Spatial GeoRaster" );
static const QString sPluginVersion = QObject::tr( "Version 0.1" );
static const QgisPlugin::PLUGINTYPE sPluginType = QgisPlugin::UI;
static const QString sPluginIcon = ":/oracleplugin/oracleplugin.png";

//////////////////////////////////////////////////////////////////////
//
// THE FOLLOWING METHODS ARE MANDATORY FOR ALL PLUGINS
//
//////////////////////////////////////////////////////////////////////

/**
 * Constructor for the plugin. The plugin is passed a pointer
 * an interface object that provides access to exposed functions in QGIS.
 * @param theQGisInterface - Pointer to the QGIS interface object
 */
QgsOraclePlugin::QgsOraclePlugin( QgisInterface * theQgisInterface ) :
    QgisPlugin( sName, sDescription, sPluginVersion, sPluginType ),
    mQGisIface( theQgisInterface )
{
}

QgsOraclePlugin::~QgsOraclePlugin()
{

}

/*
 * Initialize the GUI interface for the plugin - this is only called once when the plugin is
 * added to the plugin registry in the QGIS application.
 */
void QgsOraclePlugin::initGui()
{

  // Create the action for tool
  mQActionPointer = new QAction( QIcon( ":/oracleplugin/oracleplugin.png" ), tr( "Add Oracle GeoRaster Layer..." ), this );
  // Set the what's this text
  mQActionPointer->setWhatsThis( tr( "Add a Oracle Spatial GeoRaster..." ) );
  // Connect the action to the run
  connect( mQActionPointer, SIGNAL( triggered() ), this, SLOT( run() ) );
  // Add the icon to the toolbar
  mQGisIface->addToolBarIcon( mQActionPointer );
  mQGisIface->insertAddLayerAction( mQActionPointer );

}
//method defined in interface

void QgsOraclePlugin::help()
{
  //implement me!
}

// Slot called when the menu item is triggered
// If you created more menu items / toolbar buttons in initiGui, you should
// create a separate handler for each action - this single run() method will
// not be enough

void QgsOraclePlugin::run()
{
  QgsOracleSelectGeoraster *myPluginGui = new QgsOracleSelectGeoraster( mQGisIface->mainWindow(), mQGisIface, QgisGui::ModalDialogFlags );
  myPluginGui->setAttribute( Qt::WA_DeleteOnClose );

  myPluginGui->show();
}

// Unload the plugin by cleaning up the GUI

void QgsOraclePlugin::unload()
{
  // remove the GUI
  mQGisIface->removeToolBarIcon( mQActionPointer );
  mQGisIface->removeAddLayerAction( mQActionPointer );
  delete mQActionPointer;
}


//////////////////////////////////////////////////////////////////////////
//
//
//  THE FOLLOWING CODE IS AUTOGENERATED BY THE PLUGIN BUILDER SCRIPT
//    YOU WOULD NORMALLY NOT NEED TO MODIFY THIS, AND YOUR PLUGIN
//      MAY NOT WORK PROPERLY IF YOU MODIFY THIS INCORRECTLY
//
//
//////////////////////////////////////////////////////////////////////////


/**
 * Required extern functions needed  for every plugin
 * These functions can be called prior to creating an instance
 * of the plugin class
 */
// Class factory to return a new instance of the plugin class

QGISEXTERN QgisPlugin * classFactory( QgisInterface * theQgisInterfacePointer )
{
  return new QgsOraclePlugin( theQgisInterfacePointer );
}
// Return the name of the plugin - note that we do not user class members as
// the class may not yet be insantiated when this method is called.

QGISEXTERN QString name()
{
  return sName;
}

// Return the description

QGISEXTERN QString description()
{
  return sDescription;
}

// Return the type (either UI or MapLayer plugin)

QGISEXTERN int type()
{
  return sPluginType;
}

// Return the version number for the plugin

QGISEXTERN QString version()
{
  return sPluginVersion;
}

QGISEXTERN QString icon()
{
  return sPluginIcon;
}

// Delete ourself

QGISEXTERN void unload( QgisPlugin * thePluginPointer )
{
  delete thePluginPointer;
}
