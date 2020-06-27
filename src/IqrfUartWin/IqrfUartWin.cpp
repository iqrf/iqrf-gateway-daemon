#define IIqrfChannelService_EXPORTS

#include <windows.h>
#include "IqrfUartWin.h"
#include "AccessControl.h"
#include "rapidjson/pointer.h"
#include <thread>
#include <mutex>
#include <memory>
#include <deque>

#ifdef TRC_CHANNEL
#undef TRC_CHANNEL
#endif
#define TRC_CHANNEL 0

#include "IIqrfChannelService.h"
#include "Trace.h"

#include "iqrf__IqrfUartWin.hxx"

TRC_INIT_MODULE(iqrf::IqrfUartWin);

namespace iqrf {

  class IqrfUartWin::Imp
  {
  public:

    Imp()
      :m_accessControl(this)
    {}

    ~Imp()
    {}

    void send(const std::basic_string<unsigned char>& message)
    {
      static int counter = 0;
      int attempt = 0;
      counter++;

      uint8_t packetCnt;
      uint8_t CRC;
      std::vector<uint8_t> sendVect;

      // initialize CRC
      uint8_t crc = 0xFF;
      // start of packet character
      sendVect.push_back(HDLC_FRM_FLAG_SEQUENCE);
      // send user data
      for (uint8_t bt : message) {
        writeByte(sendVect, bt, crc);
      }
      // send CRC
      writeByte(sendVect, crc, crc);
      // send stop of packet character
      sendVect.push_back(HDLC_FRM_FLAG_SEQUENCE);

      sendCommand(sendVect);
    }

    bool enterProgrammingState()
    {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION("NotImplemented");
      TRC_FUNCTION_LEAVE("");
      return false;
    }

    bool terminateProgrammingState() {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION("Not implemented");
      TRC_FUNCTION_LEAVE("");
      return false;
    }

    void startListen()
    {
      TRC_FUNCTION_ENTER("");

      resetMyEvent(m_readStartEvent);

      m_readMsgHandle = std::thread([&]()
      {
        readMsgThread();
      });

      // waiting for reading thread
      waitForMyEvent(m_readStartEvent, TM_START_READ);

      m_uartValid = true;

      TRC_FUNCTION_LEAVE("");
    }

    IIqrfChannelService::State getState() const
    {
      if (m_uartValid) {
        return IIqrfChannelService::State::Ready;
      }
      else {
        return IIqrfChannelService::State::NotReady;
      }
    }

    std::unique_ptr<IIqrfChannelService::Accessor>  getAccess(ReceiveFromFunc receiveFromFunc, AccesType access)
    {
      return m_accessControl.getAccess(receiveFromFunc, access);
    }

    bool hasExclusiveAccess() const
    {
      return m_accessControl.hasExclusiveAccess();
    }

    void activate(const shape::Properties *props)
    {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "******************************" << std::endl <<
        "IqrfUartWin instance activate" << std::endl <<
        "******************************"
      );

      try {
        modify(props);
      }
      catch (std::exception & e) {
        CATCH_EXC_TRC_WAR(std::exception, e, "UART failed: ");
      }

      //m_portHandle = openPort(m_interfaceName, m_baudRate);

      createMyEvent(m_readEndEvent);
      createMyEvent(m_readStartEvent);

      TRC_FUNCTION_LEAVE("")
    }

    void deactivate()
    {
      TRC_FUNCTION_ENTER("");

      setMyEvent(m_readEndEvent);

      if (m_readMsgHandle.joinable())
        m_readMsgHandle.join();

      destroyMyEvent(m_readStartEvent);
      destroyMyEvent(m_readEndEvent);

      closePort(m_portHandle);

      TRC_INFORMATION(std::endl <<
        "******************************" << std::endl <<
        "IqrfUartWin instance deactivate" << std::endl <<
        "******************************"
      );
      TRC_FUNCTION_LEAVE("")
    }

    void modify(const shape::Properties *props)
    {
      TRC_FUNCTION_ENTER("");

      using namespace rapidjson;

      Document d;
      d.CopyFrom(props->getAsJson(), d.GetAllocator());

      Value* comName = Pointer("/IqrfInterface").Get(d);
      if (comName != nullptr && comName->IsString()) {
        m_interfaceName = comName->GetString();
      }
      else {
        THROW_EXC_TRC_WAR(std::logic_error, "Cannot find property: /IqrfInterface");
      }

      Value* baudRate = Pointer("/baudRate").Get(d);
      if (baudRate != nullptr && baudRate->IsInt()) {
        m_baudRate = baudRate->GetInt();
      }
      else {
        THROW_EXC_TRC_WAR(std::logic_error, "Cannot find property: /baudRate");
      }

      TRC_INFORMATION(PAR(m_interfaceName) << PAR(m_baudRate))

      TRC_FUNCTION_LEAVE("");
    }

