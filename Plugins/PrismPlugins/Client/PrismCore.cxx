

#include "PrismCore.h"
#include "pqApplicationCore.h"
#include "vtkSMPrismCubeAxesRepresentationProxy.h"
#include "pqServerManagerSelectionModel.h"
#include "pqPipelineSource.h"
#include "pqServer.h"
#include "pqObjectBuilder.h"
#include "vtkSMProxy.h"
#include "vtkSMSourceProxy.h"
#include "vtkSelection.h"
#include "pqView.h"
#include "pqOutputPort.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkSMProperty.h"
#include "vtkSMInputProperty.h"
#include "vtkSMProxyManager.h"
#include "pqServerManagerModel.h"
#include "pqSelectionManager.h"
#include "pqDataRepresentation.h"
#include "pqSMAdaptor.h"
#include "pqObjectBuilder.h"
#include "pqCoreUtilities.h"
#include <QApplication>
#include <QActionGroup>
#include <QStyle>
#include <QMessageBox>
#include <QtDebug>
#include <pqFileDialog.h>
#include <QAction>
#include <QMainWindow>
#include "pqUndoStack.h"
#include "vtkSMPropertyLink.h"
#include "vtkSMSelectionHelper.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "pqRenderView.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMViewProxy.h"
#include "pqRenderView.h"
#include <QHeaderView>
#include <QLayout>
#include "pqTreeWidget.h"
#include <QComboBox>
#include <QList>
#include "vtkSMOutputPort.h"
//-----------------------------------------------------------------------------
static PrismCore* Instance = 0;


//-----------------------------------------------------------------------------
PrismTableWidget::PrismTableWidget(QWidget* p)
  : QTableWidget(p)
{
  QObject::connect(this->model(), SIGNAL(rowsInserted(QModelIndex, int, int)),
                   this, SLOT(invalidateLayout()));
  QObject::connect(this->model(), SIGNAL(rowsRemoved(QModelIndex, int, int)),
                   this, SLOT(invalidateLayout()));
  QObject::connect(this->model(), SIGNAL(modelReset()),
                   this, SLOT(invalidateLayout()));

}

//-----------------------------------------------------------------------------
PrismTableWidget::~PrismTableWidget()
{
}



//-----------------------------------------------------------------------------
QSize PrismTableWidget::sizeHint() const
{
  // lets show X items before we get a scrollbar
  // probably want to make this a member variable
  // that a caller has access to
  int maxItemHint = 10;
  // for no items, let's give a space of X pixels
  int minItemHeight = 20;

  int num = this->rowCount() + 1; /* extra room for scroll bar */
  num = qMin(num, maxItemHint);

  int pix = minItemHeight;

  if (num)
    {
    pix = qMax(pix, this->sizeHintForRow(0) * num);
    }

  int margin[4];
  this->getContentsMargins(margin, margin+1, margin+2, margin+3);
  int h = pix + margin[1] + margin[3] + this->horizontalHeader()->frameSize().height();
  return QSize(156, h);
}

//-----------------------------------------------------------------------------
QSize PrismTableWidget::minimumSizeHint() const
{
  return this->sizeHint();
}

//-----------------------------------------------------------------------------
void PrismTableWidget::invalidateLayout()
{
  // sizeHint is dynamic, so we need to invalidate parent layouts
  // when items are added or removed
  for(QWidget* w = this->parentWidget();
      w && w->layout();
      w = w->parentWidget())
    {
    w->layout()->invalidate();
    }
  // invalidate() is not enough, we need to reset the cache of the
  // QWidgetItemV2, so sizeHint() could be recomputed
  this->updateGeometry();
}


SESAMEComboBoxDelegate::SESAMEComboBoxDelegate(QObject *par): QItemDelegate(par)
{

this->SPanel=NULL;
this->PPanel=NULL;
}
void SESAMEComboBoxDelegate::setPanel(PrismSurfacePanel* panel)
{
this->SPanel=panel;
this->PPanel=NULL;
}
void SESAMEComboBoxDelegate::setPanel(PrismPanel* panel)
{
this->PPanel=panel;
this->SPanel=NULL;
}
 void SESAMEComboBoxDelegate::setVariableList(QStringList &variables)
{
  this->Variables=variables;
}

