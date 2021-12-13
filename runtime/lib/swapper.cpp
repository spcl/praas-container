
#include <chrono>
#include <filesystem>
#include <fstream>

#include <boost/interprocess/streams/bufferstream.hpp>
#include <spdlog/spdlog.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/core/utils/stream/PreallocatedStreamBuf.h>

#include <praas/swapper.hpp>

namespace praas::swapper {

  void DiskSwapper::shutdown_swapping()
  {
    _enabled = false;
  }

  bool DiskSwapper::swap(std::string_view session_id, char* memory, int32_t size)
  {
    if(!_enabled) {
      spdlog::debug("Swapping of session {} not conducted - swapping is disabled");
      return false;
    }
    spdlog::debug("Swapping session {} - {} bytes starting at {}", session_id, size, fmt::ptr(memory));

    std::filesystem::path path;
    if(_user_location)
      path = _user_location;
    else
      path = DEFAULT_LOCATION;
    path /= session_id;
    std::ofstream fout;
    fout.open(path, std::ios::binary | std::ios::out);

    fout.write(memory, size);
    fout.close();

    return true;
  }

  bool DiskSwapper::swap_in(std::string_view session_id, char* memory, int32_t size)
  {
    return false;
  }

  bool DiskSwapper::enable_swapping()
  {
    _user_location = getenv("SWAPPING_DISK_LOCATION");
    _enabled = true;
    return true;
  }

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
    if(_user_s3_bucket)
      request.WithBucket(_user_s3_bucket).WithKey(Aws::String{session_id});
    else
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
    if(_user_s3_bucket)
      request.WithBucket(_user_s3_bucket).WithKey(Aws::String{session_id});
    else
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
    _user_s3_bucket = getenv("SWAPPING_S3_BUCKET");
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
