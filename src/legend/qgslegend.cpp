
/***************************************************************************
                          qgslegend.cpp  -  description
                             -------------------
    begin                : Sun Jul 28 2002
    copyright            : (C) 2002 by Gary E.Sherman
    email                : sherman at mrcc dot com
               Romans 3:23=>Romans 6:23=>Romans 10:9,10=>Romans 12
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/* $Id$ */

#include "qgisapp.h"
#include "qgslegend.h"
#include "qgslegendgroup.h"
#include "qgslegendlayer.h"
#include "qgslegendpropertygroup.h"
#include "qgslegendsymbologygroup.h"
#include "qgslegendlayerfile.h"
#include "qgslegendlayerfilegroup.h"
#include "qgsmapcanvas.h"
#include "qgsmaplayer.h"
#include "qgsmaplayerregistry.h"
#include "qgsproject.h"
#include "qgsrasterlayerproperties.h"
#include <QCoreApplication>
#include <QPixmap>
#include <QMouseEvent>
#include <iostream>
#include <QTreeWidgetItem>
#include <Q3PopupMenu>

static const char *const ident_ = "$Id$";

const int AUTOSCROLL_MARGIN = 16;

/**
   @note
 
   set mItemBeingMoved pointer to 0 to prevent SuSE 9.0 crash
*/
QgsLegend::QgsLegend(QgisApp* app, QWidget * parent, const char *name)
    : QTreeWidget(parent), mApp(app), mMousePressedFlag(false), mItemBeingMoved(0), mMapCanvas(0)
{
  connect( this, SIGNAL(selectionChanged(QTreeWidgetItem *)),
           this, SLOT(updateLegendItem(QTreeWidgetItem *)) );
  
  connect( this, SIGNAL(itemChanged(QTreeWidgetItem*, int)),
	   this, SLOT(handleItemChange(QTreeWidgetItem*, int)));

  setSortingEnabled(false);
  setDragEnabled(false);
  setAutoScroll(true);
}



QgsLegend::~QgsLegend()
{}

void QgsLegend::addGroup()
{
    QgsLegendGroup* group = new QgsLegendGroup(this, tr("group"));
    setExpanded(indexFromItem(group), true);
}

void QgsLegend::updateLegendItem( QTreeWidgetItem * li )
{
  QgsLegendItem * qli = dynamic_cast<QgsLegendItem*>(li);

  if ( ! qli )
  {
    qDebug( "QgsLegend::updateLegendItem(): couldn't get QgsLegendItem" );
    return;
  }

} 

void QgsLegend::removeAll()
{
  mStateOfCheckBoxes.clear();
  clear();
}

void QgsLegend::removeLayer(QString layer_key)
{
  QTreeWidgetItem* theItem = firstItem();
#ifdef QGISDEBUG
  qWarning("in QgsLegend::removeLayer");
#endif
  while(theItem)
    {
	QgsLegendItem *li = dynamic_cast<QgsLegendItem*>(theItem);
	if(li)
	{
	    QgsLegendLayerFile* llf = dynamic_cast<QgsLegendLayerFile*>(li);
	    if(llf)
	    {
		if (llf->layer()&&llf->layer()->getLayerID() == layer_key)
		{
		  //remove the map entry for the checkbox
		  mStateOfCheckBoxes.erase(llf);
		  removeItem(llf);
		  break;
		}
	    }
	}
	theItem = nextItem(theItem);
    }
    //update the overview canvas
    mApp->setOverviewZOrder(this);
}

void QgsLegend::mousePressEvent(QMouseEvent * e)
{
  if (e->button() == Qt::LeftButton)
  {
    mLastPressPos = e->pos();
    mMousePressedFlag = true;
  }
  else if(e->button() == Qt::RightButton)
    {
      QTreeWidgetItem* item = itemAt(e->pos());
      setCurrentItem(item);
      handleRightClickEvent(item, e->globalPos());
    }
  QTreeWidget::mousePressEvent(e);
}                               // contentsMousePressEvent

