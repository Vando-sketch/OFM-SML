#include "SMLModule.h"

const std::string SMLModule::name()
{
    return "SML";
}

const std::string SMLModule::version()
{
    return MODULE_SML_Version;
}

void SMLModule::setup(bool configured)
{
    _led = openknx.ledFunctions.get(200);

    for (uint8_t i = 0; i < SML_ChannelCount; i++)
    {
        _channels[i] = new SMLChannel(i);
        _channels[i]->setup(configured);
    }
    logInfoP("Setup completed with %u channels", SML_ChannelCount);

    ledHelper(_led, false, _lastReceivedByte);
}

void SMLModule::loop(bool configured)
{
    uint8_t processed = 0;
    do
    {
        if (_channels[_currentChannel] != nullptr)
            _channels[_currentChannel]->loop(configured);
    }
    while (openknx.freeLoopIterate(SML_ChannelCount, _currentChannel, processed));

    loopStatus();
}

// Der Gesamtstatus wird aus den Kanälen abgeleitet: sobald einer empfängt, gilt das Modul
// als aktiv. Die Zeitschwelle liegt damit ausschließlich im Kanal und muss hier nicht
// zusätzlich gepflegt werden.
void SMLModule::loopStatus()
{
    if (_led == nullptr) return;

    bool receiveStatus = false;
    for (uint8_t i = 0; i < SML_ChannelCount; i++)
    {
        if (_channels[i] != nullptr && _channels[i]->receiveStatus())
        {
            receiveStatus = true;
            break;
        }
    }

    if (receiveStatus == _lastReceivedStatus) return;

    _lastReceivedStatus = receiveStatus;
    ledHelper(_led, receiveStatus, _lastReceivedByte);
}

#ifdef OPENKNX_DUALCORE
void SMLModule::setup1(bool configured)
{
    for (uint8_t i = 0; i < SML_ChannelCount; i++)
    {
        if (_channels[i] != nullptr)
            _channels[i]->setup1(configured);
    }
}
void SMLModule::loop1(bool configured)
{
    for (uint8_t i = 0; i < SML_ChannelCount; i++)
    {
        if (_channels[i] != nullptr)
            _channels[i]->loop1(configured);
    }
}
#endif

void SMLModule::processInputKo(GroupObject &ko)
{
}

SMLChannel *SMLModule::getChannel(uint8_t index)
{
    return _channels[index];
}

bool SMLModule::processCommand(const std::string command, bool diagnose)
{
    if (!diagnose && command == "sml debug")
    {
        _debug = !_debug;

        if (_debug)
            logInfoP("SML Debug enabled");
        else
            logInfoP("SML Debug disabled");

        return true;
    }
    else if (command.rfind("sml info") == 0 && command.length() == 10)
    {
        uint8_t channel = atoi(command.substr(8, 2).c_str());
        if (channel >= 1 && channel <= SML_ChannelCount && _channels[channel - 1] != nullptr)
        {
            // Das Diagnose-KO ist ein DPT 16 mit 14 Zeichen: dort nur der kurze Status. Auf der
            // Konsole steht davor, mit welchem Protokoll und welcher Schnittstelle der Kanal läuft.
            if (diagnose)
            {
                openknx.console.writeDiagnoseKo("%s", _channels[channel - 1]->diagnoseInfo().c_str());
            }
            else
            {
                const std::string serial = _channels[channel - 1]->serialInfo();
                if (serial.empty())
                    logInfoP("%s", _channels[channel - 1]->diagnoseInfo().c_str());
                else
                    logInfoP("%s %s", serial.c_str(), _channels[channel - 1]->diagnoseInfo().c_str());
            }
            return true;
        }
    }
    return false;
}

void SMLModule::showHelp()
{
    openknx.console.printHelpLine("sml debug", "Toggle SML debug logging");
    openknx.console.printHelpLine("sml infoNN", "Show status of SML channel NN (e.g. sml info01)");
}

bool SMLModule::debug()
{
    return _debug;
}

void SMLModule::ledHelper(OpenKNX::Led::FunctionGroup *_led, bool status, uint32_t &activity)
{
    if (status)
    {
        _led->color(OpenKNX::Led::Color::Green);
        _led->activity(activity, true);
    }
    else
    {
        _led->color(OpenKNX::Led::Color::Red);
        _led->activity(activity, true, OpenKNX::Led::Capability::COLOR);
        _led->activity(activity, false, OpenKNX::Led::Capability::MONOCHROME);
    }
}

SMLModule openknxSMLModule;