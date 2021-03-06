/*=========================================================================

   Program: ParaView
   Module:    PrismDisplayProxyEditor.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

// this include
#include "PrismDisplayProxyEditor.h"
#include "ui_PrismDisplayProxyEditor.h"

// Qt includes
#include <QDoubleValidator>
#include <QFileInfo>
#include <QIcon>
#include <QIntValidator>
#include <QKeyEvent>
#include <QMetaType>
#include <QPointer>
#include <QtDebug>
#include <QTimer>

// ParaView Server Manager includes
#include "vtkEventQtSlotConnect.h"
#include "vtkLabeledDataMapper.h"
#include "vtkMaterialLibrary.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkSMLookupTableProxy.h"
#include "vtkSMProperty.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMPVRepresentationProxy.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMCompositeTreeDomain.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMInputProperty.h"
#include "vtkSMProxyManager.h"
#include "vtkSMPropertyHelper.h"

// ParaView widget includes
#include "pqSignalAdaptors.h"

// ParaView client includes
#include "pqApplicationCore.h"
#include "pqColorScaleEditor.h"
#include "pqCoreUtilities.h"
#include "pqFileDialog.h"
#include "pqOutputPort.h"
#include "pqPipelineRepresentation.h"
#include "pqPipelineSource.h"
#include "pqPropertyLinks.h"
#include "pqRenderView.h"
#include "pqScalarsToColors.h"
#include "pqSignalAdaptorCompositeTreeWidget.h"
#include "pqSMAdaptor.h"
#include "pqStandardColorLinkAdaptor.h"
#include "pqUndoStack.h"
#include "pqWidgetRangeDomain.h"
#include "pqObjectBuilder.h"
#include "PrismCubeAxesEditorDialog.h"

#include "PrismCore.h"
class PrismDisplayProxyEditorInternal : public Ui::PrismDisplayProxyEditor
{
public:
  PrismDisplayProxyEditorInternal()
    {
    this->Links = new pqPropertyLinks;
    this->InterpolationAdaptor = 0;
    this->EdgeColorAdaptor = 0;
    this->AmbientColorAdaptor = 0;
    this->SliceDirectionAdaptor = 0;
    this->BackfaceRepresentationAdaptor = 0;
    this->SliceDomain = 0;
    this->SelectedMapperAdaptor = 0;
    this->CompositeTreeAdaptor = 0;
    }

  ~PrismDisplayProxyEditorInternal()
    {
    delete this->Links;
    delete this->InterpolationAdaptor;
    delete this->SliceDirectionAdaptor;
    delete this->BackfaceRepresentationAdaptor;
    delete this->SliceDomain;
    delete this->AmbientColorAdaptor;
    delete this->EdgeColorAdaptor;
    }

  pqPropertyLinks* Links;

  // The representation whose properties are being edited.
  QPointer<pqPipelineRepresentation> Representation;
  pqSignalAdaptorComboBox* InterpolationAdaptor;
  pqSignalAdaptorColor*    EdgeColorAdaptor;
  pqSignalAdaptorColor*    AmbientColorAdaptor;
  pqSignalAdaptorComboBox* SliceDirectionAdaptor;
  pqSignalAdaptorComboBox* SelectedMapperAdaptor;
  pqSignalAdaptorComboBox* BackfaceRepresentationAdaptor;
  pqWidgetRangeDomain* SliceDomain;
  pqSignalAdaptorCompositeTreeWidget* CompositeTreeAdaptor;

  // map of <material labels, material files>
  static QMap<QString, QString> MaterialMap;
 };

QMap<QString, QString> PrismDisplayProxyEditorInternal::MaterialMap;

//-----------------------------------------------------------------------------
/// constructor
PrismDisplayProxyEditor::PrismDisplayProxyEditor(pqPipelineRepresentation* repr, QWidget* p)
  : pqDisplayPanel(repr, p), DisableSlots(0)
{
  this->Internal = new PrismDisplayProxyEditorInternal;
  this->Internal->setupUi(this);
  this->setupGUIConnections();

  // setting a repr proxy will enable this
  this->setEnabled(false);

  this->setRepresentation(repr);

  QObject::connect(this->Internal->Links, SIGNAL(smPropertyChanged()),
    this, SLOT(updateAllViews()));
  QObject::connect(this->Internal->EditCubeAxes, SIGNAL(clicked(bool)),
    this, SLOT(editCubeAxes()));
  QObject::connect(this->Internal->compositeTree, SIGNAL(itemSelectionChanged()),
    this, SLOT(volumeBlockSelected()));

  this->DisableSpecularOnScalarColoring = true;
  this->Representation= repr;

  this->CubeAxesActor=NULL;
  PrismCore* core=PrismCore::instance();

  QMap<pqDataRepresentation*,vtkSMPrismCubeAxesRepresentationProxy*>::iterator iter;
  iter=core->CubeAxesRepMap.find(repr);
  if(iter!=core->CubeAxesRepMap.end())
  {

    this->CubeAxesActor =iter.value();



    vtkSMProperty* prop = 0;


    if ((prop = this->CubeAxesActor->GetProperty("Visibility")) != 0)
    {
      QObject::connect(this->Internal->ShowCubeAxes, SIGNAL(toggled(bool)),
        this, SLOT(cubeAxesVisibilityChanged()));

      //needed so the undo / redo properly activate the checkbox
      this->Internal->Links->addPropertyLink(this->Internal->ShowCubeAxes,
        "checked", SIGNAL(stateChanged(int)),
        this->CubeAxesActor,  this->CubeAxesActor->GetProperty("Visibility"));
      this->Internal->AnnotationGroup->show();
    }
    else
    {
      this->Internal->AnnotationGroup->hide();
    }


  }
}

//-----------------------------------------------------------------------------
/// destructor
PrismDisplayProxyEditor::~PrismDisplayProxyEditor()
{
  delete this->Internal;
}


//-----------------------------------------------------------------------------
/// set the proxy to repr display properties for
void PrismDisplayProxyEditor::setRepresentation(pqPipelineRepresentation* repr)
{
  if(this->Internal->Representation == repr)
  {
    return;
  }

  delete this->Internal->SliceDomain;
  this->Internal->SliceDomain = 0;
  delete this->Internal->CompositeTreeAdaptor;
  this->Internal->CompositeTreeAdaptor = 0;

  vtkSMProxy* reprProxy = (repr)? repr->getProxy() : NULL;
  if(this->Internal->Representation)
  {
    // break all old links.
    this->Internal->Links->removeAllPropertyLinks();
  }

  this->Internal->Representation = repr;
  if (!repr )
  {
    this->setEnabled(false);
    return;
  }

  this->setEnabled(true);

  // The slots are already connected but we do not want them to execute
  // while we are initializing the GUI
  this->DisableSlots = 1;

  // setup for visibility
  this->Internal->Links->addPropertyLink(this->Internal->ViewData,
    "checked", SIGNAL(stateChanged(int)),
    reprProxy, reprProxy->GetProperty("Visibility"));

  this->Internal->Links->addPropertyLink(this->Internal->Selectable,
    "checked", SIGNAL(stateChanged(int)),
    reprProxy, reprProxy->GetProperty("Pickable"));

  vtkSMProperty* prop = 0;


  //if ((prop = reprProxy->GetProperty("PieceBoundsVisibility")) != 0) //DDM TODO
  //  {
  //  this->Internal->Links->addPropertyLink(this->Internal->ShowPieceBounds,
  //    "checked", SIGNAL(stateChanged(int)),
  //    reprProxy, prop);
  //  }

  // setup for choosing color
  if (reprProxy->GetProperty("DiffuseColor"))
    {
    QList<QVariant> curColor = pqSMAdaptor::getMultipleElementProperty(
      reprProxy->GetProperty("DiffuseColor"));

    bool prev = this->Internal->ColorActorColor->blockSignals(true);
    this->Internal->ColorActorColor->setChosenColor(
      QColor(qRound(curColor[0].toDouble()*255),
        qRound(curColor[1].toDouble()*255),
        qRound(curColor[2].toDouble()*255), 255));
    this->Internal->ColorActorColor->blockSignals(prev);

    // setup for specular lighting
    QObject::connect(this->Internal->SpecularWhite, SIGNAL(toggled(bool)),
      this, SIGNAL(specularColorChanged()));
    this->Internal->Links->addPropertyLink(this->Internal->SpecularIntensity,
      "value", SIGNAL(editingFinished()),
      reprProxy, reprProxy->GetProperty("Specular"));
    this->Internal->Links->addPropertyLink(this,
      "specularColor", SIGNAL(specularColorChanged()),
      reprProxy, reprProxy->GetProperty("SpecularColor"));
    this->Internal->Links->addPropertyLink(this->Internal->SpecularPower,
      "value", SIGNAL(editingFinished()),
      reprProxy, reprProxy->GetProperty("SpecularPower"));
    QObject::connect(this->Internal->SpecularIntensity, SIGNAL(editingFinished()),
      this, SLOT(updateAllViews()));
    QObject::connect(this, SIGNAL(specularColorChanged()),
      this, SLOT(updateAllViews()));
    QObject::connect(this->Internal->SpecularPower, SIGNAL(editingFinished()),
      this, SLOT(updateAllViews()));
    }

  // setup for interpolation
  this->Internal->StyleInterpolation->clear();
  if ((prop = reprProxy->GetProperty("Interpolation")) != 0)
    {
    prop->UpdateDependentDomains();
    QList<QVariant> items = pqSMAdaptor::getEnumerationPropertyDomain(prop);
    foreach(QVariant item, items)
      {
      this->Internal->StyleInterpolation->addItem(item.toString());
      }
    this->Internal->Links->addPropertyLink(this->Internal->InterpolationAdaptor,
      "currentText", SIGNAL(currentTextChanged(const QString&)),
      reprProxy, prop);
    this->Internal->StyleInterpolation->setEnabled(true);
    }
  else
    {
    this->Internal->StyleInterpolation->setEnabled(false);
    }

  // setup for point size
  if ((prop = reprProxy->GetProperty("PointSize")) !=0)
    {
    this->Internal->Links->addPropertyLink(this->Internal->StylePointSize,
      "value", SIGNAL(editingFinished()),
      reprProxy, reprProxy->GetProperty("PointSize"));
    this->Internal->StylePointSize->setEnabled(true);
    }
  else
    {
    this->Internal->StylePointSize->setEnabled(false);
    }

  // setup for line width
  if ((prop = reprProxy->GetProperty("LineWidth")) != 0)
    {
    this->Internal->Links->addPropertyLink(this->Internal->StyleLineWidth,
      "value", SIGNAL(editingFinished()),
      reprProxy, reprProxy->GetProperty("LineWidth"));
    this->Internal->StyleLineWidth->setEnabled(true);
    }
  else
    {
    this->Internal->StyleLineWidth->setEnabled(false);
    }

  // setup for translate
  this->Internal->Links->addPropertyLink(this->Internal->TranslateX,
    "text", SIGNAL(editingFinished()),
    reprProxy, reprProxy->GetProperty("Position"), 0);
  QDoubleValidator* validator = new QDoubleValidator(this);
  this->Internal->TranslateX->setValidator(validator);

  this->Internal->Links->addPropertyLink(this->Internal->TranslateY,
    "text", SIGNAL(editingFinished()),
    reprProxy, reprProxy->GetProperty("Position"), 1);
  validator = new QDoubleValidator(this);
  this->Internal->TranslateY->setValidator(validator);

  this->Internal->Links->addPropertyLink(this->Internal->TranslateZ,
    "text", SIGNAL(editingFinished()),
    reprProxy, reprProxy->GetProperty("Position"), 2);
  validator = new QDoubleValidator(this);
  this->Internal->TranslateZ->setValidator(validator);


  // setup for scale
  this->Internal->Links->addPropertyLink(this->Internal->ScaleX,
    "text", SIGNAL(editingFinished()),
    reprProxy, reprProxy->GetProperty("Scale"), 0);
  validator = new QDoubleValidator(this);
  this->Internal->ScaleX->setValidator(validator);

  this->Internal->Links->addPropertyLink(this->Internal->ScaleY,
    "text", SIGNAL(editingFinished()),
    reprProxy, reprProxy->GetProperty("Scale"), 1);
  validator = new QDoubleValidator(this);
  this->Internal->ScaleY->setValidator(validator);

  this->Internal->Links->addPropertyLink(this->Internal->ScaleZ,
    "text", SIGNAL(editingFinished()),
    reprProxy, reprProxy->GetProperty("Scale"), 2);
  validator = new QDoubleValidator(this);
  this->Internal->ScaleZ->setValidator(validator);

  // setup for orientation
  this->Internal->Links->addPropertyLink(this->Internal->OrientationX,
    "text", SIGNAL(editingFinished()),
    reprProxy, reprProxy->GetProperty("Orientation"), 0);
  validator = new QDoubleValidator(this);
  this->Internal->OrientationX->setValidator(validator);

  this->Internal->Links->addPropertyLink(this->Internal->OrientationY,
    "text", SIGNAL(editingFinished()),
    reprProxy, reprProxy->GetProperty("Orientation"), 1);
  validator = new QDoubleValidator(this);
  this->Internal->OrientationY->setValidator(validator);

  this->Internal->Links->addPropertyLink(this->Internal->OrientationZ,
    "text", SIGNAL(editingFinished()),
    reprProxy, reprProxy->GetProperty("Orientation"), 2);
  validator = new QDoubleValidator(this);
  this->Internal->OrientationZ->setValidator(validator);

  // setup for origin
  this->Internal->Links->addPropertyLink(this->Internal->OriginX,
    "text", SIGNAL(editingFinished()),
    reprProxy, reprProxy->GetProperty("Origin"), 0);
  validator = new QDoubleValidator(this);
  this->Internal->OriginX->setValidator(validator);

  this->Internal->Links->addPropertyLink(this->Internal->OriginY,
    "text", SIGNAL(editingFinished()),
    reprProxy, reprProxy->GetProperty("Origin"), 1);
  validator = new QDoubleValidator(this);
  this->Internal->OriginY->setValidator(validator);

  this->Internal->Links->addPropertyLink(this->Internal->OriginZ,
    "text", SIGNAL(editingFinished()),
    reprProxy, reprProxy->GetProperty("Origin"), 2);
  validator = new QDoubleValidator(this);
  this->Internal->OriginZ->setValidator(validator);

  // setup for opacity
  this->Internal->Links->addPropertyLink(this->Internal->Opacity,
    "value", SIGNAL(editingFinished()),
    reprProxy, reprProxy->GetProperty("Opacity"));

  // setup of nonlinear subdivision
  if (reprProxy->GetProperty("NonlinearSubdivisionLevel"))
    {
    this->Internal->Links->addPropertyLink(
                this->Internal->NonlinearSubdivisionLevel,
                "value", SIGNAL(valueChanged(int)),
                reprProxy, reprProxy->GetProperty("NonlinearSubdivisionLevel"));
    }

  // setup for map scalars
  this->Internal->Links->addPropertyLink(
    this->Internal->ColorMapScalars, "checked", SIGNAL(stateChanged(int)),
    reprProxy, reprProxy->GetProperty("MapScalars"));

  // setup for InterpolateScalarsBeforeMapping
  if (reprProxy->GetProperty("InterpolateScalarsBeforeMapping"))
    {
    this->Internal->Links->addPropertyLink(
      this->Internal->ColorInterpolateScalars, "checked", SIGNAL(stateChanged(int)),
      reprProxy, reprProxy->GetProperty("InterpolateScalarsBeforeMapping"));
    }

  this->Internal->ColorBy->setRepresentation(repr);
  QObject::connect(this->Internal->ColorBy,
    SIGNAL(modified()),
    this, SLOT(updateEnableState()), Qt::QueuedConnection);

  this->Internal->StyleRepresentation->setRepresentation(repr);
  QObject::connect(this->Internal->StyleRepresentation,
    SIGNAL(currentTextChanged(const QString&)),
    this->Internal->ColorBy, SLOT(reloadGUI()));

  QObject::connect(this->Internal->StyleRepresentation,
    SIGNAL(currentTextChanged(const QString&)),
    this, SLOT(updateEnableState()), Qt::QueuedConnection);

  this->Internal->Texture->setRepresentation(repr);

  if ( (prop = reprProxy->GetProperty("EdgeColor")) != 0)
    {
    this->Internal->Links->addPropertyLink(this->Internal->EdgeColorAdaptor,
      "color", SIGNAL(colorChanged(const QVariant&)),
      reprProxy, prop);
    }

  if ( (prop = reprProxy->GetProperty("AmbientColor")) != 0)
    {
    this->Internal->Links->addPropertyLink(this->Internal->AmbientColorAdaptor,
      "color", SIGNAL(colorChanged(const QVariant&)),
      reprProxy, prop);
    }

  if (reprProxy->GetProperty("Slice"))
    {
    this->Internal->SliceDomain = new pqWidgetRangeDomain(
      this->Internal->Slice, "minimum", "maximum",
      reprProxy->GetProperty("Slice"), 0);

    this->Internal->Links->addPropertyLink(this->Internal->Slice,
      "value", SIGNAL(valueChanged(int)),
      reprProxy, reprProxy->GetProperty("Slice"));

    QList<QVariant> sliceModes =
      pqSMAdaptor::getEnumerationPropertyDomain(
        reprProxy->GetProperty("SliceMode"));
    foreach(QVariant item, sliceModes)
      {
      this->Internal->SliceDirection->addItem(item.toString());
      }
    this->Internal->Links->addPropertyLink(
      this->Internal->SliceDirectionAdaptor,
      "currentText", SIGNAL(currentTextChanged(const QString&)),
      reprProxy, reprProxy->GetProperty("SliceMode"));
    }

  if (reprProxy->GetProperty("ExtractedBlockIndex"))
    {
    this->Internal->CompositeTreeAdaptor =
      new pqSignalAdaptorCompositeTreeWidget(
        this->Internal->compositeTree,
        this->Internal->Representation->getOutputPortFromInput()->getOutputPortProxy(),
        vtkSMCompositeTreeDomain::NONE,
        pqSignalAdaptorCompositeTreeWidget::INDEX_MODE_FLAT, false, true);
    }

  if (reprProxy->GetProperty("SelectedMapperIndex"))
    {
    QList<QVariant> mapperNames =
      pqSMAdaptor::getEnumerationPropertyDomain(
        reprProxy->GetProperty("SelectedMapperIndex"));
    foreach(QVariant item, mapperNames)
      {
      this->Internal->SelectedMapperIndex->addItem(item.toString());
      }
    this->Internal->Links->addPropertyLink(
      this->Internal->SelectedMapperAdaptor,
      "currentText", SIGNAL(currentTextChanged(const QString&)),
      reprProxy, reprProxy->GetProperty("SelectedMapperIndex"));
    }

  this->Internal->BackfaceStyleRepresentation->clear();
  if ((prop = reprProxy->GetProperty("BackfaceRepresentation")) != NULL)
    {
    prop->UpdateDependentDomains();
    QList<QVariant> items = pqSMAdaptor::getEnumerationPropertyDomain(prop);
    foreach (QVariant item, items)
      {
      this->Internal->BackfaceStyleRepresentation->addItem(item.toString());
      }
    this->Internal->Links->addPropertyLink(
                      this->Internal->BackfaceRepresentationAdaptor,
                      "currentText", SIGNAL(currentTextChanged(const QString&)),
                      reprProxy, prop);
    this->Internal->BackfaceStyleGroup->setEnabled(true);
    }
  else
    {
    this->Internal->BackfaceStyleGroup->setEnabled(false);
    }

  QObject::connect(this->Internal->BackfaceStyleRepresentation,
                   SIGNAL(currentIndexChanged(const QString&)),
                   this, SLOT(updateEnableState()), Qt::QueuedConnection);

  // setup for choosing backface color
  if (reprProxy->GetProperty("BackfaceDiffuseColor"))
    {
    QList<QVariant> curColor = pqSMAdaptor::getMultipleElementProperty(
                                reprProxy->GetProperty("BackfaceDiffuseColor"));

    bool prev = this->Internal->BackfaceActorColor->blockSignals(true);
    this->Internal->BackfaceActorColor->setChosenColor(
                               QColor(qRound(curColor[0].toDouble()*255),
                                      qRound(curColor[1].toDouble()*255),
                                      qRound(curColor[2].toDouble()*255), 255));
    this->Internal->BackfaceActorColor->blockSignals(prev);

    new pqStandardColorLinkAdaptor(this->Internal->BackfaceActorColor,
                                   reprProxy, "BackfaceDiffuseColor");
    }

  // setup for backface opacity
  if (reprProxy->GetProperty("BackfaceOpacity"))
    {
    this->Internal->Links->addPropertyLink(this->Internal->BackfaceOpacity,
                                         "value", SIGNAL(editingFinished()),
                                         reprProxy,
                                         reprProxy->GetProperty("BackfaceOpacity"));
    }

#if 0                                       //FIXME
  // material
  this->Internal->StyleMaterial->blockSignals(true);
  this->Internal->StyleMaterial->clear();
  if(vtkMaterialLibrary::GetNumberOfMaterials() > 0)
    {
    this->Internal->StyleMaterial->addItem("None");
    this->Internal->StyleMaterial->addItem("Browse...");
    this->Internal->StyleMaterial->addItems(this->Internal->MaterialMap.keys());
    const char* mat = this->Internal->Representation->getDisplayProxy()->GetMaterialCM();
    if(mat)
      {
      QString filename = mat;
      QMap<QString, QString>::iterator iter;
      for(iter = this->Internal->MaterialMap.begin();
          iter != this->Internal->MaterialMap.end();
          ++iter)
        {
        if(filename == iter.value())
          {
          int foundidx = this->Internal->StyleMaterial->findText(iter.key());
          this->Internal->StyleMaterial->setCurrentIndex(foundidx);
          return;
          }
        }
      }
    }
  else
    {
    this->Internal->StyleMaterial->addItem("Unavailable");
    this->Internal->StyleMaterial->setEnabled(false);
    }
  this->Internal->StyleMaterial->blockSignals(false);
#endif


  new pqStandardColorLinkAdaptor(this->Internal->ColorActorColor,
    reprProxy, "DiffuseColor");
  if (reprProxy->GetProperty("EdgeColor"))
    {
    new pqStandardColorLinkAdaptor(this->Internal->EdgeColor,
      reprProxy, "EdgeColor");
    }
  if (reprProxy->GetProperty("AmbientColor"))
    {
    new pqStandardColorLinkAdaptor(this->Internal->AmbientColor,
      reprProxy, "AmbientColor");
    }

  this->DisableSlots = 0;
  QTimer::singleShot(0, this, SLOT(updateEnableState()));


  //
  if(reprProxy->GetProperty("SampleDistance"))
    {
    this->Internal->Links->addPropertyLink(this->Internal->SampleDistanceValue,
      "text", SIGNAL(editingFinished()),
      reprProxy, reprProxy->GetProperty("SampleDistance"));
    validator = new QDoubleValidator(this);
    this->Internal->SampleDistanceValue->setValidator(validator);
    }

  if(reprProxy->GetProperty("AutoAdjustSampleDistances"))
    {
    this->Internal->Links->addPropertyLink(this->Internal->AutoAdjustSampleDistances,
      "checked", SIGNAL(toggled(bool)),
      reprProxy, reprProxy->GetProperty("AutoAdjustSampleDistances"));
    }
}

//-----------------------------------------------------------------------------
void PrismDisplayProxyEditor::setupGUIConnections()
{
  QObject::connect(
    this->Internal->ViewZoomToData, SIGNAL(clicked(bool)),
    this, SLOT(zoomToData()));
  QObject::connect(
    this->Internal->EditColorMapButton, SIGNAL(clicked()),
    this, SLOT(openColorMapEditor()));
  QObject::connect(
    this->Internal->RescaleButton, SIGNAL(clicked()),
    this, SLOT(rescaleToDataRange()));

  // Create an connect signal adapters.
  if (!QMetaType::isRegistered(QMetaType::type("QVariant")))
    {
    qRegisterMetaType<QVariant>("QVariant");
    }

  this->Internal->InterpolationAdaptor = new pqSignalAdaptorComboBox(
    this->Internal->StyleInterpolation);
  this->Internal->InterpolationAdaptor->setObjectName(
    "StyleInterpolationAdapator");

  QObject::connect(this->Internal->ColorActorColor,
    SIGNAL(chosenColorChanged(const QColor&)),
    this, SLOT(setSolidColor(const QColor&)));

  /// Set up signal-slot connections to create a single undo-set for all the
  /// changes that happen when the solid color is changed.
  /// We need to do this for both solid and edge color since we want to make
  /// sure that the undo-element for setting up of the "global property" link
  /// gets added in the same set in which the solid/edge color is changed.
  this->Internal->ColorActorColor->setUndoLabel("Change Solid Color");
  pqUndoStack* stack = pqApplicationCore::instance()->getUndoStack();
  if (stack)
    {
    QObject::connect(this->Internal->ColorActorColor,
      SIGNAL(beginUndo(const QString&)),
      stack, SLOT(beginUndoSet(const QString&)));
    QObject::connect(this->Internal->ColorActorColor,
      SIGNAL(endUndo()), stack, SLOT(endUndoSet()));
    }

  this->Internal->EdgeColorAdaptor = new pqSignalAdaptorColor(
    this->Internal->EdgeColor, "chosenColor",
    SIGNAL(chosenColorChanged(const QColor&)), false);
  this->Internal->EdgeColor->setUndoLabel("Change Edge Color");
  if (stack)
    {
    QObject::connect(this->Internal->EdgeColor,
      SIGNAL(beginUndo(const QString&)),
      stack, SLOT(beginUndoSet(const QString&)));
    QObject::connect(this->Internal->EdgeColor,
      SIGNAL(endUndo()), stack, SLOT(endUndoSet()));
    }

  this->Internal->AmbientColorAdaptor = new pqSignalAdaptorColor(
    this->Internal->AmbientColor, "chosenColor",
    SIGNAL(chosenColorChanged(const QColor&)), false);
  this->Internal->AmbientColor->setUndoLabel("Change Ambient Color");
  if (stack)
    {
    QObject::connect(this->Internal->AmbientColor,
      SIGNAL(beginUndo(const QString&)),
      stack, SLOT(beginUndoSet(const QString&)));
    QObject::connect(this->Internal->AmbientColor,
      SIGNAL(endUndo()), stack, SLOT(endUndoSet()));
    }

  QObject::connect(this->Internal->StyleMaterial, SIGNAL(currentIndexChanged(int)),
                   this, SLOT(updateMaterial(int)));

  this->Internal->SliceDirectionAdaptor = new pqSignalAdaptorComboBox(
    this->Internal->SliceDirection);
  QObject::connect(this->Internal->SliceDirectionAdaptor,
    SIGNAL(currentTextChanged(const QString&)),
    this, SLOT(sliceDirectionChanged()), Qt::QueuedConnection);

  this->Internal->SelectedMapperAdaptor = new pqSignalAdaptorComboBox(
    this->Internal->SelectedMapperIndex);
  QObject::connect(this->Internal->SelectedMapperAdaptor,
    SIGNAL(currentTextChanged(const QString&)),
    this, SLOT(selectedMapperChanged()), Qt::QueuedConnection);

  this->Internal->BackfaceRepresentationAdaptor = new pqSignalAdaptorComboBox(
                                   this->Internal->BackfaceStyleRepresentation);
  this->Internal->BackfaceRepresentationAdaptor->setObjectName(
                                         "BackfaceStyleRepresentationAdapator");

  QObject::connect(this->Internal->BackfaceActorColor,
                   SIGNAL(chosenColorChanged(const QColor&)),
                   this, SLOT(setBackfaceSolidColor(const QColor&)));

  this->Internal->BackfaceActorColor->setUndoLabel("Change Backface Solid Color");
  stack = pqApplicationCore::instance()->getUndoStack();
  if (stack)
    {
    QObject::connect(this->Internal->BackfaceActorColor,
                     SIGNAL(beginUndo(const QString&)),
                     stack, SLOT(beginUndoSet(const QString&)));
    QObject::connect(this->Internal->BackfaceActorColor,
                     SIGNAL(endUndo()), stack, SLOT(endUndoSet()));
    }


  QObject::connect(this->Internal->AutoAdjustSampleDistances,
                   SIGNAL(toggled(bool)),
                   this,
                   SLOT(setAutoAdjustSampleDistances(bool)));
}

//-----------------------------------------------------------------------------
void PrismDisplayProxyEditor::updateEnableState()
{
  int reprType = this->Internal->Representation->getRepresentationType();

  if (this->Internal->ColorBy->getCurrentText() == "Solid Color")
    {
    this->Internal->ColorInterpolateScalars->setEnabled(false);
    if (reprType == vtkSMPVRepresentationProxy::WIREFRAME ||
      reprType == vtkSMPVRepresentationProxy::POINTS ||
      reprType == vtkSMPVRepresentationProxy::OUTLINE)
      {
      this->Internal->ColorButtonStack->setCurrentWidget(
        this->Internal->AmbientColorPage);
      this->Internal->LightingGroup->setEnabled(false);
      }
    else
      {
      this->Internal->ColorButtonStack->setCurrentWidget(
        this->Internal->SolidColorPage);
      this->Internal->LightingGroup->setEnabled(true);
      }
    this->Internal->BackfaceActorColor->setEnabled(true);
    }
  else
    {
    if (this->DisableSpecularOnScalarColoring)
      {
      this->Internal->LightingGroup->setEnabled(false);
      }
    this->Internal->ColorInterpolateScalars->setEnabled(true);
    this->Internal->ColorButtonStack->setCurrentWidget(
        this->Internal->ColorMapPage);
    this->Internal->BackfaceActorColor->setEnabled(false);
    }


  this->Internal->EdgeStyleGroup->setEnabled(
    reprType == vtkSMPVRepresentationProxy::SURFACE_WITH_EDGES);

  this->Internal->SliceGroup->setEnabled(
    reprType == vtkSMPVRepresentationProxy::SLICE);
  if (reprType == vtkSMPVRepresentationProxy::SLICE)
    {
    // every time the user switches to Slice mode we update the domain for the
    // slider since the domain depends on the input to the image mapper which
    // may have changed.
    QTimer::singleShot(0, this, SLOT(sliceDirectionChanged()));
    }

  this->Internal->compositeTree->setVisible(
   this->Internal->CompositeTreeAdaptor &&
   (reprType == vtkSMPVRepresentationProxy::VOLUME));

  this->Internal->SelectedMapperIndex->setEnabled(
    reprType == vtkSMPVRepresentationProxy::VOLUME
    && this->Internal->Representation->getProxy()->GetProperty("SelectedMapperIndex"));

  vtkSMProperty *backfaceRepProperty = this->Internal->Representation
    ->getRepresentationProxy()->GetProperty("BackfaceRepresentation");
  if (   !backfaceRepProperty
      || (   (reprType != vtkSMPVRepresentationProxy::POINTS)
          && (reprType != vtkSMPVRepresentationProxy::WIREFRAME)
          && (reprType != vtkSMPVRepresentationProxy::SURFACE)
          && (reprType != vtkSMPVRepresentationProxy::SURFACE_WITH_EDGES) ) )
    {
    this->Internal->BackfaceStyleGroup->setEnabled(false);
    }
  else
    {
    this->Internal->BackfaceStyleGroup->setEnabled(true);
    int backRepType
      = pqSMAdaptor::getElementProperty(backfaceRepProperty).toInt();

    bool backFollowsFront
      = (   (backRepType == vtkSMPVRepresentationProxy::FOLLOW_FRONTFACE)
         || (backRepType == vtkSMPVRepresentationProxy::CULL_BACKFACE)
         || (backRepType == vtkSMPVRepresentationProxy::CULL_FRONTFACE) );

    this->Internal->BackfaceStyleGroupOptions->setEnabled(!backFollowsFront);
    }

  vtkSMRepresentationProxy* display =
    this->Internal->Representation->getRepresentationProxy();
  if (display)
    {
    QVariant scalarMode = pqSMAdaptor::getEnumerationProperty(
      display->GetProperty("ColorAttributeType"));
    vtkPVDataInformation* geomInfo = display->GetRepresentedDataInformation();
    if (!geomInfo)
      {
      return;
      }
    vtkPVDataSetAttributesInformation* attrInfo;
    if (scalarMode == "POINT_DATA")
      {
      attrInfo = geomInfo->GetPointDataInformation();
      }
    else
      {
      attrInfo = geomInfo->GetCellDataInformation();
      }
    vtkPVArrayInformation* arrayInfo = attrInfo->GetArrayInformation(
      this->Internal->Representation->getColorField(true).toAscii().data());

    if (arrayInfo && arrayInfo->GetDataType() == VTK_UNSIGNED_CHAR)
      {
      // Number of component restriction.
      // Upto 4 component unsigned chars can be direcly mapped.
      if (arrayInfo->GetNumberOfComponents() <= 4)
        {
        // One component causes more trouble than it is worth.
        this->Internal->ColorMapScalars->setEnabled(true);
        return;
        }
      }
    }

  this->Internal->ColorMapScalars->setCheckState(Qt::Checked);
  this->Internal->ColorMapScalars->setEnabled(false);
}

//-----------------------------------------------------------------------------
void PrismDisplayProxyEditor::openColorMapEditor()
{
  pqColorScaleEditor editor(pqCoreUtilities::mainWidget());
  editor.setObjectName("pqColorScaleDialog");
  editor.setRepresentation(this->Internal->Representation);
  editor.exec();
}

//-----------------------------------------------------------------------------
void PrismDisplayProxyEditor::rescaleToDataRange()
{
  if(this->Internal->Representation.isNull())
    {
    return;
    }

  this->Internal->Representation->resetLookupTableScalarRange();
  this->updateAllViews();
}

//-----------------------------------------------------------------------------
void PrismDisplayProxyEditor::zoomToData()
{
  if (this->DisableSlots)
    {
    return;
    }

  pqRenderView* renModule = qobject_cast<pqRenderView*>(
    this->Internal->Representation->getView());
  if (renModule)
    {
    vtkSMRenderViewProxy* rm = renModule->getRenderViewProxy();
    rm->ZoomTo(this->Internal->Representation->getProxy());
    renModule->render();
    }
}

//-----------------------------------------------------------------------------
// TODO:  get rid of me !!  as soon as vtkSMDisplayProxy can tell us when new
// arrays are added.
void PrismDisplayProxyEditor::reloadGUI()
{
  this->Internal->ColorBy->setRepresentation(this->Internal->Representation);
}


//-----------------------------------------------------------------------------
QVariant PrismDisplayProxyEditor::specularColor() const
{
  if(this->Internal->SpecularWhite->isChecked())
    {
    QList<QVariant> ret;
    ret.append(1.0);
    ret.append(1.0);
    ret.append(1.0);
    return ret;
    }

  vtkSMProxy* proxy = this->Internal->Representation->getProxy();
  return pqSMAdaptor::getMultipleElementProperty(
       proxy->GetProperty("DiffuseColor"));
}

//-----------------------------------------------------------------------------
void PrismDisplayProxyEditor::setSpecularColor(QVariant specColor)
{
  QList<QVariant> whiteLight;
  whiteLight.append(1.0);
  whiteLight.append(1.0);
  whiteLight.append(1.0);

  if(specColor == whiteLight && !this->Internal->SpecularWhite->isChecked())
    {
    this->Internal->SpecularWhite->setChecked(true);
    emit this->specularColorChanged();
    }
  else if(this->Internal->SpecularWhite->isChecked())
    {
    this->Internal->SpecularWhite->setChecked(false);
    emit this->specularColorChanged();
    }
}

//-----------------------------------------------------------------------------
void PrismDisplayProxyEditor::updateMaterial(int vtkNotUsed(idx))
{
  // FIXME: when we enable materials.
#if 0
  if(idx == 0)
    {
    this->Internal->Representation->getDisplayProxy()->SetMaterialCM(0);
    this->updateAllViews();
    }
  else if(idx == 1)
    {
    pqFileDialog diag(NULL, this, "Open Material File", QString(),
                      "Material Files (*.xml)");
    diag.setFileMode(pqFileDialog::ExistingFile);
    if(diag.exec() == QDialog::Accepted)
      {
      QString filename = diag.getSelectedFiles()[0];
      QMap<QString, QString>::iterator iter;
      for(iter = this->Internal->MaterialMap.begin();
          iter != this->Internal->MaterialMap.end();
          ++iter)
        {
        if(filename == iter.value())
          {
          int foundidx = this->Internal->StyleMaterial->findText(iter.key());
          this->Internal->StyleMaterial->setCurrentIndex(foundidx);
          return;
          }
        }
      QFileInfo fi(filename);
      this->Internal->MaterialMap.insert(fi.fileName(), filename);
      this->Internal->StyleMaterial->addItem(fi.fileName());
      this->Internal->StyleMaterial->setCurrentIndex(
        this->Internal->StyleMaterial->count() - 1);
      }
    }
  else
    {
    QString label = this->Internal->StyleMaterial->itemText(idx);
    this->Internal->Representation->getDisplayProxy()->SetMaterialCM(
      this->Internal->MaterialMap[label].toAscii().data());
    this->updateAllViews();
    }
#endif
}
//-----------------------------------------------------------------------------
void PrismDisplayProxyEditor::cubeAxesVisibilityChanged()
{
  vtkSMPropertyHelper(this->CubeAxesActor, "Visibility").Set(this->isCubeAxesVisible());
  this->CubeAxesActor->UpdateVTKObjects();
  this->Representation->renderViewEventually();
//this->updateAllViews();


  //vtkSMProxy* reprProxy = (this->Internal->Representation)? this->Internal->Representation->getProxy() : NULL;
  //vtkSMProperty* prop = 0;

  //// setup cube axes visibility.
  //if ((prop = reprProxy->GetProperty("CubeAxesVisibility")) != 0)
  //  {
  //  pqSMAdaptor::setElementProperty(prop, this->Internal->ShowCubeAxes->isChecked());
  //  reprProxy->UpdateVTKObjects();
  //  }
  //this->updateAllViews();
}
//-----------------------------------------------------------------------------
void PrismDisplayProxyEditor::editCubeAxes()
{
  PrismCubeAxesEditorDialog dialog(this);
  dialog.setRepresentationProxy(this->CubeAxesActor);
  dialog.exec();

/*

  pqCubeAxesEditorDialog dialog(this);
  dialog.setRepresentationProxy(this->Internal->Representation->getProxy());
  dialog.exec();*/
}
//----------------------------------------------------------------------------
bool PrismDisplayProxyEditor::isCubeAxesVisible()
{
  return this->Internal->ShowCubeAxes->isChecked();
}

