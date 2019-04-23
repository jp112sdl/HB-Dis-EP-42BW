//- -----------------------------------------------------------------------------------------------------------------------
// AskSin++
// 2016-10-31 papa Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
// 2018-12-01 jp112sdl Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------

// use Arduino IDE Board Setting: STANDARD Layout

// define this to read the device id, serial and device type from bootloader section
// #define USE_OTA_BOOTLOADER
// #define USE_HW_SERIAL
// #define NDEBUG
// #define NDISPLAY
#define BATTERY_MODE

#ifdef BATTERY_MODE
#define USE_WOR
#endif





//////////////////// DISPLAY DEFINITIONS /////////////////////////////////////
#include <GxEPD.h>
#include <GxGDEW042T2/GxGDEW042T2.h>      // 4.2" b/w

#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxIO/GxIO.h>
#include "U8G2_FONTS_GFX.h"

#define GxRST_PIN  14   // PD6
#define GxBUSY_PIN 11   // PD3
#define GxDC_PIN   12   // PD4
#define GxCS_PIN   18   // PC2

GxIO_Class io(SPI, GxCS_PIN, GxDC_PIN, GxRST_PIN);
GxEPD_Class display(io, GxRST_PIN, GxBUSY_PIN);

U8G2_FONTS_GFX u8g2Fonts(display);
//////////////////////////////////////////////////////////////////////////////

#define EI_NOTEXTERNAL
#include <EnableInterrupt.h>

#include <AskSinPP.h>
#include <LowPower.h>

#include <Register.h>
#include <Remote.h>
#include <MultiChannelDevice.h>

#define CC1101_CS_PIN       4   // PB4
#define CC1101_GDO0_PIN     2   // PB2
#define CC1101_SCK_PIN      7   // PB7
#define CC1101_MOSI_PIN     5   // PB5
#define CC1101_MISO_PIN     6   // PB6
#define CONFIG_BUTTON_PIN  15   // PD7
#define LED_PIN_1           0   // PB0
#define LED_PIN_2           1   // PB1
#define BTN1_PIN            3   // PB3
#define BTN2_PIN           A1   // PA1
#define BTN3_PIN           A2   // PA2
#define BTN4_PIN           A3   // PA3
#define BTN5_PIN           A4   // PA4
#define BTN6_PIN           A5   // PA5
#define BTN7_PIN           A6   // PA6
#define BTN8_PIN           A7   // PA7
#define BTN9_PIN           23   // PC7
#define BTN10_PIN          22   // PC6

#define TEXT_LENGTH        16
#define DISPLAY_LINES      10

#define DISPLAY_ROTATE     3 // 0 = 0° , 1 = 90°, 2 = 180°, 3 = 270°

#define PEERS_PER_CHANNEL 8
#define NUM_CHANNELS      11
#define LOWBAT_VOLTAGE    24

#define MSG_START_KEY     0x02
#define MSG_TEXT_KEY      0x12
#define MSG_ICON_KEY      0x13
#define MSG_CLR_LINE_KEY  0xFE

#include "Icons.h"

// all library classes are placed in the namespace 'as'
using namespace as;

const struct DeviceInfo PROGMEM devinfo = {
  {0xf3, 0x43, 0x00},          // Device ID
  "JPDISEP000",                // Device Serial
#ifdef BATTERY_MODE
  {0xf3, 0x43},                // Device Model
#else
  {0xf3, 0x53},                // Device Model
#endif
  0x11,                        // Firmware Version
  as::DeviceType::Remote,      // Device Type
  {0x01, 0x01}                 // Info Bytes
};

enum Alignments {AlignRight = 0, AlignCenterIconRight = 1, AlignCenterIconLeft = 3, AlignLeft = 2};

typedef struct {
  uint8_t Alignment = AlignRight;
  uint8_t Icon      = 0xff;
  String Text       = "";
  bool showLine     = false;
} DisplayLine;
DisplayLine DisplayLines[DISPLAY_LINES];

String List1Texts[DISPLAY_LINES * 2];

bool runSetup          = true;

/**
   Configure the used hardware
*/
typedef AvrSPI<CC1101_CS_PIN, CC1101_MOSI_PIN, CC1101_MISO_PIN, CC1101_SCK_PIN> SPIType;
typedef Radio<SPIType, CC1101_GDO0_PIN> RadioType;
typedef StatusLed<LED_PIN_1> LedType;