    IIqrfChannelService::osInfo getTrModuleInfo()
    {
      TRC_FUNCTION_ENTER("");
      TRC_WARNING("Reading TR module identification - not implemented.");

      IIqrfChannelService::osInfo myOsInfo;
      memset(&myOsInfo, 0, sizeof(myOsInfo));

      TRC_FUNCTION_LEAVE("");
      return myOsInfo;
    }

    IIqrfChannelService::UploadErrorCode upload(
      const UploadTarget target,
      const std::basic_string<uint8_t>& data,
      const uint16_t address
    )
    {
      TRC_FUNCTION_ENTER("");
      TRC_WARNING("Not implemented");
      TRC_FUNCTION_LEAVE("");
      return IIqrfChannelService::UploadErrorCode::UPLOAD_ERROR_NOT_SUPPORTED;
    }

  private:
    const uint8_t HDLC_FRM_FLAG_SEQUENCE = 0x7E;
    const uint8_t HDLC_FRM_CONTROL_ESCAPE = 0x7D;
    const uint8_t HDLC_FRM_ESCAPE_BIT = 0x20;

    bool m_uartValid = false;
    std::string m_interfaceName;
    int m_baudRate = 0;
    AccessControl<IqrfUartWin::Imp> m_accessControl;

    HANDLE m_portHandle = INVALID_HANDLE_VALUE;		// handle to COM-port

    std::thread m_readMsgHandle;

    // Signal for main thread, that read thread has started.
    HANDLE m_readStartEvent;
    // Signal for read thread to cancel.
    HANDLE m_readEndEvent;

    //OPERATION TIMEOUTS.
    // Starting read thread.
    const DWORD TM_START_READ = 5000;
    // Sending message to COM-port.
    const DWORD TM_SEND_MSG = 5000;
    // Waiting for a response.
    const DWORD TM_WAIT_RESP = 5000;


    //Appends COM-port prefix to the specified COM-port and returns it.
    LPTSTR getCompletePortName(LPCTSTR portName)
    {
      LPTSTR portPrefix = "\\\\.\\";

      int completeSize = lstrlen(portPrefix) + lstrlen(portName);
      LPTSTR completePortName = shape_new TCHAR[completeSize + 1];
      memset(completePortName, '\0', (completeSize + 1) * sizeof(TCHAR));
      if (lstrcat(completePortName, portPrefix) == NULL)
        return NULL;

      if (lstrcat(completePortName, portName) == NULL)
        return NULL;

      return completePortName;
    }

    /**
     * Compute the CRC8 value of a data set
     * @param inData One byte of data to compute CRC from
     * @param seed The starting value of the CRC
     * @return The CRC8 of inData with seed as initial value
     */
    uint8_t dpa_do_CRC8(uint8_t inData, uint8_t seed)
    {
      uint8_t bitsLeft;
      uint8_t seedTemp;

      for (bitsLeft = 8; bitsLeft > 0; bitsLeft--) {
        if (((seed ^ inData) & 0x01) == 0) {
          seed >>= 1;
        }
        else {
          seedTemp = (seed >>= 1);
          seedTemp ^= 0x8C;
          seed = seedTemp;
        }
        inData >>= 1;
      }
      return (seed);
    }

    void writeByte(std::vector<uint8_t> & dataBuffer, uint8_t dataByte, uint8_t & crc)
    {
      crc = dpa_do_CRC8(dataByte, crc);

      if (dataByte == HDLC_FRM_FLAG_SEQUENCE || dataByte == HDLC_FRM_CONTROL_ESCAPE) {
        dataBuffer.push_back(HDLC_FRM_CONTROL_ESCAPE);
        dataBuffer.push_back(dataByte ^ HDLC_FRM_ESCAPE_BIT);
      }
      else {
        dataBuffer.push_back(dataByte);
      }
    }

