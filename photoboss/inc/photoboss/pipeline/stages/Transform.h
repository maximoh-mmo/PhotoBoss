#pragma once
#include "Pipeline.h"

namespace photoboss {
    /// <summary>
    /// 
	/// Transform stage that consumes data from an input queue,
	/// performs a transformation, and produces data into an output queue.
    /// 
    /// </summary>
    /// <typeparam name="In"></typeparam>
    /// <typeparam name="Out"></typeparam>
    
    template<typename In, typename Out>
    class Transform : public PipelineStage
    {
    public:
        explicit Transform(Queue<In>& input, Queue<Out>& output, QObject* parent = nullptr)
            : PipelineStage(parent),
            m_input(input),
            m_output(output) {
        }
        // derived classes must implement actual transformation logic
        virtual Out transform(const In& item) = 0;

        void Run() override {
            In item;
            while (m_input.wait_and_pop(item)) {
                Out out_item = transform(item);
                m_output.push(std::move(out_item));
            }
            m_output.shutdown();
        }

    protected:
        Queue<In>& m_input;
        Queue<Out>& m_output;
    };
}