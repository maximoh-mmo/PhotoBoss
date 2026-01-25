#pragma once
namespace photoboss {

    enum class PipelineState {
        Stopped,
        Starting,
        Idle,
        Scanning,
        Stopping
    };

}