void QgsLegend::mouseMoveEvent(QMouseEvent * e)
{
    if(mMousePressedFlag)
    {
	//set the flag back such that the else if(mItemBeingMoved)
	//code part is passed during the next mouse moves
	mMousePressedFlag = false;

	// remember item we've pressed as the one being moved
	// and where it was originally
	//QTreeWidgetItem* item = itemAt(contentsToViewport(mLastPressPos));
	QTreeWidgetItem* item = itemAt(mLastPressPos);
	if(item)
	{
	    mItemBeingMoved = item;
	    mItemBeingMovedOrigPos = getItemPos(mItemBeingMoved);

	    //store information to insert the item back to the original position
	    storeInitialPosition(mItemBeingMoved);
 
	    setCursor(Qt::SizeVerCursor);
	}
    }
    else if (mItemBeingMoved)
    { 
      QPoint p(e->pos());
      mLastPressPos=p;
     
      // change the cursor appropriate to if drop is allowed
      QTreeWidgetItem* item = itemAt(p);
      QgsLegendItem* origin = dynamic_cast<QgsLegendItem*>(mItemBeingMoved);
      QgsLegendItem* dest = dynamic_cast<QgsLegendItem*>(item);

      if (item && (item != mItemBeingMoved))
	{
	  QgsLegendItem::DRAG_ACTION action = dest->accept(origin);
	  if(action == QgsLegendItem::REORDER)
	    {
#ifdef QGISDEBUG
	      qWarning("mouseMoveEvent::REORDER");
#endif
	      if(!yCoordAboveCenter(dest, e->y())) //over bottom of item
		{
		  if(origin->nextSibling() != dest)
		    {
		      if(origin->parent() != dest->parent())
			{
			  dest->parent()->insertChild(dest->parent()->childCount(), origin);
			  origin->moveItem(dest);
			  dest->moveItem(origin);
		      }
		      else
		      {
			  dest->moveItem(origin);
		      }
		  }
	      }
	      else //over top of item
	      {
		  if (mItemBeingMoved != dest->nextSibling())
		  {
		      origin->moveItem(dest);
		  } 
	      }
	      setCurrentItem(origin);
	    }
	    else if(action == QgsLegendItem::INSERT)
	    {
#ifdef QGISDEBUG
	      qWarning("mouseMoveEvent::INSERT");
#endif
	      setCursor( QCursor(Qt::PointingHandCursor) );
	      if(origin->parent() != dest)
		{
		  removeItem(origin);
		  dest->insert(origin, false);
		  setCurrentItem(origin);
		}
	    }
	    else//no action
	    {
#ifdef QGISDEBUG
	      qWarning("mouseMoveEvent::NO_ACTION");
#endif
	      if(mItemBeingMovedOrigPos != getItemPos(mItemBeingMoved))
		{
		  resetToInitialPosition(mItemBeingMoved);
		}
	      setCursor( QCursor(Qt::ForbiddenCursor) );
	    }
	}     
    }
    //QTreeWidget::mouseMoveEvent(e);
}

void QgsLegend::mouseReleaseEvent(QMouseEvent * e)
{
  QTreeWidget::mouseReleaseEvent(e);
  setCursor(QCursor(Qt::ArrowCursor));

  if (mItemBeingMoved)
  {
      QTreeWidgetItem *destItem = itemAt(e->pos());
      
      QgsLegendItem* origin = dynamic_cast<QgsLegendItem*>(mItemBeingMoved);
      QgsLegendItem* dest = dynamic_cast<QgsLegendItem*>(destItem);

      if(!dest || !origin)
      {
	  return;
      }

      if(dest && origin && getItemPos(dest) != mItemBeingMovedOrigPos)
      {
	QgsLegendItem::LEGEND_ITEM_TYPE originType = origin->type();
	QgsLegendItem::LEGEND_ITEM_TYPE destType = dest->type();

	if(originType == QgsLegendItem::LEGEND_LAYER_FILE && destType == QgsLegendItem::LEGEND_LAYER_FILE_GROUP)
	  {
	    QgsMapLayer* origLayer = ((QgsLegendLayerFile*)(origin))->layer();
	    
	    if(origLayer->legendSymbologyGroupParent() != dest->nextSibling())
	      {
		origLayer->setLegendSymbologyGroupParent((QgsLegendSymbologyGroup*)(dest->nextSibling()));
		if(dest->childCount() > 1)
		  {
		    //find the first layer in the legend layer group != origLayer and copy its settings
		    QgsLegendItem* currentItem = dynamic_cast<QgsLegendItem*>(dest->child(0));
		    while(currentItem)
		      {
			if(currentItem != origin)
			  {
			    QgsMapLayer* origLayer = ((QgsLegendLayerFile*)(origin))->layer();
			    QgsMapLayer* currentLayer = ((QgsLegendLayerFile*)(currentItem))->layer();
			    origLayer->copySymbologySettings(*currentLayer);
			    break;
			  }
			currentItem = currentItem->nextSibling();
		      }
		  }
	      }
	  }
	else if(originType == QgsLegendItem::LEGEND_LAYER_FILE && destType == QgsLegendItem::LEGEND_LAYER_FILE)
	  {
	    QgsMapLayer* origLayer = ((QgsLegendLayerFile*)(origin))->layer();
	    QgsMapLayer* destLayer = ((QgsLegendLayerFile*)(dest))->layer();
	    if(dest == origin)//origin item has been moved in mouseMoveEvent such that it is under the mouse cursor now
	      {
		if(origLayer->legendSymbologyGroupParent() != origin->parent())
		  {
		    origLayer->setLegendSymbologyGroupParent((QgsLegendSymbologyGroup*)(dynamic_cast<QgsLegendItem*>(origin->parent())->nextSibling()));
		    if(origin->parent()->childCount() > 1)
		      {
			//find the first layer in the legend layer group != origLayer and copy its settings
			QTreeWidgetItem* currentItem = dest->parent()->child(0);
			while(currentItem)
			  {
			    if(currentItem != origin)
			      {
				QgsMapLayer* origLayer = ((QgsLegendLayerFile*)(origin))->layer();
				QgsMapLayer* currentLayer = ((QgsLegendLayerFile*)(currentItem))->layer();
				origLayer->copySymbologySettings(*currentLayer);
				break;
			      }
			    currentItem = dynamic_cast<QgsLegendItem*>(currentItem)->nextSibling();
			  }
		      }
		  }
	      }
	    else
	      {
		QgsMapLayer* origLayer = ((QgsLegendLayerFile*)(origin))->layer();
		QgsMapLayer* destLayer = ((QgsLegendLayerFile*)(dest))->layer();
		origLayer->copySymbologySettings(*destLayer);
		origLayer->setLegendSymbologyGroupParent((QgsLegendSymbologyGroup*)(dynamic_cast<QgsLegendItem*>(dest->parent())->nextSibling()));
	      }
	  }
	emit zOrderChanged(this);
      }
  }
  mMousePressedFlag = false;
  mItemBeingMoved = NULL;
}

