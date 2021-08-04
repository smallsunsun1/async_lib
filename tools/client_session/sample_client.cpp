#include "tools/client_session/sample_client.h"

#include <glog/logging.h>

namespace sss {
namespace async {

std::string AsyncUnaryClient::RemoteCall(const std::string& request) {
  Request remote_request;
  remote_request.set_type(request);
  grpc::ClientContext context;
  Response result;
  grpc::Status status = stub_->RemoteCall(&context, remote_request, &result);
  if (status.ok()) {
    return result.type();
  } else {
    LOG(ERROR) << status.error_code() << " : " << status.error_message();
    return "RPC Failed!";
  }
}

void AsyncUnaryClient::AsyncRemoteCall(const std::string& request) {
  AsyncCallData* req_data = new AsyncCallData();
  Request req;
  req.set_type(request);
  req_data->reader =
      stub_->PrepareAsyncRemoteCall(&req_data->context, req, queue_.get());
  req_data->reader->StartCall();
  req_data->reader->Finish(&req_data->response, &req_data->status,
                           (void*)req_data);
}

void AsyncUnaryClient::HandleAsyncWork() {
  void* tag;
  bool ok;
  while (queue_->Next(&tag, &ok)) {
    AsyncCallData* data = static_cast<AsyncCallData*>(tag);
    if (!data->status.ok()) {
      std::cerr << "error: " << data->status.error_code() << "  "
                << data->status.error_message();
    }
    delete data;
  }
}

void AsyncStreamClient::StreamRemoteCall(Request req) {
  grpc::ClientContext context;
  std::unique_ptr<grpc::ClientReaderWriter<Request, Response>> reader_writer =
      stub_->RemoteStreamCall(&context);
  for (int i = 0; i < 100; ++i) {
    req.set_type(std::to_string(i));
    reader_writer->Write(req);
  }
  reader_writer->WritesDone();
  Response res;
  while (reader_writer->Read(&res)) {
    LOG(INFO) << res.type();
  }
  grpc::Status status = reader_writer->Finish();
  if (status.ok()) {
    LOG(INFO) << "ListFeatures rpc succeeded.";
  } else {
    LOG(INFO) << "ListFeatures rpc failed.";
  }
}

}  // namespace async
}  // namespace sss