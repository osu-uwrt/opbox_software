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
#include <QTimer>
#include <iostream>
#include <iomanip>
#include "opbox_software/opboxcomms.hpp"

enum ArgumentType
{
    ALERT_ARG_NOTIFICATION_TYPE,
    ALERT_ARG_HEADER,
    ALERT_ARG_BODY,
    ALERT_ARG_BUTTON2_ENABLED,
    ALERT_ARG_BUTTON1_TEXT,
    ALERT_ARG_BUTTON2_TEXT,
    ALERT_ARG_TIMEOUT,
    ALERT_ARG_DEFAULT_EXIT_CODE
};

typedef std::map<ArgumentType, std::string> ArgMap;
typedef std::map<std::string, ArgumentType> InverseArgMap;

const InverseArgMap ARG_FLAGS = {
    { "--notification-type", ALERT_ARG_NOTIFICATION_TYPE },
    { "--header", ALERT_ARG_HEADER },
    { "--body", ALERT_ARG_BODY },
    { "--button2-enabled", ALERT_ARG_BUTTON2_ENABLED },
    { "--button1-text", ALERT_ARG_BUTTON1_TEXT },
    { "--button2-text", ALERT_ARG_BUTTON2_TEXT },
    { "--timeout", ALERT_ARG_TIMEOUT },
    { "--default-exit-code", ALERT_ARG_DEFAULT_EXIT_CODE }
};

const ArgMap ARG_DESCRIPTIONS = {
    { ALERT_ARG_NOTIFICATION_TYPE, "Notification type, can be NOTIFICATION_WARNING, NOTIFICATION_ERROR, or NOTIFICATION_FATAL" },
    { ALERT_ARG_HEADER, "Shown on the large bolded label on the top of the window" },
    { ALERT_ARG_BODY, "Shown on the smaller label underneath the header" },
    { ALERT_ARG_BUTTON2_ENABLED, "If \"true\", button 2 will be enabled. Button will not render otherwise" },
    { ALERT_ARG_BUTTON1_TEXT, "Shown on button 1" },
    { ALERT_ARG_BUTTON2_TEXT, "Shown on button 2" },
    { ALERT_ARG_TIMEOUT, "Number of seconds window should last before closing itself" },
    { ALERT_ARG_DEFAULT_EXIT_CODE, "Exit code returned by window if it closes itself" }
};

const ArgMap DEFAULT_ARGS = {
    { ALERT_ARG_NOTIFICATION_TYPE, "NOTIFICATION_WARNING" },
    { ALERT_ARG_HEADER, "Alert" },
    { ALERT_ARG_BODY, "Alert" },
    { ALERT_ARG_BUTTON2_ENABLED, "false" },
    { ALERT_ARG_BUTTON1_TEXT, "OK" },
    { ALERT_ARG_BUTTON2_TEXT, "Cancel" },
    { ALERT_ARG_TIMEOUT, "600" },
    { ALERT_ARG_DEFAULT_EXIT_CODE, "0" }
};

const std::map<opbox::NotificationType, std::string> FRIENDLY_NOTIFICATION_TYPE_NAMES = {
    { opbox::NotificationType::NOTIFICATION_WARNING, "Warning" },
    { opbox::NotificationType::NOTIFICATION_ERROR, "Error" },
    { opbox::NotificationType::NOTIFICATION_FATAL, "FATAL ERROR" }
};


QStyle::StandardPixmap notificationTypeToIconType(const opbox::NotificationType& notificationType)
{
    switch(notificationType)
    {
        case opbox::NotificationType::NOTIFICATION_WARNING:
            return QStyle::SP_MessageBoxInformation;
        case opbox::NotificationType::NOTIFICATION_ERROR:
            return QStyle::SP_MessageBoxWarning;
        case opbox::NotificationType::NOTIFICATION_FATAL:
            return QStyle::SP_MessageBoxCritical;
        default:
            return QStyle::SP_MessageBoxQuestion;
    }
}

