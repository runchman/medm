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

#define DEBUG_DELETE 0

#include "medm.h"
#include <Xm/MwmUtil.h>

#ifdef __cplusplus
extern "C" {
#endif
#include <cvtFast.h>
#ifdef __cplusplus
	   }
#endif

typedef struct _MedmValuator {
    DlElement        *dlElement;     /* Must be first */
    UpdateTask       *updateTask;    /* Must be second */
    Record           *record;
    int              oldIntegerValue;
    int              fontIndex;
} MedmValuator;

static void valuatorDraw(XtPointer);
static void valuatorUpdateGraphicalInfoCb(XtPointer cd);
static void valuatorUpdateValueCb(XtPointer);
static void valuatorDestroyCb(XtPointer);
static void valuatorSetValue(MedmValuator *, double, Boolean force);
static void valuatorRedrawValue(MedmValuator *, DisplayInfo *, Widget, DlValuator *, double);
static void handleValuatorExpose(Widget, XtPointer, XEvent *, Boolean *);
static void valuatorGetRecord(XtPointer, Record **, int *);
static void valuatorInheritValues(ResourceBundle *pRCB, DlElement *p);
static void valuatorSetBackgroundColor(ResourceBundle *pRCB, DlElement *p);
static void valuatorSetForegroundColor(ResourceBundle *pRCB, DlElement *p);
static void valuatorGetValues(ResourceBundle *pRCB, DlElement *p);
static void handleValuatorRelease(Widget, XtPointer, XEvent *, Boolean *);
static void valuatorGetLimits(DlElement *pE, DlLimits **ppL, char **pN);

static DlDispatchTable valuatorDlDispatchTable = {
    createDlValuator,
    NULL,
    executeDlValuator,
    hideDlValuator,
    writeDlValuator,
    valuatorGetLimits,
    valuatorGetValues,
    valuatorInheritValues,
    valuatorSetBackgroundColor,
    valuatorSetForegroundColor,
    genericMove,
    genericScale,
    genericOrient,
    NULL,
    NULL};

int valuatorFontListIndex(DlValuator *dlValuator)
{
    int i;
  /* More complicated calculation based on orientation, etc */
    for(i = MAX_FONTS-1; i >=  0; i--) {
	switch (dlValuator->direction) {
	case UP:
	    switch(dlValuator->label) {
	    case LABEL_NONE:
	    case NO_DECORATIONS:
		if((int)(.30*dlValuator->object.width) >= 
		  (fontTable[i]->max_bounds.width) )
		  return(i);
		break;
	    case OUTLINE:
	    case LIMITS:
		if((int)(.20*dlValuator->object.width) >= 
		  (fontTable[i]->max_bounds.width) )
		  return(i);
		break;
	    case CHANNEL:
		if((int)(.10*dlValuator->object.width) >= 
		  (fontTable[i]->max_bounds.width) )
		  return(i);
		break;
	    }
	    break;
	case RIGHT:
	    switch(dlValuator->label) {
	    case LABEL_NONE:
	    case NO_DECORATIONS:
	    case OUTLINE:
	    case LIMITS:
		if((int)(.45*dlValuator->object.height) >= 
		  (fontTable[i]->ascent + fontTable[i]->descent) )
		  return(i);
		break;
	    case CHANNEL:
		if((int)(.32*dlValuator->object.height) >= 
		  (fontTable[i]->ascent + fontTable[i]->descent) )
		  return(i);
		break;
	    }
	    break;
	}
    }
    return (0);
}

void createValuatorRunTimeInstance(DisplayInfo *displayInfo,
  DlElement *dlElement) {
    MedmValuator *pv;
    Arg args[25];
    int i, n, heightDivisor, scalePopupBorder;
    WidgetList children;
    Cardinal numChildren;
    DlValuator *dlValuator = dlElement->structure.valuator;

  /* If the widget is already created just return.  The update task
     will handle it. */
    if(dlElement->widget) return;

    if(dlElement->data) {
	pv = (MedmValuator *)dlElement->data;
    } else {
	pv = (MedmValuator *)malloc(sizeof(MedmValuator));
	dlElement->data = (void *)pv;
	if(pv == NULL) {
	    medmPrintf(1,"\nvaluatorCreateRunTimeInstance: Memory allocation error\n");
	    return;
	}
	pv->dlElement = dlElement;
	pv->updateTask = updateTaskAddTask(displayInfo,
	  &(dlValuator->object),
	  valuatorDraw,
	  (XtPointer)pv);
	
	if(pv->updateTask == NULL) {
	    medmPrintf(1,"\nvaluatorCreateRunTimeInstance: Memory allocation error\n");
	    free((char *)pv);
	    return;
	} else {
	    updateTaskAddDestroyCb(pv->updateTask,valuatorDestroyCb);
	    updateTaskAddNameCb(pv->updateTask,valuatorGetRecord);
	}
	pv->record = medmAllocateRecord(dlValuator->control.ctrl,
	  valuatorUpdateValueCb,
	  valuatorUpdateGraphicalInfoCb,
	  (XtPointer) pv);
	pv->fontIndex = valuatorFontListIndex(dlValuator);
	drawWhiteRectangle(pv->updateTask);
    }

  /* From the valuator structure, we've got Valuator's specifics */
    n = 0;
    XtSetArg(args[n],XmNx,(Position)dlValuator->object.x); n++;
    XtSetArg(args[n],XmNy,(Position)dlValuator->object.y); n++;
    XtSetArg(args[n],XmNwidth,(Dimension)dlValuator->object.width); n++;
    XtSetArg(args[n],XmNheight,(Dimension)dlValuator->object.height); n++;
    XtSetArg(args[n],XmNorientation,XmHORIZONTAL); n++;
    XtSetArg(args[n],XmNforeground,displayInfo->colormap[dlValuator->control.clr]); n++;
    XtSetArg(args[n],XmNbackground,displayInfo->colormap[dlValuator->control.bclr]); n++;
    XtSetArg(args[n],XmNhighlightThickness,1); n++;
    XtSetArg(args[n],XmNhighlightOnEnter,TRUE); n++;
    switch(dlValuator->label) {
    case LABEL_NONE: 		/* add in border for keyboard popup */
    case NO_DECORATIONS:
	scalePopupBorder = BORDER_WIDTH;
	heightDivisor = 1;
	break;
    case OUTLINE:
    case LIMITS: 
	scalePopupBorder = 0;
	heightDivisor = 2;
	break;
    case CHANNEL:
	scalePopupBorder = 0;
	heightDivisor = 3;
	break;
    }
  /* Need to handle Direction */
    switch (dlValuator->direction) {
    case UP:
	XtSetArg(args[n],XmNorientation,XmVERTICAL); n++;
	XtSetArg(args[n],XmNscaleWidth,dlValuator->object.width/heightDivisor 
	  - scalePopupBorder); n++;
	break;
    case RIGHT:
	XtSetArg(args[n],XmNorientation,XmHORIZONTAL); n++;
	XtSetArg(args[n],XmNscaleHeight,dlValuator->object.height/heightDivisor
	  - scalePopupBorder); n++;
	break;
    }

  /* Add in Valuator as userData for valuator keyboard entry handling */
    XtSetArg(args[n],XmNuserData,(XtPointer)pv); n++;
    XtSetArg(args[n],XmNfontList,fontListTable[pv->fontIndex]); n++;
    pv->dlElement->widget =  XtCreateWidget("valuator",
      xmScaleWidgetClass, displayInfo->drawingArea, args, n);

  /* Update the limits to reflect current src's */
    updatePvLimits(&dlValuator->limits);

  /* Set virtual range */
    n = 0;
  /* The Valuator works with fixed values of these three parameters
   * Only the labels are changed to reflect the actual numbers */
    XtSetArg(args[n],XmNminimum,VALUATOR_MIN); n++;
    XtSetArg(args[n],XmNmaximum,VALUATOR_MAX); n++;
    XtSetArg(args[n],XmNscaleMultiple,VALUATOR_MULTIPLE_INCREMENT); n++;
    XtSetValues(pv->dlElement->widget,args,n);

  /* Get children of scale */
    XtVaGetValues(pv->dlElement->widget,XmNnumChildren,&numChildren,
      XmNchildren,&children,NULL);

  /* Change translations for scrollbar child of valuator */
    for(i = 0; i < (int)numChildren; i++) {
	if(XtClass(children[i]) == xmScrollBarWidgetClass) {
	    XtOverrideTranslations(children[i],parsedTranslations);
	  /* Add event handler for Key/ButtonRelease which enables updates */
	    XtAddEventHandler(children[i],KeyReleaseMask|ButtonReleaseMask,
	      False,handleValuatorRelease,
	      (XtPointer)pv);
	}
    }

  /* Add the callbacks for update */
    XtAddCallback(pv->dlElement->widget, XmNvalueChangedCallback,
      valuatorValueChanged,(XtPointer)pv);
    XtAddCallback(pv->dlElement->widget, XmNdragCallback,
      valuatorValueChanged,(XtPointer)pv);
    
  /* Add event handler for expose - forcing display of min/max and value
   *   in own format */
    XtAddEventHandler(pv->dlElement->widget,ExposureMask,
      False,handleValuatorExpose,
      (XtPointer)pv);
    
  /* Add event handler for Key/ButtonRelease which enables updates */
    XtAddEventHandler(pv->dlElement->widget,KeyReleaseMask|ButtonReleaseMask,
      False,handleValuatorRelease,
      (XtPointer)pv);
}