#ifdef BATTERY_MODE
typedef AskSin<LedType, BatterySensor, RadioType> BaseHal;
#else
typedef AskSin<LedType, NoBattery, RadioType> BaseHal;
#endif

class Hal: public BaseHal {
  public:
    void init(const HMID& id) {
      BaseHal::init(id);
#ifdef BATTERY_MODE
      battery.init(seconds2ticks(60UL * 60 * 21), sysclock); //battery measure once an day
      battery.low(24);
      battery.critical(22);
      activity.stayAwake(seconds2ticks(15));
#endif
    }

    bool runready () {
      return sysclock.runready() || BaseHal::runready();
    }
} hal;

class ePaperWorkingLedType : public StatusLed<LED_PIN_2>  {
private:
  bool enabled;
public:
  ePaperWorkingLedType () : enabled(true) {}
  virtual ~ePaperWorkingLedType () {}
  void Enabled(bool e) {
    enabled = e;
  }
  bool Enabled() {
    return enabled;
  }
} ePaperWorkingLed;

void updateDisplay();
class ePaperType : public Alarm {
private:
  bool mustUpdateDisplay;
  bool inverted;
  uint16_t clFG;
  uint16_t clBG;
public:
  ePaperType () :  Alarm(0), mustUpdateDisplay(false), inverted(false), clFG(GxEPD_BLACK), clBG(GxEPD_WHITE)  {}
  virtual ~ePaperType () {}

  uint16_t ForegroundColor() {
    return clFG;
  }

  void ForegroundColor(uint16_t c) {
    clFG = c;
  }

  uint16_t BackgroundColor() {
    return clBG;
  }

  void BackgroundColor(uint16_t c) {
    clBG = c;
  }

  bool Inverted() {
    return inverted;
  }

  void Inverted(bool i) {
    inverted = i;
  }

  bool MustUpdateDisplay() {
    return mustUpdateDisplay;
  }

  void MustUpdateDisplay(bool m) {
    if (m == true && ePaperWorkingLed.Enabled() == true) ePaperWorkingLed.set(LedStates::pairing);
    mustUpdateDisplay = m;
  }

  void setRefreshAlarm (uint32_t t) {
    sysclock.cancel(*this);
    Alarm::set(millis2ticks(t));
    sysclock.add(*this);
  }
  virtual void trigger (__attribute__((unused)) AlarmClock& clock) {
    if (this->MustUpdateDisplay()) {
      this->MustUpdateDisplay(false);
  #ifndef NDISPLAY
      if (ePaperWorkingLed.Enabled() == true) {
        ePaperWorkingLed.set(LedStates::nothing);
        ePaperWorkingLed.ledOn();
      }
      display.drawPaged(updateDisplay);
      ePaperWorkingLed.ledOff();
  #else
      DPRINTLN("UPDATEDISPLAY!");
  #endif
    }
  }
} ePaper;

DEFREGISTER(Reg0, MASTERID_REGS, DREG_TRANSMITTRYMAX, DREG_LEDMODE, DREG_LOWBATLIMIT, 0x06, 0x07)
class DispList0 : public RegList0<Reg0> {
  public:
    DispList0(uint16_t addr) : RegList0<Reg0>(addr) {}

    bool displayInvertingHb(bool v) const { return this->writeRegister(0x06, 0x01,0,v); }
    bool displayInvertingHb() const { return this->readRegister(0x06, 0x01,0,false); }

    uint8_t displayRefreshWaitTime () const { return this->readRegister(0x07,0); }
    bool displayRefreshWaitTime (uint8_t value) const { return this->writeRegister(0x07,value); }

    void defaults () {
      clear();
      displayInvertingHb(false);
      ledMode(1);
      transmitDevTryMax(2);
      displayRefreshWaitTime(50);
#ifdef BATTERY_MODE
      lowBatLimit(24);
#endif
    }
};

DEFREGISTER(Reg1, CREG_AES_ACTIVE, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x90, 0x91)
class DispList1 : public RegList1<Reg1> {
  public:
    DispList1 (uint16_t addr) : RegList1<Reg1>(addr) {}