QWidget *SESAMEComboBoxDelegate::createEditor(QWidget *par,
     const QStyleOptionViewItem &/* option */,
     const QModelIndex &/* index */) const
 {
   QComboBox *editor = new QComboBox(par);
   editor->addItems(this->Variables);


   if(this->SPanel)
   {
    QObject::connect(editor, SIGNAL(currentIndexChanged(int)),
        this->SPanel, SLOT(onConversionVariableChanged(int)));
   }
   else if(this->PPanel)
   {
    QObject::connect(editor, SIGNAL(currentIndexChanged(int)),
        this->PPanel, SLOT(onConversionVariableChanged(int)));

   }


   return editor;
 }

 void SESAMEComboBoxDelegate::setEditorData(QWidget *editor,
                                     const QModelIndex &index) const
 {
   QString value = index.model()->data(index, Qt::DisplayRole).toString();

   QComboBox *comboBox = static_cast<QComboBox*>(editor);
   comboBox->blockSignals(true);
   int cbIndex=comboBox->findText(value);
   comboBox->setCurrentIndex(cbIndex);
   comboBox->blockSignals(false);
 }

 void SESAMEComboBoxDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
   const QModelIndex &index) const
 {

   QComboBox *comboBox = static_cast<QComboBox*>(editor);
   QString value=comboBox->currentText();
   model->setData(index, value, Qt::EditRole);


 }

void SESAMEComboBoxDelegate::updateEditorGeometry(QWidget *editor,
     const QStyleOptionViewItem &option, const QModelIndex &/* index */) const
 {
     editor->setGeometry(option.rect);
 }




PrismCore::PrismCore(QObject* p)
:QObject(p)
    {
    this->ProcessingEvent=false;
    this->VTKConnections = NULL;
    this->PrismViewAction=NULL;
    this->SesameViewAction=NULL;
    this->MenuPrismViewAction=NULL;
    this->MenuSesameViewAction=NULL;


    pqServerManagerModel* model=pqApplicationCore::instance()->getServerManagerModel();

    this->connect(model, SIGNAL(connectionAdded(pqPipelineSource*,pqPipelineSource*, int)),
        this, SLOT(onConnectionAdded(pqPipelineSource*,pqPipelineSource*)));
    this->connect(model, SIGNAL(viewAdded(pqView*)),
        this, SLOT(onViewAdded(pqView*)));
    this->connect(model, SIGNAL(preViewRemoved(pqView*)),
        this, SLOT(onViewRemoved(pqView*)));

    this->connect(model, SIGNAL(preRepresentationRemoved(pqRepresentation*)),
        this, SLOT(onPreRepresentationRemoved(pqRepresentation*)));

    QList<pqView*> views=model->findItems<pqView*>();

    for(int i=0;i<views.count();i++)
    {
      pqView *view=views.at(i);
      this->onViewAdded(view);
    }

    this->setParent(model);

    pqServerManagerSelectionModel *selection =
        pqApplicationCore::instance()->getSelectionModel();
    this->connect(selection, SIGNAL(currentChanged(pqServerManagerModelItem*)),
        this, SLOT(onSelectionChanged()));
    this->connect(selection,
        SIGNAL(selectionChanged(const pqServerManagerSelection&, const pqServerManagerSelection&)),
        this, SLOT(onSelectionChanged()));


    pqObjectBuilder *builder=pqApplicationCore::instance()->getObjectBuilder();
    this->connect(builder,SIGNAL(proxyCreated(pqProxy*)),
        this, SLOT(onSelectionChanged()));



    this->onSelectionChanged();
    }

PrismCore::~PrismCore()
    {
      QMap<vtkSMPrismCubeAxesRepresentationProxy*,pqRenderView*>::iterator viter;
      for(viter=this->CubeAxesViewMap.begin();viter!=this->CubeAxesViewMap.end();viter++)
      {
        pqRenderView* view=viter.value();
        vtkSMViewProxy* renv= view->getViewProxy();
        vtkSMPropertyHelper(renv, "HiddenRepresentations").Remove(viter.key());
      }
      this->CubeAxesViewMap.clear();

      QMap<pqDataRepresentation*,vtkSMPrismCubeAxesRepresentationProxy*>::iterator iter;
      for(iter=this->CubeAxesRepMap.begin();iter!=this->CubeAxesRepMap.end();iter++)
      {

          vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
          pxm->UnRegisterProxy(iter.value()->GetXMLGroup(),iter.value()->GetClassName(),iter.value());
      }
      this->CubeAxesRepMap.clear();

    Instance=NULL;
    }