void QgsLegend::mouseDoubleClickEvent(QMouseEvent* e)
{
    QgsLegendItem* li = dynamic_cast<QgsLegendItem*>(currentItem());
    QgsMapLayer* ml = 0;

    if(li)
    {
	if(li->type() == QgsLegendItem::LEGEND_LAYER_FILE)
	{
	  QgsLegendLayerFile* llf = dynamic_cast<QgsLegendLayerFile*>(li);
	  ml = llf->layer();
	}
	else if(li->type() == QgsLegendItem::LEGEND_LAYER)
	{
	  QgsLegendLayer* ll = dynamic_cast<QgsLegendLayer*>(li);
	  ml = ll->firstMapLayer();
	}
       
	if (ml && ml->type() == QgsMapLayer::RASTER)
	  {
	    QgsRasterLayerProperties *rlp = new QgsRasterLayerProperties(ml);
	    if (rlp->exec())
	      {
		delete rlp;
		QCoreApplication::processEvents();
	      }
	  }
	else if(ml) //vector
	  {
	    ml->showLayerProperties();
	  }
    }
}

void QgsLegend::handleRightClickEvent(QTreeWidgetItem* item, const QPoint& position)
{
#ifdef QGISDEBUG
  qWarning("in QgsLegend::handleRightClickEvent");
#endif
    if(!mMapCanvas->isDrawing())
    {
#if defined(Q_OS_MACX) || defined(WIN32)
    QString iconsPath(QCoreApplication::applicationDirPath()+QString("/share/qgis"));
#else
    QString iconsPath(PKGDATAPATH);
#endif
    iconsPath += QString("/images/icons/");
	if(!item)//show list view popup menu
	{
	    Q3PopupMenu pm;
	    pm.insertItem(QIcon(QPixmap(iconsPath+QString("folder_new.png"))), tr("&Add group"), this, SLOT(addGroup()));
	    pm.insertItem(QIcon(QPixmap(iconsPath+QString("expand_tree.png"))), tr("&Expand all"), this, SLOT(expandAll()));
	    pm.insertItem(QIcon(QPixmap(iconsPath+QString("collapse_tree.png"))), tr("&Collapse all"), this, SLOT(collapseAll()));
	    pm.exec(position);
	    return;
	}
	QgsLegendItem* li = dynamic_cast<QgsLegendItem*>(item);
	if(li)
	{
	    if(li->type() == QgsLegendItem::LEGEND_LAYER_FILE)
	    {
		QgsLegendLayerFile* llf = dynamic_cast<QgsLegendLayerFile*>(li);
		if(llf)
		{
		   QgsMapLayer* ml = llf->layer();
		   if(ml)
		   {
		       Q3PopupMenu *mPopupMenu = ml->contextMenu();
		       if (mPopupMenu)
		       {
			   mPopupMenu->exec(position);
		       }
		   }
		}
	    }
	    else if(li->type() == QgsLegendItem::LEGEND_LAYER)
	    {
		Q3PopupMenu pm;
		pm.insertItem(QIcon(QPixmap(iconsPath+QString("remove.png"))), tr("&Remove"), this, SLOT(legendLayerRemove()));
		pm.insertItem(tr("Re&name"), this, SLOT(openEditor()));
		pm.insertItem(tr("&Properties"), this, SLOT(legendLayerShowProperties()));
		pm.insertItem(QIcon(QPixmap(iconsPath+QString("inoverview.png"))), tr("&Add to overview"), this, SLOT(legendLayerAddToOverview()));
		pm.insertItem(QIcon(QPixmap(iconsPath+QString("remove_from_overview.png"))), tr("&Remove from overview"), this, SLOT(legendLayerRemoveFromOverview()));
		pm.insertItem(QIcon(QPixmap(iconsPath+QString("folder_new.png"))), tr("&Add group"), this, SLOT(addGroup()));
		pm.insertItem(QIcon(QPixmap(iconsPath+QString("expand_tree.png"))), tr("&Expand all"), this, SLOT(expandAll()));
		pm.insertItem(QIcon(QPixmap(iconsPath+QString("collapse_tree.png"))), tr("&Collapse all"), this, SLOT(collapseAll()));
		pm.exec(position);
	    }
	    else if(li->type() == QgsLegendItem::LEGEND_GROUP)
	    {
		Q3PopupMenu pm;
		pm.insertItem(tr("&Remove"), this, SLOT(legendGroupRemove()));
		pm.insertItem(tr("Re&name"), this, SLOT(openEditor()));
		pm.insertItem(QIcon(QPixmap(iconsPath+QString("folder_new.png"))), tr("&Add group"), this, SLOT(addGroup()));
		pm.insertItem(QIcon(QPixmap(iconsPath+QString("expand_tree.png"))), tr("&Expand all"), this, SLOT(expandAll()));
		pm.insertItem(QIcon(QPixmap(iconsPath+QString("collapse_tree.png"))), tr("&Collapse all"), this, SLOT(collapseAll()));
		pm.exec(position);
	    }
	}
    }
}