    bool showLine (uint8_t value) const {
      return this->writeRegister(0x90, 0x01, 0, value & 0xff);
    }
    bool showLine () const {
      return this->readRegister(0x90, 0x01, 0, false);
    }

    bool Alignment (uint8_t value) const {
      return this->writeRegister(0x91, value & 0xff);
    }
    uint8_t Alignment () const {
      return this->readRegister(0x91, 0);
    }

    bool TEXT1 (uint8_t value[TEXT_LENGTH]) const {
      for (int i = 0; i < TEXT_LENGTH; i++) {
        this->writeRegister(0x36 + i, value[i] & 0xff);
      }
      return true;
    }
    String TEXT1 () const {
      String a = "";
      for (int i = 0; i < TEXT_LENGTH; i++) {
        byte b = this->readRegister(0x36 + i, 0x20);
        if (b == 0x00) b = 0x20;
        a += char(b);
      }
      return a;
    }

    bool TEXT2 (uint8_t value[TEXT_LENGTH]) const {
      for (int i = 0; i < TEXT_LENGTH; i++) {
        this->writeRegister(0x46 + i, value[i] & 0xff);
      }
      return true;
    }
    String TEXT2 () const {
      String a = "";
      for (int i = 0; i < TEXT_LENGTH; i++) {
        byte b = this->readRegister(0x46 + i, 0x20);
        if (b == 0x00) b = 0x20;
        a += char(b);
      }
      return a;
    }

    void defaults () {
      clear();
      //aesActive(false);
      uint8_t initValues[TEXT_LENGTH];
      memset(initValues, 0x00, TEXT_LENGTH);
      TEXT1(initValues);
      TEXT2(initValues);
      showLine(false);
      Alignment(AlignRight);
    }
};

class DispChannel : public RemoteChannel<Hal,PEERS_PER_CHANNEL,DispList0, DispList1>  {
private:
  uint8_t       repeatcnt;
  volatile bool isr;
  uint8_t       commandIdx;
  uint8_t       command[224];
public:
  DispChannel () : RemoteChannel() , repeatcnt(0), isr(false), commandIdx(0) {}
    virtual ~DispChannel () {}

    void configChanged() {
      if (number() < NUM_CHANNELS) {
        List1Texts[(number() - 1)  * 2] = this->getList1().TEXT1();
        List1Texts[((number() - 1) * 2) + 1] = this->getList1().TEXT2();

        bool somethingChanged = (
                                  DisplayLines[(number() - 1)].showLine != this->getList1().showLine() ||
                                  DisplayLines[(number() - 1)].Alignment != this->getList1().Alignment()
                                );

        DisplayLines[(number() - 1)].showLine = this->getList1().showLine();
        DisplayLines[(number() - 1)].Alignment = this->getList1().Alignment();
        DPRINT(number() < 10 ? "0":"");DDEC(number()); DPRINT(F(" - TEXT1 = ")); DPRINT(this->getList1().TEXT1());DPRINT(F(" - TEXT2 = ")); DPRINT(this->getList1().TEXT2());DPRINT(F(" - Line  = ")); DDEC(this->getList1().showLine());DPRINT(F(" - Align = ")); DDECLN(this->getList1().Alignment());

        if (!runSetup && somethingChanged) ePaper.MustUpdateDisplay(true);
      }
    }