void createValuatorEditInstance(DisplayInfo *displayInfo,
  DlElement *dlElement) {
    Arg args[25];
    int i, n, heightDivisor, scalePopupBorder;
    Widget widget;
    WidgetList children;
    Cardinal numChildren;
    DlValuator *dlValuator = dlElement->structure.valuator;
    
  /* Update the limits to reflect current src's */
    updatePvLimits(&dlValuator->limits);

  /* From the valuator structure, we've got Valuator's specifics */
    n = 0;
    XtSetArg(args[n],XmNx,(Position)dlValuator->object.x); n++;
    XtSetArg(args[n],XmNy,(Position)dlValuator->object.y); n++;
    XtSetArg(args[n],XmNwidth,(Dimension)dlValuator->object.width); n++;
    XtSetArg(args[n],XmNheight,(Dimension)dlValuator->object.height); n++;
    XtSetArg(args[n],XmNorientation,XmHORIZONTAL); n++;
    XtSetArg(args[n],XmNforeground,(Pixel)
      displayInfo->colormap[dlValuator->control.clr]); n++;
    XtSetArg(args[n],XmNbackground,(Pixel)
      displayInfo->colormap[dlValuator->control.bclr]); n++;
    XtSetArg(args[n],XmNhighlightThickness,1); n++;
    XtSetArg(args[n],XmNhighlightOnEnter,TRUE); n++;
    switch(dlValuator->label) {
    case LABEL_NONE: 		/* Add in border for keyboard popup */
    case NO_DECORATIONS:
	scalePopupBorder = BORDER_WIDTH;
	heightDivisor = 1;
	break;
    case OUTLINE:
    case LIMITS: 
	scalePopupBorder = 0;
	heightDivisor = 2;
	break;
    case CHANNEL:
	scalePopupBorder = 0;
	heightDivisor = 3;
	break;
    }
  /* Need to handle Direction */
    switch (dlValuator->direction) {
    case UP:
	XtSetArg(args[n],XmNorientation,XmVERTICAL); n++;
	XtSetArg(args[n],XmNscaleWidth,dlValuator->object.width/heightDivisor 
	  - scalePopupBorder); n++;
	break;
    case RIGHT:
	XtSetArg(args[n],XmNorientation,XmHORIZONTAL); n++;
	XtSetArg(args[n],XmNscaleHeight,dlValuator->object.height/heightDivisor
	  - scalePopupBorder); n++;
	break;
    }

    XtSetArg(args[n],XmNfontList,fontListTable[
      valuatorFontListIndex(dlValuator)]); n++;
    widget =  XtCreateWidget("valuator",
      xmScaleWidgetClass, displayInfo->drawingArea, args, n);
    
    dlElement->widget = widget;
    
  /* Get children of scale */
    XtVaGetValues(widget,XmNnumChildren,&numChildren,
      XmNchildren,&children,NULL);
    
  /* If in EDIT mode remove translations for children of valuator */
    for(i = 0; i < (int)numChildren; i++)
      XtUninstallTranslations(children[i]);

  /* If in EDIT mode add dlValuator as userData, and pass NULL in expose */
    XtVaSetValues(widget,XmNuserData,(XtPointer)dlValuator,NULL);
    
  /* Add event handler for expose - forcing display of min/max and value
   *   in own format */
    XtAddEventHandler(widget,ExposureMask,False,
      (XtEventHandler)handleValuatorExpose,(XtPointer)NULL);
    
  /* Add handlers */
    addCommonHandlers(widget, displayInfo);
    
    XtManageChild(widget);
}

void executeDlValuator(DisplayInfo *displayInfo, DlElement *dlElement)
{
  /* Don't do anyting if the element is hidden */
    if(dlElement->hidden) return;

    if(displayInfo->traversalMode == DL_EXECUTE) {
	createValuatorRunTimeInstance(displayInfo, dlElement);
    } else if(displayInfo->traversalMode == DL_EDIT) {
	if(dlElement->widget) {
	    XtDestroyWidget(dlElement->widget);
	    dlElement->widget = 0;
	}
	createValuatorEditInstance(displayInfo, dlElement);
    }
}

void hideDlValuator(DisplayInfo *displayInfo, DlElement *dlElement)
{
  /* Use generic hide for an element with a widget */
    hideWidgetElement(displayInfo, dlElement);
}

static void valuatorUpdateValueCb(XtPointer cd) {
    MedmValuator *pv = (MedmValuator *) ((Record *) cd)->clientData;
    updateTaskMarkUpdate(pv->updateTask);
}

