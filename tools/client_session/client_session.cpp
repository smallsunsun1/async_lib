#include <fcntl.h>
#include <glog/logging.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/text_format.h>
#include <grpc++/grpc++.h>
#include <grpc/grpc.h>
#include <stdio.h>
#include <stdlib.h>

#include <iostream>
#include <string>
#include <vector>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "async/context/host_context.h"
#include "tools/client_session/sample_client.h"
#include "tools/proto/config.pb.h"

ABSL_FLAG(std::string, config_file, "tools/data/config.pb.txt",
          "config filename for client session");

namespace sss {
namespace async {

class ClientSession {
 public:
  explicit ClientSession(const SessionConfig& config);
  void RunCalc(const Request& request);

 private:
  void RunStreamCalc(const Request& request);
  void RunUnaryCalc(const Request& request);

  SessionConfig config_;
  std::unique_ptr<HostContext> context_;
  std::vector<std::string> worker_addresses_;
  std::vector<AsyncUnaryClient> unary_clients_;
  std::vector<AsyncStreamClient> stream_clients_;
  std::atomic<uint32_t> index_;
  uint32_t total_clients_;
};

void ClientSession::RunStreamCalc(const Request& request) {
  uint32_t index = index_.fetch_add(1) % total_clients_;
  stream_clients_[index].StreamRemoteCall(request);
}

void ClientSession::RunUnaryCalc(const Request& request) {
  uint32_t index = index_.fetch_add(1) % total_clients_;
  unary_clients_[index].AsyncRemoteCall(request);
}

void ClientSession::RunCalc(const Request& request) {
  if (config_.client_config().type() == ClientConfig::kUnary) {
    RunUnaryCalc(request);
  } else if (config_.client_config().type() == ClientConfig::kStream) {
    RunStreamCalc(request);
  }
}

ClientSession::ClientSession(const SessionConfig& config) : config_(config) {
  grpc::ChannelArguments arguments;
  arguments.SetLoadBalancingPolicyName("grpclb");
  context_ = CreateCustomHostContext(
      config_.context_config().non_block_worker_threads(),
      config_.context_config().block_worker_threads());
  for (const std::string& address : config_.worker_addresses()) {
    worker_addresses_.push_back(address);
    if (config_.client_config().type() == ClientConfig::kUnary) {
      LOG(INFO) << "Build Unary Clients";
      unary_clients_.emplace_back(grpc::CreateCustomChannel(
          address, grpc::InsecureChannelCredentials(), arguments));
    } else if (config_.client_config().type() == ClientConfig::kStream) {
      LOG(INFO) << "Build Stream Clients";
      stream_clients_.emplace_back(grpc::CreateCustomChannel(
          address, grpc::InsecureChannelCredentials(), arguments));
    }
  }
  total_clients_ = config_.worker_addresses().size();
}

}  // namespace async
}  // namespace sss

int main(int argc, char* argv[]) {
  absl::ParseCommandLine(argc, argv);
  std::string config_file = absl::GetFlag(FLAGS_config_file);
  int file_descriptor = open(config_file.c_str(), O_RDONLY);
  if (file_descriptor < 0) {
    LOG(ERROR) << "config file not exist : " << config_file.c_str();
    return -1;
  }
  google::protobuf::io::FileInputStream in_stream(file_descriptor);
  sss::async::SessionConfig config;
  if (!google::protobuf::TextFormat::Parse(&in_stream, &config)) {
    LOG(ERROR) << "parse config file failed : " << config_file.c_str();
    return -1;
  }
  auto session = std::make_unique<sss::async::ClientSession>(config);
  sss::async::Request request;
  request.set_type("hello world");
  for (int i = 0; i < 2; ++i) {
    session->RunCalc(request);
  }
  return 0;
}
