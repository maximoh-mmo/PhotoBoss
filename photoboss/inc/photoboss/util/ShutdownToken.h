#pragma once
namespace photoboss
{
    class ShutdownToken {
    private:
        friend class PipelineController;
        ShutdownToken() = default;
    };
}