    bool process (const ActionCommandMsg& msg) {
      static bool getText = false;
      String Text = "";
      for (int i = 0; i < msg.len(); i++) {
        command[commandIdx] = msg.value(i);
        commandIdx++;
      }

      if (msg.eot(AS_ACTION_COMMAND_EOT)) {
        static uint8_t currentLine = 0;
        if (commandIdx > 0 && command[0] == MSG_START_KEY) {
          DPRINT("RECV: ");
          for (int i = 0; i < commandIdx; i++) {
            DHEX(command[i]); DPRINT(" ");

            if (command[i] == AS_ACTION_COMMAND_EOL) {
              if (Text != "") DisplayLines[currentLine].Text = Text;
              //DPRINT("EOL DETECTED. currentLine = ");DDECLN(currentLine);
              currentLine++;
              Text = "";
              getText = false;
            }

            if (getText == true) {
              if ((command[i] >= 0x20 && command[i] < 0x80) || command[i] == 0xb0 ) {
                char c = command[i];
                Text += c;
              } else if (command[i] >= 0x80 && command[i] < 0x80 + (DISPLAY_LINES * 2)) {
                uint8_t textNum = command[i] - 0x80;
                String fixText = List1Texts[textNum];
                fixText.trim();
                Text += fixText;
                //DPRINTLN(""); DPRINT("USE PRECONF TEXT NUMBER "); DDEC(textNum); DPRINT(" = "); DPRINTLN(List1Texts[textNum]);
              }
            }

            if (command[i] == MSG_TEXT_KEY) {
              getText = true;
              DisplayLines[currentLine].Icon = 0xff;  //clear icon
            }

            if (command[i] == MSG_ICON_KEY) {
              getText = false;
              DisplayLines[currentLine].Icon = command[i + 1] - 0x80;
            }

            if (command[i] == MSG_CLR_LINE_KEY) {
              getText = false;
              for (uint8_t i = 0; i < TEXT_LENGTH; i++)
                Text += F(" ");
              DisplayLines[currentLine].Icon = 0xff;
            }
          }
        }
        DPRINTLN("");
        currentLine = 0;
        commandIdx = 0;
        memset(command, 0, sizeof(command));

        for (int i = 0; i < DISPLAY_LINES; i++) {
         DPRINT("LINE "); DDEC(i + 1); DPRINT(" ICON = "); DDEC(DisplayLines[i].Icon); DPRINT(" TEXT = "); DPRINT(DisplayLines[i].Text); DPRINTLN("");
        }
        ePaper.MustUpdateDisplay(true);
      }

      return true;
    }

    bool process (const Message& msg) {
      return process(msg);
    }

    bool process (const RemoteEventMsg& msg) {
      return process(msg);
    }

    uint8_t flags () const {
      return hal.battery.low() ? 0x80 : 0x00;
    }
};

class DisplayDevice : public ChannelDevice<Hal, VirtBaseChannel<Hal, DispList0>, NUM_CHANNELS, DispList0> {
  public:
    VirtChannel<Hal, DispChannel, DispList0> c[NUM_CHANNELS];
  public:
    typedef ChannelDevice<Hal, VirtBaseChannel<Hal, DispList0>, NUM_CHANNELS, DispList0> DeviceType;
    DisplayDevice (const DeviceInfo& info, uint16_t addr) : DeviceType(info, addr) {
      for (uint8_t i = 0; i < NUM_CHANNELS; i++) {
        DeviceType::registerChannel(c[i], i+1);
      }
    }
    virtual ~DisplayDevice () {}

    DispChannel& dispChannel (uint8_t num)  {
      return c[num - 1];
    }

    bool process(Message& msg) {
      HMID devid;
      this->getDeviceID(devid);
      if (msg.to() == devid) {
        uint16_t rtime = this->getList0().displayRefreshWaitTime() * 100;
        ePaper.setRefreshAlarm(rtime);
      }
      return ChannelDevice::process(msg);
    }

    virtual void configChanged () {
      DPRINTLN(F("CONFIG LIST0 CHANGED"));

#ifdef BATTERY_MODE
      uint8_t lowbat = getList0().lowBatLimit();
      if( lowbat > 0 ) battery().low(lowbat);
      DPRINT(F("lowBat          : ")); DDECLN(lowbat);
#endif

      uint8_t ledmode = this->getList0().ledMode();
      DPRINT(F("ledMode         : ")); DDECLN(ledmode);
      ePaperWorkingLed.Enabled(ledmode);

      if (this->getList0().displayInvertingHb()) {
        ePaper.ForegroundColor(GxEPD_WHITE);
        ePaper.BackgroundColor(GxEPD_BLACK);
      } else {
        ePaper.ForegroundColor(GxEPD_BLACK);
        ePaper.BackgroundColor(GxEPD_WHITE);
      }
      bool invertChanged = (ePaper.Inverted() != this->getList0().displayInvertingHb());
      ePaper.Inverted(this->getList0().displayInvertingHb());
      DPRINT(F("displayInverting: ")); DDECLN(this->getList0().displayInvertingHb());

      DPRINT(F("RefreshWaitTime : ")); DDECLN(this->getList0().displayRefreshWaitTime());


      if (!runSetup && invertChanged) ePaper.MustUpdateDisplay(true);
    }
};
DisplayDevice sdev(devinfo, 0x20);
class ConfBtn : public ConfigButton<DisplayDevice>  {
public:
  ConfBtn (DisplayDevice& i) : ConfigButton(i)  {}
  virtual ~ConfBtn () {}