static void valuatorDraw(XtPointer cd) {
    MedmValuator *pv = (MedmValuator *) cd;
    Record *pr = pv->record;
    DlElement *dlElement = pv->dlElement;
    DlValuator *dlValuator = dlElement->structure.valuator;
    Boolean dummy;
    Widget widget = dlElement->widget;

#if DEBUG_DELETE
    print("valuatorDraw: connected=%s readAccess=%s value=%g\n",
      pr->connected?"Yes":"No",pr->readAccess?"Yes":"No",pr->value);
#endif    
    
  /* Check if hidden */
    if(dlElement->hidden) {
	if(widget && XtIsManaged(widget)) {
	    XtUnmanageChild(widget);
	}
	return;
    }
    
    if(pr->connected) {
	if(pr->readAccess) {
	    if(widget) {
		addCommonHandlers(widget, pv->updateTask->displayInfo);
		XtManageChild(widget);
	    } else {
		return;
	    }

	  /* Valuator is only controller/monitor which can have updates disabled */

	    if(dlValuator->enableUpdates) {
		valuatorSetValue(pv,pr->value,True);
		{
		    XExposeEvent event;
		    event.count = 0;
		    handleValuatorExpose(widget,(XtPointer) pv,(XEvent *) &event, &dummy);
		}
	    }
	    if(pr->writeAccess)
	      XDefineCursor(XtDisplay(widget),XtWindow(widget),rubberbandCursor);
	    else
	      XDefineCursor(XtDisplay(widget),XtWindow(widget),noWriteAccessCursor);
	} else {
	    draw3DPane(pv->updateTask,
	      pv->updateTask->displayInfo->colormap[dlValuator->control.bclr]);
	    draw3DQuestionMark(pv->updateTask);
	    if(widget) XtUnmanageChild(widget);
	}
    } else {
	if(widget) XtUnmanageChild(widget);
	drawWhiteRectangle(pv->updateTask);
    }
}

static void valuatorUpdateGraphicalInfoCb(XtPointer cd) {
    Record *pr = (Record *) cd;
    MedmValuator *pv = (MedmValuator *) pr->clientData;
    DlValuator *dlValuator = pv->dlElement->structure.valuator;
    Widget widget = pv->dlElement->widget;
    XcVType hopr, lopr, val;
    short precision;

    switch (pr->dataType) {
    case DBF_STRING :
	medmPostMsg(1,"valuatorUpdateGraphicalInfoCb:\n"
	  "  Illegal channel type for %s\n"
	  "  Cannot attach valuator\n",
	  dlValuator->control.ctrl);
	return;
    case DBF_ENUM :
    case DBF_CHAR :
    case DBF_INT :
    case DBF_LONG :
    case DBF_FLOAT :
    case DBF_DOUBLE :
	hopr.fval = (float) pr->hopr;
	lopr.fval = (float) pr->lopr;
	val.fval = (float) pr->value;
	precision = pr->precision;
	break;
    default :
	medmPostMsg(1,"valuatorUpdateGraphicalInfoCb:\n"
	  "  Unknown channel type for %s\n"
	  "  Cannot attach valuator\n",
	  dlValuator->control.ctrl);
	break;
    }
    if((hopr.fval == 0.0) && (lopr.fval == 0.0)) {
	hopr.fval += 1.0;
    }
    if(widget != NULL) {
      /* Set Channel and User limits (if apparently not set yet) */
	dlValuator->limits.loprChannel = lopr.fval;
	if(dlValuator->limits.loprSrc != PV_LIMITS_USER &&
	  dlValuator->limits.loprUser == LOPR_DEFAULT) {
	    dlValuator->limits.loprUser = lopr.fval;
	}
	dlValuator->limits.hoprChannel = hopr.fval;
	if(dlValuator->limits.hoprSrc != PV_LIMITS_USER &&
	  dlValuator->limits.hoprUser == HOPR_DEFAULT) {
	    dlValuator->limits.hoprUser = hopr.fval;
	}
	dlValuator->limits.precChannel = precision;
	if(dlValuator->limits.precSrc != PV_LIMITS_USER &&
	  dlValuator->limits.precUser == PREC_DEFAULT) {
	    dlValuator->limits.precUser = precision;
	}

      /* Set values in the dlValuator if src is Channel */
	if(dlValuator->limits.loprSrc == PV_LIMITS_CHANNEL) {
	    dlValuator->limits.lopr = lopr.fval;
	}
	if(dlValuator->limits.hoprSrc == PV_LIMITS_CHANNEL) {
	    dlValuator->limits.hopr = hopr.fval;
	}
	if(dlValuator->limits.precSrc == PV_LIMITS_CHANNEL) {
	    dlValuator->limits.prec = precision;
	}
	valuatorRedrawValue(pv,
	  pv->updateTask->displayInfo,pv->dlElement->widget,
	  pv->dlElement->structure.valuator, pv->record->value);
    }
}

static void valuatorDestroyCb(XtPointer cd) {
    MedmValuator *pv = (MedmValuator *) cd;
    if(pv) {
	medmDestroyRecord(pv->record);
	pv->dlElement->data = 0;
	free((char *)pv);
    }
    return;
}

#ifdef __cplusplus
static void handleValuatorExpose(Widget w, XtPointer clientData,
  XEvent *pEvent, Boolean *)
#else
static void handleValuatorExpose(Widget w, XtPointer clientData,
  XEvent *pEvent, Boolean *continueToDispatch)
