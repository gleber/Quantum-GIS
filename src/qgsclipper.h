/***************************************************************************
                          qgsclipper.h  -  a class that clips line
                          segments and polygons
                             -------------------
    begin                : March 2004
    copyright            : (C) 2005 by Gavin Macaulay
    email                : 
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

#ifndef QGSCLIPPER_H
#define QGSCLIPPER_H

#include "qgspoint.h"
#include "qpointarray.h"

#include <vector>
#include <utility>
#include <cmath>
#include <iostream>

// The functions in this class are likely to be called from within a
// render loop and hence need to as CPU efficient as possible. 

// The main purpose of the functions in this class are to trim lines
// and polygons to lie within a rectangular region. This is necessary
// for drawing items to an X11 display which have a limit on the
// magnitude of the screen coordinates (+/- 32768, i.e. 16 bit integer).

class QgsClipper
{
 public:

  // Coordinates of the rectangular box that we trim to.
  //
  // These are the limits for X11 screen coordinates. The actual
  // values are +/-32767, but we allow a little bit of space for
  // rounding errors. 

  // You may wonder why the clipping is done to these coordindates
  // rather than the boundaries of the qgis canvas. Reasons include:
  // - making the boundaries static const allows the compiler to
  //   optimise the code that uses these values more than if they changed
  //   for every call to the trim code.
  // - clipping takes quite a bit of CPU effort, and the less that this is
  //   done the better. More stuff would have to be clipped if the
  //   boundaries were the qgis canvas (but this may be offset by
  //   having less to draw).
  // 
  // The limit is set to 30,000 instead of 32768 because that things
  // still go wrong.

  static const double maxX =  30000;
  static const double minX = -30000;
  static const double maxY =  30000;
  static const double minY = -30000;

  // Used when testing for equivalance to 0.0
  static const double SMALL_NUM = 1e-12;

  // A handy way to refer to the four boundaries
  enum boundary {Xmax, Xmin, Ymax, Ymin};

  // Trims the given feature to a rectangular box. Returns the trimmed
  // feature in x and y. The shapeOpen parameter determines whether
  // the function treats the points as a closed shape (polygon), or as
  // an open shape (linestring).
  static void trimFeature(std::vector<double>& x,
			  std::vector<double>& y,
			  bool shapeOpen);

 private:

  // Trims the given feature to the given boundary. Returns the
  // trimmed feature in the outX and outY vectors.
  static void trimFeatureToBoundary(const std::vector<double>& inX,
				    const std::vector<double>& inY,
				    std::vector<double>& outX, 
				    std::vector<double>& outY,
				    boundary b, 
				    bool shapeOpen);

  // Determines if a point is inside or outside the given boundary
  static bool inside(const double x, const double y, boundary b);

  // Calculates the intersection point between a line defined by a
  // (x1, y1), and (x2, y2) and the given boundary
  static QgsPoint intersect(const double x1, const double y1, 
			    const double x2, const double y2,
			    boundary b);
};

// The inline functions

// Trim the feature using Sutherland and Hodgman's
// polygon-clipping algorithm. See J. D. Foley, A. van Dam,
// S. K. Feiner, and J. F. Hughes, Computer Graphics, Principles and
// Practice. Addison-Wesley Systems Programming Series,
// Addison-Wesley, 2nd ed., 1991. 

// I understand that this is not the most efficient algorithm, but is
// one (the only?) that is guaranteed to always give the correct
// result. 

inline void QgsClipper::trimFeature(std::vector<double>& x,
				    std::vector<double>& y,
				    bool shapeOpen)
{
  std::vector<double> tmpX;
  std::vector<double> tmpY;
  trimFeatureToBoundary(x, y, tmpX, tmpY, Xmax, shapeOpen);

  x.resize(0);
  y.resize(0);
  trimFeatureToBoundary(tmpX, tmpY, x, y, Ymax, shapeOpen);

  tmpX.resize(0);
  tmpY.resize(0);
  trimFeatureToBoundary(x, y, tmpX, tmpY, Xmin, shapeOpen);

  x.resize(0);
  y.resize(0);
  trimFeatureToBoundary(tmpX, tmpY, x, y, Ymin, shapeOpen);
}

// An auxilary function that is part of the polygon trimming
// code. Will trim the given polygon to the given boundary and return
// the trimmed polygon in the out pointer. Uses Sutherland and
// Hodgman's polygon-clipping algorithm.

inline void QgsClipper::trimFeatureToBoundary(
            const std::vector<double>& inX, 
	    const std::vector<double>& inY, 
            std::vector<double>& outX, 
	    std::vector<double>& outY, 
	    boundary b, bool shapeOpen)
{
  // The shapeOpen parameter selects whether this function treats the
  // shape as open or closed. False is appropriate for polygons and
  // true for polylines.

  int i1 = inX.size() - 1; // start with last point

  // and compare to the first point initially.
  for (int i2 = 0; i2 < inX.size() ; ++i2)
  { // look at each edge of the polygon in turn
    if (inside(inX[i2], inY[i2], b)) // end point of edge is inside boundary
    {
      if (inside(inX[i1], inY[i1], b)) 
      {
	outX.push_back( inX[i2] );
	outY.push_back( inY[i2] );
      }
      else
      {
	// edge crosses into the boundary, so trim back to the boundary, and
	// store both ends of the new edge
	if (!(i2 == 0 && shapeOpen))
	{
	  QgsPoint p = intersect(inX[i1], inY[i1], inX[i2], inY[i2], b);
	  outX.push_back( p.x() );
	  outY.push_back( p.y() );
	}

	outX.push_back( inX[i2] );
	outY.push_back( inY[i2] );
      }
    }
    else // end point of edge is outside boundary
    {
      // start point is in boundary, so need to trim back
      if (inside(inX[i1], inY[i1], b))
      {
	if (!(i2 == 0 && shapeOpen))
	{
	  QgsPoint p = intersect(inX[i1], inY[i1], inX[i2], inY[i2], b);
	  outX.push_back( p.x() );
	  outY.push_back( p.y() );
	}
      }
    }
    i1 = i2;
  }
}

// An auxilary function to trimPolygonToBoundarY() that returns
// whether a point is inside or outside the given boundary. 

inline bool QgsClipper::inside(const double x, const double y, boundary b)
{
  switch (b)
  {
  case Xmax: // x < maxX is inside
    if (x < maxX)
      return true;
    break;
  case Xmin: // x > minX is inside
    if (x > minX)
      return true;
    break;
  case Ymax: // y < maxY is inside
    if (y < maxY)
      return true;
    break;
  case Ymin: // y > minY is inside
    if (y > minY)
      return true;
    break;
  }
  return false;
}


// An auxilary function to trimPolygonToBoundarY() that calculates and
// returns the intersection of the line defined by the given points
// and the given boundary.

inline QgsPoint QgsClipper::intersect(const double x1, const double y1, 
				      const double x2, const double y2,
				      boundary b)
{
  // This function assumes that the two given points (x1, y1), and
  // (x2, y2) cross the given boundary. Making this assumption allows
  // some optimisations. 

  double r_n, r_d;

  switch (b)
  {
  case Xmax: // x = maxX boundary
    r_n = -(x1 - maxX) * (maxY - minY);
    r_d =  (x2 - x1)   * (maxY - minY);
    break;
  case Xmin: // x = minX boundary
    r_n = -(x1 - minX) * (maxY - minY);
    r_d =  (x2 - x1)   * (maxY - minY);
    break;
  case Ymax: // y = maxY boundary
    r_n =  (y1 - maxY) * (maxX - minX);
    r_d = -(y2 - y1)   * (maxX - minX);
    break;
  case Ymin: // y = minY boundary
    r_n =  (y1 - minY) * (maxX - minX);
    r_d = -(y2 - y1)   * (maxX - minX);
    break;
  }

  QgsPoint p;

  if (std::abs(r_d) > SMALL_NUM && std::abs(r_n) > SMALL_NUM)
  { // they cross
    double r = r_n / r_d;
    p.set(x1 + r*(x2 - x1), y1 + r*(y2 - y1));
  }
  else
  {
    // Should never get here, but if we do for some reason, cause a
    // clunk because something else is wrong if we do.
    Q_ASSERT(std::abs(r_d) > SMALL_NUM && std::abs(r_n) > SMALL_NUM);
  }

  return p;
}


#endif