//-----------------------------------------------------------------------------
void PrismDisplayProxyEditor::sliceDirectionChanged()
{
  if (this->Internal->Representation)
    {
    vtkSMProxy* reprProxy = this->Internal->Representation->getProxy();
    vtkSMProperty* prop = reprProxy->GetProperty("SliceMode");
    if (prop)
      {
      prop->UpdateDependentDomains();
      }
    }
}

//-----------------------------------------------------------------------------
/// Handle user selection of block for volume rendering
void PrismDisplayProxyEditor::volumeBlockSelected()
{
  if (this->Internal->CompositeTreeAdaptor
      && this->Internal->Representation)
    {
    bool valid = false;
    unsigned int selectedIndex =
      this->Internal->CompositeTreeAdaptor->getCurrentFlatIndex(&valid);
    if (valid && selectedIndex > 0)
      {
      vtkSMRepresentationProxy* repr =
        this->Internal->Representation->getRepresentationProxy();
      pqSMAdaptor::setElementProperty(
        repr->GetProperty("ExtractedBlockIndex"), selectedIndex);
      repr->UpdateVTKObjects();
      this->Internal->Representation->renderViewEventually();
      this->Internal->ColorBy->reloadGUI();
      }
    }
}

//-----------------------------------------------------------------------------
// Called when the GUI selection for the solid color changes.
void PrismDisplayProxyEditor::setSolidColor(const QColor& color)
{
  QList<QVariant> val;
  val.push_back(color.red()/255.0);
  val.push_back(color.green()/255.0);
  val.push_back(color.blue()/255.0);
  pqSMAdaptor::setMultipleElementProperty(
    this->Internal->Representation->getProxy()->GetProperty("DiffuseColor"), val);

  // If specular white is off, then we want to update the specular color as
  // well.
  emit this->specularColorChanged();
}