#endif
{
    XExposeEvent *event = (XExposeEvent *) pEvent;
    MedmValuator *pv;
    DlValuator *dlValuator;
    unsigned long foreground, background;
    Dimension scaleWidth, scaleHeight;
    int useableWidth, useableHeight, textHeight, textWidth, startX, startY;
    int nChars;
    XFontStruct *font;
    char stringValue[40];
    unsigned long gcValueMask;
    XGCValues gcValues;
    DisplayInfo *displayInfo;
    double localLopr, localHopr;
    char *localTitle;
    short precision;

    if(event->count > 0) return;

    if(clientData) {
      /* Then valid controllerData exists */
	Record *pr;

	pv = (MedmValuator *) clientData;
	pr = pv->record;
	displayInfo = pv->updateTask->displayInfo;
	dlValuator = pv->dlElement->structure.valuator;
	localLopr = dlValuator->limits.lopr;
	localHopr = dlValuator->limits.hopr;
	precision = MAX(0,dlValuator->limits.prec);
	localTitle = dlValuator->control.ctrl;

    } else {
      /* No controller data, therefore userData = dlValuator */
      /* KE: (Probably) means we are in EDIT mode
       *   Might be better to branch on that */
	XtVaGetValues(w,XmNuserData,&dlValuator,NULL);
	if(dlValuator == NULL) return;
	localLopr = 0.0;
	localHopr = 0.0;
	precision = 0;
	localTitle = dlValuator->control.ctrl;
	displayInfo = dmGetDisplayInfoFromWidget(w);
	if(displayInfo == NULL) return;
    }

  /* Since XmScale doesn't really do the right things, we'll do it by hand */

    if(dlValuator->label != LABEL_NONE && dlValuator->label != NO_DECORATIONS) {

	foreground = displayInfo->colormap[dlValuator->control.clr];
	background = displayInfo->colormap[dlValuator->control.bclr];
	font = fontTable[valuatorFontListIndex(dlValuator)];
	textHeight = font->ascent + font->descent;

	gcValueMask = GCForeground | GCFont | GCBackground | GCFunction;
	gcValues.function = GXcopy;
	gcValues.foreground = foreground;
	gcValues.background = background;
	gcValues.font = font->fid;
	XChangeGC(display, displayInfo->pixmapGC, gcValueMask, &gcValues);
	XSetClipOrigin(display,displayInfo->pixmapGC,0,0);
	XSetClipMask(display,displayInfo->pixmapGC,None);

      /* KE: Value can be received before the graphical info
       *   Set precision to 0 if it is still -1 from initialization */
	if(precision < 0) precision = 0;
      /* Convert bad values of precision to high precision */
	if(precision > 17) precision = 17;
	switch (dlValuator->direction) {
	case UP:
	    XtVaGetValues(w,XmNscaleWidth,&scaleWidth,NULL);
	    useableWidth = dlValuator->object.width - scaleWidth;
	    if(dlValuator->label == OUTLINE || dlValuator->label == LIMITS
	      || dlValuator->label == CHANNEL) {
	      /* LOPR */
		cvtDoubleToString(localLopr,stringValue,precision);
		if(stringValue != NULL) {
		    nChars = strlen(stringValue);
		    textWidth = XTextWidth(font,stringValue,nChars);
		    startX = MAX(1,useableWidth - textWidth);
		    startY = dlValuator->object.height - font->descent - 3;
		    XSetForeground(display,displayInfo->pixmapGC,background);
		    XFillRectangle(display,XtWindow(w),displayInfo->pixmapGC,
		      startX,MAX(1,startY-font->ascent),
		      textWidth,font->ascent+font->descent);
		    XSetForeground(display,displayInfo->pixmapGC,foreground);
		    XDrawString(display,XtWindow(w),displayInfo->pixmapGC,startX,startY,
		      stringValue,nChars);
		}
	      /* HOPR */
		cvtDoubleToString(localHopr,stringValue,precision);
		if(stringValue != NULL) {
		    nChars = strlen(stringValue);
		    textWidth = XTextWidth(font,stringValue,nChars);
		    startX = MAX(1,useableWidth - textWidth);
		    if(dlValuator->label == CHANNEL) {
		      /* need room for label above */
			startY = (int) (1.3*(font->ascent + font->descent)
			  + font->ascent);
		    } else {
			startY = font->ascent + 3;
		    }
		    XSetForeground(display,displayInfo->pixmapGC,background);
		    XFillRectangle(display,XtWindow(w),displayInfo->pixmapGC,
		      startX,MAX(1,startY-font->ascent),
		      textWidth,font->ascent+font->descent);
		    XSetForeground(display,displayInfo->pixmapGC,foreground);
		    XDrawString(display,XtWindow(w),displayInfo->pixmapGC,startX,startY,
		      stringValue,nChars);
		}
	    }
	    if(dlValuator->label == CHANNEL) {
	      /* TITLE */
		if(localTitle != NULL) {
		    nChars = strlen(localTitle);
		    textWidth = XTextWidth(font,localTitle,nChars);
		    startX = MAX(1,useableWidth - textWidth);
		    startY = font->ascent + 2;
		    XDrawString(display,XtWindow(w),displayInfo->pixmapGC,startX,startY,
		      localTitle,nChars);
		}
	    }
	    break;
	    
	case RIGHT:
	    XtVaGetValues(w,XmNscaleHeight,&scaleHeight,NULL);
	    useableHeight = dlValuator->object.height - scaleHeight;
	    
	    if(dlValuator->label == OUTLINE || dlValuator->label == LIMITS
	      || dlValuator->label == CHANNEL) {
	      /* LOPR */
		cvtDoubleToString(localLopr,stringValue,precision);
		if(stringValue != NULL) {
		    nChars = strlen(stringValue);
		    textWidth = XTextWidth(font,stringValue,nChars);
		    startX = 2;
		    startY = useableHeight - font->descent;/* NB: descent=0 for #s */
		    XSetForeground(display,displayInfo->pixmapGC,background);
		    XFillRectangle(display,XtWindow(w),displayInfo->pixmapGC,
		      startX,MAX(1,startY-font->ascent),
		      textWidth,font->ascent+font->descent);
		    XSetForeground(display,displayInfo->pixmapGC,foreground);
		    XDrawString(display,XtWindow(w),displayInfo->pixmapGC,startX,startY,
		      stringValue,nChars);
		}
	      /* HOPR */
		cvtDoubleToString(localHopr,stringValue,precision);
		if(stringValue != NULL) {
		    nChars = strlen(stringValue);
		    textWidth = XTextWidth(font,stringValue,nChars);
		    startX = dlValuator->object.width - textWidth - 2;
		    startY = useableHeight - font->descent;
		    XSetForeground(display,displayInfo->pixmapGC,background);
		    XFillRectangle(display,XtWindow(w),displayInfo->pixmapGC,
		      startX,MAX(1,startY-font->ascent),
		      textWidth,font->ascent+font->descent);
		    XSetForeground(display,displayInfo->pixmapGC,foreground);
		    XDrawString(display,XtWindow(w),displayInfo->pixmapGC,startX,startY,
		      stringValue,nChars);
		}
	    }
	    if(dlValuator->label == CHANNEL) {
	      /* TITLE */
		if(localTitle != NULL) {
		    nChars = strlen(localTitle);
		    textWidth = XTextWidth(font,localTitle,nChars);
		    startX = 2;
		    startY = font->ascent + 2;
		    XDrawString(display,XtWindow(w),displayInfo->pixmapGC,startX,startY,
		      localTitle,nChars);
		}
	    }
	    break;
	}
	if(clientData != NULL) {
	  /* Real data */
	    valuatorRedrawValue(pv,
	      pv->updateTask->displayInfo,pv->dlElement->widget,
	      pv->dlElement->structure.valuator, pv->record->value);
	    
	    
	} else {
	  /* Fake data */
	    valuatorRedrawValue(NULL,displayInfo,w,dlValuator,0.0);
	}
    }     /* End of if(dlValuator->label != LABEL_NONE) */
}

/*
 * Set value (with implicit redraw of value) for valuator
 */
static void valuatorSetValue(MedmValuator *pv, double forcedValue,
  Boolean force)
{
    int iValue;
    double dValue;
    DlValuator *dlValuator = pv->dlElement->structure.valuator;
    Record *pr = pv->record;

    if(dlValuator->limits.hopr != dlValuator->limits.lopr) {
	if(force)
	  dValue = forcedValue;
	else
	  dValue = pr->value;
#if 0
      /* To make reworked event handling for Valuator work */
	pr->value = dValue;
#endif
	
      /* Update scale widget */
	iValue = (int) (VALUATOR_MIN + ((dValue - dlValuator->limits.lopr)
	  /(dlValuator->limits.hopr - dlValuator->limits.lopr))
	  *((double)(VALUATOR_MAX - VALUATOR_MIN)));
	pv->oldIntegerValue = iValue;
	XtVaSetValues(pv->dlElement->widget,XmNvalue,iValue,NULL);
	valuatorRedrawValue(pv,pv->updateTask->displayInfo,
	  pv->dlElement->widget,
	  pv->dlElement->structure.valuator,dValue);
    }
}

/*
 * Redraw value for valuator
 */
