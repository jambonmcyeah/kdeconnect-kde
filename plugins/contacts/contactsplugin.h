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
#include "phoneentry.h"

/**
 * Request the device send us the entire contacts book
 *
 * This package type is soon to be depreciated and deleted
 */
#define PACKAGE_TYPE_CONTACTS_REQUEST_ALL QStringLiteral("kdeconnect.contacts.request_all")

/**
 * Used to request the device send the unique ID of every contact
 */
#define PACKAGE_TYPE_CONTACTS_REQUEST_ALL_UIDS QStringLiteral("kdeconnect.contacts.request_all_uids")

/**
 * Used to request the names for a the contacts corresponding to a list of UIDs
 *
 * It shall contain the key "uids", which will have a list of uIDs (long int, as string)
 */
#define PACKAGE_TYPE_CONTACTS_REQUEST_NAMES_BY_UIDS QStringLiteral("kdeconnect.contacts.request_names_by_uid")

/**
 * Used to request the phone numbers for the contacts corresponding to a list of UIDs
 *
 * It shall contain the key "uids", which will have a list of uIDs (long int, as string)
 */
#define PACKAGE_TYPE_CONTACTS_REQUEST_PHONES_BY_UIDS QStringLiteral("kdeconnect.contacts.request_phones_by_uid")

/**
 * Used to request the email addresses for the contacts corresponding to a list of UIDs
 *
 * It shall contain the key "uids", which will have a list of uIDs (long int, as string)
 */
#define PACKAGE_TYPE_CONTACTS_REQUEST_EMAILS_BY_UIDS QStringLiteral("kdeconnect.contacts.request_emails_by_uid")

/**
 * Response from the device containing a list of zero or more pairings of names and phone numbers
 *
 * This package type is soon to be depreciated and deleted
 */
#define PACKAGE_TYPE_CONTACTS_RESPONSE QStringLiteral("kdeconnect.contacts.response")

/**
 * Response indicating the package contains a list of contact uIDs
 *
 * It shall contain the key "uids", which will mark a list of uIDs (long int, as string)
 * The returned IDs can be used in future requests for more information about the contact
 */
#define PACKAGE_TYPE_CONTACTS_RESPONSE_UIDS QStringLiteral("kdeconnect.contacts.response_uids")

/**
 * Response indicating the package contains a list of contact names
 *
 * It shall contain the key "uids", which will mark a list of uIDs (long int, as string)
 * then, for each UID, there shall be a field with the key of that UID and the value of the name of the contact
 *
 * For example:
 * ( 'uids' : ['1', '3', '15'],
 *  '1'  : 'John Smith',
 *  '3'  : 'Abe Lincoln',
 *  '15' : 'Mom' )
 */
#define PACKAGE_TYPE_CONTACTS_RESPONSE_NAMES QStringLiteral("kdeconnect.contacts.response_names")

/**
 * Response indicating the package contains a list of contact numbers
 *
 * It shall contain the key "uids", which will mark a list of uIDs (long int, as string)
 * then, for each UID, there shall be a 3-field list containing the phone number, the type, and the label
 *
 * For now, the values in types are undefined, but coincidentally match the list here:
 * https://developer.android.com/reference/android/provider/ContactsContract.CommonDataKinds.Phone.html
 *
 * The label field is defined to be the custom label if the number is a custom type, otherwise the empty string
 *
 * For example:
 * ( 'uids' : ['1', '3', '15'],
 *  '1'  : [ [ '+221234',  '2', '' ] ]
 *  '3'  : [ [ '+1(222)333-4444', '0', 'Big Red Button' ] ] // This number has a custom type
 *  '15' : [ [ '6061234', '1', '' ] ] )
 */
#define PACKAGE_TYPE_CONTACTS_RESPONSE_PHONES QStringLiteral("kdeconnect.contacts.response_phones")

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

typedef long long uID_t;
Q_DECLARE_METATYPE(uID_t)

typedef QList<uID_t> uIDList_t;
Q_DECLARE_METATYPE(uIDList_t)

typedef QSet<uID_t> UIDCache_t;
Q_DECLARE_METATYPE(UIDCache_t)

typedef QHash<uID_t, QString> NameCache_t;
Q_DECLARE_METATYPE(NameCache_t)

typedef QList<PhoneEntry> PhoneEntryList_t;
Q_DECLARE_METATYPE(PhoneEntryList_t)

