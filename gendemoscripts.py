with open("src/demo.cc", "w") as f:
    f.write("""
  /* This file is generated automatically, where any changes can be overwritten. 
  \n
  #include <thread>
  #include <chrono>
  #include <iostream>
  #include <vector>
  #include <assert.h>
  #include <mutex>
  #include <unistd.h>
  #include <functional>
   
  #include "cryptowallet/validator.h"
  #include "cryptowallet/wallet.h"
  #include "config/settings.h"
  #include "bracha/node.h"
  #include "bracha/logging.h""" % "\n   ".join())
