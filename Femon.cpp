/******************************************************************************
 * w_scan_cpp - a dtv channel scanner based on VDR (www.tvdr.de) and it's
 * Plugins.
 *
 * See the README file for copyright information and how to reach the author.
 *****************************************************************************/

#include "Helpers.h"
#include "Femon.h"
#include "CmdOpts.h"


/*******************************************************************************
 * class cData, a dummy receiver.
 ******************************************************************************/

class cData : public cReceiver, cThread {
private:
protected:
  virtual void Activate(bool On);
  virtual void Receive(const uchar* Data, int Length);
  virtual void Action(void);
public:
  cData();
  virtual ~cData() { cReceiver::Detach(); };
};

cData::cData() : cReceiver(NULL, 99), cThread("cData") { }

void cData::Receive(const uchar* Data, int Length) {
  (void)Data;
  (void)Length;
}

void cData::Action(void) {
  while(Running())
     Sleep(100);
};

void cData::Activate(bool On) {
  if (On) Start();
  else Cancel(0);
};



/*******************************************************************************
 * SignalMonitor(), print femon-like signal stats.
 ******************************************************************************/

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
     Message("monitoring device '" + std::string(*Device->DeviceName()) + "'");

  for(;;) {
     if (not Device->SwitchChannel(&aChannel, false)) {
        Message("tuning failed - try again..");
        Sleep(1000);
        continue;
        }

     cData* data = new cData();
     Device->AttachReceiver(data);

     for(;;) {
        double SignalLevel_dBm, CNR_dB, BER;
        int result = 0, DemodState, i;
        std::string s;

        if (not Device->SignalStats(result, &SignalLevel_dBm, &CNR_dB, nullptr, &BER, nullptr, &DemodState))
           result = 0;

        bool HasLock = (result & status_available) ? (DemodState & 0x10) > 0 : Device->HasLock();

        if ((satip != nullptr) and ((SignalLevel_dBm >= 0) or (SignalLevel_dBm < -71.5)))
           result &= ~strength_available;

        s = "lock ";
        if (HasLock)
           s += "1 | ";
        else
           s += "0 | ";

        if (result & strength_available)
           s += "signal " + FloatToStr(SignalLevel_dBm + 108.75, 2) + "dBuV | ";
        else if (HasLock) {
           i = Device->SignalStrength();
           if (i >= 0 and i <= 100)
              s += "signal " + std::to_string(i) + "% | ";
           }

        if ((i = Device->SignalQuality()) > -1)
           s += "quality " + std::to_string(i) + "% | ";

        if (result & cnr_available)
           s += "snr " + FloatToStr(CNR_dB, 2) + "dB | ";

        if (result & ber_available)
           s += "ber " + ExpToStr(BER);

        Message("\e[A");
        Message(s);
        Sleep(1000);
        }

     delete data;
     }

  return 0;
}