static void valuatorRedrawValue(MedmValuator *pv, DisplayInfo *displayInfo,
  Widget w, DlValuator *dlValuator, double value)
{
    unsigned long foreground, background;
    Dimension scaleWidth, scaleHeight;
    int useableWidth, useableHeight, textHeight, textWidth, startX, startY;
    int nChars;
    XFontStruct *font;
    char stringValue[40];
    unsigned long gcValueMask;
    XGCValues gcValues;
    short precision;
    int x, y, height, width;
    
  /* Return if no window for widget yet, or if displayInfo == NULL, or ... */
    if(XtWindow(w) == (Window)NULL || displayInfo == (DisplayInfo *)NULL ||
      dlValuator == (DlValuator *)NULL || pv == NULL) return;
    
  /* Simply return if no value to render */
    if(!(dlValuator->label == LIMITS || dlValuator->label == CHANNEL)) return;
    
    foreground = displayInfo->colormap[dlValuator->control.clr];
    background = displayInfo->colormap[dlValuator->control.bclr];
    if(dlValuator->limits.prec >= 0) {
	precision = dlValuator->limits.prec;
	font = fontTable[pv->fontIndex];
	if(dlValuator->clrmod == ALARM)
	  foreground = alarmColor(pv->record->severity);
    } else {
	precision = 0;
	font = fontTable[valuatorFontListIndex(dlValuator)];
    }

    textHeight = font->ascent + font->descent;
  /* Convert bad values of precision to high precision */
    if(precision < 0 || precision > 17) precision=17;
    cvtDoubleToString(value,stringValue,precision);
    nChars = strlen(stringValue);

    switch (dlValuator->direction) {
    case UP:
	XtVaGetValues(w, XmNscaleWidth,&scaleWidth, NULL);
	useableWidth = dlValuator->object.width - scaleWidth;
	textWidth = XTextWidth(font,stringValue,nChars);
	startX = MAX(1,useableWidth - textWidth);
	startY = (dlValuator->object.height + font->ascent)/2;

	x =  0;
	y =  MAX(1,startY - font->ascent);
	width = useableWidth;
	height = font->ascent+font->descent;
	break;
    case RIGHT:
	XtVaGetValues(w,XmNscaleHeight,&scaleHeight,NULL);
	useableHeight = dlValuator->object.height - scaleHeight;
	textWidth = XTextWidth(font,stringValue,nChars);
	startX = dlValuator->object.width/2 - textWidth/2;
	startY = useableHeight - font->descent;

	x = dlValuator->object.width/3,
	  y = startY - font->ascent,
	  width = dlValuator->object.width/3,
	  height = font->ascent+font->descent;
	break;
    }  /* end switch() */

  /* Set up the graphic context */
    gcValueMask = GCForeground | GCFont | GCFunction;
    gcValues.function = GXcopy;
    gcValues.foreground = background;
    gcValues.font = font->fid;
    XChangeGC(XtDisplay(w), displayInfo->pixmapGC, gcValueMask, &gcValues);
  /* Fill the background */
    XFillRectangle(XtDisplay(w),
      XtWindow(w),
      displayInfo->pixmapGC,
      x,y,
      width,height);
  /* Draw the string */
    XSetForeground(XtDisplay(w),
      displayInfo->pixmapGC,
      foreground);
    XDrawString(XtDisplay(w),
      XtWindow(w),
      displayInfo->pixmapGC,
      startX,startY,
      stringValue,nChars);
}

/*
 * Thanks to complicated valuator interactions, need to rely on
 *    Key/ButtonRelease events to re-enable updates for dlValuator display
 */
#ifdef __cplusplus
static void handleValuatorRelease(Widget, XtPointer passedData, XEvent *event,
  Boolean *)
#else
static void handleValuatorRelease(Widget w, XtPointer passedData, XEvent *event,
  Boolean *continueToDispatch)
#endif
{
    MedmValuator *pv = (MedmValuator *) passedData;
    DlValuator *dlValuator = pv->dlElement->structure.valuator;

    switch(event->type) {
    case ButtonRelease:
    case KeyRelease:
	dlValuator->enableUpdates = True;
	updateTaskMarkUpdate(pv->updateTask);
      /* Don't reset ->dragging: let valuatorValueChanged() do that */
	break;
    }
}

static void precisionToggleChangedCallback(Widget w, XtPointer clientData,
  XtPointer cbs)
{
    Widget widget;
    long value;
    MedmValuator *pv;
    XmToggleButtonCallbackStruct *call_data = (XmToggleButtonCallbackStruct *) cbs;
    
  /* Only respond to the button actually set */
    if(call_data->event && call_data->set) {
	value = (long)clientData;
	XtVaGetValues(w,XmNuserData,&pv,NULL);
      /* Set the prec field in the valuator data structure, and update
       *    the valuator (scale) resources */
	if(pv) {
	    pv->dlElement->structure.valuator->dPrecision = pow(10.,(double)value);
	}
      /* Destroy the popup
       * Hierarchy = TB<-RB<-Frame<-SelectionBox<-Dialog */
	widget = w;
	while(XtClass(widget) != xmDialogShellWidgetClass) {
	    widget = XtParent(widget);
	}
	XtDestroyWidget(widget);
    }
}

/*
 * Text field processing callback
 */
#ifdef __cplusplus
static void precTextFieldActivateCallback(Widget w, XtPointer cd, XtPointer)
#else
static void precTextFieldActivateCallback(Widget w, XtPointer cd, XtPointer cbs)
#endif
{
    DlValuator *dlValuator = (DlValuator *) cd;
    char *stringValue;
    Widget widget;

    stringValue = XmTextFieldGetString(w);
    dlValuator->dPrecision = atof(stringValue);
    XtFree(stringValue);

  /* Destroy the popup
   * Hierarchy = TB<-RB<-Frame<-SelectionBox<-Dialog */
    widget = w;
    while(XtClass(widget) != xmDialogShellWidgetClass) {
	widget = XtParent(widget);
    }
    XtDestroyWidget(widget);
}

/*
 * Text field losing focus callback
 */
#ifdef __cplusplus
static void precTextFieldLosingFocusCallback(
  Widget w,
  XtPointer cd,
  XtPointer)
#else
static void precTextFieldLosingFocusCallback(
  Widget w,
  XtPointer cd,
  XtPointer cbs)
#endif
{
    DlValuator *dlValuator = (DlValuator *) cd;
    char string[MAX_TOKEN_LENGTH];
    int tail;

  /* Losing focus - make sure that the text field remains accurate
   *   wrt dlValuator */
    sprintf(string,"%f",dlValuator->dPrecision);
  /* Strip trailing zeroes */
    tail = strlen(string);
    while(string[--tail] == '0') string[tail] = '\0';
    XmTextFieldSetString(w,string);
}

static void keyboardDialogCallback(Widget w, XtPointer clientData,
  XtPointer callbackStruct)
{
    XmSelectionBoxCallbackStruct *call_data =
      (XmSelectionBoxCallbackStruct *)callbackStruct;

    switch(call_data->reason) {
    case XmCR_OK: {
	MedmValuator *pv = (MedmValuator *)clientData;
	Record *pr = pv->record;
	double value;
	char *stringValue;
	
	if(pv == NULL) return;
	
	XmStringGetLtoR(call_data->value,XmFONTLIST_DEFAULT_TAG,&stringValue);
	if(stringValue != NULL) {
	    value = atof(stringValue);
	    
	  /* Move/redraw valuator & value, force use of user-selected value */
	    if((pr->connected) && pr->writeAccess) {
		medmSendDouble(pv->record,value);
		valuatorSetValue(pv,value,True);
	    }
	    XtFree(stringValue);
	}
	XtDestroyWidget(XtParent(w));
	break;
    }

    case XmCR_CANCEL:
	XtDestroyWidget(XtParent(w));
	break;

    case XmCR_HELP:
	callBrowser(MEDM_HELP_PATH"/MEDM.html#Slider");
	break;
    }
}