    // Configures and opens port for communication.
    HANDLE openPort(const std::string& portName, int baudRate)
    {
      TRC_FUNCTION_ENTER(PAR(portName) << PAR(baudRate));

      std::string portNameU(portName);
      if (portNameU.empty())
        portNameU = "COM1";
#ifdef _UNICODE
      LPCWSTR wcharCommPort = convertToWideChars(portNameU);
      if (wcharCommPort == L'\0')
        THROW_EX(std::logic_error, "Port name character conversion failed");
      portNameU = wcharCommPort;
      delete[] wcharCommPort;
#else
      portNameU = portName;
#endif

      LPTSTR completePortName = getCompletePortName(portNameU.c_str());
      if (completePortName == NULL)
        THROW_EXC_TRC_WAR(std::logic_error, "Complete port name creation failed: " << PAR(portName));

      HANDLE m_portHandle = CreateFile(completePortName,
        GENERIC_READ | GENERIC_WRITE, // read and write
        0,      //  must be opened with exclusive-access
        NULL,   //  default security attributes
        OPEN_EXISTING, //  must use OPEN_EXISTING
        FILE_FLAG_OVERLAPPED, // overlapped operation
        NULL); //   must be NULL for comm devices

      // Handle the error.
      if (m_portHandle == INVALID_HANDLE_VALUE)
        THROW_EXC_TRC_WAR(std::logic_error, "Port handle creation failed with error: " << GetLastError() << ' ' << PAR(portName));

      delete[] completePortName;

      DCB dcb;
      memset(&dcb, 0, sizeof(DCB));
      dcb.DCBlength = sizeof(DCB);

      BOOL getStateResult = GetCommState(m_portHandle, &dcb);
      if (!getStateResult)
        THROW_EXC_TRC_WAR(std::logic_error, "Port state getting failed with error: " << GetLastError() << PAR(portName));

      // set comm parameters
      dcb.BaudRate = m_baudRate;
      dcb.ByteSize = 8;
      dcb.Parity = NOPARITY;
      dcb.StopBits = ONESTOPBIT;

      BOOL setStateResult = SetCommState(m_portHandle, &dcb);
      if (!setStateResult)
        THROW_EXC_TRC_WAR(std::logic_error, "Port state getting failed with error: " << GetLastError() << PAR(portName));

      COMMTIMEOUTS timeouts;
      memset(&timeouts, 0, sizeof(COMMTIMEOUTS));

      BOOL getToutsResult = GetCommTimeouts(m_portHandle, &timeouts);
      if (!getToutsResult)
        THROW_EXC_TRC_WAR(std::logic_error, "Port timeouts getting failed with error: " << GetLastError() << PAR(portName));

      timeouts.ReadIntervalTimeout = 50;
      timeouts.ReadTotalTimeoutConstant = 50;
      timeouts.ReadTotalTimeoutMultiplier = 10;
      timeouts.WriteTotalTimeoutConstant = 50;
      timeouts.WriteTotalTimeoutMultiplier = 10;

      if (!SetCommTimeouts(m_portHandle, &timeouts))
        THROW_EXC_TRC_WAR(std::logic_error, "Port timeouts setting failed with error: " << GetLastError() << PAR(portName));

      TRC_FUNCTION_LEAVE(PAR(m_portHandle));
      return m_portHandle;
    }

    void closePort(HANDLE & m_portHandle)
    {
      CloseHandle(m_portHandle);
    }