//-----------------------------------------------------------------------------
PrismCore* PrismCore::instance()
    {
        
        if(!Instance)
        {
            Instance=new PrismCore(NULL);
        }
    return Instance;
    }
void PrismCore::createMenuActions(QActionGroup* ag)
    {
      if(!this->MenuPrismViewAction)
      {
        this->MenuPrismViewAction = new QAction("Prism View",ag);
        this->MenuPrismViewAction->setToolTip("Create Prism View");
        this->MenuPrismViewAction->setIcon(QIcon(":/Prism/Icons/PrismSmall.png"));
        this->MenuPrismViewAction->setEnabled(false);

        QObject::connect(this->MenuPrismViewAction, SIGNAL(triggered(bool)), this, SLOT(onCreatePrismView()));
      }
      if(!this->MenuSesameViewAction)
      {
        this->MenuSesameViewAction = new QAction("SESAME Surface",ag);
        this->MenuSesameViewAction->setToolTip("Open SESAME Surface");
        this->MenuSesameViewAction->setIcon(QIcon(":/Prism/Icons/CreateSESAME.png"));

        QObject::connect(this->MenuSesameViewAction, SIGNAL(triggered(bool)), this, SLOT(onSESAMEFileOpen()));
      }
    }


void PrismCore::createActions(QActionGroup* ag)
    {
      if(!this->PrismViewAction)
      {
        this->PrismViewAction = new QAction("Prism View",ag);
        this->PrismViewAction->setToolTip("Create Prism View");
        this->PrismViewAction->setIcon(QIcon(":/Prism/Icons/PrismSmall.png"));
        this->PrismViewAction->setEnabled(false);

        QObject::connect(this->PrismViewAction, SIGNAL(triggered(bool)), this, SLOT(onCreatePrismView()));
      }
      if(!this->SesameViewAction)
      {
        this->SesameViewAction = new QAction("SESAME Surface",ag);
        this->SesameViewAction->setToolTip("Open SESAME Surface");
        this->SesameViewAction->setIcon(QIcon(":/Prism/Icons/CreateSESAME.png"));

        QObject::connect(this->SesameViewAction, SIGNAL(triggered(bool)), this, SLOT(onSESAMEFileOpen()));
      }
    }

pqPipelineSource* PrismCore::getActiveSource() const
    {
    pqApplicationCore* core = pqApplicationCore::instance();
    pqServerManagerSelection sels = *core->getSelectionModel()->selectedItems();
    pqPipelineSource* source = 0;
    pqServerManagerModelItem* item = 0;
    if(sels.empty())
    {
        return NULL;
    }
    pqServerManagerSelection::ConstIterator iter = sels.begin();

    item = *iter;
    source = dynamic_cast<pqPipelineSource*>(item);   

    return source;
    }
pqServer* PrismCore::getActiveServer() const
    {
    pqApplicationCore* core = pqApplicationCore::instance();

    pqServer* server=core->getActiveServer();
    return server;
    }


void PrismCore::onSESAMEFileOpen()
    {
    // Get the list of selected sources.

    pqServer* server = this->getActiveServer();

    if(!server)
        {
        qDebug() << "No active server selected.";
        }




    QString filters = "All files (*)";
    pqFileDialog* const file_dialog = new pqFileDialog(server, 
        pqCoreUtilities::mainWidget(), tr("Open File:"), QString(), filters);

    file_dialog->setAttribute(Qt::WA_DeleteOnClose);
    file_dialog->setObjectName("FileOpenDialog");
    file_dialog->setFileMode(pqFileDialog::ExistingFile);
    QObject::connect(file_dialog, SIGNAL(filesSelected(const QStringList&)), 
        this, SLOT(onSESAMEFileOpen(const QStringList&)));
    file_dialog->setModal(true); 
    file_dialog->show(); 

    }
