#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QDialog>
#include <QDir>
#include <QFileDialog>
#include <QFile>
#include <QFileInfo>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QProcess>
#include <QProgressBar>
#include <QPushButton>
#include <QRegularExpression>
#include <QSettings>
#include <QScrollArea>
#include <QSpinBox>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTextEdit>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include <memory>

class VmStudioWindow final : public QWidget {
public:
    VmStudioWindow() {
        setWindowTitle("VM Studio");
        setMinimumSize(680, 760);
        resize(800, 900);

        auto *layout = new QVBoxLayout(this);
        layout->setContentsMargins(32, 28, 32, 28);
        layout->setSpacing(18);

        auto *heading = new QHBoxLayout;
        auto *title = new QLabel("VM Studio");
        QFont titleFont = title->font();
        titleFont.setPointSize(25);
        titleFont.setBold(true);
        title->setFont(titleFont);
        auto *badge = new QLabel("LINUX  ·  WINDOWS");
        badge->setObjectName("badge");
        badge->setAlignment(Qt::AlignCenter);
        heading->addWidget(title);
        heading->addStretch();
        heading->addWidget(badge);
        layout->addLayout(heading);

        auto *subtitle = new QLabel(
            "Create and launch a virtual machine on your Mac. Choose an operating system installer to get started.");
        subtitle->setObjectName("subtitle");
        subtitle->setWordWrap(true);
        layout->addWidget(subtitle);

        auto *settingsScroll = new QScrollArea;
        settingsScroll->setObjectName("settingsScroll");
        settingsScroll->setWidgetResizable(true);
        settingsScroll->setFrameShape(QFrame::NoFrame);
        auto *settingsContent = new QWidget;
        auto *settingsLayout = new QVBoxLayout(settingsContent);
        settingsLayout->setContentsMargins(0, 0, 0, 0);
        settingsLayout->setSpacing(14);

        auto *card = new QFrame;
        card->setObjectName("card");
        auto *form = new QFormLayout(card);
        form->setContentsMargins(20, 20, 20, 20);
        form->setHorizontalSpacing(16);
        form->setVerticalSpacing(15);
        form->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);

        nameInput_ = new QLineEdit("My Linux VM");
        nameInput_->setPlaceholderText("e.g. Ubuntu");
        form->addRow("Machine name", nameInput_);

        osInput_ = new QComboBox;
        osInput_->addItem("Linux", "linux");
        osInput_->addItem("Windows", "windows");
        form->addRow("Operating system", osInput_);

        architectureInput_ = new QComboBox;
        architectureInput_->addItem("ARM64", "aarch64");
        architectureInput_->addItem("x86_64", "x86_64");
        architectureInput_->setCurrentIndex(
            architectureInput_->findData(hostArchitecture()));
        form->addRow("Guest architecture", architectureInput_);

        auto *isoRow = new QHBoxLayout;
        isoInput_ = new QLineEdit;
        isoInput_->setPlaceholderText("Select a Linux or Windows .iso installer");
        isoInput_->setReadOnly(true);
        auto *chooseIso = new QPushButton("Browse…");
        isoRow->addWidget(isoInput_, 1);
        isoRow->addWidget(chooseIso);
        form->addRow("Installer image", isoRow);

        bootInstallerInput_ = new QCheckBox("Boot from installer ISO");
        bootInstallerInput_->setChecked(true);
        form->addRow("", bootInstallerInput_);

        tpmInput_ = new QCheckBox("Enable TPM 2.0 (Windows 11)");
        tpmInput_->setChecked(true);
        form->addRow("", tpmInput_);

        secureBootInput_ = new QCheckBox("Enable Secure Boot (Windows 11)");
        secureBootInput_->setChecked(true);
        form->addRow("", secureBootInput_);

        auto *secureBootNote = new QLabel(
            "Secure Boot firmware with Microsoft 2023 keys is bundled for both architectures. "
            "The firmware fields below are optional overrides.");
        secureBootNote->setWordWrap(true);
        secureBootNote->setObjectName("note");
        form->addRow("", secureBootNote);

        auto *secureCodeRow = new QHBoxLayout;
        secureCodeInput_ = new QLineEdit;
        secureCodeInput_->setPlaceholderText("Bundled firmware (or select an override)");
        secureCodeInput_->setReadOnly(true);
        auto *chooseSecureCode = new QPushButton("Browse…");
        secureCodeRow->addWidget(secureCodeInput_, 1);
        secureCodeRow->addWidget(chooseSecureCode);
        form->addRow("Secure Boot code", secureCodeRow);

        auto *secureVarsRow = new QHBoxLayout;
        secureVarsInput_ = new QLineEdit;
        secureVarsInput_->setPlaceholderText("Bundled Microsoft 2023 keys (or select an override)");
        secureVarsInput_->setReadOnly(true);
        auto *chooseSecureVars = new QPushButton("Browse…");
        secureVarsRow->addWidget(secureVarsInput_, 1);
        secureVarsRow->addWidget(chooseSecureVars);
        form->addRow("UEFI variables", secureVarsRow);

