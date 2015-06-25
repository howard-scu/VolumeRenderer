#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <QApplication>
#include <QMessageBox>
#include <QFileDialog>

#include <iostream>
#include <memory>
#include <cstdlib>

#include <QVTKWidget.h>
#include <vtkSmartPointer.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkColorTransferFunction.h>
#include <vtkPiecewiseFunction.h>
#include <vtkSmartVolumeMapper.h>
#include <vtkColorTransferFunction.h>
#include <vtkPiecewiseFunction.h>
#include <vtkVolumeProperty.h>
#include <vtkMetaImageReader.h>
#include <vtkVolume16Reader.h>
#include <vtkNew.h>
#include <vtkNrrdReader.h>
#include <vtkImageShiftScale.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkRendererCollection.h>

#include "ctkTransferFunction.h"
#include "ctkVTKColorTransferFunction.h"
#include "ctkTransferFunctionView.h"
#include "ctkTransferFunctionGradientItem.h"
#include "ctkTransferFunctionControlPointsItem.h"
#include "ctkVTKVolumePropertyWidget.h"

#include "ui_mainwindow.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    
private:
    Ui::MainWindow *ui;

	QString filename;
	vtkSmartPointer<vtkRenderWindowInteractor> interactor;
	QVTKWidget widget;
	ctkVTKVolumePropertyWidget volumePropertywidget;
	
private slots:
		void onAboutSlot()
		{
			QMessageBox msgBox;
			msgBox.setText(QString::fromUtf8("Volume Renderer"));
			msgBox.exec();
		}

		void onExitSlot()
		{
			qApp->quit();
		}

		void onOpenSlot()
		{
			// show file dialog. change filename only when the new filename is not empty.
			QString filter("Meta image file (*.mhd *.mha)");
			QString filename_backup = filename;
			filename_backup = QFileDialog::getOpenFileName(this, QString(tr("Open a volume data set")), filename_backup, filter);
			if (!filename_backup.trimmed().isEmpty())
			{
				filename = filename_backup;
			}
			else
			{
				return;
			}

			// show filename on window title
			this->setWindowTitle(QString::fromUtf8("Volume Renderer - ") + filename);

			// get local 8-bit representation of the string in locale encoding (in case the filename contains non-ASCII characters) 
			QByteArray ba = filename.toLocal8Bit();  
			const char *filename_str = ba.data();

#if 1
			// read Meta Image (.mhd or .mha) files
			auto reader = vtkSmartPointer<vtkMetaImageReader>::New();
			reader->SetFileName(filename_str);
#elif 1
			// read a series of raw files in the specified folder
			auto reader = vtkSmartPointer<vtkVolume16Reader>::New();
			reader->SetDataDimensions (512, 512);
			reader->SetImageRange (1, 361);
			reader->SetDataByteOrderToBigEndian();
			reader->SetFilePrefix(filename_str);
			reader->SetFilePattern("%s%d");
			reader->SetDataSpacing(1, 1, 1);
#else
			// read NRRD files
			vtkNew<vtkNrrdReader> reader;
			if (!reader->CanReadFile(filename_str))
			{
				std::cerr << "Reader reports " << filename_str << " cannot be read.";
				exit(EXIT_FAILURE);
			}
			reader->SetFileName(filename_str);
			reader->Update();
#endif

			// scale the volume data to unsigned char (0-255) before passing it to volume mapper
			auto shiftScale = vtkSmartPointer<vtkImageShiftScale>::New();
			shiftScale->SetInputConnection(reader->GetOutputPort());
			shiftScale->SetOutputScalarTypeToUnsignedChar();

			// Create transfer mapping scalar value to opacity.
			auto opacityTransferFunction = vtkSmartPointer<vtkPiecewiseFunction>::New();
			opacityTransferFunction->AddPoint(0.0,  0.0);
			opacityTransferFunction->AddPoint(36.0,  0.125);
			opacityTransferFunction->AddPoint(72.0,  0.25);
			opacityTransferFunction->AddPoint(108.0, 0.375);
			opacityTransferFunction->AddPoint(144.0, 0.5);
			opacityTransferFunction->AddPoint(180.0, 0.625);
			opacityTransferFunction->AddPoint(216.0, 0.75);
			opacityTransferFunction->AddPoint(255.0, 0.875);

			// Create transfer mapping scalar value to color.
			auto colorTransferFunction = vtkSmartPointer<vtkColorTransferFunction>::New();
			colorTransferFunction->AddRGBPoint(0.0,  0.0, 0.0, 0.0);
			colorTransferFunction->AddRGBPoint(36.0, 1.0, 0.0, 0.0);
			colorTransferFunction->AddRGBPoint(72.0, 1.0, 1.0, 0.0);
			colorTransferFunction->AddRGBPoint(108.0, 0.0, 1.0, 0.0);
			colorTransferFunction->AddRGBPoint(144.0, 0.0, 1.0, 1.0);
			colorTransferFunction->AddRGBPoint(180.0, 0.0, 0.0, 1.0);
			colorTransferFunction->AddRGBPoint(216.0, 1.0, 0.0, 1.0);
			colorTransferFunction->AddRGBPoint(255.0, 1.0, 1.0, 1.0);

			// set up volume property
			auto volumeProperty = vtkSmartPointer<vtkVolumeProperty>::New();
			volumeProperty->SetColor(colorTransferFunction);
			volumeProperty->SetScalarOpacity(opacityTransferFunction);
			volumeProperty->ShadeOff();
			volumeProperty->SetInterpolationTypeToLinear();

			// assign volume property to the volume property widget
			volumePropertywidget.setVolumeProperty(volumeProperty);

			// The mapper that renders the volume data.
			auto volumeMapper = vtkSmartPointer<vtkSmartVolumeMapper>::New();
			volumeMapper->SetRequestedRenderMode(vtkSmartVolumeMapper::GPURenderMode);
			volumeMapper->SetInputConnection(shiftScale->GetOutputPort());

			// The volume holds the mapper and the property and can be used to position/orient the volume.
			auto volume = vtkSmartPointer<vtkVolume>::New();
			volume->SetMapper(volumeMapper);
			volume->SetProperty(volumeProperty);

			// add the volume into the renderer
			auto renderer = vtkSmartPointer<vtkRenderer>::New();
			renderer->AddVolume(volume);
			renderer->SetBackground(0.1, 0.01, 0.1);

			// clean previous renderers and then add the current renderer
			auto window = widget.GetRenderWindow();
			auto collection = window->GetRenderers();
			auto item = collection->GetNextItem();
			while (item != NULL)
			{
				window->RemoveRenderer(item);
				item = collection->GetNextItem();
			}
			window->AddRenderer(renderer);
			window->Render();

			// initialize the interactor
			interactor->Initialize();
			interactor->Start();
		}
};

#endif // MAINWINDOW_H
