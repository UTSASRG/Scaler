#include <grpc/ChannelPool.h>

std::shared_ptr<grpc::ChannelInterface> scaler::ChannelPool::channel=std::shared_ptr<grpc::ChannelInterface>();