     //	Function of reading thread of incoming COM port messages.
    void readMsgThread()
    {
      TRC_FUNCTION_ENTER("");

      DWORD eventFlags = EV_RXCHAR;

      std::deque<uint8_t> receivedBytes;

      DWORD bytesTotal = 0;
      unsigned char byteRead = '\0';

      BOOL fWaitingOnRead = FALSE;
      DWORD occurredEvent = 0;

      int count1 = 0;
      int count2 = 0;

      OVERLAPPED overlap;
      memset(&overlap, 0, sizeof(OVERLAPPED));

      try {
        m_portHandle = openPort(m_interfaceName, m_baudRate);

        // critical initialization setting - if it fails, cannot continue
        if (!SetCommMask(m_portHandle, eventFlags))
          THROW_EXC_TRC_WAR(std::logic_error, "SetCommMask failed with error: " << GetLastError());

        // critical initialization setting - if it fails, cannot continue
        overlap.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (overlap.hEvent == NULL)
          THROW_EXC_TRC_WAR(std::logic_error, "Create read char event failed with error: " << GetLastError());

        HANDLE waitEvents[2];
        waitEvents[0] = overlap.hEvent;
        waitEvents[1] = m_readEndEvent;

        // signal for main thread to start incoming user requests
        setMyEvent(m_readStartEvent);

        //READ_BEGIN:
        bool run = true;
        while (run) {
          //cout << "WaitCommEvent" << endl;
          DWORD waitEventResult = WaitCommEvent(m_portHandle, &occurredEvent, &overlap);

          if (!waitEventResult) {
            if (GetLastError() != ERROR_IO_PENDING) {
              THROW_EXC_TRC_WAR(std::logic_error, "Waiting for char event failed with error: " << GetLastError());
            }
            else {
              // Waiting for WaitCommEvent to finish
              //cout << "WaitCommEvent waiting to finish" << endl;

              // Wait a little while for an event to occur.
              //const DWORD READ_TIMEOUT = 500;
              DWORD waitResult = WaitForMultipleObjects(2, waitEvents, FALSE, INFINITE);
              switch (waitResult) {
                // Event occurred.
              case WAIT_OBJECT_0:
                if (!GetOverlappedResult(m_portHandle, &overlap, &bytesTotal, FALSE)) {
                  THROW_EXC_TRC_WAR(std::logic_error, "Waiting for char event failed with error: " << GetLastError());
                }
                else {
                  // WaitCommEvent returned.
                  // Deal with reading event as appropriate.
                  //cout << "WaitCommEvent returned in overlapped" << endl;
                }
                break;

              case (WAIT_OBJECT_0 + 1):
                //cout << "End reading thread..." << endl;
                run = false;

              case WAIT_TIMEOUT:
                // Operation isn't complete yet. fWaitingOnStatusHandle flag
                // isn't changed since I'll loop back around and I don't want
                // to issue another WaitCommEvent until the first one finishes.
                //
                // This is a good time to do some background work.
                break;

              default:
                // Error in the WaitForSingleObject; abort
                // This indicates a problem with the OVERLAPPED structure's
                // event handle.
                break;
              }
            }
          }
          else {
            // WaitCommEvent returned immediately.
            // Deal with reading event as appropriate.
            //cout << "WaitCommEvent returned immediately" << endl;
          }

          //reading loop
          do {
            if (!fWaitingOnRead) {

              // Issue read operation - overlapped - just once, must waiting for completion
              DWORD dwReadResult = ReadFile(m_portHandle, &byteRead, 1, &bytesTotal, &overlap);

              if (!dwReadResult) {
                // read not delayed?
                if (GetLastError() != ERROR_IO_PENDING) {
                  // Error in communications; report it.
                  THROW_EXC_TRC_WAR(std::logic_error, "Reading failed with error: " << GetLastError());
                }
                else {
                  //cout << "Waiting for reading..." << endl;

                  //cout << "Read result:" << dwReadResult << endl;
                  //cout << "Last error:" << GetLastError() << endl;
                  //cout << "Read byte:" << byteRead << endl;
                  //cout << "TotalBytes:" << bytesTotal << endl;

                  fWaitingOnRead = TRUE;
                }
              }
              else {
                // read completed immediately
                //cout << "Reading immediately:" << ++count1 << endl;

                //cout << "Read result:" << dwReadResult << endl;
                //cout << "Last error:" << GetLastError() << endl;
                //cout << "Read byte:" << byteRead << endl;
                //cout << "TotalBytes:" << bytesTotal << endl;

                receivedBytes.push_back(byteRead);

                if (byteRead == 0x0D) { //HDLC_FRM_FLAG_SEQUENCE
                  //basic_string<unsigned char>::iterator its;
                  //cout.setf(ios_base::hex, ios_base::basefield);
                  //cout.setf(ios_base::uppercase);
                  //for(its = receivedBytes.begin(); its != receivedBytes.end(); its++)
                  //	cout << setw(3) << (int) *its;
                  //cout << endl;
                  processAllMessages(receivedBytes);
                }

                // ready to read out another byte (bytesTotal) if available
                fWaitingOnRead = FALSE;
              }
            }

            if (fWaitingOnRead) {
              //cout << "WaitForMultipleObjects" << endl;

              //const DWORD READ_TIMEOUT = 500;
              DWORD waitResult = WaitForMultipleObjects(2, waitEvents, FALSE, INFINITE);
              switch (waitResult) {
                // Read completed.
              case WAIT_OBJECT_0:
                if (!GetOverlappedResult(m_portHandle, &overlap, &bytesTotal, FALSE)) {
                  THROW_EXC_TRC_WAR(std::logic_error, "Waiting for reading event failed with error: " << GetLastError());
                }
                else {
                  // Read completed successfully.
                  //cout << "Reading overlap:" << ++count2 << endl;
                  //cout << "TotalBytes:" << bytesTotal << endl;

                  if (bytesTotal != 0) {
                    //cout << "Read byte:" << byteRead << endl;
                    receivedBytes.push_back(byteRead);
                  }

                  if (byteRead == 0x7E) { //HDLC_FRM_FLAG_SEQUENCE
                    //basic_string<unsigned char>::iterator its;
                    //cout.setf(ios_base::hex, ios_base::basefield);
                    //cout.setf(ios_base::uppercase);
                    //for(its = receivedBytes.begin(); its != receivedBytes.end(); its++)
                    //	cout << setw(3) << (int) *its;
                    //cout << endl;
                    processAllMessages(receivedBytes);
                  }
                }
                //  Reset flag so that another opertion can be issued.
                fWaitingOnRead = FALSE;
                break;

              case (WAIT_OBJECT_0 + 1):
                //cout << "End reading thread..." << endl;
                run = false;

              case WAIT_TIMEOUT:
                // Operation isn't complete yet. fWaitingOnRead flag isn't
                // changed since I'll loop back around, and I don't want
                // to issue another read until the first one finishes.
                //
                // This is a good time to do some background work.
                //cout << "Reading timeouted..." << endl;
                break;

              default:
                // Error in the WaitForSingleObject; abort.
                // This indicates a problem with the OVERLAPPED structure's
                // event handle.
                THROW_EXC_TRC_WAR(std::logic_error, "Waiting for event in read cycle failed with error: " << GetLastError());
              }
            }
          } while (bytesTotal);
        }
      }
      //READ_ERROR:
      catch (std::exception &e) {
        CloseHandle(overlap.hEvent);
      }

      //READ_END:
      m_uartValid = false;
      CloseHandle(overlap.hEvent);

      TRC_FUNCTION_LEAVE("");
    }

