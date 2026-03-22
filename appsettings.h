#ifndef APPSETTINGS_H
#define APPSETTINGS_H

#include <QSettings>

class AppSettings : public QSettings
{
public:
    explicit AppSettings();

    bool autoAssembleOnFileOpen() const { return value("autoAssembleOnFileOpen", true).toBool(); };
    void setAutoAssembleOnFileOpen(bool autoAssemble) { setValue("autoAssembleOnFileOpen", autoAssemble); };
    int processEventsEverySoOften() const { return value("processEventsEverySoOften", 0).toInt(); };
    void setProcessEventsEverySoOften(int everySoOften) { setValue("processEventsEverySoOften", everySoOften); };
    int processEventsForVerticalSyncs() const { return value("processEventsForVerticalSyncs", 10 * 20).toInt(); };
    void setProcessEventsForVerticalSyncs(int verticalSyncs) { setValue("processEventsForVerticalSyncs", verticalSyncs); };
    bool profilingEnabled() const { return value("profilingEnabled", true).toBool(); };
    void setProfilingEnabled(bool enabled) { setValue("profilingEnabled", enabled); };
    int profilingGranularityShift() const { return value("profilingGranularityShift", 1).toInt(); };
    void setProfilingGranularityShift(int granularityShift) { setValue("profilingGranularityShift", granularityShift); };
    QStringList recentFiles() const { return value("recentFiles", 1).toStringList(); };
    void setRecentFiles(QStringList recentFiles) { setValue("recentFiles", recentFiles); };
};

AppSettings& settings();

#endif // APPSETTINGS_H
