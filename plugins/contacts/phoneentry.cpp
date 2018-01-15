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

#include <phoneentry.h>
#include <QtDBus>
#include <QVariant>

PhoneEntry::PhoneEntry ()
{
    this->type = 0;
}

PhoneEntry::PhoneEntry(const PhoneEntry &other)
{
    this->number = other.number;
    this->type = other.type;
    this->label = other.label;
}

PhoneEntry::PhoneEntry (QString number, int type, QString label)
{
    this->number = number;
    this->type = type;
    this->label = label;
}

void PhoneEntry::registerMetaType()
{
    qRegisterMetaType<PhoneEntry>("PhoneEntry");
    qDBusRegisterMetaType<PhoneEntry>();
}

QDBusArgument &operator<<(QDBusArgument &argument, const PhoneEntry& entry)
{
    argument.beginStructure();
    argument << entry.number;
    argument << entry.type;
    argument << entry.label;
    argument.endStructure();

    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, PhoneEntry &entry)
{
    argument.beginStructure();
    argument >> entry.number;
    argument >> entry.type;
    argument >> entry.label;
    argument.endStructure();

    return argument;
}