    // Sends command stored in buffer to COM port.
    void sendCommand(std::vector<uint8_t> cmd)
    {
      if (!m_uartValid) {
        THROW_EXC_TRC_WAR(std::logic_error, "UART is not active: " << PAR(m_interfaceName));
      }

      OVERLAPPED overlap;
      memset(&overlap, 0, sizeof(OVERLAPPED));

      overlap.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
      if (overlap.hEvent == NULL)
        THROW_EXC_TRC_WAR(std::logic_error, "Creating send event failed with error: " << GetLastError());

      DWORD bytesWritten = 0;
      if (!WriteFile(m_portHandle, cmd.data(), cmd.size(), &bytesWritten, &overlap)) {
        if (GetLastError() != ERROR_IO_PENDING) {
          THROW_EXC_TRC_WAR(std::logic_error, "Sending message failed with error: " << GetLastError());
        }
        else {
          DWORD waitResult = WaitForSingleObject(overlap.hEvent, TM_SEND_MSG);
          switch (waitResult) {
          case WAIT_OBJECT_0:
            if (!GetOverlappedResult(m_portHandle, &overlap, &bytesWritten, FALSE)) {
              THROW_EXC_TRC_WAR(std::logic_error, "Waiting for send failed with error: " << GetLastError());
            }
            else {
              // Write operation completed successfully
            }
            break;

          case WAIT_TIMEOUT:
            THROW_EXC_TRC_WAR(std::logic_error, "Waiting for send timeouted: " << GetLastError());

          default:
            THROW_EXC_TRC_WAR(std::logic_error, "Waiting for send failed with error: " << GetLastError());
          }
        }
      }
      else {
        // Write operation completed successfully
      }

      CloseHandle(overlap.hEvent);
    }

    ///////////////////////////////////////////
    void setMyEvent(HANDLE evnt)
    {
      if (!SetEvent(evnt))
        THROW_EXC_TRC_WAR(std::logic_error, "Signaling a new message event failed with error: " << GetLastError());
    }

    void resetMyEvent(HANDLE evnt)
    {
      if (!ResetEvent(evnt))
        THROW_EXC_TRC_WAR(std::logic_error, "Reset start read event failed with error: " << GetLastError());
    }

