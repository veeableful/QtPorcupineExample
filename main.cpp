#include <QCoreApplication>
#include <QAudioDeviceInfo>
#include <QAudioFormat>
#include <QAudioInput>
#include <QDebug>
#include <pv_porcupine.h>

// Change these to match your environment!
static const char MODEL_PATH[] = "porcupine_params.pv";
static const char KEYWORDS_PATH[] = "keywords.ppn";

static pv_porcupine_object_t *porcupineObject;
static QAudioInput *audioInput;
static QIODevice *ioDevice;
static QByteArray audioDataBuffer;

// Check if we have enough bytes of audio data for Porcupine to work with
bool hasEnoughSamples(int porcupineFrameLength)
{
    // We use porcupineFrameLength * 2 because frame length type is int16_t while audioDataBuffer's type is byte
    return audioDataBuffer.size() >= porcupineFrameLength * 2;
}

// Go through the audio data and detect the wake word if there's any
static void processSamples(int porcupineFrameLength)
{
    while (hasEnoughSamples(porcupineFrameLength)) {
        const int16_t *audioData = reinterpret_cast<int16_t*>(audioDataBuffer.data());
        bool detected;

        pv_porcupine_process(porcupineObject, &audioData[0], &detected);
        if (detected) {
            // Detected keywords. Do something!
            qDebug() << "Detected keywords!";
        }

        // Remove the audio data that we have processed
        audioDataBuffer.remove(0, porcupineFrameLength * 2);
    }
}

// Listen to the microphone's audio data and look for the keyword
static void listen(const QByteArray &audioData)
{
    const int porcupineFrameLength = pv_porcupine_frame_length();
    audioDataBuffer.append(audioData);
    processSamples(porcupineFrameLength);
}

// Setup Porcupine library
static void setupPorcupine()
{
    pv_status_t status = pv_porcupine_init(MODEL_PATH, KEYWORDS_PATH, 0.5, &porcupineObject);
    if (status != PV_STATUS_SUCCESS) {
        qDebug() << "Failed to initialize Porcupine";
        exit(2);
    }
}

// Setup microphone
static void setupMicrophone()
{
    QAudioFormat qtFormat;

    // Get default audio input device
    QAudioDeviceInfo qtInfo = QAudioDeviceInfo::defaultInputDevice();

    // Set the audio format settings
    qtFormat.setCodec("audio/pcm");
    qtFormat.setByteOrder(QAudioFormat::Endian::LittleEndian);
    qtFormat.setChannelCount(1);
    qtFormat.setSampleRate(16000);
    qtFormat.setSampleSize(16);
    qtFormat.setSampleType(QAudioFormat::SignedInt);

    // Check whether the format is supported
    if (!qtInfo.isFormatSupported(qtFormat)) {
        qWarning() << "Default format is not supported";
        exit(3);
    }

    // Instantiate QAudioInput with the settings
    audioInput = new QAudioInput(qtFormat);

    // Start receiving data from audio input
    ioDevice = audioInput->start();

    // Listen to the received data for wake words
    QObject::connect(ioDevice, &QIODevice::readyRead, [=] {
        listen(ioDevice->readAll());
    });
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    setupPorcupine();
    setupMicrophone();

    qDebug() << "Say the keywords! I'm listening.";

    return a.exec();
}