  virtual void state (uint8_t s) {
    if( s == ButtonType::longreleased ) {
      ePaper.MustUpdateDisplay(true);
      ePaper.setRefreshAlarm(20);
    }
    ConfigButton::state(s);
  }
};
ConfBtn cfgBtn(sdev);

void setup () {
  runSetup = true;
  for (int i = 0; i < DISPLAY_LINES; i++) {
    DisplayLines[i].Icon = 0xff;
    DisplayLines[i].Text = "";
  }

  initIcons();
#ifndef NDISPLAY
  display.init(57600);
#endif

  DINIT(57600, ASKSIN_PLUS_PLUS_IDENTIFIER);
  sdev.init(hal);
  remoteChannelISR(sdev.dispChannel(1),BTN1_PIN);
  remoteChannelISR(sdev.dispChannel(2),BTN2_PIN);
  remoteChannelISR(sdev.dispChannel(3),BTN3_PIN);
  remoteChannelISR(sdev.dispChannel(4),BTN4_PIN);
  remoteChannelISR(sdev.dispChannel(5),BTN5_PIN);
  remoteChannelISR(sdev.dispChannel(6),BTN6_PIN);
  remoteChannelISR(sdev.dispChannel(7),BTN7_PIN);
  remoteChannelISR(sdev.dispChannel(8),BTN8_PIN);
  remoteChannelISR(sdev.dispChannel(9),BTN9_PIN);
  remoteChannelISR(sdev.dispChannel(10),BTN10_PIN);

  buttonISR(cfgBtn, CONFIG_BUTTON_PIN);
  sdev.initDone();
  DDEVINFO(sdev);
  sdev.dispChannel(11).changed(true);
  ePaperWorkingLed.init();

#ifndef NDISPLAY
  u8g2Fonts.begin(display);
  ePaperWorkingLed.ledOn();
  display.drawPaged(showInitDisplay);
  ePaperWorkingLed.ledOff();
#endif
  runSetup = false;
}

void loop() {
  bool worked = hal.runready();
  bool poll = sdev.pollRadio();
  if ( worked == false && poll == false ) {
#ifdef BATTERY_MODE
    if (hal.battery.critical()) {
      hal.activity.sleepForever(hal);
    }
    hal.activity.savePower<Sleep<>>(hal);
#else
    hal.activity.savePower<Idle<>>(hal);
#endif
  }
}

uint16_t centerPosition(const char * text) {
  return (display.width() / 2) - (u8g2Fonts.getUTF8Width(text) / 2);
}

void showInitDisplay() {
  HMID devid;
  sdev.getDeviceID(devid);
  uint8_t serial[11];
  sdev.getDeviceSerial(serial);
  serial[10] = 0;
  initDisplay(serial);
}

