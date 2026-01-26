#pragma once
#include "Pipeline.h"

namespace photoboss {
    /// <summary>
	/// 
    /// A sink stage that consumes data from an input queue.
	/// will perform the given consume operation on each item.
    /// 
    /// </summary>
    /// <typeparam name="In"></typeparam>
    
    template<typename In>
    class Sink : public PipelineStage
    {
    public:
        explicit Sink(Queue<In>& input)
            : m_input(input) {
        }

        // derived classes must implement actual consume logic
        virtual void consume(const In& item) = 0;

        void Run() override {
            In item;
            while (m_input.wait_and_pop(item)) {
                consume(item);
            }
			m_input.shutdown(); // propagate shutdown to any other consumers
        }

    protected:
        Queue<In>& m_input;
    };
}