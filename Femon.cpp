/******************************************************************************
 * w_scan_cpp - a dtv channel scanner based on VDR (www.tvdr.de) and it's
 * Plugins.
 *
 * See the README file for copyright information and how to reach the author.
 *****************************************************************************/

#include "Helpers.h"
#include "Femon.h"

static const int strength_available = 0x01;
static const int cnr_available      = 0x02;
static const int ber_available      = 0x08;
static const int status_available   = 0x20;

int SignalMonitor(cDevice* Device, std::string& Channel) {
  if (Device == nullptr) {
     ErrorMessage("No Device");
     return -1;
     }

  cChannel aChannel;

  if (not aChannel.Parse(Channel.c_str())) {
     ErrorMessage("Invalid Channel: '" + Channel + "'");
     return -1;
     }

  if (*Device->DeviceName() != nullptr)
     Message("monitoring device '" + *Device->DeviceName() + "'");

  for(;;) {
     if (not Device->SwitchChannel(aChannel, false)) {
        Message("tuning failed - try again..");
        Sleep(1000);
        continue;
        }

     for(;;) {
        double SignalLevel_dBm, CNR_dB, BER;
        int result, DemodState, i;
        std::string s;

        if (not Device->SignalStats(&result, &SignalLevel_dBm, &CNR_dB, 0, &BER, 0, &DemodState))
           result = 0;

        bool HasLock = (result & status_available) ? (DemodState & 0x10) > 0 : Device->HasLock();

        s = "lock ";
        if (HasLock)
           s += "1 | ";
        else
           s += "0 | ";

        if ((result & strength_available) and (SignalLevel_dBm < 0))
           s += "signal " + std::to_string(SignalLevel_dBm + 108.75) + "dBuV | ";
        else if ((i = Device->SignalStrength()) > -1)
           s += "signal " + std::to_string(i) + "% | ";

        if ((i = Device->SignalQuality()) > -1)
           s += "quality " + std::to_string(i) + "% | ";

        if (result & cnr_available)
           s += "snr " + std::to_string(CNR_dB) + "dB | ";

        if (result & ber_available)
           s += "ber " + ExpToStr(BER);

        Message(s);
        Sleep(1000);
        }
     }

  return 0;
}
