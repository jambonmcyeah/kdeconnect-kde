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
 * Response indicating the package contains a list of contact email addresses
 *
 * It shall contain the key "uids", which will mark a list of uIDs (long int, as string)
 * then, for each UID, there shall be a 3-field list containing the email address, the type, and the label
 *
 * For now, the values in types are undefined, but coincidentally match the list here:
 * https://developer.android.com/reference/android/provider/ContactsContract.CommonDataKinds.Email.html
 *
 * The label field is defined to be the custom label if the number is a custom type, otherwise the empty string
 *
 * For example:
 * ( 'uids' : ['1', '3', '15'],
 *  '1'  : [ [ 'john@example.com',  '2', '' ] ]
 *  '3'  : [ [ 'abel@example.com', '0', 'Priority' ] ] // This email address has a custom type
 *  '15' : [ [ 'mom@example.com', '1', '' ] ] )
 */
#define PACKAGE_TYPE_CONTACTS_RESPONSE_EMAILS QStringLiteral("kdeconnect.contacts.response_emails")

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

// Email and Phone entries turn out to want the same fields
typedef PhoneEntry EmailEntry;

typedef QList<EmailEntry> EmailEntryList_t;

typedef QHash<uID_t, EmailEntryList_t> EmailCache_t;

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

    /**
     * Reply with pairs of values, connecting uIDs to EmailEntries
     */
    Q_SCRIPTABLE PhoneCache_t getEmailsByUIDs(uIDList_t);

public: Q_SIGNALS:
    /**
     * Emitted to indicate we have received some contacts' uIDs from the device
     */
    Q_SCRIPTABLE void cachedUIDsAvailable();

    /**
     * Emitted to indicate we have received some names from the device
     */
    Q_SCRIPTABLE void cachedNamesAvailable();

    /**
     * Emitted to indicate we have received some phone numbers from the device
     */
    Q_SCRIPTABLE void cachedPhonesAvailable();

    /**
     * Emitted to indicate we have received some email addresses from the device
     */
    Q_SCRIPTABLE void cachedEmailsAvailable();

protected:

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
     * Store the mapping of locally-known uIDs mapping to names
     */
    EmailCache_t emailsCache;

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
     * Enforce mutual exclusion when accessing the cached phone numbers
     */
    QMutex emailsCacheLock;

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
     *  Handle a packet of type PACKAGE_TYPE_CONTACTS_RESPONSE_PHONES
     */
    bool handleResponseEmails(const NetworkPackage&);

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
     * Get the locally-known collection of uID -> EmailEntry mapping
     *
     * If the cache has not yet been populated, populate it first
     *
     * @param uIDs List of IDs for which to fetch mappings
     * @return Locally-cached contacts' uID -> EmailEntry mapping
     */
    PhoneCache_t getCachedEmailsForIDs(uIDList_t uIDs);

    /**
     * Send a request-type packet, which contains no body
     *
     * @return True if the send was successful, false otherwise
     */
    bool sendRequest(QString packageType);

    /**
     * Send a request-type packet which has a body with the key 'uids' and the value the list of
     * specified uIDs
     *
     * @param packageType Type of package to send
     * @param uIDs List of uIDs to request
     * @return True if the send was successful, false otherwise
     */
    bool sendRequestWithIDs(QString packageType, uIDList_t uIDs);
};

#endif // CONTACTSPLUGIN_H
