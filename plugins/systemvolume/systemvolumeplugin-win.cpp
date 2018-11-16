#include "systemvolumeplugin-win.h"
#include <core/device.h>

#include <KPluginFactory>

#include <QDebug>
#include <QLoggingCategory>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

K_PLUGIN_FACTORY_WITH_JSON( KdeConnectPluginFactory, "kdeconnect_systemvolume.json", registerPlugin< SystemvolumePlugin >(); )

Q_LOGGING_CATEGORY(KDECONNECT_PLUGIN_SYSTEMVOLUME, "kdeconnect.plugin.systemvolume")

SystemvolumePlugin::SystemvolumePlugin(QObject *parent, const QVariantList &args)
    : KdeConnectPlugin(parent, args),
      sinkList()
{
    CoInitialize(nullptr);
    deviceEnumerator = nullptr;
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_INPROC_SERVER, __uuidof(IMMDeviceEnumerator), (LPVOID *)&(deviceEnumerator));
    if (hr != S_OK)
    {
        throw hr;
    }
}

SystemvolumePlugin::~SystemvolumePlugin()
{
    deviceEnumerator->UnregisterEndpointNotificationCallback(deviceCallback);
    deviceEnumerator->Release();
    deviceEnumerator = nullptr;
    CoUninitialize();
}

void SystemvolumePlugin::sendSinkList()
{

    QJsonDocument document;
    QJsonArray array;

    HRESULT hr;
    if (!sinkList.empty())
    {
        for (auto const &sink : sinkList)
        {
            sink.first->UnregisterControlChangeNotify(sink.second);
            sink.first->Release();
            sink.second->Release();
        }
        sinkList.clear();
    }
    IMMDeviceCollection *devices = nullptr;
    hr = deviceEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &devices);

    if (hr != S_OK)
    {
        throw hr;
    }
    unsigned int deviceCount;
    devices->GetCount(&deviceCount);
    for (unsigned int i = 0; i < deviceCount; i++)
    {
        IMMDevice *device = nullptr;

        IPropertyStore *deviceProperties = nullptr;
        PROPVARIANT deviceProperty;
        QString name;
        QString desc;
        float volume;
        BOOL muted;

        IAudioEndpointVolume *endpoint = nullptr;
        CAudioEndpointVolumeCallback *callback;

        // Get Properties
        devices->Item(i, &device);
        device->OpenPropertyStore(STGM_READ, &deviceProperties);

        deviceProperties->GetValue(PKEY_Device_FriendlyName, &deviceProperty);
        name = QString::fromWCharArray(deviceProperty.pwszVal);
        //PropVariantClear(&deviceProperty);

        deviceProperties->GetValue(PKEY_Device_DeviceDesc, &deviceProperty);
        desc = QString::fromWCharArray(deviceProperty.pwszVal);
        //PropVariantClear(&deviceProperty);

        QJsonObject sinkObject;
        sinkObject.insert("name", name);
        sinkObject.insert("description", desc);

        hr = device->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, NULL, (void **)&endpoint);
        if (hr != S_OK)
        {
            qWarning() << QString::fromWCharArray(L"Warning: Failed to create IAudioEndpointVolume for ") << name;
            qWarning() << QStringLiteral("Code: ") << hr;
            continue;
        }
        endpoint->GetMasterVolumeLevelScalar(&volume);
        endpoint->GetMute(&muted);

        sinkObject.insert("muted", (bool)muted);
        sinkObject.insert("volume", qint64(volume * 100));
        sinkObject.insert("maxVolume", qint64(100));

        // Register Callback
        callback = new CAudioEndpointVolumeCallback(*this, name);
        sinkList[name] = qMakePair(endpoint, callback);
        endpoint->RegisterControlChangeNotify(callback);

        device->Release();
        array.append(sinkObject);
    }
    devices->Release();

    document.setArray(array);

    NetworkPacket np(PACKET_TYPE_SYSTEMVOLUME);
    np.set<QJsonDocument>(QStringLiteral("sinkList"), document);
    sendPacket(np);
}

void SystemvolumePlugin::connected()
{
    deviceCallback = new CMMNotificationClient(*this);
    deviceEnumerator->RegisterEndpointNotificationCallback(deviceCallback);
    sendSinkList();
}

bool SystemvolumePlugin::receivePacket(const NetworkPacket &np)
{
    if (np.has(QStringLiteral("requestSinks")))
    {
        sendSinkList();
    }
    else
    {

        QString name = np.get<QString>(QStringLiteral("name"));

        if (sinkList.contains(name))
        {
            if (np.has(QStringLiteral("volume")))
            {
                sinkList[name].first->SetMasterVolumeLevelScalar(float(np.get<int>(QStringLiteral("volume"))) / 100, NULL);
            }
            if (np.has(QStringLiteral("muted")))
            {
                sinkList[name].first->SetMute((BOOL)np.get<bool>(QStringLiteral("muted")), NULL);
            }
        }
    }
    return true;
}