int QgsLegend::getItemPos(QTreeWidgetItem* item)
{
  int counter = 1;
  QTreeWidgetItem* theItem = firstItem();
  while(theItem)
    {
      if(theItem == item)
	{
	  return counter;
	}
      theItem = nextItem(theItem);
      ++counter;
    }
  return -1;
}

void QgsLegend::addLayer( QgsMapLayer * layer )
{
    QgsLegendLayer * llayer = new QgsLegendLayer(QString(layer->name()));
    mStateOfCheckBoxes.insert(std::make_pair(llayer, Qt::Checked)); //insert the check state into the map to query for changes later
    QgsLegendLayerFileGroup * llfgroup = new QgsLegendLayerFileGroup(llayer,QString("Files"));
    QgsLegendLayerFile * llfile = new QgsLegendLayerFile(llfgroup, QgsLegendLayerFile::nameFromLayer(layer), layer);
    mStateOfCheckBoxes.insert(std::make_pair(llfile, Qt::Checked)); //insert the check state into the map to query for changes later
    QgsLegendSymbologyGroup * lsgroup = new QgsLegendSymbologyGroup(llayer,QString("Symbology"));
    layer->setLegendSymbologyGroupParent(lsgroup);
    QgsLegendPropertyGroup * lpgroup = new QgsLegendPropertyGroup(llayer,QString("Properties"));
    layer->setLegendLayerFile(llfile);
    layer->initContextMenu(mApp);

    insertTopLevelItem(0, llayer);
    
    setExpanded(indexFromItem(llayer), false);
    setExpanded(indexFromItem(lpgroup), true);
    setExpanded(indexFromItem(lsgroup), true);
    setExpanded(indexFromItem(llfgroup), true);
}

QgsMapLayer* QgsLegend::currentLayer()
{
    QgsLegendItem* citem=dynamic_cast<QgsLegendItem*>(currentItem());
    if(citem)
    {
	QgsLegendLayerFile* llf=dynamic_cast<QgsLegendLayerFile*>(citem);
	if(llf)
	{
	    return llf->layer();
	}
	else
	{
	    QgsLegendLayer* ll = dynamic_cast<QgsLegendLayer*>(citem);
	    if(ll)
	    {
		return ll->firstMapLayer();
	    }
	    else
	    {
		return 0;
	    }
	}
    }
    else
    {
	return 0;
    }
}

void QgsLegend::legendGroupRemove()
{
    QgsLegendGroup* lg = dynamic_cast<QgsLegendGroup*>(currentItem());
    if(lg)
    {
	//delete the legend layers first
	QTreeWidgetItem * child = lg->child(0);
        while(child) 
	{
	    setCurrentItem(child);
	    legendLayerRemove();
            child = lg->child(0);
        }
	delete lg;
    }
}

void QgsLegend::legendLayerRemove()
{
    //remove all layers of the current legendLayer
   QgsLegendLayer* ll = dynamic_cast<QgsLegendLayer*>(currentItem());
   if(!ll)
   {
       return;
   }

   std::list<QgsMapLayer*> maplayers = ll->mapLayers();
   mStateOfCheckBoxes.erase(ll);

   //todo: also remove the entries for the QgsLegendLayerFiles from the map
   std::list<QgsLegendLayerFile*> llfiles = ll->legendLayerFiles();
   for(std::list<QgsLegendLayerFile*>::iterator it = llfiles.begin(); it != llfiles.end(); ++it)
     {
       mStateOfCheckBoxes.erase(*it);
     }

   for(std::list<QgsMapLayer*>::iterator it = maplayers.begin(); it!=maplayers.end(); ++it)
   {
       //remove the layer
       if(*it)
       {
	   QgsMapLayerRegistry::instance()->removeMapLayer((*it)->getLayerID());
       }
   }

   if(maplayers.size()>0)
   {
     mMapCanvas->removeDigitizingLines();
     mMapCanvas->clear();
// For Qt4, deprecate direct calling of render().  Let render() be called by the 
// paint event loop of the map canvas widget.
//     mMapCanvas->render();
     mMapCanvas->update();
   }
   removeItem(ll);
   //update the overview canvas
   mApp->setOverviewZOrder(this);
}

void QgsLegend::legendLayerAddToOverview()
{
   //add or remove all layers to/ from overview
   QgsLegendLayer* ll = dynamic_cast<QgsLegendLayer*>(currentItem());
   if(!ll)
   {
       return;
   }

   std::list<QgsMapLayer*> maplayers = ll->mapLayers();
   for(std::list<QgsMapLayer*>::iterator it = maplayers.begin(); it!=maplayers.end(); ++it)
   {
       if(*it)
       {
	       (*it)->inOverview(true);
       }
   }

   if(maplayers.size()>0)
   {
       //update the overview canvas
       mApp->setOverviewZOrder(this);
   } 
}

