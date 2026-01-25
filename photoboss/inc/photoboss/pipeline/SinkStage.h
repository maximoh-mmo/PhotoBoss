#pragma once
namespace photoboss {
    template<typename In>
    class SinkStage : public PipelineStage 
    {
    public:
        explicit SinkStage(Queue<In>& input)
            : m_input(input) {
        }

        // derived classes must implement actual consume logic
        virtual void consume(const In& item) = 0;
        
        void Run() override {
            In item;
            while (m_input.wait_and_pop(item)) {
                consume(item);
            }
        }

    protected:
        Queue<In>& m_input;
    };
}