    void createMyEvent(HANDLE & evnt)
    {
      evnt = CreateEvent(NULL, TRUE, FALSE, NULL);
      if (evnt == NULL)
        THROW_EXC_TRC_WAR(std::logic_error, "Create a new message event failed with error: " << GetLastError());
    }

    void destroyMyEvent(HANDLE & evnt)
    {
      CloseHandle(evnt);
    }

    /*
    * Waits for response of sent message.
    * @return WAIT_OBJECT_0 - response OK.
    *		    WAIT_TIMEOUT - waiting timeouted
    *			other value - error
    */
    DWORD waitForMyEvent(HANDLE evnt, DWORD timeout)
    {
      std::stringstream excStream;
      DWORD waitResult = WaitForSingleObject(evnt, timeout);
      switch (waitResult) {
      case WAIT_OBJECT_0:
        // OK
        break;
      case WAIT_TIMEOUT:
        THROW_EXC_TRC_WAR(std::logic_error, "Waiting for event timeouted: " << GetLastError());
      default:
        THROW_EXC_TRC_WAR(std::logic_error, "WaitForSingleObject failed with error: " << GetLastError());
      }

      return waitResult;
    }

    // Extracts and processes all messages inside the specified buffer.
    void processAllMessages(std::deque<uint8_t> & msgBuffer)
    {
      do {
        while (msgBuffer.size() > 0) {
          if (msgBuffer.front() == HDLC_FRM_FLAG_SEQUENCE) {
            msgBuffer.pop_front(); // drop leading separators
          }
          else {
            break;
          }
        }

        if (msgBuffer.size() == 0) {
          return; //msg or empty
        }

        std::vector<uint8_t> msg;
        std::vector<uint8_t> rollback;

        bool wasEscape = false;
        bool msgComplete = false;
        uint8_t crc = 0xFF;

        while (msgBuffer.size() > 0) {

          uint8_t bt = msgBuffer.front();
          msgBuffer.pop_front();

          rollback.push_back(bt);

          if (bt == HDLC_FRM_FLAG_SEQUENCE) {
            //end of msg
            msgComplete = true;
            msg.pop_back(); //remove ending crc
            break;
          }
          else {
            // discard received ESCAPE character
            if (bt == HDLC_FRM_CONTROL_ESCAPE) {
              wasEscape = true;
              continue;
            }

            // previous character was ESCAPE
            if (wasEscape) {
              wasEscape = 0;
              bt ^= HDLC_FRM_ESCAPE_BIT;
            }

            // add Rx byte to CRC
            crc = dpa_do_CRC8(bt, crc);

            msg.push_back(bt);
          }
        }

        if (msgComplete) {
          if (crc == 0 && msg.size() > 0) {
            //handle msg
            std::basic_string<unsigned char> message(msg.data(), msg.size());
            m_accessControl.messageHandler(message);
          }
          else {
            TRC_WARNING("garbage on IQRF iface: " << PAR(msg.size()) << PAR((int)crc));
          }
        }
        else {
          TRC_DEBUG("rollback of uncomplete msg: " << PAR(rollback.size()));
          msgBuffer.insert(msgBuffer.begin(), rollback.begin(), rollback.end());
          break;
        }
        rollback.clear();

      } while (msgBuffer.size() > 0); // continue wit other msg
    }

  };

  //////////////////////////////////////////////////
  IqrfUartWin::IqrfUartWin()
  {
    m_imp = shape_new Imp();
  }

  IqrfUartWin::~IqrfUartWin()
  {
    delete m_imp;
  }

  void IqrfUartWin::startListen()
  {
    m_imp->startListen();
  }

  IIqrfChannelService::State IqrfUartWin::getState() const
  {
    return m_imp->getState();
  }

  std::unique_ptr<IIqrfChannelService::Accessor>  IqrfUartWin::getAccess(ReceiveFromFunc receiveFromFunc, AccesType access)
  {
    return m_imp->getAccess(receiveFromFunc, access);
  }

  bool IqrfUartWin::hasExclusiveAccess() const
  {
    return m_imp->hasExclusiveAccess();
  }

  void IqrfUartWin::activate(const shape::Properties *props)
  {
    m_imp->activate(props);
  }

  void IqrfUartWin::deactivate()
  {
    m_imp->deactivate();
  }

  void IqrfUartWin::modify(const shape::Properties *props)
  {
    m_imp->modify(props);
  }

  void IqrfUartWin::attachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void IqrfUartWin::detachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().removeTracerService(iface);
  }
}