void updateDisplay() {
  u8g2Fonts.setForegroundColor(ePaper.ForegroundColor());
  u8g2Fonts.setBackgroundColor(ePaper.BackgroundColor());
  display.fillScreen(ePaper.BackgroundColor());
  u8g2Fonts.setFont(u8g2_font_helvB18_tf);
  u8g2Fonts.setFontMode(1);

  for (uint16_t i = 0; i < 10; i++) {
    if (DisplayLines[i].showLine && i < 10) display.drawLine(0, ((i + 1) * 40), display.width(), ((i + 1) * 40), ePaper.ForegroundColor());
    DisplayLines[i].Text.trim();
    String viewText = DisplayLines[i].Text;
    viewText.replace("{", "ä");
    viewText.replace("|", "ö");
    viewText.replace("}", "ü");
    viewText.replace("[", "Ä");
    viewText.replace("#", "Ö");
    viewText.replace("$", "Ü");
    viewText.replace("~", "ß");
    viewText.replace("'", "=");

    uint8_t icon_number = DisplayLines[i].Icon;
    uint16_t icon_top = (24 - ( Icons[icon_number].height / 2)) + (i * 40) - 4;

    uint16_t leftTextPos = 0xffff;
    uint16_t fontWidth = u8g2Fonts.getUTF8Width(viewText.c_str());

    switch (DisplayLines[i].Alignment) {
      case AlignLeft:
        leftTextPos = 40;
        if (icon_number != 255) display.drawBitmap(Icons[icon_number].Icon, (( 24 - Icons[icon_number].width ) / 2) + 8, icon_top, Icons[icon_number].width, Icons[icon_number].height, ePaper.ForegroundColor(), GxEPD::bm_default);
        break;
      case AlignCenterIconRight:
        leftTextPos = (display.width() / 2) - (fontWidth / 2);
        if (icon_number != 255) {
          leftTextPos -= ((Icons[icon_number].width  / 2) + 4);
          display.drawBitmap(Icons[icon_number].Icon, leftTextPos + u8g2Fonts.getUTF8Width(viewText.c_str()) + 8 + (( 24 - Icons[icon_number].width ) / 2) , icon_top, Icons[icon_number].width, Icons[icon_number].height, ePaper.ForegroundColor(), GxEPD::bm_default);
        }
        break;
      case AlignCenterIconLeft:
        leftTextPos = (display.width() / 2) - (fontWidth / 2);
        if (icon_number != 255) {
          leftTextPos += ((Icons[icon_number].width  / 2) + 4);
          display.drawBitmap(Icons[icon_number].Icon, leftTextPos - Icons[icon_number].width - 8 , icon_top, Icons[icon_number].width, Icons[icon_number].height, ePaper.ForegroundColor(), GxEPD::bm_default);
        }
        break;
      case AlignRight:
      default:
        leftTextPos = display.width() - 40 - fontWidth;
        if (icon_number != 255) display.drawBitmap(Icons[icon_number].Icon, display.width() - 32 + (( 24 - Icons[icon_number].width ) / 2) , icon_top, Icons[icon_number].width, Icons[icon_number].height, ePaper.ForegroundColor(), GxEPD::bm_default);
        break;
    }

    u8g2Fonts.setCursor(leftTextPos, (i * 40) + 30);
    u8g2Fonts.print(viewText);
  }
}

void initDisplay(uint8_t serial[11]) {
  display.setRotation(DISPLAY_ROTATE);
  u8g2Fonts.setFontMode(1);
  u8g2Fonts.setFontDirection(0);
  u8g2Fonts.setForegroundColor(ePaper.ForegroundColor());
  u8g2Fonts.setBackgroundColor(ePaper.BackgroundColor());
  display.fillScreen(ePaper.BackgroundColor());


  const char * title        PROGMEM = "HB-Dis-EP-42BW";
  const char * asksinpp     PROGMEM = "AskSinPP";
  const char * version      PROGMEM = "V " ASKSIN_PLUS_PLUS_VERSION;
  const char * compiledMsg  PROGMEM = "compiled on";
  const char * compiledDate PROGMEM = __DATE__ " " __TIME__;
  const char * ser                  = (char*)serial;

  u8g2Fonts.setFont(u8g2_font_helvB24_tf);

  u8g2Fonts.setCursor(centerPosition(title), 95);
  u8g2Fonts.print(title);

  u8g2Fonts.setCursor(centerPosition(asksinpp), 170);
  u8g2Fonts.print(asksinpp);

  u8g2Fonts.setCursor(centerPosition(version), 210);
  u8g2Fonts.print(version);

  u8g2Fonts.setFont(u8g2_font_helvB12_tf);
  u8g2Fonts.setCursor(centerPosition(compiledMsg), 235);
  u8g2Fonts.print(compiledMsg);
  u8g2Fonts.setCursor(centerPosition(compiledDate), 255);
  u8g2Fonts.print(compiledDate);

  u8g2Fonts.setFont(u8g2_font_helvB18_tf);
  u8g2Fonts.setCursor(centerPosition(ser), 320);
  u8g2Fonts.print(ser);

  display.drawRect(60, 138, 180, 125, ePaper.ForegroundColor());
}
