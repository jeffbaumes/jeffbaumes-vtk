/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDistanceWidget.h,v

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkDistanceWidget - measure the distance between two points
// .SECTION Description
// The vtkDistanceWidget is used to measure the distance between two points.
// The two end points can be positioned independently, and when they are
// released, a special PlacePointEvent is invoked so that special operations
// may be take to reposition the point (snap to grid, etc.) The widget has
// two different modes of interaction: when initially defined (i.e., placing
// the two points) and then a manipulate mode (adjusting the position of
// the two points).
//
// To use this widget, specify an instance of vtkDistanceWidget and a
// representation (a subclass of vtkDistanceRepresentation). The widget is
// implemented using two instances of vtkHandleWidget which are used to
// position the end points of the line. The representations for these two
// handle widgets are provided by the vtkDistanceRepresentation.
//
// .SECTION Event Bindings
// By default, the widget responds to the following VTK events (i.e., it
// watches the vtkRenderWindowInteractor for these events):
// <pre>
//   LeftButtonPressEvent - add a point or select a handle
//   MouseMoveEvent - position the second point or move a handle
//   LeftButtonReleaseEvent - release the handle
// </pre>
//
// Note that the event bindings described above can be changed using this
// class's vtkWidgetEventTranslator. This class translates VTK events
// into the vtkDistanceWidget's widget events:
// <pre>
//   vtkWidgetEvent::AddPoint -- add one point; depending on the state
//                               it may the first or second point added. Or,
//                               if near a handle, select the handle.
//   vtkWidgetEvent::Move -- move the second point or handle depending on the state.
//   vtkWidgetEvent::EndSelect -- the handle manipulation process has completed.
// </pre>
//
// This widget invokes the following VTK events on itself (which observers
// can listen for):
// <pre>
//   vtkCommand::StartInteractionEvent (beginning to interact)
//   vtkCommand::EndInteractionEvent (completing interaction)
//   vtkCommand::InteractionEvent (moving after selecting something)
//   vtkCommand::PlacePointEvent (after point is positioned;
//                                call data includes handle id (0,1))
// </pre>

// .SECTION See Also
// vtkHandleWidget


#ifndef __vtkDistanceWidget_h
#define __vtkDistanceWidget_h

#include "vtkAbstractWidget.h"

class vtkDistanceRepresentation;
class vtkHandleWidget;
class vtkDistanceWidgetCallback;


class VTK_WIDGETS_EXPORT vtkDistanceWidget : public vtkAbstractWidget
{
public:
  // Description:
  // Instantiate this class.
  static vtkDistanceWidget *New();

  // Description:
  // Standard methods for a VTK class.
  vtkTypeMacro(vtkDistanceWidget,vtkAbstractWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The method for activiating and deactiviating this widget. This method
  // must be overridden because it is a composite widget and does more than
  // its superclasses' vtkAbstractWidget::SetEnabled() method.
  virtual void SetEnabled(int);

  // Description:
  // Specify an instance of vtkWidgetRepresentation used to represent this
  // widget in the scene. Note that the representation is a subclass of vtkProp
  // so it can be added to the renderer independent of the widget.
  void SetRepresentation(vtkDistanceRepresentation *r)
    {this->Superclass::SetWidgetRepresentation(
      reinterpret_cast<vtkWidgetRepresentation*>(r));}

  // Description:
  // Create the default widget representation if one is not set.
  void CreateDefaultRepresentation();

  // Description:
  // Methods to change the whether the widget responds to interaction.
  // Overridden to pass the state to component widgets.
  virtual void SetProcessEvents(int);

  // Description:
  // Set the state of the widget to "defined" (in case its widget and its
  // representation were initialized programmatically). This must generally
  // be followed by a Render() for things to visually take effect.
  virtual void WidgetIsDefined();

  // Description:
  // Has the widget been defined completely yet ? ie. Have the end points been
  // laid and is it in Manipulate mode ?
  virtual int IsWidgetDefined();

protected:
  vtkDistanceWidget();
  ~vtkDistanceWidget();

  // The state of the widget
//BTX
  enum {Start=0,Define,Manipulate};
//ETX
  int WidgetState;
  int CurrentHandle;

  // Callback interface to capture events when
  // placing the widget.
  static void AddPointAction(vtkAbstractWidget*);
  static void MoveAction(vtkAbstractWidget*);
  static void EndSelectAction(vtkAbstractWidget*);

  // The positioning handle widgets
  vtkHandleWidget *Point1Widget;
  vtkHandleWidget *Point2Widget;
  vtkDistanceWidgetCallback *DistanceWidgetCallback1;
  vtkDistanceWidgetCallback *DistanceWidgetCallback2;

  // Methods invoked when the handles at the
  // end points of the widget are manipulated
  void StartDistanceInteraction(int handleNum);
  void DistanceInteraction(int handleNum);
  void EndDistanceInteraction(int handleNum);

//BTX
  friend class vtkDistanceWidgetCallback;
//ETX

private:
  vtkDistanceWidget(const vtkDistanceWidget&);  //Not implemented
  void operator=(const vtkDistanceWidget&);  //Not implemented
};

#endif