void PrismCore::onSESAMEFileOpen(const QStringList& files)
    {
    if (files.empty())
        {
        return ;
        }

    pqApplicationCore* core = pqApplicationCore::instance();
    pqObjectBuilder* builder = core->getObjectBuilder();

    pqServer* server = this->getActiveServer();
    if(!server)
        {
        qCritical() << "Cannot create reader without an active server.";
        return ;
        }

    pqUndoStack *stack=core->getUndoStack();
    if(stack)
        {
        stack->beginUndoSet("Open Prism Surface");
        }

    pqPipelineSource* filter = 0;
    filter =  builder->createReader("sources", "PrismSurfaceReader", files, server);
    this->connect(filter, SIGNAL(representationAdded(pqPipelineSource*,
      pqDataRepresentation*, int)),
      this, SLOT(onPrismRepresentationAdded(pqPipelineSource*,
      pqDataRepresentation*,
      int)));





    if(stack)
        {
        stack->endUndoSet();
        }  
    }


void PrismCore::onCreatePrismView()
{
    // Get the list of selected sources.

    pqPipelineSource* source = 0;

    source = this->getActiveSource();   

    if(!source)
    {
        QMessageBox::warning(NULL, tr("No Object Selected"),
            tr("No pipeline object is selected.\n"
            "Please select a pipeline object from the list on the left."),
            QMessageBox::Ok );

        return;
    }   
       

    pqServer* server = source->getServer();

    if(!server)
        {
        qDebug() << "No active server selected.";
        return;
        }



    QString filters = "All files (*)";
    pqFileDialog* const file_dialog = new pqFileDialog(server, 
         pqCoreUtilities::mainWidget(), tr("Open File:"), QString(), filters);

    file_dialog->setAttribute(Qt::WA_DeleteOnClose);
    file_dialog->setObjectName("FileOpenDialog");
    file_dialog->setFileMode(pqFileDialog::ExistingFile);
    QObject::connect(file_dialog, SIGNAL(filesSelected(const QStringList&)), 
        this, SLOT(onCreatePrismView(const QStringList&)));
    file_dialog->setModal(true); 
    file_dialog->show(); 

    }
void PrismCore::onCreatePrismView(const QStringList& files)
    {
    // Get the list of selected sources.
    pqApplicationCore* core = pqApplicationCore::instance();
    pqObjectBuilder* builder = core->getObjectBuilder();
    pqPipelineSource* source = 0;
    pqPipelineSource* filter = 0;
    pqServer* server = 0;
    QList<pqOutputPort*> inputs;


    source = this->getActiveSource();

    if(!source)
    {
        QMessageBox::warning(NULL, tr("No Object Selected"),
            tr("No pipeline object is selected.\n"
            "Please select a pipeline object from the list on the left."),
            QMessageBox::Ok );

        return;
    }
    server = source->getServer();
    if(!server)
        {
        qDebug() << "No active server selected.";
        }
    builder->createView("RenderView",server);
 
    inputs.push_back(source->getOutputPort(0));

    QMap<QString, QList<pqOutputPort*> > namedInputs;
    namedInputs["Input"] = inputs;

    pqUndoStack *stack=core->getUndoStack();
    if(stack)
        {
        stack->beginUndoSet("Create Prism Filter");
        }

    QMap<QString,QVariant> defaultProperties;
    defaultProperties["FileName"]=files;

    filter = builder->createFilter("filters", "PrismFilter", namedInputs, server,defaultProperties);

 //   vtkSMProperty *fileNameProperty=filter->getProxy()->GetProperty("FileName");

 //   pqSMAdaptor::setElementProperty(fileNameProperty, files[0]);

//    filter->getProxy()->UpdateVTKObjects();
    //I believe that this is needs to be called twice because there are properties that depend on other properties.
    //Calling it once doesn't set all the properties right.
    filter->setDefaultPropertyValues();
    filter->setDefaultPropertyValues();


    this->connect(filter, SIGNAL(representationAdded(pqPipelineSource*,
      pqDataRepresentation*, int)),
      this, SLOT(onPrismRepresentationAdded(pqPipelineSource*,
      pqDataRepresentation*,
      int)));

    if(stack)
        {
        stack->endUndoSet();
        }  
    }

