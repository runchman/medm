/*
*****************************************************************
                          COPYRIGHT NOTIFICATION
*****************************************************************

THE FOLLOWING IS A NOTICE OF COPYRIGHT, AVAILABILITY OF THE CODE,
AND DISCLAIMER WHICH MUST BE INCLUDED IN THE PROLOGUE OF THE CODE
AND IN ALL SOURCE LISTINGS OF THE CODE.

(C)  COPYRIGHT 1993 UNIVERSITY OF CHICAGO

Argonne National Laboratory (ANL), with facilities in the States of
Illinois and Idaho, is owned by the United States Government, and
operated by the University of Chicago under provision of a contract
with the Department of Energy.

Portions of this material resulted from work developed under a U.S.
Government contract and are subject to the following license:  For
a period of five years from March 30, 1993, the Government is
granted for itself and others acting on its behalf a paid-up,
nonexclusive, irrevocable worldwide license in this computer
software to reproduce, prepare derivative works, and perform
publicly and display publicly.  With the approval of DOE, this
period may be renewed for two additional five year periods.
Following the expiration of this period or periods, the Government
is granted for itself and others acting on its behalf, a paid-up,
nonexclusive, irrevocable worldwide license in this computer
software to reproduce, prepare derivative works, distribute copies
to the public, perform publicly and display publicly, and to permit
others to do so.

*****************************************************************
                                DISCLAIMER
*****************************************************************

NEITHER THE UNITED STATES GOVERNMENT NOR ANY AGENCY THEREOF, NOR
THE UNIVERSITY OF CHICAGO, NOR ANY OF THEIR EMPLOYEES OR OFFICERS,
MAKES ANY WARRANTY, EXPRESS OR IMPLIED, OR ASSUMES ANY LEGAL
LIABILITY OR RESPONSIBILITY FOR THE ACCURACY, COMPLETENESS, OR
USEFULNESS OF ANY INFORMATION, APPARATUS, PRODUCT, OR PROCESS
DISCLOSED, OR REPRESENTS THAT ITS USE WOULD NOT INFRINGE PRIVATELY
OWNED RIGHTS.

*****************************************************************
LICENSING INQUIRIES MAY BE DIRECTED TO THE INDUSTRIAL TECHNOLOGY
DEVELOPMENT CENTER AT ARGONNE NATIONAL LABORATORY (630-252-2000).
*/
/*****************************************************************************
 *
 *     Original Author : Mark Anderson
 *     Second Author   : Frederick Vong
 *     Third Author    : Kenneth Evans, Jr.
 *
 *****************************************************************************
*/

#ifndef CARTESIAN_PLOT_H
#define CARTESIAN_PLOT_H

#define CP_XDATA_COLUMN         0
#define CP_YDATA_COLUMN         1
#define CP_COLOR_COLUMN         2
 
#define CP_APPLY_BTN    0
#define CP_CLOSE_BTN    1

#define CP_X   0
#define CP_Y   1
#define CP_Y2  2

#define CP_LINE_SOLID 0
#define CP_LINE_NONE  1

#define CP_FAST 0
#define CP_FULL 1
 
typedef struct {
    float axisMin;
    float axisMax;
    Boolean isCurrentlyFromChannel;
} CartesianPlotAxisRange;

typedef enum {
    CP_XYScalar,
    CP_XScalar,         CP_YScalar,
    CP_XVector,         CP_YVector,
    CP_XVectorYScalar,
    CP_YVectorXScalar,
    CP_XYVector
} XYChannelTypeEnum;

typedef struct {
    struct _MedmCartesianPlot *cartesianPlot;
    CpDataHandle          hcp;
    int                   trace;
    Record                *recordX;
    Record                *recordY;
    XYChannelTypeEnum     type;
    int                   init;
} XYTrace;

typedef struct _MedmCartesianPlot {
    DlElement        *dlElement;    /* Must be first */
    UpdateTask       *updateTask;   /* Must be second */
    XYTrace          xyTrace[MAX_TRACES];
    XYTrace          eraseCh;
    XYTrace          triggerCh;
    int              nTraces;       /* number of traces (<=MAX_TRACES) */
    CpDataHandle     hcp1, hcp2;    /* CpData handles */
  /* Used for channel-based range determination (filled in at connect) */
    CartesianPlotAxisRange  axisRange[3]; /* X, Y, Y2 _AXIS_ELEMENT          */
    eraseMode_t      eraseMode;     /* erase mode */
    Boolean          dirty1;        /* cpData1 needs screen update */
    Boolean          dirty2;        /* cpData2 needs screen update */
    TS_STAMP         startTime;
    Boolean          timeScale;
} MedmCartesianPlot;