void QgsLegend::legendLayerRemoveFromOverview()
{
    //add or remove all layers to/ from overview
   QgsLegendLayer* ll = dynamic_cast<QgsLegendLayer*>(currentItem());
   if(!ll)
   {
       return;
   }
   std::list<QgsMapLayer*> maplayers = ll->mapLayers();

   for(std::list<QgsMapLayer*>::iterator it = maplayers.begin(); it!=maplayers.end(); ++it)
   {
       if(*it)
       {
	   (*it)->inOverview(false); //else
       }
   }

   if(maplayers.size()>0)
   {
       //update the overview canvas
       mApp->setOverviewZOrder(this);
   } 
}

void QgsLegend::legendLayerShowProperties()
{
   QgsLegendItem* citem=dynamic_cast<QgsLegendItem*>(currentItem());
    if(!citem)
    {
	return;
    }
    QgsLegendLayer* ll = dynamic_cast<QgsLegendLayer*>(citem);
    if(!ll)
    {
	return;
    }
    QgsMapLayer* ml = ll->firstMapLayer(); 
    if(!ml)
    {
	return;
    }
    ml->showLayerProperties();
}

void QgsLegend::expandAll()
{
    QTreeWidgetItem* theItem = firstItem();
    while(theItem)
      {
	setExpanded(indexFromItem(theItem), true);
	theItem = nextItem(theItem);
      }
}

void QgsLegend::collapseAll()
{
  QTreeWidgetItem* theItem = firstItem();
    while(theItem)
      {
	setExpanded(indexFromItem(theItem), false);
	theItem = nextItem(theItem);
      }
}

bool QgsLegend::writeXML( QDomNode & layer_node, QDomDocument & document )
{
    QDomElement legendnode = document.createElement("legend");
    layer_node.appendChild(legendnode);

    QDomElement tmplegendnode = legendnode; /*copy of the legendnode*/
    QDomElement legendgroupnode;
    QDomElement legendlayernode;
    QDomElement layerfilegroupnode;
    QDomElement legendsymbolnode;
    QDomElement legendpropertynode;
    QDomElement legendlayerfilenode;
    QgsLegendLayerFile* llf;

    QTreeWidgetItem* currentItem = firstItem();
    while(currentItem) 
    {
	QgsLegendItem *item = dynamic_cast<QgsLegendItem*>(currentItem);
	if(item)
	{
	    switch(item->type())
	      {
	        case QgsLegendItem::LEGEND_GROUP:
		//make sure the legendnode is 'legend' again after a legend group
		if(!(item->parent()))
		    {
			legendnode = tmplegendnode;
		    }
		    legendgroupnode = document.createElement("legendgroup");
		    if(isItemExpanded(item))
		    {
			legendgroupnode.setAttribute("open","true");
		    }
		    else
		    {
			legendgroupnode.setAttribute("open","false");
		    }
		    legendgroupnode.setAttribute("name",item->text(0));
		    legendnode.appendChild(legendgroupnode);
		    tmplegendnode =  legendnode;
		    legendnode = legendgroupnode;
		    break;

		case QgsLegendItem::LEGEND_LAYER:
		    //make sure the legendnode is 'legend' again after a legend group
		    if(!(item->parent()))
		    {
			legendnode = tmplegendnode;
		    }
		    legendlayernode = document.createElement("legendlayer");
		    if(isItemExpanded(item))
		    {
			legendlayernode.setAttribute("open","true");
		    }
		    else
		    {
			legendlayernode.setAttribute("open","false");
		    }
		    legendlayernode.setAttribute("name", item->text(0));
		    legendnode.appendChild(legendlayernode);
		    break;

		case QgsLegendItem::LEGEND_PROPERTY_GROUP:
		    legendpropertynode = document.createElement("propertygroup");
		    if(isItemExpanded(item))
		    {
			legendpropertynode.setAttribute("open","true");	
		    }
		    else
		    {
			legendpropertynode.setAttribute("open","false");
		    }
		    legendlayernode.appendChild(legendpropertynode);
		    break;

		case QgsLegendItem::LEGEND_SYMBOL_GROUP:
		    legendsymbolnode = document.createElement("symbolgroup");
		    if(isItemExpanded(item))
		    {
			legendsymbolnode.setAttribute("open", "true");
		    }
		    else
		    {
			legendsymbolnode.setAttribute("open", "false");
		    }
		    legendlayernode.appendChild(legendsymbolnode);
		    break;
		
  
		case QgsLegendItem::LEGEND_LAYER_FILE_GROUP:
		    layerfilegroupnode = document.createElement("filegroup");
		    if(isItemExpanded(item))
		    {
			layerfilegroupnode.setAttribute("open", "true");
		    }
		    else
		    {
			layerfilegroupnode.setAttribute("open", "false");
		    }
		    legendlayernode.appendChild(layerfilegroupnode);
		    break;
	    
	      case QgsLegendItem::LEGEND_LAYER_FILE:
		legendlayerfilenode = document.createElement("legendlayerfile");
		llf = dynamic_cast<QgsLegendLayerFile*>(item);
		if(llf)
		  {
		    legendlayerfilenode.setAttribute("layerid", llf->layer()->getLayerID());
		    layerfilegroupnode.appendChild(legendlayerfilenode);
		  }
		break;

		default: //do nothing for the leaf nodes
		    break;
	    }
	}
	currentItem = nextItem(currentItem);
    }
    return true;
}