void PrismCore::onConnectionAdded(pqPipelineSource* source, 
                                  pqPipelineSource* consumer)
{
  if (consumer)
  {
    QString name=consumer->getProxy()->GetXMLName();
    if(name=="PrismFilter")
    {
      vtkSMSourceProxy* prismP = vtkSMSourceProxy::SafeDownCast(consumer->getProxy());
      vtkSMSourceProxy* sourceP = vtkSMSourceProxy::SafeDownCast(source->getProxy());

      if(this->VTKConnections==NULL)
      {
        this->VTKConnections = vtkSmartPointer<vtkEventQtSlotConnect>::New();
      }

      this->VTKConnections->Connect(sourceP, vtkCommand::SelectionChangedEvent,
        this,
        SLOT(onGeometrySelection(vtkObject*, unsigned long, void*, void*)),prismP);

      this->VTKConnections->Connect(prismP, vtkCommand::SelectionChangedEvent,
        this,
        SLOT(onPrismSelection(vtkObject*, unsigned long, void*, void*)),sourceP);
    }
  }
}




void PrismCore::onViewAdded(pqView* view)
{
  //For 3D Views we need to listen for repr added.

  if(view->getViewType()=="RenderView")
  {
    this->connect(view, SIGNAL(representationAdded(pqRepresentation*)),
        this, SLOT(onViewRepresentationAdded(pqRepresentation*)));

    this->connect(view, SIGNAL(representationRemoved(pqRepresentation*)),
        this, SLOT(onViewRepresentationRemoved(pqRepresentation*)));
  }

}
void PrismCore::onViewRemoved(pqView* view)
{
  QList<pqRepresentation*> reps=view->getRepresentations();
  for(int i=0;i<reps.count();i++)
  {
    pqRepresentation* rep=reps.at(i);
    pqDataRepresentation* dataRepr=qobject_cast<pqDataRepresentation*>(rep);
    if(dataRepr)
    {
      QMap<pqDataRepresentation*,vtkSMPrismCubeAxesRepresentationProxy*>::iterator iter;
      iter=this->CubeAxesRepMap.find(dataRepr);
      if(iter!=this->CubeAxesRepMap.end())
      {
        vtkSMViewProxy* renv= view->getViewProxy();
        vtkSMPropertyHelper(renv, "HiddenRepresentations").Remove(iter.value());
        this->CubeAxesViewMap.remove(iter.value());
      }
    }
  }
}
void PrismCore::onViewRepresentationAdded(pqRepresentation* repr)
{
  //If the Rep is for a Prism filter then we need to add the Cube Axes rep to the same view.
  pqDataRepresentation* dataRepr=qobject_cast<pqDataRepresentation*>(repr);
  if(dataRepr)
  {
    pqPipelineSource* input = dataRepr->getInput();
    QString name=input->getProxy()->GetXMLName();
    if(name=="PrismFilter" || name=="PrismSurfaceReader")
    {
      QMap<pqDataRepresentation*,vtkSMPrismCubeAxesRepresentationProxy*>::iterator iter;
      iter=this->CubeAxesRepMap.find(dataRepr);
      if(iter!=this->CubeAxesRepMap.end())
      {

        pqRenderView * view= qobject_cast<pqRenderView*>(dataRepr->getView());
        if(view)
        {
          vtkSMViewProxy* renv= view->getViewProxy();
          vtkSMPropertyHelper(renv, "HiddenRepresentations").Add(iter.value());
          this->CubeAxesViewMap[iter.value()]=view;
          renv->UpdateVTKObjects();
          view->render();
        }
      }
    }
  }
}
void PrismCore::onViewRepresentationRemoved(pqRepresentation* repr)
{
  //If the rep is for a Prism filter than we need to remove the cube axes from the same view.
  pqDataRepresentation* dataRepr=qobject_cast<pqDataRepresentation*>(repr);
  if(dataRepr)
  {
    pqPipelineSource* input = dataRepr->getInput();
    QString name=input->getProxy()->GetXMLName();
    if(name=="PrismFilter" || name=="PrismSurfaceReader")
    {
      QMap<pqDataRepresentation*,vtkSMPrismCubeAxesRepresentationProxy*>::iterator iter;
      iter=this->CubeAxesRepMap.find(dataRepr);
      if(iter!=this->CubeAxesRepMap.end())
      {
        QMap<vtkSMPrismCubeAxesRepresentationProxy*,pqRenderView*>::iterator vIter;
        vIter=this->CubeAxesViewMap.find(iter.value());
        if(vIter!=this->CubeAxesViewMap.end())
        {
          pqRenderView * view= vIter.value();
          if(view)
          {
            vtkSMViewProxy* renv= view->getViewProxy();
            vtkSMPropertyHelper(renv, "HiddenRepresentations").Remove(iter.value());
            this->CubeAxesViewMap.erase(vIter);
            renv->UpdateVTKObjects();
            view->render();
          }
        }
      }
    }
  }
}