        auto *diskRow = new QHBoxLayout;
        diskInput_ = new QLineEdit(defaultDiskPath());
        auto *chooseDisk = new QPushButton("Select…");
        auto *createDisk = new QPushButton("New…");
        diskRow->addWidget(diskInput_, 1);
        diskRow->addWidget(chooseDisk);
        diskRow->addWidget(createDisk);
        form->addRow("Virtual disk", diskRow);

        diskSizeInput_ = new QSpinBox;
        diskSizeInput_->setRange(8, 1024);
        diskSizeInput_->setValue(64);
        diskSizeInput_->setSuffix(" GiB");

        memoryInput_ = new QSpinBox;
        memoryInput_->setRange(1, 128);
        memoryInput_->setValue(4);
        memoryInput_->setSuffix(" GiB");
        coresInput_ = new QSpinBox;
        coresInput_->setRange(1, 32);
        coresInput_->setValue(4);
        coresInput_->setSuffix(" cores");
        settingsLayout->addWidget(card);

        auto *note = new QLabel(architectureNote());
        note->setObjectName("note");
        note->setWordWrap(true);
        settingsLayout->addWidget(note);
        settingsLayout->addStretch();
        settingsScroll->setWidget(settingsContent);
        layout->addWidget(settingsScroll, 1);

        auto *resourcesCard = new QFrame;
        resourcesCard->setObjectName("card");
        auto *resourcesForm = new QFormLayout(resourcesCard);
        resourcesForm->setContentsMargins(18, 12, 18, 12);
        resourcesForm->setHorizontalSpacing(16);
        resourcesForm->setVerticalSpacing(8);
        resourcesForm->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        resourcesForm->addRow("Disk capacity (maximum)", diskSizeInput_);
        resourcesForm->addRow("Memory (while VM runs)", memoryInput_);
        resourcesForm->addRow("CPU cores", coresInput_);
        layout->addWidget(resourcesCard);

        status_ = new QLabel("Ready to create a virtual machine");
        status_->setObjectName("status");
        layout->addWidget(status_);

        auto *actions = new QHBoxLayout;
        actions->addStretch();
        stopButton_ = new QPushButton("Stop VM");
        stopButton_->setEnabled(false);
        startButton_ = new QPushButton("Create & Start");
        startButton_->setObjectName("primary");
        actions->addWidget(stopButton_);
        actions->addWidget(startButton_);
        layout->addLayout(actions);

        output_ = new QTextEdit;
        output_->setReadOnly(true);
        output_->setPlaceholderText("Virtual machine startup messages will appear here.");
        output_->setMinimumHeight(65);
        output_->setMaximumHeight(95);
        layout->addWidget(output_);

