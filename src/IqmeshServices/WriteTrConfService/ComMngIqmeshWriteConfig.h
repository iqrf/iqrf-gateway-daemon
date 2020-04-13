#pragma once

#include "ComBase.h"
#include <vector>
#include "JsonUtils.h"

namespace iqrf {

  // WriteTrConf input paramaters
  typedef struct TWriteTrConfInputParams
  {
    TWriteTrConfInputParams()
    {
      std::memset( (void*)&embPers, 0, sizeof( embPers ) );
      std::memset( (void*)&dpaConfigBits, 0, sizeof( dpaConfigBits ) );
      std::memset( (void*)&RFPGM, 0, sizeof( RFPGM ) );
      std::memset( (void*)&rfSettings, -1, sizeof( rfSettings ) );
      security.accessPassword.clear();
      security.userKey.clear();
      repeat = 1;
      hwpId = HWPID_DoNotCheck;
      uartBaudRate = -1;
      restartNeeded = false;
    }

    // Embedded peripherals
    struct
    {
      bool coordinator;
      bool node;
      bool os;
      bool eeprom;
      bool eeeprom;
      bool ram;
      bool ledr;
      bool ledg;
      bool spi;
      bool io;
      bool thermometer;
      bool pwm;
      bool uart;
      bool frc;
      union
      {
        uint8_t maskBytes[PNUM_USER / 8];
        uint32_t mask;
      };
      union
      {
        uint8_t valueBytes[PNUM_USER / 8];
        uint32_t value;
      };
    }embPers;

    // DPA configuration bits
    struct
    {
      bool customDpaHandler;
      bool dpaPeerToPeer;
      bool nodeDpaInterface;
      bool dpaAutoexec;
      bool routingOff;
      bool ioSetup;
      bool peerToPeer;
      bool neverSleep;
      bool stdAndLpNetwork;
      uint8_t mask;
      uint8_t value;
    }dpaConfigBits;

    // RFPGM
    struct
    {
      bool rfPgmDualChannel;
      bool rfPgmLpMode;
      bool rfPgmEnableAfterReset;
      bool rfPgmTerminateAfter1Min;
      bool rfPgmTerminateMcuPin;
      uint8_t mask;
      uint8_t value;
    }RFPGM;

    // RF settings
    struct
    {
      int rfBand;
      int rfChannelA;
      int rfChannelB;
      int rfSubChannelA;
      int rfSubChannelB;
      int txPower;
      int rxFilter;
      int lpRxTimeout;
      int rfAltDsmChannel;
    }rfSettings;

    // Secutity
    struct
    {
      std::basic_string<uint8_t> accessPassword;
      std::basic_string<uint8_t> userKey;
    }security;

    int repeat;
    uint16_t deviceAddress;
    uint16_t hwpId;
    int uartBaudRate;
    bool restartNeeded;
  }TWriteTrConfInputParams;

  class ComMngIqmeshWriteConfig : public ComBase
  {
  public:
    ComMngIqmeshWriteConfig() = delete;
    ComMngIqmeshWriteConfig( rapidjson::Document& doc ) : ComBase( doc )
    {
      parse( doc );
    }

    virtual ~ComMngIqmeshWriteConfig()
    {
    }

    const TWriteTrConfInputParams getWriteTrConfParams() const
    {
      return m_writeTrConfParams;
    }

  protected:
    void createResponsePayload( rapidjson::Document& doc, const IDpaTransactionResult2& res ) override
    {
      rapidjson::Pointer( "/data/rsp/response" ).Set( doc, encodeBinary( res.getResponse().DpaPacket().Buffer, res.getResponse().GetLength() ) );
    }

  private:
    TWriteTrConfInputParams m_writeTrConfParams;