typedef QHash<uID_t, PhoneEntryList_t> PhoneCache_t;
Q_DECLARE_METATYPE(PhoneCache_t)

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

    /**
     * Enumerate a uID for every contact on the phone
     *
     * These uIDs can be used in future dbus calls to get more information about the contact
     */
    Q_SCRIPTABLE uIDList_t getAllContactUIDs();

    /**
     * Reply with pairs of values, connecting uIDs to names
     */
    Q_SCRIPTABLE NameCache_t getNamesByUIDs(uIDList_t);

    /**
     * Reply with pairs of values, connecting uIDs to PhoneEntries
     */
    Q_SCRIPTABLE PhoneCache_t getPhonesByUIDs(uIDList_t);

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
     * Store list of locally-known contacts' uIDs
     */
    UIDCache_t uIDCache;

    /**
     * Store the mapping of locally-known uIDs mapping to names
     */
    NameCache_t namesCache;

    /**
     * Store the mapping of locally-known uIDs mapping to names
     */
    PhoneCache_t phonesCache;

    /**
     * Enforce mutual exclusion when accessing the cached contacts
     */
    QMutex cacheLock;

    /**
     * Enforce mutual exclusion when accessing the cached uIDs
     */
    QMutex uIDCacheLock;

    /**
     * Enforce mutual exclusion when accessing the cached names
     */
    QMutex namesCacheLock;

    /**
     * Enforce mutual exclusion when accessing the cached phone numbers
     */
    QMutex phonesCacheLock;

    /**
     *  Handle a packet of type PACKAGE_TYPE_CONTACTS_RESPONSE_UIDS
     */
    bool handleResponseUIDs(const NetworkPackage&);

    /**
     *  Handle a packet of type PACKAGE_TYPE_CONTACTS_RESPONSE_NAMES
     */
    bool handleResponseNames(const NetworkPackage&);

    /**
     *  Handle a packet of type PACKAGE_TYPE_CONTACTS_RESPONSE_PHONES
     */
    bool handleResponsePhones(const NetworkPackage&);

    /**
     * Get the locally-known collection of contacts
     *
     * If the cache has not yet been populated, populate it first
     *
     * @return Locally-cached contacts buffers
     */
    QPair<ContactsCache, ContactsCache> getCachedContacts();

    /**
     * Get the locally-known collection of uIDs
     *
     * If the cache has not yet been populated, populate it first
     *
     * @return Locally-cached contacts' uIDs
     */
    UIDCache_t getCachedUIDs();

    /**
     * Get the locally-known collection of uID -> name mapping
     *
     * If the cache has not yet been populated, populate it first
     *
     * @param uIDs List of IDs for which to fetch mappings
     * @return Locally-cached contacts' uID -> name mapping
     */
    NameCache_t getCachedNamesForIDs(uIDList_t uIDs);

    /**
     * Get the locally-known collection of uID -> PhoneEntry mapping
     *
     * If the cache has not yet been populated, populate it first
     *
     * @param uIDs List of IDs for which to fetch mappings
     * @return Locally-cached contacts' uID -> PhoneEntry mapping
     */
    PhoneCache_t getCachedPhonesForIDs(uIDList_t uIDs);

    /**
     * Query the remote device for its contacts book, bypassing and populating the local cache
     */
    void sendAllContactsRequest();

    /**
     * Send a request-type packet, which contains no body
     *
     * @return True if the send was successful, false otherwise
     */
    bool sendRequest(QString packageType);

    /**
     * Send a PACKAGE_TYPE_CONTACTS_REQUEST_NAMES_BY_UIDS-type packet with the list of UIDs to request
     *
     * @param uIDs List of uIDs to request
     * @return True if the send was successful, false otherwise
     */
    bool sendNamesForIDsRequest(uIDList_t uIDs);

    /**
     * Send a PACKAGE_TYPE_CONTACTS_REQUEST_PHONES_BY_UIDS-type packet with the list of UIDs to request
     *
     * @param uIDs List of uIDs to request
     * @return True if the send was successful, false otherwise
     */
    bool sendPhonesForIDsRequest(uIDList_t uIDs);

public: Q_SIGNALS:
    /**
     * Emitted to indicate that we have received some contacts from the device
     */
    Q_SCRIPTABLE void cachedContactsAvailable();

    /**
     * Emitted to indicate we have received some contacts' uIDs from the device
     */
    Q_SCRIPTABLE void cachedUIDsAvailable();

    /**
     * Emitted to indicate we have received some names from the device
     */
    Q_SCRIPTABLE void cachedNamesAvailable();

    /**
     * Emitted to indicate we have received some names from the device
     */
    Q_SCRIPTABLE void cachedPhonesAvailable();
};

#endif // CONTACTSPLUGIN_H