        loadSettings();
        updateWindowsOptions();
        connect(nameInput_, &QLineEdit::textChanged, this, [this](const QString &name) {
            if (!diskPathTouched_)
                diskInput_->setText(defaultDiskPath(name));
            saveSettings();
        });
        connect(osInput_, &QComboBox::currentIndexChanged, this, [this](int index) {
            nameInput_->setText(index == 0 ? "My Linux VM" : "My Windows VM");
            updateWindowsOptions();
            saveSettings();
        });
        connect(architectureInput_, &QComboBox::currentIndexChanged,
                this, [this] { saveSettings(); });
        connect(memoryInput_, &QSpinBox::valueChanged, this, [this] { saveSettings(); });
        connect(coresInput_, &QSpinBox::valueChanged, this, [this] { saveSettings(); });
        connect(diskSizeInput_, &QSpinBox::valueChanged, this, [this] { saveSettings(); });
        connect(isoInput_, &QLineEdit::textChanged, this, [this] { saveSettings(); });
        connect(bootInstallerInput_, &QCheckBox::toggled, this, [this] { saveSettings(); });
        connect(tpmInput_, &QCheckBox::toggled, this, [this] { saveSettings(); });
        connect(secureBootInput_, &QCheckBox::toggled, this, [this] {
            updateWindowsOptions();
            saveSettings();
        });
        connect(secureCodeInput_, &QLineEdit::textChanged, this, [this] { saveSettings(); });
        connect(secureVarsInput_, &QLineEdit::textChanged, this, [this] { saveSettings(); });
        connect(diskInput_, &QLineEdit::textEdited, this, [this] {
            diskPathTouched_ = true;
            saveSettings();
        });
        connect(chooseIso, &QPushButton::clicked, this, [this] {
            QFileDialog dialog(this, "Choose operating system installer", QDir::homePath());
            dialog.setOption(QFileDialog::DontUseNativeDialog);
            dialog.setFileMode(QFileDialog::ExistingFile);
            dialog.setAcceptMode(QFileDialog::AcceptOpen);
            dialog.setNameFilters({"ISO images (*.iso)", "All files (*)"});
            if (dialog.exec() == QDialog::Accepted && !dialog.selectedFiles().isEmpty())
                isoInput_->setText(QFileInfo(dialog.selectedFiles().first()).absoluteFilePath());
        });
        connect(chooseSecureCode, &QPushButton::clicked, this, [this] {
            QFileDialog dialog(this, "Choose Secure Boot-capable firmware code", QDir::homePath());
            dialog.setOption(QFileDialog::DontUseNativeDialog);
            dialog.setFileMode(QFileDialog::ExistingFile);
            dialog.setNameFilter("UEFI firmware (*.fd *.bin);;All files (*)");
            if (dialog.exec() == QDialog::Accepted && !dialog.selectedFiles().isEmpty())
                secureCodeInput_->setText(
                    QFileInfo(dialog.selectedFiles().first()).absoluteFilePath());
        });
        connect(chooseSecureVars, &QPushButton::clicked, this, [this] {
            QFileDialog dialog(this, "Choose matching UEFI variables template", QDir::homePath());
            dialog.setOption(QFileDialog::DontUseNativeDialog);
            dialog.setFileMode(QFileDialog::ExistingFile);
            dialog.setNameFilter("UEFI firmware (*.fd *.bin);;All files (*)");
            if (dialog.exec() == QDialog::Accepted && !dialog.selectedFiles().isEmpty())
                secureVarsInput_->setText(
                    QFileInfo(dialog.selectedFiles().first()).absoluteFilePath());
        });
        connect(chooseDisk, &QPushButton::clicked, this, [this] {
            QFileDialog dialog(this, "Select existing virtual disk",
                               existingDirectoryForPath(diskInput_->text()));
            dialog.setOption(QFileDialog::DontUseNativeDialog);
            dialog.setFileMode(QFileDialog::ExistingFile);
            dialog.setAcceptMode(QFileDialog::AcceptOpen);
            dialog.setNameFilter("QEMU virtual disks (*.qcow2)");
            if (dialog.exec() == QDialog::Accepted && !dialog.selectedFiles().isEmpty())
                setDiskPath(QFileInfo(dialog.selectedFiles().first()).absoluteFilePath());
        });
        connect(createDisk, &QPushButton::clicked, this, [this] {
            QFileDialog dialog(this, "Create a new virtual disk",
                               existingDirectoryForPath(diskInput_->text()));
            dialog.setOption(QFileDialog::DontUseNativeDialog);
            dialog.setFileMode(QFileDialog::AnyFile);
            dialog.setAcceptMode(QFileDialog::AcceptSave);
            dialog.setNameFilter("QEMU virtual disks (*.qcow2)");
            dialog.setDefaultSuffix("qcow2");
            dialog.selectFile(QFileInfo(diskInput_->text()).fileName());
            if (dialog.exec() != QDialog::Accepted || dialog.selectedFiles().isEmpty())
                return;
            const QString path = dialog.selectedFiles().first();
            if (QFileInfo::exists(path)) {
                QMessageBox::warning(this, "Disk already exists",
                                     "Choose Select… to use an existing virtual disk, or choose "
                                     "a different name to create a new disk.");
                return;
            }
            setDiskPath(QFileInfo(path).absoluteFilePath());
        });
        connect(startButton_, &QPushButton::clicked, this, [this] { createOrStartVm(); });
        connect(stopButton_, &QPushButton::clicked, this, [this] {
            if (vmProcess_->state() != QProcess::NotRunning) {
                status_->setText("Stopping virtual machine…");
                vmProcess_->terminate();
            }
        });

        diskProcess_ = new QProcess(this);
        diskProcess_->setProcessChannelMode(QProcess::MergedChannels);
        connect(diskProcess_, &QProcess::readyRead, this, [this] {
            appendOutput(QString::fromLocal8Bit(diskProcess_->readAll()).trimmed());
        });
        connect(diskProcess_, &QProcess::finished, this,
                [this](int exitCode, QProcess::ExitStatus exitStatus) {
                    if (exitStatus != QProcess::NormalExit || exitCode != 0) {
                        setBusy(false);
                        status_->setText("Could not create virtual disk");
                        appendOutput("qemu-img failed to create the disk image.");
                        return;
                    }
                    launchVm();
                });
        connect(diskProcess_, &QProcess::errorOccurred, this,
                [this](QProcess::ProcessError error) {
                    if (error == QProcess::FailedToStart) {
                        setBusy(false);
                        status_->setText("QEMU tools not found");
                        appendOutput("Could not start qemu-img. Install QEMU with `brew install qemu`.");
                    }
                });

