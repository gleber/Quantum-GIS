// The code is heavily based on Christopher Michaelis'  DXF to Shapefile Converter 
// (http://www.wanderingidea.com/content/view/12/25/), released under GPL License
//
// This code is based on two other products:
// DXFLIB (http://www.ribbonsoft.com/dxflib.html)
//    This is a library for reading DXF files, also GPL.
// SHAPELIB (http://shapelib.maptools.org/)
//    Used for the Shapefile functionality.
//    It is Copyright (c) 1999, Frank Warmerdam, released under the following "MIT Style" license:
//Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated 
//documentation files (the "Software"), to deal in the Software without restriction, including without limitation 
//the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, 
//and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

//The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

//THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES 
//OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE 
//LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR 
//IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//

#include "dxflib/src/dl_creationadapter.h"
#include "shapelib-1.2.10/shapefil.h"
#include "getInsertions.h"
#include <vector>
#include <fstream>
using namespace std;

class Builder: public DL_CreationAdapter
{
  public:
    Builder();
    ~Builder();

    void FinalizeAnyPolyline();

    virtual void addLayer(const DL_LayerData &data);
    virtual void addPoint(const DL_PointData &data);
    virtual void addLine(const DL_LineData &data);
    virtual void addPolyline(const DL_PolylineData &data);
    virtual void addArc(const DL_ArcData &data);
    virtual void addCircle(const DL_CircleData &data);
    virtual void addVertex(const DL_VertexData &data);
    virtual void addBlock(const DL_BlockData &data);
    virtual void endBlock();
    virtual void endSequence();
    virtual void addText(const DL_TextData &data);

    void set_numlayers(int n);
    void set_numpoints(int n);
    void set_numlines(int n);
    void set_numplines(int n);
    void set_shptype(int n);

    void set_convertText(bool);
    void initBuilder(string, int, double*, double*, string*, int, bool);
    void print_shpObjects();

    int ret_numlabels();
    int ret_numlayers();
    int ret_numpoints();
    int ret_numlines();
    int ret_numplines();
    int ret_shptype();
    int ret_textObjectsSize();
    string ret_outputshp();
    string ret_outputtshp();
    bool convertText; 



  private:
    string outputdbf;
    string outputshp;
    string outputtdbf;
    string outputtshp;
    string logfname;
    ofstream logfile;


    vector < DL_VertexData > polyVertex;
    vector <SHPObject *> shpObjects; // metto qui tutti gli oggetti letti
    vector <DL_TextData> textObjects;

    int numlayers;
    int numpoints;
    int numlines;
    int numplines;

    int shptype; // 0 ,1 ,2
    int shapefileType; // SHPT_POINT, ...
    int fetchedprims;
    int fetchedtexts;

    bool ignoringBlock;
    bool current_polyline_willclose;
    bool store_next_vertex_for_polyline_close;

    int awaiting_polyline_vertices;
    long current_polyline_pointcount;

    SHPObject *currently_Adding_PolyLine;

    double *grpXVals;
    double *grpYVals;
    string *grpNames;
    string fname;
    int insertCount;

    double closePolyX, closePolyY, closePolyZ;
    double currentBlockX, currentBlockY;
};
