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
 * signal strength hints
 ******************************************************************************/

std::string StrengthHint(cChannel& Channel, double Strength) {
  std::string s;
  double min = -1e9, max = 1e9;
  char src = Channel.Source() >> 24;
  int mod = cDvbTransponderParameters(Channel.Parameters()).Modulation();
  int sys = cDvbTransponderParameters(Channel.Parameters()).System();

  switch(src) {
     case 'A':
        break;
     case 'C': {
        switch(mod) {
           case 3:  /* M64  */ min = 47, max = 67; break;
           case 4:  /* M128 */ min = 51, max = 71; break;
           case 5:  /* M256 */ min = 54, max = 74; break;
           default:;
           }
        }
        break;
     case 'S':
        min = 47, max = 77; /* any */
        break;
     case 'T':
        if (sys == 0)
           switch(mod) {
              case 0:  /* M4   */ min = 33, max = 74; break;
              case 1:  /* M16  */ min = 36, max = 74; break;
              case 3:  /* M64  */ min = 45, max = 74; break;
              default:;
              }
        else
           switch(mod) {
              case 0:  /* M4   */ min = 33, max = 74; break;
              case 1:  /* M16  */ min = 35, max = 74; break;
              case 3:  /* M64  */ min = 39, max = 74; break;
              case 5:  /* M256 */ min = 43, max = 74; break;
              default:;
              }
        break;
     default:;
     }

  return Strength <  min      ? "(too low)"    :
         Strength >  max      ? "(too high)"   :
         Strength < (min + 6) ? "(a bit low)"  :
         Strength > (max -3)  ? "(a bit high)" :
         "";
}


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
        int result = 0, DemodState, i, SignalLevel_rel;
        bool HasLock;
        std::string s;

        if (not Device->SignalStats(result, &SignalLevel_dBm, &CNR_dB, nullptr, &BER, nullptr, &DemodState))
           result = 0;

        HasLock         = (result & status_available  ) ? (DemodState & 0x10) > 0 : Device->HasLock();
        SignalLevel_rel = (result & strength_available) ? 0                       : Device->SignalStrength();

        if (satip) {
           /* Bug in satip plugin, plugin reports "no signal" as -71.67dBm,
            * where it should be "no signal" (not to be reported),
            * see https://github.com/rofafor/vdr-plugin-satip/issues/80
            */
           if (SignalLevel_dBm < -71.5)
              result &= ~strength_available;

           /* Bug in satip plugin, plugin initializes tuners cached dBm value
            * to 0.0dBm (a very large value in terms of signal),
            * see https://github.com/rofafor/vdr-plugin-satip/issues/80
            */
           if (SignalLevel_dBm > -18.5)
              result &= ~strength_available;
           }

        if (HasLock)
           s = "lock 1 | ";
        else
           s = "lock 0 | ";

        if (result & strength_available)
           s += "signal " + FloatToStr(SignalLevel_dBm + 108.75, 2) + "dBuV " +
                StrengthHint(aChannel, SignalLevel_dBm + 108.75) + " | ";
        else
           if (SignalLevel_rel > -1)
              s += "signal " + IntToStr(SignalLevel_rel) + "% | ";

        /* w/o lock, demods may not report valid stats for
         * quality, cnr, ber
         */
        if (HasLock) {
           if ((i = Device->SignalQuality()) > -1)
              s += "quality " + IntToStr(i) + "% | ";

           if (result & cnr_available)
              s += "snr " + FloatToStr(CNR_dB, 2) + "dB | ";

           if (result & ber_available) {
              s += "ber " + ExpToStr(BER);
              if (BER > 1e-4)
                 s += " (too high)";
              }
           }

        Message(s);
        Sleep(1000);
        }

     delete data;
     }

  return 0;
}