// CMMNotificationClient

SystemvolumePlugin::CMMNotificationClient::CMMNotificationClient(SystemvolumePlugin &x) : enclosing(x), _cRef(1) {}

SystemvolumePlugin::CMMNotificationClient::~CMMNotificationClient() {}

// Callback methods for device-event notifications.

HRESULT STDMETHODCALLTYPE SystemvolumePlugin::CMMNotificationClient::OnDefaultDeviceChanged(EDataFlow flow, ERole role, LPCWSTR pwstrDeviceId)
{
    if (flow == eRender)
    {
        enclosing.sendSinkList();
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE SystemvolumePlugin::CMMNotificationClient::OnDeviceAdded(LPCWSTR pwstrDeviceId)
{
    enclosing.sendSinkList();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE SystemvolumePlugin::CMMNotificationClient::OnDeviceRemoved(LPCWSTR pwstrDeviceId)
{
    enclosing.sendSinkList();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE SystemvolumePlugin::CMMNotificationClient::OnDeviceStateChanged(LPCWSTR pwstrDeviceId, DWORD dwNewState)
{
    enclosing.sendSinkList();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE SystemvolumePlugin::CMMNotificationClient::OnPropertyValueChanged(LPCWSTR pwstrDeviceId, const PROPERTYKEY key)
{
    enclosing.sendSinkList();
    return S_OK;
}

// IUnknown methods -- AddRef, Release, and QueryInterface

ULONG STDMETHODCALLTYPE SystemvolumePlugin::CMMNotificationClient::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG STDMETHODCALLTYPE SystemvolumePlugin::CMMNotificationClient::Release()
{
    ULONG ulRef = InterlockedDecrement(&_cRef);
    if (0 == ulRef)
    {
        delete this;
    }
    return ulRef;
}

HRESULT STDMETHODCALLTYPE SystemvolumePlugin::CMMNotificationClient::QueryInterface(REFIID riid, VOID **ppvInterface)
{
    if (IID_IUnknown == riid)
    {
        AddRef();
        *ppvInterface = (IUnknown *)this;
    }
    else if (__uuidof(IMMNotificationClient) == riid)
    {
        AddRef();
        *ppvInterface = (IMMNotificationClient *)this;
    }
    else
    {
        *ppvInterface = NULL;
        return E_NOINTERFACE;
    }
    return S_OK;
}

// CAudioEndpointVolumeCallback

SystemvolumePlugin::CAudioEndpointVolumeCallback::CAudioEndpointVolumeCallback(SystemvolumePlugin &x, QString sinkName) : enclosing(x), name(sinkName), _cRef(1) {}

SystemvolumePlugin::CAudioEndpointVolumeCallback::~CAudioEndpointVolumeCallback() {}

// Callback method for endpoint-volume-change notifications.

HRESULT STDMETHODCALLTYPE SystemvolumePlugin::CAudioEndpointVolumeCallback::OnNotify(PAUDIO_VOLUME_NOTIFICATION_DATA pNotify)
{
    NetworkPacket np(PACKET_TYPE_SYSTEMVOLUME);
    np.set<int>(QStringLiteral("volume"), qint64(pNotify->fMasterVolume * 100));
    np.set<QString>(QStringLiteral("name"), name);
    enclosing.sendPacket(np);

    NetworkPacket np2(PACKET_TYPE_SYSTEMVOLUME);
    np2.set<bool>(QStringLiteral("muted"), (bool)pNotify->bMuted);
    np2.set<QString>(QStringLiteral("name"), name);
    enclosing.sendPacket(np2);

    return S_OK;
}

// IUnknown methods -- AddRef, Release, and QueryInterface

ULONG STDMETHODCALLTYPE SystemvolumePlugin::CAudioEndpointVolumeCallback::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG STDMETHODCALLTYPE SystemvolumePlugin::CAudioEndpointVolumeCallback::Release()
{
    ULONG ulRef = InterlockedDecrement(&_cRef);
    if (0 == ulRef)
    {
        delete this;
    }
    return ulRef;
}

HRESULT STDMETHODCALLTYPE SystemvolumePlugin::CAudioEndpointVolumeCallback::QueryInterface(REFIID riid, VOID **ppvInterface)
{
    if (IID_IUnknown == riid)
    {
        AddRef();
        *ppvInterface = (IUnknown *)this;
    }
    else if (__uuidof(IMMNotificationClient) == riid)
    {
        AddRef();
        *ppvInterface = (IMMNotificationClient *)this;
    }
    else
    {
        *ppvInterface = NULL;
        return E_NOINTERFACE;
    }
    return S_OK;
}

#include "systemvolumeplugin-win.moc"