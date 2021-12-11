
#ifndef __RUNTIME_SWAPPER_HPP__
#define __RUNTIME_SWAPPER_HPP__

#include <cstdint>
#include <optional>
#include <string>
#include <execinfo.h>
#include <climits>

#include <aws/s3/S3Client.h>
#include <aws/core/Aws.h>
#include <spdlog/spdlog.h>

namespace praas::swapper {

  struct S3Swapper
  {
    // AWS S3 configuration
    Aws::SDKOptions _s3_options;
    std::optional<Aws::S3::S3Client> _s3_client;
    bool _enabled;

    static constexpr char DEFAULT_S3_BUCKET[] = "praas-swapping";

    S3Swapper(bool verbose):
      _s3_client(std::optional<Aws::S3::S3Client>{}),
      _enabled(false)
    {
      if(verbose)
        _s3_options.loggingOptions.logLevel = Aws::Utils::Logging::LogLevel::Debug;
      else
        _s3_options.loggingOptions.logLevel = Aws::Utils::Logging::LogLevel::Info;
    }

    S3Swapper(S3Swapper && obj)
    {
      _s3_options = std::move(obj._s3_options);
      _s3_client = std::move(obj._s3_client);
      _enabled = obj._enabled;
      obj._enabled = false;
    }

    S3Swapper& operator=(S3Swapper && obj)
    {
      if(this != &obj) {
        _s3_options = std::move(obj._s3_options);
        _s3_client = std::move(obj._s3_client);
        _enabled = obj._enabled;
        obj._enabled = false;
      }
      return *this;
    }

    S3Swapper(const S3Swapper &) = delete;
    S3Swapper& operator=(const S3Swapper &) = delete;

    ~S3Swapper()
    {
      if(is_enabled())
        shutdown_swapping();
    }

    bool is_enabled() const
    {
      return _enabled;
    }

    // MUST be called before shutdown!
    void shutdown_swapping();
    bool swap(std::string_view session_id, char* memory, int32_t size);
    bool swap_in(std::string_view session_id, char* memory, int32_t size);
    bool enable_swapping();
  };

}

#endif
