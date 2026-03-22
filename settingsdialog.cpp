#include "appsettings.h"
#include "settingsdialog.h"
#include "ui_settingsdialog.h"

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);

    ui->chkAutoAssembleOnFileOpen->setChecked(settings().autoAssembleOnFileOpen());
    ui->spnProcessEventsEverySoOften->setValue(settings().processEventsEverySoOften());
    ui->spnProcessEventsForVerticalSyncs->setValue(settings().processEventsForVerticalSyncs());
    ui->chkProfilingEnabled->setChecked(settings().profilingEnabled());
    ui->spnProfilingGranularityShift->setValue(settings().profilingGranularityShift());

    connect(this, &QDialog::accepted, this, &SettingsDialog::acceptSettings);
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

/*slot*/ void SettingsDialog::acceptSettings()
{
    settings().setAutoAssembleOnFileOpen(ui->chkAutoAssembleOnFileOpen->isChecked());
    settings().setProcessEventsEverySoOften(ui->spnProcessEventsEverySoOften->value());
    settings().setProcessEventsForVerticalSyncs(ui->spnProcessEventsForVerticalSyncs->value());
    settings().setProfilingEnabled(ui->chkProfilingEnabled->isChecked());
    settings().setProfilingGranularityShift(ui->spnProfilingGranularityShift->value());
}
