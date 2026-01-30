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
    class Transform : public StageBase
    {
    public:
        explicit Transform(Queue<In>& input, Queue<Out>& output, QString id, QObject* parent = nullptr)
            : StageBase(id, parent),
            m_input(input),
            m_output(output) {
        }
        // derived classes must implement actual transformation logic
        virtual Out transform(const In& item) = 0;

        void run() override {
            In item;
            while (m_input.wait_and_pop(item)) {
                Out out_item = transform(item);
                m_output.push(std::move(out_item));
            }
        }

    protected:
        Queue<In>& m_input;
        Queue<Out>& m_output;

        // Inherited via StageBase
        void onStart() override
        {
            m_output.register_producer();
        }
        void onStop() override
        {
            m_output.producer_done();
        }
    };
}