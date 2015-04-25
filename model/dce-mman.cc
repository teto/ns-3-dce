#include "dce-stdlib.h"
#include "sys/dce-mman.h"
#include "ns3/log.h"


int dce_mlockall(int flags)
{
  //!
  NS_LOG_UNCOND("mlockall");
}

int dce_munlockall(void)
{
  //!
  NS_LOG_UNCOND("munlockall");
}