void PrismCore::onPreRepresentationRemoved(pqRepresentation* repr)
{
  //If the rep is for a Prism filter we need to remove the cube axes from its view and dekete it,
  pqDataRepresentation* dataRepr=qobject_cast<pqDataRepresentation*>(repr);
  if(dataRepr)
  {
    pqPipelineSource* input = dataRepr->getInput();
    QString name=input->getProxy()->GetXMLName();
    if(name=="PrismFilter" || name=="PrismSurfaceReader")
    {
      QMap<pqDataRepresentation*,vtkSMPrismCubeAxesRepresentationProxy*>::iterator iter;
      iter=this->CubeAxesRepMap.find(dataRepr);
      if(iter!=this->CubeAxesRepMap.end())
      {
        vtkSMPrismCubeAxesRepresentationProxy* cubeAxes=iter.value();
        QMap<vtkSMPrismCubeAxesRepresentationProxy*,pqRenderView*>::iterator vIter;
        vIter=this->CubeAxesViewMap.find(cubeAxes);
        if(vIter!=this->CubeAxesViewMap.end())
        {
          pqRenderView * view=vIter.value();
          if(view)
          {
            vtkSMViewProxy* renv= view->getViewProxy();
            vtkSMPropertyHelper(renv, "HiddenRepresentations").Remove(iter.value());
            this->CubeAxesViewMap.erase(vIter);
            renv->UpdateVTKObjects();
            view->render();
          }
        }
        vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
        pxm->UnRegisterProxy(iter.value()->GetXMLGroup(),iter.value()->GetClassName(),iter.value());
        this->CubeAxesRepMap.erase(iter);
      }
    }
  }
}