    // Parse document into structure TWriteTrConfInputParams 
    void parse( rapidjson::Document& doc )
    {
      rapidjson::Value* jsonVal;

      // Repeat
      if ( jsonVal = rapidjson::Pointer( "/data/repeat" ).Get( doc ) )
        m_writeTrConfParams.repeat = jsonVal->GetInt();

      // Device address
      if ( jsonVal = rapidjson::Pointer( "/data/req/deviceAddr" ).Get( doc ) )
        m_writeTrConfParams.deviceAddress = (uint16_t)jsonVal->GetInt();

      // HWPID
      if ( jsonVal = rapidjson::Pointer( "/data/req/hwpId" ).Get( doc ) )
        m_writeTrConfParams.hwpId = (uint16_t)jsonVal->GetInt();

      // embPers/coordinator
      if ( jsonVal = rapidjson::Pointer( "/data/req/embPers/coordinator" ).Get( doc ) )
      {
        m_writeTrConfParams.embPers.coordinator = jsonVal->GetBool();
        if ( m_writeTrConfParams.embPers.coordinator == true )
          m_writeTrConfParams.embPers.value |= ( 1 << PNUM_COORDINATOR );
        m_writeTrConfParams.embPers.mask |= ( 1 << PNUM_COORDINATOR );
      }

      // embPers/node
      if ( jsonVal = rapidjson::Pointer( "/data/req/embPers/node" ).Get( doc ) )
      {
        m_writeTrConfParams.embPers.node = jsonVal->GetBool();
        if ( m_writeTrConfParams.embPers.node == true )
          m_writeTrConfParams.embPers.value |= ( 1 << PNUM_NODE );
        m_writeTrConfParams.embPers.mask |= ( 1 << PNUM_NODE );
      }

      // embPers/os
      if ( jsonVal = rapidjson::Pointer( "/data/req/embPers/os" ).Get( doc ) )
      {
        m_writeTrConfParams.embPers.os = jsonVal->GetBool();
        if ( m_writeTrConfParams.embPers.os == true )
          m_writeTrConfParams.embPers.value |= ( 1 << PNUM_OS );
        m_writeTrConfParams.embPers.mask |= ( 1 << PNUM_OS );
      }

      // embPers/eeprom
      if ( jsonVal = rapidjson::Pointer( "/data/req/embPers/eeprom" ).Get( doc ) )
      {
        m_writeTrConfParams.embPers.eeprom = jsonVal->GetBool();
        if ( m_writeTrConfParams.embPers.eeprom == true )
          m_writeTrConfParams.embPers.value |= ( 1 << PNUM_EEPROM );
        m_writeTrConfParams.embPers.mask |= ( 1 << PNUM_EEPROM );
      }

      // embPers/eeeprom
      if ( jsonVal = rapidjson::Pointer( "/data/req/embPers/eeeprom" ).Get( doc ) )
      {
        m_writeTrConfParams.embPers.eeeprom = jsonVal->GetBool();
        if ( m_writeTrConfParams.embPers.eeeprom == true )
          m_writeTrConfParams.embPers.value |= ( 1 << PNUM_EEEPROM );
        m_writeTrConfParams.embPers.mask |= ( 1 << PNUM_EEEPROM );
      }

      // embPers/ram
      if ( jsonVal = rapidjson::Pointer( "/data/req/embPers/ram" ).Get( doc ) )
      {
        m_writeTrConfParams.embPers.ram = jsonVal->GetBool();
        if ( m_writeTrConfParams.embPers.ram == true )
          m_writeTrConfParams.embPers.value |= ( 1 << PNUM_RAM );
        m_writeTrConfParams.embPers.mask |= ( 1 << PNUM_RAM );
      }

      // embPers/ledr
      if ( jsonVal = rapidjson::Pointer( "/data/req/embPers/ledr" ).Get( doc ) )
      {
        m_writeTrConfParams.embPers.ledr = jsonVal->GetBool();
        if ( m_writeTrConfParams.embPers.ledr == true )
          m_writeTrConfParams.embPers.value |= ( 1 << PNUM_LEDR );
        m_writeTrConfParams.embPers.mask |= ( 1 << PNUM_LEDR );
      }

      // embPers/ledg
      if ( jsonVal = rapidjson::Pointer( "/data/req/embPers/ledg" ).Get( doc ) )
      {
        m_writeTrConfParams.embPers.ledg = jsonVal->GetBool();
        if ( m_writeTrConfParams.embPers.ledg == true )
          m_writeTrConfParams.embPers.value |= ( 1 << PNUM_LEDG );
        m_writeTrConfParams.embPers.mask |= ( 1 << PNUM_LEDG );
      }

      // embPers/spi
      if ( jsonVal = rapidjson::Pointer( "/data/req/embPers/spi" ).Get( doc ) )
      {
        m_writeTrConfParams.embPers.spi = jsonVal->GetBool();
        if ( m_writeTrConfParams.embPers.spi == true )
          m_writeTrConfParams.embPers.value |= ( 1 << PNUM_SPI );
        m_writeTrConfParams.embPers.mask |= ( 1 << PNUM_SPI );
      }

      // embPers/io
      if ( jsonVal = rapidjson::Pointer( "/data/req/embPers/io" ).Get( doc ) )
      {
        m_writeTrConfParams.embPers.io = jsonVal->GetBool();
        if ( m_writeTrConfParams.embPers.io == true )
          m_writeTrConfParams.embPers.value |= ( 1 << PNUM_IO );
        m_writeTrConfParams.embPers.mask |= ( 1 << PNUM_IO );
      }

      // thermometer
      if ( jsonVal = rapidjson::Pointer( "/data/req/embPers/thermometer" ).Get( doc ) )
      {
        m_writeTrConfParams.embPers.thermometer = jsonVal->GetBool();
        if ( m_writeTrConfParams.embPers.thermometer == true )
          m_writeTrConfParams.embPers.value |= ( 1 << PNUM_THERMOMETER );
        m_writeTrConfParams.embPers.mask |= ( 1 << PNUM_THERMOMETER );
      }

      // embPers/pwm
      if ( jsonVal = rapidjson::Pointer( "/data/req/embPers/pwm" ).Get( doc ) )
      {
        m_writeTrConfParams.embPers.pwm = jsonVal->GetBool();
        if ( m_writeTrConfParams.embPers.pwm == true )
          m_writeTrConfParams.embPers.value |= ( 1 << PNUM_PWM );
        m_writeTrConfParams.embPers.mask |= ( 1 << PNUM_PWM );
      }

      // embPers/uart
      if ( jsonVal = rapidjson::Pointer( "/data/req/embPers/uart" ).Get( doc ) )
      {
        m_writeTrConfParams.embPers.uart = jsonVal->GetBool();
        if ( m_writeTrConfParams.embPers.uart == true )
          m_writeTrConfParams.embPers.value |= ( 1 << PNUM_UART );
        m_writeTrConfParams.embPers.mask |= ( 1 << PNUM_UART );
      }

      // embPers/frc
      if ( jsonVal = rapidjson::Pointer( "/data/req/embPers/frc" ).Get( doc ) )
      {
        m_writeTrConfParams.embPers.frc = jsonVal->GetBool();
        if ( m_writeTrConfParams.embPers.frc == true )
          m_writeTrConfParams.embPers.value |= ( 1 << PNUM_FRC );
        m_writeTrConfParams.embPers.mask |= ( 1 << PNUM_FRC );
      }

      // rfChannelA
      if ( jsonVal = rapidjson::Pointer( "/data/req/rfChannelA" ).Get( doc ) )
        m_writeTrConfParams.rfSettings.rfChannelA = jsonVal->GetInt();

      // rfChannelB
      if ( jsonVal = rapidjson::Pointer( "/data/req/rfChannelB" ).Get( doc ) )
        m_writeTrConfParams.rfSettings.rfChannelB = jsonVal->GetInt();

      // rfSubChannelA
      if ( jsonVal = rapidjson::Pointer( "/data/req/rfSubChannelA" ).Get( doc ) )
        m_writeTrConfParams.rfSettings.rfSubChannelA = jsonVal->GetInt();

      // rfSubChannelB
      if ( jsonVal = rapidjson::Pointer( "/data/req/rfSubChannelB" ).Get( doc ) )
        m_writeTrConfParams.rfSettings.rfSubChannelB = jsonVal->GetInt();

      // txPower
      if ( jsonVal = rapidjson::Pointer( "/data/req/txPower" ).Get( doc ) )
      {
        m_writeTrConfParams.rfSettings.txPower = jsonVal->GetInt();
        if ( m_writeTrConfParams.rfSettings.txPower > 7 )
          m_writeTrConfParams.rfSettings.txPower = 7;
      }

      // rxFilter
      if ( jsonVal = rapidjson::Pointer( "/data/req/rxFilter" ).Get( doc ) )
      {
        int rxFilter = jsonVal->GetInt();
        if ( ( rxFilter >= 0 ) && ( rxFilter <= 64 ) )
          m_writeTrConfParams.rfSettings.rxFilter = rxFilter;
      }

      // lpRxTimeout
      if ( jsonVal = rapidjson::Pointer( "/data/req/lpRxTimeout" ).Get( doc ) )
      {
        int lpRxTimeout = jsonVal->GetInt();
        if ( ( lpRxTimeout >= 1 ) && ( lpRxTimeout <= 255 ) )
          m_writeTrConfParams.rfSettings.lpRxTimeout = lpRxTimeout;
      }

      // rfAltDsmChannel
      if ( jsonVal = rapidjson::Pointer( "/data/req/rfAltDsmChannel" ).Get( doc ) )
        m_writeTrConfParams.rfSettings.rfAltDsmChannel = jsonVal->GetInt();

      // uartBaudrate
      if ( jsonVal = rapidjson::Pointer( "/data/req/uartBaudrate" ).Get( doc ) )
      {
        int baudRate = jsonVal->GetInt();
        switch ( baudRate )
        {
          case 1200:
            m_writeTrConfParams.uartBaudRate = 0;
            break;

          case 2400:
            m_writeTrConfParams.uartBaudRate = 1;
            break;

          case 4800:
            m_writeTrConfParams.uartBaudRate = 2;
            break;

          case 9600:
            m_writeTrConfParams.uartBaudRate = 3;
            break;

          case 19200:
            m_writeTrConfParams.uartBaudRate = 4;
            break;

          case 38400:
            m_writeTrConfParams.uartBaudRate = 5;
            break;

          case 57600:
            m_writeTrConfParams.uartBaudRate = 6;
            break;

          case 115200:
            m_writeTrConfParams.uartBaudRate = 7;
            break;

          case 230400:
            m_writeTrConfParams.uartBaudRate = 8;
            break;

          default:
            m_writeTrConfParams.uartBaudRate = -1;
            break;
        }
      }

      // customDpaHandler
      if ( jsonVal = rapidjson::Pointer( "/data/req/customDpaHandler" ).Get( doc ) )
      {
        m_writeTrConfParams.dpaConfigBits.customDpaHandler = jsonVal->GetBool();
        if ( m_writeTrConfParams.dpaConfigBits.customDpaHandler == true )
          m_writeTrConfParams.dpaConfigBits.value |= 0x01;
        m_writeTrConfParams.dpaConfigBits.mask |= 0x01;
      }

      // nodeDpaInterface
      if ( jsonVal = rapidjson::Pointer( "/data/req/nodeDpaInterface" ).Get( doc ) )
        m_writeTrConfParams.dpaConfigBits.nodeDpaInterface = jsonVal->GetBool();

      // dpaPeerToPeer
      if ( jsonVal = rapidjson::Pointer( "/data/req/dpaPeerToPeer" ).Get( doc ) )
        m_writeTrConfParams.dpaConfigBits.dpaPeerToPeer = jsonVal->GetBool();

      // dpaAutoexec
      if ( jsonVal = rapidjson::Pointer( "/data/req/dpaAutoexec" ).Get( doc ) )
      {
        m_writeTrConfParams.dpaConfigBits.dpaAutoexec = jsonVal->GetBool();
        if ( m_writeTrConfParams.dpaConfigBits.dpaAutoexec == true )
          m_writeTrConfParams.dpaConfigBits.value |= 0x04;
        m_writeTrConfParams.dpaConfigBits.mask |= 0x04;
      }

      // routingOff
      if ( jsonVal = rapidjson::Pointer( "/data/req/routingOff" ).Get( doc ) )
      {
        m_writeTrConfParams.dpaConfigBits.routingOff = jsonVal->GetBool();
        if ( m_writeTrConfParams.dpaConfigBits.routingOff == true )
          m_writeTrConfParams.dpaConfigBits.value |= 0x08;
        m_writeTrConfParams.dpaConfigBits.mask |= 0x08;
      }

      // ioSetup
      if ( jsonVal = rapidjson::Pointer( "/data/req/ioSetup" ).Get( doc ) )
      {
        m_writeTrConfParams.dpaConfigBits.ioSetup = jsonVal->GetBool();
        if ( m_writeTrConfParams.dpaConfigBits.ioSetup == true )
          m_writeTrConfParams.dpaConfigBits.value |= 0x10;
        m_writeTrConfParams.dpaConfigBits.mask |= 0x10;
      }

      // peerToPeer
      if ( jsonVal = rapidjson::Pointer( "/data/req/peerToPeer" ).Get( doc ) )
      {
        m_writeTrConfParams.dpaConfigBits.peerToPeer = jsonVal->GetBool();
        if ( m_writeTrConfParams.dpaConfigBits.peerToPeer == true )
          m_writeTrConfParams.dpaConfigBits.value |= 0x20;
        m_writeTrConfParams.dpaConfigBits.mask |= 0x20;
      }

      // neverSleep
      if ( jsonVal = rapidjson::Pointer( "/data/req/neverSleep" ).Get( doc ) )
      {
        m_writeTrConfParams.dpaConfigBits.neverSleep = jsonVal->GetBool();
        if ( m_writeTrConfParams.dpaConfigBits.neverSleep == true )
          m_writeTrConfParams.dpaConfigBits.value |= 0x40;
        m_writeTrConfParams.dpaConfigBits.mask |= 0x40;
      }

      // stdAndLpNetwork
      if ( jsonVal = rapidjson::Pointer( "/data/req/stdAndLpNetwork" ).Get( doc ) )
      {
        m_writeTrConfParams.dpaConfigBits.stdAndLpNetwork = jsonVal->GetBool();
        if ( m_writeTrConfParams.dpaConfigBits.stdAndLpNetwork == true )
          m_writeTrConfParams.dpaConfigBits.value |= 0x80;
        m_writeTrConfParams.dpaConfigBits.mask |= 0x80;
      }

      // rfPgmDualChannel
      if ( jsonVal = rapidjson::Pointer( "/data/req/rfPgmDualChannel" ).Get( doc ) )
      {
        m_writeTrConfParams.RFPGM.rfPgmDualChannel = jsonVal->GetBool();
        if ( m_writeTrConfParams.RFPGM.rfPgmDualChannel == true )
          m_writeTrConfParams.RFPGM.value |= 0x03;
        m_writeTrConfParams.RFPGM.mask |= 0x03;
      }

      // rfPgmLpMode
      if ( jsonVal = rapidjson::Pointer( "/data/req/rfPgmLpMode" ).Get( doc ) )
      {
        m_writeTrConfParams.RFPGM.rfPgmLpMode = jsonVal->GetBool();
        if ( m_writeTrConfParams.RFPGM.rfPgmLpMode == true )
          m_writeTrConfParams.RFPGM.value |= 0x04;
        m_writeTrConfParams.RFPGM.mask |= 0x04;
      }

      // rfPgmEnableAfterReset
      if ( jsonVal = rapidjson::Pointer( "/data/req/rfPgmEnableAfterReset" ).Get( doc ) )
      {
        m_writeTrConfParams.RFPGM.rfPgmEnableAfterReset = jsonVal->GetBool();
        if ( m_writeTrConfParams.RFPGM.rfPgmEnableAfterReset == true )
          m_writeTrConfParams.RFPGM.value |= 0x10;
        m_writeTrConfParams.RFPGM.mask |= 0x10;
      }

      // rfPgmTerminateAfter1Min
      if ( jsonVal = rapidjson::Pointer( "/data/req/rfPgmTerminateAfter1Min" ).Get( doc ) )
      {
        m_writeTrConfParams.RFPGM.rfPgmTerminateAfter1Min = jsonVal->GetBool();
        if ( m_writeTrConfParams.RFPGM.rfPgmTerminateAfter1Min == true )
          m_writeTrConfParams.RFPGM.value |= 0x40;
        m_writeTrConfParams.RFPGM.mask |= 0x40;
      }

      // rfPgmTerminateMcuPin
      if ( jsonVal = rapidjson::Pointer( "/data/req/rfPgmTerminateMcuPin" ).Get( doc ) )
      {
        m_writeTrConfParams.RFPGM.rfPgmTerminateMcuPin = jsonVal->GetBool();
        if ( m_writeTrConfParams.RFPGM.rfPgmTerminateMcuPin == true )
          m_writeTrConfParams.RFPGM.value |= 0x80;
        m_writeTrConfParams.RFPGM.mask |= 0x80;
      }

      // fBand
      if ( jsonVal = rapidjson::Pointer( "/data/req/rfBand" ).Get( doc ) )
      {
        std::string rfBandStr = jsonVal->GetString();
        if ( rfBandStr == "433" )
          m_writeTrConfParams.rfSettings.rfBand = 433;
        if ( rfBandStr == "868" )
          m_writeTrConfParams.rfSettings.rfBand = 868;
        if ( rfBandStr == "916" )
          m_writeTrConfParams.rfSettings.rfBand = 916;
      }

      // accessPassword
      if ( jsonVal = rapidjson::Pointer( "/data/req/accessPassword" ).Get( doc ) )
      {
        std::string s = jsonVal->GetString();
        for ( int i = 0; i < 16; i++ )
          m_writeTrConfParams.security.accessPassword.push_back( 0x00 );
        size_t len = s.length() > 16 ? 16 : s.length();        
        for ( int i = 0; i < len; i++ )
          m_writeTrConfParams.security.accessPassword.push_back( s[i] );
      }

      // securityUserKey
      if ( jsonVal = rapidjson::Pointer( "/data/req/securityUserKey" ).Get( doc ) )
      {
        std::string s = jsonVal->GetString();
        for ( int i = 0; i < 16; i++ )
          m_writeTrConfParams.security.userKey.push_back( 0x00 );
        size_t len = s.length() > 16 ? 16 : s.length();
        for ( int i = 0; i < len; i++ )
          m_writeTrConfParams.security.userKey.push_back( s[i] );
      }
    }
  };
}