//
// Opbox alert window, courtesy of ChatGPT + some of my own edits
// Done using custom window instead of QMessageBox so that extra capability can be added in the future
//
int alertWindow(int argc, char **argv, const ArgMap& params)
{
     // Initialize the Qt application
    QApplication app(argc, argv);

    // Create the main window
    QMainWindow window;
    window.setWindowTitle(
        QString::fromStdString(
            FRIENDLY_NOTIFICATION_TYPE_NAMES.at(
                opbox::notificationTypeFromString(
                    params.at(ALERT_ARG_NOTIFICATION_TYPE)))));

    // Create a central widget for the main window
    QWidget *centralWidget = new QWidget(&window);

    // Create a vertical layout for the entire window
    QVBoxLayout *mainLayout = new QVBoxLayout();

    // Create a horizontal layout for the icon and the labels
    QHBoxLayout *iconAndLabelsLayout = new QHBoxLayout();

    // Add the default "info" icon to the left
    QLabel *iconLabel = new QLabel(centralWidget);

    QPixmap infoIcon = QApplication::style()->standardIcon(
        notificationTypeToIconType(
            opbox::notificationTypeFromString(
                params.at(ALERT_ARG_NOTIFICATION_TYPE)))).pixmap(64, 64);

    iconLabel->setPixmap(infoIcon);
    iconLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed); // Set minimum size policy for the icon
    iconAndLabelsLayout->addWidget(iconLabel);

    // Create a vertical layout for the labels
    QVBoxLayout *labelsLayout = new QVBoxLayout();
    QWidget *labelsContainer = new QWidget(centralWidget);
    labelsContainer->setLayout(labelsLayout);

    // Add the first label and set it to bold with a larger font size
    QLabel *label1 = new QLabel(QString::fromStdString(params.at(ALERT_ARG_HEADER)), centralWidget);
    QFont boldFont = label1->font();
    boldFont.setBold(true);
    boldFont.setPointSize(18); // Larger font size for Label 1
    label1->setFont(boldFont);
    labelsLayout->addWidget(label1);

    // Add the second label with a slightly smaller font size
    QLabel *label2 = new QLabel(QString::fromStdString(params.at(ALERT_ARG_BODY)), centralWidget);
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

    int retCode = 0;

    // Create two buttons
    QPushButton 
        *button1 = new QPushButton(QString::fromStdString(params.at(ALERT_ARG_BUTTON1_TEXT)), centralWidget),
        *button2 = new QPushButton(QString::fromStdString(params.at(ALERT_ARG_BUTTON2_TEXT)), centralWidget);
    
    QObject::connect(button1, &QPushButton::clicked, [&]() {
        retCode = 0;
        app.quit();
    });

    QObject::connect(button2, &QPushButton::clicked, [&]() {
        retCode = 1;
        app.quit();
    });

    button2->setVisible(params.at(ALERT_ARG_BUTTON2_ENABLED) == "true");
    button1->setText(QString::fromStdString(params.at(ALERT_ARG_BUTTON1_TEXT)));
    button2->setText(QString::fromStdString(params.at(ALERT_ARG_BUTTON2_TEXT)));


    // Set buttons to expand horizontally
    button1->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    button2->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    // Add buttons to the horizontal layout
    buttonLayout->addStretch();
    buttonLayout->addWidget(button2);
    buttonLayout->addWidget(button1);

    // Add the button layout to the main layout
    mainLayout->addLayout(buttonLayout);

    // Set Button 2 as the default focus
    button1->setFocus();

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

    // set timer for auto close
    int
        autoCloseSeconds = std::stoi(params.at(ALERT_ARG_TIMEOUT)),
        autoCloseCode = std::stoi(params.at(ALERT_ARG_DEFAULT_EXIT_CODE));

    QTimer::singleShot(autoCloseSeconds * 1000, [&]() {
        app.quit();
    });

    // Execute the application
    app.exec();

    return retCode;
}


ArgMap readArgs(int argc, char **argv)
{
    ArgMap params(DEFAULT_ARGS);
    for(int i = 0; i < argc; i++)
    {
        std::string arg = argv[i];
        if(ARG_FLAGS.find(arg) != ARG_FLAGS.end()) //if argument is a flag...
        {
            ArgumentType type = ARG_FLAGS.at(arg);
            params[type] = argv[i + 1];
        }

        if(arg == "--help")
        {
            std::cout << "Show an alert window.\n";
            std::cout << " Usage: ./opbox_alert [args]\n\n";

            for(auto it = ARG_FLAGS.begin(); it != ARG_FLAGS.end(); it++)
            {
                std::cout << "  ";

                std::cout
                    << std::setiosflags(std::ios::fixed) 
                    << std::setw(24) 
                    << std::left
                    << it->first;

                std::cout << ARG_DESCRIPTIONS.at(it->second) << "\n";
            }

            std::cout << std::endl;
            exit(0);
        }
    }

    return params;
}


int main(int argc, char *argv[])
{
    ArgMap params = readArgs(argc, argv);
    return alertWindow(argc, argv, params);
}