bool QgsLegend::readXML(QDomNode& legendnode)
{
    QString open;
    QgsLegendItem* theItem = dynamic_cast<QgsLegendItem*>(topLevelItem(0)); //first level hierarchy items
    QgsLegendItem* prevchild = 0; //store last value of theItem because of legend group
    QgsLegendItem* secondLevelItem = 0; //second level item
    QgsLegendGroup* group = 0; //pointer to the last inserted legend group

    QDomNode child = legendnode.firstChild();
    if(!child.isNull())
    {
	do //iterate over legend layers/ legend groups
	{
	  if(!theItem)
	    {
	      break;
	    }
	    
	    QDomElement legendlayerelem = child.toElement();
	    
	    if(legendlayerelem.tagName()=="legendgroup")
	    {
		group = new QgsLegendGroup(legendlayerelem.attribute("name")); 
		insertTopLevelItem(0, group);
		open = legendlayerelem.attribute("open");
		if(open == "true")
		{
		    expandItem(group);
		}
		if(prevchild)
		  {
		    moveItem(group, prevchild);
		  }
		theItem = group->nextSibling();
		if(!child.firstChild().isNull())
		  {
		    child = child.firstChild();//go one hierarchy step down
		  }
		else
		  {
		    child = child.nextSibling();
		  }
		continue;
	    }

	    open = legendlayerelem.attribute("open");
	    if(child.parentNode().toElement().tagName()=="legendgroup")
	    {
	      removeItem(theItem);
	      group->insertChild(0, theItem);
	      QgsLegendItem* currentChild = group->firstChild();
	      if(!currentChild)
		{
		  removeItem(theItem);
		  group->insertChild(0, theItem);//insert the first child in the group
		}
	      else //find the last child and insert the new one after it
		{
		  removeItem(theItem);
		  group->addChild(theItem);
		}
	    }
	    if(open == "true")
	    {
		expandItem(theItem);
	    }
	    theItem->setText(0, legendlayerelem.attribute("name"));

	    //file group
	    secondLevelItem = theItem->firstChild();
	    QDomNode filegroupnode = child.firstChild();
	    QDomElement filegroupelem = filegroupnode.toElement();
	    open = filegroupelem.attribute("open");
	    if(open == "true")
	    {
		expandItem(secondLevelItem);
	    }
	    QDomNode layerfilenode = filegroupnode.firstChild();
	    QDomElement layerfileelem = layerfilenode.toElement();
     
	    //remove the existing legendlayerfile and insert the one(s) according to the entries in the XML file
	    if(secondLevelItem->child(0))
	      {
		QgsLegendLayerFile* llfdelete = dynamic_cast<QgsLegendLayerFile*>(secondLevelItem->child(0));
		if(llfdelete)
		  {
		    removeItem(llfdelete);
		    mStateOfCheckBoxes.erase(llfdelete);
		  }
	      }
	    
	    //if there are several legend layer files in this group, create the additional items
	    std::map<QString,QgsMapLayer*> layers = QgsMapLayerRegistry::instance()->mapLayers();
	    while(true)
	      {
		if(layerfilenode.isNull())
		  {
		    break;
		  }
		
		std::map<QString,QgsMapLayer*>::iterator it = layers.find(layerfileelem.attribute("layerid"));
		if(it != layers.end())
		  {
		    QgsMapLayer* newlayer = it->second;
		    QgsLegendLayerFile* newfile = new QgsLegendLayerFile(secondLevelItem, QgsLegendLayerFile::nameFromLayer(newlayer), newlayer);
		    if(newlayer->visible())
		      {
			mStateOfCheckBoxes.insert(std::make_pair(newfile, Qt::Checked));
		      }
		    else
		      {
			mStateOfCheckBoxes.insert(std::make_pair(newfile, Qt::Unchecked));
		      }
		    newlayer->setLegendLayerFile(newfile);
		    newlayer->initContextMenu(mApp);

		    //move newfile as the last child of the legendlayerfilegroup
		    QgsLegendItem* curitem = newfile;
		    while(curitem = newfile->nextSibling())
		      {
			moveItem(newfile, curitem);
		      }
		  }
		layerfilenode = layerfilenode.nextSibling();
		layerfileelem = layerfilenode.toElement();
	      }

	    //symbology group
	    secondLevelItem = secondLevelItem->nextSibling();
	    QDomNode symbologygroupnode = filegroupnode.nextSibling();
	    QDomElement symbologygroupelem = symbologygroupnode.toElement();
	    open = symbologygroupelem.attribute("open");
	    if(open == "true")
	    {
		expandItem(secondLevelItem);
	    }

	    //property group
	    secondLevelItem = secondLevelItem->nextSibling();
	    QDomNode propertygroupnode = symbologygroupnode.nextSibling();
	    QDomElement propertygroupelem = propertygroupnode.toElement();
	    open = propertygroupelem.attribute("open");
	    {
		expandItem(secondLevelItem);
	    }

	    if(child.nextSibling().isNull() && !child.parentNode().isNull()) //go one hierarchy step up
	      {
		child = child.parentNode();
	      }
	    if(theItem->nextSibling() == 0)
	      {
		theItem = dynamic_cast<QgsLegendItem*>(theItem->parent());
	      }

	    if(!theItem)
	      {
		break;
	      }

	    prevchild = theItem;
	    theItem = theItem->nextSibling();
	    child = child.nextSibling();
	    if(!theItem)
	      {
		break; //reached the end
	      }
	}
	while(!(child.isNull()));
    }
    return true;
}

