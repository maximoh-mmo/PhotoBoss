#pragma once
namespace photoboss {
    template<typename In, typename Out>
    class TransformStage : public PipelineStage 
    {
    public:
        explicit TransformStage(Queue<In> &input, Queue<Out> &output, QObject* parent = nullptr)
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