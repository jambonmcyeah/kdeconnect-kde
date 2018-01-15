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

#ifndef PLUGINS_CONTACTS_PHONEENTRY_H_
#define PLUGINS_CONTACTS_PHONEENTRY_H_

#include <QtDBus>
#include <QVariant>

/**
 * A PhoneEntry defines what fields are stored with a phone number
 */
class PhoneEntry
{
public:
    PhoneEntry ();
    PhoneEntry (QString number, int type, QString label);
    PhoneEntry (const PhoneEntry &other);

    /**
     * Stores the phone number as it is in the device's contacts book
     */
    QString number;

    /**
     * Represents the type of number, such as home, mobile, work, etc.
     *
     * The values of this field are currently undefined
     */
    int type;

    /**
     * Stores the label of the type of this number if it is a custom type
     */
    QString label;

    friend QDBusArgument &operator<<(QDBusArgument &argument, const PhoneEntry &entry);
    friend const QDBusArgument &operator>>(const QDBusArgument &argument, PhoneEntry &entry);

    /**
     * Register with the Qt type system
     *
     * Should be called sometime while initializing dbus
     */
    static void registerMetaType();
};

Q_DECLARE_METATYPE(PhoneEntry)

#endif /* PLUGINS_CONTACTS_PHONEENTRY_H_ */
