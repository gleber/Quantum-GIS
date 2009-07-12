#ifndef PALLABELING_H
#define PALLABELING_H

class QPainter;
class QgsMapRenderer;
class QgsRectangle;
class QgsCoordinateTransform;

#include <QString>
#include <QFont>
#include <QColor>
#include <QList>
#include <QRectF>

namespace pal
{
  class Pal;
  class Layer;
}

class QgsMapToPixel;
class QgsFeature;
#include "qgspoint.h"

class MyLabel;

class LayerSettings
{
public:
  LayerSettings();
  LayerSettings(const LayerSettings& s);
  ~LayerSettings();
  
  enum Placement
  {
    AroundPoint, // Point / Polygon
    OverPoint, // Point / Polygon
    Line, // Line / Polygon
    Horizontal, // Polygon
    Free // Polygon
  };

  enum LinePlacementFlags
  {
    OnLine    = 1,
    AboveLine = 2,
    BelowLine = 4,
    MapOrientation = 8
  };

  QString layerId;
  QString fieldName;
  Placement placement;
  unsigned long placementFlags;
  QFont textFont;
  QColor textColor;
  bool enabled;
  int priority; // 0 = low, 10 = high
  bool obstacle; // whether it's an obstacle
  double dist; // distance from the feature (in pixels)
  int scaleMin, scaleMax; // disabled if both are zero
  int bufferSize;
  QColor bufferColor;

  // called from register feature hook
  void calculateLabelSize(QString text, double& labelX, double& labelY);

  // implementation of register feature hook
  void registerFeature(QgsFeature& f);

  // temporary stuff: set when layer gets prepared
  pal::Layer* palLayer;
  int fieldIndex;
  QFontMetrics* fontMetrics;
  int fontBaseline;
  const QgsMapToPixel* xform;
  const QgsCoordinateTransform* ct;
  QgsPoint ptZero, ptOne;
  QList<MyLabel*> geometries;
};

class LabelCandidate
{
public:
  LabelCandidate(QRectF r, double c): rect(r), cost(c) {}

  QRectF rect;
  double cost;
};

class PalLabeling
{
public:
    PalLabeling(QgsMapRenderer* renderer);
    ~PalLabeling();

    void doLabeling(QPainter* painter, QgsRectangle extent);

    void addLayer(LayerSettings layerSettings);

    void removeLayer(QString layerId);

    const LayerSettings& layer(QString layerId);

    void numCandidatePositions(int& candPoint, int& candLine, int& candPolygon);
    void setNumCandidatePositions(int candPoint, int candLine, int candPolygon);

    enum Search { Chain, Popmusic_Tabu, Popmusic_Chain, Popmusic_Tabu_Chain, Falp };

    void setSearchMethod(Search s);
    Search searchMethod() const;

    bool isShowingCandidates() const { return mShowingCandidates; }
    void setShowingCandidates(bool showing) { mShowingCandidates = showing; }
    const QList<LabelCandidate>& candidates() { return mCandidates; }

    //! hook called when drawing layer before issuing select()
    static int prepareLayerHook(void* context, void* layerContext, int& attrIndex);
    //! hook called when drawing for every feature in a layer
    static void registerFeatureHook(QgsFeature& f, void* layerContext);


    static void drawLabelBuffer(QPainter* p, QString text, int size, QColor color);

protected:

    void initPal();

protected:
    QList<LayerSettings> mLayers;
    LayerSettings mInvalidLayer;

    QgsMapRenderer* mMapRenderer;
    int mCandPoint, mCandLine, mCandPolygon;
    Search mSearch;

    pal::Pal* mPal;

    // list of candidates from last labeling
    QList<LabelCandidate> mCandidates;
    bool mShowingCandidates;
};

#endif // PALLABELING_H