void QgsLegend::storeInitialPosition(QTreeWidgetItem* li)
{
  if(li == firstItem()) //the item is the first item in the list view
    {
      mRestoreInformation = FIRST_ITEM;
      mRestoreItem = 0;
    }
  else if(li->parent() == 0) //li is a toplevel item, but not the first one
    {
      mRestoreInformation = YOUNGER_SIBLING;
      mRestoreItem = ((QgsLegendItem*)(li))->findYoungerSibling();
    }
  else if(li == li->parent()->child(0))//li is not a toplevel item, but the first child
    {
      mRestoreInformation = FIRST_CHILD;
      mRestoreItem = li->parent();
    }
  else
    {
      mRestoreInformation = YOUNGER_SIBLING;
      mRestoreItem = ((QgsLegendItem*)(li))->findYoungerSibling();
    }
}

void QgsLegend::resetToInitialPosition(QTreeWidgetItem* li)
{
  if(mRestoreInformation == FIRST_ITEM)
    {
#ifdef QGISDEBUG
      qWarning("FIRST_ITEM");
#endif
      removeItem(li);
      insertTopLevelItem(0, li);
    }
  else if(mRestoreInformation == FIRST_CHILD)
    {
#ifdef QGISDEBUG
      qWarning("FIRST_CHILD");
#endif
      removeItem(li);
      mRestoreItem->insertChild(0, li);
    }
  else if(mRestoreInformation == YOUNGER_SIBLING)
    {
#ifdef QGISDEBUG
      qWarning("YOUNGER_SIBLING");
#endif
      dynamic_cast<QgsLegendItem*>(li)->moveItem(dynamic_cast<QgsLegendItem*>(mRestoreItem));
    }
}

bool QgsLegend::yCoordAboveCenter(QgsLegendItem* it, int ycoord)
{
  QRect rect = visualItemRect(it);
  int height = rect.height();
  int top = rect.top();
  int mid = top + (height / 2);
  if (ycoord > mid) //bottom, remember the y-coordinate increases downwards
    {
      return false;
    }
  else//top
    {
      return true;
    }
}

/**Returns the first item in the hierarchy*/
QTreeWidgetItem* QgsLegend::firstItem()
{
  return topLevelItem(0);
}

/**Returns the next item (next sibling or next item on level above)*/
QTreeWidgetItem* QgsLegend::nextItem(QTreeWidgetItem* item)
{
  QgsLegendItem* litem = dynamic_cast<QgsLegendItem*>(item);
  if(litem->childCount() > 0)
    {
      return litem->child(0);
    }
  else if(litem->nextSibling())
    {
      return litem->nextSibling();
    }
  else if(!(litem->parent()))
    {
      return 0;
    }
  //todo: implement this in a nicer and safer way
  else if(dynamic_cast<QgsLegendItem*>(litem->parent())->nextSibling())
    {
      return (dynamic_cast<QgsLegendItem*>(litem->parent())->nextSibling());
    }
  else if(dynamic_cast<QgsLegendItem*>(litem->parent()->parent())->nextSibling())
    {
      return (dynamic_cast<QgsLegendItem*>(litem->parent()->parent())->nextSibling());
    }
  /*else if(dynamic_cast<QgsLegendItem*>(litem->parent()->parent()->parent())->nextSibling())//maximum four nesting states in the current legend
    {
      return (dynamic_cast<QgsLegendItem*>(litem->parent()->parent()->parent())->nextSibling());
      }*/
  else
    {
      return 0;
    }
}

QTreeWidgetItem* QgsLegend::nextSibling(QTreeWidgetItem* item)
{
  QModelIndex thisidx = indexFromItem(item);
  QModelIndex nextsidx = thisidx.sibling(thisidx.row()+1, thisidx.column());
  if(nextsidx.isValid())
    {
      return dynamic_cast<QgsLegendItem*>(itemFromIndex(nextsidx));
    }
  else
    {
      return 0;
    }
}

QTreeWidgetItem* QgsLegend::previousSibling(QTreeWidgetItem* item)
{
  QModelIndex thisidx = indexFromItem(item);
  QModelIndex nextsidx = thisidx.sibling(thisidx.row()-1, thisidx.column());
  if(nextsidx.isValid())
    {
      return dynamic_cast<QgsLegendItem*>(itemFromIndex(nextsidx));
    }
  else
    {
      return 0;
    }
}

