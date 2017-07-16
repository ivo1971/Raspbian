/* Source: http://www.hertaville.com/introduction-to-accessing-the-raspberry-pis-gpio-in-c.html
 */
#include <fstream>
#include <iostream>
#include <signal.h>
#include <sstream>
#include <stdio.h>
#include <string>
#include <unistd.h>

namespace {
  class CGPIOPin {
  public:
                               CGPIOPin(const unsigned short usPin, const bool bIn);
                               ~CGPIOPin(void);
    
  public:
    void                       Set(const bool bOn) const;
    bool                       Get(void) const;
    
  private:
    void                       Configure(const std::string& strSysfs, const unsigned short usValue) const;
    void                       Configure(const std::string& strSysfs, const std::string& strValue) const;
    static std::string         Us2Str(const unsigned short value);
    
  private:
    const std::string          m_strPin;
    const std::string          m_strSysfsFileValue;
    const bool                 m_bIn;
    static const std::string   m_strSysfsGpioExport;
    static const std::string   m_strSysfsGpioUnexport;
    static const std::string   m_strSysfsGpioGpio;
    static const std::string   m_strSysfsGpioSubDirection;
    static const std::string   m_strSysfsGpioSubValue;
    static const std::string   m_strSysfsGpioValDirectionIn;
    static const std::string   m_strSysfsGpioValDirectionOut;
    static const std::string   m_strSysfsGpioValValueOn;
    static const std::string   m_strSysfsGpioValValueOff;
  };

  CGPIOPin::CGPIOPin(const unsigned short usPin, const bool bIn)
    : m_strPin(Us2Str(usPin))
    , m_strSysfsFileValue(m_strSysfsGpioGpio + m_strPin + m_strSysfsGpioSubValue)
    , m_bIn(bIn)
  {
    //trace
    std::cout << "Configure pin ["<< m_strPin.c_str() <<"] as [" << (bIn ? "input" : "output") << "]."<< std::endl;

    //configure pin as GPIO
    Configure(m_strSysfsGpioExport, m_strPin);
    
    //configure direction
    Configure(m_strSysfsGpioGpio + m_strPin + m_strSysfsGpioSubDirection, m_bIn ? m_strSysfsGpioValDirectionIn : m_strSysfsGpioValDirectionOut);
  }
  
  CGPIOPin::~CGPIOPin(void)
  {
    //trace
    std::cout << "Release pin ["<< m_strPin <<"]."<< std::endl;

    //avoid throwing exceptions from the destructor
    try {
      //set to 0
      if(!m_bIn) {
	Set(0);
      }

      //release pin
      Configure(m_strSysfsGpioUnexport, m_strPin);
    } catch(...) {
      //ignore
    }
  }
  
  void CGPIOPin::Set(const bool bOn) const
  {
    //trace
    std::cout << "Set pin ["<< m_strPin <<"] to [" << (bOn ? "on" : "off") << "]."<< std::endl;

    //sanity check
    if(m_bIn) {
      std::cerr << "OPERATION FAILED: Cannot call set on input pin ["<< m_strPin <<"]."<< std::endl;
      throw -1;
    }

    //set pin
    Configure(m_strSysfsFileValue, bOn ? m_strSysfsGpioValValueOn : m_strSysfsGpioValValueOff);
  }
  
  bool CGPIOPin::Get(void) const
  {
    //trace
    std::cout << "Get pin ["<< m_strPin <<"]."<< std::endl;

    //sanity check
    if(!m_bIn) {
      std::cerr << "OPERATION FAILED: Cannot call get on output pin ["<< m_strPin <<"]."<< std::endl;
      throw -1;
    }

    //get pin
    std::string value;
    {
      std::ifstream ifs(m_strSysfsFileValue.c_str());
      if(0 > ifs){
	std::cerr << "OPERATION FAILED: Unable to read ["<< m_strSysfsFileValue.c_str() <<"]."<< std::endl;
	throw -1;
      }
      ifs >> value;
      ifs.close();
    }

    //return pin value
    return m_strSysfsGpioValValueOn == value;
  }
  
  void CGPIOPin::Configure(const std::string& strSysfs, const unsigned short usValue) const
  {
    std::stringstream ss;
    ss << usValue;
    Configure(strSysfs, ss.str());
  }
  
  void CGPIOPin::Configure(const std::string& strSysfs, const std::string& strValue) const
  {
    std::cout << "Write ["<< strValue.c_str() <<"] to [" << strSysfs.c_str() << "]."<< std::endl;
    std::ofstream ofs(strSysfs.c_str());
    if(0 > ofs){
      std::cerr << "OPERATION FAILED: Unable to write ["<< strValue.c_str() <<"] to [" << strSysfs.c_str() << "]."<< std::endl;
      throw -1;
    }
    ofs << strValue.c_str();
    ofs.close();
  }
  
  //Unsigned short to string
  std::string CGPIOPin::Us2Str(const unsigned short value)
  {
    std::stringstream ssValue;
    ssValue << value;
    return std::string(ssValue.str());
  }
  
  const std::string CGPIOPin::m_strSysfsGpioExport          = "/sys/class/gpio/export";
  const std::string CGPIOPin::m_strSysfsGpioUnexport        = "/sys/class/gpio/unexport";
  const std::string CGPIOPin::m_strSysfsGpioGpio            = "/sys/class/gpio/gpio";
  const std::string CGPIOPin::m_strSysfsGpioSubDirection    = "/direction";
  const std::string CGPIOPin::m_strSysfsGpioSubValue        = "/value";
  const std::string CGPIOPin::m_strSysfsGpioValDirectionIn  = "in";
  const std::string CGPIOPin::m_strSysfsGpioValDirectionOut = "out";
  const std::string CGPIOPin::m_strSysfsGpioValValueOn      = "1";
  const std::string CGPIOPin::m_strSysfsGpioValValueOff     = "0";
};

namespace {
  bool gs_bContinue = true;

  void SigHandler(int /* iSig */)
  {
    const std::string msg("CTRL-C caught\n");
    write(0, msg.c_str(), msg.length());
    gs_bContinue = false;
  }

  void RegisterSignalHandlers(void)
  {
    struct sigaction sig;
    sig.sa_handler = SigHandler;
    sig.sa_flags = 0;
    sigemptyset(&sig.sa_mask);

    if(-1 == sigaction(SIGINT, &sig, NULL)) {
      std::cerr << "Problem with sigaction" << std::endl;
      throw -1;
    }
  }
};

int main(void)
{
  std::cout << "GPIO test in" << std::endl;

  //run test
  try {
    //register signal handlers
    RegisterSignalHandlers();

    //construct pin
    CGPIOPin out(5, false);

    //toggle loop
    {
      bool bOn = true;
      gs_bContinue = true;
      while(gs_bContinue) { //can be changed in signal handler
	//toggle pin
	out.Set(bOn);
	bOn = bOn ? false : true;

	//wait a bit
	usleep(500000);
      }
    }
  } catch(...) {
    std::cerr << "GPIO test failed" << std::endl;
  }

  std::cout << "GPIO test out" << std::endl;
  return 0;
}
