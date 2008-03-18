/*****************************************************************************
 * $Source$
 * $Author$
 * $Date$
 * $Revision$
 *****************************************************************************/

/** \file matmul3d.h
 *  Author: Abhinav S Bhatele
 *  Date Created: March 13th, 2008
 *
 */

// Read-only global variables

/*readonly*/ CProxy_Main mainProxy;
/*readonly*/ CProxy_Compute compute;
/*readonly*/ int arrayDimX;
/*readonly*/ int arrayDimY;
/*readonly*/ int arrayDimZ;
/*readonly*/ int blockDimX;
/*readonly*/ int blockDimY;
/*readonly*/ int blockDimZ;
/*readonly*/ int torusDimX;
/*readonly*/ int torusDimY;
/*readonly*/ int torusDimZ;

/*readonly*/ int num_chare_x;
/*readonly*/ int num_chare_y;
/*readonly*/ int num_chare_z;

static unsigned long next = 1;

int myrand(int numpes) {
  next = next * 1103515245 + 12345;
  return((unsigned)(next/65536) % numpes);
}

#define USE_TOPOMAP	0
#define USE_RRMAP	0
#define USE_RNDMAP	0

double startTime;
double endTime;

/** \class Main
 *
 */
class Main : public CBase_Main {
  public:    
    Main(CkArgMsg* m);
};

/** \class Compute
 *
 */
class Compute: public CBase_Compute {
  public:
    Compute();
    Compute(CkMigrateMessage* m);

    ~Compute();

    void beginCopying();
};

/** \class ComputeMap
 *
 */
class ComputeMap: public CBase_ComputeMap {
  public:
    int X, Y, Z;
    int ***mapping;

    ComputeMap(int x, int y, int z);
    ~ComputeMap();
    int procNum(int, const CkArrayIndex &idx);
};

#include "matmul3d.def.h"