void PrismCore::onPrismRepresentationAdded(pqPipelineSource* source,
                                           pqDataRepresentation* repr,
                                           int srcOutputPort)
{

  pqApplicationCore* core = pqApplicationCore::instance();
  pqObjectBuilder* builder = core->getObjectBuilder();
  pqServer* server = 0;

  server = source->getServer();
  if(!server)
  {
    qDebug() << "No active server selected.";
  }

  vtkSMPrismCubeAxesRepresentationProxy* cubeAxesActor=vtkSMPrismCubeAxesRepresentationProxy::SafeDownCast(builder->createProxy("props",
    "PrismCubeAxesRepresentation", server,
    "props"));

  this->CubeAxesRepMap[repr]=cubeAxesActor;

  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    cubeAxesActor->GetProperty("Input"));
  vtkSMInputProperty* ip = vtkSMInputProperty::SafeDownCast(pp);
  if (!pp)
  {
    vtkErrorWithObjectMacro(cubeAxesActor,"Failed to locate property " << "Input"
      << " on the consumer " <<  (cubeAxesActor->GetXMLName()));
    return;
  }

  if (ip)
  {
    ip->RemoveAllProxies();
    ip->AddInputConnection(source->getProxy(),0);
  }
  else
  {
    pp->RemoveAllProxies();
    pp->AddProxy(source->getProxy());
  }
  cubeAxesActor->UpdateProperty("Input");


  if(srcOutputPort==0)
  {
    pqSMAdaptor::setElementProperty(repr->getProxy()->GetProperty("Pickable"),0);
  }

}
void PrismCore::onPrismSelection(vtkObject* caller,
                                 unsigned long,
                                 void* client_data, void* call_data)
{
  if(!this->ProcessingEvent)
  {
    this->ProcessingEvent=true;

    unsigned int portIndex = *(unsigned int*)call_data;
    vtkSMSourceProxy* prismP=static_cast<vtkSMSourceProxy*>(caller);
    vtkSMSourceProxy* sourceP=static_cast<vtkSMSourceProxy*>(client_data);

    pqServerManagerModel* model= pqApplicationCore::instance()->getServerManagerModel();
    pqPipelineSource* pqPrismP=model->findItem<pqPipelineSource*>(prismP);


    vtkSMSourceProxy* selPrism = prismP->GetSelectionInput(portIndex);
    if(!selPrism)
    {
      sourceP->CleanSelectionInputs(0);
      this->ProcessingEvent=false;
      pqPipelineSource* pqSourceP=model->findItem<pqPipelineSource*>(sourceP);
      if(pqSourceP)
      {
        QList<pqView*> Views=pqSourceP->getViews();

        foreach(pqView* v,Views)
        {
          v->render();
        }
      }
      return;
    }

    pqApplicationCore* const core = pqApplicationCore::instance();
    pqSelectionManager* slmanager = qobject_cast<pqSelectionManager*>(
      core->manager("SelectionManager"));
    pqOutputPort* opport = pqPrismP->getOutputPort(portIndex);

    slmanager->select(opport);




    vtkSMSourceProxy* newSource=NULL;
    if (strcmp(selPrism->GetXMLName(), "GlobalIDSelectionSource") != 0)
    {
      newSource = vtkSMSourceProxy::SafeDownCast(
        vtkSMSelectionHelper::ConvertSelection(vtkSelectionNode::GLOBALIDS,
        selPrism,
        prismP,
        portIndex));

      if(!newSource)
      {
        return;
      }
      newSource->UpdateVTKObjects();
   //   prismP->SetSelectionInput(portIndex,newSource, 0);
      selPrism=newSource;
    }



    vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();

    vtkSMSourceProxy* selSource=vtkSMSourceProxy::SafeDownCast(
      pxm->NewProxy("sources", "GlobalIDSelectionSource"));





    pxm->UnRegisterLink(prismP->GetSelfIDAsString());//TODO we need a unique id that represents the connection.
    //Otherwise we geometry in multiple SESAME views and vise versa.




    vtkSMPropertyLink* link = vtkSMPropertyLink::New();

    // bi-directional link

    link->AddLinkedProperty(selPrism,
      "IDs",
      vtkSMLink::INPUT);
    link->AddLinkedProperty(selSource,
      "IDs",
      vtkSMLink::OUTPUT);
    link->AddLinkedProperty(selSource,
      "IDs",
      vtkSMLink::INPUT);
    link->AddLinkedProperty(selPrism,
      "IDs",
      vtkSMLink::OUTPUT);
    pxm->RegisterLink(prismP->GetSelfIDAsString(), link);
    link->Delete();




    selSource->UpdateVTKObjects();
    sourceP->SetSelectionInput(0,selSource,0);

    selSource->Delete();

    if(newSource)
    {
      newSource->Delete();
    }


    pqPipelineSource* pqPrismSourceP=model->findItem<pqPipelineSource*>(sourceP);
    QList<pqView*> Views=pqPrismSourceP->getViews();

    foreach(pqView* v,Views)
    {
      v->render();
    }

    this->ProcessingEvent=false;
  }
}