        vmProcess_ = new QProcess(this);
        vmProcess_->setProcessChannelMode(QProcess::MergedChannels);
        connect(vmProcess_, &QProcess::readyRead, this, [this] {
            appendOutput(QString::fromLocal8Bit(vmProcess_->readAll()).trimmed());
        });
        connect(vmProcess_, &QProcess::started, this, [this] {
            stopButton_->setEnabled(true);
            status_->setText("Virtual machine is running — use the QEMU window to interact");
            appendOutput("VM started. Close the QEMU window or select Stop VM to shut it down.");
        });
        connect(vmProcess_, &QProcess::finished, this,
                [this](int exitCode, QProcess::ExitStatus exitStatus) {
                    setBusy(false);
                    stopTpm();
                    if (exitStatus == QProcess::NormalExit && exitCode == 0)
                        status_->setText("Virtual machine stopped");
                    else
                        status_->setText("Virtual machine exited");
                });
        connect(vmProcess_, &QProcess::errorOccurred, this,
                [this](QProcess::ProcessError error) {
                    if (error == QProcess::FailedToStart) {
                        setBusy(false);
                        status_->setText("QEMU not found");
                        appendOutput("Could not start QEMU. Install it with `brew install qemu`.");
                        stopTpm();
                    }
                });

        tpmProcess_ = new QProcess(this);
        tpmProcess_->setProcessChannelMode(QProcess::MergedChannels);
        connect(tpmProcess_, &QProcess::readyRead, this, [this] {
            appendOutput(QString::fromLocal8Bit(tpmProcess_->readAll()).trimmed());
        });
        connect(tpmProcess_, &QProcess::started, this, [this] { waitForTpmSocket(0); });
        connect(tpmProcess_, &QProcess::errorOccurred, this,
                [this](QProcess::ProcessError error) {
                    if (error == QProcess::FailedToStart) {
                        setBusy(false);
                        status_->setText("TPM emulator not found");
                        appendOutput("Could not start swtpm. Install it with `brew install swtpm`.");
                        tpmSocketDirectory_.reset();
                    }
                });
        connect(tpmProcess_, &QProcess::finished, this,
                [this](int, QProcess::ExitStatus) {
                    if (vmProcess_->state() == QProcess::NotRunning)
                        tpmSocketDirectory_.reset();
                });

        setStyleSheet(R"(
            QWidget { background: #f5f7fb; color: #172033; font-size: 13px; }
            QLabel#subtitle, QLabel#note { color: #647087; }
            QLabel#badge { color: #3156b4; background: #e8edff; border-radius: 9px;
                           padding: 7px 11px; font-size: 11px; font-weight: 700; }
            QFrame#card { background: white; border: 1px solid #e3e8f0; border-radius: 12px; }
            QLineEdit, QComboBox, QSpinBox, QTextEdit {
                background: white; border: 1px solid #d6dce7; border-radius: 7px;
                padding: 8px; selection-background-color: #4267d5;
            }
            QTextEdit { padding: 10px; }
            QPushButton { background: white; border: 1px solid #d6dce7; border-radius: 7px;
                          padding: 9px 14px; }
            QPushButton:hover { background: #f0f3f9; }
            QPushButton:disabled { color: #a0a8b7; }
            QPushButton#primary { color: white; background: #365fd1; border-color: #365fd1;
                                  font-weight: 700; }
            QPushButton#primary:hover { background: #2e52b9; }
            QLabel#status { font-weight: 600; }
        )");
    }

private:
    static QString hostArchitecture() {
#if defined(Q_PROCESSOR_ARM)
        return "aarch64";
#else
        return "x86_64";
#endif
    }

    static QString architectureNote() {
        const QString hostLabel = hostArchitecture() == "aarch64" ? "ARM64" : "x86_64";
        const QString guestLabel = hostArchitecture() == "aarch64" ? "x86_64" : "ARM64";
        return QString("The ISO must match the selected architecture. %1 guests use Apple's "
                       "Hypervisor framework; %2 guests are software-emulated and can be slower.")
            .arg(hostLabel, guestLabel);
    }

    static QString defaultDiskPath(const QString &name = "My Linux VM") {
        QString safeName = name.trimmed();
        safeName.replace(QRegularExpression(R"([^\w.-]+)"), "-");
        if (safeName.isEmpty())
            safeName = "Virtual-Machine";
        QString directory = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
        if (directory.isEmpty())
            directory = QDir::homePath();
        return QDir(directory).filePath("Virtual Machines/" + safeName + ".qcow2");
    }

    static QString existingDirectoryForPath(const QString &path) {
        const QString directory = QFileInfo(path).absolutePath();
        return QDir(directory).exists() ? directory : QDir::homePath();
    }

    void setDiskPath(const QString &path) {
        diskPathTouched_ = true;
        diskInput_->setText(path);
        saveSettings();
    }

