
#ifndef SCALER_CHANNELPOOL_H
#define SCALER_CHANNELPOOL_H


#include <grpcpp/channel.h>
#include <util/tool/Logging.h>

namespace scaler {

    class ChannelPool {
    public:
        static std::shared_ptr<grpc::ChannelInterface> channel;
        ~ChannelPool(){
            INFO_LOG("Channel pool deconstructs");
        }
    };

}


#endif //SCALER_CHANNELPOOL_H
