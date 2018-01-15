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

#include <KLocalizedString>
#include <KPluginFactory>

#include <QDebug>
#include <QDBusConnection>
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
    // TODO: Error checking like https://doc.qt.io/qt-5/qtdbus-pingpong-pong-cpp.html
    QDBusConnection::sessionBus().registerService(this->dbusPath());
    QDBusConnection::sessionBus().registerObject(this->dbusPath(), this, QDBusConnection::ExportAllSlots);
    qCDebug(KDECONNECT_PLUGIN_CONTACTS) << "Contacts constructor for device " << device()->name();
}

ContactsPlugin::~ContactsPlugin()
{
//     qCDebug(KDECONNECT_PLUGIN_CONTACTS) << "Contacts plugin destructor for device" << device()->name();
}

bool ContactsPlugin::receivePackage(const NetworkPackage& np)
{
    qCDebug(KDECONNECT_PLUGIN_CONTACTS) << "Package Received for device " << device()->name();
    qCDebug(KDECONNECT_PLUGIN_CONTACTS) << np.body();

    if (np.type() == PACKAGE_TYPE_CONTACTS_RESPONSE)
    {
        int index = 0;

        cacheLock.lock();
        while(true)
        {
            auto contact = np.get<QStringList>(QString::number(index));
            if (contact.length() == 0)
            {
                // If nothing was returned, assume we have processed all contacts
                break;
            }
            QString contactName = contact[0];
            QString contactNumber = contact[2];
            QString contactNumberCategory = contact[1];

            ContactsEntry newContact =
                    ContactsEntry(contactName,
                                  QPair<QString, QString>(contactNumberCategory, contactNumber));

            cachedContactsByName[contactName].insert(newContact);
            cachedContactsByNumber[contactNumber].insert(newContact);

            index ++;
        }
        cacheLock.unlock();

        // Now that we have processed an incoming packet, there (should be) contacts available
        Q_EMIT cachedContactsAvailable();
    } else if (np.type() == PACKAGE_TYPE_CONTACTS_RESPONSE_UIDS)
    {
        this->handleResponseUIDs(np);
    }
    else
    {
        // Is this check necessary?
        qCDebug(KDECONNECT_PLUGIN_CONTACTS) << "Unknown package type received from device: "
                << device()->name() << ". Maybe you need to upgrade KDE Connect?";
        return false;
    }

    return true;
}

void ContactsPlugin::sendAllContactsRequest()
{
    NetworkPackage np(PACKAGE_TYPE_CONTACTS_REQUEST_ALL);
    bool success = sendPackage(np);
    qCDebug(KDECONNECT_PLUGIN_CONTACTS) << "sendAllContactsRequest:" << success;
}

bool ContactsPlugin::sendRequest(QString packageType)
{
    NetworkPackage np(packageType);
    bool success = sendPackage(np);
    qCDebug(KDECONNECT_PLUGIN_CONTACTS) << "sendRequest: Sending " << packageType << success;

    return success;
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
            qCDebug(KDECONNECT_PLUGIN_CONTACTS)<< "getCachedContacts:" << "Timeout waiting for device reply";
        }
    }

    uIDCacheLock.lock();
    toReturn = uIDCache;
    uIDCacheLock.unlock();

    return toReturn;
}

QPair<ContactsCache, ContactsCache> ContactsPlugin::getCachedContacts()
{
    // I assume the remote device has at least one contact, so if there is nothing in the cache
    // it needs to be populated
    bool cachePopulated = cachedContactsByName.size() > 0;

    QPair<ContactsCache, ContactsCache> toReturn;

    if (cachePopulated)
    {
        // Do nothing. Fall through to return code.
    }
    else
    {
        // Otherwise we need to request the contacts book from the remote device
        this->sendAllContactsRequest();

        // Wait to receive result from phone or timeout
        QTimer timer;
        timer.setSingleShot(true);
        timer.setInterval(CONTACTS_TIMEOUT_MS);
        QEventLoop waitForReplyLoop;
        // Allow timeout
        connect(&timer, SIGNAL(timeout()), &waitForReplyLoop, SLOT(quit()));
        // Also allow a reply
        connect(this, SIGNAL(cachedContactsAvailable()), &waitForReplyLoop, SLOT(quit()));

        // Wait
        waitForReplyLoop.exec();

        if (!(timer.isActive()))
        {
            // The device did not reply before we timed out
            // Note that it still might reply eventually, and receivePackage(..) will import the
            // contacts to our local cache at that point
            qCDebug(KDECONNECT_PLUGIN_CONTACTS)<< "getCachedContacts:" << "Timeout waiting for device reply";
        }
    }

    cacheLock.lock();
    toReturn = QPair<ContactsCache, ContactsCache>(cachedContactsByName, cachedContactsByNumber);
    cacheLock.unlock();
    return toReturn;
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

QStringList ContactsPlugin::getAllContacts()
{
    QPair<ContactsCache, ContactsCache> contactsCaches = this->getCachedContacts();

    // Test code: Iterate through the list of contacts and reply to the DBus request with their names
    QStringList toReturn;
    for ( auto contactSet : contactsCaches.first)
    {
        for (ContactsEntry contact : contactSet)
        {
            toReturn.append(contact.first); // Name
            toReturn.append(contact.second.second); // Number
        }
    }

    toReturn.append(QString::number(contactsCaches.first.size()));

    return toReturn;
}

QStringList ContactsPlugin::getAllContactUIDs()
{
    QSet<long> uIDs = this->getCachedUIDs();
    QStringList toReturn;

    for (long uID : uIDs)
    {
        toReturn.append(QString::number(uID));
    }

    toReturn.append(QString::number(uIDCache.size()));

    return toReturn;
}

QString ContactsPlugin::dbusPath() const
{
    return "/modules/kdeconnect/devices/" + device()->id() + "/contacts";
}

#include "contactsplugin.moc"

