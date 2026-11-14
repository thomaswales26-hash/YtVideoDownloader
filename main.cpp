#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QProcess>
#include <QProcessEnvironment>
#include <QProgressBar>
#include <QPushButton>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTextEdit>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>

class DownloaderWindow final : public QWidget {
public:
    DownloaderWindow() {
        setWindowTitle("Video Downloader");
        setMinimumSize(520, 390);
        resize(620, 460);

        auto *layout = new QVBoxLayout(this);
        layout->setContentsMargins(28, 24, 28, 24);
        layout->setSpacing(14);

        auto *title = new QLabel("Video Downloader");
        QFont titleFont = title->font();
        titleFont.setPointSize(22);
        titleFont.setBold(true);
        title->setFont(titleFont);
        layout->addWidget(title);

        auto *subtitle = new QLabel(
            "Paste a video URL from a site supported by yt-dlp. Only download content you are authorized to save.");
        subtitle->setWordWrap(true);
        layout->addWidget(subtitle);

        layout->addSpacing(4);
        layout->addWidget(new QLabel("Video URL"));
        urlInput_ = new QLineEdit;
        urlInput_->setPlaceholderText("https://example.com/video");
        urlInput_->setClearButtonEnabled(true);
        layout->addWidget(urlInput_);

        layout->addWidget(new QLabel("Save to"));
        auto *folderRow = new QHBoxLayout;
        folderInput_ = new QLineEdit(defaultDownloadDirectory());
        folderInput_->setReadOnly(true);
        auto *chooseFolder = new QPushButton("Choose…");
        folderRow->addWidget(folderInput_, 1);
        folderRow->addWidget(chooseFolder);
        layout->addLayout(folderRow);
        connect(chooseFolder, &QPushButton::clicked, this, [this] {
            const QString folder = QFileDialog::getExistingDirectory(
                this, "Choose download folder", folderInput_->text());
            if (!folder.isEmpty())
                folderInput_->setText(folder);
        });

        progress_ = new QProgressBar;
        progress_->setRange(0, 100);
        progress_->setValue(0);
        progress_->setFormat("Ready");
        layout->addWidget(progress_);

        auto *actions = new QHBoxLayout;
        actions->addStretch();
        stopButton_ = new QPushButton("Stop");
        stopButton_->setEnabled(false);
        downloadButton_ = new QPushButton("Download");
        downloadButton_->setDefault(true);
        actions->addWidget(stopButton_);
        actions->addWidget(downloadButton_);
        layout->addLayout(actions);

        output_ = new QTextEdit;
        output_->setReadOnly(true);
        output_->setPlaceholderText("Download status will appear here.");
        output_->setMaximumHeight(110);
        layout->addWidget(output_);

        auto *note = new QLabel(
            "Supports sites recognized by yt-dlp; site restrictions and DRM cannot be bypassed.");
        note->setWordWrap(true);
        layout->addWidget(note);

        process_ = new QProcess(this);
        process_->setProcessChannelMode(QProcess::MergedChannels);
        QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
        QStringList pathEntries = {
            QDir(QCoreApplication::applicationDirPath()).filePath("tools"),
            "/opt/homebrew/bin",
            "/usr/local/bin",
            QDir::homePath() + "/.local/bin"
        };
        const QString inheritedPath = environment.value("PATH");
        if (!inheritedPath.isEmpty())
            pathEntries.append(inheritedPath);
        environment.insert("PATH", pathEntries.join(QDir::listSeparator()));
        process_->setProcessEnvironment(environment);
        connect(process_, &QProcess::readyRead, this, [this] { readProcessOutput(); });
        connect(process_, &QProcess::finished, this,
                [this](int exitCode, QProcess::ExitStatus exitStatus) {
                    readProcessOutput();
                    if (!outputBuffer_.trimmed().isEmpty())
                        handleOutputLine(outputBuffer_.trimmed());
                    outputBuffer_.clear();
                    setRunning(false);
                    if (exitStatus == QProcess::NormalExit && exitCode == 0) {
                        progress_->setValue(100);
                        progress_->setFormat("Complete");
                        appendOutput("Download complete.");
                    } else if (stopping_) {
                        progress_->setFormat("Stopped");
                        appendOutput("Download stopped.");
                    } else {
                        progress_->setFormat("Failed");
                        appendOutput("yt-dlp exited with an error. Check the URL and yt-dlp output above.");
                    }
                    stopping_ = false;
                });
        connect(process_, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
            if (error == QProcess::FailedToStart) {
                setRunning(false);
                progress_->setFormat("yt-dlp not found");
                appendOutput("Could not start yt-dlp. Install it with Homebrew (`brew install yt-dlp`) "
                             "and relaunch the app.");
                appendOutput(process_->errorString());
            }
        });

        connect(downloadButton_, &QPushButton::clicked, this, [this] { startDownload(); });
        connect(stopButton_, &QPushButton::clicked, this, [this] {
            if (process_->state() != QProcess::NotRunning) {
                stopping_ = true;
                process_->terminate();
                QTimer::singleShot(2000, process_, [this] {
                    if (process_->state() != QProcess::NotRunning)
                        process_->kill();
                });
            }
        });
        connect(urlInput_, &QLineEdit::returnPressed, this, [this] { startDownload(); });
    }

