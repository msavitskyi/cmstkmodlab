#include <string>

#include <QGroupBox>
#include <QFileDialog>
#include <QProcess>
#include <QApplication>
#include <QTabWidget>

#include "ApplicationConfig.h"

#include "DefoImageWidget.h"
#include "DefoRecoImageWidget.h"

#include "DefoRecoMainWindow.h"

#include "DefoROICopyFromWidget.h"
#include "DefoROICopyToWidget.h"
#include "DefoROIIOWidget.h"
#include "DefoPointRecognitionModel.h"
#include "DefoRecoPointRecognitionWidget.h"
#include "DefoMeasurementListComboBox.h"
#include "DefoPointIndexerListComboBox.h"
#include "DefoRecoColorHistoWidget.h"

//#define ANALYSISWIDGET

#ifdef ANALYSISWIDGET
#include "DefoAnalysisWidget.h"
#endif

DefoRecoMainWindow::DefoRecoMainWindow(QWidget *parent)
: QMainWindow(parent)
{
  ApplicationConfig* config = ApplicationConfig::instance();

  connect(QApplication::instance(), SIGNAL(aboutToQuit()),
          this, SLOT(quit()));

  currentDir_ = config->getValue<std::string>("DataPath").c_str();

  // MEASUREMENT MODEL
  listModel_ = new DefoRecoMeasurementListModel(this);
  selectionModel_ = new DefoMeasurementSelectionModel(this);

  roiSelectionModel_ = new DefoMeasurementSelectionModel(this);
  roiModel_ = new DefoRecoROIModel(roiSelectionModel_, this);

  alignmentSelectionModel_ = new DefoMeasurementSelectionModel(this);
  alignmentModel_ = new DefoAlignmentModel(this);

  // POINT MODEL
  refSelectionModel_ = new DefoMeasurementSelectionModel(this);
  refROIModel_ = new DefoRecoROIModel(refSelectionModel_, roiModel_, this);
  refPointModel_ = new DefoPointRecognitionModel(this);
  defoSelectionModel_ = new DefoMeasurementSelectionModel(this);
  defoROIModel_ = new DefoRecoROIModel(defoSelectionModel_, roiModel_, this);
  defoPointModel_ = new DefoPointRecognitionModel(this);

  pointIndexerModel_ = new DefoPointIndexerModel(this);

  refColorModel_ = new DefoColorSelectionModel(this);
  defoColorModel_ = new DefoColorSelectionModel(this);

  lensModel_ = new DefoLensModel(this);
  geometryModel_ = new DefoGeometryModel(this);
  calibrationModel_ = new DefoCalibrationModel(this);
  interpolationModel_ = new Defo2DSplineInterpolationModel(this);

  measurementPairListModel_ = new DefoMeasurementPairListModel(this);
  measurementPairSelectionModel_ = new DefoMeasurementPairSelectionModel(this);

  offlineModel_ = new DefoOfflinePreparationModel(listModel_,
                                                  refSelectionModel_,
                                                  defoSelectionModel_,
                                                  alignmentModel_,
                                                  pointIndexerModel_,
                                                  refColorModel_,
                                                  defoColorModel_,
                                                  measurementPairListModel_,
                                                  measurementPairSelectionModel_,
                                                  this);
  offlineModel_->setCurrentDir(currentDir_);

  reconstructionModel_ = new DefoReconstructionModel(listModel_,
                                                     refSelectionModel_,
                                                     defoSelectionModel_,
                                                     alignmentModel_,
                                                     pointIndexerModel_,
                                                     refColorModel_,
                                                     defoColorModel_,
                                                     measurementPairListModel_,
                                                     measurementPairSelectionModel_,
						     lensModel_,
                                                     geometryModel_,
                                                     calibrationModel_,
                                                     interpolationModel_,
                                                     this);
  reconstructionModel_->setCurrentDir(currentDir_);

  tabWidget_ = new QTabWidget(this);
  tabWidget_->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

  QBoxLayout *layout = new QHBoxLayout();
  QWidget * measurementWidget = new QWidget(tabWidget_);
  measurementWidget->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
  measurementWidget->setLayout(layout);

  QBoxLayout *vbox = new QVBoxLayout();
  vbox->setContentsMargins(0, 0, 0, 0);
  QWidget * measurementInfoWidget = new QWidget(measurementWidget);
  measurementInfoWidget->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
  measurementInfoWidget->setLayout(vbox);
  layout->addWidget(measurementInfoWidget);

  QBoxLayout *hbox = new QHBoxLayout();
  hbox->setContentsMargins(0, 0, 0, 0);
  QWidget * measurementButtonWidget = new QWidget(measurementInfoWidget);
  measurementButtonWidget->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
  measurementButtonWidget->setLayout(hbox);
  vbox->addWidget(measurementButtonWidget);

  QPushButton * loadMeasurementButton = new QPushButton("&Load", measurementButtonWidget);
  connect(loadMeasurementButton, SIGNAL(clicked()),
   	  this, SLOT(loadMeasurementButtonClicked()));
  hbox->addWidget(loadMeasurementButton);

  QPushButton * saveMeasurementButton = new QPushButton("&Save", measurementButtonWidget);
  connect(saveMeasurementButton, SIGNAL(clicked()),
          this, SLOT(saveMeasurementButtonClicked()));
  hbox->addWidget(saveMeasurementButton);

  QPushButton * exportMeasurementButton = new QPushButton("&Export", measurementButtonWidget);
  connect(exportMeasurementButton, SIGNAL(clicked()),
          this, SLOT(exportMeasurementButtonClicked()));
  hbox->addWidget(exportMeasurementButton);

  DefoMeasurementListTreeWidget * treeWidget = new DefoMeasurementListTreeWidget(listModel_,
                                                                                 selectionModel_,
                                                                                 measurementInfoWidget);
  vbox->addWidget(treeWidget);

  DefoMeasurementCommentTextView * commentView = new DefoMeasurementCommentTextView(selectionModel_,
                                                                                    measurementInfoWidget);
  vbox->addWidget(commentView);

  DefoMeasurementInfoListTreeWidget * infoTreeWidget = new DefoMeasurementInfoListTreeWidget(selectionModel_,
                                                                                             measurementInfoWidget);
  vbox->addWidget(infoTreeWidget);

  DefoRecoRawImageWidget *rawImage = new DefoRecoRawImageWidget(selectionModel_,
                                                                measurementInfoWidget);
  layout->addWidget(rawImage);

  tabWidget_->addTab(measurementWidget, "Measurement");

  vbox = new QVBoxLayout();
  QWidget * roiWidget = new QWidget(tabWidget_);
  roiWidget->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
  roiWidget->setLayout(vbox);

  DefoMeasurementListComboBox *roiSelect = new DefoMeasurementListComboBox(listModel_,
                                                                           roiSelectionModel_,
                                                                           roiWidget);
  vbox->addWidget(roiSelect);

  DefoRecoROIImageWidget *roiImage = new DefoRecoROIImageWidget(roiSelectionModel_,
                                                                roiModel_,
                                                                roiWidget);
  vbox->addWidget(roiImage);

  DefoROICopyFromWidget *roiCopyFrom = new DefoROICopyFromWidget(listModel_,
                                                                 roiModel_,
                                                                 roiWidget);
  vbox->addWidget(roiCopyFrom);

  DefoROICopyToWidget *roiCopyTo = new DefoROICopyToWidget(listModel_,
                                                           roiModel_,
                                                           roiWidget);
  vbox->addWidget(roiCopyTo);

  DefoROIIOWidget *roiIO = new DefoROIIOWidget(listModel_,
                                               roiModel_,
                                               roiWidget);
  vbox->addWidget(roiIO);

  tabWidget_->addTab(roiWidget, "ROI");

  layout = new QHBoxLayout();
  QWidget * pointsWidget = new QWidget(tabWidget_);
  pointsWidget->setLayout(layout);

  vbox = new QVBoxLayout();
  vbox->setContentsMargins(0, 0, 0, 0);
  QWidget * refPointWidget = new QWidget(pointsWidget);
  refPointWidget->setLayout(vbox);

  DefoMeasurementListComboBox *refSelect = new DefoMeasurementListComboBox(listModel_,
                                                                           refSelectionModel_,
                                                                           refPointWidget);
  vbox->addWidget(refSelect);

  DefoRecoPointRecognitionWidget * refPointRecognitionWidget =
      new DefoRecoPointRecognitionWidget(listModel_,
                                         refSelectionModel_,
                                         refPointModel_,
                                         refROIModel_,
                                         refPointWidget);
  refPointModel_->setThresholdValue(DefoPointRecognitionModel::THRESHOLD_1,
				    ApplicationConfig::instance()->getValue<int>("STEP1_THRESHOLD"));
  refPointModel_->setThresholdValue(DefoPointRecognitionModel::THRESHOLD_2,
				    ApplicationConfig::instance()->getValue<int>("STEP2_THRESHOLD"));
  refPointModel_->setThresholdValue(DefoPointRecognitionModel::THRESHOLD_3, 
				    ApplicationConfig::instance()->getValue<int>("STEP3_THRESHOLD"));
  refPointModel_->setHalfSquareWidth(ApplicationConfig::instance()->getValue<int>("HALF_SQUARE_WIDTH"));
  vbox->addWidget(refPointRecognitionWidget);

  layout->addWidget(refPointWidget);

  vbox = new QVBoxLayout();
  vbox->setContentsMargins(0, 0, 0, 0);
  QWidget * defoPointWidget = new QWidget(pointsWidget);
  defoPointWidget->setLayout(vbox);
  
  DefoMeasurementListComboBox *defoSelect = new DefoMeasurementListComboBox(listModel_,
                                                                            defoSelectionModel_,
                                                                            defoPointWidget);
  vbox->addWidget(defoSelect);

  DefoRecoPointRecognitionWidget * defoPointRecognitionWidget =
          new DefoRecoPointRecognitionWidget(listModel_,
                                             defoSelectionModel_,
                                             defoPointModel_,
                                             defoROIModel_,
                                             defoPointWidget);
  defoPointModel_->setThresholdValue(DefoPointRecognitionModel::THRESHOLD_1,
				     ApplicationConfig::instance()->getValue<int>("STEP1_THRESHOLD"));
  defoPointModel_->setThresholdValue(DefoPointRecognitionModel::THRESHOLD_2,
				     ApplicationConfig::instance()->getValue<int>("STEP2_THRESHOLD"));
  defoPointModel_->setThresholdValue(DefoPointRecognitionModel::THRESHOLD_3, 
				     ApplicationConfig::instance()->getValue<int>("STEP3_THRESHOLD"));
  defoPointModel_->setHalfSquareWidth(ApplicationConfig::instance()->getValue<int>("HALF_SQUARE_WIDTH"));
  vbox->addWidget(defoPointRecognitionWidget);

  layout->addWidget(defoPointWidget);

  tabWidget_->addTab(pointsWidget, "Points");

  vbox = new QVBoxLayout();
  QWidget * alignmentWidget = new QWidget(tabWidget_);
  alignmentWidget->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
  alignmentWidget->setLayout(vbox);

  DefoMeasurementListComboBox *alignmentSelect = new DefoMeasurementListComboBox(listModel_,
                                                                                 alignmentSelectionModel_,
                                                                                 alignmentWidget);
  vbox->addWidget(alignmentSelect);

  DefoRecoAlignmentImageWidget *alignmentImage = new DefoRecoAlignmentImageWidget(alignmentSelectionModel_,
                                                                                  alignmentModel_,
                                                                                  alignmentWidget);
  vbox->addWidget(alignmentImage);

  tabWidget_->addTab(alignmentWidget, "Alignment");

  vbox = new QVBoxLayout();
  QWidget * indexerWidget = new QWidget(tabWidget_);
  indexerWidget->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
  indexerWidget->setLayout(vbox);

  DefoPointIndexerListComboBox *indexerSelect = new DefoPointIndexerListComboBox(pointIndexerModel_,
                                                                                 indexerWidget);
  vbox->addWidget(indexerSelect);

  hbox = new QHBoxLayout();
  QWidget * indexerColorWidget = new QWidget(indexerWidget);
  indexerColorWidget->setLayout(hbox);
  vbox->addWidget(indexerColorWidget);
  
  DefoRecoColorHistoWidget *refIndexerSelect = new DefoRecoColorHistoWidget(listModel_,
                                                                            refSelectionModel_,
                                                                            refColorModel_,
                                                                            indexerColorWidget);
  hbox->addWidget(refIndexerSelect);

  DefoRecoColorHistoWidget *defoIndexerSelect = new DefoRecoColorHistoWidget(listModel_,
                                                                             defoSelectionModel_,
                                                                             defoColorModel_,
                                                                             indexerColorWidget);
  hbox->addWidget(defoIndexerSelect);
  
  QPushButton * offlineButton = new QPushButton("&Prepare for offline processing", indexerWidget);
  connect(offlineButton, SIGNAL(clicked()),
          offlineModel_, SLOT(prepare()));
  vbox->addWidget(offlineButton);

  tabWidget_->addTab(indexerWidget, "Indexer");

  vbox = new QVBoxLayout();
  QWidget * recoWidget = new QWidget(tabWidget_);
  recoWidget->setLayout(vbox);

  DefoReconstructionWidget *recoControllerWidget = new DefoReconstructionWidget(reconstructionModel_,
                                                                                recoWidget);
  vbox->addWidget(recoControllerWidget);

  DefoGeometryWidget *geometryWidget = new DefoGeometryWidget(geometryModel_,
                                                              recoWidget);
  vbox->addWidget(geometryWidget);

  /*
  hbox = new QHBoxLayout();
  hbox->setContentsMargins(0, 0, 0, 0);
  QWidget * parameterWidget = new QWidget(recoWidget);
  parameterWidget->setLayout(hbox);

  DefoGeometryWidget *geometryWidget = new DefoGeometryWidget(geometryModel_,
                                                              parameterWidget);
  hbox->addWidget(geometryWidget);

  DefoReconstructionParameterWidget *recoParameter = new DefoReconstructionParameterWidget(geometryModel_,
											   calibrationModel_,
											   interpolationModel_,
											   parameterWidget);
  hbox->addWidget(recoParameter);

  vbox->addWidget(parameterWidget);
  */

  QTabWidget * parameterTabs = new QTabWidget(recoWidget);
  parameterTabs->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

  DefoLensComboBox *lensWidget = new DefoLensComboBox(lensModel_,
						  parameterTabs);
  parameterTabs->addTab(lensWidget, "Lens");

  DefoGeometryParameterWidget *geometryParameterWidget = new DefoGeometryParameterWidget(geometryModel_,
                                                                                         parameterTabs);
  parameterTabs->addTab(geometryParameterWidget, "Geometry");

  DefoCalibrationWidget *calibrationWidget = new DefoCalibrationWidget(calibrationModel_,
                                                                       parameterTabs);
  parameterTabs->addTab(calibrationWidget, "Calibration");

  Defo2DSplineInterpolationWidget *interpolationWidget = new Defo2DSplineInterpolationWidget(interpolationModel_,
                                                                                             parameterTabs);
  parameterTabs->addTab(interpolationWidget, "Interpolation");

  vbox->addWidget(parameterTabs);

  tabWidget_->addTab(recoWidget, "Reconstruction");

#ifdef ANALYSISWIDGET
  vbox = new QVBoxLayout();
  QWidget * analysisWidget = new QWidget(tabWidget_);
  analysisWidget->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
  analysisWidget->setLayout(vbox);

  DefoMeasurementPairListComboBox *analysisSelect = new DefoMeasurementPairListComboBox(measurementPairListModel_,
                                                                                        measurementPairSelectionModel_,
                                                                                        analysisWidget);
  vbox->addWidget(analysisSelect);

  DefoAnalysisWidget *analysisDisplay = new DefoAnalysisWidget(measurementPairListModel_,
                                                               measurementPairSelectionModel_,
                                                               analysisWidget);
  vbox->addWidget(analysisDisplay);

  tabWidget_->addTab(analysisWidget, "Analysis");
#endif

  setCentralWidget(tabWidget_);

  pointIndexerModel_->setSelectedIndexer(pointIndexerModel_->getIndexer(0));
}

