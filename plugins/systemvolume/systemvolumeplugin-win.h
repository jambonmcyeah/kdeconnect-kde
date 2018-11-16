#ifndef SYSTEMVOLUMEPLUGINWIN_H
#define SYSTEMVOLUMEPLUGINWIN_H

#include <QObject>
#include <QMap>

#include <core/kdeconnectplugin.h>

#include <Windows.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <Functiondiscoverykeys_devpkey.h>

#define PACKET_TYPE_SYSTEMVOLUME QStringLiteral("kdeconnect.systemvolume")
#define PACKET_TYPE_SYSTEMVOLUME_REQUEST QStringLiteral("kdeconnect.systemvolume.request")

class Q_DECL_EXPORT SystemvolumePlugin : public KdeConnectPlugin
{
    Q_OBJECT

  public:
    explicit SystemvolumePlugin(QObject *parent, const QVariantList &args);
    ~SystemvolumePlugin();
    bool receivePacket(const NetworkPacket& np) override;
    void connected() override;

  private:
    class CMMNotificationClient : public IMMNotificationClient
    {
        LONG _cRef;

      public:
        CMMNotificationClient(SystemvolumePlugin &x);

        ~CMMNotificationClient();

        // IUnknown methods -- AddRef, Release, and QueryInterface

        ULONG STDMETHODCALLTYPE AddRef() override;

        ULONG STDMETHODCALLTYPE Release() override;

        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, VOID **ppvInterface) override;

        // Callback methods for device-event notifications.

        HRESULT STDMETHODCALLTYPE OnDefaultDeviceChanged(EDataFlow flow, ERole role, LPCWSTR pwstrDeviceId) override;

        HRESULT STDMETHODCALLTYPE OnDeviceAdded(LPCWSTR pwstrDeviceId) override;

        HRESULT STDMETHODCALLTYPE OnDeviceRemoved(LPCWSTR pwstrDeviceId) override;

        HRESULT STDMETHODCALLTYPE OnDeviceStateChanged(LPCWSTR pwstrDeviceId, DWORD dwNewState) override;

        HRESULT STDMETHODCALLTYPE OnPropertyValueChanged(LPCWSTR pwstrDeviceId, const PROPERTYKEY key) override;

      private:
        SystemvolumePlugin &enclosing;
    };
    class CAudioEndpointVolumeCallback : public IAudioEndpointVolumeCallback
    {
        LONG _cRef;

      public:
        CAudioEndpointVolumeCallback(SystemvolumePlugin &x, QString sinkName);
        ~CAudioEndpointVolumeCallback();

        // IUnknown methods -- AddRef, Release, and QueryInterface

        ULONG STDMETHODCALLTYPE AddRef() override;

        ULONG STDMETHODCALLTYPE Release() override;

        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, VOID **ppvInterface) override;

        // Callback method for endpoint-volume-change notifications.

        HRESULT STDMETHODCALLTYPE OnNotify(PAUDIO_VOLUME_NOTIFICATION_DATA pNotify) override;

      private:
        SystemvolumePlugin &enclosing;
        QString name;
    };
    IMMDeviceEnumerator *deviceEnumerator;
    IMMNotificationClient *deviceCallback;
    QMap<QString, QPair<IAudioEndpointVolume *, CAudioEndpointVolumeCallback *>> sinkList;

    void sendSinkList();
};

#endif // SYSTEMVOLUMEPLUGINWIN_H