void QgsLegend::moveItem(QTreeWidgetItem* move, QTreeWidgetItem* after)
{
  if(move->parent())
    {
      move->parent()->takeChild(move->parent()->indexOfChild(move));
    }
  else //move is toplevel item
    {
      takeTopLevelItem(indexOfTopLevelItem(move));
    }
  if(after->parent())
    {
      after->parent()->insertChild(after->parent()->indexOfChild(after)+1, move);
    }
  else //toplevel item
    {
      insertTopLevelItem(indexOfTopLevelItem(after)+1, move);
    }
}

void QgsLegend::removeItem(QTreeWidgetItem* item)
{
  if(item->parent())
    {
      item->parent()->takeChild(item->parent()->indexOfChild(item));
    }
  else
    {
      takeTopLevelItem(indexOfTopLevelItem(item));
    }
}

std::deque<QString> QgsLegend::layerIDs()
{
  std::deque<QString> layers;
  QTreeWidgetItem* theItem = firstItem();
  while (theItem)
    {
      QgsLegendItem *li = dynamic_cast<QgsLegendItem*>(theItem);
      QgsLegendLayerFile* llf = dynamic_cast<QgsLegendLayerFile*>(li);
      if(llf)
	{
	  QgsMapLayer *lyr = llf->layer();
	  layers.push_front(lyr->getLayerID());
	}
      theItem = nextItem(theItem);
    }
  
#ifdef QGISDEBUG
  qWarning("QgsLegend::layerIDs()");
  for(std::deque<QString>::iterator it = layers.begin(); it != layers.end(); ++it)
    {
      qWarning(*it);
    }
#endif

  return layers;
}

void QgsLegend::handleItemChange(QTreeWidgetItem* item, int row)
{
  closePersistentEditor(item, row);
  QgsLegendLayerFile* llf = dynamic_cast<QgsLegendLayerFile*>(item);
  if(llf)
    {
#ifdef QGISDEBUG
      qWarning("item is a QgsLegendLayerFile*");
#endif
      std::map<QTreeWidgetItem*, Qt::CheckState>::iterator it = mStateOfCheckBoxes.find(item);
      if(it != mStateOfCheckBoxes.end())
	{
	  if(it->second != item->checkState(0)) //the checkState has changed
	    {
	      QgsMapLayer* theLayer = llf->layer();
	      if(theLayer)
		{
		  bool checked = (item->checkState(0) == Qt::Checked); 
		  theLayer->setVisible(checked);
		  //todo: check, how the checkbox of the legendlayer needs to be updated
		  QgsLegendLayer* ll = dynamic_cast<QgsLegendLayer*>(item->parent()->parent());
		  std::list<QgsLegendLayerFile*> llfiles = ll->legendLayerFiles();
		  std::list<QgsLegendLayerFile*>::iterator iter = llfiles.begin();
		  Qt::CheckState theState = (*iter)->checkState(0);
		  for(; iter != llfiles.end(); ++iter)
		    {
		      if(theState != (*iter)->checkState(0))
			{
			  theState = Qt::PartiallyChecked;
			  break;
			}
		    }
		  //and update the checkbox of the legendlayer if necessary
		  if(theState != ll->checkState(0));
		  {
		    blockSignals(true);
		    ll->setCheckState(0, theState);
		    mStateOfCheckBoxes[ll] = theState;
		    blockSignals(false);
		  }
		}
	      mStateOfCheckBoxes[item] = item->checkState(0);
	    }
	}
      return;
    }

  QgsLegendLayer* ll = dynamic_cast<QgsLegendLayer*>(item);
  if(ll)
    {
#ifdef QGISDEBUG
      qWarning("item is a QgsLegendLayer");
#endif
      std::map<QTreeWidgetItem*, Qt::CheckState>::iterator it = mStateOfCheckBoxes.find(item);
      if(it != mStateOfCheckBoxes.end())
	{
	  if(it->second != item->checkState(0)) //the checkState has changed
	    {
	      bool checked = (item->checkState(0) == Qt::Checked); 
	      std::list<QgsLegendLayerFile*> llflist = ll->legendLayerFiles();
	      mMapCanvas->setRenderFlag(false);
	      //go through all the legendlayerfiles and set their checkState
	      for(std::list<QgsLegendLayerFile*>::iterator it = llflist.begin(); it != llflist.end(); ++it)
		{
		  if(checked)
		    {
		      (*it)->setCheckState(0, Qt::Checked);
		      mStateOfCheckBoxes[(*it)] = Qt::Checked;
		    }
		  else
		    {
		      (*it)->setCheckState(0, Qt::Unchecked);
		      mStateOfCheckBoxes[(*it)] = Qt::Unchecked;
		    }
		}
	      mMapCanvas->setRenderFlag(true);
	    }
	}
    }
}

void QgsLegend::openEditor()
{
  QTreeWidgetItem* theItem = currentItem();
  if(theItem)
    {
      openPersistentEditor(theItem, 0);
    }
}
