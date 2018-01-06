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

#ifndef CONTACTSPLUGIN_H
#define CONTACTSPLUGIN_H

#include <QObject>

#include <QDBusArgument>
#include <QMutex>

#include <core/kdeconnectplugin.h>

/**
 * Request the device send us the entire contacts book
 */
#define PACKAGE_TYPE_CONTACTS_REQUEST_ALL QStringLiteral("kdeconnect.contacts.request_all")

/**
 * Response from the device containing a list of zero or more pairings of names and phone numbers
 */
#define PACKAGE_TYPE_CONTACTS_RESPONSE QStringLiteral("kdeconnect.contacts.response")

/**
 * Amount of time we are willing to wait before deciding the device is not going to reply
 *
 * This is a random number picked by me, and might need to be adjusted based on real-world testing
 */
#define CONTACTS_TIMEOUT_MS 10000

/**
 * Type definition for a single contact database entry
 *
 * A contact is identified by:
 * a name, paired to a phone number and a phone number category
 * or
 * a phone number, paired to a name and a phone number category
 */
typedef QPair<QString, QPair<QString, QString>> ContactsEntry;

/**
 * Type definition for a contacts database
 *
 * A contacts database pairs either names or numbers to a list of corresponding contacts
 */
typedef QHash<QString, QSet<ContactsEntry>> ContactsCache;

class Q_DECL_EXPORT ContactsPlugin
    : public KdeConnectPlugin
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kdeconnect.device.contacts")

public:
    explicit ContactsPlugin(QObject *parent, const QVariantList &args);
    ~ContactsPlugin() override;

    bool receivePackage(const NetworkPackage& np) override;
    void connected() override {}

    QString dbusPath() const override;

public Q_SLOTS:

    /**
     * Get all the contacts known from the phone
     *
     * @return Map of names to pairs of phone number categories and phone numbers
     * e.g. <Mom, <Work, +12025550101>>
     */
    Q_SCRIPTABLE QStringList getAllContacts();

    // TODO: Combine sendAllContactsRequest and getCachedContacts so that only one DBus call needs to be made

protected:
    /**
     * Store locally-known list of contacts keyed by name, e.g. <Mom, <Work, +12025550101>>
     */
    ContactsCache cachedContactsByName;

    /**
     * Store locally-known list of contacts keyed by number, e.g. <+12025550101, <Mom, Work>>
     */
    ContactsCache cachedContactsByNumber;

    /**
     * Enforce mutual exclusion when accessing the cached contacts
     */
    QMutex cacheLock;

    /**
     * Get the locally-known collection of contacts
     *
     * If the cache has not yet been populated, populate it first
     *
     * @return Locally-cached contacts buffers
     */
    QPair<ContactsCache, ContactsCache> getCachedContacts();

    /**
     * Query the remote device for its contacts book, bypassing and populating the local cache
     */
    void sendAllContactsRequest();


protected: Q_SIGNALS:
    /**
     * Emitted to indicate that we have received some contacts from the device
     */
    Q_SCRIPTABLE void cachedContactsAvailable();
};

#endif // CONTACTSPLUGIN_H
