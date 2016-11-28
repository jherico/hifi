#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QAudioDeviceInfo>
#include <QMediaPlayer>
#include <QMediaService>
#include <QAudioOutputSelectorControl>
#include <QDebug>
#include <QQmlContext>
#include <QThreadPool>
#include <QtWebEngine/qtwebengineglobal.h>

#include <functional>


QQmlApplicationEngine* enginePtr = nullptr;

template<typename T>
void forAllChildren(QObject* root, std::function<void(T*)> f) {
    for (auto child : root->findChildren<T*>()) {
        f(child);
    }
}

template<typename T>
void forAllChildren(QList<QObject*> roots, std::function<void(T*)> f) {
    for (auto root : roots) {
        forAllChildren<T>(root, f);
    }
}

class AudioHandler : public QObject, QRunnable {
    Q_OBJECT
    Q_PROPERTY(QStringList audioDevices READ getAudioDevices)

public:
    AudioHandler(QObject* parent = nullptr) : QObject(parent) {
        setAutoDelete(false);
        foreach(const QAudioDeviceInfo &deviceInfo, QAudioDeviceInfo::availableDevices(QAudio::AudioOutput)) {
            _audioDevices << deviceInfo.deviceName();
        }
    }

    const QStringList& getAudioDevices() {
        return _audioDevices;
    }

    virtual ~AudioHandler() {
        qDebug() << "Audio Handler Destroyed";
    }

    void run() override {
        static bool riftActive = false;
        QString device;
        QString targetDevice;
        {
            QMutexLocker locker(&_lock);
            targetDevice = _newTargetDevice;
        }
        if (targetDevice.isEmpty()) {
            return;
        }

        forAllChildren<QMediaPlayer>(enginePtr->rootObjects(), [&](QMediaPlayer* player) {
            QMediaService *svc = player->service();
            if (nullptr == svc) {
                return;
            }

            QAudioOutputSelectorControl *out = qobject_cast<QAudioOutputSelectorControl *>
                (svc->requestControl(QAudioOutputSelectorControl_iid));
            if (nullptr == out) {
                return;
            }

            if (device.isEmpty()) {
                for (auto output : out->availableOutputs()) {
                    QString description = out->outputDescription(output);
                    if (targetDevice == description) {
                        device = output;
                    }
                }
            }

            if (device.isEmpty()) {
                return;
            }

            out->setActiveOutput(device);
            svc->releaseControl(out);
        });
        riftActive = !riftActive;
    }

public slots:
    void changeAudioDevice(const QString& newDevice) {
        QObject* webEngineView = nullptr;
        for (auto root : enginePtr->rootObjects()) {
            webEngineView = root->findChild<QObject*>("web");
            if (webEngineView) {
                break;
            }
        }

        qDebug() << "Web " << webEngineView;
            
        QMutexLocker locker(&_lock);
        _newTargetDevice = newDevice;
        QThreadPool::globalInstance()->start(this);
    }

private:
    QStringList _audioDevices;

    QMutex _lock;
    QString _newTargetDevice;

};


int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QtWebEngine::initialize();
    QQmlApplicationEngine engine;
    enginePtr = &engine;
    engine.addImportPath("C:/Users/bdavis/Git/hifi/tests/qmlaudio/src");
    engine.rootContext()->setContextProperty("AudioHandler", new AudioHandler());
    engine.load(QUrl(QStringLiteral("file:///C:/Users/bdavis/Git/hifi/tests/qmlaudio/src/main.qml")));
    return app.exec();
}

#include "main.moc"