    void loadSettings() {
        QSettings settings;
        nameInput_->setText(settings.value("machine/name", nameInput_->text()).toString());
        osInput_->setCurrentIndex(settings.value("machine/os", 0).toInt());
        const QString architecture = settings.value(
            "machine/architecture", architectureInput_->currentData().toString()).toString();
        architectureInput_->setCurrentIndex(architectureInput_->findData(architecture));
        isoInput_->setText(settings.value("machine/iso").toString());
        bootInstallerInput_->setChecked(settings.value("machine/bootInstaller", true).toBool());
        diskInput_->setText(settings.value("machine/disk", diskInput_->text()).toString());
        diskPathTouched_ = settings.value("machine/diskTouched", false).toBool();
        diskSizeInput_->setValue(settings.value("machine/diskSize", 64).toInt());
        memoryInput_->setValue(settings.value("machine/memory", 4).toInt());
        coresInput_->setValue(settings.value("machine/cores", 4).toInt());
        tpmInput_->setChecked(settings.value("machine/tpm", true).toBool());
        secureBootInput_->setChecked(settings.value("machine/secureBoot", true).toBool());
        secureCodeInput_->setText(settings.value("machine/secureCode").toString());
        secureVarsInput_->setText(settings.value("machine/secureVars").toString());
    }

    void saveSettings() const {
        QSettings settings;
        settings.setValue("machine/name", nameInput_->text());
        settings.setValue("machine/os", osInput_->currentIndex());
        settings.setValue("machine/architecture", architectureInput_->currentData());
        settings.setValue("machine/iso", isoInput_->text());
        settings.setValue("machine/bootInstaller", bootInstallerInput_->isChecked());
        settings.setValue("machine/disk", diskInput_->text());
        settings.setValue("machine/diskTouched", diskPathTouched_);
        settings.setValue("machine/diskSize", diskSizeInput_->value());
        settings.setValue("machine/memory", memoryInput_->value());
        settings.setValue("machine/cores", coresInput_->value());
        settings.setValue("machine/tpm", tpmInput_->isChecked());
        settings.setValue("machine/secureBoot", secureBootInput_->isChecked());
        settings.setValue("machine/secureCode", secureCodeInput_->text());
        settings.setValue("machine/secureVars", secureVarsInput_->text());
    }

    void updateWindowsOptions() {
        const bool windows = osInput_->currentData().toString() == "windows";
        tpmInput_->setEnabled(windows);
        secureBootInput_->setEnabled(windows);
        secureCodeInput_->setEnabled(windows && secureBootInput_->isChecked());
        secureVarsInput_->setEnabled(windows && secureBootInput_->isChecked());
    }

    QString findTool(const QString &tool) const {
        QString path = QStandardPaths::findExecutable(tool);
        if (!path.isEmpty())
            return path;
        const QStringList directories = {"/opt/homebrew/bin", "/usr/local/bin"};
        for (const QString &directory : directories) {
            const QString candidate = QDir(directory).filePath(tool);
            if (QFileInfo(candidate).isExecutable())
                return candidate;
        }
        return {};
    }

