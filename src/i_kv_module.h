#pragma once

#include <QString>
#include <QtPlugin>

/**
 * IKvModule — Abstract interface for direct kv_module access.
 *
 * Allows CalendarStore to call KV operations without going through
 * logos_host / QtRemoteObjects.
 */
class IKvModule {
public:
    virtual ~IKvModule() = default;

    virtual void set(const QString &ns, const QString &key, const QString &value) = 0;
    virtual QString get(const QString &ns, const QString &key) = 0;
    virtual void remove(const QString &ns, const QString &key) = 0;
    virtual QString list(const QString &ns, const QString &prefix) = 0;
    virtual QString listAll(const QString &ns) = 0;
    virtual void clear(const QString &ns) = 0;
};

#define IKvModule_iid "com.logos.module.IKvModule"
Q_DECLARE_INTERFACE(IKvModule, IKvModule_iid)
