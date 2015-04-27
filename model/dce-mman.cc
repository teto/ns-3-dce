#include "dce-stdlib.h"
#include "sys/dce-mman.h"
#include "ns3/log.h"


int dce_mlockall(int flags)
{
  //!
  NS_LOG_UNCOND("stub mlockall; not really implemented");
  return 0;
}

int dce_munlockall(void)
{
  //!
  NS_LOG_UNCOND("stub munlockall implementation");
  return 0;
}