private:
    static QString defaultDownloadDirectory() {
        QString path = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
        if (path.isEmpty())
            path = QDir::homePath() + "/Downloads";
        path += "/Video Downloads";
        QDir().mkpath(path);
        return path;
    }

    QString ytDlpPath() const {
        const QString bundled = QDir(QCoreApplication::applicationDirPath())
                                    .filePath("tools/yt-dlp");
        if (QFileInfo(bundled).isExecutable())
            return bundled;

        const QString onPath = QStandardPaths::findExecutable("yt-dlp");
        if (!onPath.isEmpty())
            return onPath;
        const QStringList commonPaths = {
            "/opt/homebrew/bin/yt-dlp",
            "/usr/local/bin/yt-dlp",
            QDir::homePath() + "/.local/bin/yt-dlp"
        };
        for (const QString &path : commonPaths) {
            if (QFileInfo(path).isExecutable())
                return path;
        }
        return {};
    }

    QString ffmpegPath() const {
        const QString bundled = QDir(QCoreApplication::applicationDirPath())
                                    .filePath("tools/ffmpeg");
        if (QFileInfo(bundled).isExecutable())
            return bundled;

        const QString onPath = QStandardPaths::findExecutable("ffmpeg");
        if (!onPath.isEmpty())
            return onPath;

        const QStringList commonPaths = {
            "/opt/homebrew/bin/ffmpeg",
            "/usr/local/bin/ffmpeg"
        };
        for (const QString &path : commonPaths) {
            if (QFileInfo(path).isExecutable())
                return path;
        }
        return {};
    }

    void startDownload() {
        if (process_->state() != QProcess::NotRunning)
            return;

        const QUrl url(urlInput_->text().trimmed());
        if (!url.isValid() || (url.scheme() != "https" && url.scheme() != "http")
            || url.host().isEmpty()) {
            progress_->setFormat("Enter a valid HTTP(S) URL");
            urlInput_->setFocus();
            return;
        }
        if (!QDir(folderInput_->text()).exists()) {
            progress_->setFormat("Choose a valid folder");
            return;
        }

        const QString executable = ytDlpPath();
        if (executable.isEmpty()) {
            progress_->setFormat("yt-dlp not found");
            appendOutput("yt-dlp is missing from this app bundle. Rebuild the app with "
                         "`bash scripts/package_macos.sh`.");
            return;
        }
        const QString ffmpeg = ffmpegPath();
        if (ffmpeg.isEmpty()) {
            progress_->setFormat("FFmpeg not found");
            appendOutput("FFmpeg is missing from this app bundle. Rebuild the app with "
                         "`bash scripts/package_macos.sh`.");
            return;
        }

        output_->clear();
        progress_->setValue(0);
        progress_->setFormat("Starting…");
        outputBuffer_.clear();
        stopping_ = false;
        setRunning(true);

        process_->start(executable, {
            "--ffmpeg-location",
            QFileInfo(ffmpeg).absolutePath(),
            "--no-playlist",
            "--newline",
            "--progress-template",
            "download:__YTDLP_PROGRESS__%(progress._percent_str)s|%(progress._speed_str)s|%(progress._eta_str)s",
            "--format",
            "bestvideo[height<=2160][ext=mp4]+bestaudio[ext=m4a]/best[height<=2160][ext=mp4]/bestvideo[height<=2160]+bestaudio/best[height<=2160]/best",
            "--output",
            QDir(folderInput_->text()).filePath("%(title).200B [%(id)s].%(ext)s"),
            url.toString()
        });
    }

    void readProcessOutput() {
        outputBuffer_ += QString::fromLocal8Bit(process_->readAll());
        int newline = -1;
        while ((newline = outputBuffer_.indexOf('\n')) >= 0) {
            const QString line = outputBuffer_.left(newline).trimmed();
            outputBuffer_.remove(0, newline + 1);
            handleOutputLine(line);
        }
    }

    void handleOutputLine(const QString &line) {
        static const QRegularExpression progressPattern(
            R"(^__YTDLP_PROGRESS__\s*([\d.]+)%\s*\|\s*(.*?)\s*\|\s*(.*?)\s*$)");
        const auto match = progressPattern.match(line);
        if (match.hasMatch()) {
            progress_->setValue(qBound(0, qRound(match.captured(1).toDouble()), 100));
            progress_->setFormat(QString("%1%  •  %2  •  ETA %3")
                                     .arg(match.captured(1), match.captured(2), match.captured(3)));
        } else if (!line.isEmpty()) {
            appendOutput(line);
        }
    }

    void appendOutput(const QString &line) {
        output_->append(line.toHtmlEscaped());
    }

    void setRunning(bool running) {
        downloadButton_->setEnabled(!running);
        stopButton_->setEnabled(running);
        urlInput_->setEnabled(!running);
    }

    QLineEdit *urlInput_ = nullptr;
    QLineEdit *folderInput_ = nullptr;
    QProgressBar *progress_ = nullptr;
    QPushButton *downloadButton_ = nullptr;
    QPushButton *stopButton_ = nullptr;
    QTextEdit *output_ = nullptr;
    QProcess *process_ = nullptr;
    QString outputBuffer_;
    bool stopping_ = false;
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QApplication::setStyle("Fusion");
    QApplication::setApplicationName("Video Downloader");
    DownloaderWindow window;
    window.show();
    return app.exec();
}
