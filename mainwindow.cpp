#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

	// add VTK widgets
	ui->verticalLayout->addWidget(&widget);
	ui->verticalLayout_2->addWidget(&volumePropertywidget);

	// set up interactor
	interactor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
	interactor->SetRenderWindow(widget.GetRenderWindow());

	// allow the user to interactively manipulate (rotate, pan, etc.) the camera, the viewpoint of the scene.
	auto style = vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New();
	interactor->SetInteractorStyle(style);

	// set default data set
	filename = "../../data/nucleon.mhd";

	// connect signals to slots
	connect(ui->action_Open, SIGNAL(triggered()), this, SLOT(onOpenSlot()));
	connect(ui->action_About, SIGNAL(triggered()), this, SLOT(onAboutSlot()));
	connect(ui->action_Exit, SIGNAL(triggered()), this, SLOT(onExitSlot()));
}

MainWindow::~MainWindow()
{
    delete ui;
}
