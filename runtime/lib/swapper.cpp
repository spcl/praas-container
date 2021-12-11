
#include <chrono>

#include <boost/interprocess/streams/bufferstream.hpp>
#include <spdlog/spdlog.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/core/utils/stream/PreallocatedStreamBuf.h>

#include <praas/swapper.hpp>

namespace praas::swapper {

  bool S3Swapper::swap(std::string_view session_id, char* memory, int32_t size)
  {
    if(!_s3_client.has_value()) {
      spdlog::debug("Swapping of session {} not conducted - swapping is disabled");
      return false;
    }

    spdlog::debug("Swapping session {} - {} bytes starting at {}", session_id, size, fmt::ptr(memory));
    // This is stupid - Boost dependency because of an outdated AWS API.
    const std::shared_ptr<Aws::IOStream> input_data = std::make_shared<boost::interprocess::bufferstream>(memory, size);

    Aws::S3::Model::PutObjectRequest request;
    request.WithBucket(DEFAULT_S3_BUCKET).WithKey(Aws::String{session_id});
    request.SetBody(input_data);
    Aws::S3::Model::PutObjectOutcome outcome = _s3_client.value().PutObject(request);
    if (!outcome.IsSuccess()) {
      spdlog::error(
        "Swapping of session {} failed! Reason: {}",
        session_id, outcome.GetError().GetMessage()
      );
      return false;
    }
    return true;
  }

  bool S3Swapper::swap_in(std::string_view session_id, char* memory, int32_t size)
  {
    if(!_s3_client.has_value()) {
      spdlog::debug("Swapping of session {} not conducted - swapping is disabled");
      return false;
    }

    spdlog::debug("Swapping in session {} - write {} bytes at {}", session_id, size, fmt::ptr(memory));
    // This is stupid - Boost dependency because of an outdated AWS API.
    const std::shared_ptr<Aws::IOStream> data = std::make_shared<boost::interprocess::bufferstream>(memory, size);
    Aws::Utils::Stream::PreallocatedStreamBuf streambuf(reinterpret_cast<unsigned char*>(memory), size);

    Aws::S3::Model::GetObjectRequest request;
    request.WithBucket(DEFAULT_S3_BUCKET).WithKey(Aws::String{session_id});
    request.SetResponseStreamFactory([&streambuf]() { return Aws::New<Aws::IOStream>("", &streambuf); });

    Aws::S3::Model::GetObjectOutcome outcome = _s3_client.value().GetObject(request);
    if (!outcome.IsSuccess()) {
      spdlog::error(
        "Swapping in of session {} failed! Reason: {}",
        session_id, outcome.GetError().GetMessage()
      );
      return false;
    }
    return true;
  }

  bool S3Swapper::enable_swapping()
  {
    auto b = std::chrono::high_resolution_clock::now();
    // Initialize S3 backend
    Aws::InitAPI(_s3_options); 
    _s3_client.emplace();
    _enabled = true;
    auto e = std::chrono::high_resolution_clock::now();
    spdlog::debug("Initialized S3 swapper, time took {}", std::chrono::duration_cast<std::chrono::duration<double>>(e - b).count());
    // FIXME: catch errors
    return true;
  }

  void S3Swapper::shutdown_swapping()
  {
    if(_s3_client.has_value())
      _s3_client.reset();
    ShutdownAPI(_s3_options);
    _enabled = false;
  }

}
