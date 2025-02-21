#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QTextEdit>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QThread>
#include <QDebug>

#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/cdrom.h>

#include "../lib/makemkv-backup.h"

#define OUTPUT_BUFFER_SIZE 4096

pid_t start_process(const char *command, char *const argv[], int *pipe_out) {
    int pipefd[2];
    pid_t pid;

    if (pipe(pipefd) == -1) {
        perror("pipe");
        return -1;
    }

    pid = fork();
    if (pid == -1) {
        perror("fork");
        return -1;
    }

    if (pid == 0) {  // Child process
        close(pipefd[0]);
        if (dup2(pipefd[1], STDOUT_FILENO) == -1 || dup2(pipefd[1], STDERR_FILENO) == -1) {
            perror("dup2");
            exit(EXIT_FAILURE);
        }
        close(pipefd[1]);

        // Replace child process with a new program
        execvp(command, argv);
        perror("execvp");
        exit(EXIT_FAILURE);
    }

    // Parent process
    close(pipefd[1]);
    *pipe_out = pipefd[0];

    return pid;
}

void ejectTray(const char *device) {
    int cdrom_fd = open(device, O_RDONLY | O_NONBLOCK);
    if (cdrom_fd < 0) {
        perror("Failed to open device");
        return;
    }

    // Stop the disc
    if (ioctl(cdrom_fd, CDROMSTOP) < 0) {
        perror("Failed to stop the disc");
    }
    sleep(2); // Give it a moment to stop

    // Unlock the tray
    if (ioctl(cdrom_fd, CDROM_LOCKDOOR, 0) < 0) {
        perror("Failed to unlock tray");
    }

    // Send the eject command
    if (ioctl(cdrom_fd, CDROMEJECT) < 0) {
        perror("Eject failed");
        close(cdrom_fd);
        return;
    }

    printf("Tray ejected successfully!\n");

    close(cdrom_fd);
}

// Worker thread for makemkv_backup
class MakemkvBackupThread : public QThread {
    Q_OBJECT

public:
    MakemkvBackupThread(
        const QString &device,
        const QString &path
    ) : m_device(device),
        m_path(path),
        pipe(NULL),
        stopRequested(false)
    {}

    void run() override {
        stopRequested = false;
        int pipeOut;

        const char *c_device = m_device.toLocal8Bit().constData();

        int n_disc = makemkv_get_drive_index(c_device);
        if (n_disc == -1) {
            fprintf(stderr, "Failed to get device %s index number.\n", c_device);
            return;
        }

        QString q_disc = "disc:" + QString::number(n_disc);

        const char *argv[] = {
            "makemkvcon",
            "backup",
            "--decrypt",
            "--cache=1024",
            "--noscan",
            "-r",
            "--progress=-same",
            q_disc.toLocal8Bit().constData(),
            m_path.toLocal8Bit().constData(),
            NULL
        };

        childPid = start_process("makemkvcon", (char *const *)argv, &pipeOut);
        if (childPid == -1) {
            fprintf(stderr, "Failed to start process.\n");
            return;
        }

        FILE *pipe = fdopen(pipeOut, "r");
        if (!pipe) {
            perror("fdopen");
            kill(childPid, SIGKILL);
            return;
        }

        char *output = (char*)malloc(OUTPUT_BUFFER_SIZE);

        char *token;
        char *state;
        makemkv_progress mkv_progress;
        makemkv_progress_title mkv_progress_title;
        makemkv_message mkv_msg;

        while (!stopRequested && fgets(output, OUTPUT_BUFFER_SIZE - 1, pipe)) {
            token = strtok_r(output, ":", &state);

            if (token != NULL) {
                if (strcmp(token, "PRGV") == 0) {
                    makemkv_scan_prgv(mkv_progress, &state);
                    emit progressUpdated(mkv_progress.current, mkv_progress.total);
                    if (mkv_progress.max > 0) {
                        emit maxProgressUpdated(mkv_progress.max);
                    }
                } else if (strcmp(token, "PRGC") == 0) {
                    makemkv_scan_prgc(mkv_progress_title, &state);
                    emit currentTitleUpdated(mkv_progress_title.name);
                } else if (strcmp(token, "PRGT") == 0) {
                    makemkv_scan_prgt(mkv_progress_title, &state);
                    emit totalTitleUpdated(mkv_progress_title.name);
                } else if (strcmp(token, "MSG") == 0) {
                    makemkv_scan_msg(mkv_msg, &state);
                    emit messageUpdated(mkv_msg.message);
                }
            }
        }

        free(output);
        close(pipeOut);
        fclose(pipe);
    }

