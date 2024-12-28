#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFont>
#include <QShortcut>
#include <QStyle>
#include <QPixmap>

//
// Opbox alert window, courtesy of ChatGPT.
// Done using custom window instead of QMessageBox so that extra capability can be added in the future
//

int main(int argc, char *argv[]) {
    // Initialize the Qt application
    QApplication app(argc, argv);

    // Create the main window
    QMainWindow window;
    window.setWindowTitle("Opbox Alert");

    // Create a central widget for the main window
    QWidget *centralWidget = new QWidget(&window);

    // Create a vertical layout for the entire window
    QVBoxLayout *mainLayout = new QVBoxLayout();

    // Create a horizontal layout for the icon and the labels
    QHBoxLayout *iconAndLabelsLayout = new QHBoxLayout();

    // Add the default "info" icon to the left
    QLabel *iconLabel = new QLabel(centralWidget);
    QPixmap infoIcon = QApplication::style()->standardIcon(QStyle::SP_MessageBoxInformation).pixmap(64, 64);
    iconLabel->setPixmap(infoIcon);
    iconLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed); // Set minimum size policy for the icon
    iconAndLabelsLayout->addWidget(iconLabel);

    // Create a vertical layout for the labels
    QVBoxLayout *labelsLayout = new QVBoxLayout();
    QWidget *labelsContainer = new QWidget(centralWidget);
    labelsContainer->setLayout(labelsLayout);

    // Add the first label and set it to bold with a larger font size
    QLabel *label1 = new QLabel("Label 1", centralWidget);
    QFont boldFont = label1->font();
    boldFont.setBold(true);
    boldFont.setPointSize(18); // Larger font size for Label 1
    label1->setFont(boldFont);
    labelsLayout->addWidget(label1);

    // Add the second label with a slightly smaller font size
    QLabel *label2 = new QLabel("Label 2", centralWidget);
    QFont normalFont = label2->font();
    normalFont.setPointSize(14); // Standard font size
    label2->setFont(normalFont);
    labelsLayout->addWidget(label2);

    // Align the labels layout to the left and set the container to expand horizontally
    labelsContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    labelsLayout->setAlignment(Qt::AlignLeft);

    // Add the labels container to the horizontal layout
    iconAndLabelsLayout->addWidget(labelsContainer);

    // Add the horizontal layout to the main layout
    mainLayout->addLayout(iconAndLabelsLayout);

    // Add a vertical spacer to push buttons to the bottom
    mainLayout->addStretch();

    // Add a horizontal layout for buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();

    // Create two buttons
    QPushButton *button1 = new QPushButton("Button 1", centralWidget);
    QPushButton *button2 = new QPushButton("Button 2", centralWidget);

    // Set buttons to expand horizontally
    button1->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    button2->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    // Add buttons to the horizontal layout
    buttonLayout->addWidget(button1);
    buttonLayout->addWidget(button2);

    // Add the button layout to the main layout
    mainLayout->addLayout(buttonLayout);

    // Set Button 2 as the default focus
    button2->setFocus();

    // Set the main layout for the central widget
    centralWidget->setLayout(mainLayout);

    // Set the central widget for the main window
    window.setCentralWidget(centralWidget);

    // Add a shortcut for Ctrl+W to close the application
    QShortcut *closeShortcut = new QShortcut(QKeySequence("Ctrl+W"), &window);
    QObject::connect(closeShortcut, &QShortcut::activated, [&]() {
        app.quit(); // Exit the application
    });

    // Show the window
    window.resize(400, 300);
    window.show();

    // Execute the application
    return app.exec();
}