#ifdef __cplusplus
void popupValuatorKeyboardEntry(Widget w, DisplayInfo *, XEvent *event)
#else
void popupValuatorKeyboardEntry(Widget w, DisplayInfo *displayInfo, XEvent *event)
#endif
{
#define MAX_TOGGLES 20
    Widget keyboardDialog;
    char valueLabel[MAX_TOKEN_LENGTH + 8];
    XmString xmTitle, xmValueLabel, valueXmString;
    char valueString[40];
    char *channel;
    Arg args[10];
    int n;
    MedmValuator *pv;
    Record   *pr;

    Widget frame, frameLabel, radioBox, toggles[MAX_TOGGLES];
    Widget form, textField;
    XmString frameXmString, toggleXmString;
    double hoprLoprAbs;
    short numColumns, numPlusColumns, numMinusColumns, shortValue;
    char toggleString[4];
    int i, count, tail;
    long longValue;
    DlValuator *dlValuator;

    XButtonEvent *xEvent = (XButtonEvent *)event;

    if(globalDisplayListTraversalMode == DL_EDIT) return;
    if(xEvent->button != Button3) return;
    
    XtVaGetValues(w,XmNuserData,&pv,NULL);
    if(!pv) return;

    pr = pv->record;
    channel = pv->dlElement->structure.valuator->control.ctrl;
    if(!pr->connected || !pr->writeAccess || !*channel) return;
    
  /* Create selection box/prompt dialog */
    strcpy(valueLabel,"VALUE: ");
    strcat(valueLabel,channel);
    xmValueLabel = XmStringCreateLocalized(valueLabel);
    xmTitle = XmStringCreateLocalized(channel);
    dlValuator = pv->dlElement->structure.valuator;
    cvtDoubleToString(pr->value,valueString,
      dlValuator->limits.prec);
    valueXmString = XmStringCreateLocalized(valueString);
    n = 0;
    XtSetArg(args[n],XmNdialogStyle,
      XmDIALOG_PRIMARY_APPLICATION_MODAL); n++;
    XtSetArg(args[n],XmNselectionLabelString,xmValueLabel); n++;
    XtSetArg(args[n],XmNdialogTitle,xmTitle); n++;
    XtSetArg(args[n],XmNtextString,valueXmString); n++;
    keyboardDialog = XmCreatePromptDialog(w,channel,args,n);
    
  /* Remove resize handles from shell */
    XtVaSetValues(XtParent(keyboardDialog),
      XmNmwmDecorations,MWM_DECOR_ALL|MWM_DECOR_RESIZEH,
      NULL);
    
    XtAddCallback(keyboardDialog,XmNokCallback,keyboardDialogCallback,pv);
    XtAddCallback(keyboardDialog,XmNhelpCallback,keyboardDialogCallback,NULL);
    XtAddCallback(keyboardDialog,XmNcancelCallback,keyboardDialogCallback,NULL);
    
  /* Create frame/radiobox/toggles for increment selection */
    hoprLoprAbs = fabs(dlValuator->limits.hopr);
    hoprLoprAbs = MAX(hoprLoprAbs,fabs(dlValuator->limits.lopr));
  /* log10 + 1 */
    numPlusColumns =  (short)log10(hoprLoprAbs) + 1;
    numMinusColumns = (short)dlValuator->limits.prec;
  /* leave room for decimal point */
    numColumns = numPlusColumns + 1 + numMinusColumns;
    if(numColumns > MAX_TOGGLES) {
	numColumns = MAX_TOGGLES;
	if(numMinusColumns < MAX_TOGGLES) {
	    numPlusColumns = numColumns - 1 - numMinusColumns;
	} else {
	    numMinusColumns = numMinusColumns - numColumns - 1;
	}
    }
    n = 0;
    frame = XmCreateFrame(keyboardDialog,"frame",args,n);
    frameXmString = XmStringCreateLocalized("Increment (Buttons set 10^N)");
    XtSetArg(args[n],XmNlabelString,frameXmString); n++;
    XtSetArg(args[n],XmNchildType,XmFRAME_TITLE_CHILD); n++;
    frameLabel = XmCreateLabel(frame,"frameLabel",args,n);
    XtManageChild(frameLabel);
    
    n = 0;
    XtSetArg(args[n],XmNchildType,XmFRAME_WORKAREA_CHILD); n++;
    XtSetArg(args[n],XmNshadowThickness,0); n++;
    form = XmCreateForm(frame,"form",args,n);
    
  /* Radio box */
    n = 0;
    XtSetArg(args[n],XmNtopAttachment,XmATTACH_FORM); n++;
    XtSetArg(args[n],XmNnumColumns,numColumns); n++;
    XtSetArg(args[n],XmNorientation,XmVERTICAL); n++;
    XtSetArg(args[n],XmNisAligned,True); n++;
    XtSetArg(args[n],XmNentryAlignment,XmALIGNMENT_CENTER); n++;
    XtSetArg(args[n],XmNadjustLast,False); n++;
    XtSetArg(args[n],XmNspacing,0); n++;
    radioBox = XmCreateRadioBox(form,"radioBox",args,n);
    
    toggleXmString = (XmString)NULL;
    XtSetArg(args[0],XmNindicatorOn,False);
  /* Digits to the left of the decimal point */
    count = 0;
    for(i = numPlusColumns - 1; i >= 0; i--) {
	if(toggleXmString != NULL) XmStringFree(toggleXmString);
	shortValue = (short)i;
	cvtShortToString(shortValue,toggleString);
	toggleXmString = XmStringCreateLocalized(toggleString);
	XtSetArg(args[1],XmNlabelString,toggleXmString);
	XtSetArg(args[2],XmNuserData,(XtPointer) pv);
	if(log10(dlValuator->dPrecision) == (double)i) {
	    XtSetArg(args[3],XmNset,True);
	}
	toggles[count++] = XmCreateToggleButton(radioBox,"toggles",args,
	  (log10(dlValuator->dPrecision) == (double)i ? 4 : 3));
	longValue = (long)shortValue;
	XtAddCallback(toggles[count-1],XmNvalueChangedCallback,
	  precisionToggleChangedCallback,(XtPointer)longValue);
    }
  /* The decimal point */
    if(toggleXmString != NULL) XmStringFree(toggleXmString);
    toggleString[0] = '.'; toggleString[1] = '\0';
    toggleXmString = XmStringCreateLocalized(toggleString);
    XtSetArg(args[1],XmNlabelString,toggleXmString);
    XtSetArg(args[2],XmNshadowThickness,0);
    toggles[count++] = XmCreateToggleButton(radioBox,"toggles",args,3);
    XtSetSensitive(toggles[count-1],False);
    
  /* Digits to the right of the decimal point */
    for(i = 1; i <= numMinusColumns; i++) {
	if(toggleXmString != NULL) XmStringFree(toggleXmString);
	shortValue = (short)-i;
	cvtShortToString(shortValue,toggleString);
	toggleXmString = XmStringCreateLocalized(toggleString);
	XtSetArg(args[1],XmNlabelString,toggleXmString);
	XtSetArg(args[2],XmNuserData,(XtPointer) pv);
	if(log10(dlValuator->dPrecision) == (double)-i) {
	    XtSetArg(args[3],XmNset,True);
	}
	toggles[count++] = XmCreateToggleButton(radioBox,"toggles",args,
	  (log10(dlValuator->dPrecision) == (double)-i ? 4 : 3));
	longValue = (long)shortValue;
	XtAddCallback(toggles[count-1],XmNvalueChangedCallback,
	  precisionToggleChangedCallback, (XtPointer)longValue);
    }
    
  /* Text field */
    n = 0;
    XtSetArg(args[n],XmNtopAttachment,XmATTACH_WIDGET); n++;
    XtSetArg(args[n],XmNleftAttachment,XmATTACH_FORM); n++;
    XtSetArg(args[n],XmNrightAttachment,XmATTACH_FORM); n++;
    XtSetArg(args[n],XmNbottomAttachment,XmATTACH_FORM); n++;
    XtSetArg(args[n],XmNtopWidget,radioBox); n++;
    textField = XmCreateTextField(form,"textField",args,n);
    XtAddCallback(textField,XmNactivateCallback,
      precTextFieldActivateCallback,(XtPointer)dlValuator);
    XtAddCallback(textField,XmNlosingFocusCallback,
      precTextFieldLosingFocusCallback,(XtPointer)dlValuator);
    XtAddCallback(textField,XmNmodifyVerifyCallback,
      textFieldFloatVerifyCallback, NULL);
    sprintf(valueString,"%f",dlValuator->dPrecision);
  /* Strip trailing zeroes */
    tail = strlen(valueString);
    while(valueString[--tail] == '0') valueString[tail] = '\0';
    XmTextFieldSetString(textField,valueString);
    
  /* Manage everything */
    XtManageChildren(toggles,numColumns);
    XtManageChild(radioBox);
    XtManageChild(textField);
    XtManageChild(form);
    XtManageChild(frame);
    if(toggleXmString != NULL) XmStringFree(toggleXmString);
    XmStringFree(frameXmString);
    
    XtManageChild(keyboardDialog);
    XmStringFree(xmValueLabel);
    XmStringFree(xmTitle);
    XmStringFree(valueXmString);
}