    void createOrStartVm() {
        if (vmProcess_->state() != QProcess::NotRunning
            || diskProcess_->state() != QProcess::NotRunning)
            return;

        const QFileInfo iso(isoInput_->text());
        if (bootInstallerInput_->isChecked()
            && (!iso.exists() || !iso.isFile()
                || iso.suffix().compare("iso", Qt::CaseInsensitive) != 0)) {
            QMessageBox::warning(this, "Choose an installer", "Select an existing .iso installer image.");
            return;
        }
        if (nameInput_->text().trimmed().isEmpty()) {
            QMessageBox::warning(this, "Name required", "Enter a name for this virtual machine.");
            nameInput_->setFocus();
            return;
        }
        const QFileInfo disk(diskInput_->text());
        const QString diskDirectory = disk.absolutePath();
        if (diskInput_->text().trimmed().isEmpty() || !QDir().mkpath(diskDirectory)) {
            QMessageBox::warning(this, "Invalid disk location",
                                 "Choose a writable location for the virtual disk.");
            return;
        }
        if (disk.exists() && !disk.isFile()) {
            QMessageBox::warning(this, "Invalid disk location",
                                 "The virtual disk path must be a file, not a directory.");
            return;
        }
        if (bootInstallerInput_->isChecked()
            && QDir::cleanPath(disk.absoluteFilePath())
                == QDir::cleanPath(iso.absoluteFilePath())) {
            QMessageBox::warning(this, "Invalid disk location",
                                 "The virtual disk cannot overwrite the installer ISO.");
            return;
        }

        const QString architecture = architectureInput_->currentData().toString();
        const bool windows = osInput_->currentData().toString() == "windows";
        const bool useTpm = windows && tpmInput_->isChecked();
        const bool useSecureBoot = windows && secureBootInput_->isChecked();
        const QString qemu = findTool(architecture == "aarch64"
                                          ? "qemu-system-aarch64"
                                          : "qemu-system-x86_64");
        if (qemu.isEmpty()) {
            QMessageBox::critical(this, "QEMU not found",
                                  "Install QEMU with Homebrew (`brew install qemu`) and try again.");
            return;
        }
        if (useTpm && findTool("swtpm").isEmpty()) {
            QMessageBox::critical(this, "TPM emulator not found",
                                  "Install swtpm with Homebrew (`brew install swtpm`) to enable TPM 2.0.");
            return;
        }
        if (useSecureBoot) {
            const QString selectedCode = secureCodeInput_->text().trimmed();
            const QString selectedVars = secureVarsInput_->text().trimmed();
            if (selectedCode.isEmpty() != selectedVars.isEmpty()) {
                QMessageBox::critical(
                    this, "Select both firmware files",
                    "Secure Boot needs both a Secure-Boot-capable UEFI code file and its matching "
                    "variables template.");
                return;
            }
            const auto paths = selectedCode.isEmpty()
                ? secureBootFirmware(architecture)
                : QPair<QString, QString>{selectedCode, selectedVars};
            if (paths.first.isEmpty()) {
                QMessageBox::critical(
                    this, "Secure Boot firmware not found",
                    "The bundled Secure Boot firmware is missing. Reinstall the app or select matching "
                    "Secure-Boot-capable firmware code and a Microsoft-key-enrolled variables "
                    "template for the selected architecture.");
                return;
            }
            const QFileInfo codeInfo(paths.first);
            const QFileInfo varsInfo(paths.second);
            if (!codeInfo.isReadable() || !varsInfo.isReadable()
                || codeInfo.size() == 0 || varsInfo.size() == 0) {
                QMessageBox::critical(
                    this, "Invalid Secure Boot firmware",
                    "The selected firmware code and variable template must both exist, be readable, "
                    "and be non-empty.");
                return;
            }
            secureCodePath_ = paths.first;
            secureVarsTemplatePath_ = paths.second;
        } else {
            secureCodePath_.clear();
            secureVarsTemplatePath_.clear();
        }
        useTpm_ = useTpm;
        useSecureBoot_ = useSecureBoot;
        qemuExecutable_ = qemu;
        isoPath_ = bootInstallerInput_->isChecked() ? iso.absoluteFilePath() : QString();
        diskPath_ = disk.absoluteFilePath();
        output_->clear();
        if (disk.exists()) {
            launchVm();
            return;
        }

        const QString qemuImg = findTool("qemu-img");
        if (qemuImg.isEmpty()) {
            QMessageBox::critical(this, "QEMU tools not found",
                                  "Could not find qemu-img. Install QEMU with `brew install qemu`.");
            return;
        }
        setBusy(true);
        status_->setText("Creating virtual disk…");
        appendOutput(QString("Creating %1 GiB disk image: %2")
                         .arg(diskSizeInput_->value())
                         .arg(diskPath_));
        diskProcess_->start(qemuImg, {"create", "-f", "qcow2", diskPath_,
                                      QString("%1G").arg(diskSizeInput_->value())});
    }

    QString firmwarePath(const QString &architecture) const {
        const QStringList filenames = architecture == "aarch64"
            ? QStringList{"edk2-aarch64-code.fd", "QEMU_EFI.fd"}
            : QStringList{"edk2-x86_64-code.fd", "OVMF_CODE.fd", "edk2-x86_64-code.secboot.fd"};
        const QStringList directories = {
            "/opt/homebrew/share/qemu",
            "/usr/local/share/qemu",
            "/opt/homebrew/share",
            "/usr/local/share"
        };
        for (const QString &directory : directories) {
            for (const QString &filename : filenames) {
                const QString path = QDir(directory).filePath(filename);
                if (QFileInfo(path).isReadable())
                    return path;
            }
        }
        return {};
    }

    QPair<QString, QString> secureBootFirmware(const QString &architecture) const {
        const QString resources = QDir(QCoreApplication::applicationDirPath())
                                      .filePath("../Resources/firmware");
        const QList<QPair<QString, QString>> bundledPairs = architecture == "aarch64"
            ? QList<QPair<QString, QString>>{
                  {QDir(resources).filePath("aarch64/AAVMF_CODE.secboot.fd"),
                   QDir(resources).filePath("aarch64/AAVMF_VARS.ms.fd")}}
            : QList<QPair<QString, QString>>{
                  {QDir(resources).filePath("x86_64/OVMF_CODE.secboot.fd"),
                   QDir(resources).filePath("x86_64/OVMF_VARS.ms.fd")}};
        for (const auto &firmwarePair : bundledPairs) {
            if (QFileInfo(firmwarePair.first).isReadable()
                && QFileInfo(firmwarePair.second).isReadable())
                return firmwarePair;
        }

        const QStringList directories = {
            "/opt/homebrew/share/qemu",
            "/usr/local/share/qemu",
            "/opt/homebrew/share",
            "/usr/local/share"
        };
        const QList<QPair<QString, QString>> firmwarePairs = architecture == "aarch64"
            ? QList<QPair<QString, QString>>{
                  {"AAVMF_CODE.secboot.fd", "AAVMF_VARS.ms.fd"},
                  {"AAVMF_CODE.ms.fd", "AAVMF_VARS.ms.fd"},
                  {"edk2-aarch64-secure-code.fd", "edk2-aarch64-secure-vars.fd"}}
            : QList<QPair<QString, QString>>{
                  {"OVMF_CODE_4M.secboot.fd", "OVMF_VARS_4M.ms.fd"},
                  {"OVMF_CODE.secboot.fd", "OVMF_VARS.ms.fd"},
                  {"edk2-x86_64-secure-code.fd", "edk2-x86_64-secure-vars.fd"}};
        for (const QString &directory : directories) {
            for (const auto &firmwarePair : firmwarePairs) {
                const QString codePath = QDir(directory).filePath(firmwarePair.first);
                const QString varsPath = QDir(directory).filePath(firmwarePair.second);
                if (QFileInfo(codePath).isReadable() && QFileInfo(varsPath).isReadable())
                    return {codePath, varsPath};
            }
        }
        return {};
    }

