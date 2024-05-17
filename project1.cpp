#include <QApplication>
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QTextEdit>
#include <QProcess>
#include <array>
#include <memory>
#include <sys/statvfs.h>
#include <unistd.h>
#include <sys/wait.h>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <iomanip>

std::string exec(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
    if (!pipe) return "";
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

void getDiskSize(QLabel* label) {
    std::string output = exec("lsblk -b -n -o SIZE /dev/sda");
    unsigned long long diskSize = std::stoull(output);
    label->setText(QString("Total capacity of the hard disk: %1 MB").arg(static_cast<double>(diskSize) / (1024 * 1024), 0, 'f', 2));
}

void getDiskUsage(QLabel* label) {
    struct statvfs stat;
    if (statvfs("/", &stat) != 0) {
        perror("statvfs");
        return;
    }
    unsigned long long totalBytes = stat.f_blocks * stat.f_frsize;
    unsigned long long freeBytes = stat.f_bfree * stat.f_frsize;
    unsigned long long usedBytes = totalBytes - freeBytes;
    label->setText(QString("Free space: %1 MB\nUsed space: %2 MB").arg(static_cast<double>(freeBytes) / (1024 * 1024), 0, 'f', 2).arg(static_cast<double>(usedBytes) / (1024 * 1024), 0, 'f', 2));
}

void getMountPoints(QLabel* label) {
    std::string output = exec("lsblk -o MOUNTPOINT");
    std::istringstream iss(output);
    std::string line;
    QString mountPoints = "Mount Points:\n";
    while (std::getline(iss, line)) {
        if (!line.empty()) {
            mountPoints += QString::fromStdString(line) + '\n';
        }
    }
    label->setText(mountPoints);
}

void displayFolderHierarchy(QTextEdit* textEdit) {
    std::string command = "du -h / | sort -hr";
    std::string output = exec(command.c_str());
    std::istringstream iss(output);
    std::string line;
    QString folderHierarchy = "Folder Hierarchy and Sizes:\n";
    while (std::getline(iss, line)) {
        if (!line.empty()) {
            folderHierarchy += QString::fromStdString(line) + '\n';
        }
    }

    std::string readCommand = "find / -type f -exec stat --format='%s' {} + | awk '{sum+=$1} END {print sum}'";
    std::string readOutput = exec(readCommand.c_str());
    unsigned long long totalReadBytes = std::stoull(readOutput);
    double readRateMBps = static_cast<double>(totalReadBytes) / (1024 * 1024);
    folderHierarchy += QString("Total Read Rate: %1 MB/s").arg(readRateMBps, 0, 'f', 2);

    textEdit->setText(folderHierarchy);
}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    QWidget window;
    window.setWindowTitle("Storage Analytics");

    QVBoxLayout *layout = new QVBoxLayout;

    QLabel *labelDiskSize = new QLabel("Disk Size:");
    layout->addWidget(labelDiskSize);

    QLabel *labelDiskUsage = new QLabel("Disk Usage:");
    layout->addWidget(labelDiskUsage);

    QLabel *labelMountPoints = new QLabel("Mount Points:");
    layout->addWidget(labelMountPoints);

    QTextEdit *textFolderHierarchy = new QTextEdit();
    textFolderHierarchy->setReadOnly(true);
    layout->addWidget(textFolderHierarchy);

    QPushButton *buttonRun = new QPushButton("Run Analysis");
    layout->addWidget(buttonRun);

    QObject::connect(buttonRun, &QPushButton::clicked, [&]() {
        pid_t pid1, pid2, pid3, pid4;

        if ((pid1 = fork()) == 0) {
            getDiskSize(labelDiskSize);
            exit(EXIT_SUCCESS);
        } else if ((pid2 = fork()) == 0) {
            getDiskUsage(labelDiskUsage);
            exit(EXIT_SUCCESS);
        } else if ((pid3 = fork()) == 0) {
            getMountPoints(labelMountPoints);
            exit(EXIT_SUCCESS);
        } else if ((pid4 = fork()) == 0) {
            displayFolderHierarchy(textFolderHierarchy);
            exit(EXIT_SUCCESS);
        }

        waitpid(pid1, NULL, 0);
        waitpid(pid2, NULL, 0);
        waitpid(pid3, NULL, 0);
        waitpid(pid4, NULL, 0);
    });

    window.setLayout(layout);
    window.show();

    return app.exec();
}