/*
 * Valuatorvaluechanged - drag and value changed callback for valuator
 */

#ifdef __cplusplus
void valuatorValueChanged(Widget, XtPointer clientData,
  XtPointer callbackStruct)
#else
void valuatorValueChanged(Widget  w, XtPointer clientData,
  XtPointer callbackStruct)
#endif
{
    MedmValuator *pv = (MedmValuator *) clientData;
    Record *pr = pv->record;
    XmScaleCallbackStruct *call_data = (XmScaleCallbackStruct *) callbackStruct;
    DlValuator *dlValuator = (DlValuator *) pv->dlElement->structure.valuator;
    XButtonEvent *buttonEvent;
    XKeyEvent *keyEvent;
    double value;

    if(pr->connected) {

      /* Set modified flag on monitor data so that next update traversal will
       *   set controller visual state correctly (noting that for controllers
       *   as monitors the ->modified flag alone is used to do updates */

	value = pr->value;
	if(call_data->reason == XmCR_DRAG) {
	    dlValuator->dragging = True;            /* Mark beginning of drag  */
	    dlValuator->enableUpdates = False;      /* Disable updates in drag */
	    
	  /* Drag - set value based on relative position (easy) */
	    pv->oldIntegerValue = call_data->value;
	    value = dlValuator->limits.lopr + ((double)(call_data->value - VALUATOR_MIN))
	      /((double)(VALUATOR_MAX - VALUATOR_MIN) )*(dlValuator->limits.hopr - dlValuator->limits.lopr);
	    
	} else if(call_data->reason == XmCR_VALUE_CHANGED) { 
	    if(dlValuator->dragging) {
	      /* Valuechanged can mark conclusion of drag, hence enable updates */
		dlValuator->enableUpdates = True;
		dlValuator->dragging = False;
	    } else {
	      /* Rely on Button/KeyRelease event handler to re-enable updates */
		dlValuator->enableUpdates = False;
		dlValuator->dragging = False;
	    }
	    
	  /* Value changed - has to deal with precision, etc (hard) */
	    if(call_data->event != NULL) {
		if(call_data->event->type == KeyPress) {
		    keyEvent = (XKeyEvent *)call_data->event;
		    if(keyEvent->state & ControlMask) {
		      /* Multiple increment (10*precision) */
			if(pv->oldIntegerValue > call_data->value) {
			  /* Decrease value one 10*precision value */
			    value = MAX(dlValuator->limits.lopr, pr->value - 10.*dlValuator->dPrecision);
			} else if(pv->oldIntegerValue < call_data->value) {
			  /* Increase value one 10*precision value */
			    value = MIN(dlValuator->limits.hopr, pr->value + 10.*dlValuator->dPrecision);
			}
		    } else {
		      /* Single increment (precision) */
			if(pv->oldIntegerValue > call_data->value) {
			  /* Decrease value one precision value */
			    value = MAX(dlValuator->limits.lopr, pr->value - dlValuator->dPrecision);
			} else if(pv->oldIntegerValue < call_data->value) {
			  /* Increase value one precision value */
			    value = MIN(dlValuator->limits.hopr, pr->value + dlValuator->dPrecision);
			}
		    }
		} else if(call_data->event->type == ButtonPress) {
		    buttonEvent = (XButtonEvent *)call_data->event;
		    if(buttonEvent->state & ControlMask) {
		      /* Handle this as multiple increment/decrement */
			if(call_data->value - pv->oldIntegerValue < 0) {
			  /* Decrease value one 10*precision value */
			    value = MAX(dlValuator->limits.lopr, pr->value - 10.*dlValuator->dPrecision);
			} else if(call_data->value - pv->oldIntegerValue > 0) {
				/* Increase value one 10*precision value */
			    value = MIN(dlValuator->limits.hopr, pr->value + 10.*dlValuator->dPrecision);
			}
		    } else {
		      /* Single increment (precision) */
			if(pv->oldIntegerValue > call_data->value) {
			  /* decrease value one precision value */
			    value = MAX(dlValuator->limits.lopr, pr->value - dlValuator->dPrecision);
			} else if(pv->oldIntegerValue < call_data->value) {
				/* Increase value one precision value */
			    value = MIN(dlValuator->limits.hopr, pr->value + dlValuator->dPrecision);
			}
		    }
		}  /* end if/else (KeyPress/ButtonPress) */
	    } else {
	      /* Handle null events (direct MB1, etc does this)
	       *   (MDA) modifying valuator to depart somewhat from XmScale, but more
	       *   useful for real application (of valuator)
	       * NB: modifying - MB1 either side of slider means move one increment only;
	       *   even though std. is Multiple (let Ctrl-MB1 mean multiple (not go-to-end))
	       */
		if(call_data->value - pv->oldIntegerValue < 0) {
		  /* Decrease value one precision value */
		    value = MAX(dlValuator->limits.lopr, pr->value - dlValuator->dPrecision);
		} else if(call_data->value - pv->oldIntegerValue > 0) {
		  /* Increase value one precision value */
		    value = MIN(dlValuator->limits.hopr, pr->value + dlValuator->dPrecision);
		}
	    }  /* end if(call_data->event != NULL) */
	}
	
	if(pr->writeAccess) {
	    medmSendDouble(pv->record,value);
	  /* Move/redraw valuator & value, but force use of user-selected value */
	    valuatorSetValue(pv,value,True);
	} else {
	    XBell(display,50); XBell(display,50); XBell(display,50);
	    valuatorSetValue(pv,pr->value,True);
	}
    }
}

static void valuatorGetRecord(XtPointer cd, Record **record, int *count)
{
    MedmValuator *pv = (MedmValuator *) cd;
    *count = 1;
    record[0] = pv->record;
}

DlElement *createDlValuator(DlElement *p)
{
    DlValuator *dlValuator;
    DlElement *dlElement;
 
    dlValuator = (DlValuator *)malloc(sizeof(DlValuator));
    if(!dlValuator) return 0;
    if(p) {
	*dlValuator = *(p->structure.valuator);
    } else {
	objectAttributeInit(&(dlValuator->object));
	controlAttributeInit(&(dlValuator->control));
	limitsAttributeInit(&(dlValuator->limits));
	dlValuator->label = LABEL_NONE;
	dlValuator->clrmod = STATIC;
	dlValuator->direction = RIGHT;
	dlValuator->dPrecision = 1.;
    }
 
  /* Private run-time valuator field */
    dlValuator->enableUpdates = True;
    dlValuator->dragging = False;

    if(!(dlElement = createDlElement(DL_Valuator,
      (XtPointer)dlValuator,
      &valuatorDlDispatchTable))) {
	free(dlValuator);
    }
 
    return(dlElement);
}