/* Function prototypes for generic plotting routines
 *   These should be the same for any plot package */

CpDataHandle CpDataCreate(Widget w, CpDataType type, int nsets, int npoints);
int CpDataGetLastPoint(CpDataHandle hData, int set);
double CpDataGetXElement(CpDataHandle hData, int set, int point);
double CpDataGetYElement(CpDataHandle hData, int set, int point);
void CpDataDestroy(CpDataHandle hData);
int CpDataSetHole(CpDataHandle hData, double hole);
int CpDataSetLastPoint(CpDataHandle hData, int set, int npoints);
int CpDataSetXElement(CpDataHandle hData, int set, int point, double x);
int CpDataSetYElement(CpDataHandle hData, int set, int point, double y);

void CpGetAxisInfo(Widget w,
  XtPointer *userData, Boolean *xAxisIsTime, char **timeFormat,
  Boolean *xAxisIsLog, Boolean *yAxisIsLog, Boolean *y2AxisIsLog,
  Boolean *xAxisIsAuto, Boolean *yAxisIsAuto, Boolean *y2AxisIsAuto,
  XcVType *xMaxF, XcVType *yMaxF, XcVType *y2MaxF,
  XcVType *xMinF, XcVType *yMinF, XcVType *y2MinF);
void CpGetAxisMaxMin(Widget w, int axis, XcVType *maxF, XcVType *minF);
void CpSetAxisStyle(Widget w, CpDataHandle hData, int trace, int lineType,
  int fillType, XColor color, int pointSize);
void CpSetAxisAll(Widget w, int axis, XcVType max, XcVType min,
  XcVType tick, XcVType num, int precision);
void CpSetAxisMax(Widget w, int axis, XcVType max, XcVType tick,
  XcVType num, int precision);
void CpSetAxisMin(Widget w, int axis, XcVType min, XcVType tick,
  XcVType num, int precision);
void CpSetAxisAuto(Widget w, int axis);
void CpSetAxisChannel(Widget w, int axis, XcVType max, XcVType min);
void CpSetAxisLinear(Widget w, int axis);
void CpSetAxisLog(Widget w, int axis);
void CpSetAxisMaxMin(Widget w, int axis, XcVType max, XcVType min);
void CpSetAxisTime(Widget w, int axis, time_t base, char * format);
void CpSetData(Widget w, int axis, CpDataHandle hData);
void CpSetTimeBase(Widget w, time_t base);
void CpSetTimeFormat(Widget w, char *format);
void CpUpdateWidget(Widget w, int full);
Widget CpCreateCartesianPlot(DisplayInfo *displayInfo,
  DlCartesianPlot *dlCartesianPlot, MedmCartesianPlot *pcp);

#define CP_X_AXIS_STYLE   0
#define CP_Y_AXIS_STYLE   1
#define CP_Y2_AXIS_STYLE  2
#define CP_X_RANGE_STYLE  3
#define CP_Y_RANGE_STYLE  4
#define CP_Y2_RANGE_STYLE 5
#define CP_X_RANGE_MIN    6
#define CP_Y_RANGE_MIN    7
#define CP_Y2_RANGE_MIN   8
#define CP_X_RANGE_MAX    9
#define CP_Y_RANGE_MAX    10
#define CP_Y2_RANGE_MAX   11
#define CP_X_TIME_FORMAT  12
 
#define MAX_CP_AXIS_ELEMENTS    20
/* The following should be the largest of NUM_CP_TIME_FORMAT,
 *   NUM_CARTESIAN_PLOT_RANGE_STYLES, NUM_CARTESIAN_PLOT_AXIS_STYLES */
#define MAX_CP_AXIS_BUTTONS  NUM_CP_TIME_FORMAT

EXTERN char *cpTimeFormatString[NUM_CP_TIME_FORMAT];

#endif     /* #ifndef CARTESIAN_PLOT_H */