void PrismCore::onGeometrySelection(vtkObject* caller,
                                    unsigned long,
                                    void* client_data, void* call_data)
{
  if(!this->ProcessingEvent)
  {
    this->ProcessingEvent=true;

    unsigned int portIndex = *(unsigned int*)call_data;
    vtkSMSourceProxy* sourceP=static_cast<vtkSMSourceProxy*>(caller);
    vtkSMSourceProxy* prismP=static_cast<vtkSMSourceProxy*>(client_data);

    pqServerManagerModel* model= pqApplicationCore::instance()->getServerManagerModel();
    pqPipelineSource* pqSourceP=model->findItem<pqPipelineSource*>(sourceP);

    vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();

    vtkSMSourceProxy* selSource = sourceP->GetSelectionInput(portIndex);
    if(!selSource)
    {
      prismP->CleanSelectionInputs(3);
      this->ProcessingEvent=false;
      pqPipelineSource* pqPrismP=model->findItem<pqPipelineSource*>(prismP);
      if(pqPrismP)
      {
        QList<pqView*> Views=pqPrismP->getViews();

        foreach(pqView* v,Views)
        {
          v->render();
        }
      }
      return;
    }

    pqApplicationCore* const core = pqApplicationCore::instance();
    pqSelectionManager* slmanager = qobject_cast<pqSelectionManager*>(
      core->manager("SelectionManager"));
    pqOutputPort* opport = pqSourceP->getOutputPort(portIndex);

    slmanager->select(opport);



    vtkSMSourceProxy* newSource=NULL;
    if (strcmp(selSource->GetXMLName(), "GlobalIDSelectionSource") != 0)
    {
      newSource = vtkSMSourceProxy::SafeDownCast(
        vtkSMSelectionHelper::ConvertSelection(vtkSelectionNode::GLOBALIDS,
        selSource,
        sourceP,
        portIndex));

      if(!newSource)
      {
        return;
      }
      newSource->UpdateVTKObjects();
   //   sourceP->SetSelectionInput(portIndex,newSource, 0);
      selSource=newSource;
    }


    vtkSMSourceProxy* selPrism=vtkSMSourceProxy::SafeDownCast(
      pxm->NewProxy("sources", "GlobalIDSelectionSource"));




    pxm->UnRegisterLink(prismP->GetSelfIDAsString());//TODO we need a unique id that represents the connection.
    //Otherwise we geometry in multiple SESAME views and vise versa.



    vtkSMPropertyLink* link = vtkSMPropertyLink::New();

    // bi-directional link

    link->AddLinkedProperty(selSource,
      "IDs",
      vtkSMLink::INPUT);
    link->AddLinkedProperty(selPrism,
      "IDs",
      vtkSMLink::OUTPUT);
    link->AddLinkedProperty(selPrism,
      "IDs",
      vtkSMLink::INPUT);
    link->AddLinkedProperty(selSource,
      "IDs",
      vtkSMLink::OUTPUT);
    pxm->RegisterLink(prismP->GetSelfIDAsString(), link);
    link->Delete();


    selPrism->UpdateVTKObjects();
    prismP->SetSelectionInput(3,selPrism,0);
    selPrism->Delete();
    if(newSource)
    {
      newSource->Delete();
    }




    pqPipelineSource* pqPrismSourceP=model->findItem<pqPipelineSource*>(prismP);
    QList<pqView*> Views=pqPrismSourceP->getViews();

    foreach(pqView* v,Views)
    {
      v->render();
    }

    this->ProcessingEvent=false;
  }
}
void PrismCore::onSelectionChanged()
{
  pqServerManagerSelectionModel *selection =
    pqApplicationCore::instance()->getSelectionModel();
  pqServerManagerModelItem *item = selection->currentItem();
  if(item)
  {
    pqOutputPort* port = qobject_cast<pqOutputPort*>(item);
    pqPipelineSource *source = NULL;
    int portNumber=0;
    if(port)
    {
      source=port->getSource();
      portNumber=port->getPortNumber();
    }
    if(!source)
    {
      source= qobject_cast<pqPipelineSource *>(item);
    }

    if(source)
    {
      vtkSMProxyManager *proxyManager = vtkSMProxyManager::GetProxyManager();
      proxyManager->InstantiateGroupPrototypes("filters");
      vtkSMProxy* prismFilter = proxyManager->GetProxy("filters_prototypes", "PrismFilter");

      if (source && prismFilter)
      {
        vtkSMProperty *inputP = prismFilter->GetProperty("Input");
        vtkSMInputProperty* input = vtkSMInputProperty::SafeDownCast(inputP);

        if (input)
        {
          if (input->GetNumberOfProxies() == 1)
          {
            input->SetUncheckedInputConnection(0, source->getProxy(), portNumber);
          }
          else
          {
            input->RemoveAllUncheckedProxies();
            input->AddUncheckedInputConnection(source->getProxy(), portNumber);
          }

          if(input->IsInDomains())
          {
            if(this->PrismViewAction)
            {
              this->PrismViewAction->setEnabled(true);
            }
            if(this->MenuPrismViewAction)
            {
              this->MenuPrismViewAction->setEnabled(true);
            }
            return;
          }
        }
      }
    }
  }

  if(this->PrismViewAction)
  {
    this->PrismViewAction->setEnabled(false);
  }
  if(this->MenuPrismViewAction)
  {
    this->MenuPrismViewAction->setEnabled(false);
  }
}
