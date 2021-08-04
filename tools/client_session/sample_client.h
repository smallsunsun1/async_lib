#ifndef TOOLS_CLIENT_SESSION_SAMPLE_CLIENT_
#define TOOLS_CLIENT_SESSION_SAMPLE_CLIENT_

#include <grpc++/grpc++.h>
#include <grpc/grpc.h>

#include <iostream>
#include <thread>

#include "tools/proto/message.grpc.pb.h"
#include "tools/proto/message.pb.h"

namespace sss {
namespace async {

class AsyncUnaryClient {
 public:
  explicit AsyncUnaryClient(std::shared_ptr<grpc::ChannelInterface> channel)
      : stub_(RpcWork::NewStub(channel)) {}
  std::string RemoteCall(const Request& request);
  void AsyncRemoteCall(const Request& request);
  void HandleAsyncWork();

 private:
  struct AsyncCallData {
    Response response;
    grpc::ClientContext context;
    grpc::Status status;
    std::unique_ptr<grpc::ClientAsyncResponseReader<Response>> reader;
  };
  std::unique_ptr<grpc::CompletionQueue> queue_;
  std::unique_ptr<RpcWork::Stub> stub_;
};

class AsyncStreamClient {
 public:
  explicit AsyncStreamClient(std::shared_ptr<grpc::ChannelInterface> channel)
      : stub_(RpcWork::NewStub(channel)) {}
  void StreamRemoteCall(Request req);

 private:
  std::unique_ptr<RpcWork::Stub> stub_;
};

}  // namespace async
}  // namespace sss

#endif /* TOOLS_CLIENT_SESSION_SAMPLE_CLIENT_ */