    bool prepareSecureBootVariables() {
        if (!useSecureBoot_)
            return true;
        const QString varsPath = diskPath_ + ".vars.fd";
        const QFileInfo existing(varsPath);
        const QFileInfo varsTemplate(secureVarsTemplatePath_);
        if (existing.exists()) {
            if (!existing.isFile() || existing.size() != varsTemplate.size()) {
                appendOutput("The saved UEFI variables file is invalid or does not match the firmware.");
                status_->setText("Invalid Secure Boot variables");
                return false;
            }
        } else if (!QFile::copy(secureVarsTemplatePath_, varsPath)) {
            appendOutput("Could not create persistent UEFI variables file: " + varsPath);
            status_->setText("Could not initialize Secure Boot");
            return false;
        }
        secureVarsPath_ = varsPath;
        return true;
    }

    void launchVm() {
        setBusy(true);
        const QString architecture = architectureInput_->currentData().toString();
        const QString firmware = firmwarePath(architecture);
        const bool useHypervisor = architecture == hostArchitecture();
        if (!useSecureBoot_ && firmware.isEmpty()) {
            setBusy(false);
            status_->setText("UEFI firmware not found");
            appendOutput("QEMU's UEFI firmware is missing. Reinstall QEMU (`brew reinstall qemu`) "
                         "or install the matching EDK2/OVMF firmware.");
            return;
        }
        if (!prepareSecureBootVariables()) {
            setBusy(false);
            return;
        }

        QStringList arguments{
            "-name", nameInput_->text().trimmed(),
            "-machine", architecture == "aarch64"
                ? "virt" : (useSecureBoot_ ? "q35,smm=on" : "q35"),
            "-accel", useHypervisor ? "hvf" : "tcg",
            "-cpu", useHypervisor ? "host" : "max",
            "-smp", QString::number(coresInput_->value()),
            "-m", QString("%1G").arg(memoryInput_->value()),
            "-display", "cocoa",
            "-drive", QString("if=none,id=systemdisk,format=qcow2,file=%1").arg(diskPath_),
            "-device", "nvme,drive=systemdisk,serial=vmstudio",
            "-netdev", "user,id=net0",
            "-device", "virtio-net-pci,netdev=net0"
        };
        if (useSecureBoot_) {
            arguments << "-drive" << QString("if=pflash,format=raw,readonly=on,file=%1")
                                          .arg(secureCodePath_)
                      << "-drive" << QString("if=pflash,format=raw,file=%1").arg(secureVarsPath_);
        } else {
            arguments << "-bios" << firmware;
        }
        if (useTpm_) {
            const QString stateDirectory = diskPath_ + ".tpm";
            if (!QDir().mkpath(stateDirectory)) {
                setBusy(false);
                status_->setText("Could not initialize TPM");
                appendOutput("Could not create TPM state directory: " + stateDirectory);
                return;
            }
            tpmSocketDirectory_ = std::make_unique<QTemporaryDir>(
                QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation))
                    .filePath("vmstudio-tpm-XXXXXX"));
            if (!tpmSocketDirectory_->isValid()) {
                tpmSocketDirectory_.reset();
                setBusy(false);
                status_->setText("Could not initialize TPM");
                appendOutput("Could not create temporary TPM socket directory.");
                return;
            }
            tpmSocketPath_ = tpmSocketDirectory_->filePath("swtpm.sock");
            arguments << "-chardev" << QString("socket,id=chrtpm,path=%1").arg(tpmSocketPath_)
                      << "-tpmdev" << "emulator,id=tpm0,chardev=chrtpm"
                      << "-device" << (architecture == "aarch64"
                                            ? "tpm-tis-device,tpmdev=tpm0"
                                            : "tpm-tis,tpmdev=tpm0");
            vmArguments_ = arguments;
            const QString swtpm = findTool("swtpm");
            const QStringList swtpmArguments{
                "socket", "--tpm2", "--tpmstate", "dir=" + stateDirectory,
                "--ctrl", "type=unixio,path=" + tpmSocketPath_
            };
            status_->setText("Starting TPM 2.0 emulator…");
            appendOutput("Starting swtpm with persistent state at " + stateDirectory);
            tpmProcess_->start(swtpm, swtpmArguments);
            return;
        }
        if (bootInstallerInput_->isChecked()) {
            arguments << "-device" << "qemu-xhci"
                      << "-drive" << QString(
                             "if=none,id=installer,media=cdrom,readonly=on,file=%1").arg(isoPath_)
                      << "-device" << "usb-storage,drive=installer,bootindex=1"
                      << "-boot" << "order=dc";
        } else {
            arguments << "-boot" << "order=c";
        }
        setBusy(true);
        status_->setText("Starting virtual machine…");
        appendOutput("Launching QEMU: " + qemuExecutable_);
        vmProcess_->start(qemuExecutable_, arguments);
    }

    void waitForTpmSocket(int attempt) {
        if (tpmProcess_->state() != QProcess::Running) {
            setBusy(false);
            status_->setText("TPM emulator stopped unexpectedly");
            appendOutput("swtpm stopped before its control socket became ready.");
            tpmSocketDirectory_.reset();
            return;
        }
        if (!QFileInfo::exists(tpmSocketPath_)) {
            if (attempt < 100) {
                QTimer::singleShot(50, this, [this, attempt] { waitForTpmSocket(attempt + 1); });
                return;
            }
            setBusy(false);
            status_->setText("TPM emulator did not start");
            appendOutput("Timed out waiting for the swtpm control socket.");
            stopTpm();
            return;
        }

        if (bootInstallerInput_->isChecked()) {
            vmArguments_ << "-device" << "qemu-xhci"
                         << "-drive" << QString(
                                "if=none,id=installer,media=cdrom,readonly=on,file=%1").arg(isoPath_)
                         << "-device" << "usb-storage,drive=installer,bootindex=1"
                         << "-boot" << "order=dc";
        } else {
            vmArguments_ << "-boot" << "order=c";
        }
        appendOutput("Launching QEMU: " + qemuExecutable_);
        vmProcess_->start(qemuExecutable_, vmArguments_);
    }

    void stopTpm() {
        if (tpmProcess_->state() != QProcess::NotRunning)
            tpmProcess_->terminate();
        else
            tpmSocketDirectory_.reset();
    }

    void appendOutput(const QString &text) {
        if (!text.isEmpty())
            output_->append(text.toHtmlEscaped());
    }

    void setBusy(bool busy) {
        startButton_->setEnabled(!busy);
        stopButton_->setEnabled(busy && vmProcess_->state() != QProcess::NotRunning);
        nameInput_->setEnabled(!busy);
        osInput_->setEnabled(!busy);
        architectureInput_->setEnabled(!busy);
        isoInput_->setEnabled(!busy);
        bootInstallerInput_->setEnabled(!busy);
        tpmInput_->setEnabled(!busy && osInput_->currentData().toString() == "windows");
        secureBootInput_->setEnabled(!busy && osInput_->currentData().toString() == "windows");
        secureCodeInput_->setEnabled(!busy && osInput_->currentData().toString() == "windows"
                                     && secureBootInput_->isChecked());
        secureVarsInput_->setEnabled(!busy && osInput_->currentData().toString() == "windows"
                                     && secureBootInput_->isChecked());
    }

    QLineEdit *nameInput_ = nullptr;
    QComboBox *osInput_ = nullptr;
    QComboBox *architectureInput_ = nullptr;
    QLineEdit *isoInput_ = nullptr;
    QCheckBox *bootInstallerInput_ = nullptr;
    QCheckBox *tpmInput_ = nullptr;
    QCheckBox *secureBootInput_ = nullptr;
    QLineEdit *secureCodeInput_ = nullptr;
    QLineEdit *secureVarsInput_ = nullptr;
    QLineEdit *diskInput_ = nullptr;
    QSpinBox *diskSizeInput_ = nullptr;
    QSpinBox *memoryInput_ = nullptr;
    QSpinBox *coresInput_ = nullptr;
    QLabel *status_ = nullptr;
    QPushButton *startButton_ = nullptr;
    QPushButton *stopButton_ = nullptr;
    QTextEdit *output_ = nullptr;
    QProcess *diskProcess_ = nullptr;
    QProcess *vmProcess_ = nullptr;
    QProcess *tpmProcess_ = nullptr;
    std::unique_ptr<QTemporaryDir> tpmSocketDirectory_;
    QString qemuExecutable_;
    QString isoPath_;
    QString diskPath_;
    QString secureCodePath_;
    QString secureVarsTemplatePath_;
    QString secureVarsPath_;
    QString tpmSocketPath_;
    QStringList vmArguments_;
    bool diskPathTouched_ = false;
    bool useTpm_ = false;
    bool useSecureBoot_ = false;
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QApplication::setStyle("Fusion");
    QApplication::setApplicationName("VM Studio");
    QApplication::setOrganizationName("VM Studio");
    VmStudioWindow window;
    window.show();
    return app.exec();
}
