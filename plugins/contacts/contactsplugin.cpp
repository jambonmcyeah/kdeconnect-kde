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
    qCDebug(KDECONNECT_PLUGIN_CONTACTS) << "Contacts constructor for device " << device()->name();
}

ContactsPlugin::~ContactsPlugin()
{
    QDBusConnection::sessionBus().unregisterObject(dbusPath(), QDBusConnection::UnregisterTree);
//     qCDebug(KDECONNECT_PLUGIN_CONTACTS) << "Contacts plugin destructor for device" << device()->name();
}

bool ContactsPlugin::receivePacket(const NetworkPacket& np)
{
    qCDebug(KDECONNECT_PLUGIN_CONTACTS) << "Packet Received for device " << device()->name();
    qCDebug(KDECONNECT_PLUGIN_CONTACTS) << np.body();

    if (np.type() == PACKAGE_TYPE_CONTACTS_RESPONSE_UIDS_TIMESTAMPS)
    {
        return this->handleResponseUIDsTimestamps(np);
    } else if (np.type() == PACKET_TYPE_CONTACTS_RESPONSE_VCARDS)
    {
        return this->handleResponseVCards(np);
    }  else
    {
        // Is this check necessary?
        qCDebug(KDECONNECT_PLUGIN_CONTACTS) << "Unknown package type received from device: "
                << device()->name() << ". Maybe you need to upgrade KDE Connect?";
        return false;
    }
}

void ContactsPlugin::synchronizeRemoteWithLocal()
{
	this->sendRequest(PACKET_TYPE_CONTACTS_REQUEST_ALL_UIDS_TIMESTAMP);
}

bool ContactsPlugin::handleResponseUIDsTimestamps(const NetworkPacket& np)
{
    if (!np.has("uids"))
    {
        qCDebug(KDECONNECT_PLUGIN_CONTACTS) << "handleResponseUIDsTimestamps:" << "Malformed packet does not have uids key";
        return false;
    }
    uIDList_t uIDsToUpdate;

    QStringList uIDs = np.get<QStringList>("uids");

    // TODO: Check local storage... As soon as local storage is implemented. For now, just send everything
    for (QString ID : uIDs)
    {
        uIDsToUpdate.push_back(ID.toLongLong());
    }

    this->sendRequestWithIDs(PACKET_TYPE_CONTACTS_REQUEST_VCARDS_BY_UIDS, uIDsToUpdate);

    return true;
}

bool ContactsPlugin::handleResponseVCards(const NetworkPacket& np)
{
    if (!np.has("uids"))
    {
        qCDebug(KDECONNECT_PLUGIN_CONTACTS) << "handleResponseVCards:" << "Malformed packet does not have uids key";
        return false;
    }

    QStringList uIDs = np.get<QStringList>("uids");

    for (auto ID : uIDs)
    {
        qWarning() << "Got VCard:" << np.get<QString>(ID);
    }
    qCDebug(KDECONNECT_PLUGIN_CONTACTS) << "handleResponseVCards:" << "Got" << uIDs.size() << "VCards";
    return true;
}

bool ContactsPlugin::sendRequest(QString packageType)
{
    NetworkPacket np(packageType);
    bool success = sendPacket(np);
    qCDebug(KDECONNECT_PLUGIN_CONTACTS) << "sendRequest: Sending " << packageType << success;

    return success;
}

bool ContactsPlugin::sendRequestWithIDs(QString packageType, uIDList_t uIDs)
{
    NetworkPacket np(packageType);

    // Convert IDs to strings
    QStringList uIDsAsStrings;
    for (auto uID : uIDs)
    {
        uIDsAsStrings.append(QString::number(uID));
    }
    np.set<QStringList>("uids", uIDsAsStrings);
    bool success = sendPacket(np);
    return success;
}

QString ContactsPlugin::dbusPath() const
{
    return "/modules/kdeconnect/devices/" + device()->id() + "/contacts";
}

#include "contactsplugin.moc"