//-----------------------------------------------------------------------------
// Called when the GUI selection for the backface solid color changes.
void PrismDisplayProxyEditor::setBackfaceSolidColor(const QColor& color)
{
  QList<QVariant> val;
  val.push_back(color.red()/255.0);
  val.push_back(color.green()/255.0);
  val.push_back(color.blue()/255.0);

  pqSMAdaptor::setMultipleElementProperty(
    this->Internal->Representation->getProxy()->GetProperty("BackfaceAmbientColor"), val);
  pqSMAdaptor::setMultipleElementProperty(
    this->Internal->Representation->getProxy()->GetProperty("BackfaceDiffuseColor"), val);

  // If specular white is off, then we want to update the specular color as
  // well.
  emit this->specularColorChanged();
}

//-----------------------------------------------------------------------------
void PrismDisplayProxyEditor::setAutoAdjustSampleDistances(bool flag)
{
  this->Internal->SampleDistanceValue->setEnabled(!flag);
}

//-----------------------------------------------------------------------------
void PrismDisplayProxyEditor::selectedMapperChanged()
{
  if(!this->Internal->SelectedMapperIndex->currentText().compare(
      QString("Fixed Point"), Qt::CaseInsensitive))
    {
    // Fixed point does not uses sample distance.
    this->Internal->SampleDistance->setEnabled(0);
    this->Internal->SampleDistanceValue->setEnabled(0);
    this->Internal->AutoAdjustSampleDistances->setEnabled(0);
    }
  else if(!this->Internal->SelectedMapperIndex->currentText().compare(
      QString("GPU"), Qt::CaseInsensitive))
    {
    this->Internal->SampleDistance->setEnabled(1);
    this->Internal->SampleDistanceValue->setEnabled(0);
    this->Internal->AutoAdjustSampleDistances->setEnabled(1);
    }
  else
    {
      // Do nothing.
  }
}
pqServer* PrismDisplayProxyEditor::getActiveServer() const
{
  pqApplicationCore* core = pqApplicationCore::instance();
  pqServer* server=core->getActiveServer();


  return server;
}