    void terminateProcess() {
        stopRequested = true;
        if (childPid > 0) {
            kill(childPid, SIGKILL);
            //waitpid(childPid, NULL, 0);
        }
    }

signals:
    void progressUpdated(int current, int total);
    void maxProgressUpdated(int max);
    void currentTitleUpdated(const QString &message);
    void totalTitleUpdated(const QString &message);
    void messageUpdated(const QString &message);
    void errorOccurred(const QString &error);

private:
    QString m_device;
    QString m_path;
    FILE *pipe;
    pid_t childPid;
    bool stopRequested;
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // Command-line argument parsing
    QCommandLineParser parser;
    parser.setApplicationDescription("Qt Program with Device and Path Arguments");
    parser.addHelpOption();

    QCommandLineOption deviceOption("device", "Device argument", "device");
    QCommandLineOption pathOption("path", "Path argument", "path");

    parser.addOption(deviceOption);
    parser.addOption(pathOption);

    parser.process(app);

    QString device = parser.value(deviceOption);
    QString path = parser.value(pathOption);

    if (device.isEmpty() || path.isEmpty()) {
        qWarning("Both --device and --path arguments are required.");
        return 1;
    }

    // Main Window
    QWidget window;
    window.setWindowTitle("MakeMKV Backup");

    // Layout configuration
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(8);

    // First Progress Bar with Label
    QLabel *label1 = new QLabel("Current Progress");
    QProgressBar *progressBar1 = new QProgressBar();
    layout->addWidget(label1);
    layout->addWidget(progressBar1);

    // Second Progress Bar with Label
    QLabel *label2 = new QLabel("Total Progress");
    QProgressBar *progressBar2 = new QProgressBar();
    layout->addWidget(label2);
    layout->addWidget(progressBar2);

    // Read-Only Text Box
    QTextEdit *textBox = new QTextEdit();
    textBox->setReadOnly(true);
    textBox->setText("Device: " + device + "\nPath: " + path);
    layout->addWidget(textBox);

    // Initial window size
    window.resize(700, 450);

    // Set layout and display window
    window.setLayout(layout);
    window.show();

    // Create and start the backup thread
    MakemkvBackupThread *backupThread = new MakemkvBackupThread(device, path);

    // Connect signals to slots for GUI updates
    QObject::connect(backupThread, &MakemkvBackupThread::progressUpdated,
        [progressBar1, progressBar2](int current, int total) {
            QMetaObject::invokeMethod(progressBar1, "setValue", Qt::QueuedConnection, Q_ARG(int, current));
            QMetaObject::invokeMethod(progressBar2, "setValue", Qt::QueuedConnection, Q_ARG(int, total));
        }
    );

    QObject::connect(backupThread, &MakemkvBackupThread::maxProgressUpdated,
        [progressBar1, progressBar2](int max) {
            QMetaObject::invokeMethod(progressBar1, "setMaximum", Qt::QueuedConnection, Q_ARG(int, max));
            QMetaObject::invokeMethod(progressBar2, "setMaximum", Qt::QueuedConnection, Q_ARG(int, max));
        }
    );

    QObject::connect(backupThread, &MakemkvBackupThread::currentTitleUpdated,
        [label1](const QString &text) {
            QMetaObject::invokeMethod(label1, "setText", Qt::QueuedConnection, Q_ARG(QString, text));
        }
    );

    QObject::connect(backupThread, &MakemkvBackupThread::totalTitleUpdated,
        [label2](const QString &text) {
            QMetaObject::invokeMethod(label2, "setText", Qt::QueuedConnection, Q_ARG(QString, text));
        }
    );

    QObject::connect(backupThread, &MakemkvBackupThread::messageUpdated,
        [textBox](const QString &message) {
            QMetaObject::invokeMethod(textBox, "append", Qt::QueuedConnection, Q_ARG(QString, message));
        }
    );

    QObject::connect(backupThread, &MakemkvBackupThread::errorOccurred,
        [](const QString &error) {
            qWarning() << error;
        }
    );

    QObject::connect(&app, &QApplication::aboutToQuit, [&]() {
        backupThread->terminateProcess();
        backupThread->wait();
        delete backupThread;
    });

    QObject::connect(backupThread, &QThread::finished, [&]() {
        ejectTray(device.toLocal8Bit().constData());
        qDebug() << "MakemkvBackupThread finished, exiting application.";
        app.quit();
    });

    backupThread->start();

    return app.exec();
}

#include "main.moc"