void DefoRecoMainWindow::quit()
{
  ApplicationConfig* config = ApplicationConfig::instance();
  config->safe(std::string(Config::CMSTkModLabBasePath) + "/defo/defo.cfg");
}

void DefoRecoMainWindow::loadMeasurementButtonClicked()
{
  QString filename = QFileDialog::getOpenFileName(this,
                                                  QString("Load Measurement"),
                                                  currentDir_.absolutePath(),
                                                  QString("ODM Measurement Files (*.odmx);;ODM Measurement Archive (*.odma)"));
  if (filename.isNull()) return;

  if (filename.endsWith(".odma")) {
    QString dirname = filename;
    dirname.remove(0, dirname.lastIndexOf('/'));
    dirname.remove(".odma");

    QStringList args;
    args << "-cvzf";
    args << filename;
    args << "-C";
    args << QDir::temp().path();

    QProcess tar(this);
    tar.start("tar", args);

    QDir tempDir = QDir::temp();
    tempDir.cd(dirname);

    tempDirs_.append(tempDir);
    filename = tempDir.absoluteFilePath("measurements.odmx");
  }

  QFileInfo fi(filename);
  currentDir_ = fi.absolutePath();
  std::cout << currentDir_.absolutePath().toStdString() << std::endl;

  alignmentModel_->read(currentDir_.absoluteFilePath("alignment.xml"));
  refColorModel_->read(currentDir_.absoluteFilePath("refcolor.xml"));
  defoColorModel_->read(currentDir_.absoluteFilePath("defocolor.xml"));
  lensModel_->read(currentDir_.absoluteFilePath("lens.xml"));
  geometryModel_->read(currentDir_.absoluteFilePath("geometry.xml"));
  calibrationModel_->read(currentDir_.absoluteFilePath("calibration.xml"));
  interpolationModel_->read(currentDir_.absoluteFilePath("interpolation.xml"));

  listModel_->clear();
  listModel_->read(filename);
  listModel_->readPoints(currentDir_);
  offlineModel_->setCurrentDir(currentDir_);
  reconstructionModel_->setCurrentDir(currentDir_);
}

void DefoRecoMainWindow::saveMeasurementButtonClicked()
{
  alignmentModel_->write(currentDir_.absoluteFilePath("alignment.xml"));
  refColorModel_->write(currentDir_.absoluteFilePath("refcolor.xml"));
  defoColorModel_->write(currentDir_.absoluteFilePath("defocolor.xml"));
  lensModel_->write(currentDir_.absoluteFilePath("lens.xml"));
  geometryModel_->write(currentDir_.absoluteFilePath("geometry.xml"));
  calibrationModel_->write(currentDir_.absoluteFilePath("calibration.xml"));
  interpolationModel_->write(currentDir_.absoluteFilePath("interpolation.xml"));

  listModel_->write(currentDir_);
  listModel_->writePoints(currentDir_);
}

void DefoRecoMainWindow::exportMeasurementButtonClicked()
{
  saveMeasurementButtonClicked();

  QString filename = QFileDialog::getSaveFileName(this,
                                                  "export measurement",
                                                  "./",
                                                  "ODM Measurement Archive (*.odma)",
                                                  0,
                                                  0);
  if (filename.isNull()) return;
  if (!filename.endsWith(".odma")) filename += ".odma";

  listModel_->exportMeasurement(currentDir_.absolutePath(), filename);
}
