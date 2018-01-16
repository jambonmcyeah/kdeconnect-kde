/**
 * Copyright 2018 Simon Redman <simon@ergotech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <contactsplugin.h>
#include "phoneentry.h"

#include <KLocalizedString>
#include <KPluginFactory>

#include <QDebug>
#include <QDBusConnection>
#include <QtDBus>
#include <QEventLoop>
#include <QLoggingCategory>
#include <QMutex>
#include <QTimer>

#include <core/device.h>

K_PLUGIN_FACTORY_WITH_JSON( KdeConnectPluginFactory, "kdeconnect_contacts.json", registerPlugin< ContactsPlugin >(); )

Q_LOGGING_CATEGORY(KDECONNECT_PLUGIN_CONTACTS, "kdeconnect.plugin.contacts")

ContactsPlugin::ContactsPlugin(QObject* parent, const QVariantList& args)
    : KdeConnectPlugin(parent, args)
{
    // Initialize the dbus interface
    qRegisterMetaType<uID_t>("uID");
    qDBusRegisterMetaType<uID_t>();

    qRegisterMetaType<uIDList_t>("uIDList");
    qDBusRegisterMetaType<uIDList_t>();

    qRegisterMetaType<NameCache_t>("NameCache");
    qDBusRegisterMetaType<NameCache_t>();

    PhoneEntry::registerMetaType();

    qRegisterMetaType<PhoneEntryList_t>("PhoneEntryList");
    qDBusRegisterMetaType<PhoneEntryList_t>();

    qRegisterMetaType<PhoneCache_t>("PhoneCache");
    qDBusRegisterMetaType<PhoneCache_t>();

    qCDebug(KDECONNECT_PLUGIN_CONTACTS) << "Contacts constructor for device " << device()->name();
}

ContactsPlugin::~ContactsPlugin()
{
    QDBusConnection::sessionBus().unregisterObject(dbusPath(), QDBusConnection::UnregisterTree);
//     qCDebug(KDECONNECT_PLUGIN_CONTACTS) << "Contacts plugin destructor for device" << device()->name();
}

bool ContactsPlugin::receivePackage(const NetworkPackage& np)
{
    qCDebug(KDECONNECT_PLUGIN_CONTACTS) << "Package Received for device " << device()->name();
    qCDebug(KDECONNECT_PLUGIN_CONTACTS) << np.body();

    if (np.type() == PACKAGE_TYPE_CONTACTS_RESPONSE_UIDS)
    {
        return this->handleResponseUIDs(np);
    } else if (np.type() == PACKAGE_TYPE_CONTACTS_RESPONSE_NAMES)
    {
        return this->handleResponseNames(np);
    }  else if (np.type() == PACKAGE_TYPE_CONTACTS_RESPONSE_PHONES)
    {
        return this->handleResponsePhones(np);
    }  else if (np.type() == PACKAGE_TYPE_CONTACTS_RESPONSE_EMAILS)
    {
        return this->handleResponseEmails(np);
    } else
    {
        // Is this check necessary?
        qCDebug(KDECONNECT_PLUGIN_CONTACTS) << "Unknown package type received from device: "
                << device()->name() << ". Maybe you need to upgrade KDE Connect?";
        return false;
    }
}

uIDList_t ContactsPlugin::getAllContactUIDs()
{
    UIDCache_t uIDs = this->getCachedUIDs();
    uIDList_t toReturn;

    for (uID_t uID : uIDs)
    {
        toReturn.push_back(uID);
    }

    return toReturn;
}

NameCache_t ContactsPlugin::getNamesByUIDs(uIDList_t uIDs)
{
    return this->getCachedNamesForIDs(uIDs);
}

PhoneCache_t ContactsPlugin::getPhonesByUIDs(uIDList_t uIDs)
{
    return this->getCachedPhonesForIDs(uIDs);
}

PhoneCache_t ContactsPlugin::getEmailsByUIDs(uIDList_t uIDs)
{
    return this->getCachedEmailsForIDs(uIDs);
}

bool ContactsPlugin::handleResponseUIDs(const NetworkPackage& np)
{
    if (!np.has("uids"))
    {
        qCDebug(KDECONNECT_PLUGIN_CONTACTS) << "handleResponseUIDs:" << "Malformed packet does not have uids key";
        return false;
    }

    QStringList uIDs = np.get<QStringList>("uids");

    uIDCacheLock.lock();
    for (const QString& uID : uIDs)
    {
        uIDCache.insert(uID.toLong());
    }
    uIDCacheLock.unlock();

    Q_EMIT cachedUIDsAvailable();
    return true;
}

bool ContactsPlugin::handleResponseNames(const NetworkPackage& np)
{
    if (!np.has("uids"))
    {
        qCDebug(KDECONNECT_PLUGIN_CONTACTS) << "handleResponseNames:" << "Malformed packet does not have uids key";
        return false;
    }

    QStringList uIDs = np.get<QStringList>("uids");

    namesCacheLock.lock();
    for (QString uID : uIDs)
    {
        if (!np.has(uID))
        {
            // The packet is malformed
            qCDebug(KDECONNECT_PLUGIN_CONTACTS) << "handleResponseNames:" << "Malformed packet does not have key " << uID;
            // Struggle on anyway. Maybe we have other useful data.
            continue;
        }
        namesCache.insert(uID.toLongLong(), np.get<QString>(uID));
    }
    namesCacheLock.unlock();


    Q_EMIT cachedNamesAvailable();
    return true;
}

bool ContactsPlugin::handleResponsePhones(const NetworkPackage& np)
{
    if (!np.has("uids"))
    {
        qCDebug(KDECONNECT_PLUGIN_CONTACTS) << "handleResponsePhones:" << "Malformed packet does not have uids key";
        return false;
    }

    QStringList uIDs = np.get<QStringList>("uids");

    phonesCacheLock.lock();
    for (QString uID : uIDs)
    {
        if (!np.has(uID))
        {
            // The packet is malformed
            qCDebug(KDECONNECT_PLUGIN_CONTACTS) << "handleResponsePhones:" << "Malformed packet does not have key " << uID;
            // Struggle on anyway. Maybe we have other useful data.
            continue;
        }
        // Get the list of all phone numbers for this contact
        auto EntriesList = np.get<QVariantList>(uID);

        // For each list, extract a single NumberEntry
        // This should be possible to do directly with QVarient and NumberEntry, but I can't get it to work
        for (QVariant entryVariant : EntriesList)
        {
            QStringList entryStr = entryVariant.value<QStringList>();
            if (entryStr.size() < 3)
            {
                qCDebug(KDECONNECT_PLUGIN_CONTACTS) << "handleResponsePhones:" << "Malformed packet does not have enough entries for a PhoneEntry for UID " << uID;
                // If the packet is malformed, better to continue than crash...
                continue;
            }
            QString number = entryStr[0];
            int type = entryStr[1].toInt();
            QString label = entryStr[2];
            PhoneEntry entry(number, type, label);

            phonesCache[uID.toLongLong()].append(entry);
        }
    }
    phonesCacheLock.unlock();


    Q_EMIT cachedPhonesAvailable();
    return true;
}

bool ContactsPlugin::handleResponseEmails(const NetworkPackage& np)
{
    if (!np.has("uids"))
    {
        qCDebug(KDECONNECT_PLUGIN_CONTACTS) << "handleResponseEmails:" << "Malformed packet does not have uids key";
        return false;
    }

    QStringList uIDs = np.get<QStringList>("uids");

    emailsCacheLock.lock();
    for (QString uID : uIDs)
    {
        if (!np.has(uID))
        {
            // The packet is malformed
            qCDebug(KDECONNECT_PLUGIN_CONTACTS) << "handleResponseEmails:" << "Malformed packet does not have key " << uID;
            // Struggle on anyway. Maybe we have other useful data.
            continue;
        }
        // Get the list of all email addresses for this contact
        auto EntriesList = np.get<QVariantList>(uID);

        // For each list, extract a single EmailEntry
        // This should be possible to do directly with QVarient and NumberEntry, but I can't get it to work
        for (QVariant entryVariant : EntriesList)
        {
            QStringList entryStr = entryVariant.value<QStringList>();
            if (entryStr.size() < 3)
            {
                qCDebug(KDECONNECT_PLUGIN_CONTACTS) << "handleResponseEmails:" << "Malformed packet does not have enough entries for a EmailEntry for UID " << uID;
                // If the packet is malformed, better to continue than crash...
                continue;
            }
            QString address = entryStr[0];
            int type = entryStr[1].toInt();
            QString label = entryStr[2];
            EmailEntry entry(address, type, label);

            emailsCache[uID.toLongLong()].append(entry);
        }
    }
    emailsCacheLock.unlock();

    Q_EMIT cachedEmailsAvailable();
    return true;
}

UIDCache_t ContactsPlugin::getCachedUIDs()
{
    UIDCache_t toReturn;

    // I assume the remote device has at least one contact, so if there is nothing in the cache
    // it needs to be populated
    bool cachePopulated = uIDCache.size() > 0;

    if (!cachePopulated)
    {
        this->sendRequest(PACKAGE_TYPE_CONTACTS_REQUEST_ALL_UIDS);

        // Wait to receive result from phone or timeout
        QTimer timer;
        timer.setSingleShot(true);
        timer.setInterval(CONTACTS_TIMEOUT_MS);
        QEventLoop waitForReplyLoop;
        // Allow timeout
        connect(&timer, SIGNAL(timeout()), &waitForReplyLoop, SLOT(quit()));
        // Also allow a reply
        connect(this, SIGNAL(cachedUIDsAvailable()), &waitForReplyLoop, SLOT(quit()));

        // Wait
        waitForReplyLoop.exec();

        if (!(timer.isActive()))
        {
            // The device did not reply before we timed out
            // Note that it still might reply eventually, and receivePackage(..) will import the
            // contacts to our local cache at that point
            qCDebug(KDECONNECT_PLUGIN_CONTACTS)<< "getCachedUIDs:" << "Timeout waiting for device reply";
        }
    }

    uIDCacheLock.lock();
    toReturn = uIDCache;
    uIDCacheLock.unlock();

    return toReturn;
}

NameCache_t ContactsPlugin::getCachedNamesForIDs(QList<uID_t> uIDs)
{
    NameCache_t toReturn;

    // Figure out the list of IDs for which we don't have names
    QList<uID_t> uncachedIDs;
    for (auto id : uIDs)
    {
        if (!namesCache.contains(id))
        {
            uncachedIDs.append(id);
        }
    }

    if (uncachedIDs.length() > 0) // If there are uncached IDs
    {
        this->sendRequestWithIDs(PACKAGE_TYPE_CONTACTS_REQUEST_NAMES_BY_UIDS, uncachedIDs);

        // Wait to receive result from phone or timeout
        QTimer timer;
        timer.setSingleShot(true);
        timer.setInterval(CONTACTS_TIMEOUT_MS);
        QEventLoop waitForReplyLoop;
        // Allow timeout
        connect(&timer, SIGNAL(timeout()), &waitForReplyLoop, SLOT(quit()));
        // Also allow a reply
        connect(this, SIGNAL(cachedNamesAvailable()), &waitForReplyLoop, SLOT(quit()));

        // Wait
        waitForReplyLoop.exec();
    }

    for (auto id : uIDs)
    {
        // Still need to check, since we may have an invalid ID
        if (namesCache.contains(id))
        {
            toReturn.insert(id, namesCache[id]);
        }
    }

    return toReturn;
}

PhoneCache_t ContactsPlugin::getCachedPhonesForIDs(uIDList_t uIDs)
{
    PhoneCache_t toReturn;

    // Figure out the list of IDs for which we don't have phone numbers
    QList<uID_t> uncachedIDs;
    phonesCacheLock.lock();
    for (auto id : uIDs)
    {
        if (!phonesCache.contains(id))
        {
            uncachedIDs.append(id);
        }
    }
    phonesCacheLock.unlock();

    if (uncachedIDs.length() > 0) // If there are uncached IDs
    {
        this->sendRequestWithIDs(PACKAGE_TYPE_CONTACTS_REQUEST_PHONES_BY_UIDS, uncachedIDs);

        // Wait to receive result from phone or timeout
        QTimer timer;
        timer.setSingleShot(true);
        timer.setInterval(CONTACTS_TIMEOUT_MS);
        QEventLoop waitForReplyLoop;
        // Allow timeout
        connect(&timer, SIGNAL(timeout()), &waitForReplyLoop, SLOT(quit()));
        // Also allow a reply
        connect(this, SIGNAL(cachedPhonesAvailable()), &waitForReplyLoop, SLOT(quit()));

        // Wait
        waitForReplyLoop.exec();
    }

    phonesCacheLock.lock();
    for (auto id : uIDs)
    {
        // Still need to check, since we may have an invalid ID
        if (phonesCache.contains(id))
        {
            toReturn.insert(id, phonesCache[id]);
        }
    }
    phonesCacheLock.unlock();

    return toReturn;
}

EmailCache_t ContactsPlugin::getCachedEmailsForIDs(uIDList_t uIDs)
{
    EmailCache_t toReturn;

    // Figure out the list of IDs for which we don't have phone numbers
    QList<uID_t> uncachedIDs;
    emailsCacheLock.lock();
    for (auto id : uIDs)
    {
        if (!emailsCache.contains(id))
        {
            uncachedIDs.append(id);
        }
    }
    emailsCacheLock.unlock();

    if (uncachedIDs.length() > 0) // If there are uncached IDs
    {
        this->sendRequestWithIDs(PACKAGE_TYPE_CONTACTS_REQUEST_EMAILS_BY_UIDS, uncachedIDs);

        // Wait to receive result from phone or timeout
        QTimer timer;
        timer.setSingleShot(true);
        timer.setInterval(CONTACTS_TIMEOUT_MS);
        QEventLoop waitForReplyLoop;
        // Allow timeout
        connect(&timer, SIGNAL(timeout()), &waitForReplyLoop, SLOT(quit()));
        // Also allow a reply
        connect(this, SIGNAL(cachedEmailsAvailable()), &waitForReplyLoop, SLOT(quit()));

        // Wait
        waitForReplyLoop.exec();
    }

    emailsCacheLock.lock();
    for (auto id : uIDs)
    {
        // Still need to check, since we may have an invalid ID
        if (emailsCache.contains(id))
        {
            toReturn.insert(id, emailsCache[id]);
        }
    }
    emailsCacheLock.unlock();

    return toReturn;
}

bool ContactsPlugin::sendRequest(QString packageType)
{
    NetworkPackage np(packageType);
    bool success = sendPackage(np);
    qCDebug(KDECONNECT_PLUGIN_CONTACTS) << "sendRequest: Sending " << packageType << success;

    return success;
}

bool ContactsPlugin::sendRequestWithIDs(QString packageType, uIDList_t uIDs)
{
    NetworkPackage np(packageType);

    // Convert IDs to strings
    QStringList uIDsAsStrings;
    for (auto uID : uIDs)
    {
        uIDsAsStrings.append(QString::number(uID));
    }
    np.set<QStringList>("uids", uIDsAsStrings);
    bool success = sendPackage(np);
    return success;
}

QString ContactsPlugin::dbusPath() const
{
    return "/modules/kdeconnect/devices/" + device()->id() + "/contacts";
}

#include "contactsplugin.moc"