DlElement *parseValuator(DisplayInfo *displayInfo)
{
    char token[MAX_TOKEN_LENGTH];
    TOKEN tokenType;
    int nestingLevel = 0;
    DlValuator *dlValuator;
    DlElement *dlElement = createDlValuator(NULL);
    if(!dlElement) return 0;
    dlValuator = dlElement->structure.valuator;

    do {
	switch((tokenType=getToken(displayInfo,token)) ) {
	case T_WORD:
	    if(!strcmp(token,"object"))
	      parseObject(displayInfo,&(dlValuator->object));
	    else if(!strcmp(token,"control"))
	      parseControl(displayInfo,&(dlValuator->control));
	    else if(!strcmp(token,"label")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		if(!strcmp(token,"none"))
		  dlValuator->label = LABEL_NONE;
		else if(!strcmp(token,"no decorations"))
		  dlValuator->label = NO_DECORATIONS;
		else if(!strcmp(token,"outline"))
		  dlValuator->label = OUTLINE;
		else if(!strcmp(token,"limits"))
		  dlValuator->label = LIMITS;
		else if(!strcmp(token,"channel"))
		  dlValuator->label = CHANNEL;
	    } else if(!strcmp(token,"clrmod")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		if(!strcmp(token,"static")) 
		  dlValuator->clrmod = STATIC;
		else if(!strcmp(token,"alarm"))
		  dlValuator->clrmod = ALARM;
		else if(!strcmp(token,"discrete"))
		  dlValuator->clrmod = DISCRETE;
	    } else  if(!strcmp(token,"direction")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		if(!strcmp(token,"up")) 
		  dlValuator->direction = UP;
		else if(!strcmp(token,"right"))
		  dlValuator->direction = RIGHT;
	      /* Backward compatibility */
		else if(!strcmp(token,"down"))
		  dlValuator->direction = UP;
		else if(!strcmp(token,"left"))
		  dlValuator->direction = RIGHT;
	    } else if(!strcmp(token,"dPrecision")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		dlValuator->dPrecision = atof(token);
	    } else if(!strcmp(token,"limits")) {
		parseLimits(displayInfo,&(dlValuator->limits));
	    }
	    break;
	case T_EQUAL:
	    break;
	case T_LEFT_BRACE:
	    nestingLevel++; break;
	case T_RIGHT_BRACE:
	    nestingLevel--; break;
	}
    } while((tokenType != T_RIGHT_BRACE) && (nestingLevel > 0)
      && (tokenType != T_EOF) );
    
    return dlElement;
}

void writeDlValuator(FILE *stream, DlElement *dlElement, int level)
{
    char indent[16];
    DlValuator *dlValuator = dlElement->structure.valuator;

    memset(indent,'\t',level);
    indent[level] = '\0';

#ifdef SUPPORT_0201XX_FILE_FORMAT
    if(MedmUseNewFileFormat) {
#endif 
	fprintf(stream,"\n%svaluator {",indent);
	writeDlObject(stream,&(dlValuator->object),level+1);
	writeDlControl(stream,&(dlValuator->control),level+1);
	if(dlValuator->label != LABEL_NONE) 
	  fprintf(stream,"\n%s\tlabel=\"%s\"",indent,
	    stringValueTable[dlValuator->label]);
	if(dlValuator->clrmod != STATIC) 
	  fprintf(stream,"\n%s\tclrmod=\"%s\"",indent,
	    stringValueTable[dlValuator->clrmod]);
	if(dlValuator->direction != RIGHT)
	  fprintf(stream,"\n%s\tdirection=\"%s\"",indent,
	    stringValueTable[dlValuator->direction]);
	if(dlValuator->direction != 1.) 
	  fprintf(stream,"\n%s\tdPrecision=%f",indent,dlValuator->dPrecision);
	writeDlLimits(stream,&(dlValuator->limits),level+1);
	fprintf(stream,"\n%s}",indent);
#ifdef SUPPORT_0201XX_FILE_FORMAT
    } else {
	fprintf(stream,"\n%svaluator {",indent);
	writeDlObject(stream,&(dlValuator->object),level+1);
	writeDlControl(stream,&(dlValuator->control),level+1);
	fprintf(stream,"\n%s\tlabel=\"%s\"",indent,
	  stringValueTable[dlValuator->label]);
	fprintf(stream,"\n%s\tclrmod=\"%s\"",indent,
	  stringValueTable[dlValuator->clrmod]);
	fprintf(stream,"\n%s\tdirection=\"%s\"",indent,
	  stringValueTable[dlValuator->direction]);
	fprintf(stream,"\n%s\tdPrecision=%f",indent,dlValuator->dPrecision);
	writeDlLimits(stream,&(dlValuator->limits),level+1);
	fprintf(stream,"\n%s}",indent);
    }
#endif
}

static void valuatorInheritValues(ResourceBundle *pRCB, DlElement *p)
{
    DlValuator *dlValuator = p->structure.valuator;
    medmGetValues(pRCB,
      CTRL_RC,       &(dlValuator->control.ctrl),
      CLR_RC,        &(dlValuator->control.clr),
      BCLR_RC,       &(dlValuator->control.bclr),
      LABEL_RC,      &(dlValuator->label),
      DIRECTION_RC,  &(dlValuator->direction),
      CLRMOD_RC,     &(dlValuator->clrmod),
      PRECISION_RC,  &(dlValuator->dPrecision),
      LIMITS_RC,     &(dlValuator->limits),
      -1);
}

static void valuatorGetLimits(DlElement *pE, DlLimits **ppL, char **pN)
{
    DlValuator *dlValuator = pE->structure.valuator;

    *(ppL) = &(dlValuator->limits);
    *(pN) = dlValuator->control.ctrl;
}

static void valuatorGetValues(ResourceBundle *pRCB, DlElement *p)
{
    DlValuator *dlValuator = p->structure.valuator;
    medmGetValues(pRCB,
      X_RC,          &(dlValuator->object.x),
      Y_RC,          &(dlValuator->object.y),
      WIDTH_RC,      &(dlValuator->object.width),
      HEIGHT_RC,     &(dlValuator->object.height),
      CTRL_RC,       &(dlValuator->control.ctrl),
      CLR_RC,        &(dlValuator->control.clr),
      BCLR_RC,       &(dlValuator->control.bclr),
      LABEL_RC,      &(dlValuator->label),
      DIRECTION_RC,  &(dlValuator->direction),
      CLRMOD_RC,     &(dlValuator->clrmod),
      PRECISION_RC,  &(dlValuator->dPrecision),
      LIMITS_RC,     &(dlValuator->limits),
      -1);
}

static void valuatorSetBackgroundColor(ResourceBundle *pRCB, DlElement *p)
{
    DlValuator *dlValuator = p->structure.valuator;
    medmGetValues(pRCB,
      BCLR_RC,       &(dlValuator->control.bclr),
      -1);
}

static void valuatorSetForegroundColor(ResourceBundle *pRCB, DlElement *p)
{
    DlValuator *dlValuator = p->structure.valuator;
    medmGetValues(pRCB,
      CLR_RC,        &(dlValuator->control.clr),
      -1);
}
