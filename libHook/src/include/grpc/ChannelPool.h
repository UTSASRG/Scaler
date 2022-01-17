
#ifndef SCALER_CHANNELPOOL_H
#define SCALER_CHANNELPOOL_H


#include <grpcpp/channel.h>

namespace scaler {

    class ChannelPool {
    public:
        static std::shared_ptr<grpc::ChannelInterface> channel;
    };

}


#endif //SCALER_CHANNELPOOL_H
