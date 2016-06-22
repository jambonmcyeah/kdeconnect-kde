/**
 * Copyright 2013 Albert Vaca <albertvaka@gmail.com>
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

#ifndef TELEPHONYPLUGIN_H
#define TELEPHONYPLUGIN_H

#include <QLoggingCategory>

#include <core/kdeconnectplugin.h>
#include "kdeconnectinterface.h"

#define PACKAGE_TYPE_TELEPHONY_REQUEST QLatin1String("kdeconnect.telephony.request")
#define PACKAGE_TYPE_SMS_REQUEST QLatin1String("kdeconnect.sms.request")

Q_DECLARE_LOGGING_CATEGORY(KDECONNECT_PLUGIN_TELEPHONY)

class TelepathyPlugin
    : public KdeConnectPlugin
{
    Q_OBJECT

public:
    explicit TelepathyPlugin(QObject *parent, const QVariantList &args);
    void connected() override {}

public Q_SLOTS:
    bool receivePackage(const NetworkPackage& np) override;

private Q_SLOTS:
    void sendSms(const QString& phoneNumber, const QString& messageBody);

private:
    OrgFreedesktopTelepathyConnectionManagerKdeconnectInterface* m_telepathyInterface;
};

#endif