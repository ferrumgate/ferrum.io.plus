#ifndef __FERRUM_SOCKET_H__
#define __FERRUM_SOCKET_H__

#include "../common/common.h"
#include "../error/base_exception.h"
#include "../error/result.h"
#include "../log/logger.h"
#include "../memory/buffer.h"
#include "ferrum_addr.h"

namespace ferrum::io::net {

  struct FerrumShared {
    using Ptr = std::shared_ptr<FerrumShared>;
  };
  struct FerrumSocketOptions {};

  // zero copy buffer
  using BufferByte = memory::Buffer<std::byte>;  //  std::vector<std::byte>;
  // using Shared = std::shared_ptr<FerrumShared>;

}  // namespace ferrum::io::net

#endif